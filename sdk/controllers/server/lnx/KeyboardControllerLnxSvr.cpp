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

#if defined(__linux)

#include "KeyboardControllerLnxSvr.h"
#include "controllers/tbls/WindowsToX11KeyTable.h"
#include "amf/public/common/TraceAdapter.h"
#include <X11/extensions/XTest.h>

namespace ssdk::ctls::svr
{
    static constexpr const wchar_t* AMF_FACILITY = L"KeyboardControllerLnx";

    //-------------------------------------------------------------------------------------------------
    // KeyboardControllerLnx
    //-------------------------------------------------------------------------------------------------
    KeyboardControllerLnx::KeyboardControllerLnx(ControllerManagerPtr pControllerManager, XDisplay::Ptr pDisplay) :
        KeyboardController(pControllerManager),
        m_pDisplay(pDisplay)
    {
    }

    void KeyboardControllerLnx::SendKeyboardScanCode(amf_int64 key)
    {
        InsertKeyboardScanCode(key);
        int virtualKeyCode = key & 0xFFFF;
        int hardwareScanCode = (key >> 16) & 0xFF;
        bool isDown = (key & DEVICE_KEYBOARD_UP) == 0;
        AMFTraceInfo(AMF_FACILITY, L"vkc: %d, hwsc: %d, id: %d", virtualKeyCode, hardwareScanCode, isDown);

        int keySym = 0;
        if (virtualKeyCode != 0)
        {
            for (int i = 0; WindowsToX11KeyTable[i][0] != 0; i++)
            {
                if (WindowsToX11KeyTable[i][1] == virtualKeyCode)
                {
                    keySym = WindowsToX11KeyTable[i][0];
                }
            }
        }
        XDisplayPtr display(m_pDisplay);
        int keyCode = XKeysymToKeycode(display, keySym);
        XTestFakeKeyEvent(display, keyCode, isDown, 0);
        XSync(display, false);
    }

    void KeyboardControllerLnx::SendKeyboardCharacter(unsigned int /*unicodeChar*/)
    {
        // Unimplemented: server supports unicode characters like languages other than English if it set up on the server
    }

}// namespace ssdk::ctls::svr

#endif // __linux
