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
// Copyright (c) 2018 Advanced Micro Devices, Inc. All rights reserved.
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

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#include "../Defines.h"
#include "transports/transport-common/Video.h"
#include "amf/public/include/core/Context.h"
#include "amf/public/include/core/Buffer.h"
#include "amf/public/include/components/Component.h"
#include "amf/public/include/components/ColorSpace.h"

#include <memory>

namespace ssdk::video
{
    class ColorParameters
    {
    public:
        ColorParameters() = default;

        inline AMF_COLOR_TRANSFER_CHARACTERISTIC_ENUM GetXferCharacteristic() const noexcept { return m_XferChar; }
        inline void SetXferCharacteristic(AMF_COLOR_TRANSFER_CHARACTERISTIC_ENUM val) noexcept { m_XferChar = val; }

        inline AMF_COLOR_PRIMARIES_ENUM GetColorPrimaries() const noexcept { return m_ColorPrimaries; }
        inline void SetColorPrimaries(AMF_COLOR_PRIMARIES_ENUM val) noexcept { m_ColorPrimaries = val; }

        inline AMF_VIDEO_CONVERTER_COLOR_PROFILE_ENUM GetColorProfile() const noexcept { return m_ColorProfile; }
        inline void SetColorProfile(AMF_VIDEO_CONVERTER_COLOR_PROFILE_ENUM val) noexcept { m_ColorProfile = val; }

        inline amf::AMF_SURFACE_FORMAT GetFormat() const noexcept { return m_Format; }
        inline void SetFormat(amf::AMF_SURFACE_FORMAT val) noexcept { m_Format = val; }
    private:
        AMF_COLOR_TRANSFER_CHARACTERISTIC_ENUM m_XferChar = AMF_COLOR_TRANSFER_CHARACTERISTIC_UNDEFINED;
        AMF_COLOR_PRIMARIES_ENUM m_ColorPrimaries = AMF_COLOR_PRIMARIES_UNDEFINED;
        AMF_VIDEO_CONVERTER_COLOR_PROFILE_ENUM m_ColorProfile = AMF_VIDEO_CONVERTER_COLOR_PROFILE_UNKNOWN;
        amf::AMF_SURFACE_FORMAT m_Format = amf::AMF_SURFACE_UNKNOWN;
    };


    class VideoEncodeEngine
    {
    public:
        typedef std::shared_ptr<VideoEncodeEngine>  Ptr;

    protected:
        VideoEncodeEngine(amf::AMFContext* context);

    private:
        VideoEncodeEngine(const VideoEncodeEngine&) = delete;
        VideoEncodeEngine& operator=(const VideoEncodeEngine&) = delete;

    public:
        virtual ~VideoEncodeEngine();

        virtual AMF_RESULT Init(const AMFSize& encoderResolution, int64_t bitrate, float frameRate, int64_t intraRefreshPeriod, const ColorParameters& inputColorParams, size_t instance = 0);
        void Terminate();

        virtual AMF_RESULT SubmitInput(amf::AMFSurface* inputSurface);
        AMF_RESULT QueryOutput(amf::AMFBuffer** outputBuffer, transport_common::VideoFrame::SubframeType& frameType);
        AMF_RESULT Flush();
        AMF_RESULT Drain();

        AMF_RESULT SetProperty(const wchar_t* name, const amf::AMFVariant& value);
        AMF_RESULT GetProperty(const wchar_t* name, amf::AMFVariant& value) const;

        virtual const char* GetCodecName() const noexcept = 0;

        virtual AMF_RESULT GetNumOfEncoderInstances(size_t& numOfInstances) const = 0;

        virtual bool IsFormatSupported(amf::AMF_SURFACE_FORMAT defaultFormat, bool hdr, amf::AMFSurface* pSurface) const;

        virtual AMF_RESULT GetExtraData(amf::AMFBuffer** pExtraData) const = 0;
        virtual AMF_RESULT ForceKeyFrame(amf::AMFData* pSurface, bool bSet = true) = 0;
        virtual AMF_RESULT UpdateBitrate(int64_t bitRate) = 0;
        virtual AMF_RESULT UpdateFramerate(const AMFRate& rate) = 0;

        virtual bool IsHDRSupported() const noexcept = 0;
        virtual bool IsHDREnabled() const noexcept { return false; }
        virtual AMF_RESULT EnableHDR(bool enable) noexcept = 0;

        inline amf::AMF_SURFACE_FORMAT GetPreferredSDRFormat() const noexcept { return m_PreferredSDRFormat; }
        inline amf::AMF_SURFACE_FORMAT GetPreferredHDRFormat() const noexcept { return m_PreferredHDRFormat; }

        virtual AMF_COLOR_RANGE_ENUM GetSupportedColorRange() const noexcept;

        inline int64_t  GetBitrate() const noexcept { return m_Bitrate; }
        inline float    GetFramerate() const noexcept { return m_Framerate; }
        inline const AMFSize& GetResolution() const noexcept { return m_Resolution; }
        inline int64_t  GetIntraRefreshPeriod() const noexcept { return m_IntraRefreshPeriod; }
        inline const ColorParameters& GetInputColorParameters() const noexcept { return m_InputColorParams; }

    protected:
        static uint64_t AlignValue(uint64_t value, uint64_t alignment);
        AMF_RESULT GetNumOfEncoderInstancesPrivate(const wchar_t* codec, const wchar_t* propName, size_t& numOfInstances) const;
        virtual AMF_RESULT DetermineFrameType(amf::AMFBuffer* buffer, transport_common::VideoFrame::SubframeType& frameType) const = 0;

    protected:
        amf::AMFContextPtr      m_Context;
        amf::AMFComponentPtr    m_Encoder;
        amf::AMF_SURFACE_FORMAT m_PreferredSDRFormat = amf::AMF_SURFACE_UNKNOWN,
                                m_PreferredHDRFormat = amf::AMF_SURFACE_UNKNOWN;
        int64_t                 m_Bitrate = 10000;
        AMFSize                 m_Resolution = { 0, 0 };
        float                   m_Framerate = 60;
        int64_t                 m_IntraRefreshPeriod = 0;
        ColorParameters         m_InputColorParams;
        size_t                  m_Instance = 0;
        bool                    m_Initialized = false;
    };

}