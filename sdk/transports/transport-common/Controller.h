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

#pragma once

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#include <cstdint>

namespace ssdk::transport_common
{
    enum class ControllerType
    {
        UNKNOWN = 0,
        TRACKABLE_DEVICE = 1,   //  HMD, head tracker, anything that tracks its position/orienation in space on its own
        VR_CONTROLLER = 2,
        MOUSE = 3,
        KEYBOARD = 4,
        GAME_CONTROLLER = 5,
        TOUCHSCREEN = 6,
    };

    class Pose
    {
    protected:
        const int VALIDITY_NONE = 0x0000;
        const int VALIDITY_ORIENTATION = 0x0001;
        const int VALIDITY_POSITION = 0x0002;
        const int VALIDITY_ORIENTATION_VELOCITY = 0x0004;
        const int VALIDITY_POSITION_VELOCITY = 0x0008;
        const int VALIDITY_ORIENTATION_ACCELERATION = 0x0010;
        const int VALIDITY_POSITION_ACCELERATION = 0x0020;
        const int VALIDITY_COORDINATES = VALIDITY_ORIENTATION | VALIDITY_POSITION;
        const int VALIDITY_VELOCITY = VALIDITY_ORIENTATION_VELOCITY | VALIDITY_POSITION_VELOCITY;
        const int VALIDITY_ACCELERATION = VALIDITY_ORIENTATION_ACCELERATION | VALIDITY_POSITION_ACCELERATION;
        const int VALIDITY_ALL = VALIDITY_COORDINATES | VALIDITY_VELOCITY | VALIDITY_ACCELERATION;

    public:
        Pose() = default;
        explicit Pose(int64_t id, const AMFFloatVector4D& orientation);
        explicit Pose(int64_t id, const AMFFloatPoint3D& position);
        explicit Pose(int64_t id, const AMFFloatVector4D& orientation, const AMFFloatPoint3D& position);
        explicit Pose(int64_t id, const AMFFloatVector4D& orientation, const AMFFloatPoint3D& position,
                      const AMFFloatPoint3D& orientationVelocity, const AMFFloatPoint3D& orientationAccel,
                      const AMFFloatPoint3D& positionVelocity, const AMFFloatPoint3D& positionAccel);

        inline bool IsPosition() const noexcept { return (m_Validity & VALIDITY_POSITION) != 0; }
        inline const AMFFloatPoint3D& GetPosition() const noexcept { return m_Position; }

        inline bool IsPositionVelocity() const noexcept { return (m_Validity & VALIDITY_POSITION_VELOCITY) != 0; }
        inline const AMFFloatPoint3D& GetPositionVelocity() const noexcept { return m_PositionVelocity; }

        inline bool IsPositionAcceleration() const noexcept { return (m_Validity & VALIDITY_POSITION_ACCELERATION) != 0; }
        inline const AMFFloatPoint3D& GetPositionAcceleration() const noexcept { return m_PositionAcceleration; }

        inline bool IsOrientation() const noexcept { return (m_Validity & VALIDITY_ORIENTATION) != 0; }
        inline const AMFFloatVector4D& GetOrientation() const noexcept { return m_Orientation; }

        inline bool IsOrientationVelocity() const noexcept { return (m_Validity & VALIDITY_ORIENTATION_VELOCITY) != 0; }
        inline const AMFFloatPoint3D& GetOrientationVelocity() const noexcept { return m_OrientationVelocity; }

        inline bool IsOrientationAcceleration() const noexcept { return (m_Validity & VALIDITY_ORIENTATION_ACCELERATION) != 0; }
        inline const AMFFloatPoint3D& GetOrientationAcceleration() const noexcept { return m_OrientationAcceleration; }

        inline int64_t GetID() const noexcept { return m_ID; }

    private:
        int64_t                             m_ID = 0;
        int                                 m_Validity = VALIDITY_NONE;

        AMFFloatPoint3D                     m_Position = {};
        AMFFloatVector4D                    m_Orientation = {};

        AMFFloatPoint3D                     m_PositionVelocity = {};
        AMFFloatPoint3D                     m_OrientationVelocity = {};
        AMFFloatPoint3D                     m_PositionAcceleration = {};
        AMFFloatPoint3D                     m_OrientationAcceleration = {};
    };
}