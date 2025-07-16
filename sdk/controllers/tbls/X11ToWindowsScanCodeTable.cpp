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

#include "X11ToWindowsScanCodeTable.h"
#include <X11/keysym.h>

namespace ssdk::ctls
{
//A table that maps linux x11 keycodes to Windows Scan codes
const int X11ToWindowsScanCodeTable[][2] = {
{XK_Escape, 0x01},
{XK_1, 0x02},
{XK_2, 0x03},
{XK_3, 0x04},
{XK_4, 0x05},
{XK_5, 0x06},
{XK_6, 0x07},
{XK_7, 0x08},
{XK_8, 0x09},
{XK_9, 0x0A},
{XK_0, 0x0B},
{XK_KP_1, 0x4F},
{XK_KP_2, 0x50},
{XK_KP_3, 0x51},
{XK_KP_4, 0x4B},
{XK_KP_5, 0x4C},
{XK_KP_6, 0x4D},
{XK_KP_7, 0x47},
{XK_KP_8, 0x48},
{XK_KP_9, 0x49},
{XK_KP_0, 0x52},
{XK_minus, 0x0C},
{XK_equal, 0x0D},
{XK_BackSpace, 0x0E},
{XK_Tab, 0x0F},
{XK_q, 0x10},
{XK_w, 0x11},
{XK_e, 0x12},
{XK_r, 0x13},
{XK_t, 0x14},
{XK_y, 0x15},
{XK_u, 0x16},
{XK_i, 0x17},
{XK_o, 0x18},
{XK_p, 0x19},
{XK_bracketleft, 0x1A},
{XK_bracketright, 0x1B},
{XK_Return, 0x1C},
{XK_Control_L, 0x1D},
{XK_a, 0x1E},
{XK_s, 0x1F},
{XK_d, 0x20},
{XK_f, 0x21},
{XK_g, 0x22},
{XK_h, 0x23},
{XK_j, 0x24},
{XK_k, 0x25},
{XK_l, 0x26},
{XK_semicolon, 0x27},
{XK_apostrophe, 0x28},
{XK_asciitilde, 0x29},
{XK_grave, 0x29},
{XK_Select, 0x28},
{XK_Shift_L, 0x2A},
{XK_backslash, 0x2B},
{XK_z, 0x2C},
{XK_x, 0x2D},
{XK_c, 0x2E},
{XK_v, 0x2F},
{XK_b, 0x30},
{XK_n, 0x31},
{XK_m, 0x32},
{XK_comma, 0x33},
{XK_period, 0x34},
{XK_slash, 0x35},
{XK_Shift_R, 0x36},
{XK_KP_Multiply, 0x37},
{XK_Alt_L, 0x38},
{XK_space, 0x39},
{XK_Caps_Lock, 0x3A},
{XK_F1, 0x3B},
{XK_F2, 0x3C},
{XK_F3, 0x3D},
{XK_F4, 0x3E},
{XK_F5, 0x3F},
{XK_F6, 0x40},
{XK_F7, 0x41},
{XK_F8, 0x42},
{XK_F9, 0x43},
{XK_F10, 0x44},
{XK_Num_Lock, 0x45},
{XK_Scroll_Lock, 0x46},
{XK_KP_Home, 0x47},
{XK_KP_Up, 0x48},
{XK_KP_Prior, 0x49},
{XK_KP_Subtract, 0x4A},
{XK_KP_Left, 0x4B},
{XK_KP_Begin, 0x4C},
{XK_KP_Right, 0x4D},
{XK_KP_Add, 0x4E},
{XK_KP_End, 0x4F},
{XK_KP_Down, 0x50},
{XK_KP_Next, 0x51},
{XK_KP_Insert, 0x52},
{XK_KP_Delete, 0x53},
{XK_KP_Decimal, 0x53},
{XK_ISO_Level3_Shift, 0x56},
{XK_less, 0x56},
{XK_F11, 0x57},
{XK_F12, 0x58},
{XK_Help, 0x63},
{XK_F13, 0x64},
{XK_F14, 0x65},
{XK_F15, 0x66},
{XK_F16, 0x67},
{XK_F17, 0x68},
{XK_F18, 0x69},
{XK_F19, 0x71},
{XK_F20, 0x72},
{XK_F21, 0x73},
{XK_F22, 0x74},
{XK_F23, 0x75},
{XK_F24, 0x76},
{XK_Undo, 0x7A},
{XK_Find, 0x7F},
{XK_KP_Enter, 0xE01C},
{XK_KP_Divide, 0xE035},
{XK_Return, 0xE01C},
{XK_Control_R, 0xE09D},
{XK_KP_Divide, 0xB5},
{XK_Print, 0xE0B7},
{XK_Alt_R, 0xE0B8},
{XK_Home, 0xE047},
{XK_Up, 0xE048},
{XK_Prior, 0xE049},
{XK_Left, 0xE04B},
{XK_Right, 0xE04D},
{XK_End, 0xE04F},
{XK_Down, 0xE050},
{XK_Next, 0xE051},
{XK_Insert, 0xE052},
{XK_Delete, 0xE053},
{XK_Multi_key, 0xF0},
{XK_KP_Enter, 0x9C},
{XK_KP_Multiply, 0x37},
{XK_Pause, 0xE19DC5},
{XK_Break, 0xE19DC5},
};

} // namespace ssdk::ctls
#endif
