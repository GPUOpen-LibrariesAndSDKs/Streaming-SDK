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

#include "VideoDecodeEngine.h"
#include "amf/public/common/TraceAdapter.h"

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::video::VideoDecodeEngine";

namespace ssdk::video
{
    VideoDecodeEngine::VideoDecodeEngine(amf::AMFContext* context) :
        m_Context(context)
    {
    }

    VideoDecodeEngine::~VideoDecodeEngine()
    {
        m_Decoder = nullptr;
        m_Context = nullptr;
    }

    AMF_RESULT VideoDecodeEngine::Init(amf::AMF_SURFACE_FORMAT format, const AMFSize& resolution, float frameRate, amf::AMFBuffer* /*initBlock*/)
    {
        amf::AMFLock lock(&m_Guard);
        AMF_RETURN_IF_FALSE(m_Decoder != nullptr, AMF_NOT_INITIALIZED, L"VideoDecodeEngine::Init(): Video decoder has not been instantiated");
        AMF_RESULT result = m_Decoder->Init(amf::AMF_SURFACE_FORMAT(format), resolution.width, resolution.height);
        AMF_RETURN_IF_FAILED(result, L"m_Decoder->Init(%s, %d, %d) failed", AMFSurfaceGetFormatName(format), resolution.width, resolution.height);

        m_Format = format;
        m_Resolution = resolution;
        m_FrameRate = frameRate;
        m_Stats.Reset();
        m_FirstFrameAfterInit = true;
        m_Initialized = true;

        return AMF_OK;
    }

    AMF_RESULT VideoDecodeEngine::Terminate()
    {
        AMF_RESULT result = AMF_OK;
        amf::AMFComponentPtr decoder;
        {
            amf::AMFLock lock(&m_Guard);
            decoder = m_Decoder;
            m_Stats.Reset();
            m_Initialized = false;
        }
        if (decoder != nullptr)
        {
            result = decoder->Terminate();
        }
        return result;
    }

    AMF_RESULT VideoDecodeEngine::SubmitInput(amf::AMFBuffer* input, transport_common::VideoFrame::SubframeType frameType)
    {
        AMF_RESULT result = AMF_OK;
        amf::AMFComponentPtr decoder;
        amf::AMFBufferPtr fullFrame;

        m_Stats.InputSubmitted(input->GetPts());
        amf_pts now = amf_high_precision_clock();
        ssdk::util::ComponentStats::BasicStatsSnapshot statsSnapshot;
        m_Stats.GetBasicStatsSnapshot(statsSnapshot);
        if ((now - m_Stats.GetLastResetTime()) > (1 * AMF_SECOND))
        {
//            AMFTraceDebug(AMF_FACILITY, L"Decoder stats: input rate: %5.2f fps, output rate: %5.2f fps, submitted: %lld, decoded: %lld, in transit %lld, avg latency: %5.2f ms", statsSnapshot.avgInputRate, statsSnapshot.avgOutputRate, statsSnapshot.inputCnt, statsSnapshot.outputCnt, statsSnapshot.inTransitCnt, float(statsSnapshot.avgLatency) / AMF_MILLISECOND);
            m_Stats.Reset();
        }


        {
            amf::AMFLock lock(&m_Guard);
            AMF_RETURN_IF_FALSE(m_Decoder != nullptr, AMF_NOT_INITIALIZED, L"VideoDecodeEngine::SubmitFrame(): Video decoder has not been instantiated");
            AMF_RETURN_IF_FALSE(m_Initialized == true, AMF_NOT_INITIALIZED, L"VideoDecodeEngine::SubmitFrame(): Video decoder has not been initialized yet, submit the init block (SPS/PPS) first");
            decoder = m_Decoder;
        }

        {
            amf::AMFLock lock(&m_InputGuard);
            if (m_Slices.size() > 0)
            {
                if (m_Slices.front()->GetPts() != input->GetPts())
                {
                    AMFTraceWarning(AMF_FACILITY, L"New slice belongs to a diffrent frame: expected pts=%lld new pts=%lld. Frame dropped", m_Slices.front()->GetPts(), input->GetPts());
                    m_Slices.clear();
                }
            }

            //  Sliced frames arrive as a sequence of slices with the same PTS with the last slice marked as a frame
            if (frameType == transport_common::VideoFrame::SubframeType::SLICE)
            {   //  Accumulate input slices until we get a buffer marked as frame rather than a slice
                m_Slices.push_back(input);
                result = AMF_OK;
            }
            else
            {   //  This is either the last slice of the sliced frame or a full frame
                if (m_Slices.size() > 0) // combine buffers
                {
                    m_Slices.push_back(input);
                    size_t fullSize = 0;
                    for (FrameSlices::const_iterator it = m_Slices.begin(); it != m_Slices.end(); it++)
                    {
                        fullSize += (*it)->GetSize();
                    }
                    result = m_Context->AllocBuffer(amf::AMF_MEMORY_HOST, fullSize, &fullFrame);
                    AMF_RETURN_IF_FAILED(result, L"AllocBuffer() failed");

                    uint8_t* data = static_cast<uint8_t*>(fullFrame->GetNative());
                    for (FrameSlices::const_iterator it = m_Slices.begin(); it != m_Slices.end(); it++)
                    {
                        memcpy(data, (*it)->GetNative(), (*it)->GetSize());
                        data += (*it)->GetSize();
                    }
                    m_Slices.clear();
                    input->CopyTo(fullFrame, true);
                    fullFrame->SetPts(input->GetPts());
                }
                else
                {
                    fullFrame = input;
                }
            }
            if (fullFrame != nullptr)
            {
                if (m_FirstFrameAfterInit == true && (frameType != transport_common::VideoFrame::SubframeType::IDR && frameType != transport_common::VideoFrame::SubframeType::I))
                {
                    AMFTraceWarning(AMF_FACILITY, L"The first frame after decoder init must be an I/IDR/Key frame, but received type %d, ignored", static_cast<int>(frameType));
                }
                else
                {
                    result = decoder->SubmitInput(fullFrame);
                    m_FirstFrameAfterInit = false;
                }
            }
        }
        return result;
    }

    AMF_RESULT VideoDecodeEngine::QueryOutput(amf::AMFSurface** output)
    {
        AMF_RESULT result = AMF_OK;
        amf::AMFComponentPtr decoder;
        {
            amf::AMFLock lock(&m_Guard);
            AMF_RETURN_IF_FALSE(m_Decoder != nullptr, AMF_NOT_INITIALIZED, L"VideoDecodeEngine::QueryOutput(): Video decoder has not been instantiated");
            AMF_RETURN_IF_FALSE(m_Initialized == true, AMF_NOT_INITIALIZED, L"VideoDecodeEngine::QueryOutput(): Video decoder has not been initialized yet, submit the init block (SPS/PPS) first");
            AMF_RETURN_IF_FALSE(output != nullptr, AMF_INVALID_ARG, L"VideoDecodeEngine::QueryOutput(): Video decoder output should not be NULL");
            decoder = m_Decoder;
        }
        amf::AMFDataPtr outData;
        result = decoder->QueryOutput(&outData);
        if (outData != nullptr)
        {
            m_Stats.OutputObtained(outData->GetPts());
            amf::AMFSurfacePtr outSurface(outData);
            *output = outSurface;
            (*output)->Acquire();
        }
        return result;
    }


}
