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

#include "AudioOutput.h"
#include "amf/public/include/components/FFMPEGComponents.h"
#include "amf/public/include/components/FFMPEGAudioConverter.h"
#include "amf/public/common/TraceAdapter.h"

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::audio::AudioOutput";

#if defined(_WIN32)
    #define FFMPEG_HELPER_DLL_NAME    FFMPEG_DLL_NAME
#elif defined(__linux)
    #define FFMPEG_HELPER_DLL_NAME    L"libamf-component-ffmpeg64.so"
#endif

namespace ssdk::audio
{
    AudioOutput::AudioOutput(TransmitterAdapter& transportAdapter, AudioEncodeEngine::Ptr encoder, amf::AMFContext* context) :
        m_TransportAdapter(transportAdapter),
        m_Context(context),
        m_Encoder(std::move(encoder))
    {
        m_Pump = Pump::Ptr(new Pump(*this, m_InputQueue));
    }

    AudioOutput::~AudioOutput()
    {
        Terminate();
        m_Encoder = nullptr;
        m_Context = nullptr;
        m_Pump = nullptr;
    }

    AMF_RESULT AudioOutput::Init(amf::AMF_AUDIO_FORMAT inputFormat, int32_t inputSamplingRate, int32_t inputChannels, int32_t inputLayout,
                                 int32_t outputSamplingRate, int32_t outputChannels, int32_t outputLayout, int32_t bitrate)
    {
        AMF_RESULT result = AMF_OK;
        Terminate();

        amf::AMF_AUDIO_FORMAT actualOutputFormat = inputFormat;
        int32_t actualOutputSamplingRate = outputSamplingRate;
        int32_t actualOutputChannels = outputChannels;
        int32_t actualOutputLayout = outputLayout;

        if (result == AMF_OK && m_Encoder != nullptr)
        {
            result = InitializeEncoder(outputSamplingRate, outputChannels, outputLayout, bitrate);
            if (result == AMF_OK)
            {
                actualOutputFormat = m_Encoder->GetFormat();
                actualOutputSamplingRate = m_Encoder->GetSamplingRate();
                actualOutputChannels = m_Encoder->GetChannels();
                actualOutputLayout = m_Encoder->GetLayout();
            }
        }
        if (inputSamplingRate != outputSamplingRate || inputChannels != outputChannels || inputLayout != outputLayout || inputFormat != actualOutputFormat)
        {   //  Input parameters do not match the output parameters, need an audio converter to resample
            result = InitializeConverter(inputFormat, inputSamplingRate, inputChannels, inputLayout, actualOutputFormat, actualOutputSamplingRate, actualOutputChannels, actualOutputLayout);
        }
        if (result == AMF_OK)
        {
            {
                amf::AMFLock lock(&m_Guard);
                m_InputFormat = inputFormat;
                m_OutputFormat = actualOutputFormat;
                m_InputSamplingRate = inputSamplingRate;
                m_OutputSamplingRate = actualOutputSamplingRate;
                m_InputChannels = inputChannels;
                m_OutputChannels = actualOutputChannels;
                m_InputLayout = inputLayout;
                m_OutputLayout = actualOutputLayout;
                m_SequenceNumber = 0;
                m_ExpectedPts = 0;
                m_Initialized = true;
            }
            m_Pump->Start();
        }
        return result;
    }

    AMF_RESULT AudioOutput::InitializeEncoder(int32_t outputSamplingRate, int32_t outputChannels, int32_t outputLayout, int32_t bitrate)
    {
        AMF_RESULT result = AMF_OK;
        if (m_Encoder != nullptr)
        {
            result = m_Encoder->Init(outputSamplingRate, outputChannels, outputLayout, bitrate);
            AMF_RETURN_IF_FAILED(result, L"Failed to initialize audio encoder, result = %s", amf::AMFGetResultText(result));
            amf::AMFBufferPtr extraData;
            if (m_Encoder->GetExtraData(&extraData) == AMF_OK)
            {
                SaveExtraData(extraData);
            }
        }
        else
        {
            SaveExtraData(nullptr);
        }
        return result;
    }

    AMF_RESULT AudioOutput::InitializeConverter(amf::AMF_AUDIO_FORMAT inputFormat, int32_t inputSamplingRate, int32_t inputChannels, int32_t inputLayout, amf::AMF_AUDIO_FORMAT outputFormat, int32_t outputSamplingRate, int32_t outputChannels, int32_t outputLayout)
    {
        amf::AMFComponentPtr converter;
        AMF_RESULT result = g_AMFFactory.LoadExternalComponent(m_Context, FFMPEG_HELPER_DLL_NAME, "AMFCreateComponentInt", (void*)FFMPEG_AUDIO_CONVERTER, &converter);
        AMF_RETURN_IF_FAILED(result, L"LoadExternalComponent(%s) failed", FFMPEG_AUDIO_CONVERTER);

        //  Configure converter input
        result = converter->SetProperty(AUDIO_CONVERTER_IN_AUDIO_SAMPLE_RATE, inputSamplingRate);
        AMF_RETURN_IF_FAILED(result, L"Failed to set input sampling rate %d on audio converter, result = %s", inputSamplingRate, amf::AMFGetResultText(result));

        result = converter->SetProperty(AUDIO_CONVERTER_IN_AUDIO_SAMPLE_FORMAT, inputFormat);
        AMF_RETURN_IF_FAILED(result, L"Failed to set input format %d on audio converter, result = %s", inputFormat, amf::AMFGetResultText(result));

        result = converter->SetProperty(AUDIO_CONVERTER_IN_AUDIO_CHANNELS, inputChannels);
        AMF_RETURN_IF_FAILED(result, L"Failed to set input channels %d on audio converter, result = %s", inputChannels, amf::AMFGetResultText(result));

        result = converter->SetProperty(AUDIO_CONVERTER_IN_AUDIO_CHANNEL_LAYOUT, inputLayout);
        AMF_RETURN_IF_FAILED(result, L"Failed to set input channel layout %d on audio converter, result = %s", inputLayout, amf::AMFGetResultText(result));

        //  Configure converter output
        result = converter->SetProperty(AUDIO_CONVERTER_OUT_AUDIO_SAMPLE_RATE, outputSamplingRate);
        AMF_RETURN_IF_FAILED(result, L"Failed to set output sampling rate %d on audio converter, result = %s", outputSamplingRate, amf::AMFGetResultText(result));

        result = converter->SetProperty(AUDIO_CONVERTER_OUT_AUDIO_SAMPLE_FORMAT, outputFormat);
        AMF_RETURN_IF_FAILED(result, L"Failed to set output format %d on audio converter, result = %s", outputFormat, amf::AMFGetResultText(result));

        result = converter->SetProperty(AUDIO_CONVERTER_OUT_AUDIO_CHANNELS, outputChannels);
        AMF_RETURN_IF_FAILED(result, L"Failed to set output channels %d on audio converter, result = %s", outputChannels, amf::AMFGetResultText(result));

        result = converter->SetProperty(AUDIO_CONVERTER_OUT_AUDIO_CHANNEL_LAYOUT, outputLayout);
        AMF_RETURN_IF_FAILED(result, L"Failed to set output channel layout %d on audio converter, result = %s", outputLayout, amf::AMFGetResultText(result));

        result = converter->Init(amf::AMF_SURFACE_UNKNOWN, 0, 0);
        AMF_RETURN_IF_FAILED(result, L"Failed to initialize audio converter, result = %s", amf::AMFGetResultText(result));

        m_Converter = converter;
        return result;
    }

    AMF_RESULT AudioOutput::Terminate()
    {
        {
            amf::AMFLock lock(&m_Guard);

            m_InputFormat = amf::AMF_AUDIO_FORMAT::AMFAF_UNKNOWN;
            m_OutputFormat = amf::AMF_AUDIO_FORMAT::AMFAF_UNKNOWN;
            m_InputSamplingRate = DEFAULT_SAMPLING_RATE;
            m_OutputSamplingRate = DEFAULT_SAMPLING_RATE;
            m_InputChannels = DEFAULT_CHANNELS;
            m_OutputChannels = DEFAULT_CHANNELS;
            m_InputLayout = DEFAULT_LAYOUT;
            m_OutputLayout = DEFAULT_LAYOUT;

            m_SequenceNumber = 0;
            m_ExpectedPts = 0;
            m_Initialized = false;
        }
        if (m_Pump != nullptr)
        {
            m_Pump->RequestStop();
            m_Pump->WaitForStop();
        }
        if (m_Converter != nullptr)
        {
            m_Converter->Terminate();
            m_Converter = nullptr;
        }
        if (m_Encoder != nullptr)
        {
            m_Encoder->Terminate(); //  Encoder is passed to a constructor, so do not NULL it here
        }
        return AMF_OK;
    }

    AMF_RESULT AudioOutput::SetBitrate(int32_t bitrate)
    {
        AMF_RESULT result = AMF_OK;
        amf::AMFLock lock(&m_Guard);
        if (m_Encoder != nullptr)
        {
            result = m_Encoder->UpdateBitrate(bitrate);
        }
        return result;
    }

    AMF_RESULT AudioOutput::SubmitInput(amf::AMFAudioBuffer* input)
    {
        AMF_RESULT result = AMF_OK;
        AMF_RETURN_IF_FALSE(input != nullptr, AMF_INVALID_ARG, L"AudioOutput::SubmitInput(NULL) - input must be non-NULL");
        amf::AMFLock lock(&m_Guard);
        AMF_RETURN_IF_FALSE(m_Initialized, AMF_NOT_INITIALIZED, L"AudioOutput::SubmitInput() - not initialized");

        if (m_ExpectedPts != 0 && input->GetPts() != m_ExpectedPts)
        {   //  We've detected a break in the pts sequence, need to set a discontinuity flag on the first buffer that would come out
            //  Flush the pipeline to ensure the discontinuity flag is set on the output that matches the input where discontinuity was detected
            //  Flush is also necessary since ffmpeg converter doesn't like non-contiguous timestamps
            //  Flush might result in an audiable click, but it would likely be there anyway because of the nature of discontinuity
            m_Discontinuity = true;
            if (m_Converter != nullptr)
            {
                m_Converter->Flush();
            }
            if (m_Encoder != nullptr)
            {
                m_Encoder->Flush();
            }
        }
        static constexpr unsigned long TIMEOUT = 100;
        if (m_InputQueue.Add(0, input, 0, TIMEOUT) == false)
        {
            result = AMF_INPUT_FULL;
        }
        else
        {   //  Successfully submitted a buffer
            m_ExpectedPts = input->GetPts() + input->GetDuration();
        }
        return result;
    }

    AMF_RESULT AudioOutput::ConvertBuffer(amf::AMFAudioBuffer* inBuffer, amf::AMFAudioBuffer** outBuffer)
    {
        AMF_RESULT res = AMF_OK;

        if (m_Converter != nullptr)
        {
            bool bSubmitRepeat = true;
            do
            {
                // try to submit
                res = m_Converter->SubmitInput(inBuffer);
                bSubmitRepeat = (res == AMF_REPEAT || res == AMF_INPUT_FULL);
                // poll output
                while (true)
                {
                    amf::AMFDataPtr	pDataConverted;
                    res = m_Converter->QueryOutput(&pDataConverted);
                    if (res != AMF_OK || pDataConverted == nullptr)
                    {
                        amf_sleep(1);
                    }
                    else
                    {
                        *outBuffer = amf::AMFAudioBufferPtr(pDataConverted).Detach();
                        res = AMF_OK;
                        break;
                    }
                }
                // if repeat submission - small sleep
                if (bSubmitRepeat == true)
                {
                    amf_sleep(1);
                }
            } while (bSubmitRepeat == true);
        }
        else
        {   //  When conversion is not required, simply pass the buffer through
            *outBuffer = inBuffer;
            (*outBuffer)->Acquire();
        }
        return res;
    }

    AMF_RESULT AudioOutput::EncodeAndSendBuffer(amf::AMFAudioBuffer* inBuffer)
    {
        AMF_RESULT res = AMF_OK;
        bool submit = true;

        if (m_Encoder != nullptr)
        {   //  We're sending compressed audio data, so submit to encoder and wait for its output
            //  The number of encoded buffers might not be the same as the number of input buffers depending on the codec used
            do
            {
                if (submit == true)
                {
                    res = m_Encoder->SubmitInput(inBuffer);
                    if (res == AMF_OK || res == AMF_NEED_MORE_INPUT)
                    {
                        submit = false;
                    }
                    else if (res != AMF_INPUT_FULL)
                    {
                        submit = false;
                    }
                }
                while (true)
                {
                    amf::AMFBufferPtr pDataEncoded;
                    res = m_Encoder->QueryOutput(&pDataEncoded);
                    if (pDataEncoded != nullptr)
                    {   //  Send the encoded buffer to all sessions and continue to poll the encoder until there's no more output
                        if (SendBufferToAllClients(pDataEncoded) != transport_common::Result::OK)
                        {
                            res = AMF_FAIL;
                        }
                    }
                    else
                    {   //  No more output
                        break;
                    }
                }
            } while (submit == true);
        }
        else
        {   //  We're sending uncompressed PCM data, just send the inBuffer to all sessions
            amf::AMFBufferPtr buf(inBuffer);
            if (SendBufferToAllClients(buf) != transport_common::Result::OK)
            {
                res = AMF_FAIL;
            }
        }
        return res;
    }

    void AudioOutput::SaveExtraData(amf::AMFBuffer* buffer)
    {
        amf::AMFLock lock(&m_Guard);
        m_ExtraData = buffer;
        m_InitID = amf_high_precision_clock();
    }

    bool AudioOutput::GetExtraData(amf::AMFBuffer** buffer, amf_pts& id)
    {
        amf::AMFLock lock(&m_Guard);
        *buffer = m_ExtraData;
        (*buffer)->Acquire();
        id = m_InitID;
        bool result = m_LastRetrievedExtraDataID != m_InitID;
        m_LastRetrievedExtraDataID = m_InitID;
        return result;
    }

    transport_common::Result AudioOutput::SendBufferToAllClients(amf::AMFBuffer* inBuffer)
    {
        int64_t     sequenceNumber;
        bool        discontinuity;
        const char* codecName = m_Encoder != nullptr ? m_Encoder->GetCodecID() : CODEC_ID_PCM;
        transport_common::InitID initID;
        int32_t channels;
        int32_t layout;
        int32_t samplingRate;
        amf::AMF_AUDIO_FORMAT outputFormat;
        amf::AMFBufferPtr extraData;
        {
            amf::AMFLock lock(&m_Guard);
            sequenceNumber = m_SequenceNumber++;
            discontinuity = m_Discontinuity;
            m_Discontinuity = false;
            initID = m_InitID;
            channels = m_OutputChannels;
            layout = m_OutputLayout;
            outputFormat = m_OutputFormat;
            samplingRate = m_OutputSamplingRate;
            extraData = m_ExtraData;
        }
        if (GetExtraData(&extraData, initID) == true)
        {   //  Encoder has been reinitialized, send the new init block to all clients
            m_TransportAdapter.SendAudioInit(codecName, initID, channels, layout, outputFormat, samplingRate, extraData);
        }

        transport_common::Result result = transport_common::Result::OK;
        transport_common::TransmittableAudioBuffer transmittableBuffer(inBuffer, sequenceNumber, discontinuity);
        result = m_TransportAdapter.SendAudioBuffer(transmittableBuffer);
        return result;
    }

    AudioOutput::Pump::Pump(AudioOutput& pipeline, amf::AMFQueue<amf::AMFDataPtr>& inputQueue) :
        m_Pipeline(pipeline),
        amf::AMFQueueThread<amf::AMFDataPtr, int>(&inputQueue, nullptr)
    {
    }

    bool AudioOutput::Pump::Process(amf_ulong& /*ulID*/, amf::AMFDataPtr& inData, int&)
    {
        bool result = false;

        amf::AMFAudioBufferPtr  pAudioBuffer(inData);

        if (pAudioBuffer != nullptr)
        {
            amf::AMFAudioBufferPtr convertedBuffer;
            if (m_Pipeline.ConvertBuffer(pAudioBuffer, &convertedBuffer) == AMF_OK)
            {
                m_Pipeline.EncodeAndSendBuffer(convertedBuffer);
            }
        }
        return result;
    }

}