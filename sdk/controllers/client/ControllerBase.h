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

#include "amf/public/common/InterfaceImpl.h"
#include "amf/public/common/PropertyStorageExImpl.h"
#include "../ControllerTypes.h"
#include "../UserInput.h"

#if defined(_WIN32)
#include <xinput.h>
typedef MSG WindowMessage;
typedef POINT WindowPoint;
#else
#include <X11/Xlib.h>
#include <X11/extensions/XI2.h>
#include <X11/extensions/XInput2.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <libevdev-1.0/libevdev/libevdev.h>
typedef XEvent WindowMessage;
typedef XIDeviceEvent ExtWindowMessage;
typedef struct {
    int x;
    int y;
} WindowPoint;
#endif

namespace ssdk::ctls
{
    //----------------------------------------------------------------------------------------------
    // ControllerBase: base class for input devices
    //----------------------------------------------------------------------------------------------
    class ControllerBase;
    class ControllerConfig
    {
    public:
        virtual ~ControllerConfig() {};
        virtual void AMF_STD_CALL Attach(ControllerBase* observer);
        virtual void AMF_STD_CALL Detach(ControllerBase* observer);
        virtual void AMF_STD_CALL Notify();

    protected:
        mutable amf::AMFCriticalSection m_Guard;
        std::list<ControllerBase*> m_Observers;
    };
    //----------------------------------------------------------------------------------------------
    typedef std::shared_ptr<ControllerConfig> ControllerConfigPtr;
    //----------------------------------------------------------------------------------------------

    //----------------------------------------------------------------------------------------------
    class ControllerManager;
    typedef std::shared_ptr<ControllerManager> ControllerManagerPtr;
    //----------------------------------------------------------------------------------------------
    class ControllerBase
    {
    public:
        ControllerBase(ControllerManagerPtr pControllerManager);
        virtual ~ControllerBase();

        typedef std::shared_ptr<ControllerBase> Ptr;

        virtual const char*       AMF_STD_CALL GetID() const { return m_ID.c_str(); }
        virtual AMF_RESULT        AMF_STD_CALL SetID(const char* id) { m_ID = id; return AMF_OK; }
        virtual CTRL_DEVICE_STATE AMF_STD_CALL GetState() const { return CTRL_DEVICE_STATE::CONNECTED; }
        virtual CTRL_DEVICE_TYPE  AMF_STD_CALL GetType() const = 0;
        virtual AMF_RESULT        AMF_STD_CALL Update() = 0;

        // ControllerBase interface
        virtual AMF_RESULT  AMF_STD_CALL ProcessMessage(const WindowMessage& /*msg*/) { return AMF_FAIL; }
#if !defined(_WIN32)
        virtual AMF_RESULT  AMF_STD_CALL ProcessMessageExt(const ExtWindowMessage& /*msg*/) { return AMF_FAIL; }
#endif
        virtual void        AMF_STD_CALL ProcessInputEvent(const char* /*eventID*/, const amf::AMFVariant& /*event*/) {};
        virtual void        AMF_STD_CALL OnConnectionEstablished() {};
        virtual void        AMF_STD_CALL OnConnectionTerminated() {};
        virtual void        AMF_STD_CALL UpdateSensors() {};

        // Set/Get Config
        void AMF_STD_CALL SetConfig(const ControllerConfigPtr& pConfig);
        ControllerConfigPtr AMF_STD_CALL GetConfig() const;
        // Called by Subject (Config) to update controller once config has been changed
        virtual void AMF_STD_CALL OnConfigUpdated();

    protected:

        std::string                 m_ID;
        std::string                 m_Description;
        std::vector<std::string>    m_EventIDs;
        ControllerConfigPtr         m_pConfig;
        ControllerManagerPtr        m_pControllerManager;

    private:
        mutable amf::AMFCriticalSection m_GuardConfig;
        amf::AMFCriticalSection m_GuardCallback;
    };
}// namespace ssdk::ctls
