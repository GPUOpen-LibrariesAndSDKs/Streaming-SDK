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

#ifdef _WIN32

#include "KeyboardControllerWinSvr.h"
#include "amf/public/common/TraceAdapter.h"

namespace ssdk::ctls::svr
{
    static constexpr const wchar_t* AMF_FACILITY = L"KeyboardControllerWin";

    //-------------------------------------------------------------------------------------------------
    // KeyboardControllerWin
    //-------------------------------------------------------------------------------------------------
    KeyboardControllerWin::KeyboardControllerWin(ControllerManagerPtr pControllerManager) :
        KeyboardController(pControllerManager)
    {
    }

    void KeyboardControllerWin::SendKeyboardScanCode(amf_int64 key)
    {
        InsertKeyboardScanCode(key);
        INPUT inp = {};
        inp.type = INPUT_KEYBOARD;
        inp.ki.wVk = WORD(key & 0xFFFF);
        inp.ki.wScan = (key >> 16) & 0xFF;

        if ((key & DEVICE_KEYBOARD_NO_SCANCODE) == false)
        {
            inp.ki.dwFlags |= KEYEVENTF_SCANCODE;
        }
        if ((key & DEVICE_KEYBOARD_UP))
        {
            inp.ki.dwFlags |= KEYEVENTF_KEYUP;
        }
        if ((key & DEVICE_KEYBOARD_EXTENDED))
        {
            inp.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        }

        ::SendInput(1, &inp, sizeof(inp));
    }

    void KeyboardControllerWin::SendKeyboardCharacter(unsigned int unicodeChar)
    {

        if (unicodeChar > 0x10000)
        {
            WORD h = (unicodeChar >> 16) & 0xFFFF;
            WORD l = unicodeChar & 0xFFFF;

            // if not already in surrogate pair, convert it
            if (!IS_HIGH_SURROGATE(h) && !IS_LOW_SURROGATE(l))
            {
                unsigned int code = unicodeChar - 0x10000;
                h = 0xD800 | WORD(code >> 10);
                l = 0xDC00 | WORD(code & 0x3ff);
            }

            if (IS_HIGH_SURROGATE(h) && IS_LOW_SURROGATE(l))
            {
                INPUT inp[4];
                memset(inp, 0, sizeof(inp));

                for (int i = 0; i < 4; i++)
                {
                    inp[i].type = INPUT_KEYBOARD;
                    inp[i].ki.dwFlags = KEYEVENTF_UNICODE;
                }

                // Send input in this order, high, low, high(keyup), low(keyup)
                inp[0].ki.wScan = h;

                inp[1].ki.wScan = l;

                inp[2].ki.wScan = h;
                inp[2].ki.dwFlags |= KEYEVENTF_KEYUP;

                inp[3].ki.wScan = l;
                inp[3].ki.dwFlags |= KEYEVENTF_KEYUP;
                ::SendInput(4, inp, sizeof(inp));
            }
            else
            {
                AMFTraceInfo(AMF_FACILITY, L"invalid utf32 character: 0x%x, 0x%x", h, l);
            }
        }
        else
        {
            INPUT inp = {};

            // To input the character to foreground application, send down event first, followed by up event
            inp.type = INPUT_KEYBOARD;
            inp.ki.wScan = WORD(unicodeChar);
            inp.ki.dwFlags = KEYEVENTF_UNICODE;

            UINT res = ::SendInput(1, &inp, sizeof(inp));

            inp.ki.dwFlags |= KEYEVENTF_KEYUP;
            res = ::SendInput(1, &inp, sizeof(inp));
        }
    }

}// namespace ssdk::ctls::svr

#endif // _WIN32
