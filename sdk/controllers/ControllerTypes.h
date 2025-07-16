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

namespace ssdk::ctls
{
    //----------------------------------------------------------------------------------------------
    // CtlEvent interface
    //----------------------------------------------------------------------------------------------
    struct CtlEvent
    {
        amf::AMFVariantStruct value;
        uint64_t              flags{0};
    };

    //----------------------------------------------------------------------------------------------
    enum class CTRL_DEVICE_TYPE
    {
        UNKNOWN = 0,
        DISPLAY = 1,
        VR_CONTROLLER = 2,
        MOUSE = 3,
        KEYBOARD = 4,
        GAME_CONTROLLER = 5,
        TOUCHSCREEN = 6
    };

    //----------------------------------------------------------------------------------------------
    enum class CTRL_DEVICE_STATE
    {
        CONNECTED = 0,
        DISCONNECTED = 1
    };

    //----------------------------------------------------------------------------------------------
    enum POSE_VALIDITY_FLAG_BITS
    {
        // validity flags
        POSE_VALIDITY_NONE                      = 0x0000,
        POSE_VALIDITY_ORIENTATION               = 0x0001,
        POSE_VALIDITY_POSITION                  = 0x0002,
        POSE_VALIDITY_ORIENTATION_VELOCITY      = 0x0004,
        POSE_VALIDITY_POSITION_VELOCITY         = 0x0008,
        POSE_VALIDITY_ORIENTATION_ACCELERATION  = 0x0010,
        POSE_VALIDITY_POSITION_ACCELERATION     = 0x0020,
        POSE_VALIDITY_COORDINATES = POSE_VALIDITY_ORIENTATION | POSE_VALIDITY_POSITION,
        POSE_VALIDITY_VELOCITY = POSE_VALIDITY_ORIENTATION_VELOCITY | POSE_VALIDITY_POSITION_VELOCITY,
        POSE_VALIDITY_ACCELERATION = POSE_VALIDITY_ORIENTATION_ACCELERATION | POSE_VALIDITY_POSITION_ACCELERATION,
        POSE_VALIDITY_ALL = POSE_VALIDITY_COORDINATES | POSE_VALIDITY_VELOCITY | POSE_VALIDITY_ACCELERATION,
    };
    typedef uint32_t POSE_VALIDITY_FLAGS;

    //----------------------------------------------------------------------------------------------
    typedef struct CtlPose
    {
        amf_uint64          id;
        POSE_VALIDITY_FLAGS validity;

        AMFFloatPoint3D     position;
        AMFFloatVector4D    orientation;

        AMFFloatPoint3D     positionVelocity;
        AMFFloatPoint3D     orientationVelocity;
        AMFFloatPoint3D     positionAcceleration;
        AMFFloatPoint3D     orientationAcceleration;
    } CtlPose;

} // namespace ssdk::ctls
