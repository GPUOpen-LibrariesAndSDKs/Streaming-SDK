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

#include "video/android/MonoscopicVideoInputAndroid.h"
#include "video/VideoReceiverPipeline.h"
#include "util/pipeline/AVSynchronizer.h"
#include "util/jni/ANativeWindowHolder.h"
#include "util/stats/ClientStatsManager.h"

#include "amf/public/common/Thread.h"

#include <memory>

namespace ssdk::video
{
    class VideoReceiver :
        public ssdk::video::VideoReceiverPipeline
    {
    public:
        typedef std::shared_ptr<VideoReceiver> Ptr;

    public:
        explicit VideoReceiver(amf::AMFContext *context,
                               ssdk::util::AVSynchronizer::Ptr avSynchronizer);

        virtual ~VideoReceiver();

        AMF_RESULT Init(ssdk::util::ANativeWindowHolder&& pDecodeNativeWindow, const AMFSize& displayResolution, const AMFRect& displayViewport, float_t refreshRate);

        void Terminate();

        void SetStatsManager(ssdk::util::ClientStatsManager::Ptr statsManager);

        virtual AMF_RESULT OnInputChanged(const char* codec, ssdk::transport_common::InitID initID,
                                          const AMFSize& streamResolution, const AMFRect& viewport,
                                          uint32_t bitDepth, bool stereoscopic, bool foveated, amf::AMFBuffer* initBlock) override;

        virtual AMF_RESULT SubmitInput(amf::AMFBuffer *compressedFrame,
                                       ssdk::transport_common::VideoFrame::SubframeType subframeType,
                                       bool discontinuity) override;

        inline const AMFRect& GetDisplayViewport() const noexcept { amf::AMFLock lock(&m_Guard); return m_DisplayViewport; }
        inline const AMFSize& GetDisplayResolution() const noexcept { amf::AMFLock lock(&m_Guard); return m_DisplayResolution; }

    protected:
        virtual void OnVideoFrame(amf::AMFSurface *frame) override; //  This is a callback to video input (decoder), which is called when a decoder produces a frame and it needs to be submitted to the rest of the pipeline

    private:
        mutable amf::AMFCriticalSection m_Guard;

        ssdk::video::MonoscopicVideoInputAndroid::Ptr m_Input;
        ssdk::util::AVSynchronizer::Ptr m_AVSynchronizer;
        ssdk::util::ClientStatsManager::Ptr m_StatsManager;

        AMFSize     m_DisplayResolution = {};
        AMFRect     m_DisplayViewport = {};
        int         m_Framerate = 0;
        std::string m_DeviceStoragePath;
    };
}
