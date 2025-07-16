// 
// Notice Regarding Standards.  AMD does not provide a license or sublicense to
// any Intellectual Property Rights relating to any standards, including but not
// limited to any audio and/or video codec technologies such as MPEG-2, MPEG-4;
// AVC/H.264; HEVC/H.265; AAC decode/FFMPEG; AAC encode/FFMPEG; VC-1; and MP3
// (collectively, the "Media Technologies"). For clarity, you will pay any
// royalties due for such third party technologies, which may include the Media
// Technologies that are owed as a result of AMD providing the Software to you.
// 
// MIT license 
// 
//
// Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#if defined(__linux)

#include "XDGRDPortalConnection.h"
#include "public/common/TraceAdapter.h"
#include <fcntl.h>

#include <systemd/sd-bus.h>
#include <iostream>
#include <stdexcept>
#include <string>

namespace ssdk::ctls
{
    static constexpr const wchar_t* const AMF_FACILITY = L"XDGRemoteDesktopPortalConnection";
    //-------------------------------------------------------------------------------------------------
    // the definitions for these methods can be found here:
    // https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.RemoteDesktop.html
    //-------------------------------------------------------------------------------------------------

    static const char* XDG_PORTAL_DESTINATION = "org.freedesktop.portal.Desktop";
    static const char* XDG_PORTAL_PATH = "/org/freedesktop/portal/desktop";
    static const char* XDG_PORTAL_REMOTEDESKTOP_INTERFACE = "org.freedesktop.portal.RemoteDesktop";
    static const char* XDG_PORTAL_REQUEST_INTERFACE = "org.freedesktop.portal.Request";
    static const char* XDG_PORTAL_CREATE_SESSION_METHOD = "CreateSession";
    static const char* XDG_PORTAL_SELECT_DEVICES_METHOD = "SelectDevices";
    static const char* XDG_PORTAL_START_METHOD = "Start";
    static const char* XDG_PORTAL_NOTIFYPOINTER_METHOD = "NotifyPointerMotion";
    static const char* XDG_PORTAL_NOTIFYPOINTER_ABSOLUTE_METHOD = "NotifyPointerMotionAbsolute";

    XDGRDPortalConnection::XDGRDPortalConnection()
    {}

    XDGRDPortalConnection::~XDGRDPortalConnection()
    {
        Terminate();
    }

    AMF_RESULT XDGRDPortalConnection::Initialize(DBusConnection* pDBus, const std::string& handleToken)
    {
        AMF_RETURN_IF_FALSE(pDBus != nullptr, AMF_FAIL, L"pDBus must not be null.");
        m_pDBus = pDBus;
        m_handleToken = handleToken;
        return AMF_OK;
    }

    AMF_RESULT XDGRDPortalConnection::Terminate()
    {
        m_pDBus = nullptr;
        m_handleToken = "";
        m_sessionHandle = "";
        return AMF_OK;
    }

    AMF_RESULT XDGRDPortalConnection::GetPropertyTrivial(const char* pProperty, char type, void* pData)
    {
        int r = m_pDBus->GetSDBus().m_pSDB_Get_Property_Trivial(
            m_pDBus->GetBus(),
            XDG_PORTAL_DESTINATION,
            XDG_PORTAL_PATH,
            XDG_PORTAL_REMOTEDESKTOP_INTERFACE,
            pProperty,
            nullptr,
            type,
            pData
        );
        return r >= 0 ? AMF_OK : AMF_FAIL;
    }

    AMF_RESULT XDGRDPortalConnection::AvailableDeviceTypes(SOURCE_TYPE& types)
    {
        AMF_RESULT res = GetPropertyTrivial("AvailableDeviceTypes", 'u', static_cast<void*>(&types));
        AMF_RETURN_IF_FAILED(res, L"AvailableDeviceTypes dbus call failed!");
        return AMF_OK;
    }

    static std::string GetExpectedHandle(const std::string& uniqueName, const std::string& handleToken)
    {
        std::string sender = uniqueName;
        std::replace(sender.begin(), sender.end(), '.', '_');
        if (sender[0] == ':')
        {
            sender.erase(0, 1);
        }
        return std::string("/org/freedesktop/portal/desktop/request/") + sender + "/" + handleToken;
    }

    AMF_RESULT XDGRDPortalConnection::CreateResponseSlot(SDBusSlotPtr& slot)
    {
        // setup listener for response signal from dbus
        std::string uniqueName;
        AMF_RETURN_IF_FAILED(m_pDBus->GetUniqueName(uniqueName));

        std::string expectedHandle = GetExpectedHandle(uniqueName, m_handleToken);

        AMF_RETURN_IF_FAILED(m_pDBus->CreateSlot(
            expectedHandle.c_str(),
            XDG_PORTAL_REQUEST_INTERFACE,
            "Response",
            slot
        ));
        return AMF_OK;
    }

    //-----------------------------------------------------------------------------------------
    AMF_RESULT XDGRDPortalConnection::CreateSession(const std::string& sessionHandleToken)
    {
        SDBusSlotPtr slot;
        AMF_RETURN_IF_FAILED(CreateResponseSlot(slot));

        // Construct the message to send to the CreateSession method of the remote desktop interface
        SDBusMessagePtr message;
        AMF_RETURN_IF_FAILED(m_pDBus->CreateMessage(
            XDG_PORTAL_DESTINATION,
            XDG_PORTAL_PATH,
            XDG_PORTAL_REMOTEDESKTOP_INTERFACE,
            XDG_PORTAL_CREATE_SESSION_METHOD,
            message
        ));

        AMF_RETURN_IF_FAILED(message.Append("a{sv}", 2,
            "handle_token", "s", m_handleToken.c_str(),
            "session_handle_token", "s", sessionHandleToken.c_str(),
            nullptr
        ));

        // Send the message and get the reply
        SDBusMessagePtr reply;
        sd_bus_error err;
        AMF_RETURN_IF_FAILED(m_pDBus->CallBus(message, reply, err));

        const char* pHandle;
        AMF_RETURN_IF_FAILED(reply.Read("o", &pHandle));
        AMF_RETURN_IF_FALSE(slot.GetPath() == pHandle, AMF_FAIL);

        // Wait for the response signal
        // the XDG desktop protocol is a litte weird in that you have
        // to explicitly wait for a separate reply message, different
        // from the message you just recieved from using CallBus
        AMF_RETURN_IF_FAILED(slot.WaitForMessage(100));
        SDBusMessagePtr responseMessage = slot.GetMessage();

        // Check response result, if nonzero then we have an error
        uint32_t result = 0;
        AMF_RETURN_IF_FAILED(responseMessage.Read("u", &result));
        AMF_RETURN_IF_FALSE(result == 0, AMF_FAIL, L"Response from request nonzero!");

        AMF_RETURN_IF_FAILED(responseMessage.EnterContainer('a', "{sv}"));

        while (responseMessage.EnterContainer('e', "sv") == AMF_OK)
        {
            std::string key, sig;
            AMF_RETURN_IF_FAILED(responseMessage.GetKeyAndSignature(key, sig));

            AMF_RETURN_IF_FAILED(responseMessage.EnterContainer('v', sig.c_str()));
            if (key == "session_handle")
            {
                const char* pSessionHandle;
                AMF_RETURN_IF_FAILED(responseMessage.Read(sig.c_str(), &pSessionHandle));
                m_sessionHandle = std::string(pSessionHandle);
            }
            else
            {
                AMF_RETURN_IF_FAILED(responseMessage.Skip(sig.c_str()))
            }
            // undo EnterContainer('v')
            AMF_RETURN_IF_FAILED(responseMessage.ExitContainer());

            // undo EnterContainer('sv')
            AMF_RETURN_IF_FAILED(responseMessage.ExitContainer());
        };
        // undo EnterContainer('{sv}')
        AMF_RETURN_IF_FAILED(responseMessage.ExitContainer());

        return AMF_OK;
    }

    //-----------------------------------------------------------------------------------------
    AMF_RESULT XDGRDPortalConnection::SelectDevices(SOURCE_TYPE sources, const std::string& restoreToken)
    {
        SDBusSlotPtr slot;
        AMF_RETURN_IF_FAILED(CreateResponseSlot(slot));

        // Construct the message to send to the SelectDevices method of the remote desktop interface
        SDBusMessagePtr message;
        AMF_RETURN_IF_FAILED(m_pDBus->CreateMessage(
            XDG_PORTAL_DESTINATION,
            XDG_PORTAL_PATH,
            XDG_PORTAL_REMOTEDESKTOP_INTERFACE,
            XDG_PORTAL_SELECT_DEVICES_METHOD,
            message
        ));

        AMF_RETURN_IF_FAILED(message.Append("o", m_sessionHandle.c_str()));

        // using MessageOpenContainer
        AMF_RETURN_IF_FAILED(message.OpenContainer('a', "{sv}"));

        AMF_RETURN_IF_FAILED(message.Append("{sv}",
            "handle_token", "s", m_handleToken.c_str()
        ));

        AMF_RETURN_IF_FAILED(message.Append("{sv}",
            "types", "u", sources
        ));

        if (restoreToken != "")
        {
            AMF_RETURN_IF_FAILED(message.Append("{sv}",
                "restore_token", "s", restoreToken.c_str()
            ));
        }

        AMF_RETURN_IF_FAILED(message.CloseContainer());

        // Send the message and get the reply
        SDBusMessagePtr reply;
        sd_bus_error err;
        AMF_RETURN_IF_FAILED(m_pDBus->CallBus(message, reply, err));
        const char* pHandle;

        AMF_RETURN_IF_FAILED(reply.Read("o", &pHandle));

        AMF_RETURN_IF_FALSE(slot.GetPath() == pHandle, AMF_FAIL);

        // Wait for the response signal
        AMF_RETURN_IF_FAILED(slot.WaitForMessage(100));
        SDBusMessagePtr responseMessage = slot.GetMessage();

        // Check response result, if nonzero then we have an error
        uint32_t result = 0;
        AMF_RETURN_IF_FAILED(responseMessage.Read("u", &result));
        AMF_RETURN_IF_FALSE(result == 0, AMF_FAIL, L"Response from request nonzero!");

        return AMF_OK;
    }

    //-----------------------------------------------------------------------------------------
    AMF_RESULT XDGRDPortalConnection::Start(const std::string& parentWindow, const AbortController& controller)
    {
        SDBusSlotPtr slot;
        AMF_RETURN_IF_FAILED(CreateResponseSlot(slot));

        // Construct the message to send to the Start method of the remote desktop interface
        SDBusMessagePtr message;
        AMF_RETURN_IF_FAILED(m_pDBus->CreateMessage(
            XDG_PORTAL_DESTINATION,
            XDG_PORTAL_PATH,
            XDG_PORTAL_REMOTEDESKTOP_INTERFACE,
            XDG_PORTAL_START_METHOD,
            message
        ));

        AMF_RETURN_IF_FAILED(message.Append("o", m_sessionHandle.c_str()));
        AMF_RETURN_IF_FAILED(message.Append("s", parentWindow.c_str()));

        AMF_RETURN_IF_FAILED(message.Append("a{sv}", 1,
            "handle_token", "s", m_handleToken.c_str(),
            nullptr
        ));

        // Send the message and get the reply
        SDBusMessagePtr reply;
        sd_bus_error err;
        AMF_RETURN_IF_FAILED(m_pDBus->CallBus(message, reply, err));

        const char* pHandle;
        AMF_RETURN_IF_FAILED(reply.Read("o", &pHandle));
        AMF_RETURN_IF_FALSE(slot.GetPath() == pHandle, AMF_FAIL);

        // Wait for the response signal
        AMF_RETURN_IF_FAILED(slot.WaitForMessage(-1, &controller));
        SDBusMessagePtr responseMessage = slot.GetMessage();

        // Check response result, if nonzero then we have an error
        uint32_t result = 0;
        AMF_RETURN_IF_FAILED(responseMessage.Read("u", &result));
        AMF_RETURN_IF_FALSE(result == 0, AMF_FAIL, L"Response from request nonzero!");

        return AMF_OK;
    }

    //-----------------------------------------------------------------------------------------
    AMF_RESULT XDGRDPortalConnection::NotifyPointerMotion(double dx, double dy)
    {
        // Construct the message to send to the NotifyPointerMotion method of the remote desktop interface
        SDBusMessagePtr message;
        AMF_RETURN_IF_FAILED(m_pDBus->CreateMessage(
            XDG_PORTAL_DESTINATION,
            XDG_PORTAL_PATH,
            XDG_PORTAL_REMOTEDESKTOP_INTERFACE,
            XDG_PORTAL_NOTIFYPOINTER_METHOD,
            message
        ));

        // IN session_handle o
        AMF_RETURN_IF_FAILED(message.Append("o", m_sessionHandle.c_str()));

        // IN options a{sv}
        AMF_RETURN_IF_FAILED(message.Append("a{sv}", 1,
            "handle_token", "s", m_handleToken.c_str(),
            nullptr
        ));

        // IN x dx, IN y dx - Mouse cursor relative movement
        AMF_RETURN_IF_FAILED(message.Append("d", dx));
        AMF_RETURN_IF_FAILED(message.Append("d", dy));

        // Send the message and get the reply
        SDBusMessagePtr reply;
        sd_bus_error err;
        AMF_RETURN_IF_FAILED(m_pDBus->CallBus(message, reply, err));

        return AMF_OK;
    }

    //-----------------------------------------------------------------------------------------
    AMF_RESULT XDGRDPortalConnection::NotifyPointerMotionAbsolute(double x, double y)
    {
        // Construct the message to send to the NotifyPointerMotionAbsolute method of the remote desktop interface
        SDBusMessagePtr message;
        AMF_RETURN_IF_FAILED(m_pDBus->CreateMessage(
            XDG_PORTAL_DESTINATION,
            XDG_PORTAL_PATH,
            XDG_PORTAL_REMOTEDESKTOP_INTERFACE,
            XDG_PORTAL_NOTIFYPOINTER_ABSOLUTE_METHOD,
            message
        ));

        // IN session_handle o
        AMF_RETURN_IF_FAILED(message.Append("o", m_sessionHandle.c_str()));

        // IN options a{sv}
        AMF_RETURN_IF_FAILED(message.Append("a{sv}", 1,
            "handle_token", "s", m_handleToken.c_str(),
            nullptr
        ));

        // IN stream u - Passing 0 indicates that the pointer motion is not tied to any specific PipeWire stream
        // In this case the pointer motion to apply to the primary screen
        AMF_RETURN_IF_FAILED(message.Append("u", 0));

        // IN x d, IN y d - Mouse cursor coordinates
        AMF_RETURN_IF_FAILED(message.Append("d", x));
        AMF_RETURN_IF_FAILED(message.Append("d", y));

        // Send the message and get the reply
        SDBusMessagePtr reply;
        sd_bus_error err;
        AMF_RETURN_IF_FAILED(m_pDBus->CallBus(message, reply, err));

        return AMF_OK;
    }
    //-----------------------------------------------------------------------------------------

} // namespace ssdk::ctls

#endif // __linux
