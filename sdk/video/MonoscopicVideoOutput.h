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


#include "encoders/VideoEncodeEngine.h"
#include "VideoTransmitterAdapter.h"

#include "amf/public/include/components/Component.h"
#include "amf/public/common/Thread.h"

#include <memory>
#include <set>

namespace ssdk::video
{
    class MonoscopicVideoOutput
    {
    public:
        typedef std::shared_ptr<MonoscopicVideoOutput>    Ptr;

    public:
        MonoscopicVideoOutput(TransmitterAdapter& transportAdapter, VideoEncodeEngine::Ptr encoder, amf::AMFContext* context, amf::AMF_MEMORY_TYPE memoryType);
        ~MonoscopicVideoOutput();

        AMF_RESULT Init(amf::AMF_SURFACE_FORMAT format, const AMFSize& inputResolution, const AMFSize& streamResolution,
                        int64_t bitrate, float frameRate, bool hdr, bool preserveAspectRatio = true, int64_t intraRefreshPeriod = 0);

        AMF_RESULT Terminate();

        AMF_RESULT SubmitInput(amf::AMFSurface* input, amf_pts originPts, amf_pts videoInPts);

        void ForceKeyFrame();

        AMFSize GetEncodedResolution() const noexcept;
        AMF_RESULT SetEncodedResolution(const AMFSize& resolution);

        int64_t GetBitrate() const noexcept;
        AMF_RESULT SetBitrate(int64_t bitrate);

        float GetFramerate() const noexcept;
        AMF_RESULT SetFramerate(float framerate);

    protected:
        AMF_RESULT InitializeEncoder(const AMFSize& streamResolution, int64_t bitrate, float frameRate, const ColorParameters& colorParams, int64_t intraRefreshPeriod);
        AMF_RESULT InitializeConverter(amf::AMF_SURFACE_FORMAT format, const AMFSize& inputResolution, const AMFSize& streamResolution, const ColorParameters& encoderInputColorParams);

        void DetermineEncoderInputColorParams(amf::AMFSurface* input, bool needsConverter, bool hdr, ColorParameters& inputColorParams);

        bool UpdateColorParametersIfChanged(amf::AMFSurface* input);
        bool UpdateInputResolutionIfChanged(amf::AMFSurface* input);
        bool UpdateInputFormatIfChanged(amf::AMFSurface* input);

        AMFSize GetSurfaceResolution(amf::AMFSurface* input) const;

        void PollForEncoderOutput();

        void SaveExtraData(amf::AMFBuffer* buffer);
        bool GetExtraData(amf::AMFBuffer** buffer, amf_pts& id);

        bool IsKeyFrameRequested() noexcept;

        uint32_t GetColorDepthInBits() const noexcept;
        AMFRect CalculateViewport() const noexcept;

    protected:
        class EncoderPoller : public amf::AMFThread
        {
        public:
            EncoderPoller(MonoscopicVideoOutput* pPipeline) : m_pPipeline(pPipeline) {}

            virtual void Run() override;

        private:
            MonoscopicVideoOutput* m_pPipeline = nullptr;
        };

    protected:
        mutable amf::AMFCriticalSection m_Guard;

        bool                        m_Initialized = false;

        TransmitterAdapter&         m_TransportAdapter;
        amf::AMFContextPtr          m_Context;
        amf::AMFComponentPtr        m_Converter;
        VideoEncodeEngine::Ptr      m_Encoder;
        EncoderPoller               m_EncoderPoller;

        amf::AMF_MEMORY_TYPE        m_MemoryType = amf::AMF_MEMORY_TYPE::AMF_MEMORY_UNKNOWN;

        AMFSize                     m_InputResolution = {};
        amf::AMF_ROTATION_ENUM      m_Orientation = amf::AMF_ROTATION_ENUM::AMF_ROTATION_NONE;

        AMFSize                     m_StreamResolution = {};
        ColorParameters             m_CurrentInputColorParams;

        int64_t                     m_Bitrate = 0;
        float                       m_FrameRate = 0;
        int64_t                     m_IntraRefreshPeriod = 0;

        bool                        m_HDR = false;
        bool                        m_PreserveAspectRatio = true;
        bool                        m_NeedsCSC = false;
        amf::AMFBufferPtr           m_ExtraData;
        transport_common::InitID    m_InitID = 0,
                                    m_LastRetrievedExtraDataID = 0;

        bool                        m_ForceKeyFrame = false;

        int64_t                     m_SequenceNumber = 0;
        int64_t                     m_FramesSubmitted = 0;
    };
}
