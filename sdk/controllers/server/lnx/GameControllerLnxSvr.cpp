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

#include "GameControllerLnxSvr.h"
#include "amf/public/common/TraceAdapter.h"

#include <libevdev-1.0/libevdev/libevdev.h>
#include <libevdev-1.0/libevdev/libevdev-uinput.h>
#include <fcntl.h>

namespace ssdk::ctls::svr
{
    static constexpr const wchar_t* AMF_FACILITY = L"GameControllerLnxSvr";

    //-------------------------------------------------------------------------------------------------
    // GameControllerLnx
    //-------------------------------------------------------------------------------------------------
    GameControllerLnx::GameControllerLnx(ControllerManagerPtr pControllerManager, int8_t iIndex) :
        GameController(pControllerManager, iIndex)
    {
        // If AMFXInput interface is not implemented for Linux then use evdev
        if (m_pXInputController == nullptr)
        {
            m_dev = libevdev_new();
            std::string ctrlName = "Virtual Xbox Controller " + std::to_string(m_iIndex);
            libevdev_set_name(m_dev, ctrlName.c_str());

            // Enable every game pad code for this device defined in input-event-codes.h
            libevdev_enable_event_type(m_dev, EV_KEY);
            libevdev_enable_event_type(m_dev, EV_ABS); 

            // Enabled controller buttons
            libevdev_enable_event_code(m_dev, EV_KEY, BTN_A, nullptr);
            libevdev_enable_event_code(m_dev, EV_KEY, BTN_B, nullptr);
            libevdev_enable_event_code(m_dev, EV_KEY, BTN_X, nullptr);
            libevdev_enable_event_code(m_dev, EV_KEY, BTN_Y, nullptr);

            libevdev_enable_event_code(m_dev, EV_KEY, BTN_TL, nullptr);
            libevdev_enable_event_code(m_dev, EV_KEY, BTN_TR, nullptr);

            libevdev_enable_event_code(m_dev, EV_KEY, BTN_TL2, nullptr);
            libevdev_enable_event_code(m_dev, EV_KEY, BTN_TR2, nullptr);

            libevdev_enable_event_code(m_dev, EV_KEY, BTN_SELECT, nullptr);
            libevdev_enable_event_code(m_dev, EV_KEY, BTN_START, nullptr);

            libevdev_enable_event_code(m_dev, EV_KEY, BTN_THUMBL, nullptr);
            libevdev_enable_event_code(m_dev, EV_KEY, BTN_THUMBR, nullptr);

            libevdev_enable_event_code(m_dev, EV_KEY, BTN_DPAD_UP, nullptr);
            libevdev_enable_event_code(m_dev, EV_KEY, BTN_DPAD_DOWN, nullptr);
            libevdev_enable_event_code(m_dev, EV_KEY, BTN_DPAD_LEFT, nullptr);
            libevdev_enable_event_code(m_dev, EV_KEY, BTN_DPAD_RIGHT, nullptr);
            
            // Enable controller analog buttons
            // Enable ABS_X
            struct input_absinfo abs_x = {0};
            abs_x.minimum = -32768;
            abs_x.maximum = 32767;  // range for a joystick
            libevdev_enable_event_code(m_dev, EV_ABS, ABS_X, &abs_x);
            // Enable ABS_Y
            struct input_absinfo abs_y = {0};
            abs_y.minimum = -32768;
            abs_y.maximum = 32767;  // range for a joystick
            libevdev_enable_event_code(m_dev, EV_ABS, ABS_Y, &abs_y);

            // Enable ABS_RX
            struct input_absinfo abs_rx = {0};
            abs_rx.minimum = -32768;
            abs_rx.maximum = 32767;  // range for a joystick
            libevdev_enable_event_code(m_dev, EV_ABS, ABS_RX, &abs_rx);
            // Enable ABS_RY
            struct input_absinfo abs_ry = {0};
            abs_ry.minimum = -32768;
            abs_ry.maximum = 32767;  // range for a joystick
            libevdev_enable_event_code(m_dev, EV_ABS, ABS_RY, &abs_ry);

            // Enable ABS_Z
            struct input_absinfo abs_z = {0};
            abs_z.minimum = -256;
            abs_z.maximum = 255;  // range for a triggers
            libevdev_enable_event_code(m_dev, EV_ABS, ABS_Z, &abs_z);
            // Enable ABS_RZ
            struct input_absinfo abs_rz = {0};
            abs_rz.minimum = -256;
            abs_rz.maximum = 255;  // range for a triggers
            libevdev_enable_event_code(m_dev, EV_ABS, ABS_RZ, &abs_rz);

            int status = libevdev_uinput_create_from_device(m_dev, LIBEVDEV_UINPUT_OPEN_MANAGED, &m_uidev);
            if (status != 0)
            {
                AMFTraceError(AMF_FACILITY, L"GameControllerLnxSvr - could not create uinput from evdev device");
            }
        }
    }

    GameControllerLnx::~GameControllerLnx()
    {
        libevdev_uinput_destroy(m_uidev);
        libevdev_free(m_dev);
    }

    wchar_t* wstrerror(int errnum)
    {
        // Get the narrow string error message
        const char* error_message = strerror(errnum);
        // Determine the required buffer size
        size_t size = mbstowcs(NULL, error_message, 0) + 1;

        if (size == (size_t)-1)
        {
            // Handle conversion error
            return NULL;
        }

        // Allocate memory for the wide-string
        wchar_t* werror_message = (wchar_t*)malloc(size * sizeof(wchar_t));
        if (!werror_message)
        {
            // Handle memory allocation error
            return NULL;
        }

        // Convert to wide character string
        mbstowcs(werror_message, error_message, size);

        return werror_message;
    }

    void GameControllerLnx::SendEventCode(unsigned int type, unsigned int keyCode, int value)
    {
        if (m_uidev != nullptr)
        {
            //AMFTraceInfo(AMF_FACILITY, L"GameControllerLnx: Event key code - %d, value - %d", keyCode, value);
            int rc = libevdev_uinput_write_event(m_uidev, type, keyCode, value);
            if (rc < 0)
            {
                AMFTraceWarning(AMF_FACILITY, L"GameControllerLnx: Error writing event - %s", wstrerror(-rc));
            }
            else
            {
                rc = libevdev_uinput_write_event(m_uidev, EV_SYN, SYN_REPORT, 0);
                if (rc < 0)
                {
                    AMFTraceWarning(AMF_FACILITY, L"GameControllerLnx: Error writing event - %s", wstrerror(-rc));
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------------
    void GameControllerLnx::ProcessInputEvent(const char* eventID, const ssdk::ctls::CtlEvent& event)
    {
        // If Input Event is Game Controller actions (from Client)

        // If AMFXInput interface implemented for Linux then use it
        if (m_pXInputController != nullptr)
        {
            return GameController::ProcessInputEvent(eventID, event);
        }

        // Process by using libevdev uinput
        if (strcmp(eventID, DEVICE_CTRL_TRIGGER_LEFT_VALUE) == 0)
        {
            SendEventCode(EV_ABS, ABS_Z, int(event.value.floatValue*255.f));
        }
        else if (strcmp(eventID, DEVICE_CTRL_TRIGGER_RIGHT_VALUE) == 0)
        {
            SendEventCode(EV_ABS, ABS_RZ, int(event.value.floatValue*255.f));
        }
        else if (strcmp(eventID, DEVICE_CTRL_JOYSTICK_LEFT_VALUE) == 0)
        {
            SendEventCode(EV_ABS, ABS_X, int(event.value.floatPoint2DValue.x*32767.f));
            SendEventCode(EV_ABS, ABS_Y, -int(event.value.floatPoint2DValue.y*32767.f));
        }
        else if (strcmp(eventID, DEVICE_CTRL_JOYSTICK_RIGHT_VALUE) == 0)
        {
            SendEventCode(EV_ABS, ABS_RX, int(event.value.floatPoint2DValue.x*32767.f));
            SendEventCode(EV_ABS, ABS_RY, -int(event.value.floatPoint2DValue.y*32767.f));
        }
        else if (strcmp(eventID, DEVICE_CTRL_DPAD_UP_CLICK) == 0)
        {
            SendEventCode(EV_KEY, BTN_DPAD_UP, event.value.boolValue);
        }
        else if (strcmp(eventID, DEVICE_CTRL_DPAD_DOWN_CLICK) == 0)
        {
            SendEventCode(EV_KEY, BTN_DPAD_DOWN, event.value.boolValue);
        }
        else if (strcmp(eventID, DEVICE_CTRL_DPAD_LEFT_CLICK) == 0)
        {
            SendEventCode(EV_KEY, BTN_DPAD_LEFT, event.value.boolValue);
        }
        else if (strcmp(eventID, DEVICE_CTRL_DPAD_RIGHT_CLICK) == 0)
        {
            SendEventCode(EV_KEY, BTN_DPAD_RIGHT, event.value.boolValue);
        }
        else if (strcmp(eventID, DEVICE_CTRL_JOYSTICK_LEFT_CLICK) == 0)
        {
            SendEventCode(EV_KEY, BTN_THUMBL, event.value.boolValue);
        }
        else if (strcmp(eventID, DEVICE_CTRL_JOYSTICK_RIGHT_CLICK) == 0)
        {
            SendEventCode(EV_KEY, BTN_THUMBR, event.value.boolValue);
        }
        else if (strcmp(eventID, DEVICE_CTRL_SYSTEM_CLICK) == 0)
        {
            SendEventCode(EV_KEY, BTN_START, event.value.boolValue);
        }
        else if (strcmp(eventID, DEVICE_CTRL_APPMENU_CLICK) == 0)
        {
            SendEventCode(EV_KEY, BTN_SELECT, event.value.boolValue);
        }
        else if (strcmp(eventID, DEVICE_CTRL_GRIP_LEFT_CLICK) == 0)
        {
            SendEventCode(EV_KEY, BTN_TL, event.value.boolValue);
        }
        else if (strcmp(eventID, DEVICE_CTRL_GRIP_RIGHT_CLICK) == 0)
        {
            SendEventCode(EV_KEY, BTN_TR, event.value.boolValue);
        }
        else if (strcmp(eventID, DEVICE_CTRL_A_CLICK) == 0)
        {
            SendEventCode(EV_KEY, BTN_A, event.value.boolValue);
        }
        else if (strcmp(eventID, DEVICE_CTRL_B_CLICK) == 0)
        {
            SendEventCode(EV_KEY, BTN_B, event.value.boolValue);
        }
        else if (strcmp(eventID, DEVICE_CTRL_X_CLICK) == 0)
        {
            SendEventCode(EV_KEY, BTN_X, event.value.boolValue);
        }
        else if (strcmp(eventID, DEVICE_CTRL_Y_CLICK) == 0)
        {
            SendEventCode(EV_KEY, BTN_Y, event.value.boolValue);
        }
    }

}// namespace ssdk::ctls::svr

#endif // __linux
