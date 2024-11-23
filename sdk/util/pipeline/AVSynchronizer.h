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

#include "PipelineSlot.h"
#include "amf/public/samples/CPPSamples/common/VideoPresenter.h"
#include "amf/public/samples/CPPSamples/common/AudioPresenter.h"
#include "sdk/util/stats/ClientStatsManager.h"

#include <deque>
#include <memory>

namespace ssdk::util
{
    class AVSynchronizer
    {
    public:
        typedef std::shared_ptr<AVSynchronizer>     Ptr;

        class Input : public SinkSlot
        {
        protected:
            Input(const char* name, AVSynchronizer& synchronizer);

        public:
            virtual void Start() override;
            virtual void Stop() override;

        protected:
            mutable amf::AMFCriticalSection     m_Guard;
            AVSynchronizer& m_Synchronizer;
            bool            m_Enabled = false;
        };

        class VideoInput : public Input
        {
        public:
            typedef std::shared_ptr<VideoInput>     Ptr;
        public:
            VideoInput(AVSynchronizer& synchronizer);

            virtual AMF_RESULT SubmitInput(amf::AMFData* input) override;
            virtual AMF_RESULT Flush() override;
        };

        class AudioInput : public Input
        {
        public:
            typedef std::shared_ptr<AudioInput>     Ptr;
        public:
            AudioInput(AVSynchronizer& synchronizer);

            virtual AMF_RESULT SubmitInput(amf::AMFData* input) override;
            virtual AMF_RESULT Flush() override;
        };

    public:
        AVSynchronizer(VideoPresenterPtr videoPresenter, AudioPresenterPtr audioPresenter);
        ~AVSynchronizer();

        inline void GetVideoInput(VideoInput::Ptr& input) const noexcept { input = m_VideoInput; }
        inline void GetAudioInput(AudioInput::Ptr& input) const noexcept { input = m_AudioInput; }

        void SetStatsManager(ClientStatsManager::Ptr statsManager);

    private:
        AMF_RESULT SubmitVideoInput(amf::AMFData* input);
        AMF_RESULT SubmitAudioInput(amf::AMFData* input);

        AMF_RESULT FlushVideo();
        AMF_RESULT FlushAudio();

    protected:
        mutable amf::AMFCriticalSection     m_Guard;
        VideoInput::Ptr     m_VideoInput;
        VideoPresenterPtr   m_VideoPresenter;
        AudioInput::Ptr     m_AudioInput;
        AudioPresenterPtr   m_AudioPresenter;
        ClientStatsManager::Ptr    m_StatsManager;

        amf_pts             m_LastVideoPts = -1LL;
        int                 m_SequentiallyDroppedAudioSamplesCnt = 0;
        int                 m_DropCyclesCnt = 0;
        bool                m_ResetAVSync = false;
        amf_pts             m_DesyncToIgnore = 0;
        std::deque<amf_pts> m_AverageAVDesyncQueue;
    };
}