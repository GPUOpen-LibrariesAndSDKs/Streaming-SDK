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

#include "VideoReceiverAndroid.h"
#include "amf/public/common/TraceAdapter.h"

static constexpr const wchar_t* const AMF_FACILITY = L"VideoReceiver";

namespace ssdk::video
{

    VideoReceiver::VideoReceiver(amf::AMFContext *context,
                                 ssdk::util::AVSynchronizer::Ptr avSynchronizer) :
            ssdk::video::VideoReceiverPipeline(context),
            m_AVSynchronizer(avSynchronizer)
    {
    }

    VideoReceiver::~VideoReceiver()
    {
        Terminate();
        AMFTraceDebug(AMF_FACILITY, L"VideoReceiver destroyed");
    }

    AMF_RESULT VideoReceiver::Init(ssdk::util::ANativeWindowHolder&& pDecodeNativeWindow, const AMFSize& displayResolution, const AMFRect& displayViewport, float_t refreshRate)
    {
        AMF_RESULT result = AMF_OK;
        if (pDecodeNativeWindow == nullptr)
        {
            AMFTraceError(AMF_FACILITY, L"Decoder target cannot be nullptr");
            result = AMF_INVALID_ARG;
        }
        else
        {
            ssdk::util::ClientStatsManager::Ptr statsManager;
            int32_t decoderTargetWidth = ANativeWindow_getWidth(*pDecodeNativeWindow);
            int32_t decoderTargetHeight = ANativeWindow_getHeight(*pDecodeNativeWindow);
            AMFRect viewport = displayViewport;
            if (viewport.Width() == 0 || viewport.Height() == 0)
            {
                viewport.right = decoderTargetWidth;
                viewport.bottom = decoderTargetHeight;
            }
            bool preserveAspectRatio = displayResolution.width / displayResolution.height == viewport.Width() / viewport.Height();

            amf::AMFLock lock(&m_Guard);
            if ((result = VideoReceiverPipeline::Init(preserveAspectRatio)) != AMF_OK)
            {
                AMFTraceError(AMF_FACILITY,
                              L"Failed to initialize video reciver pipeline (base), result=%s",
                              amf::AMFGetResultText(result));
            }
            else
            {
                m_DisplayViewport = displayViewport;
                m_DisplayResolution = displayResolution;
                m_Framerate = int(refreshRate);
                m_Input = ssdk::video::MonoscopicVideoInputAndroid::Ptr(
                        new ssdk::video::MonoscopicVideoInputAndroid(m_Context, std::move(
                                pDecodeNativeWindow), *this));
                statsManager = m_StatsManager;
            }

            if (result == AMF_OK && statsManager != nullptr)
            {
                statsManager->Init();
            }
        }
        return result;
    }

    void VideoReceiver::Terminate()
    {
        AMFTraceDebug(AMF_FACILITY, L"VideoReceiver::Terminate() started");
        ssdk::video::MonoscopicVideoInput::Ptr input;
        {   //  We don't want to call terminate under a lock to avoid a deadlock, but we want to null all of the class members
            amf::AMFLock lock(&m_Guard);
            input = m_Input;
            m_Input = nullptr;
            m_StatsManager = nullptr;
        }

        if (input != nullptr)
        {
            input->Terminate();
        }

        AMFTraceDebug(AMF_FACILITY, L"VideoReceiver::Terminate() completed");
    }

    AMF_RESULT VideoReceiver::OnInputChanged(const char *codec, ssdk::transport_common::InitID initID,
                                  const AMFSize &streamResolution, const AMFRect &viewport,
                                  uint32_t bitDepth, bool stereoscopic, bool foveated, amf::AMFBuffer *initBlock)
    {
        AMF_RESULT result = AMF_OK;
        amf::AMFLock lock(&m_Guard);

        if (initID != GetInitID())
        {
            if ((result = VideoReceiverPipeline::OnInputChanged(codec, initID, streamResolution, viewport, bitDepth, stereoscopic, foveated, initBlock)) != AMF_OK)
            {
                AMFTraceError(AMF_FACILITY, L"Failed to initialize video reciver pipeline (base), result=%s", amf::AMFGetResultText(result));
            }
            else
            {
                ssdk::util::AVSynchronizer::VideoInput::Ptr videoSink;
                m_AVSynchronizer->GetVideoInput(
                        videoSink); //  AV Syncronizer's video input terminates the pipeline and passes the frame to the presenter, for video it's just a passthrough
                if (videoSink != nullptr)
                {
                    videoSink->Stop();
                }
                else
                {
                    AMFTraceError(AMF_FACILITY, L"AVSynchronizer is not initialized");
                }
                m_Input->Terminate();
                if ((result = m_Input->Init(codec, streamResolution, bitDepth, 0.0, initBlock)) !=
                    AMF_OK)
                {
                    AMFTraceError(AMF_FACILITY,
                                  L"Failed to initialize video input (decoder), result=%s",
                                  amf::AMFGetResultText(result));
                }
                else
                {
                    AMFTraceInfo(AMF_FACILITY,
                                 L"Video input (decoder) reinitialized successfully for codec: %S, init ID: %lld, resolution: %dx%d, viewport: (%d,%d,%d,%d), bit depth: %u",
                                 codec, initID, streamResolution.width, streamResolution.height,
                                 viewport.left, viewport.top, viewport.right, viewport.bottom,
                                 bitDepth);

                    if (videoSink != nullptr)
                    {
                        videoSink->Start();
                    }
                    else
                    {
                        AMFTraceError(AMF_FACILITY, L"AVSynchronizer is not initialized");
                    }
                }
            }
        }

        return result;
    }

    AMF_RESULT VideoReceiver::SubmitInput(amf::AMFBuffer* compressedFrame,
                                          ssdk::transport_common::VideoFrame::SubframeType subframeType,
                                          bool discontinuity)
    {
        ssdk::video::MonoscopicVideoInput::Ptr input;
        ssdk::util::ClientStatsManager::Ptr statsManager;
        {
            amf::AMFLock lock(&m_Guard);
            AMF_RETURN_IF_FALSE(m_Input != nullptr, AMF_NOT_INITIALIZED, L"Video input is NULL");
            input = m_Input;
            statsManager = m_StatsManager;
        }

        if (statsManager != nullptr)
        {
            statsManager->IncrementDecoderQueueDepth(compressedFrame->GetPts());
        }

        return input->SubmitInput(compressedFrame, subframeType, discontinuity);
    }
    void VideoReceiver::SetStatsManager(ssdk::util::ClientStatsManager::Ptr statsManager)
    {
        amf::AMFLock lock(&m_Guard);
        m_StatsManager = statsManager;
    }

/// OnVideoFrame() is a callback which is called when the decoder produces a frame. It then submits to the post-processing pipeline
    void VideoReceiver::OnVideoFrame(amf::AMFSurface* frame)
    {
        ssdk::util::ClientStatsManager::Ptr statsManager;
        {
            amf::AMFLock    lock(&m_Guard);
            statsManager = m_StatsManager;
        }
        if (statsManager != nullptr)
        {
            amf_pts pts = frame->GetPts();
            statsManager->DecrementDecoderQueueDepth(pts);
        }

        ssdk::util::AVSynchronizer::VideoInput::Ptr videoSink;
        m_AVSynchronizer->GetVideoInput(videoSink); //  AV Syncronizer's video input terminates the pipeline and passes the frame to the presenter, for video it's just a passthrough
        if (videoSink != nullptr)
        {
            videoSink->SubmitInput(frame);
        }
        else
        {
            AMFTraceError(AMF_FACILITY, L"AVSynchronizer is not initialized");
        }
    }
}