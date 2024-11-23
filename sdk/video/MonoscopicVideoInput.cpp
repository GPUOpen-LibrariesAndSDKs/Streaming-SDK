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

#include "MonoscopicVideoInput.h"
#include "decoders/UVDDecoderH264.h"
#include "decoders/UVDDecoderHEVC.h"
#include "decoders/UVDDecoderAV1.h"
#include "amf/public/common/TraceAdapter.h"

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::video::MonoscopicVideoInput";
static constexpr const int32_t MAX_INPUT_QUEUE_DEPTH = 30;

namespace ssdk::video
{
    MonoscopicVideoInput::MonoscopicVideoInput(amf::AMFContext* context, Consumer& sink, size_t hwInstance) :
        m_Context(context),
        m_HWInstance(hwInstance),
        m_Sink(sink),
        m_Pump(*this)
    {
    }

    MonoscopicVideoInput::~MonoscopicVideoInput()
    {
        Terminate();
    }

    AMF_RESULT MonoscopicVideoInput::Init(const char* codecName, const AMFSize& resolution, uint32_t bitDepth, float frameRate, amf::AMFBuffer* initBlock)
    {
        AMF_RESULT result = AMF_OK;
        amf::AMFLock m_Lock(&m_Guard);
        AMF_RETURN_IF_FALSE(m_Decoder == nullptr, AMF_ALREADY_INITIALIZED, L"Init: Already initialized, call Terminate() first to reinitialize");
        if ((result = CreateDecoder(codecName, m_Decoder)) != AMF_OK)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to create the %S video decoder, result=%s", codecName, amf::AMFGetResultText(result));
        }
        else
        {
            m_InputFrameCnt = m_OutputFrameCnt = 0;
            m_BitDepth = bitDepth;
            amf::AMF_SURFACE_FORMAT decoderOutputFormat = amf::AMF_SURFACE_UNKNOWN;
            switch (bitDepth)
            {
            case 8:     //  SDR
                decoderOutputFormat = amf::AMF_SURFACE_NV12;
                break;
            case 10:    //  10-bit HDR
                if (m_Decoder->IsHDRSupported() == true)
                {
                    decoderOutputFormat = amf::AMF_SURFACE_P010;
                }
                else
                {
                    result = AMF_FAIL;
                    AMFTraceError(AMF_FACILITY, L"Bit depth %u is not supported by the video decoder for the %S codec - decoder is not HDR-capable", bitDepth, codecName);
                }
                break;
            default:
                result = AMF_FAIL;
                AMFTraceError(AMF_FACILITY, L"Bit depth %u is not supported by the video decoder for the %S codec", bitDepth, codecName);
                break;
            }
            if (result == AMF_OK)
            {
                result = m_Decoder->Init(decoderOutputFormat, resolution, frameRate, initBlock);
                if (result != AMF_OK)
                {
                    AMFTraceError(AMF_FACILITY, L"Failed to initialize video decoder: error=%s. codec: %S, resolution: %dx%d, frame rate %5.2f, format: %s",
                                  amf::AMFGetResultText(result), codecName, resolution.width, resolution.height, double(frameRate), amf::AMFSurfaceGetFormatName(decoderOutputFormat));
                }
                else
                {
                    m_OutputFormat = decoderOutputFormat;
                    m_Resolution = resolution;
                    m_FrameRate = frameRate;
                    m_Pump.Start();
                }
            }
        }
        return result;
    }

    AMF_RESULT MonoscopicVideoInput::Terminate()
    {
        if (m_Pump.IsRunning() == true)
        {
            m_Pump.RequestStop();
            m_Pump.WaitForStop();
        }
        {
            amf::AMFLock m_Lock(&m_Guard);
            if (m_Decoder != nullptr)
            {
                m_Decoder->Terminate();
                m_Decoder = nullptr;
            }
            m_OutputFormat = amf::AMF_SURFACE_UNKNOWN;
            m_Resolution = {};
            m_FrameRate = 0;
        }
        {
            amf::AMFLock m_LockInput(&m_InputGuard);
            m_InputFrameCnt = 0;
            m_InputQueue.clear();
        }
        {
            amf::AMFLock m_LockOutput(&m_OutputGuard);
            m_OutputFrameCnt = 0;

        }
        return AMF_OK;
    }

    bool MonoscopicVideoInput::IsCodecSupported(const char* codecName) const
    {
        VideoDecodeEngine::Ptr decoder;

        return CreateDecoder(codecName, decoder) == AMF_OK;
    }

    amf::AMF_SURFACE_FORMAT MonoscopicVideoInput::GetOutputFormat() const noexcept
    {
        amf::AMFLock m_Lock(&m_Guard);
        return m_OutputFormat;
    }

    AMFSize MonoscopicVideoInput::GetResolution() const noexcept
    {
        amf::AMFLock m_Lock(&m_Guard);
        return m_Resolution;
    }

    float MonoscopicVideoInput::GetFrameRate() const noexcept
    {
        amf::AMFLock m_Lock(&m_Guard);
        return m_FrameRate;
    }

    uint32_t MonoscopicVideoInput::GetBitDepth() const noexcept
    {
        amf::AMFLock m_Lock(&m_Guard);
        return m_BitDepth;
    }

    size_t MonoscopicVideoInput::GetInputQueueDepth() const noexcept
    {
        amf::AMFLock m_LockInput(&m_InputGuard);
        return m_InputQueue.size();
    }

    uint64_t MonoscopicVideoInput::GetFramesInDecoder() const noexcept
    {
        amf::AMFLock m_LockInput(&m_InputGuard);
        amf::AMFLock m_LockOutput(&m_OutputGuard);
        return m_InputFrameCnt - m_OutputFrameCnt;
    }

    AMF_RESULT MonoscopicVideoInput::CreateDecoder(const char* codecName, VideoDecodeEngine::Ptr& decoder) const
    {
        AMF_RESULT result = AMF_OK;
        std::string codecNameStr(codecName);
        if (codecNameStr == CODEC_H264)
        {
            decoder = VideoDecodeEngine::Ptr(new UVDDecoderH264(m_Context, m_HWInstance));
        }
        else if (codecNameStr == CODEC_HEVC)
        {
            decoder = VideoDecodeEngine::Ptr(new UVDDecoderHEVC(m_Context, m_HWInstance));
        }
        else if (codecNameStr == CODEC_AV1)
        {
            decoder = VideoDecodeEngine::Ptr(new UVDDecoderAV1(m_Context, m_HWInstance));
        }
        else
        {
            result = AMF_NOT_SUPPORTED;
        }
        return result;
    }

    AMF_RESULT MonoscopicVideoInput::SubmitInput(amf::AMFBuffer* input, transport_common::VideoFrame::SubframeType frameType, bool discontinuity)
    {
        AMF_RESULT result = AMF_OK;

        amf::AMFLock m_Lock(&m_Guard);
        AMF_RETURN_IF_FALSE(m_Decoder != nullptr, AMF_NOT_INITIALIZED, L"Video decoder has not be instantiated");
        AMF_RETURN_IF_FALSE(input != nullptr, AMF_INVALID_ARG, L"MonoscopicVideoInput::SubmitInput(): Video input cannot be NULL");

        amf::AMFLock m_LockInput(&m_InputGuard);
        if (m_InputQueue.size() < MAX_INPUT_QUEUE_DEPTH)
        {
            if (discontinuity == true)
            {
                input->SetProperty(ssdk::video::VIDEO_DISCONTINUITY, discontinuity);
            }
            m_InputQueue.push_back({ input, frameType });
            int retryCnt = 0;
            constexpr const int MAX_RETRIES = 10;
            do
            {
                result = m_Decoder->SubmitInput(m_InputQueue.front().m_Buffer, m_InputQueue.front().m_FrameType);
                if (result == AMF_OK || result == AMF_NEED_MORE_INPUT)
                {   //  Successfully submitted a frame to the decoder
                    m_InputQueue.pop_front();
                    ++m_InputFrameCnt;
                    result = AMF_OK;
                }
                else if (result == AMF_INPUT_FULL)
                {
                    ++retryCnt;
                    amf_sleep(1);
                    AMFTraceDebug(AMF_FACILITY, L"Video decoder input is full, queue depth %lu, retries count %d", m_InputQueue.size(), retryCnt);
                }
            } while (result == AMF_INPUT_FULL && retryCnt < MAX_RETRIES);
            if (result != AMF_OK)
            {
                AMFTraceError(AMF_FACILITY, L"Failed to submit input to video decoder, queue depth %lu, retries count %d, result=%s", m_InputQueue.size(), retryCnt, amf::AMFGetResultText(result));
            }
        }
        else
        {
            AMFTraceWarning(AMF_FACILITY, L"MonoscopicVideoInput::SubmitInput() - input full, queue depth is %lu", m_InputQueue.size());
            result = AMF_INPUT_FULL;
        }
        return result;
    }

    void MonoscopicVideoInput::PumpFrames()
    {
        amf::AMFSurfacePtr outputFrame;
        VideoDecodeEngine::Ptr decoder;
        {
            amf::AMFLock m_Lock(&m_Guard);
            decoder = m_Decoder;
        }
        if (decoder != nullptr)
        {
            if (decoder->QueryOutput(&outputFrame) == AMF_OK && outputFrame != nullptr)
            {
                amf::AMFLock m_LockOutput(&m_OutputGuard);
                ++m_OutputFrameCnt;
            }
            else
            {
                size_t queueDepth;
                {
                    amf::AMFLock m_LockInput(&m_InputGuard);
                    queueDepth = m_InputQueue.size();
                }
                if (queueDepth == 0)
                {   //  No output has been produced yet and the input queue is empty - sleep for 1ms not to burn CPU cycles
                    amf_sleep(1);
                }
            }
        }
        if (outputFrame != nullptr)
        {
            m_Sink.OnVideoFrame(outputFrame);
        }
    }

    void MonoscopicVideoInput::Pump::Run()
    {
        while (StopRequested() == false)
        {
            m_VideoInput.PumpFrames();
        }
    }
}
