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

#include "util/stats/ComponentStats.h"

#include "amf/public/include/core/Context.h"
#include "amf/public/include/core/Buffer.h"
#include "amf/public/include/components/Component.h"
#include "amf/public/common/Thread.h"

#include <memory>
#include <vector>

namespace ssdk::video
{
    class VideoDecodeEngine
    {
    public:
        typedef std::shared_ptr<VideoDecodeEngine>  Ptr;


    protected:
        VideoDecodeEngine(amf::AMFContext* context);

    private:
        VideoDecodeEngine(const VideoDecodeEngine&) = delete;
        VideoDecodeEngine& operator=(const VideoDecodeEngine&) = delete;

    public:
        virtual ~VideoDecodeEngine();

        virtual AMF_RESULT Init(amf::AMF_SURFACE_FORMAT format, const AMFSize& resolution, float frameRate, amf::AMFBuffer* initBlock);
        virtual AMF_RESULT Terminate();

        virtual const char* GetCodecName() const noexcept = 0;

        inline amf::AMF_SURFACE_FORMAT GetOutputFormat() const noexcept { return m_Format; }
        inline const AMFSize& GetResolution() const noexcept { return m_Resolution; }
        inline float GetFrameRate() const noexcept { return m_FrameRate; }

        virtual bool IsHDRSupported() const noexcept = 0;

        AMF_RESULT SubmitInput(amf::AMFBuffer* input, transport_common::VideoFrame::SubframeType frameType);
        AMF_RESULT QueryOutput(amf::AMFSurface** output);

    protected:
        mutable amf::AMFCriticalSection     m_Guard;
        mutable amf::AMFCriticalSection     m_InputGuard;
        amf::AMFContextPtr      m_Context;
        amf::AMFComponentPtr    m_Decoder;
        amf::AMF_SURFACE_FORMAT m_Format = amf::AMF_SURFACE_FORMAT::AMF_SURFACE_UNKNOWN;
        AMFSize                 m_Resolution = {};
        float                   m_FrameRate = 0;
        bool                    m_Initialized = false;

        typedef std::vector<amf::AMFBufferPtr>  FrameSlices;
        FrameSlices             m_Slices;

        util::ComponentStats    m_Stats;
        amf_pts                 m_LastStatsTime = 0;

        bool                    m_FirstFrameAfterInit = true;
    };
}