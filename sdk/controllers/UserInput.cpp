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

#include "UserInput.h"

namespace ssdk::ctls
{

    //----------------------------------------------------------------------------------------------
    // Event IDs
    //----------------------------------------------------------------------------------------------
    // examples of events
    // /ctrlLeft/in/tr/click value = true
    // /ctrlLeft/in/tp/val value = AMFFLoatPoint(0.4f,-0.3f)

    //  Device IDs:
    const char* DEVICE_HMD = "/hmd";
    const char* DEVICE_PC = "/pc";
    const char* DEVICE_REDIRECT = "/redirect";
    const char* DEVICE_CONTROLLER_LEFT = "/ctrlLeft";
    const char* DEVICE_CONTROLLER_RIGHT = "/ctrlRight";
    const char* DEVICE_MOUSE = "/mouse";
    const char* DEVICE_TOUCHSCREEN = "/tchScr";
    const char* DEVICE_KEYBOARD = "/keyboard";
    const char* DEVICE_GAME_CONTROLLER = "/ctrlGame";      // for full ID adds index field : "/ctrlGame0", "/ctrlGame1"

    //  Pose ID for trackable devices - must be concatenated to device ID:
    const char* DEVICE_POSE = "/pose";          // ANSPose
    const char* DEVICE_BATTERY = "/battery";       // float 0.0-1.0f

    const char* DEVICE_CTRL_TOUCH = "/touch";
    const char* DEVICE_CTRL_CLICK = "/click";
    const char* DEVICE_CTRL_VAL = "/val";

    //  VR Controller IDs - must be concatenated to device ID:
    const char* DEVICE_CTRL_TRIGGER = "/in/tr";
    const char* DEVICE_CTRL_TRIGGER_CLICK = "/in/tr/click";   // bool
    const char* DEVICE_CTRL_TRIGGER_TOUCH = "/in/tr/touch";   // bool
    const char* DEVICE_CTRL_TRIGGER_VALUE = "/in/tr/val";     // float 0.0-1.0f

    const char* DEVICE_CTRL_SYSTEM = "/in/sys";
    const char* DEVICE_CTRL_SYSTEM_CLICK = "/in/sys/click";  // bool

    const char* DEVICE_CTRL_APPMENU = "/in/menu";
    const char* DEVICE_CTRL_APPMENU_CLICK = "/in/menu/click"; // bool

    const char* DEVICE_CTRL_MODE = "/in/mode";
    const char* DEVICE_CTRL_MODE_CLICK = "/in/mode/click"; // bool

    const char* DEVICE_CTRL_GRIP = "/in/grip";
    const char* DEVICE_CTRL_GRIP_CLICK = "/in/grip/click"; // bool
    const char* DEVICE_CTRL_GRIP_TOUCH = "/in/grip/touch"; // bool
    const char* DEVICE_CTRL_GRIP_VALUE = "/in/grip/val";   // float 0.0-1.0f

    const char* DEVICE_CTRL_JOYSTICK = "/in/js";
    const char* DEVICE_CTRL_JOYSTICK_VALUE = "/in/js/val";    // AMFFloatPoint2D(x, y), ((-1.0f, 1.0f), (-1.0f, 1.0f))
    const char* DEVICE_CTRL_JOYSTICK_CLICK = "/in/js/click";  // bool
    const char* DEVICE_CTRL_JOYSTICK_TOUCH = "/in/js/touch";  // bool

    const char* DEVICE_CTRL_TRACKPAD = "/in/tp";
    const char* DEVICE_CTRL_TRACKPAD_VALUE = "/in/tp/val";    // AMFFloatPoint2D(x, y), ((-1.0f, 1.0f), (-1.0f, 1.0f))
    const char* DEVICE_CTRL_TRACKPAD_CLICK = "/in/tp/click";  // bool
    const char* DEVICE_CTRL_TRACKPAD_TOUCH = "/in/tp/touch";  // bool

    const char* DEVICE_CTRL_VOLUME_UP = "/in/vol/+";
    const char* DEVICE_CTRL_VOLUME_UP_CLICK = "/in/vol/+/click";    // bool
    const char* DEVICE_CTRL_VOLUME_DOWN = "/in/vol/-";
    const char* DEVICE_CTRL_VOLUME_DOWN_CLICK = "/in/vol/-/click";    // bool

    const char* DEVICE_CTRL_DPAD_UP = "/in/dpad/up";
    const char* DEVICE_CTRL_DPAD_UP_CLICK = "/in/dpad/up/click";      // bool
    const char* DEVICE_CTRL_DPAD_UP_TOUCH = "/in/dpad/up/touch";      // bool

    const char* DEVICE_CTRL_DPAD_DOWN = "/in/dpad/dn";
    const char* DEVICE_CTRL_DPAD_DOWN_CLICK = "/in/dpad/dn/click";    // bool
    const char* DEVICE_CTRL_DPAD_DOWN_TOUCH = "/in/dpad/dn/touch";    // bool

    const char* DEVICE_CTRL_DPAD_LEFT = "/in/dpad/left";
    const char* DEVICE_CTRL_DPAD_LEFT_CLICK = "/in/dpad/left/click";  // bool
    const char* DEVICE_CTRL_DPAD_LEFT_TOUCH = "/in/dpad/left/touch";  // bool

    const char* DEVICE_CTRL_DPAD_RIGHT = "/in/dpad/right";
    const char* DEVICE_CTRL_DPAD_RIGHT_CLICK = "/in/dpad/right/click"; // bool
    const char* DEVICE_CTRL_DPAD_RIGHT_TOUCH = "/in/dpad/right/touch"; // bool

    const char* DEVICE_CTRL_X = "/in/x";
    const char* DEVICE_CTRL_X_CLICK = "/in/x/click";    // bool
    const char* DEVICE_CTRL_X_TOUCH = "/in/x/touch";    // bool

    const char* DEVICE_CTRL_Y = "/in/y";
    const char* DEVICE_CTRL_Y_CLICK = "/in/y/click";    // bool
    const char* DEVICE_CTRL_Y_TOUCH = "/in/y/touch";    // bool

    const char* DEVICE_CTRL_A = "/in/a";
    const char* DEVICE_CTRL_A_CLICK = "/in/a/click";    // bool
    const char* DEVICE_CTRL_A_TOUCH = "/in/a/touch";    // bool

    const char* DEVICE_CTRL_B = "/in/b";
    const char* DEVICE_CTRL_B_CLICK = "/in/b/click";    // bool
    const char* DEVICE_CTRL_B_TOUCH = "/in/b/touch";    // bool

    const double DEVICE_CTRL_HAPTIC_LOWFREQUENCYTHRESHOLD = 100.f;  // in Herz
    const char* DEVICE_CTRL_HAPTIC = "/out/haptic";    // AMFFloatPoint3D(amplitude, frequency, duration), ((0.0f, 1.0f), Herz, seconds)

    // mouse
    const uint64_t DEVICE_MOUSE_POS_ABSOLUTE = 0x000000001;// set as flags to ANSEvent flags
    const char* DEVICE_MOUSE_POS = "/in/pos";        // AMFFloatPoint2D(x, y), ((0.0f, 0.0f), 0.0-1.0
    const char* DEVICE_MOUSE_L = "/in/l";
    const char* DEVICE_MOUSE_L_CLICK = "/in/l/click";    // bool
    const char* DEVICE_MOUSE_R = "/in/r";
    const char* DEVICE_MOUSE_R_CLICK = "/in/r/click";    // bool
    const char* DEVICE_MOUSE_M = "/in/m";
    const char* DEVICE_MOUSE_M_CLICK = "/in/m/click";    // bool
    const char* DEVICE_MOUSE_M_TRACK = "/in/m/track";    // amf_int64 - Windows wheel units (120 per click)

    // touchscreen
    const char* DEVICE_TOUCHSCREEN_TOUCH = "/in/tch";        // AMFFloatPoint2D(x, y), ((0.0f, 0.0f), 0.0-1.0

    // keyboard, scan code
    const uint64_t DEVICE_KEYBOARD_UP = 0x100000000;
    const uint64_t DEVICE_KEYBOARD_EXTENDED = 0x200000000;
    const uint64_t DEVICE_KEYBOARD_NO_SCANCODE = 0x400000000;
    const char* DEVICE_KEYBOARD_KEYS = "/in/k";    // amf_int64 - low 16 bits - virtual key, next 16 bit - scan code, high 32 bits - masks

    // keyboard, unicode character
    const char* DEVICE_KEYBOARD_CHARACTER = "/in/char";       // amf_int64 - unicode character

    // Game Controller IDs - must be concatenated to device ID:
    const char* DEVICE_CTRL_TRIGGER_LEFT = "/in/tr/l";
    const char* DEVICE_CTRL_TRIGGER_LEFT_VALUE = "/in/tr/l/val";   // float 0.0-1.0
    const char* DEVICE_CTRL_TRIGGER_RIGHT = "/in/tr/r";
    const char* DEVICE_CTRL_TRIGGER_RIGHT_VALUE = "/in/tr/r/val";   // float 0.0-1.0

    const char* DEVICE_CTRL_JOYSTICK_LEFT = "/in/js/l";
    const char* DEVICE_CTRL_JOYSTICK_LEFT_VALUE = "/in/js/l/val";   // AMFFloatPoint2D(x, y), ((-1.0f, 1.0f), (-1.0f, 1.0f))
    const char* DEVICE_CTRL_JOYSTICK_LEFT_CLICK = "/in/js/l/click"; // bool
    const char* DEVICE_CTRL_JOYSTICK_RIGHT = "/in/js/r";
    const char* DEVICE_CTRL_JOYSTICK_RIGHT_VALUE = "/in/js/r/val";   // AMFFloatPoint2D(x, y), ((-1.0f, 1.0f), (-1.0f, 1.0f))
    const char* DEVICE_CTRL_JOYSTICK_RIGHT_CLICK = "/in/js/r/click"; // bool

    const char* DEVICE_CTRL_GRIP_LEFT = "/in/grip/l";
    const char* DEVICE_CTRL_GRIP_RIGHT = "/in/grip/r";
    const char* DEVICE_CTRL_GRIP_LEFT_CLICK = "/in/grip/l/click"; // bool
    const char* DEVICE_CTRL_GRIP_RIGHT_CLICK = "/in/grip/r/click"; // bool

    const char* DEVICE_CTRL_HAPTIC_STATE = "/out/hapstate";    // AMFFloatPoint2D(left, right), ((0.0f, 1.0f), (0.0f, 1.0f))

    const wchar_t* DEVICE_DOF = L"DOF";             // amf_int64; default = 3; can be 3, 6 - degree of freedom, can change after display is connected
    const wchar_t* DEVICE_DESCRIPTION = L"Description";     // wchar*; default for controller = ""; can be any like "Oculus6DoF"

    //----------------------------------------------------------------------------------------------
    // Cursor bitmap properties
    //----------------------------------------------------------------------------------------------
    const wchar_t* CURSOR_HOTSPOT           = L"Hotspot";       // AMFPoint; the hotspot coordinates of the mouse cursor.
    const wchar_t* CURSOR_MONOCHROME        = L"Monochrome";    // amf_bool; if true, the cursor is a monochrome type (e.g. I-Beam in textbox). When rendering in the client, an xor effect should be applied
    const wchar_t* CURSOR_SCREEN_RESOLUTION = L"Resolution";    // AMFSize; capture screen resolution. Used on client to calculate cursor scale.
}
