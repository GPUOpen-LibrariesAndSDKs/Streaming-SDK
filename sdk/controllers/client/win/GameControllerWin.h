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

#if defined(_WIN32)

#include "../ControllerBase.h"

namespace ssdk::ctls
{
    class GameController : public ControllerBase
    {
    public:
        GameController(ControllerManagerPtr pControllerManager, DWORD iIndex);
        ~GameController();
        typedef amf::AMFInterfacePtr_T<GameController> Ptr;

        // Input interface;
        virtual CTRL_DEVICE_TYPE AMF_STD_CALL GetType() const override { return CTRL_DEVICE_TYPE::GAME_CONTROLLER; }
        virtual AMF_RESULT  AMF_STD_CALL Update() override;

        // ControllerBase interface
        virtual void        AMF_STD_CALL ProcessInputEvent(const char* eventID, const amf::AMFVariant& event) override;
        virtual void        AMF_STD_CALL OnConnectionEstablished() override;
        virtual void        AMF_STD_CALL OnConnectionTerminated() override;
        virtual void        AMF_STD_CALL UpdateSensors() override;

        DWORD                   m_iIndex;
    protected:
        void AMF_STD_CALL FireEvent(const char* id, CtlEvent* ev);

        amf::AMFCriticalSection m_Guard;
        XINPUT_STATE            m_State;

        struct Vibration
        {
            Vibration() : ptsStart(0), ptsDuration(0) {}
            amf_pts ptsStart;
            amf_pts ptsDuration;
        };
    };
}// namespace ssdk::ctls
#endif
