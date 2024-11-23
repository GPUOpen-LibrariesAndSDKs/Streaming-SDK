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

#if defined(__linux)

#include "../ControllerBase.h"

namespace ssdk::ctls
{
    //----------------------------------------------------------------------------------------------
    class GameController : public ControllerBase
    {
    public:
        enum CONNECTION_TYPE
        {
            USB = 0,
            BLUETOOTH
        };

        GameController(ControllerManagerPtr pControllerManager, libevdev* dev, uint32_t index, CONNECTION_TYPE connection = CONNECTION_TYPE::BLUETOOTH);
        ~GameController();
        typedef amf::AMFInterfacePtr_T<GameController> Ptr;

        // Input interface;
        virtual CTRL_DEVICE_TYPE AMF_STD_CALL GetType() const override { return CTRL_DEVICE_TYPE::GAME_CONTROLLER; }
        virtual AMF_RESULT AMF_STD_CALL Update() override;
        AMF_RESULT mapEvents();

        // ControllerBase interface
        virtual void    AMF_STD_CALL ProcessInputEvent(const char* eventID, const amf::AMFVariant& event) override;
        virtual void    AMF_STD_CALL OnConnectionEstablished() override;
        virtual void    AMF_STD_CALL OnConnectionTerminated() override;
        virtual void    AMF_STD_CALL UpdateSensors() override;

        uint32_t            m_iIndex;
        struct libevdev*    m_device;
        std::map<__u16, __u16>  m_eventMap;
        int                     m_xAxisShift = 32768;
        int                     m_yAxisShift = 32767;
        int                     m_rumble_effect = -1;
        CONNECTION_TYPE         m_connectionType = BLUETOOTH;
    protected:
        void AMF_STD_CALL FireEvent(const char* id, CtlEvent* ev);

        amf::AMFCriticalSection m_Guard;
        struct DEVICE_STATE
        {
            int hat0x = 0;
            int hat0y = 0;
            int absZ = 0;
            int absRZ = 0;
            int absX = 0;
            int absY = 0;
            bool updated = false;
        };
        DEVICE_STATE m_State;

        struct Vibration
        {
            Vibration() : ptsStart(0), ptsDuration(0) {}
            amf_pts ptsStart;
            amf_pts ptsDuration;
        };
    };
}// namespace ssdk::ctls
#endif
