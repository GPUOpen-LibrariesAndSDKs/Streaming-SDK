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

#include "DBusConnection.h"
#include "public/common/TraceAdapter.h"

namespace ssdk::ctls
{
    static constexpr const wchar_t* const AMF_FACILITY = L"DBusConnection";
    //-------------------------------------------------------------------------------------------------
    DBusConnection::DBusConnection(DBUS_BUS_TYPE busType)
        : m_busType(busType)
    {}

    DBusConnection::~DBusConnection()
    {
        Terminate();
    }

    AMF_RESULT DBusConnection::Initialize()
    {
        if (m_pBus != nullptr) {
            Terminate();
        }

        AMF_RETURN_IF_FAILED(m_sd.LoadFunctionsTable(), L"Couldn't load sd_bus functions.");

        int r = (m_busType == DBUS_BUS_TYPE::SYSTEM) ? m_sd.m_pSDB_Open_System(&m_pBus) : m_sd.m_pSDB_Open_User(&m_pBus);
        AMF_RETURN_IF_FALSE((r == 0) && (m_pBus != nullptr), AMF_FAIL, L"Couldn't open dbus.");

        return AMF_OK;
    }

    AMF_RESULT DBusConnection::Terminate()
    {
        if (m_sd.m_pSDB_Unref != nullptr && m_pBus != nullptr) {
            m_sd.m_pSDB_Unref(m_pBus);
        }
        m_pBus = nullptr;

        return AMF_OK;
    }

    AMF_RESULT DBusConnection::GetUniqueName(std::string& uniqueName)
    {
        const char* pUniqueName = nullptr;
        int r = m_sd.m_pSDB_Get_Unique_Name(m_pBus, &pUniqueName);
        AMF_RETURN_IF_FALSE(r >= 0, AMF_FAIL, L"Call to dbus failed! err=%d", r);
        uniqueName = std::string(pUniqueName);
        return AMF_OK;
    }

    AMF_RESULT DBusConnection::CallBus(const SDBusMessagePtr& request, SDBusMessagePtr& reply, sd_bus_error& error)
    {
        sd_bus_message* pReply = nullptr;
        error = SD_BUS_ERROR_NULL;
        int r = m_sd.m_pSDB_Call(m_pBus, request.Get(), -1, &error, &pReply);
        if (pReply != nullptr)
        {
            reply = std::move(SDBusMessagePtr(&m_sd, pReply));
        }
        AMF_RETURN_IF_FALSE(r >= 0, AMF_FAIL, L"Call to dbus failed! err=%d", r);
        return AMF_OK;
    }

    AMF_RESULT DBusConnection::CreateMessage(const char* pDestination, const char* pPath, const char* pInterface, const char* pMember, SDBusMessagePtr& message)
    {
        sd_bus_message *pMessage = nullptr;
        int r = m_sd.m_pSDB_Message_New_Method_Call(m_pBus, &pMessage, pDestination, pPath, pInterface, pMember);
        AMF_RETURN_IF_FALSE(r >= 0, AMF_FAIL, L"Couldn't create method call! err=%d", r);
        message = std::move(SDBusMessagePtr(&m_sd, pMessage));
        return AMF_OK;
    }

    static int match_signal_cb(sd_bus_message *pMessage, void *pUserData, sd_bus_error*)
    {
        if (pUserData != nullptr)
        {
            static_cast<SDBusSlotPtr*>(pUserData)->OnSlotTriggered(pMessage);
        }
        return 0;
    }

    AMF_RESULT DBusConnection::CreateSlot(const char* pPath, const char* pInterface, const char* pMember, SDBusSlotPtr& slot)
    {
        slot.m_pSd = &m_sd;
        slot.m_pBus = m_pBus;
        slot.m_path = pPath;
        sd_bus_slot* pSlot = nullptr;
        int r = m_sd.m_pSDB_Match_Signal(
            m_pBus,
            &pSlot,
            NULL,
            pPath,
            pInterface,
            pMember,
            match_signal_cb,
            &slot
        );
        AMF_RETURN_IF_FALSE(r >= 0, AMF_FAIL, L"Couldn't create slot! err=%d", r);
        slot.m_pSlot = pSlot;
        return AMF_OK;
    }

    const SDBusImportTable& DBusConnection::GetSDBus() const
    {
        return m_sd;
    }

    sd_bus* DBusConnection::GetBus() const
    {
        return m_pBus;
    }
    //-----------------------------------------------------------------------------------------

} // namespace ssdk::ctls

#endif // __linux
