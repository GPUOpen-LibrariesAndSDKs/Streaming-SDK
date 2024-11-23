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

#include <stdint.h>

namespace ssdk::ctls
{
    //----------------------------------------------------------------------------------------------
    // Event IDs
    //----------------------------------------------------------------------------------------------
    // examples of events
    // /ctrlLeft/in/tr/click value = true
    // /ctrlLeft/in/tp/val value = AMFFLoatPoint(0.4f,-0.3f)

    //  Device IDs:
    extern const char* DEVICE_HMD;
    extern const char* DEVICE_PC;
    extern const char* DEVICE_REDIRECT;
    extern const char* DEVICE_CONTROLLER_LEFT;
    extern const char* DEVICE_CONTROLLER_RIGHT;
    extern const char* DEVICE_MOUSE;
    extern const char* DEVICE_TOUCHSCREEN;
    extern const char* DEVICE_KEYBOARD;
    extern const char* DEVICE_GAME_CONTROLLER;  // for full ID adds index field : "/ctrlGame0", "/ctrlGame1"

    //  Pose ID for trackable devices - must be concatenated to device ID:
    extern const char* DEVICE_POSE;             // ANSPose
    extern const char* DEVICE_BATTERY;          // float 0.0-1.0f

    extern const char* DEVICE_CTRL_TOUCH;
    extern const char* DEVICE_CTRL_CLICK;
    extern const char* DEVICE_CTRL_VAL;

    //  VR Controller IDs - must be concatenated to device ID:
    extern const char* DEVICE_CTRL_TRIGGER;
    extern const char* DEVICE_CTRL_TRIGGER_CLICK;   // bool
    extern const char* DEVICE_CTRL_TRIGGER_TOUCH;   // bool
    extern const char* DEVICE_CTRL_TRIGGER_VALUE;   // float 0.0-1.0f

    extern const char* DEVICE_CTRL_SYSTEM;
    extern const char* DEVICE_CTRL_SYSTEM_CLICK;    // bool

    extern const char* DEVICE_CTRL_APPMENU;
    extern const char* DEVICE_CTRL_APPMENU_CLICK;   // bool

    extern const char* DEVICE_CTRL_MODE;
    extern const char* DEVICE_CTRL_MODE_CLICK;      // bool

    extern const char* DEVICE_CTRL_GRIP;
    extern const char* DEVICE_CTRL_GRIP_CLICK;      // bool
    extern const char* DEVICE_CTRL_GRIP_TOUCH;      // bool
    extern const char* DEVICE_CTRL_GRIP_VALUE;      // float 0.0-1.0f

    extern const char* DEVICE_CTRL_JOYSTICK;
    extern const char* DEVICE_CTRL_JOYSTICK_VALUE;  // AMFFloatPoint2D(x, y), ((-1.0f, 1.0f), (-1.0f, 1.0f))
    extern const char* DEVICE_CTRL_JOYSTICK_CLICK;  // bool
    extern const char* DEVICE_CTRL_JOYSTICK_TOUCH;  // bool

    extern const char* DEVICE_CTRL_TRACKPAD;
    extern const char* DEVICE_CTRL_TRACKPAD_VALUE;  // AMFFloatPoint2D(x, y), ((-1.0f, 1.0f), (-1.0f, 1.0f))
    extern const char* DEVICE_CTRL_TRACKPAD_CLICK;  // bool
    extern const char* DEVICE_CTRL_TRACKPAD_TOUCH;  // bool

    extern const char* DEVICE_CTRL_VOLUME_UP;
    extern const char* DEVICE_CTRL_VOLUME_UP_CLICK; // bool
    extern const char* DEVICE_CTRL_VOLUME_DOWN;
    extern const char* DEVICE_CTRL_VOLUME_DOWN_CLICK;// bool

    extern const char* DEVICE_CTRL_DPAD_UP;
    extern const char* DEVICE_CTRL_DPAD_UP_CLICK;   // bool
    extern const char* DEVICE_CTRL_DPAD_UP_TOUCH;   // bool

    extern const char* DEVICE_CTRL_DPAD_DOWN;
    extern const char* DEVICE_CTRL_DPAD_DOWN_CLICK; // bool
    extern const char* DEVICE_CTRL_DPAD_DOWN_TOUCH; // bool

    extern const char* DEVICE_CTRL_DPAD_LEFT;
    extern const char* DEVICE_CTRL_DPAD_LEFT_CLICK; // bool
    extern const char* DEVICE_CTRL_DPAD_LEFT_TOUCH; // bool

    extern const char* DEVICE_CTRL_DPAD_RIGHT;
    extern const char* DEVICE_CTRL_DPAD_RIGHT_CLICK; // bool
    extern const char* DEVICE_CTRL_DPAD_RIGHT_TOUCH; // bool

    extern const char* DEVICE_CTRL_X ;
    extern const char* DEVICE_CTRL_X_CLICK;    // bool
    extern const char* DEVICE_CTRL_X_TOUCH;    // bool

    extern const char* DEVICE_CTRL_Y;
    extern const char* DEVICE_CTRL_Y_CLICK;    // bool
    extern const char* DEVICE_CTRL_Y_TOUCH;    // bool

    extern const char* DEVICE_CTRL_A;
    extern const char* DEVICE_CTRL_A_CLICK;    // bool
    extern const char* DEVICE_CTRL_A_TOUCH;    // bool

    extern const char* DEVICE_CTRL_B;
    extern const char* DEVICE_CTRL_B_CLICK;    // bool
    extern const char* DEVICE_CTRL_B_TOUCH;    // bool

    extern const double DEVICE_CTRL_HAPTIC_LOWFREQUENCYTHRESHOLD;   // in Herz
    extern const char* DEVICE_CTRL_HAPTIC;                          // AMFFloatPoint3D(amplitude, frequency, duration), ((0.0f, 1.0f), Herz, seconds)

    // mouse
    extern const uint64_t DEVICE_MOUSE_POS_ABSOLUTE;// set as flags to ANSEvent flags
    extern const char* DEVICE_MOUSE_POS;            // AMFFloatPoint2D(x, y), ((0.0f, 0.0f), 0.0-1.0
    extern const char* DEVICE_MOUSE_L;
    extern const char* DEVICE_MOUSE_L_CLICK;        // bool
    extern const char* DEVICE_MOUSE_R;
    extern const char* DEVICE_MOUSE_R_CLICK;        // bool
    extern const char* DEVICE_MOUSE_M;
    extern const char* DEVICE_MOUSE_M_CLICK;        // bool
    extern const char* DEVICE_MOUSE_M_TRACK;        // amf_int64 - Windows wheel units (120 per click)

    // touchscreen
    extern const char* DEVICE_TOUCHSCREEN_TOUCH;    // AMFFloatPoint2D(x, y), ((0.0f, 0.0f), 0.0-1.0

    // keyboard, scan code
    extern const uint64_t DEVICE_KEYBOARD_UP;
    extern const uint64_t DEVICE_KEYBOARD_EXTENDED;
    extern const uint64_t DEVICE_KEYBOARD_NO_SCANCODE;
    extern const char* DEVICE_KEYBOARD_KEYS;        // amf_int64 - low 16 bits - virtual key, next 16 bit - scan code, high 32 bits - masks

    // keyboard, unicode character
    extern const char* DEVICE_KEYBOARD_CHARACTER;   // amf_int64 - unicode character

    // Game Controller IDs - must be concatenated to device ID:
    extern const char* DEVICE_CTRL_TRIGGER_LEFT;
    extern const char* DEVICE_CTRL_TRIGGER_LEFT_VALUE;  // float 0.0-1.0
    extern const char* DEVICE_CTRL_TRIGGER_RIGHT;
    extern const char* DEVICE_CTRL_TRIGGER_RIGHT_VALUE; // float 0.0-1.0

    extern const char* DEVICE_CTRL_JOYSTICK_LEFT;
    extern const char* DEVICE_CTRL_JOYSTICK_LEFT_VALUE; // AMFFloatPoint2D(x, y), ((-1.0f, 1.0f), (-1.0f, 1.0f))
    extern const char* DEVICE_CTRL_JOYSTICK_LEFT_CLICK; // bool
    extern const char* DEVICE_CTRL_JOYSTICK_RIGHT;
    extern const char* DEVICE_CTRL_JOYSTICK_RIGHT_VALUE;// AMFFloatPoint2D(x, y), ((-1.0f, 1.0f), (-1.0f, 1.0f))
    extern const char* DEVICE_CTRL_JOYSTICK_RIGHT_CLICK;// bool

    extern const char* DEVICE_CTRL_GRIP_LEFT;
    extern const char* DEVICE_CTRL_GRIP_RIGHT;
    extern const char* DEVICE_CTRL_GRIP_LEFT_CLICK;     // bool
    extern const char* DEVICE_CTRL_GRIP_RIGHT_CLICK;    // bool

    extern const char* DEVICE_CTRL_HAPTIC_STATE;        // AMFFloatPoint2D(left, right), ((0.0f, 1.0f), (0.0f, 1.0f))

    //----------------------------------------------------------------------------------------------
    // device properties
    //----------------------------------------------------------------------------------------------
    extern const wchar_t* DEVICE_DOF;                   // amf_int64; default = 3; can be 3, 6 - degree of freedom, can change after display is connected
    extern const wchar_t* DEVICE_DESCRIPTION;           // wchar*; default for controller = ""; can be any like "Oculus6DoF"

    //----------------------------------------------------------------------------------------------
    // Cursor bitmap properties
    //----------------------------------------------------------------------------------------------
    extern const wchar_t* CURSOR_HOTSPOT;               // AMFPoint; the hotspot coordinates of the mouse cursor.
    extern const wchar_t* CURSOR_MONOCHROME;            // amf_bool; if true, the cursor is a monochrome type (e.g. I-Beam in textbox). When rendering in the client, an xor effect should be applied
    extern const wchar_t* CURSOR_SCREEN_RESOLUTION;     // AMFSize; capture screen resolution. Used on client to calculate cursor scale.

} // namespace ssdk::ctls
