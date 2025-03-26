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

#include "ControllerManagerSvr.h"
#include "amf/public/common/TraceAdapter.h"
#include <memory>

namespace ssdk::ctls::svr
{
    static constexpr const wchar_t* AMF_FACILITY = L"ControllerManagerSvr";

    //-------------------------------------------------------------------------------------------------
    ControllerManager::ControllerManager() :
        m_UpdateThread(this)
    {
        m_pAMFXInputCreateController = (AMFXInputCreateController_Fn)amf_get_proc_address(g_AMFFactory.GetAMFDLLHandle(), AMF_XINPUT_CREATE_CONTROLLER_FUNCTION_NAME);
    }
    ControllerManager::~ControllerManager()
    {
        TerminateControllers();
    }

    //-------------------------------------------------------------------------------------------------
    void ControllerManager::AddController(ControllerBase::Ptr pController)
    {
        amf::AMFLock lock(&m_Guard);
        if (pController != nullptr)
        {
            m_Controllers.push_back(pController);
        }
    }

    bool ControllerManager::RemoveController(ControllerBase::Ptr pController)
    {
        amf::AMFLock lock(&m_Guard);

        bool bRemoved = false;
        for (std::vector<ControllerBase::Ptr>::iterator it = m_Controllers.begin(); it != m_Controllers.end(); it++)
        {
            if (pController == *it)
            {
                m_Controllers.erase(it);
                bRemoved = true;
                break;
            }
        }

        return bRemoved;
    }

    void ControllerManager::TerminateControllers()
    {
        Stop();
        m_Controllers.clear();
        m_pAMFXInputCreateController = nullptr;
    }

    //-------------------------------------------------------------------------------------------------
    void ControllerManager::SetServerTransport(ssdk::transport_common::ServerTransport::Ptr pServerTransport)
    {
        amf::AMFLock lock(&m_Guard);
        m_pServerTransport = pServerTransport;
    }

    //-------------------------------------------------------------------------------------------------
    void ControllerManager::SendControllerEvent(const char* id, const ssdk::ctls::CtlEvent* pEvent)
    {
        if (m_pServerTransport != nullptr)
        {
            m_pServerTransport->SendControllerEvent(id, pEvent);
        }
    }

    //-------------------------------------------------------------------------------------------------
    void ControllerManager::OnControllerInputEvent(ssdk::transport_common::SessionHandle /*session*/, const char* controlID, const ssdk::ctls::CtlEvent& event)
    {
        // Parse controlID and find out device and event type
        /*------------------------------------------------------------------------------------------------
        Controler IDs
        DEVICE_MOUSE("/mouse") + DEVICE_MOUSE_POS("/in/pos") = "/mouse/in/pos"
        DEVICE_MOUSE("/mouse") + DEVICE_MOUSE_L("/in/l") = "/mouse/in/l"
        DEVICE_MOUSE("/mouse") + DEVICE_MOUSE_R("/in/r") = "/mouse/in/r"
        DEVICE_MOUSE("/mouse") + DEVICE_MOUSE_M("/in/m") = "/mouse/in/m"
        --------------------------------------------------------------------------------------------------
        Event IDs
        DEVICE_MOUSE("/mouse") + DEVICE_MOUSE_POS("/in/pos") = "/mouse/in/pos"
        DEVICE_MOUSE("/mouse") + DEVICE_MOUSE_L_CLICK("/in/l/click") = "/mouse/in/l/click"
        DEVICE_MOUSE("/mouse") + DEVICE_MOUSE_R_CLICK("/in/r/click") = "/mouse/in/r/click"
        DEVICE_MOUSE("/mouse") + DEVICE_MOUSE_M_CLICK("/in/m/click") = "/mouse/in/m/click"
        DEVICE_MOUSE("/mouse") + DEVICE_MOUSE_M_TRACK("/in/m/track") = "/mouse/in/m/track"
        ------------------------------------------------------------------------------------------------*/
        std::string ctrlID(controlID);
        std::string::size_type pos = ctrlID.find('/', 1);
        if (pos == std::string::npos)
        {
            return;
        }
        std::string deviceID(ctrlID.substr(0, pos));
        std::string deviceEv(ctrlID.substr(pos));

        // Get controller by ID
        ControllerBase::Ptr pController;
        for (std::vector<ControllerBase::Ptr>::iterator it = m_Controllers.begin(); it != m_Controllers.end(); it++)
        {
            if (std::string((*it)->GetID()) == deviceID)
            {
                pController = *it;
                break;
            }
        }

        // Process input event from client, mouse move...
        if (pController != nullptr)
        {
            pController->ProcessInputEvent(deviceEv.c_str(), event);
        }

    }

    //-------------------------------------------------------------------------------------------------
    amf::AMF_VARIANT_TYPE ControllerManager::GetExpectedEventDataType(const char* controlID)
    {
        amf::AMF_VARIANT_TYPE type;
        std::string ctrlID(controlID);
        std::string::size_type pos = ctrlID.find('/', 1);
        if (pos == std::string::npos)
        {
            type = amf::AMF_VARIANT_TYPE::AMF_VARIANT_EMPTY;
        }
        else
        {
            //std::string deviceID(ctrlID.substr(0, pos));
            std::string deviceEv(ctrlID.substr(pos));

            type = FindTypeByID(deviceEv.c_str());
        }

        return type;
    }

    //--------------------------------------------------------------------------------------------------------------------------------
    amf::AMF_VARIANT_TYPE ControllerManager::FindTypeByID(const std::string& id) const
    {
        struct TypeMap
        {
            const char* name;
            amf::AMF_VARIANT_TYPE type;
        };
        static TypeMap typeMap[] =
        {
            { DEVICE_POSE                    ,   amf::AMF_VARIANT_EMPTY },
            { DEVICE_BATTERY                 ,   amf::AMF_VARIANT_FLOAT },
            { DEVICE_CTRL_TRIGGER_CLICK      ,   amf::AMF_VARIANT_BOOL },
            { DEVICE_CTRL_TRIGGER_TOUCH      ,   amf::AMF_VARIANT_BOOL },
            { DEVICE_CTRL_TRIGGER_VALUE      ,   amf::AMF_VARIANT_FLOAT },
            { DEVICE_CTRL_SYSTEM_CLICK       ,   amf::AMF_VARIANT_BOOL },
            { DEVICE_CTRL_APPMENU_CLICK      ,   amf::AMF_VARIANT_BOOL },
            { DEVICE_CTRL_GRIP_CLICK         ,   amf::AMF_VARIANT_BOOL },
            { DEVICE_CTRL_GRIP_VALUE         ,   amf::AMF_VARIANT_FLOAT },
            { DEVICE_CTRL_JOYSTICK_VALUE     ,   amf::AMF_VARIANT_FLOAT_POINT2D },
            { DEVICE_CTRL_JOYSTICK_CLICK     ,   amf::AMF_VARIANT_BOOL },
            { DEVICE_CTRL_JOYSTICK_TOUCH     ,   amf::AMF_VARIANT_BOOL },
            { DEVICE_CTRL_TRACKPAD_VALUE     ,   amf::AMF_VARIANT_FLOAT_POINT2D },
            { DEVICE_CTRL_TRACKPAD_CLICK     ,   amf::AMF_VARIANT_BOOL },
            { DEVICE_CTRL_TRACKPAD_TOUCH     ,   amf::AMF_VARIANT_BOOL },
            { DEVICE_CTRL_VOLUME_UP_CLICK    ,   amf::AMF_VARIANT_BOOL },
            { DEVICE_CTRL_VOLUME_DOWN_CLICK  ,   amf::AMF_VARIANT_BOOL },
            { DEVICE_CTRL_DPAD_UP_CLICK      ,   amf::AMF_VARIANT_BOOL },
            { DEVICE_CTRL_DPAD_DOWN_CLICK    ,   amf::AMF_VARIANT_BOOL },
            { DEVICE_CTRL_DPAD_LEFT_CLICK    ,   amf::AMF_VARIANT_BOOL },
            { DEVICE_CTRL_DPAD_RIGHT_CLICK   ,   amf::AMF_VARIANT_BOOL },
            { DEVICE_CTRL_X_CLICK            ,   amf::AMF_VARIANT_BOOL },
            { DEVICE_CTRL_X_TOUCH            ,   amf::AMF_VARIANT_BOOL },
            { DEVICE_CTRL_Y_CLICK            ,   amf::AMF_VARIANT_BOOL },
            { DEVICE_CTRL_Y_TOUCH            ,   amf::AMF_VARIANT_BOOL },
            { DEVICE_CTRL_A_CLICK            ,   amf::AMF_VARIANT_BOOL },
            { DEVICE_CTRL_A_TOUCH            ,   amf::AMF_VARIANT_BOOL },
            { DEVICE_CTRL_B_CLICK            ,   amf::AMF_VARIANT_BOOL },
            { DEVICE_CTRL_B_TOUCH            ,   amf::AMF_VARIANT_BOOL },
            { DEVICE_CTRL_HAPTIC             ,   amf::AMF_VARIANT_FLOAT_POINT3D },
            // mouse
            { DEVICE_MOUSE_POS               ,   amf::AMF_VARIANT_FLOAT_POINT2D },
            { DEVICE_MOUSE_L_CLICK           ,   amf::AMF_VARIANT_BOOL },
            { DEVICE_MOUSE_R_CLICK           ,   amf::AMF_VARIANT_BOOL },
            { DEVICE_MOUSE_M_CLICK           ,   amf::AMF_VARIANT_BOOL },
            { DEVICE_MOUSE_M_TRACK           ,   amf::AMF_VARIANT_INT64 },
            // keyboard
            { DEVICE_KEYBOARD_KEYS           ,   amf::AMF_VARIANT_INT64 },
            { DEVICE_KEYBOARD_CHARACTER      ,   amf::AMF_VARIANT_INT64 },
            // touchscreen
            { DEVICE_TOUCHSCREEN_TOUCH       ,   amf::AMF_VARIANT_INTERFACE },
            // game
            { DEVICE_CTRL_TRIGGER_LEFT_VALUE   ,   amf::AMF_VARIANT_FLOAT },
            { DEVICE_CTRL_TRIGGER_RIGHT_VALUE  ,   amf::AMF_VARIANT_FLOAT },
            { DEVICE_CTRL_JOYSTICK_LEFT_VALUE  ,   amf::AMF_VARIANT_FLOAT_POINT2D },
            { DEVICE_CTRL_JOYSTICK_LEFT_CLICK  ,   amf::AMF_VARIANT_BOOL },
            { DEVICE_CTRL_JOYSTICK_RIGHT_VALUE ,   amf::AMF_VARIANT_FLOAT_POINT2D },
            { DEVICE_CTRL_JOYSTICK_RIGHT_CLICK ,   amf::AMF_VARIANT_BOOL },
            { DEVICE_CTRL_GRIP_LEFT_CLICK      ,   amf::AMF_VARIANT_BOOL },
            { DEVICE_CTRL_GRIP_RIGHT_CLICK     ,   amf::AMF_VARIANT_BOOL },
            { DEVICE_CTRL_HAPTIC_STATE         ,   amf::AMF_VARIANT_FLOAT_POINT2D },
        };

        for (int i = 0; i < amf_countof(typeMap); i++)
        {
            if (id == typeMap[i].name)
            {
                return typeMap[i].type;
            }
        }

        return amf::AMF_VARIANT_EMPTY;
    }

    void ControllerManager::OnConnectionEstablished()
    {
        amf::AMFLock lock(&m_Guard);
        for (std::vector<ControllerBase::Ptr>::iterator it = m_Controllers.begin(); it != m_Controllers.end(); it++)
        {
            (*it)->OnConnectionEstablished();
        }
    }

    void ControllerManager::OnConnectionTerminated()
    {
        amf::AMFLock lock(&m_Guard);
        for (std::vector<ControllerBase::Ptr>::iterator it = m_Controllers.begin(); it != m_Controllers.end(); it++)
        {
            (*it)->OnConnectionTerminated();
        }
    }

    void ControllerManager::Update()
    {
        amf::AMFLock lock(&m_Guard);
        for (std::vector<ControllerBase::Ptr>::iterator it = m_Controllers.begin(); it != m_Controllers.end(); it++)
        {
            (*it)->Update();
        }
    }

    void ControllerManager::Start()
    {
        OnConnectionEstablished();

        if (m_UpdateThread.IsRunning() == false)
        {
            m_UpdateThread.Start();
        }

        m_bStarted = true;
    }

    void ControllerManager::Stop()
    {
        OnConnectionTerminated();

        m_UpdateThread.Stop();
        m_bStarted = false;
    }

    void ControllerManager::UpdateThread::Run()
    {
        AMFTraceInfo(AMF_FACILITY, L"UpdateThread started");
        if (nullptr == m_pManager)
        {
            AMFTraceInfo(AMF_FACILITY, L"UpdateThread stopped prematurely becase controller manager is missing");
            return;
        }

        while (StopRequested() == false)
        {
            m_pManager->Update();
            amf_sleep(1);
        }

        AMFTraceInfo(AMF_FACILITY, L"UpdateThread terminated");
    }

    void ControllerManager::UpdateThread::Stop()
    {
        RequestStop();
        WaitForStop();
    }

} // namespace ssdk::ctls::svr
