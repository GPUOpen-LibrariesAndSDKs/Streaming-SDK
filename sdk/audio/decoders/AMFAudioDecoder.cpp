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

#include "AMFAudioDecoder.h"
#include "amf/public/include/components/FFMPEGComponents.h"
#include "amf/public/include/components/FFMPEGAudioDecoder.h"
#include "amf/public/common/TraceAdapter.h"

static constexpr const wchar_t* AMF_FACILITY = L"ssdk::audio::AMFAudioDecoder";

namespace ssdk::audio
{
    AMFAudioDecoder::AMFAudioDecoder(amf::AMFContext* context, int codecID) :
        m_Context(context),
        m_CodecID(codecID)
    {
    }

    AMFAudioDecoder::~AMFAudioDecoder()
    {
        Terminate();
        m_Context = nullptr;
    }

    AMF_RESULT AMFAudioDecoder::Init(amf::AMF_AUDIO_FORMAT format, int32_t channels, int32_t layout, int32_t samplingRate, amf::AMFBuffer* initBlock)
    {
        amf::AMFLock lockIn(&m_InputGuard);
        amf::AMFLock lockOut(&m_OutputGuard);

        AMF_RETURN_IF_FALSE(m_Context != nullptr, AMF_FAIL, L"AMFContext passed to AMFAudioEncoder must not be NULL");
        Terminate();

        AMF_RESULT result = AudioDecodeEngine::Init(format, channels, layout, samplingRate, initBlock);
        AMF_RETURN_IF_FAILED(result, L"AMFAudioDecoder::Init(): audio encoder failed to initialize, result=%s", amf::AMFGetResultText(result));

        amf::AMFComponentPtr decoder;
#if defined(__APPLE__)
        res = g_AMFFactory.GetFactory()->CreateComponent(m_pContext, AMFAudioDecoder_Apple_AAC, &decoder);
        AMF_RETURN_IF_FAILED(result, L"CreateComponent : AMFAudioDecoder_Apple_AAC FAILED");
#elif defined(__ANDROID__)
        res = g_AMFFactory.GetFactory()->CreateComponent(m_pContext, AMFAudioDecoder_Android_AAC, &decoder);
        AMF_RETURN_IF_FAILED(result, L"CreateComponent : AMFAudioDecoder_Android_AAC FAILED");
#else
        result = g_AMFFactory.LoadExternalComponent(m_Context, FFMPEG_DLL_NAME, "AMFCreateComponentInt", (void*)FFMPEG_AUDIO_DECODER, &decoder);
        AMF_RETURN_IF_FAILED(result, L"LoadExternalComponent(%s) failed", FFMPEG_AUDIO_DECODER);
#endif

        result = decoder->SetProperty(AUDIO_DECODER_IN_AUDIO_CODEC_ID, m_CodecID);
        AMF_RETURN_IF_FAILED(result, L"Failed to set codec ID %d, result = %s", m_CodecID, amf::AMFGetResultText(result));

        result = decoder->SetProperty(AUDIO_DECODER_IN_AUDIO_EXTRA_DATA, initBlock);
        AMF_RETURN_IF_FAILED(result, L"Failed to set the ExtraData block on the audio decoder, result = %s", amf::AMFGetResultText(result));

        result = decoder->SetProperty(AUDIO_DECODER_IN_AUDIO_SAMPLE_RATE, samplingRate);
        AMF_RETURN_IF_FAILED(result, L"Invalid sampling rate %d, result = %s", samplingRate, amf::AMFGetResultText(result));

        result = decoder->SetProperty(AUDIO_DECODER_IN_AUDIO_CHANNELS, channels);
        AMF_RETURN_IF_FAILED(result, L"Invalid decoder input channels %d, result = %s", channels, amf::AMFGetResultText(result));
        result = decoder->SetProperty(AUDIO_DECODER_IN_AUDIO_CHANNEL_LAYOUT, layout);
        AMF_RETURN_IF_FAILED(result, L"Invalid decoder input channel layout %d, result = %s", layout, amf::AMFGetResultText(result));

        result = decoder->SetProperty(AUDIO_DECODER_IN_AUDIO_SAMPLE_FORMAT, format);
        AMF_RETURN_IF_FAILED(result, L"Invalid decoder input format %d, result = %s", format, amf::AMFGetResultText(result));
        result = decoder->SetProperty(AUDIO_DECODER_OUT_AUDIO_SAMPLE_FORMAT, format);
        AMF_RETURN_IF_FAILED(result, L"Invalid decoder output format %d, result = %s", format, amf::AMFGetResultText(result));

        result = decoder->Init(amf::AMF_SURFACE_UNKNOWN, 0, 0);
        AMF_RETURN_IF_FAILED(result, L"Audio decoder Init() failed, result = %s", amf::AMFGetResultText(result));

        m_Decoder = decoder;

        return result;
    }

    void AMFAudioDecoder::Terminate()
    {
        amf::AMFLock lockIn(&m_InputGuard);
        amf::AMFLock lockOut(&m_OutputGuard);

        if (m_Decoder != nullptr)
        {
            m_Decoder->Terminate();
            m_Decoder = nullptr;
        }
    }

    AMF_RESULT AMFAudioDecoder::SubmitInput(amf::AMFBuffer* input)
    {
        amf::AMFLock lock(&m_InputGuard);
        AMF_RETURN_IF_FALSE(m_Decoder != nullptr, AMF_NOT_INITIALIZED, L"SubmitInput() failed, decoder not initialized");

        return m_Decoder->SubmitInput(input);
    }

    AMF_RESULT AMFAudioDecoder::QueryOutput(amf::AMFAudioBuffer** outputBuffer)
    {
        amf::AMFLock lock(&m_OutputGuard);
        AMF_RETURN_IF_FALSE(m_Decoder != nullptr, AMF_NOT_INITIALIZED, L"QueryOutput() failed, decoder not initialized");
        amf::AMFDataPtr outData;
        AMF_RESULT result = m_Decoder->QueryOutput(&outData);
        if (result == AMF_OK && outData != nullptr)
        {
            *outputBuffer = amf::AMFAudioBufferPtr(outData);
            (*outputBuffer)->Acquire();
        }
        return result;
    }

    AMF_RESULT AMFAudioDecoder::Flush()
    {
        amf::AMFLock lockIn(&m_InputGuard);
        amf::AMFLock lockOut(&m_OutputGuard);
        AMF_RETURN_IF_FALSE(m_Decoder != nullptr, AMF_NOT_INITIALIZED, L"Flush() failed, decoder not initialized");
        return m_Decoder->Flush();
    }

    AMF_RESULT AMFAudioDecoder::Drain()
    {
        amf::AMFLock lockIn(&m_InputGuard);
        amf::AMFLock lockOut(&m_OutputGuard);
        AMF_RETURN_IF_FALSE(m_Decoder != nullptr, AMF_NOT_INITIALIZED, L"Drain() failed, decoder not initialized");
        return m_Decoder->Drain();
    }
}
