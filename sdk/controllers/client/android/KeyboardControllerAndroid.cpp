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

#if defined(__ANDROID__)
#include "KeyboardControllerAndroid.h"
#include "../ControllerManager.h"
#include <android/input.h>
#include <android/keycodes.h>
#include <android/log.h>

static constexpr const char* TRACE_SCOPE = "KeyboardControllerAndroid.cpp";

namespace ssdk::ctls
{
    static constexpr const wchar_t* const AMF_FACILITY = L"KeyboardController";

    struct AndroidToWindowsKeyMapping
    {
        int androidKeyCode;
        int windowsVirtualKey;
        int windowsScanCode;
    };

    // Android keycodes to Windows virtual keys and scan codes
    static const AndroidToWindowsKeyMapping g_AndroidToWindowsKeyTable[] = {
        // Letters
        { AKEYCODE_A, 0x41, 0x1E },
        { AKEYCODE_B, 0x42, 0x30 },
        { AKEYCODE_C, 0x43, 0x2E },
        { AKEYCODE_D, 0x44, 0x20 },
        { AKEYCODE_E, 0x45, 0x12 },
        { AKEYCODE_F, 0x46, 0x21 },
        { AKEYCODE_G, 0x47, 0x22 },
        { AKEYCODE_H, 0x48, 0x23 },
        { AKEYCODE_I, 0x49, 0x17 },
        { AKEYCODE_J, 0x4A, 0x24 },
        { AKEYCODE_K, 0x4B, 0x25 },
        { AKEYCODE_L, 0x4C, 0x26 },
        { AKEYCODE_M, 0x4D, 0x32 },
        { AKEYCODE_N, 0x4E, 0x31 },
        { AKEYCODE_O, 0x4F, 0x18 },
        { AKEYCODE_P, 0x50, 0x19 },
        { AKEYCODE_Q, 0x51, 0x10 },
        { AKEYCODE_R, 0x52, 0x13 },
        { AKEYCODE_S, 0x53, 0x1F },
        { AKEYCODE_T, 0x54, 0x14 },
        { AKEYCODE_U, 0x55, 0x16 },
        { AKEYCODE_V, 0x56, 0x2F },
        { AKEYCODE_W, 0x57, 0x11 },
        { AKEYCODE_X, 0x58, 0x2D },
        { AKEYCODE_Y, 0x59, 0x15 },
        { AKEYCODE_Z, 0x5A, 0x2C },

        // Numbers
        { AKEYCODE_0, 0x30, 0x0B },
        { AKEYCODE_1, 0x31, 0x02 },
        { AKEYCODE_2, 0x32, 0x03 },
        { AKEYCODE_3, 0x33, 0x04 },
        { AKEYCODE_4, 0x34, 0x05 },
        { AKEYCODE_5, 0x35, 0x06 },
        { AKEYCODE_6, 0x36, 0x07 },
        { AKEYCODE_7, 0x37, 0x08 },
        { AKEYCODE_8, 0x38, 0x09 },
        { AKEYCODE_9, 0x39, 0x0A },

        // Function keys
        { AKEYCODE_F1, 0x70, 0x3B },
        { AKEYCODE_F2, 0x71, 0x3C },
        { AKEYCODE_F3, 0x72, 0x3D },
        { AKEYCODE_F4, 0x73, 0x3E },
        { AKEYCODE_F5, 0x74, 0x3F },
        { AKEYCODE_F6, 0x75, 0x40 },
        { AKEYCODE_F7, 0x76, 0x41 },
        { AKEYCODE_F8, 0x77, 0x42 },
        { AKEYCODE_F9, 0x78, 0x43 },
        { AKEYCODE_F10, 0x79, 0x44 },
        { AKEYCODE_F11, 0x7A, 0x57 },
        { AKEYCODE_F12, 0x7B, 0x58 },

        // Special keys
        { AKEYCODE_SPACE, 0x20, 0x39 },
        { AKEYCODE_ENTER, 0x0D, 0x1C },
        { AKEYCODE_TAB, 0x09, 0x0F },
        { AKEYCODE_ESCAPE, 0x1B, 0x01 },
        { AKEYCODE_DEL, 0x08, 0x0E },  // Backspace
        { AKEYCODE_FORWARD_DEL, 0x2E, 0xE053 },  // Delete (extended)

        // Modifiers
        { AKEYCODE_SHIFT_LEFT, 0x10, 0x2A },
        { AKEYCODE_SHIFT_RIGHT, 0x10, 0x36 },
        { AKEYCODE_CTRL_LEFT, 0x11, 0x1D },
        { AKEYCODE_CTRL_RIGHT, 0x11, 0xE01D },  // Extended
        { AKEYCODE_ALT_LEFT, 0x12, 0x38 },
        { AKEYCODE_ALT_RIGHT, 0x12, 0xE038 },  // Extended (AltGr)
        { AKEYCODE_META_LEFT, 0x5B, 0xE05B },  // Left Windows key (extended)
        { AKEYCODE_META_RIGHT, 0x5C, 0xE05C },  // Right Windows key (extended)

        // Arrow keys
        { AKEYCODE_DPAD_UP, 0x26, 0xE048 },  // Extended
        { AKEYCODE_DPAD_DOWN, 0x28, 0xE050 },  // Extended
        { AKEYCODE_DPAD_LEFT, 0x25, 0xE04B },  // Extended
        { AKEYCODE_DPAD_RIGHT, 0x27, 0xE04D },  // Extended

        // Navigation keys
        { AKEYCODE_PAGE_UP, 0x21, 0xE049 },  // Extended
        { AKEYCODE_PAGE_DOWN, 0x22, 0xE051 },  // Extended
        { AKEYCODE_MOVE_HOME, 0x24, 0xE047 },  // Extended
        { AKEYCODE_MOVE_END, 0x23, 0xE04F },  // Extended
        { AKEYCODE_INSERT, 0x2D, 0xE052 },  // Extended

        // Punctuation
        { AKEYCODE_COMMA, 0xBC, 0x33 },
        { AKEYCODE_PERIOD, 0xBE, 0x34 },
        { AKEYCODE_SLASH, 0xBF, 0x35 },
        { AKEYCODE_SEMICOLON, 0xBA, 0x27 },
        { AKEYCODE_APOSTROPHE, 0xDE, 0x28 },
        { AKEYCODE_LEFT_BRACKET, 0xDB, 0x1A },
        { AKEYCODE_RIGHT_BRACKET, 0xDD, 0x1B },
        { AKEYCODE_BACKSLASH, 0xDC, 0x2B },
        { AKEYCODE_GRAVE, 0xC0, 0x29 },
        { AKEYCODE_MINUS, 0xBD, 0x0C },
        { AKEYCODE_EQUALS, 0xBB, 0x0D },

        // Numpad
        { AKEYCODE_NUMPAD_0, 0x60, 0x52 },
        { AKEYCODE_NUMPAD_1, 0x61, 0x4F },
        { AKEYCODE_NUMPAD_2, 0x62, 0x50 },
        { AKEYCODE_NUMPAD_3, 0x63, 0x51 },
        { AKEYCODE_NUMPAD_4, 0x64, 0x4B },
        { AKEYCODE_NUMPAD_5, 0x65, 0x4C },
        { AKEYCODE_NUMPAD_6, 0x66, 0x4D },
        { AKEYCODE_NUMPAD_7, 0x67, 0x47 },
        { AKEYCODE_NUMPAD_8, 0x68, 0x48 },
        { AKEYCODE_NUMPAD_9, 0x69, 0x49 },
        { AKEYCODE_NUMPAD_DIVIDE, 0x6F, 0xE035 },  // Extended
        { AKEYCODE_NUMPAD_MULTIPLY, 0x6A, 0x37 },
        { AKEYCODE_NUMPAD_SUBTRACT, 0x6D, 0x4A },
        { AKEYCODE_NUMPAD_ADD, 0x6B, 0x4E },
        { AKEYCODE_NUMPAD_DOT, 0x6E, 0x53 },
        { AKEYCODE_NUMPAD_ENTER, 0x0D, 0xE01C },  // Extended
        { AKEYCODE_NUM_LOCK, 0x90, 0x45 },

        // Lock keys
        { AKEYCODE_CAPS_LOCK, 0x14, 0x3A },
        { AKEYCODE_SCROLL_LOCK, 0x91, 0x46 },

        // System keys
        { AKEYCODE_BREAK, 0x13, 0xE046 },  // Pause/Break (extended)
        { AKEYCODE_SYSRQ, 0x2C, 0xE037 },  // Print Screen (extended)

        // End of table marker
        { 0, 0, 0 }
    };

    KeyboardControllerAndroid::KeyboardControllerAndroid(ControllerManagerPtr pControllerManager) :
        KeyboardController(std::move(pControllerManager))
    {
    }

    //-------------------------------------------------------------------------------------------------
    AMF_RESULT KeyboardControllerAndroid::ProcessMessage(const WindowMessage& msg)
    {
        AMF_RESULT result = AMF_FAIL;
        if ((AInputEvent_getSource(msg) & AINPUT_SOURCE_KEYBOARD) && (AInputEvent_getType(msg) == AINPUT_EVENT_TYPE_KEY))
        {
            int32_t keyCode = AKeyEvent_getKeyCode(msg);
            int32_t action = AKeyEvent_getAction(msg);
            // Only process key down and key up events
            if (action != AKEY_EVENT_ACTION_DOWN && action != AKEY_EVENT_ACTION_UP)
            {
                return result;
            }

            // Find the mapping for this Android keycode
            int virtualKey = 0;
            int scanCode = 0;

            for (int i = 0; g_AndroidToWindowsKeyTable[i].androidKeyCode != 0; i++)
            {
                if (g_AndroidToWindowsKeyTable[i].androidKeyCode == keyCode)
                {
                    virtualKey = g_AndroidToWindowsKeyTable[i].windowsVirtualKey;
                    scanCode = g_AndroidToWindowsKeyTable[i].windowsScanCode;
                    break;
                }
            }

            if (virtualKey != 0)
            {
                CtlEvent ev = {};
                ev.value.type = amf::AMF_VARIANT_INT64;
                std::string id = m_EventIDs[DEVICE_KEYBOARD_INDEX].c_str();

                // Build the event value in the same format as Windows
                ev.value.int64Value = amf_int64((virtualKey) & 0xFFFF);   // virtual key
                ev.value.int64Value |= (((scanCode) & 0xFF) << 16);        // scan code

                // Set key up flag if this is a key release
                if (action == AKEY_EVENT_ACTION_UP)
                {
                    ev.value.int64Value |= DEVICE_KEYBOARD_UP;
                }

                // Set extended flag if scan code has extended bit
                if (scanCode & 0xE000)
                {
                    ev.value.int64Value |= DEVICE_KEYBOARD_EXTENDED;
                }

                if (m_pControllerManager != nullptr)
                {
                    m_pControllerManager->SendControllerEvent(id.c_str(), ev);
                }

                result = AMF_OK;
            }
            else
            {
                __android_log_print(ANDROID_LOG_WARN, TRACE_SCOPE,
                                    "No mapping for Android keycode: %d", keyCode);
            }
        }
        return result;
    }

    //-------------------------------------------------------------------------------------------------
    void KeyboardControllerAndroid::ReleaseModifiers()
    {
        // Release common modifier keys
        std::vector<int> modifierKeys{ 
            AKEYCODE_SHIFT_LEFT, AKEYCODE_SHIFT_RIGHT, 
            AKEYCODE_CTRL_LEFT, AKEYCODE_CTRL_RIGHT, 
            AKEYCODE_ALT_LEFT, AKEYCODE_ALT_RIGHT 
        };
        
        for (auto& androidKeyCode : modifierKeys)
        {
            // Find the mapping for this modifier key
            int virtualKey = 0;
            int scanCode = 0;
            
            for (int i = 0; g_AndroidToWindowsKeyTable[i].androidKeyCode != 0; i++)
            {
                if (g_AndroidToWindowsKeyTable[i].androidKeyCode == androidKeyCode)
                {
                    virtualKey = g_AndroidToWindowsKeyTable[i].windowsVirtualKey;
                    scanCode = g_AndroidToWindowsKeyTable[i].windowsScanCode;
                    break;
                }
            }

            if (virtualKey != 0)
            {
                CtlEvent ev = {};
                ev.value.type = amf::AMF_VARIANT_INT64;
                std::string id = m_EventIDs[DEVICE_KEYBOARD_INDEX].c_str();

                ev.value.int64Value = amf_int64((virtualKey) & 0xFFFF);
                ev.value.int64Value |= (((scanCode) & 0xFF) << 16);
                ev.value.int64Value |= DEVICE_KEYBOARD_UP;

                if (scanCode & 0xE000)
                {
                    ev.value.int64Value |= DEVICE_KEYBOARD_EXTENDED;
                }

                if (m_pControllerManager != nullptr)
                {
                    m_pControllerManager->SendControllerEvent(id.c_str(), ev);
                }
            }
        }
    }
}// namespace ssdk::ctls
#endif
