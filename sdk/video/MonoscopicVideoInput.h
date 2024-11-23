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


#include "decoders/VideoDecodeEngine.h"
#include "amf/public/include/core/Context.h"
#include "amf/public/common/Thread.h"

#include <deque>
#include <string>
#include <memory>

namespace ssdk::video
{
    class MonoscopicVideoInput
    {
    public:
        typedef std::shared_ptr<MonoscopicVideoInput>   Ptr;

        class Consumer
        {
        public:
            virtual void OnVideoFrame(amf::AMFSurface* frame) = 0;
        };

    private:
        MonoscopicVideoInput(const MonoscopicVideoInput&) = delete;
        MonoscopicVideoInput& operator=(const MonoscopicVideoInput&) = delete;

    public:
        MonoscopicVideoInput(amf::AMFContext* context, Consumer& sink, size_t hwInstance = 0);
        ~MonoscopicVideoInput();

        AMF_RESULT Init(const char* codecName, const AMFSize& resolution, uint32_t bitDepth, float frameRate, amf::AMFBuffer* initBlock);
        AMF_RESULT Terminate();

        bool IsCodecSupported(const char* codecName) const;

        amf::AMF_SURFACE_FORMAT GetOutputFormat() const noexcept;
        AMFSize GetResolution() const noexcept;
        float GetFrameRate() const noexcept;
        uint32_t GetBitDepth() const noexcept;

        size_t GetInputQueueDepth() const noexcept;
        uint64_t GetFramesInDecoder() const noexcept;

        AMF_RESULT SubmitInput(amf::AMFBuffer* input, transport_common::VideoFrame::SubframeType frameType, bool discontinuity);

    private:
        AMF_RESULT CreateDecoder(const char* codecName, VideoDecodeEngine::Ptr& decoder) const;
        void PumpFrames();

    private:
        struct FrameDescriptor
        {
            typedef std::deque<FrameDescriptor> Queue;

            amf::AMFBufferPtr  m_Buffer;
            transport_common::VideoFrame::SubframeType m_FrameType;
        };

        class Pump : public amf::AMFThread
        {
        public:
            Pump(MonoscopicVideoInput& videoInput) : m_VideoInput(videoInput) {}

            virtual void Run() override;

        private:
            MonoscopicVideoInput&       m_VideoInput;
        };

    private:
        mutable amf::AMFCriticalSection m_Guard;
        amf::AMFContextPtr              m_Context;
        Consumer&                       m_Sink;
        VideoDecodeEngine::Ptr          m_Decoder;
        size_t                          m_HWInstance = 0;
        amf::AMF_SURFACE_FORMAT         m_OutputFormat = amf::AMF_SURFACE_UNKNOWN;
        AMFSize                         m_Resolution = {};
        float                           m_FrameRate = 0;
        uint32_t                        m_BitDepth = 0;

        mutable amf::AMFCriticalSection m_InputGuard;
        FrameDescriptor::Queue          m_InputQueue;
        uint64_t                        m_InputFrameCnt = 0;

        mutable amf::AMFCriticalSection m_OutputGuard;
        uint64_t                        m_OutputFrameCnt = 0;
        Pump                            m_Pump;

        /* IMPORTANT NOTE: The order of nested locks for the above critical sections should always be as follows:
            amf::AMFLock lock(m_Guard);
            amf::AMFLock lockInput(m_InputGuard);
            amf::AMFLock lockOutput(m_OutputGuard);

            Violating this order will result in deadlocks!!!
        */
    };
}
