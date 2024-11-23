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

#include "ControllerBase.h"
#include "amf/public/common/AMFMath.h"

namespace ssdk::ctls
{
    //----------------------------------------------------------------------------------------------
    // Tracked object interface (VR controllers and HMDs)
    //----------------------------------------------------------------------------------------------
    class AMF_NO_VTABLE TrackedDeviceCallback
    {
    public:
        virtual void AMF_STD_CALL OnNewPose(const char* deviceID, CtlPose* pPose) = 0;
    };

    //----------------------------------------------------------------------------------------------
    // TrackedHeadset (VR controllers and HMDs)
    //----------------------------------------------------------------------------------------------
    class AMF_NO_VTABLE TrackedHeadset
    {
    public:
        typedef std::shared_ptr<TrackedHeadset> Ptr;

        TrackedHeadset() {}
        ~TrackedHeadset() {}

        // Device interface;
        virtual CTRL_DEVICE_TYPE    AMF_STD_CALL GetType() const { return CTRL_DEVICE_TYPE::DISPLAY; }
        virtual const char*         AMF_STD_CALL GetID() const { return DEVICE_HMD; }
        virtual AMF_RESULT          AMF_STD_CALL SetID(const char*) { return AMF_OK; }
        virtual CTRL_DEVICE_STATE   AMF_STD_CALL GetState() const { return CTRL_DEVICE_STATE::CONNECTED; }
        virtual AMF_RESULT          AMF_STD_CALL Update();

        // TrackedDevice interface
        virtual AMF_RESULT  AMF_STD_CALL AddCallback(TrackedDeviceCallback* pCallback);
        virtual AMF_RESULT  AMF_STD_CALL RemoveCallback(TrackedDeviceCallback* pCallback);

        virtual AMF_RESULT  AMF_STD_CALL SendHMDTrackingInfo(WindowPoint diff);
        virtual AMF_RESULT  AMF_STD_CALL GetPose(CtlPose* pPose);

    private:
        typedef std::vector<TrackedDeviceCallback*> CallbackList;

        CallbackList GetCallbacks() // always use copy to avoid thread issues
        {
            amf::AMFLock lock(&m_GuardCallback);
            return m_Callbacks;
        }
        amf::AMFCriticalSection m_GuardCallback;
        CallbackList            m_Callbacks;
        amf::Quaternion         m_LastOrientation;
    };
}
