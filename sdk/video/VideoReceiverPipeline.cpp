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

#include "VideoReceiverPipeline.h"

#include "util/pipeline/SynchronousSlot.h"
#include "util/pipeline/AsynchronousSlot.h"

#include "amf/public/include/components/HQScaler.h"
#include "amf/public/include/components/VQEnhancer.h"
#include "amf/public/common/TraceAdapter.h"

static constexpr const wchar_t* const AMF_FACILITY = L"VideoPipeline";

namespace ssdk::video
{
    VideoReceiverPipeline::VideoReceiverPipeline(amf::AMFContext* context, VideoPresenterPtr presenter, ssdk::util::AVSynchronizer::Ptr avSynchronizer) :
        ssdk::util::AVPipeline(context),
        m_Presenter(presenter),
        m_AVSynchronizer(avSynchronizer)
    {
    }

    VideoReceiverPipeline::~VideoReceiverPipeline()
    {
        Terminate();
        AMFTraceDebug(AMF_FACILITY, L"VideoPipeline destroyed");
    }

    AMF_RESULT VideoReceiverPipeline::OnInputChanged(const char* codec, ssdk::transport_common::InitID initID, const AMFSize& streamResolution, const AMFRect& viewport, uint32_t bitDepth, bool /*stereoscopic*/, bool /*foveated*/, amf::AMFBuffer* initBlock)
    {
        AMF_RESULT result = AMF_OK;
        amf::AMFLock    lock(&m_Guard);

        if (initID != m_InitID)
        {
            m_Input->Terminate();
            if ((result = m_Input->Init(codec, streamResolution, bitDepth, 0.0, initBlock)) != AMF_OK)
            {
                AMFTraceError(AMF_FACILITY, L"Failed to initialize video input (decoder), result=%s", amf::AMFGetResultText(result));
            }
            else
            {
                AMFTraceInfo(AMF_FACILITY, L"Video input (decoder) reinitialized successfully for codec: %S, init ID: %lld, resolution: %dx%d, viewport: (%d,%d,%d,%d), bit depth: %u",
                    codec, initID, streamResolution.width, streamResolution.height, viewport.left, viewport.top, viewport.right, viewport.bottom, bitDepth);

                m_BitDepth = m_Input->GetBitDepth();
                m_InitID = initID;
                m_Viewport = viewport;
#ifdef _WIN32   //  Linux doesn't support HDR properly yet
                amf::AMF_SURFACE_FORMAT prevPresenterInputFormat = m_Presenter->GetInputFormat();
                int32_t presenterWidth = m_Presenter->GetFrameWidth();
                int32_t presenterHeight = m_Presenter->GetFrameHeight();
                amf::AMF_SURFACE_FORMAT presenterInputFormat = (m_BitDepth == 10) ? amf::AMF_SURFACE_RGBA_F16 : amf::AMF_SURFACE_RGBA;
                if ((result = m_Presenter->SetInputFormat(presenterInputFormat)) != AMF_OK)
                {
                    AMFTraceError(AMF_FACILITY, L"Failed to configure video presenter input format %s, result=%s", amf::AMFSurfaceGetFormatName(presenterInputFormat), amf::AMFGetResultText(result));
                }
                else
                {
                    if (presenterInputFormat != prevPresenterInputFormat)
                    {
                        m_Presenter->Terminate();
                        m_Presenter->Init(presenterWidth, presenterHeight);
                    }
#else
                {
#endif
                    if (m_PipelineHead != nullptr)
                    {
                        m_PipelineHead->Stop();
                        m_PipelineHead = nullptr;
                    }
                    if (m_VideoConverter != nullptr)
                    {
                        m_VideoConverter->Terminate();
                    }
                    if (m_Denoiser != nullptr)
                    {
                        m_Denoiser->Terminate();
                    }
                    if (m_Scaler != nullptr)
                    {
                        m_Scaler->Terminate();
                    }
                    if (InitDenoiser() != AMF_OK)
                    {
                        AMFTraceWarning(AMF_FACILITY, L"Video denoiser is not supported on this platform");
                    }
                    if (InitScaler() != AMF_OK)
                    {
                        AMFTraceWarning(AMF_FACILITY, L"High-quality video upscaler is not supported on this platform");
                    }
                    if (InitConverter() == AMF_OK)
                    {
                        ssdk::util::AVSynchronizer::VideoInput::Ptr videoSink;
                        m_AVSynchronizer->GetVideoInput(videoSink); //  AV Syncronizer's video input terminates the pipeline and passes the frame to the presenter, for video it's just a passthrough
                        ssdk::util::PipelineSlot::Ptr converterSlot = ssdk::util::PipelineSlot::Ptr(new ssdk::util::SynchronousSlot("Video Converter", m_VideoConverter, videoSink));
                        ssdk::util::PipelineSlot::Ptr scalerSlot;
                        ssdk::util::PipelineSlot::Ptr denoiserSlot;

                        m_PipelineHead = converterSlot; //  We always have at least a VideoConverter component, continue to
                        if (m_Scaler != nullptr)
                        {
                            scalerSlot = ssdk::util::PipelineSlot::Ptr(new ssdk::util::SynchronousSlot("Video Upscaler", m_Scaler, converterSlot));
                            m_PipelineHead = scalerSlot;
                        }
                        if (m_Denoiser != nullptr)
                        {
                            denoiserSlot = ssdk::util::PipelineSlot::Ptr(new ssdk::util::SynchronousSlot("Video Denoiser", m_Denoiser, scalerSlot != nullptr ? scalerSlot : converterSlot));
                            m_PipelineHead = denoiserSlot;
                        }
                        m_PipelineHead->Start();
                    }
                    else
                    {
                        m_PipelineHead = nullptr;
                    }
                }
            }
        }

        return result;
    }

    AMF_RESULT VideoReceiverPipeline::SubmitInput(amf::AMFBuffer* compressedFrame, ssdk::transport_common::VideoFrame::SubframeType subframeType, bool discontinuity)
    {
        ssdk::video::MonoscopicVideoInput::Ptr  input;
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

    AMF_RESULT VideoReceiverPipeline::Init(size_t uvdInstance, bool preserveAspectRatio, bool hqUpscale, bool denoise)
    {
        AMF_RESULT result = AMF_OK;
        ssdk::util::ClientStatsManager::Ptr statsManager;
        {
            amf::AMFLock    lock(&m_Guard);

            m_PreserveAspectRatio = preserveAspectRatio;
            m_PipelineHead = nullptr;
            m_EnableHQUpscale = hqUpscale;
            m_EnableDenoiser = denoise;
            m_Input = ssdk::video::MonoscopicVideoInput::Ptr(new ssdk::video::MonoscopicVideoInput(m_Context, *this, size_t(uvdInstance)));
            statsManager = m_StatsManager;
        }

        if (statsManager != nullptr)
        {
            statsManager->Init();
        }

        return result;
    }

    AMF_RESULT VideoReceiverPipeline::InitConverter()
    {
        AMF_RESULT result = AMF_OK;
        if (m_VideoConverter == nullptr && (result = g_AMFFactory.GetFactory()->CreateComponent(m_Context, AMFVideoConverter, &m_VideoConverter)) != AMF_OK)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to create the Video Converter component, error= %s", amf::AMFGetResultText(result));
        }
        else
        {
            amf::AMF_MEMORY_TYPE memType = m_Presenter->GetMemoryType();
            result = m_VideoConverter->SetProperty(AMF_VIDEO_CONVERTER_MEMORY_TYPE, memType);
            AMF_RETURN_IF_FAILED(result, L"Failed to set memory type to %s on video converter, result = %s", amf::AMFGetMemoryTypeName(memType), amf::AMFGetResultText(result));
            result = m_VideoConverter->SetProperty(AMF_VIDEO_CONVERTER_COMPUTE_DEVICE, memType);
            AMF_RETURN_IF_FAILED(result, L"Failed to set compute type to %s on video converter, result = %s", amf::AMFGetMemoryTypeName(memType), amf::AMFGetResultText(result));
            amf::AMF_SURFACE_FORMAT format = m_Presenter->GetInputFormat();
            result = m_VideoConverter->SetProperty(AMF_VIDEO_CONVERTER_OUTPUT_FORMAT, format);
            AMF_RETURN_IF_FAILED(result, L"Failed to set video converter output format to %s on video converter, result = %s", amf::AMFSurfaceGetFormatName(format), amf::AMFGetResultText(result));

            m_VideoConverter->SetProperty(AMF_VIDEO_CONVERTER_SCALE, AMF_VIDEO_CONVERTER_SCALE_BICUBIC);
            m_VideoConverter->SetProperty(AMF_VIDEO_CONVERTER_KEEP_ASPECT_RATIO, m_PreserveAspectRatio);
            m_VideoConverter->SetProperty(AMF_VIDEO_CONVERTER_FILL, true);

            m_VideoConverter->SetProperty(AMF_VIDEO_CONVERTER_COLOR_PROFILE, m_ColorRange == AMF_COLOR_RANGE_ENUM::AMF_COLOR_RANGE_FULL ? AMF_VIDEO_CONVERTER_COLOR_PROFILE_FULL_709 : AMF_VIDEO_CONVERTER_COLOR_PROFILE_709);
            m_VideoConverter->SetProperty(AMF_VIDEO_CONVERTER_INPUT_COLOR_RANGE, m_ColorRange);

            if (m_BitDepth == 10)
            {
                m_VideoConverter->SetProperty(AMF_VIDEO_CONVERTER_INPUT_TONEMAPPING, AMF_VIDEO_CONVERTER_TONEMAPPING_LINEAR);
                m_VideoConverter->SetProperty(AMF_VIDEO_CONVERTER_OUTPUT_TONEMAPPING, AMF_VIDEO_CONVERTER_TONEMAPPING_LINEAR);
            }
            float displayAspectRatio = float(m_Presenter->GetFrameWidth()) / float(m_Presenter->GetFrameHeight());
            float streamAspectRatio = float(m_Input->GetResolution().width) / float(m_Input->GetResolution().height);
            int32_t inputWidth, inputHeight;
            int32_t outputWidth, outputHeight;

            if (m_Scaler != nullptr)
            {   //  When we have the HQ scaler active, it's output is already scaled to the display resolution, we're only doing CSC
                inputWidth = m_Presenter->GetFrameWidth();
                inputHeight = m_Presenter->GetFrameHeight();
                outputWidth = m_Presenter->GetFrameWidth();
                outputHeight = m_Presenter->GetFrameHeight();
            }
            else if (m_PreserveAspectRatio == true && displayAspectRatio != streamAspectRatio)
            {
                inputWidth = m_Input->GetResolution().width;
                inputHeight = m_Input->GetResolution().height;
                outputWidth = m_Presenter->GetFrameWidth();
                outputHeight = m_Presenter->GetFrameHeight();
            }
            else
            {   //  Otherwise the presenter can do basic bilinear scaling more efficiently
                outputWidth = inputWidth = m_Input->GetResolution().width;
                outputHeight = inputHeight = m_Input->GetResolution().height;
            }
#ifdef _WIN32
            if (m_Presenter != nullptr)
            {
                //  When stream's and display's aspect ratios don't match, we have to turn presenter's exlusive full screen off, otherwise scaling works incorrectly on Windows -
                //  the window still occupies the whole display, stretching the correctly scaled image to fit the entire screen.
                if (m_PreserveAspectRatio == true && displayAspectRatio != streamAspectRatio)
                {
                    m_Presenter->Terminate();
                    m_Presenter->SetExclusiveFullscreen(false);
                    m_Presenter->Init(outputWidth, outputHeight);
                    m_ExclusiveFullScreen = false;
                    AMFTraceInfo(AMF_FACILITY, L"Switched video presenter to non-exclusive mode, resolution: %dx%d", outputWidth, outputHeight);
                }
                else if (m_ExclusiveFullScreen == false)
                {
                    m_Presenter->Terminate();
                    m_Presenter->SetExclusiveFullscreen(true);
                    m_Presenter->Init(outputWidth, outputHeight);
                    m_ExclusiveFullScreen = true;
                    AMFTraceInfo(AMF_FACILITY, L"Switched video presenter to exclusive mode, resolution: %dx%d", outputWidth, outputHeight);
                }
            }
#endif
            result = m_VideoConverter->SetProperty(AMF_VIDEO_CONVERTER_OUTPUT_SIZE, AMFConstructSize(outputWidth, outputHeight));
            AMF_RETURN_IF_FAILED(result, L"Failed to video converter output size to %dx%d, result = %s", outputWidth, outputHeight, amf::AMFGetResultText(result));

            if ((result = m_VideoConverter->Init(m_Input->GetOutputFormat(), inputWidth, inputHeight)) != AMF_OK)
            {
                AMFTraceError(AMF_FACILITY, L"Failed to initialize the Video Converter component, format: %s=>%s, size: %dx%d=>%dx%d, error= %s",
                    amf::AMFSurfaceGetFormatName(m_Input->GetOutputFormat()), amf::AMFSurfaceGetFormatName(m_Presenter->GetInputFormat()),
                    inputWidth, inputHeight, outputWidth, outputHeight, amf::AMFGetResultText(result));
                m_VideoConverter = nullptr;
            }
            else
            {
                AMFTraceInfo(AMF_FACILITY, L"Successfully initialized the Video Converter component, format: %s=>%s, size: %dx%d=>%dx%d",
                    amf::AMFSurfaceGetFormatName(m_Input->GetOutputFormat()), amf::AMFSurfaceGetFormatName(m_Presenter->GetInputFormat()),
                    inputWidth, inputHeight, outputWidth, outputHeight);
            }
        }
        return result;
    }

    AMF_RESULT VideoReceiverPipeline::InitScaler()
    {
        AMF_RESULT result = AMF_OK;
#ifdef _WIN32
        if (m_EnableHQUpscale == true)
        {
            if (m_Input->GetResolution().width < m_Presenter->GetFrameWidth() && m_Input->GetResolution().height < m_Presenter->GetFrameHeight())
            {   //  Use HQScaler only when input resolution is less that output resolution
                if (m_Scaler == nullptr && (result = g_AMFFactory.GetFactory()->CreateComponent(m_Context, AMFHQScaler, &m_Scaler)) != AMF_OK)
                {
                    AMFTraceError(AMF_FACILITY, L"Failed to create the HQScaler component, error= %s", amf::AMFGetResultText(result));
                }
                else
                {
                    m_Scaler->SetProperty(AMF_HQ_SCALER_ENGINE_TYPE, m_Presenter->GetMemoryType());
                    m_Scaler->SetProperty(AMF_HQ_SCALER_ALGORITHM, AMF_HQ_SCALER_ALGORITHM_FSR);
                    m_Scaler->SetProperty(AMF_HQ_SCALER_KEEP_ASPECT_RATIO, m_PreserveAspectRatio);
                    m_Scaler->SetProperty(AMF_HQ_SCALER_FILL, true);
                    m_Scaler->SetProperty(AMF_HQ_SCALER_FILL_COLOR, AMFColor({ 0, 0, 0, 0 }));
                    m_Scaler->SetProperty(AMF_HQ_SCALER_SHARPNESS, 0.5);    // Float in the range of [0.0, 2.0] - default = 0.5
                    m_Scaler->SetProperty(AMF_HQ_SCALER_OUTPUT_SIZE, ::AMFConstructSize(m_Presenter->GetFrameWidth(), m_Presenter->GetFrameHeight()));
                    if ((result = m_Scaler->Init(m_Input->GetOutputFormat(), m_Input->GetResolution().width, m_Input->GetResolution().height)) != AMF_OK)
                    {
                        AMFTraceError(AMF_FACILITY, L"Failed to initialize the HQScaler component, memory type: %s, format: %s, resolution: %dx%d=>%dx%d, error= %s",
                            amf::AMFGetMemoryTypeName(m_Presenter->GetMemoryType()), amf::AMFSurfaceGetFormatName(m_Input->GetOutputFormat()),
                            m_Input->GetResolution().width, m_Input->GetResolution().height, m_Presenter->GetFrameWidth(), m_Presenter->GetFrameHeight(), amf::AMFGetResultText(result));
                        m_Scaler = nullptr;   //  We have failed to initialize HQScaler, so will try to run without it, destroying the component
                    }
                    else
                    {
                        AMFTraceInfo(AMF_FACILITY, L"HQScaler initialized successfully: memory type: %s, format: %s, resolution: %dx%d=>%dx%d",
                            amf::AMFGetMemoryTypeName(m_Presenter->GetMemoryType()), amf::AMFSurfaceGetFormatName(m_Input->GetOutputFormat()),
                            m_Input->GetResolution().width, m_Input->GetResolution().height, m_Presenter->GetFrameWidth(), m_Presenter->GetFrameHeight());
                    }
                }
            }
            else if (m_Scaler != nullptr)   //  Terminate and destroy the scaler if we're not upscaling
            {
                m_Scaler->Terminate();
                m_Scaler = nullptr;
            }
        }
        else
        {
            m_Scaler = nullptr;
            AMFTraceInfo(AMF_FACILITY, L"High quality upscaler disabled");
        }
#else
        m_Scaler = nullptr;
        AMFTraceInfo(AMF_FACILITY, L"High quality upscaler not supported");
#endif
        return result;
    }

    AMF_RESULT VideoReceiverPipeline::InitDenoiser()
    {
        AMF_RESULT result = AMF_OK;
#ifdef _WIN32
        if (m_EnableDenoiser == true)
        {
            if (m_Denoiser == nullptr && (result = g_AMFFactory.GetFactory()->CreateComponent(m_Context, AMFVQEnhancer, &m_Denoiser)) != AMF_OK)
            {
                AMFTraceError(AMF_FACILITY, L"Failed to create the Video denoiser component, error= %s", amf::AMFGetResultText(result));
            }
            else
            {
                //  Feel free to experiment with the values below:
                m_Denoiser->SetProperty(AMF_VIDEO_ENHANCER_ENGINE_TYPE, m_Presenter->GetMemoryType());
                m_Denoiser->SetProperty(AMF_VE_FCR_ATTENUATION, 0.1);
                m_Denoiser->SetProperty(AMF_VE_FCR_RADIUS, 2);
                m_Denoiser->SetProperty(AMF_VE_FCR_SPLIT_VIEW, false);
                m_Denoiser->SetProperty(AMF_VIDEO_ENHANCER_OUTPUT_SIZE, ::AMFConstructSize(m_Input->GetResolution().width, m_Input->GetResolution().height));
                if ((result = m_Denoiser->Init(m_Input->GetOutputFormat(), m_Input->GetResolution().width, m_Input->GetResolution().height)) != AMF_OK)
                {
                    AMFTraceError(AMF_FACILITY, L"Failed to initialize the Video denoiser (VQEnhancer) component, memory type: %s, format: %s, resolution: %dx%d, error= %s",
                        amf::AMFGetMemoryTypeName(m_Presenter->GetMemoryType()), amf::AMFSurfaceGetFormatName(m_Input->GetOutputFormat()),
                        m_Input->GetResolution().width, m_Input->GetResolution().height, amf::AMFGetResultText(result));
                    m_Denoiser = nullptr;   //  We have failed to initialize VQEnahancer, so will try to run without it, destroying the component
                }
                else
                {
                    AMFTraceInfo(AMF_FACILITY, L"Video denoiser initialized successfully: memory type: %s, format: %s, resolution: %dx%d",
                        amf::AMFGetMemoryTypeName(m_Presenter->GetMemoryType()), amf::AMFSurfaceGetFormatName(m_Input->GetOutputFormat()),
                        m_Input->GetResolution().width, m_Input->GetResolution().height);
                }
            }
        }
        else
        {
            m_Denoiser = nullptr;
            AMFTraceInfo(AMF_FACILITY, L"Video denoiser disabled");
        }
#else
        m_Denoiser = nullptr;
        AMFTraceInfo(AMF_FACILITY, L"Video denoiser not supported");
#endif
        return result;
    }

    void VideoReceiverPipeline::Terminate()
    {
        ssdk::video::MonoscopicVideoInput::Ptr  input;
        amf::AMFComponentPtr                    videoConverter;
        amf::AMFComponentPtr                    denoiser;
        amf::AMFComponentPtr                    scaler;
        ssdk::util::PipelineSlot::Ptr           pipelineHead;

        {   //  We don't want to call terminate under a lock to avoid a deadlock, but we want to null all of the class members
            amf::AMFLock    lock(&m_Guard);
            pipelineHead = m_PipelineHead;
            m_PipelineHead = nullptr;

            videoConverter = m_VideoConverter;
            m_VideoConverter = nullptr;

            denoiser = m_Denoiser;
            m_Denoiser = nullptr;

            scaler = m_Scaler;
            m_Scaler = nullptr;

            input = m_Input;
            m_Input = nullptr;
        }

        if (input != nullptr)
        {
            input->Terminate();
        }
        if (pipelineHead != nullptr)
        {
            pipelineHead->Stop();
        }
        if (videoConverter != nullptr)
        {
            videoConverter->Terminate();;
        }
        if (denoiser != nullptr)
        {
            denoiser->Terminate();
        }
        if (scaler != nullptr)
        {
            scaler->Terminate();
        }
        AMFTraceDebug(AMF_FACILITY, L"VideoPipeline::Terminate() completed");
    }

    void VideoReceiverPipeline::SetStatsManager(ssdk::util::ClientStatsManager::Ptr statsManager)
    {
        amf::AMFLock    lock(&m_Guard);
        m_StatsManager = statsManager;
    }

    /// OnVideoFrame() is a callback which is called when the decoder produces a frame. It then submits to the post-processing pipeline
    void VideoReceiverPipeline::OnVideoFrame(amf::AMFSurface* frame)
    {
        ssdk::util::PipelineSlot::Ptr pipelineHead;
        amf::AMFComponentPtr                    videoConverter;
        ssdk::util::ClientStatsManager::Ptr statsManager;
        {
            amf::AMFLock    lock(&m_Guard);
            pipelineHead = m_PipelineHead;
            statsManager = m_StatsManager;
            videoConverter = m_VideoConverter;
        }

        if (statsManager != nullptr)
        {
            statsManager->DecrementDecoderQueueDepth(frame->GetPts());
        }

        if (pipelineHead != nullptr)
        {
            bool discontinuity = false;
            frame->GetProperty(ssdk::video::VIDEO_DISCONTINUITY, &discontinuity);
            if (discontinuity == true)
            {
                pipelineHead->Flush();
            }
            int64_t colorRangeOfFrame = AMF_COLOR_RANGE_ENUM::AMF_COLOR_RANGE_FULL;
            frame->GetProperty(AMF_VIDEO_COLOR_RANGE, reinterpret_cast<int64_t*>(&colorRangeOfFrame));
            if (colorRangeOfFrame != m_ColorRange && videoConverter != nullptr)
            {
                AMFTraceInfo(AMF_FACILITY, L"Color range set on frame is different: %d->%d", m_ColorRange, colorRangeOfFrame);
                {
                    amf::AMFLock    lock(&m_Guard);
                    m_ColorRange = static_cast<AMF_COLOR_RANGE_ENUM>(colorRangeOfFrame);
                }
                videoConverter->Terminate();    //  Reinitialize video converter
                InitConverter();
            }

            AMF_RESULT result = pipelineHead->SubmitInput(frame);
            if (result != AMF_OK)
            {
                AMFTraceError(AMF_FACILITY, L"OnVideoFrame(): failed to submit a video frame to the pipeline, result=%s", amf::AMFGetResultText(result));
            }
        }
        else
        {
            AMFTraceError(AMF_FACILITY, L"OnVideoFrame(): failed to submit a video frame to the pipeline - pipeline is not initialized");
        }
    }
}
