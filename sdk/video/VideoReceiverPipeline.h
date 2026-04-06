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

#include "util/pipeline/AVPipeline.h"
#include "video/MonoscopicVideoInput.h"

namespace ssdk::video
{
    class VideoReceiverPipeline :
        public ssdk::util::AVPipeline,
        protected ssdk::video::MonoscopicVideoInput::Consumer
    {
    public:
        typedef std::shared_ptr<VideoReceiverPipeline>  Ptr;

    protected:
        VideoReceiverPipeline(amf::AMFContext* context);

        AMF_RESULT Init(bool preserveAspectRatio);
    public:
        virtual AMF_RESULT OnInputChanged(const char* codec, ssdk::transport_common::InitID initID, const AMFSize& streamResolution, const AMFRect& viewport, uint32_t bitDepth, bool stereoscopic, bool foveated, amf::AMFBuffer* initBlock);
        virtual AMF_RESULT SubmitInput(amf::AMFBuffer* compressedFrame, ssdk::transport_common::VideoFrame::SubframeType subframeType, bool discontinuity) = 0;

        inline const std::string& GetCodec() const noexcept { amf::AMFLock lock(&m_Guard); return m_Codec; }

        inline bool IsAspectRatioPreserved() const noexcept { amf::AMFLock lock(&m_Guard); return m_PreserveAspectRatio; }
        inline bool IsStereoscopic() const noexcept { amf::AMFLock lock(&m_Guard); return m_Stereoscopic; }
        inline bool IsFoveated() const noexcept { amf::AMFLock lock(&m_Guard); return m_Foveated; }
        
        const AMFSize& GetStreamResolution() const noexcept { amf::AMFLock lock(&m_Guard); return m_StreamResolution; }
        const AMFRect& GetStreamViewport() const noexcept { amf::AMFLock lock(&m_Guard); return m_Viewport; }

        uint32_t GetBitDepth() const noexcept { amf::AMFLock lock(&m_Guard); return m_BitDepth; }

        inline ssdk::transport_common::InitID GetLastInitID() const noexcept { amf::AMFLock lock(&m_Guard); return m_InitID; }
        AMF_RESULT GetLastInitBlock(amf::AMFBuffer** initBlock) const;

    protected:
        mutable amf::AMFCriticalSection     m_Guard;

        std::string         m_Codec;

        bool                m_PreserveAspectRatio = true;
        AMFRect             m_Viewport = {};
        bool                m_Stereoscopic = false;
        bool                m_Foveated = false;
        uint32_t            m_BitDepth = 8;
        AMFSize             m_StreamResolution = {};
        amf::AMFBufferPtr               m_InitBlock;
        ssdk::transport_common::InitID  m_InitID = -1;
    };
}
