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

#include "GPUEncoderHEVC.h"
#include "amf/public/common/AMFFactory.h"
#include "amf/public/common/TraceAdapter.h"
#include "amf/public/include/components/VideoEncoderHEVC.h"

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::video::GPUEncoderHEVC";

namespace ssdk::video
{
    GPUEncoderHEVC::GPUEncoderHEVC(amf::AMFContext* context) :
        HDREncoder(context)
    {
        AMF_RESULT result = g_AMFFactory.GetFactory()->CreateComponent(context, AMFVideoEncoder_HEVC, &m_Encoder);
        if (result != AMF_OK)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to create h.264 encoder component, result=%s", amf::AMFGetResultText(result));
        }
        else
        {
            m_PreferredSDRFormat = amf::AMF_SURFACE_NV12;
            m_PreferredHDRFormat = amf::AMF_SURFACE_P010;
        }
    }

    AMF_RESULT GPUEncoderHEVC::Init(const AMFSize& encoderResolution, int64_t bitrate, float frameRate, int64_t intraRefreshPeriod, const ColorParameters& inputColorParams, size_t instance)
    {
        AMF_RESULT result = AMF_OK;
        AMF_RETURN_IF_FALSE(m_Encoder != nullptr, AMF_FAIL, L"HEVC Encoder is not initialized");

        HDREncoder::Init(encoderResolution, bitrate, frameRate, intraRefreshPeriod, inputColorParams, instance);

        result = m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_INSTANCE_INDEX, static_cast<int64_t>(instance));
        AMF_RETURN_IF_FAILED(result, L"Invalid encoder instance");

        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_USAGE, AMF_VIDEO_ENCODER_HEVC_USAGE_TRANSCODING);

        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_QUALITY_PRESET, AMF_VIDEO_ENCODER_HEVC_QUALITY_PRESET_SPEED);

        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_FRAMESIZE, encoderResolution);
        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_FRAMERATE, AMFConstructRate((amf_uint32)frameRate, 1));

        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_PROFILE, AMF_VIDEO_ENCODER_HEVC_PROFILE_MAIN);
        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_PROFILE_LEVEL, AMF_LEVEL_5_2);
        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_NOMINAL_RANGE, AMF_VIDEO_ENCODER_HEVC_NOMINAL_RANGE_FULL);

        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_LOWLATENCY_MODE, true); // sets high priority queue for m_Encoder and enables POC mode = 2 - display order same as decoding order

        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_QUERY_TIMEOUT, 20);

        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_RATE_CONTROL_METHOD, AMF_VIDEO_ENCODER_HEVC_RATE_CONTROL_METHOD_PEAK_CONSTRAINED_VBR);

        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_GOP_SIZE, intraRefreshPeriod);
        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_NUM_GOPS_PER_IDR, 1);
        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_HEADER_INSERTION_MODE, AMF_VIDEO_ENCODER_HEVC_HEADER_INSERTION_MODE_IDR_ALIGNED);

        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_MIN_QP_I, 0);
        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_MAX_QP_I, 51);

        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_MIN_QP_P, 0);
        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_MAX_QP_P, 51);

        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_INPUT_QUEUE_SIZE, 3);

        amf_int64 CTBBlocks = ((AlignValue(encoderResolution.width, 64) / 64) * (AlignValue(encoderResolution.height, 64) / 64));
        amf_int64 CTBBlocksPerSlot = CTBBlocks / 8;
        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_INTRA_REFRESH_NUM_CTBS_PER_SLOT, CTBBlocksPerSlot);

        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_INPUT_TRANSFER_CHARACTERISTIC, inputColorParams.GetXferCharacteristic());
        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_INPUT_COLOR_PRIMARIES, inputColorParams.GetColorPrimaries());
        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_INPUT_COLOR_PROFILE, inputColorParams.GetColorProfile());

        ToggleHDR(m_HDREnabled);

        result = UpdateBitrate(bitrate);
        AMF_RETURN_IF_FAILED(result, L"Failed to update bitrate on HEVC encoder to %lld", bitrate);

        result = m_Encoder->Init(inputColorParams.GetFormat(), encoderResolution.width, encoderResolution.height);
        AMF_RETURN_IF_FAILED(result, L"Failed to initialize %s m_Encoder, format: %s, resolution: %dx%d", AMFVideoEncoder_HEVC, AMFSurfaceGetFormatName(inputColorParams.GetFormat()), encoderResolution.width, encoderResolution.height);
        AMFTraceInfo(AMF_FACILITY, L"HEVC Encoder successfully initialized, format: %s, resolution: %dx%d, xfer: %d, primaries: %d, color profile: %d",
            AMFSurfaceGetFormatName(inputColorParams.GetFormat()), encoderResolution.width, encoderResolution.height, inputColorParams.GetXferCharacteristic(), inputColorParams.GetColorPrimaries(), inputColorParams.GetColorProfile());
        return AMF_OK;
    }

    const char* GPUEncoderHEVC::GetCodecName() const noexcept
    {
        return CODEC_HEVC;
    }

    bool GPUEncoderHEVC::IsFormatSupported(amf::AMF_SURFACE_FORMAT defaultFormat, bool hdr, amf::AMFSurface* pSurface) const
    {
        amf::AMF_SURFACE_FORMAT format = defaultFormat;
        if (hdr == true)
        {
            int64_t xfer = AMF_COLOR_TRANSFER_CHARACTERISTIC_UNDEFINED;
            if (pSurface != nullptr)
            {
                format = pSurface->GetFormat(); //  Encoder can be initialized before the first frame, in this case we check the default format, otherwise take it from the input surface
                pSurface->GetProperty(AMF_VIDEO_COLOR_TRANSFER_CHARACTERISTIC, &xfer);
            }
            if (format == amf::AMF_SURFACE_BGRA ||
                format == amf::AMF_SURFACE_RGBA ||
                format == amf::AMF_SURFACE_RGBA_F16 ||
                (format == amf::AMF_SURFACE_R10G10B10A2 && xfer != AMF_COLOR_TRANSFER_CHARACTERISTIC_SMPTE2084))
            {
                return false;
            }
        }
        return VideoEncodeEngine::IsFormatSupported(format, hdr, pSurface);
    }

    AMF_RESULT GPUEncoderHEVC::UpdateBitrate(int64_t bitRate)
    {
        AMF_RETURN_IF_FALSE(m_Encoder != nullptr, AMF_FAIL, L"Encoder is not initialized");

        AMF_RESULT result = AMF_FAIL;
        int64_t targetBitrate = bitRate * 9 / 10;
        result = m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_TARGET_BITRATE, targetBitrate);
        AMF_RETURN_IF_FAILED(result, L"Failed to set AMF_VIDEO_ENCODER_TARGET_BITRATE to %lld", targetBitrate);
        result = m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_PEAK_BITRATE, bitRate);
        AMF_RETURN_IF_FAILED(result, L"Failed to set AMF_VIDEO_ENCODER_PEAK_BITRATE to %lld", bitRate);

        AMFRate frameRate = {};
        m_Encoder->GetProperty(AMF_VIDEO_ENCODER_HEVC_FRAMERATE, &frameRate);
        amf_int64 vbvSize = bitRate * frameRate.den / frameRate.num;
        result = m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_VBV_BUFFER_SIZE, vbvSize);
        AMF_RETURN_IF_FAILED(result, L"Failed to set AMF_VIDEO_ENCODER_VBV_BUFFER_SIZE to %lld", vbvSize);
        return result;
    }

    AMF_RESULT GPUEncoderHEVC::UpdateFramerate(const AMFRate& rate)
    {
        AMF_RETURN_IF_FALSE(m_Encoder != nullptr, AMF_FAIL, L"Encoder is not initialized");
        return m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_FRAMERATE, rate);
    }

    AMF_RESULT GPUEncoderHEVC::GetExtraData(amf::AMFBuffer** pExtraData) const
    {
        AMF_RETURN_IF_FALSE(m_Encoder != nullptr, AMF_FAIL, L"Encoder is not initialized");
        AMF_RETURN_IF_FALSE(pExtraData != nullptr, AMF_INVALID_ARG, L"GetExtraData(pExtraData) parameter must not be NULL");
        amf::AMFInterfacePtr pIface;
        AMF_RESULT result = m_Encoder->GetProperty(AMF_VIDEO_ENCODER_HEVC_EXTRADATA, &pIface);
        if (result == AMF_OK)
        {
            amf::AMFBufferPtr buf(pIface);
            *pExtraData = buf;
            (*pExtraData)->Acquire();
        }
        return result;
    }

    AMF_RESULT GPUEncoderHEVC::SetHDRMetadata(const amf::AMFVariant& metadata)
    {
        return m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_INPUT_HDR_METADATA, metadata);
    }

    bool GPUEncoderHEVC::IsHDRSupported() const noexcept
    {
        return true;
    }

    void GPUEncoderHEVC::ToggleHDR(bool enable)
    {
        if (enable == true)
        {
            m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_COLOR_BIT_DEPTH, AMF_COLOR_BIT_DEPTH_10);
            m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_OUTPUT_TRANSFER_CHARACTERISTIC, AMF_COLOR_TRANSFER_CHARACTERISTIC_SMPTE2084);
            m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_OUTPUT_COLOR_PRIMARIES, AMF_COLOR_PRIMARIES_BT2020);
            m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_OUTPUT_COLOR_PROFILE, AMF_VIDEO_CONVERTER_COLOR_PROFILE_FULL_2020);
        }
        else
        {
            m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_COLOR_BIT_DEPTH, AMF_COLOR_BIT_DEPTH_8);
            m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_OUTPUT_TRANSFER_CHARACTERISTIC, AMF_COLOR_TRANSFER_CHARACTERISTIC_BT709);
            m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_OUTPUT_COLOR_PRIMARIES, AMF_COLOR_PRIMARIES_BT709);
            m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_OUTPUT_COLOR_PROFILE, AMF_VIDEO_CONVERTER_COLOR_PROFILE_FULL_709);
        }
    }

    AMF_RESULT GPUEncoderHEVC::ForceKeyFrame(amf::AMFData* pSurface, bool bSet)
    {
        bool bOldValue = false;
        if (pSurface->GetProperty(AMF_VIDEO_ENCODER_HEVC_FORCE_PICTURE_TYPE, &bOldValue) != AMF_OK && bSet == false)
        {
            return AMF_OK;
        }
        return pSurface->SetProperty(AMF_VIDEO_ENCODER_HEVC_FORCE_PICTURE_TYPE, bSet ? AMF_VIDEO_ENCODER_HEVC_PICTURE_TYPE_IDR : AMF_VIDEO_ENCODER_HEVC_PICTURE_TYPE_NONE);
    }

    AMF_RESULT GPUEncoderHEVC::DetermineFrameType(amf::AMFBuffer* buffer, transport_common::VideoFrame::SubframeType& frameType) const
    {
        AMF_RESULT result;
        amf_int64 eEncodedType = 0;
        result = buffer->GetProperty(AMF_VIDEO_ENCODER_HEVC_OUTPUT_DATA_TYPE, &eEncodedType);
        AMF_RETURN_IF_FAILED(result, L"Failed to get the AMF_VIDEO_ENCODER_OUTPUT_DATA_TYPE property: %s", amf::AMFGetResultText(result));
        switch (eEncodedType)
        {
        case AMF_VIDEO_ENCODER_HEVC_OUTPUT_DATA_TYPE_IDR:
            frameType = transport_common::VideoFrame::SubframeType::IDR;
            break;
        case AMF_VIDEO_ENCODER_HEVC_OUTPUT_DATA_TYPE_I:
            frameType = transport_common::VideoFrame::SubframeType::I;
            break;
        case AMF_VIDEO_ENCODER_HEVC_OUTPUT_DATA_TYPE_P:
            frameType = transport_common::VideoFrame::SubframeType::P;
            break;
        default:
            frameType = transport_common::VideoFrame::SubframeType::UNKNOWN;
            break;
        }
        return AMF_OK;
    }

    AMF_RESULT GPUEncoderHEVC::GetNumOfEncoderInstances(size_t& numOfInstances) const
    {
        return GetNumOfEncoderInstancesPrivate(L"HEVC", AMF_VIDEO_ENCODER_HEVC_CAP_NUM_OF_HW_INSTANCES, numOfInstances);
    }

}