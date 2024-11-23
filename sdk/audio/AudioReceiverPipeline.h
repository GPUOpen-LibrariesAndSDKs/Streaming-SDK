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

#pragma once

#include "audio/AudioInput.h"
#include "util/pipeline/AVPipeline.h"
#include "transports/transport-common/Transport.h"
#include "util/pipeline/PipelineSlot.h"
#include "util/pipeline/AVSynchronizer.h"

#include "amf/public/samples/CPPSamples/common/AudioPresenter.h"

#include <memory>

namespace ssdk::audio
{
    class AudioReceiverPipeline :
        public ssdk::util::AVPipeline,
        protected ssdk::audio::AudioInput::Consumer
    {
    public:
        typedef std::shared_ptr<AudioReceiverPipeline>  Ptr;

    public:
        AudioReceiverPipeline(amf::AMFContext* context, AudioPresenterPtr presenter, ssdk::util::AVSynchronizer::Ptr avSynchronizer);
        ~AudioReceiverPipeline();

        AMF_RESULT Init();
        void Terminate();
        virtual AMF_RESULT OnInputChanged(const char* codec, ssdk::transport_common::InitID initID, uint32_t channels, uint32_t layout, uint32_t samplingRate, amf::AMF_AUDIO_FORMAT format, amf::AMFBuffer* initBlock);

        virtual AMF_RESULT SubmitInput(amf::AMFBuffer* buffer, bool discontinuity);

    protected:
        virtual void OnAudioBuffer(amf::AMFAudioBuffer* buffer) override;

        AMF_RESULT InitConverter();

    private:
        mutable amf::AMFCriticalSection         m_Guard;

        ssdk::audio::AudioInput::Ptr            m_Input;
        amf::AMFComponentPtr                    m_AudioConverter;

        AudioPresenterPtr                       m_Presenter;
        ssdk::util::AVSynchronizer::Ptr         m_AVSynchronizer;

        ssdk::util::PipelineSlot::Ptr           m_PipelineHead;

    };
}