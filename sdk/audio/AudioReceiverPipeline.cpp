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

#include "AudioReceiverPipeline.h"

#include "util/pipeline/SynchronousSlot.h"
#include "util/pipeline/AsynchronousSlot.h"

#include "amf/public/include/components/FFMPEGComponents.h"
#include "amf/public/include/components/FFMPEGAudioConverter.h"
#include "amf/public/common/TraceAdapter.h"

static constexpr const wchar_t* const AMF_FACILITY = L"AudioPipeline";

namespace ssdk::audio
{
    AudioReceiverPipeline::AudioReceiverPipeline(amf::AMFContext* context, AudioPresenterPtr presenter, ssdk::util::AVSynchronizer::Ptr avSynchronizer) :
        ssdk::util::AVPipeline(context),
        m_Presenter(presenter),
        m_AVSynchronizer(avSynchronizer)
    {
    }

    AudioReceiverPipeline::~AudioReceiverPipeline()
    {
    }

    AMF_RESULT AudioReceiverPipeline::OnInputChanged(const char* codec, ssdk::transport_common::InitID initID, uint32_t channels, uint32_t layout, uint32_t samplingRate, amf::AMF_AUDIO_FORMAT format, amf::AMFBuffer* initBlock)
    {
        AMF_RESULT result = AMF_OK;
        amf::AMFLock    lock(&m_Guard);
        if (m_InitID != initID)
        {
            if ((result = m_Input->Init(codec, format, channels, layout, samplingRate, initBlock)) != AMF_OK)
            {
                AMFTraceError(AMF_FACILITY, L"Failed to initialize audio decoder, codec: %S, format: %d, channels: %u, layout: %u, sampling rate: %u", codec, format, channels, layout, samplingRate);
            }
            else
            {
                if (m_AudioConverter != nullptr)
                {
                    m_AudioConverter->Terminate();
                }
                if (InitConverter() == AMF_OK)
                {
                    ssdk::util::AVSynchronizer::AudioInput::Ptr audioSink;
                    m_AVSynchronizer->GetAudioInput(audioSink); //  AV Syncronizer's video input terminates the pipeline and passes the frame to the presenter, for video it's just a passthrough
                    ssdk::util::PipelineSlot::Ptr converterSlot = ssdk::util::PipelineSlot::Ptr(new ssdk::util::SynchronousSlot("AudioConverter", m_AudioConverter, audioSink));
                    m_PipelineHead = converterSlot; //  We always have at least a VideoConverter component, continue to

                    m_InitID = initID;
                    m_PipelineHead->Start();
                    AMFTraceInfo(AMF_FACILITY, L"Successfully initialized audio decoder for codec: %S, format: %d, channels: %u, layout: %u, sampling rate: %u", codec, format, channels, layout, samplingRate);
                }
                else
                {
                    m_PipelineHead = nullptr;
                }
            }
        }
        return result;
    }

    AMF_RESULT AudioReceiverPipeline::Init()
    {
        amf::AMFLock    lock(&m_Guard);
        m_Input = ssdk::audio::AudioInput::Ptr(new ssdk::audio::AudioInput(m_Context, *this));
        return AMF_OK;
    }

    AMF_RESULT AudioReceiverPipeline::InitConverter()
    {
        amf::AMFComponentPtr converter;
        AMF_RESULT result = g_AMFFactory.LoadExternalComponent(m_Context, FFMPEG_DLL_NAME, "AMFCreateComponentInt", (void*)FFMPEG_AUDIO_CONVERTER, &converter);
        AMF_RETURN_IF_FAILED(result, L"LoadExternalComponent(%s) failed", FFMPEG_AUDIO_CONVERTER);

        //  Configure converter input
        uint32_t inputSamplingRate = m_Input->GetSamplingRate();
        result = converter->SetProperty(AUDIO_CONVERTER_IN_AUDIO_SAMPLE_RATE, inputSamplingRate);
        AMF_RETURN_IF_FAILED(result, L"Failed to set input sampling rate %d on audio converter, result = %s", inputSamplingRate, amf::AMFGetResultText(result));

        amf::AMF_AUDIO_FORMAT inputFormat = m_Input->GetOutputFormat();
        result = converter->SetProperty(AUDIO_CONVERTER_IN_AUDIO_SAMPLE_FORMAT, inputFormat);
        AMF_RETURN_IF_FAILED(result, L"Failed to set input format %d on audio converter, result = %s", inputFormat, amf::AMFGetResultText(result));

        uint32_t inputChannels = m_Input->GetChannels();
        result = converter->SetProperty(AUDIO_CONVERTER_IN_AUDIO_CHANNELS, inputChannels);
        AMF_RETURN_IF_FAILED(result, L"Failed to set input channels %d on audio converter, result = %s", inputChannels, amf::AMFGetResultText(result));

        uint32_t inputLayout = m_Input->GetLayout();
        result = converter->SetProperty(AUDIO_CONVERTER_IN_AUDIO_CHANNEL_LAYOUT, inputLayout);
        AMF_RETURN_IF_FAILED(result, L"Failed to set input channel layout %d on audio converter, result = %s", inputLayout, amf::AMFGetResultText(result));

        //  Configure converter output
        int64_t streamBitRate;
        int64_t outputSamplingRate;
        int64_t outputChannels;
        int64_t outputFormat;
        int64_t outputLayout;
        int64_t outputBlockAlign;

        result = m_Presenter->GetDescription(streamBitRate, outputSamplingRate, outputChannels, outputFormat, outputLayout, outputBlockAlign);
        AMF_RETURN_IF_FAILED(result, L"Failed get parameters of the audio presenter, result = %s", amf::AMFGetResultText(result));

        result = converter->SetProperty(AUDIO_CONVERTER_OUT_AUDIO_SAMPLE_RATE, outputSamplingRate);
        AMF_RETURN_IF_FAILED(result, L"Failed to set output sampling rate %lld on audio converter, result = %s", outputSamplingRate, amf::AMFGetResultText(result));

        result = converter->SetProperty(AUDIO_CONVERTER_OUT_AUDIO_SAMPLE_FORMAT, outputFormat);
        AMF_RETURN_IF_FAILED(result, L"Failed to set output format %lld on audio converter, result = %s", outputFormat, amf::AMFGetResultText(result));

        result = converter->SetProperty(AUDIO_CONVERTER_OUT_AUDIO_CHANNELS, outputChannels);
        AMF_RETURN_IF_FAILED(result, L"Failed to set output channels %lld on audio converter, result = %s", outputChannels, amf::AMFGetResultText(result));

        result = converter->SetProperty(AUDIO_CONVERTER_OUT_AUDIO_CHANNEL_LAYOUT, outputLayout);
        AMF_RETURN_IF_FAILED(result, L"Failed to set output channel layout %lld on audio converter, result = %s", outputLayout, amf::AMFGetResultText(result));

        result = converter->SetProperty(AUDIO_CONVERTER_OUT_AUDIO_BLOCK_ALIGN, outputBlockAlign);
        AMF_RETURN_IF_FAILED(result, L"Failed to set output block alignment %lld on audio converter, result = %s", outputBlockAlign, amf::AMFGetResultText(result));

        result = converter->Init(amf::AMF_SURFACE_UNKNOWN, 0, 0);
        AMF_RETURN_IF_FAILED(result, L"Failed to initialize audio converter, result = %s", amf::AMFGetResultText(result));

        m_AudioConverter = converter;
        return result;
    }

    void AudioReceiverPipeline::Terminate()
    {
        AMFTraceDebug(AMF_FACILITY, L"AudioPipeline::Terminate()");
        ssdk::audio::AudioInput::Ptr audioInput;
        amf::AMFComponentPtr        audioConverter;
        {
            amf::AMFLock    lock(&m_Guard);
            m_PipelineHead = nullptr;
            audioInput = m_Input;
            m_Input = nullptr;
            audioConverter = m_AudioConverter;
            m_AudioConverter = nullptr;
        }
        if (audioInput != nullptr)
        {
            audioInput->Terminate();
        }
        if (audioConverter != nullptr)
        {
            audioConverter->Terminate();
        }
    }

    AMF_RESULT AudioReceiverPipeline::SubmitInput(amf::AMFBuffer* buffer, bool discontinuity)
    {
        ssdk::audio::AudioInput::Ptr  input;
        {
            amf::AMFLock lock(&m_Guard);
            AMF_RETURN_IF_FALSE(m_Input != nullptr, AMF_NOT_INITIALIZED, L"Audio input is NULL");
            input = m_Input;
        }
        return input->SubmitInput(buffer, discontinuity);
    }

    void AudioReceiverPipeline::OnAudioBuffer(amf::AMFAudioBuffer* buffer)
    {
        ssdk::util::PipelineSlot::Ptr pipelineHead;
        {
            amf::AMFLock    lock(&m_Guard);
            pipelineHead = m_PipelineHead;
        }
        if (pipelineHead != nullptr)
        {
            bool discontinuity = false;
            buffer->GetProperty(ssdk::audio::AUDIO_DISCONTINUITY, &discontinuity);
            if (discontinuity == true)
            {
                pipelineHead->Flush();
            }
            AMF_RESULT result = pipelineHead->SubmitInput(buffer);
            if (result != AMF_OK)
            {
                AMFTraceError(AMF_FACILITY, L"OnAudioBuffer(): failed to submit an audio buffer to the pipeline, result=%s", amf::AMFGetResultText(result));
            }
        }
        else
        {
            AMFTraceError(AMF_FACILITY, L"OnAudioBuffer(): failed to submit an audio buffer to the pipeline - pipeline is not initialized");
        }
    }
}