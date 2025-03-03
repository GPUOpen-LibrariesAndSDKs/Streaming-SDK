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

#include "ControllerManager.h"
#include <memory>

namespace ssdk::ctls
{
    static constexpr const wchar_t* AMF_FACILITY = L"ControllerManager";

    //-------------------------------------------------------------------------------------------------
    ControllerManager::ControllerManager() :
        m_SensorsThread(this)
    {
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
        m_pCursor = nullptr;
    }

    //-------------------------------------------------------------------------------------------------
    void ControllerManager::SetClientTransport(ssdk::transport_common::ClientTransport::Ptr pClientTransport)
    {
        amf::AMFLock lock(&m_Guard);
        m_pClientTransport = pClientTransport;
    }

    //-------------------------------------------------------------------------------------------------
    void ControllerManager::SetCursor(MouseCursor::Ptr pCursor)
    {
        amf::AMFLock lock(&m_Guard);
        m_pCursor = pCursor;
    }

    //-------------------------------------------------------------------------------------------------
    bool ControllerManager::ProcessMessage(WindowMessage msg)
    {
        bool result = false;
        for (const ControllerBase::Ptr& pController : m_Controllers)
        {
            if (pController != nullptr)
            {
                if (pController->ProcessMessage(msg) == AMF_OK)
                {
                    result = true;
                }
            }
        }

        if (result == false && m_pCursor != nullptr)
        {
            result = m_pCursor->ProcessMessage(msg);
        }

        return result;
    }

    //-------------------------------------------------------------------------------------------------
    void ControllerManager::ReleaseModifiers()
    {
        for (const ControllerBase::Ptr& pController : m_Controllers)
        {
            if (pController != nullptr)
            {
                pController->ReleaseModifiers();
            }
        }
    }

    //-------------------------------------------------------------------------------------------------
    void ControllerManager::SendControllerEvent(const char* id, const ssdk::ctls::CtlEvent* pEvent)
    {
        if (m_pClientTransport != nullptr && m_bConnectionEstablished)
        {
            const amf::AMFVariant event((pEvent != nullptr) ? pEvent->value : amf::AMFVariant());
            m_pClientTransport->SendControllerEvent(id, event);
        }
    }

    //-------------------------------------------------------------------------------------------------
    void ControllerManager::OnControllerInputEvent(const char* controlID, const amf::AMFVariant& event)
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

        // Process input event from server, mouse move...
        if (pController != nullptr)
        {
            pController->ProcessInputEvent(deviceEv.c_str(), event);
        }

    }

    void ControllerManager::OnConnectionEstablished()
    {
        m_bConnectionEstablished = true;

        for (std::vector<ControllerBase::Ptr>::iterator it = m_Controllers.begin(); it != m_Controllers.end(); it++)
        {
            (*it)->OnConnectionEstablished();
        }
    }

    void ControllerManager::OnConnectionTerminated()
    {
        m_bConnectionEstablished = false;

        for (std::vector<ControllerBase::Ptr>::iterator it = m_Controllers.begin(); it != m_Controllers.end(); it++)
        {
            (*it)->OnConnectionTerminated();
        }
    }

    void ControllerManager::OnSensorsUpdate()
    {
        for (std::vector<ControllerBase::Ptr>::iterator it = m_Controllers.begin(); it != m_Controllers.end(); it++)
        {
            (*it)->UpdateSensors();
        }
    }

    void ControllerManager::Start()
    {
        OnConnectionEstablished();

        if (m_SensorsThread.IsRunning() == false)
        {
            m_SensorsThread.Start();
        }
    }

    void ControllerManager::Stop()
    {
        OnConnectionTerminated();

        m_SensorsThread.RequestStop();
        m_SensorsThread.WaitForStop();
    }

    void ControllerManager::SensorsThread::Run()
    {
        AMFTraceInfo(AMF_FACILITY, L"SensorsThread started");
        if (nullptr == m_pManager)
        {
            AMFTraceInfo(AMF_FACILITY, L"SensorsThread stopped prematurely becase controller manager is missing");
            return;
        }

        while (StopRequested() == false)
        {
            m_pManager->OnSensorsUpdate();
            amf_sleep(4);
        }

        m_pManager = nullptr;
        AMFTraceInfo(AMF_FACILITY, L"SensorsThread terminated");
    }

} // namespace ssdk::ctls
