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

#include "UVDDecoder.h"
#include "amf/public/include/components/VideoDecoderUVD.h"
#include "amf/public/common/TraceAdapter.h"

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::video::UVDDecodeEngine";

namespace ssdk::video
{
    UVDDecoder::UVDDecoder(amf::AMFContext* context, size_t instance) :
        VideoDecodeEngine(context),
        m_Instance(instance)
    {
    }
    UVDDecoder::~UVDDecoder()
    {
    }
    AMF_RESULT UVDDecoder::Init(amf::AMF_SURFACE_FORMAT format, const AMFSize& resolution, float frameRate, amf::AMFBuffer* initBlock)
    {
        amf::AMFLock lock(&m_Guard);
        AMF_RETURN_IF_FALSE(m_Decoder != nullptr, AMF_NOT_INITIALIZED, L"UVDDecoder::Init(): Video decoder has not been instantiated");

        AMF_RESULT result;

        //  Put the decoder into the low latency mode to avoid frame accumulation
        result = m_Decoder->SetProperty(AMF_TIMESTAMP_MODE, AMF_TS_DECODE);
        AMF_RETURN_IF_FAILED(result, L"UVD Video decoder failed to set the timestamp mode to AMF_TS_DECODE, result = %s", amf::AMFGetResultText(result));

        result = m_Decoder->SetProperty(AMF_VIDEO_DECODER_REORDER_MODE, AMF_VIDEO_DECODER_MODE_LOW_LATENCY);
        AMF_RETURN_IF_FAILED(result, L"UVD Video decoder failed to reorder mode to low latency mode, result = %s", amf::AMFGetResultText(result));

        result = m_Decoder->SetProperty(AMF_VIDEO_DECODER_LOW_LATENCY, true);
        if (result != AMF_OK)
        {
            AMFTraceError(AMF_FACILITY, L"UVD Video decoder failed to enable low latency mode, result = %s", amf::AMFGetResultText(result));

        }
        
        result = m_Decoder->SetProperty(AMF_VIDEO_DECODER_FRAME_RATE, double(frameRate));
        AMF_RETURN_IF_FAILED(result, L"UVD Video decoder failed to set frame rate to %5.2f, result = %s", double(frameRate), amf::AMFGetResultText(result));

        constexpr static int surfacePoolSize = 20;
        result = m_Decoder->SetProperty(AMF_VIDEO_DECODER_SURFACE_POOL_SIZE, int64_t(surfacePoolSize));
        AMF_RETURN_IF_FAILED(result, L"UVD Video decoder failed to set the surface pool size to %d, result = %s", surfacePoolSize, amf::AMFGetResultText(result));

        result = m_Decoder->SetProperty(AMF_VIDEO_DECODER_FULL_RANGE_COLOR, true);
        AMF_RETURN_IF_FAILED(result, L"UVD Video decoder failed to set the full color range, result = %s", amf::AMFGetResultText(result));

        amf::AMFCapsPtr decoderCaps(m_Decoder);
        if (decoderCaps != nullptr)
        {
            int64_t numOfInstances = 0;
            if (decoderCaps->GetProperty(AMF_VIDEO_DECODER_CAP_NUM_OF_HW_INSTANCES, &numOfInstances) == AMF_OK && m_Instance < static_cast<size_t>(numOfInstances))
            {
                result = m_Decoder->SetProperty(AMF_VIDEO_DECODER_INSTANCE_INDEX, m_Instance);
                AMF_RETURN_IF_FAILED(result, L"Failed to select h/w instance %d of UVD video decoder, result = %s", m_Instance, amf::AMFGetResultText(result));
            }
        }

        if (initBlock != nullptr)
        {
            result = m_Decoder->SetProperty(AMF_VIDEO_DECODER_EXTRADATA, initBlock);
            AMF_RETURN_IF_FAILED(result, L"Failed to set the ExtraData block on the UVD video decoder, result = %s", amf::AMFGetResultText(result));
        }

        return VideoDecodeEngine::Init(format, resolution, frameRate, initBlock);
    }
}