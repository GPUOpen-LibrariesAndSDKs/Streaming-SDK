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

#include "HDREncoder.h"

namespace ssdk::video
{
    class GPUEncoderHEVC : public HDREncoder
    {
    public:
        GPUEncoderHEVC(amf::AMFContext* context);

        virtual AMF_RESULT Init(const AMFSize& encoderResolution, int64_t bitrate, float frameRate, int64_t intraRefreshPeriod, const ColorParameters& inputColorParams, size_t instance = 0) override;

        virtual const char* GetCodecName() const noexcept override;

        virtual AMF_RESULT GetNumOfEncoderInstances(size_t& numOfInstances) const override;

        virtual bool IsFormatSupported(amf::AMF_SURFACE_FORMAT defaultFormat, bool hdr, amf::AMFSurface* pSurface) const override;

        virtual AMF_RESULT UpdateBitrate(int64_t bitRate) override;
        virtual AMF_RESULT UpdateFramerate(const AMFRate& rate) override;

        virtual AMF_RESULT GetExtraData(amf::AMFBuffer** pExtraData) const override;
        virtual AMF_RESULT ForceKeyFrame(amf::AMFData* pSurface, bool bSet) override;
        virtual AMF_RESULT DetermineFrameType(amf::AMFBuffer* buffer, transport_common::VideoFrame::SubframeType& frameType) const override;

        virtual AMF_RESULT SetHDRMetadata(const amf::AMFVariant& metadata) override;
        virtual bool IsHDRSupported() const noexcept override;

    private:
        virtual void ToggleHDR(bool enable) override;
    };
}