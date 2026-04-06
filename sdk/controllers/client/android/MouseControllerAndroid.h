/*
Notice Regarding Standards.  AMD does not provide a license or sublicense to
any Intellectual Property Rights relating to any standards, including but not
limited to any audio and/or video codec technologies such as MPEG-2, MPEG-4;
AVC/H.264; HEVC/H.265; AAC decode/FFMPEG; AAC encode/FFMPEG; VC-1; and MP3
(collectively, the "Media Technologies"). For clarity, you will pay any
royalties due for such third party technologies, which may include the Media
Technologies that are owed as a result of AMD providing the Software to you.

This software uses libraries from the FFmpeg project under the LGPLv2.1.

MIT license

Copyright (c) Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#pragma once

#include "../MouseController.h"

#include "sdk/video/android/VideoReceiverAndroid.h"

#include "amf/public/common/ByteArray.h"

namespace ssdk::ctls
{
    class MouseControllerAndroid : public MouseController
    {
    public:
        typedef amf::AMFInterfacePtr_T<MouseControllerAndroid> Ptr;

    public:
        MouseControllerAndroid(ControllerManagerPtr pControllerManager, ssdk::video::VideoReceiver::Ptr pVideoPipeline);

        // ControllerBase interface

        virtual AMF_RESULT  AMF_STD_CALL ProcessMessage(const WindowMessage& msg) override;
        virtual void        AMF_STD_CALL ProcessInputEvent(const char* eventID, const amf::AMFVariant& event) override;

    protected:
        virtual void UpdateFromServer() override;

    private:
        AMF_RESULT HandleButtonPresses(int32_t buttonState);
        AMF_RESULT HandleWheel(int32_t hStep, int32_t vStep);
        AMF_RESULT HandleMove(float x, float y);

    private:
        ssdk::video::VideoReceiver::Ptr m_pVideoPipeline;
        int32_t                         m_LastButtonState = 0;
    };
}