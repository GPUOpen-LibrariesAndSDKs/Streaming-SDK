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

#include "AudioDispatcher.h"

#include "amf/public/common/TraceAdapter.h"

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::audio::AudioDispatcher";

namespace ssdk::audio
{
    AudioDispatcher::AudioDispatcher(amf::AMFContext* context, ssdk::audio::AudioReceiverPipeline::Ptr pipeline) :
        AVDispatcher(context),
        m_Pipeline(pipeline)
    {
    }

    bool AudioDispatcher::OnAudioInit(const char* codec, ssdk::transport_common::StreamID streamID, int64_t initID, uint32_t channels, uint32_t layout, uint32_t samplingRate, amf::AMF_AUDIO_FORMAT format, const void* initBlock, size_t initBlockSize)
    {
        bool result = true;
        if (m_Context == nullptr || m_Pipeline == nullptr)
        {
            AMFTraceError(AMF_FACILITY, L"AMFContext and pipeline should not be NULL");
            result = false;
        }
        else
        {
            amf::AMFLock    lock(&m_Guard);

            StreamPipelineMap::iterator it = m_StreamHandlers.find(streamID);
            if (it == m_StreamHandlers.end())
            {
                AMFTraceWarning(AMF_FACILITY, L"Invalid stream ID %llu, init block discarded", streamID);
                result = false;
            }
            else
            {
                ssdk::audio::AudioReceiverPipeline::Ptr pipeline = std::static_pointer_cast<ssdk::audio::AudioReceiverPipeline>(it->second);
                AMF_RESULT amfResult = AMF_OK;
                amf::AMFBufferPtr initBlockBuf;
                if ((amfResult = AllocAMFBufferForBlock(initBlock, initBlockSize, &initBlockBuf)) != AMF_OK)
                {
                    AMFTraceError(AMF_FACILITY, L"Failed to allocate a buffer for video init (ExtraData) block, size=%llu, result=%s", uint64_t(initBlockSize), amf::AMFGetResultText(amfResult));
                    result = false;
                }
                else
                {
                    result = (pipeline->OnInputChanged(codec, initID, channels, layout, samplingRate, format, initBlockBuf) == AMF_OK);
                }
            }
        }
        return result;
    }

    void AudioDispatcher::OnAudioBuffer(ssdk::transport_common::StreamID streamID, const ssdk::transport_common::ReceivableAudioBuffer& buffer)
    {
        StreamPipelineMap::iterator it = m_StreamHandlers.find(streamID);
        if (it == m_StreamHandlers.end())
        {
            AMFTraceWarning(AMF_FACILITY, L"Invalid stream ID %llu, frame discarded", streamID);
        }
        else
        {
            ssdk::audio::AudioReceiverPipeline::Ptr pipeline = std::static_pointer_cast<ssdk::audio::AudioReceiverPipeline>(it->second);
            amf::AMFBufferPtr   inputBuf;
            const_cast<ssdk::transport_common::ReceivableAudioBuffer&>(buffer).GetBuffer(&inputBuf);
            if (inputBuf != nullptr)
            {
                amf::AMFLock    lock(&m_Guard);
                pipeline->SubmitInput(inputBuf, buffer.IsDiscontinuity());
            }
            else
            {
                AMFTraceWarning(AMF_FACILITY, L"AudioDispatcher::OnAudioBuffer(): NULL-buffer received, ignored");
            }
        }
    }

}