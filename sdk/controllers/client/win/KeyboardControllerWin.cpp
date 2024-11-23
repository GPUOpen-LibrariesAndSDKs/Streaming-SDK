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
#if defined(_WIN32)

#include "../KeyboardController.h"
#include "../ControllerManager.h"

namespace ssdk::ctls
{
    //-------------------------------------------------------------------------------------------------
    AMF_RESULT KeyboardController::ProcessMessage(const WindowMessage& msg)
    {
        AMF_RESULT result = AMF_FAIL;
        if (msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST)
        {
            CtlEvent ev = {};
            ev.value.type = amf::AMF_VARIANT_INT64;
            std::string id;
            switch (msg.message)
            {
            case WM_KEYDOWN:
                id = m_EventIDs[DEVICE_KEYBOARD_INDEX].c_str();

                ev.value.int64Value = amf_int64((msg.wParam) & 0xFFFF);   // virtual key
                ev.value.int64Value |= (((msg.lParam >> 16) & 0xFF) << 16);     // scan code

                if (msg.lParam & 0x1000000)
                {
                    ev.value.int64Value |= DEVICE_KEYBOARD_EXTENDED;
                }
                break;
            case WM_KEYUP:
                id = m_EventIDs[DEVICE_KEYBOARD_INDEX].c_str();

                ev.value.int64Value = amf_int64((msg.wParam) & 0xFFFF);   // virtual key
                ev.value.int64Value |= (((msg.lParam >> 16) & 0xFF) << 16);     // scan code

                ev.value.int64Value |= DEVICE_KEYBOARD_UP;
                if (msg.lParam & 0x1000000)
                {
                    ev.value.int64Value |= DEVICE_KEYBOARD_EXTENDED;
                }
                break;
            }
            if (id.size() != 0)
            {
                if (m_pControllerManager != nullptr)
                {
                    m_pControllerManager->SendControllerEvent(id.c_str(), &ev);
                }

                result = AMF_OK;
            }
        }

        return result;
    }

    //-------------------------------------------------------------------------------------------------
    AMF_RESULT KeyboardController::ReleaseModifiersKey()
    {
        return AMF_OK;
    }
}// namespace ssdk::ctls
#endif
