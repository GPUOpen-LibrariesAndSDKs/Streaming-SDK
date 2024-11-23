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

#if defined(__linux)
#include "../KeyboardController.h"
#include "../ControllerManager.h"
#include "tbls/WindowsToX11KeyTable.h"
#include "tbls/X11ToWindowsScanCodeTable.h"

namespace ssdk::ctls
{
    static constexpr const wchar_t* const AMF_FACILITY = L"KeyboardController";

    //-------------------------------------------------------------------------------------------------
    AMF_RESULT KeyboardController::ProcessMessage(const WindowMessage& msg)
    {
        AMF_RESULT result = AMF_FAIL;
        if (msg.type == KeyPress || msg.type == KeyRelease)
        {
            XKeyEvent* keyEvent = (XKeyEvent*)&msg;

            CtlEvent ev = {};
            ev.value.type = amf::AMF_VARIANT_INT64;
            std::string id = m_EventIDs[DEVICE_KEYBOARD_INDEX].c_str();

            int keyCode = keyEvent->keycode;
            KeySym keySym;
            bool numpad_key = false;
            if (keyEvent->state & Mod2Mask) //NumLock enabled
            {
                keySym = XkbKeycodeToKeysym(keyEvent->display, keyCode, 0, 1);

                if ((XK_KP_0 <= keySym && keySym <= XK_KP_9) || keySym == XK_KP_Decimal)
                {
                    numpad_key = true;
                }
            }

            if (!numpad_key)
            {
                keySym = XkbKeycodeToKeysym(keyEvent->display, keyCode, 0, 0);
            }

            int virtualKey = 0;
            for (int i = 0; WindowsToX11KeyTable[i][0] != 0; i++)
            {
                if (WindowsToX11KeyTable[i][0] == keySym)
                {
                    virtualKey = WindowsToX11KeyTable[i][1];
                    break;
                }
            }

            //"scancode" is not included because we can't get it through x11 :(
            // Generating scancode based on key pressed
            int scanCode = 0;
            ev.value.int64Value = amf_int64((virtualKey) & 0xFFFF);


            for (int i = 0; X11ToWindowsScanCodeTable[i][0] != 0; i++)
            {
                if (X11ToWindowsScanCodeTable[i][0] == keySym)
                {
                    scanCode = X11ToWindowsScanCodeTable[i][1];
                    break;
                }
            }

            ev.value.int64Value |= ((scanCode & 0xFF) << 16);

            if (msg.type == KeyRelease)
            {
                ev.value.int64Value |= DEVICE_KEYBOARD_UP;
            }

            if (scanCode & 0xE000)
            {
                ev.value.int64Value |= DEVICE_KEYBOARD_EXTENDED;
            }

            if (virtualKey != 0)
            {
                if (m_pControllerManager != nullptr)
                {
                    m_pControllerManager->SendControllerEvent(id.c_str(), &ev);
                }
            }
            else
            {
                AMFTraceError(AMF_FACILITY, L"No virtualKey for %ld, %ld", keyCode, keySym);
            }
        }

        return result;
    }

    //-------------------------------------------------------------------------------------------------
    AMF_RESULT KeyboardController::ReleaseModifiersKey()
    {
        std::vector<int> modifierKeys{ XK_Shift_L, XK_Shift_R, XK_Control_L, XK_Control_R, XK_Alt_L, XK_Alt_R };
        for (auto& keySym : modifierKeys)
        {
            CtlEvent ev = {};
            ev.value.type = amf::AMF_VARIANT_INT64;
            std::string id = m_EventIDs[DEVICE_KEYBOARD_INDEX].c_str();

            int virtualKey = 0;
            for (int i = 0; WindowsToX11KeyTable[i][0] != 0; i++)
            {
                if (WindowsToX11KeyTable[i][0] == keySym)
                {
                    virtualKey = WindowsToX11KeyTable[i][1];
                    break;
                }
            }

            ev.value.int64Value = amf_int64((virtualKey) & 0xFFFF);

            int scanCode = 0;
            for (int i = 0; X11ToWindowsScanCodeTable[i][0] != 0; i++)
            {
                if (X11ToWindowsScanCodeTable[i][0] == keySym)
                {
                    scanCode = X11ToWindowsScanCodeTable[i][1];
                    break;
                }
            }

            ev.value.int64Value |= ((scanCode & 0xFF) << 16);
            ev.value.int64Value |= DEVICE_KEYBOARD_UP;

            if (scanCode & 0xE000)
            {
                ev.value.int64Value |= DEVICE_KEYBOARD_EXTENDED;
            }

            if (virtualKey != 0)
            {
                if (m_pControllerManager != nullptr)
                {
                    m_pControllerManager->SendControllerEvent(id.c_str(), &ev);
                }
            }
        }

        return AMF_OK;
    }
}// namespace ssdk::ctls
#endif
