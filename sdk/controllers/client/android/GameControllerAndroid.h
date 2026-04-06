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
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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

#if defined(__ANDROID__)

#include "../ControllerBase.h"
#include "sdk/util/jni/JavaObjectHolder.h"

#include <android/input.h>

namespace ssdk::ctls
{
    class GameController : public ControllerBase
    {
    public:
        enum CONNECTION_TYPE
        {
            USB = 0,
            BLUETOOTH
        };

    public:
        GameController(ControllerManagerPtr pControllerManager, ssdk::util::JavaObjectHolder& activity, int iIndex);

        virtual AMF_RESULT  AMF_STD_CALL ProcessMessage(const WindowMessage& msg) override;

        // Input interface;
        virtual CTRL_DEVICE_TYPE AMF_STD_CALL GetType() const override { return CTRL_DEVICE_TYPE::GAME_CONTROLLER; }
        virtual AMF_RESULT  AMF_STD_CALL Update() override { return AMF_OK; }; // inputs handled by ProcessButton and ProcessAxis
        // ControllerBase interface
        virtual void        AMF_STD_CALL ProcessInputEvent(const char* eventID, const amf::AMFVariant& event) override;

        int                     m_iIndex;
        std::map<int, const char*>      m_eventMap;
        CONNECTION_TYPE         m_connectionType = BLUETOOTH;

    protected:
        enum class ANALOG_1D
        {
            LEFT_TRIGGER,
            RIGHT_TRIGGER
        };

        enum class ANALOG_2D
        {
            LEFT_JOYSTICK,
            RIGHT_JOYSTICK,
        };

        void ProcessButton(int keyCode, bool isPressed);
        void ProcessAnalogControls1D(size_t index, ANALOG_1D control, const WindowMessage& msg);
        void ProcessAnalogControls2D(size_t index, ANALOG_2D control, const WindowMessage& msg);
        void ProcessDpad(size_t index, const WindowMessage& msg);


        void AMF_STD_CALL FireEvent(const char* id, CtlEvent& ev);

        mutable amf::AMFCriticalSection m_Guard;

/////////////////////////////////////////////////////////
        struct ControlState
        {
            amf_int32 hat0x = 0;
            amf_int32 hat0y = 0;
            float absX = 0;
            float absY = 0;
            float absZ = 0;
            float absRZ = 0;
            float absLeftTrigger = 0;
            float absRightTrigger = 0;
        };
        ControlState m_State = {};

        struct Vibration
        {
            Vibration() : ptsStart(0), ptsDuration(0) {}
            amf_pts ptsStart;
            amf_pts ptsDuration;
        };

        ssdk::util::JavaObjectHolder& m_Activity;
    };
}// namespace ssdk::ctls
#endif
