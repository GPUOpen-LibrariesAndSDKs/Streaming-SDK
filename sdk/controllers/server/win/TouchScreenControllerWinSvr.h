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

Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.

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

#ifdef _WIN32

#include "../TouchScreenControllerSvr.h"

namespace ssdk::ctls::svr
{
    //-------------------------------------------------------------------------------------------------
    // TouchScreenControllerWin
    //-------------------------------------------------------------------------------------------------
    class TouchScreenControllerWin : public TouchScreenController
    {
    public:
        TouchScreenControllerWin(ControllerManagerPtr pControllerManager);
        virtual ~TouchScreenControllerWin() {}

        virtual void ProcessInputEvent(const char* eventID, const ssdk::ctls::CtlEvent& event) override;

    protected:
        void SetScreenSize(AMFSize screenSize);
        AMFSize GetScreenSize();
        bool GetMousePosition(AMFPoint* pos) const;
        POINTER_TOUCH_INFO DefaultPointerTouchInfo() const;
        POINTER_INFO DefaultPointerInfo() const;

        void UpdateTouch(const POINTER_TOUCH_INFO& touchInfo);
        void RemoveTouch(UINT32 pointerId);

        enum Flags {
            TOUCH_BEGIN = POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT | POINTER_FLAG_DOWN,
            TOUCH_MOVE = POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT | POINTER_FLAG_UPDATE,
            TOUCH_END = POINTER_FLAG_UP
        };

    private:
        using Touches = std::vector<POINTER_TOUCH_INFO>;
        Touches m_Touches;
        AMFSize m_ScreenSize{ 1920, 1080 };
    };

} // namespace ssdk::ctls::svr

#endif // _WIN32
