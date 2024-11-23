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

#include "KeyboardControllerSvr.h"

namespace ssdk::ctls::svr
{
    //-------------------------------------------------------------------------------------------------
    // KeyboardController
    //-------------------------------------------------------------------------------------------------
    KeyboardController::KeyboardController(ControllerManagerPtr pControllerManager) :
        ControllerBase(pControllerManager)
    {
        m_ID = DEVICE_KEYBOARD;
    }

    void KeyboardController::ProcessInputEvent(const char* eventID, const ssdk::ctls::CtlEvent& event)
    {
        if (event.value.type == amf::AMF_VARIANT_INT64)
        {
            if (strcmp(eventID, DEVICE_KEYBOARD_KEYS) == 0)
            {
                SendKeyboardScanCode(event.value.int64Value);
            }
            else if (strcmp(eventID, DEVICE_KEYBOARD_CHARACTER) == 0)
            {
                unsigned int unicodeChar = (unsigned int)event.value.int64Value;
                SendKeyboardCharacter(unicodeChar);
            }
        }
    }

    void KeyboardController::InsertKeyboardScanCode(amf_int64 key)
    {
        if ((key & DEVICE_KEYBOARD_UP) == 0)
        {
            m_keyDownSet.insert(key | DEVICE_KEYBOARD_UP);
        }
        else
        {
            m_keyDownSet.erase(key);
        }
    }
    void KeyboardController::ReleaseKeyboardScanCodes()
    {
        std::set<amf_int64> keyCodes = m_keyDownSet;
        for (const amf_int64& keyCode : keyCodes)
        {
            SendKeyboardScanCode(keyCode);
        }
    }

}// namespace ssdk::ctls::svr
