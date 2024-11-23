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

#include "WindowsToX11KeyTable.h"
#include <X11/keysym.h>

namespace ssdk::ctls
{
//A table that maps windows Virtual-Key Codes to X11 Key codes
const int WindowsToX11KeyTable[][2] = {
	{ XK_Left,	/* VK_LBUTTON */ 0x01 },
	{ XK_Right,	/* VK_RBUTTON */ 0x02 },
	// { XK_cancel,	/* VK_CANCEL */ 0x03 },
	// { XK_mbutton,	/* VK_MBUTTON */ 0x04 },
	// { XK_xbutton1,	/* VK_XBUTTON1 */ 0x05 },
	// { XK_xbutton2,	/* VK_XBUTTON2 */ 0x06 },
	{ XK_BackSpace,	/* VK_BACK */ 0x08 },
	{ XK_Tab,	/* VK_TAB */ 0x09 },
	{ XK_Clear,	/* VK_CLEAR */ 0x0C },
	{ XK_Return,	/* VK_RETURN */ 0x0D },
	{ XK_Shift_L,	/* VK_SHIFT */ 0x10 },
	{ XK_Control_L,	/* VK_CONTROL */ 0x11 },
	{ XK_Meta_L,	/* VK_MENU */ 0x12 },
	{ XK_Pause,	/* VK_PAUSE */ 0x13 },
	{ XK_Caps_Lock,	/* VK_CAPITAL */ 0x14 },
	// { XK_Kana,	/* VK_KANA */ 0x15 },
	// { XK_Hangeul,	/* VK_HANGEUL */ 0x15 },
	// { XK_Hangul,	/* VK_HANGUL */ 0x15 },
	// { XK_Junja,	/* VK_JUNJA */ 0x17 },
	// { XK_Final,	/* VK_FINAL */ 0x18 },
	// { XK_Hanja,	/* VK_HANJA */ 0x19 },
	// { XK_Kanji,	/* VK_KANJI */ 0x19 },
	{ XK_Escape,	/* VK_ESCAPE */ 0x1B },
	// { XK_Convert,	/* VK_CONVERT */ 0x1C },
	// { XK_Nonconvert,	/* VK_NONCONVERT */ 0x1D },
	// { XK_Accept,	/* VK_ACCEPT */ 0x1E },
	// { XK_Modechange,	/* VK_MODECHANGE */ 0x1F },
	{ XK_space,	/* VK_SPACE */ 0x20 },
	{ XK_Page_Up,	/* VK_PRIOR */ 0x21 },
	{ XK_Page_Down,	/* VK_NEXT */ 0x22 },
	{ XK_End,	/* VK_END */ 0x23 },
	{ XK_Home,	/* VK_HOME */ 0x24 },
	{ XK_Left,	/* VK_LEFT */ 0x25 },
	{ XK_Up,	/* VK_UP */ 0x26 },
	{ XK_Right,	/* VK_RIGHT */ 0x27 },
	{ XK_Down,	/* VK_DOWN */ 0x28 },
	{ XK_Select,	/* VK_SELECT */ 0x29 },
	{ XK_Print,	/* VK_PRINT */ 0x2A },
	{ XK_Execute,	/* VK_EXECUTE */ 0x2B },
	{ XK_Print,	/* VK_SNAPSHOT */ 0x2C },
	{ XK_Insert,	/* VK_INSERT */ 0x2D },
	{ XK_Delete,	/* VK_DELETE */ 0x2E },
	{ XK_Help,	/* VK_HELP */ 0x2F },
	{ XK_0,	/* 0 key */	0x30 },
	{ XK_1,	/* 1 key */	0x31 },
	{ XK_2,	/* 2 key */	0x32 },
	{ XK_3,	/* 3 key */	0x33 },
	{ XK_4,	/* 4 key */	0x34 },
	{ XK_5,	/* 5 key */	0x35 },
	{ XK_6,	/* 6 key */	0x36 },
	{ XK_7,	/* 7 key */	0x37 },
	{ XK_8,	/* 8 key */	0x38 },
	{ XK_9,	/* 9 key */	0x39 },
	{ XK_equal,	/* VK_EQUAL */ 0x3D },
	{ XK_a,	/* A key */	0x41 },
	{ XK_b,	/* B key */	0x42 },
	{ XK_c,	/* C key */	0x43 },
	{ XK_d,	/* D key */	0x44 },
	{ XK_e,	/* E key */	0x45 },
	{ XK_f,	/* F key */	0x46 },
	{ XK_g,	/* G key */	0x47 },
	{ XK_h,	/* H key */	0x48 },
	{ XK_i,	/* I key */	0x49 },
	{ XK_j,	/* J key */	0x4A },
	{ XK_k,	/* K key */	0x4B },
	{ XK_l,	/* L key */	0x4C },
	{ XK_m,	/* M key */	0x4D },
	{ XK_n,	/* N key */	0x4E },
	{ XK_o,	/* O key */	0x4F },
	{ XK_p,	/* P key */	0x50 },
	{ XK_q,	/* Q key */	0x51 },
	{ XK_r,	/* R key */	0x52 },
	{ XK_s,	/* S key */	0x53 },
	{ XK_t,	/* T key */	0x54 },
	{ XK_u,	/* U key */	0x55 },
	{ XK_v,	/* V key */	0x56 },
	{ XK_w,	/* W key */	0x57 },
	{ XK_x,	/* X key */	0x58 },
	{ XK_y,	/* Y key */	0x59 },
	{ XK_z,	/* Z key */	0x5A },
	{ XK_Super_L,	/* VK_LWIN */ 0x5B },
	{ XK_Super_R,	/* VK_RWIN */ 0x5C },
	// { XK_apps,	/* VK_APPS */ 0x5D },
	// { XK_sleep,	/* VK_SLEEP */ 0x5F },
	{ XK_KP_0,	/* VK_NUMPAD0 */ 0x60 },
	{ XK_KP_1,	/* VK_NUMPAD1 */ 0x61 },
	{ XK_KP_2,	/* VK_NUMPAD2 */ 0x62 },
	{ XK_KP_3,	/* VK_NUMPAD3 */ 0x63 },
	{ XK_KP_4,	/* VK_NUMPAD4 */ 0x64 },
	{ XK_KP_5,	/* VK_NUMPAD5 */ 0x65 },
	{ XK_KP_6,	/* VK_NUMPAD6 */ 0x66 },
	{ XK_KP_7,	/* VK_NUMPAD7 */ 0x67 },
	{ XK_KP_8,	/* VK_NUMPAD8 */ 0x68 },
	{ XK_KP_9,	/* VK_NUMPAD9 */ 0x69 },
	{ XK_KP_Multiply,	/* VK_MULTIPLY */ 	0x6A },
	{ XK_KP_Add,		/* VK_ADD */ 		0x6B },
	{ XK_KP_Separator,	/* VK_SEPARATOR */ 	0x6C },
	{ XK_KP_Subtract,	/* VK_SUBTRACT */ 	0x6D },
	{ XK_KP_Decimal,	/* VK_DECIMAL */ 	0x6E },
	{ XK_KP_Divide,		/* VK_DIVIDE */ 	0x6F },
	{ XK_KP_Enter,		/* VK_RETURN */ 	0x0D },
	{ XK_KP_Delete,		/* VK_DELETE */ 	0x6F },
	{ XK_KP_Page_Up,	/* VK_PRIOR */ 		0x69 },
	{ XK_KP_Page_Down,	/* VK_NEXT */ 		0x63 },
	{ XK_KP_End,		/* VK_END */ 		0x61 },
	{ XK_KP_Home,		/* VK_HOME */ 		0x67 },
	{ XK_KP_Left,		/* VK_LEFT */ 		0x64 },
	{ XK_KP_Begin,		/* VK_NUMPAD5 */ 	0x65 },
	{ XK_KP_Up,			/* VK_UP */ 		0x68 },
	{ XK_KP_Right,		/* VK_RIGHT */ 		0x66 },
	{ XK_KP_Down,		/* VK_DOWN */ 		0x62 },
	{ XK_KP_Insert,		/* VK_INSERT */ 	0x60 },
	{ XK_F1,	/* VK_F1 */ 0x70 },
	{ XK_F2,	/* VK_F2 */ 0x71 },
	{ XK_F3,	/* VK_F3 */ 0x72 },
	{ XK_F4,	/* VK_F4 */ 0x73 },
	{ XK_F5,	/* VK_F5 */ 0x74 },
	{ XK_F6,	/* VK_F6 */ 0x75 },
	{ XK_F7,	/* VK_F7 */ 0x76 },
	{ XK_F8,	/* VK_F8 */ 0x77 },
	{ XK_F9,	/* VK_F9 */ 0x78 },
	{ XK_F10,	/* VK_F10 */ 0x79 },
	{ XK_F11,	/* VK_F11 */ 0x7A },
	{ XK_F12,	/* VK_F12 */ 0x7B },
	{ XK_F13,	/* VK_F13 */ 0x7C },
	{ XK_F14,	/* VK_F14 */ 0x7D },
	{ XK_F15,	/* VK_F15 */ 0x7E },
	{ XK_F16,	/* VK_F16 */ 0x7F },
	{ XK_F17,	/* VK_F17 */ 0x80 },
	{ XK_F18,	/* VK_F18 */ 0x81 },
	{ XK_F19,	/* VK_F19 */ 0x82 },
	{ XK_F20,	/* VK_F20 */ 0x83 },
	{ XK_F21,	/* VK_F21 */ 0x84 },
	{ XK_F22,	/* VK_F22 */ 0x85 },
	{ XK_F23,	/* VK_F23 */ 0x86 },
	{ XK_F24,	/* VK_F24 */ 0x87 },
	// { XK_navigation_view,	/* VK_NAVIGATION_VIEW */ 0x88 },
	// { XK_navigation_menu,	/* VK_NAVIGATION_MENU */ 0x89 },
	// { XK_navigation_up,	/* VK_NAVIGATION_UP */ 0x8A },
	// { XK_navigation_down,	/* VK_NAVIGATION_DOWN */ 0x8B },
	// { XK_navigation_left,	/* VK_NAVIGATION_LEFT */ 0x8C },
	// { XK_navigation_right,	/* VK_NAVIGATION_RIGHT */ 0x8D },
	// { XK_navigation_accept,	/* VK_NAVIGATION_ACCEPT */ 0x8E },
	// { XK_navigation_cancel,	/* VK_NAVIGATION_CANCEL */ 0x8F },
	{ XK_Num_Lock,	/* VK_NUMLOCK */ 0x90 },
	{ XK_Scroll_Lock,	/* VK_SCROLL */ 0x91 },
	{ XK_Shift_L,	/* VK_LSHIFT */ 0xA0 },
	{ XK_Shift_R,	/* VK_RSHIFT */ 0xA1 },
	{ XK_Control_L,	/* VK_LCONTROL */ 0xA2 },
	{ XK_Control_R,	/* VK_RCONTROL */ 0xA3 },
	{ XK_Alt_L,	/* VK_LALT */ 0xA4 },
	{ XK_Alt_R,	/* VK_RALT */ 0xA5 },
	{ XK_Meta_L,	/* VK_LMENU */ 0xA4 },
	{ XK_Meta_R,	/* VK_RMENU */ 0xA5 },
	// { XK_browser_back,	/* VK_BROWSER_BACK */ 0xA6 },
	// { XK_browser_forward,	/* VK_BROWSER_FORWARD */ 0xA7 },
	// { XK_browser_refresh,	/* VK_BROWSER_REFRESH */ 0xA8 },
	// { XK_browser_stop,	/* VK_BROWSER_STOP */ 0xA9 },
	// { XK_browser_search,	/* VK_BROWSER_SEARCH */ 0xAA },
	// { XK_browser_favorites,	/* VK_BROWSER_FAVORITES */ 0xAB },
	// { XK_browser_home,	/* VK_BROWSER_HOME */ 0xAC },
	// { XK_volume_mute,	/* VK_VOLUME_MUTE */ 0xAD },
	// { XK_volume_down,	/* VK_VOLUME_DOWN */ 0xAE },
	// { XK_volume_up,	/* VK_VOLUME_UP */ 0xAF },
	// { XK_media_next_track,	/* VK_MEDIA_NEXT_TRACK */ 0xB0 },
	// { XK_media_prev_track,	/* VK_MEDIA_PREV_TRACK */ 0xB1 },
	// { XK_media_stop,	/* VK_MEDIA_STOP */ 0xB2 },
	// { XK_media_play_pause,	/* VK_MEDIA_PLAY_PAUSE */ 0xB3 },
	// { XK_launch_mail,	/* VK_LAUNCH_MAIL */ 0xB4 },
	// { XK_launch_media_select,	/* VK_LAUNCH_MEDIA_SELECT */ 0xB5 },
	// { XK_launch_app1,	/* VK_LAUNCH_APP1 */ 0xB6 },
	// { XK_launch_app2,	/* VK_LAUNCH_APP2 */ 0xB7 },
	{ XK_semicolon, /* VK_OEM_1 */	0xBA },   // ';:' for US
	{ XK_plus, /* VK_OEM_PLUS */	0xBB },   // '+' any country
	{ XK_comma, /* VK_OEM_COMMA */	0xBC },   // ',' any country
	{ XK_minus, /* VK_OEM_MINUS */	0xBD },   // '-' any country
	{ XK_period, /* VK_OEM_PERIOD */	0xBE },   // '.' any country
	{ XK_slash, /* VK_OEM_2 */	0xBF },   // '/?' for US
	{ XK_quoteleft, /* VK_OEM_3 */	0xC0 },   // '`~' for US
	{ XK_asciitilde, /* VK_OEM_3 */	0xC0 },   // '`~' for US
	{ XK_bracketleft, /* VK_OEM_4 */	0xDB },  //  '[{' for US
	{ XK_backslash, /* VK_OEM_5 */	0xDC },  //  '\|' for US
	{ XK_bracketright, /* VK_OEM_6 */	0xDD },  //  ']}' for US
	{ XK_apostrophe, /* VK_OEM_7 */	0xDE },  //  ''"' for US
	{ XK_less, 0xE2 }, // '<' for US
	// {  /* VK_OEM_8 */	0xDF },
	{ 0, 0 }
};

} // namespace ssdk::ctls
#endif
