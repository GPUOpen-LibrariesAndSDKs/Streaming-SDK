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

#include "AMFAudioEncoder.h"
#include "amf/public/include/components/FFMPEGComponents.h"
#include "amf/public/include/components/FFMPEGAudioEncoder.h"
#include "amf/public/common/TraceAdapter.h"

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::audio::AMFAudioEncoder";

namespace ssdk::audio
{
    AMFAudioEncoder::AMFAudioEncoder(amf::AMFContext* context, int codecID) :
        m_Context(context),
        m_CodecID(codecID)
    {
    }

    AMFAudioEncoder::~AMFAudioEncoder()
    {
        Terminate();
        m_Context = nullptr;
    }

    AMF_RESULT AMFAudioEncoder::Init(int32_t samplingRate, int32_t channels, int32_t layout, int32_t bitrate)
    {
        amf::AMFLock lockIn(&m_InputGuard);
        amf::AMFLock lockOut(&m_OutputGuard);

        AMF_RETURN_IF_FALSE(m_Context != nullptr, AMF_FAIL, L"AMFContext passed to AMFAudioEncoder must not be NULL");
        Terminate();
        AMF_RESULT result = AudioEncodeEngine::Init(channels, layout, samplingRate, bitrate);
        AMF_RETURN_IF_FAILED(result, L"AMFAudioEncoder::Init(): audio encoder failed to initialize, result=%s", amf::AMFGetResultText(result));

        amf::AMFComponentPtr encoder;
        result = g_AMFFactory.LoadExternalComponent(m_Context, FFMPEG_DLL_NAME, "AMFCreateComponentInt", (void*)FFMPEG_AUDIO_ENCODER, &encoder);
        AMF_RETURN_IF_FAILED(result, L"LoadExternalComponent(%s) failed", FFMPEG_AUDIO_ENCODER);

        result = encoder->SetProperty(AUDIO_ENCODER_AUDIO_CODEC_ID, amf_int64(m_CodecID));
        AMF_RETURN_IF_FAILED(result, L"Failed to set codec ID %d, result = %s", m_CodecID, amf::AMFGetResultText(result));

        result = encoder->SetProperty(AUDIO_ENCODER_IN_AUDIO_SAMPLE_RATE, samplingRate);
        AMF_RETURN_IF_FAILED(result, L"Invalid sampling rate %d, result = %s", samplingRate, amf::AMFGetResultText(result));

        result = encoder->SetProperty(AUDIO_ENCODER_IN_AUDIO_CHANNELS, channels);
        AMF_RETURN_IF_FAILED(result, L"Invalid encoder input channels %d, result = %s", channels, amf::AMFGetResultText(result));
        result = encoder->SetProperty(AUDIO_ENCODER_OUT_AUDIO_CHANNELS, channels);
        AMF_RETURN_IF_FAILED(result, L"Invalid encoder output channels %d, result = %s", channels, amf::AMFGetResultText(result));

        result = encoder->SetProperty(AUDIO_ENCODER_IN_AUDIO_CHANNEL_LAYOUT, layout);
        AMF_RETURN_IF_FAILED(result, L"Invalid encoder input channel layout %d, result = %s", layout, amf::AMFGetResultText(result));
        result = encoder->SetProperty(AUDIO_ENCODER_OUT_AUDIO_CHANNEL_LAYOUT, layout);
        AMF_RETURN_IF_FAILED(result, L"Invalid encoder output channel layout %d, result = %s", layout, amf::AMFGetResultText(result));

        result = encoder->SetProperty(AUDIO_ENCODER_OUT_AUDIO_BIT_RATE, bitrate);
        AMF_RETURN_IF_FAILED(result, L"Invalid encoder output bitrate %d, result = %s", bitrate, amf::AMFGetResultText(result));

        result = encoder->Init(amf::AMF_SURFACE_UNKNOWN, 0, 0);
        AMF_RETURN_IF_FAILED(result, L"Failed to initialize audio encoder, result = %s", amf::AMFGetResultText(result));

        //  Some codecs might not support specific sampling rates, formats and so on, so read them back after initializing the encoder in case they were reverted to their defaults
        //  This is necessary to initialize the audio converter in such a way that its output matches the encoder input
        int64_t format;
        encoder->GetProperty(AUDIO_ENCODER_IN_AUDIO_SAMPLE_FORMAT, &format);
        m_Format = static_cast<amf::AMF_AUDIO_FORMAT>(format);

        int64_t encoderSamplingRate;
        encoder->GetProperty(AUDIO_ENCODER_IN_AUDIO_SAMPLE_RATE, &encoderSamplingRate);
        m_SamplingRate = static_cast<int32_t>(encoderSamplingRate);

        int64_t encoderChannels;
        encoder->GetProperty(AUDIO_ENCODER_IN_AUDIO_CHANNELS, &encoderChannels);
        m_Channels = static_cast<int32_t>(encoderChannels);

        int64_t encoderLayout;
        encoder->GetProperty(AUDIO_ENCODER_IN_AUDIO_CHANNEL_LAYOUT, &encoderLayout);
        m_Layout = static_cast<int32_t>(encoderLayout);

        m_Encoder = encoder;
        return result;
    }

    void AMFAudioEncoder::Terminate()
    {
        amf::AMFLock lockIn(&m_InputGuard);
        amf::AMFLock lockOut(&m_OutputGuard);

        if (m_Encoder != nullptr)
        {
            m_Encoder->Terminate();
            m_Encoder = nullptr;
        }
    }

    AMF_RESULT AMFAudioEncoder::SubmitInput(amf::AMFAudioBuffer* input)
    {
        amf::AMFLock lock(&m_InputGuard);
        AMF_RETURN_IF_FALSE(m_Encoder != nullptr, AMF_NOT_INITIALIZED, L"SubmitInput() failed, encoder not initialized");
        return m_Encoder->SubmitInput(input);
    }

    AMF_RESULT AMFAudioEncoder::QueryOutput(amf::AMFBuffer** outputBuffer)
    {
        amf::AMFLock lock(&m_OutputGuard);
        AMF_RETURN_IF_FALSE(m_Encoder != nullptr, AMF_NOT_INITIALIZED, L"QueryOutput() failed, encoder not initialized");
        amf::AMFDataPtr outData;
        AMF_RESULT result = m_Encoder->QueryOutput(&outData);
        if (result == AMF_OK && outData != nullptr)
        {
            *outputBuffer = amf::AMFBufferPtr(outData);
            (*outputBuffer)->Acquire();
        }
        return result;
    }

    AMF_RESULT AMFAudioEncoder::Flush()
    {
        amf::AMFLock lockIn(&m_InputGuard);
        amf::AMFLock lockOut(&m_OutputGuard);
        AMF_RETURN_IF_FALSE(m_Encoder != nullptr, AMF_NOT_INITIALIZED, L"Flush() failed, encoder not initialized");
        return m_Encoder->Flush();
    }

    AMF_RESULT AMFAudioEncoder::Drain()
    {
        amf::AMFLock lockIn(&m_InputGuard);
        amf::AMFLock lockOut(&m_OutputGuard);
        AMF_RETURN_IF_FALSE(m_Encoder != nullptr, AMF_NOT_INITIALIZED, L"Drain() failed, encoder not initialized");
        return m_Encoder->Drain();
    }

    AMF_RESULT AMFAudioEncoder::GetExtraData(amf::AMFBuffer** pExtraData) const
    {
        amf::AMFLock lockIn(&m_InputGuard);
        amf::AMFLock lockOut(&m_OutputGuard);
        AMF_RETURN_IF_FALSE(m_Encoder != nullptr, AMF_FAIL, L"Encoder is not initialized");
        AMF_RETURN_IF_FALSE(pExtraData != nullptr, AMF_INVALID_ARG, L"GetExtraData(pExtraData) parameter must not be NULL");
        amf::AMFInterfacePtr pIface;
        AMF_RESULT result = m_Encoder->GetProperty(AUDIO_ENCODER_OUT_AUDIO_EXTRA_DATA, &pIface);
        if (result == AMF_OK)
        {
            amf::AMFBufferPtr buf(pIface);
            *pExtraData = buf;
            (*pExtraData)->Acquire();
        }

        return AMF_OK;
    }

    AMF_RESULT AMFAudioEncoder::SetProperty(const wchar_t* name, const amf::AMFVariant& value)
    {
        amf::AMFLock lockIn(&m_InputGuard);
        amf::AMFLock lockOut(&m_OutputGuard);
        AMF_RETURN_IF_FALSE(m_Encoder != nullptr, AMF_NOT_INITIALIZED, L"SetProperty() failed, encoder not initialized");
        return m_Encoder->SetProperty(name, value);
    }

    AMF_RESULT AMFAudioEncoder::GetProperty(const wchar_t* name, amf::AMFVariant& value) const
    {
        amf::AMFLock lockIn(&m_InputGuard);
        amf::AMFLock lockOut(&m_OutputGuard);
        AMF_RETURN_IF_FALSE(m_Encoder != nullptr, AMF_NOT_INITIALIZED, L"GetProperty() failed, encoder not initialized");
        return m_Encoder->GetProperty(name, &value);
    }

    AMF_RESULT AMFAudioEncoder::UpdateBitrate(int32_t bitrate)
    {
        amf::AMFLock lockIn(&m_InputGuard);
        amf::AMFLock lockOut(&m_OutputGuard);
        AMF_RETURN_IF_FALSE(m_Encoder != nullptr, AMF_NOT_INITIALIZED, L"UpdateBitrate() failed, encoder not initialized");
        AMF_RESULT result = m_Encoder->SetProperty(AUDIO_ENCODER_OUT_AUDIO_BIT_RATE, bitrate);
        AMF_RETURN_IF_FAILED(result, L"Invalid encoder output bitrate %d, result = %s", bitrate, amf::AMFGetResultText(result));
        return result;
    }
}
