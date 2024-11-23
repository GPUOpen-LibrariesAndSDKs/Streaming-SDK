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
#pragma once

#include "ControllerBase.h"
#include "MouseController.h"
#include "KeyboardController.h"
#include "GameController.h"
#include "TouchEvent.h"
#include "TrackedHeadset.h"
#include "transports/transport-common/ClientTransport.h"
#include "MouseCursor.h"

namespace ssdk::ctls
{
    //----------------------------------------------------------------------------------------------
    // ControllerManager: VR controllers, game controllers, keyboard, mouse, touchscreen
    //----------------------------------------------------------------------------------------------
    class ControllerManager :
        public ssdk::transport_common::ClientTransport::InputControllerCallback
    {
    public:
        ControllerManager();
        virtual ~ControllerManager();

        typedef std::shared_ptr<ControllerManager> Ptr;
        
        void AddController(ControllerBase::Ptr pController);
        bool RemoveController(ControllerBase::Ptr pController);

        bool ProcessMessage(WindowMessage msg);

        void SetClientTransport(ssdk::transport_common::ClientTransport::Ptr pClientTransport);
        ssdk::transport_common::ClientTransport::Ptr GetClientTransport() { return m_pClientTransport; };

        void SetCursor(MouseCursor::Ptr pCursor);
        MouseCursor::Ptr GetCursor() { return m_pCursor; };

        void SendControllerEvent(const char* id, const ssdk::ctls::CtlEvent* pEvent);

        // InputControllerCallback interface
        virtual void OnControllerEnabled(const char* /*deviceID*/, ssdk::transport_common::ControllerType /*type*/) override {};
        virtual void OnControllerDisabled(const char* /*deviceID*/) override {};
        virtual void OnControllerInputEvent(const char* controlID, const amf::AMFVariant& event) override;

        void TerminateControllers();

        void Start();
        void Stop();

        void OnConnectionEstablished();
        void OnConnectionTerminated();
        void OnSensorsUpdate();

        class SensorsThread : public amf::AMFThread
        {
        public:
            SensorsThread(ControllerManager* pManager) : m_pManager(pManager) {}
            virtual void Run();

        protected:
            ControllerManager* m_pManager = nullptr;
        };

    protected:
        mutable amf::AMFCriticalSection                 m_Guard;
        std::vector<ControllerBase::Ptr>                m_Controllers;
        MouseCursor::Ptr                                m_pCursor;
        ssdk::transport_common::ClientTransport::Ptr    m_pClientTransport;
        SensorsThread                                   m_SensorsThread;
        bool                                            m_bConnectionEstablished = false;
    };

} // namespace ssdk::ctls
