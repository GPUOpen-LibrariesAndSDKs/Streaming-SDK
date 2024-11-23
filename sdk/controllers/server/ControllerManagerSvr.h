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

#include "ControllerBaseSvr.h"
#include "transports/transport-common/ServerTransport.h"
#include "amf/public/include/components/CursorCapture.h"
#include "amf/public/include/components/AMFXInput.h"

namespace ssdk::ctls::svr
{
    //----------------------------------------------------------------------------------------------
    // ControllerManager: VR controllers, game controllers, keyboard, mouse, touchscreen
    //----------------------------------------------------------------------------------------------
    class ControllerManager :
        public ssdk::transport_common::ServerTransport::InputControllerCallback
    {
    public:
        ControllerManager();
        virtual ~ControllerManager();

        typedef std::shared_ptr<ControllerManager> Ptr;

        void AddController(ControllerBase::Ptr pController);
        bool RemoveController(ControllerBase::Ptr pController);

        void SetServerTransport(ssdk::transport_common::ServerTransport::Ptr pServerTransport);
        ssdk::transport_common::ServerTransport::Ptr GetServerTransport() { return m_pServerTransport; };

        amf::AMFCursorCapturePtr GetCursorCapture() { return m_pCursorCapture; };
        void SetCursorCapture(amf::AMFCursorCapturePtr pCursorCapture) { m_pCursorCapture = pCursorCapture; };

        void SendControllerEvent(const char* id, const ssdk::ctls::CtlEvent* pEvent);

        // InputControllerCallback interface
        virtual void OnControllerEnabled(ssdk::transport_common::SessionHandle /*session*/, const char* /*deviceID*/, ssdk::transport_common::ControllerType /*type*/) override {};
        virtual void OnControllerDisabled(ssdk::transport_common::SessionHandle /*session*/, const char* /*deviceID*/) override {};
        virtual void OnControllerInputEvent(ssdk::transport_common::SessionHandle session, const char* controlID, const ssdk::ctls::CtlEvent& event) override;
        virtual amf::AMF_VARIANT_TYPE GetExpectedEventDataType(const char* controlID) override;
        virtual void OnTrackableDevicePoseChange(ssdk::transport_common::SessionHandle /*session*/, const char* /*deviceID*/, const ssdk::transport_common::Pose& /*pose*/) override {};

        void TerminateControllers();

        void Start();
        void Stop();
        bool Started() const { return m_bStarted; };

        void OnConnectionEstablished();
        void OnConnectionTerminated();
        
        void Update();

        class UpdateThread : public amf::AMFThread
        {
        public:
            UpdateThread(ControllerManager* pManager) : m_pManager(pManager) {}
            virtual void Run();
            void Stop();

        protected:
            ControllerManager* m_pManager = nullptr;
        };

        AMFXInputCreateController_Fn& GetAMFXInputCreateController() { return m_pAMFXInputCreateController; };

    protected:
        amf::AMF_VARIANT_TYPE FindTypeByID(const std::string& id) const;

    protected:
        amf::AMFCriticalSection                      m_Guard;
        std::vector<ControllerBase::Ptr>             m_Controllers;
        ssdk::transport_common::ServerTransport::Ptr m_pServerTransport;
        amf::AMFCursorCapturePtr                     m_pCursorCapture;
        bool                                         m_bStarted{ false };
        UpdateThread                                 m_UpdateThread;
        AMFXInputCreateController_Fn                 m_pAMFXInputCreateController{ nullptr };
    };

} // namespace ssdk::ctls::svr
