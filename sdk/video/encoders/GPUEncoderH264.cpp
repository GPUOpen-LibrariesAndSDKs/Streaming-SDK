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

#include "GPUEncoderH264.h"
#include "amf/public/common/AMFFactory.h"
#include "amf/public/common/TraceAdapter.h"
#include "amf/public/include/components/VideoEncoderVCE.h"

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::video::GPUEncoderH264";

namespace ssdk::video
{
    GPUEncoderH264::GPUEncoderH264(amf::AMFContext* context) :
        VideoEncodeEngine(context)
    {
        AMF_RESULT result = g_AMFFactory.GetFactory()->CreateComponent(context, AMFVideoEncoderVCE_AVC, &m_Encoder);
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

    AMF_RESULT GPUEncoderH264::Init(const AMFSize& encoderResolution, int64_t bitrate, float frameRate, int64_t intraRefreshPeriod, const ColorParameters& inputColorParams, size_t instance)
    {
        AMF_RESULT result = AMF_OK;
        AMF_RETURN_IF_FALSE(m_Encoder != nullptr, AMF_FAIL, L"H.264 Encoder is not initialized");

        VideoEncodeEngine::Init(encoderResolution, bitrate, frameRate, intraRefreshPeriod, inputColorParams, instance);

        result = m_Encoder->SetProperty(AMF_VIDEO_ENCODER_INSTANCE_INDEX, static_cast<int64_t>(instance));
        AMF_RETURN_IF_FAILED(result, L"Invalid encoder instance");

        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_USAGE, AMF_VIDEO_ENCODER_USAGE_TRANSCODING);

        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_B_PIC_PATTERN, 0);
        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_QUALITY_PRESET, AMF_VIDEO_ENCODER_QUALITY_PRESET_SPEED);

        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_PROFILE, AMF_VIDEO_ENCODER_PROFILE_MAIN);
        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_PROFILE_LEVEL, 52);
        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_FULL_RANGE_COLOR, true);
//        m_Encoder->SetProperty(AMF_VIDEO_MAX_DEC_FRAME_BUFFERING, 0);
        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_LOWLATENCY_MODE, true); // sets high priority queue for m_Encoder and enables POC mode = 2 - display order same as decoding order

        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_RATE_CONTROL_METHOD, AMF_VIDEO_ENCODER_RATE_CONTROL_METHOD_PEAK_CONSTRAINED_VBR);

        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_FRAMESIZE, encoderResolution);
        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_FRAMERATE, AMFConstructRate((amf_uint32)frameRate, 1));

        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_QUERY_TIMEOUT, 20);

        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_IDR_PERIOD, intraRefreshPeriod);
        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_HEADER_INSERTION_SPACING, intraRefreshPeriod);

        amf_int64 macroBlocks = ((AlignValue(encoderResolution.width, 16) / 16) * (AlignValue(encoderResolution.height, 16) / 16));
        amf_int64 MBPerSlot = macroBlocks / 8;
        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_INTRA_REFRESH_NUM_MBS_PER_SLOT, MBPerSlot);

        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_INPUT_QUEUE_SIZE, 3);

        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_INPUT_TRANSFER_CHARACTERISTIC, inputColorParams.GetXferCharacteristic());
        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_INPUT_COLOR_PRIMARIES, inputColorParams.GetColorPrimaries());
        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_INPUT_COLOR_PROFILE, inputColorParams.GetColorProfile());
        m_Encoder->SetProperty(AMF_VIDEO_ENCODER_OUTPUT_COLOR_PROFILE, AMF_VIDEO_CONVERTER_COLOR_PROFILE_FULL_709);   // h.264 does not support HDR, output color profile must always be Full 709

        result = UpdateBitrate(bitrate);
        AMF_RETURN_IF_FAILED(result, L"Failed to update bitrate on AVC encoder to %lld", bitrate);

        result = m_Encoder->Init(inputColorParams.GetFormat(), encoderResolution.width, encoderResolution.height);
        AMF_RETURN_IF_FAILED(result, L"Failed to initialize %s m_Encoder, format: %s, resolution: %dx%d", AMFVideoEncoderVCE_AVC, AMFSurfaceGetFormatName(inputColorParams.GetFormat()), encoderResolution.width, encoderResolution.height);
        AMFTraceInfo(AMF_FACILITY, L"h.264 Encoder successfully initialized, format: %s, resolution: %dx%d, xfer: %d, primaries: %d, color profile: %d",
            AMFSurfaceGetFormatName(inputColorParams.GetFormat()), encoderResolution.width, encoderResolution.height, inputColorParams.GetXferCharacteristic(), inputColorParams.GetColorPrimaries(), inputColorParams.GetColorProfile());
        return AMF_OK;
    }

    const char* GPUEncoderH264::GetCodecName() const noexcept
    {
        return CODEC_H264;
    }

    AMF_RESULT GPUEncoderH264::UpdateBitrate(int64_t bitRate)
    {
        AMF_RETURN_IF_FALSE(m_Encoder != nullptr, AMF_FAIL, L"Encoder is not initialized");

        AMF_RESULT result = AMF_FAIL;
        int64_t targetBitrate = bitRate * 9 / 10;
        result = m_Encoder->SetProperty(AMF_VIDEO_ENCODER_TARGET_BITRATE, targetBitrate);
        AMF_RETURN_IF_FAILED(result, L"Failed to set AMF_VIDEO_ENCODER_TARGET_BITRATE to %lld", targetBitrate);
        result = m_Encoder->SetProperty(AMF_VIDEO_ENCODER_PEAK_BITRATE, bitRate);
        AMF_RETURN_IF_FAILED(result, L"Failed to set AMF_VIDEO_ENCODER_PEAK_BITRATE to %lld", bitRate);

        AMFRate frameRate = {};
        m_Encoder->GetProperty(AMF_VIDEO_ENCODER_FRAMERATE, &frameRate);
        amf_int64 vbvSize = bitRate * frameRate.den / frameRate.num;
        result = m_Encoder->SetProperty(AMF_VIDEO_ENCODER_VBV_BUFFER_SIZE, vbvSize);
        AMF_RETURN_IF_FAILED(result, L"Failed to set AMF_VIDEO_ENCODER_VBV_BUFFER_SIZE to %lld", vbvSize);
        return result;
    }

    AMF_RESULT GPUEncoderH264::UpdateFramerate(const AMFRate& rate)
    {
        AMF_RETURN_IF_FALSE(m_Encoder != nullptr, AMF_FAIL, L"Encoder is not initialized");
        AMF_RESULT result = m_Encoder->SetProperty(AMF_VIDEO_ENCODER_FRAMERATE, rate);
        if (result == AMF_OK)
        {
            m_Framerate = float(rate.num) / float(rate.den);
        }
        return result;
    }

    AMF_RESULT GPUEncoderH264::GetExtraData(amf::AMFBuffer** pExtraData) const
    {
        AMF_RETURN_IF_FALSE(m_Encoder != nullptr, AMF_FAIL, L"Encoder is not initialized");
        AMF_RETURN_IF_FALSE(pExtraData != nullptr, AMF_INVALID_ARG, L"GetExtraData(pExtraData) parameter must not be NULL");
        amf::AMFInterfacePtr pIface;
        AMF_RESULT result = m_Encoder->GetProperty(AMF_VIDEO_ENCODER_EXTRADATA, &pIface);
        if (result == AMF_OK)
        {
            amf::AMFBufferPtr buf(pIface);
            *pExtraData = buf;
            (*pExtraData)->Acquire();
        }
        return result;
    }

    AMF_RESULT GPUEncoderH264::ForceKeyFrame(amf::AMFData* pSurface, bool bSet)
    {
        bool bOldValue = false;
        if (pSurface->GetProperty(AMF_VIDEO_ENCODER_FORCE_PICTURE_TYPE, &bOldValue) != AMF_OK && bSet == false)
        {
            return AMF_OK;
        }
        return pSurface->SetProperty(AMF_VIDEO_ENCODER_FORCE_PICTURE_TYPE, bSet ? AMF_VIDEO_ENCODER_PICTURE_TYPE_IDR : AMF_VIDEO_ENCODER_PICTURE_TYPE_NONE);
    }

    AMF_RESULT GPUEncoderH264::DetermineFrameType(amf::AMFBuffer* buffer, transport_common::VideoFrame::SubframeType& frameType) const
    {
        AMF_RESULT result;
        amf_int64 eEncodedType = 0;
        result = buffer->GetProperty(AMF_VIDEO_ENCODER_OUTPUT_DATA_TYPE, &eEncodedType);
        AMF_RETURN_IF_FAILED(result, L"Failed to get the AMF_VIDEO_ENCODER_OUTPUT_DATA_TYPE property: %s", amf::AMFGetResultText(result));
        switch (eEncodedType)
        {
        case AMF_VIDEO_ENCODER_OUTPUT_DATA_TYPE_IDR:
            frameType = transport_common::VideoFrame::SubframeType::IDR;
            break;
        case AMF_VIDEO_ENCODER_OUTPUT_DATA_TYPE_I:
            frameType = transport_common::VideoFrame::SubframeType::I;
            break;
        case AMF_VIDEO_ENCODER_OUTPUT_DATA_TYPE_P:
            frameType = transport_common::VideoFrame::SubframeType::P;
            break;
        case AMF_VIDEO_ENCODER_OUTPUT_DATA_TYPE_B:
            frameType = transport_common::VideoFrame::SubframeType::B;
            break;
        default:
            frameType = transport_common::VideoFrame::SubframeType::UNKNOWN;
            break;
        }
        return AMF_OK;
    }

    AMF_RESULT GPUEncoderH264::GetNumOfEncoderInstances(size_t& numOfInstances) const
    {
        return GetNumOfEncoderInstancesPrivate(L"H.264", AMF_VIDEO_ENCODER_CAP_NUM_OF_HW_INSTANCES, numOfInstances);
    }

    bool GPUEncoderH264::IsHDRSupported() const noexcept
    {
        return false;
    }

    AMF_RESULT GPUEncoderH264::EnableHDR(bool /*enable*/) noexcept
    {
        return AMF_NOT_SUPPORTED;
    }

}