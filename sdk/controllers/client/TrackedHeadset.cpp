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

#include "TrackedHeadset.h"

namespace ssdk::ctls
{

    AMF_RESULT AMF_STD_CALL TrackedHeadset::SendHMDTrackingInfo(WindowPoint diff)
    {
        CallbackList callbacks = GetCallbacks();
        if (callbacks.size() > 0)
        {
            static const float koeff = 0.05f;

            CtlPose pose = {};
            pose.id = amf_high_precision_clock();
            pose.validity = POSE_VALIDITY_ORIENTATION;
            //    amf::Quaternion quat(float(diff.y) * amf::AMF_PI / 180.f, float(diff.x) * amf::AMF_PI / 180.f , 0); // float pitch, float yaw, float roll
            //    amf::Quaternion quat(0, koeff * float(diff.x) * amf::AMF_PI / 180.f, koeff * float(diff.y) * amf::AMF_PI / 180.f); // float pitch, float yaw, float roll
            amf::Quaternion quat(0, koeff * float(diff.x) * amf::AMF_PI / 180.f, koeff * float(diff.y) * amf::AMF_PI / 180.f); // float pitch, float yaw, float roll

            m_LastOrientation = m_LastOrientation * quat;
            pose.orientation.x = m_LastOrientation.x;
            pose.orientation.y = m_LastOrientation.y;
            pose.orientation.z = m_LastOrientation.z;
            pose.orientation.w = m_LastOrientation.w;

            std::string id(DEVICE_HMD);
            id += DEVICE_POSE;
            for (CallbackList::iterator it = callbacks.begin(); it != callbacks.end(); it++)
            {
                (*it)->OnNewPose(id.c_str(), &pose);
            }
        }
        return AMF_OK;
    }

    //-------------------------------------------------------------------------------------------------
    AMF_RESULT AMF_STD_CALL TrackedHeadset::AddCallback(TrackedDeviceCallback* pCallback)
    {
        amf::AMFLock lock(&m_GuardCallback);
        for (CallbackList::iterator it = m_Callbacks.begin(); it != m_Callbacks.end(); it++)
        {
            if ((*it) == pCallback)
            {
                return AMF_ALREADY_INITIALIZED;
            }
        }
        m_Callbacks.push_back(pCallback);
        return AMF_OK;
    }

    //-------------------------------------------------------------------------------------------------
    AMF_RESULT AMF_STD_CALL TrackedHeadset::RemoveCallback(TrackedDeviceCallback* pCallback)
    {
        amf::AMFLock lock(&m_GuardCallback);
        for (CallbackList::iterator it = m_Callbacks.begin(); it != m_Callbacks.end(); it++)
        {
            if ((*it) == pCallback)
            {
                m_Callbacks.erase(it);
                return AMF_OK;
            }
        }
        return AMF_NOT_FOUND;
    }

    //-------------------------------------------------------------------------------------------------
    AMF_RESULT AMF_STD_CALL TrackedHeadset::Update()
    {
        return AMF_OK;
    }

    //-------------------------------------------------------------------------------------------------
    AMF_RESULT AMF_STD_CALL GetPose(CtlPose* /*pPose*/)
    {
        return AMF_OK;
    }
}
