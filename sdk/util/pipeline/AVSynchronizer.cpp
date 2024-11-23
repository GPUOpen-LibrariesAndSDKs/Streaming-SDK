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

#include "AVSynchronizer.h"
#include "sdk/video/Defines.h"

#include "amf/public/common/TraceAdapter.h"

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::util::AVSynchronizer";

namespace ssdk::util
{
    AVSynchronizer::Input::Input(const char* name, AVSynchronizer& synchronizer) :
        SinkSlot(name),
        m_Synchronizer(synchronizer)
    {
    }

    void AVSynchronizer::Input::Start()
    {
        amf::AMFLock lock(&m_Guard);
        m_Enabled = true;
    }

    void AVSynchronizer::Input::Stop()
    {
        amf::AMFLock lock(&m_Guard);
        m_Enabled = false;
    }

    /// <summary>
    /// Video input
    /// </summary>
    /// <param name="name"></param>
    /// <param name="synchronizer"></param>
    AVSynchronizer::VideoInput::VideoInput(AVSynchronizer& synchronizer) :
        Input("AVSynchronizer::Video", synchronizer)
    {
    }

    AMF_RESULT AVSynchronizer::VideoInput::SubmitInput(amf::AMFData* input)
    {
        amf::AMFLock lock(&m_Guard);
        return m_Enabled == true ? m_Synchronizer.SubmitVideoInput(input) : AMF_NOT_INITIALIZED;
    }

    AMF_RESULT AVSynchronizer::VideoInput::Flush()
    {
        return m_Synchronizer.FlushVideo();
    }

    /// <summary>
    /// Audio Input
    /// </summary>
    /// <param name="name"></param>
    AVSynchronizer::AudioInput::AudioInput(AVSynchronizer& synchronizer) :
        Input("AVSynchronizer::Audio", synchronizer)
    {
    }

    AMF_RESULT AVSynchronizer::AudioInput::SubmitInput(amf::AMFData* input)
    {
        amf::AMFLock lock(&m_Guard);
        return m_Enabled == true ? m_Synchronizer.SubmitAudioInput(input) : AMF_NOT_INITIALIZED;
    }

    AMF_RESULT AVSynchronizer::AudioInput::Flush()
    {
        return m_Synchronizer.FlushAudio();
    }

    /// <summary>
    /// AVSynchronizer
    /// </summary>
    /// <param name="videoPresenter"></param>
    /// <param name="audioPresenter"></param>
    AVSynchronizer::AVSynchronizer(VideoPresenterPtr videoPresenter, AudioPresenterPtr audioPresenter) :
        m_VideoPresenter(videoPresenter),
        m_AudioPresenter(audioPresenter)
    {
        m_VideoInput = VideoInput::Ptr(new VideoInput(*this));
        m_AudioInput = AudioInput::Ptr(new AudioInput(*this));
    }

    AVSynchronizer::~AVSynchronizer()
    {
        m_VideoInput->Stop();
        m_AudioInput->Stop();
    }

    void AVSynchronizer::SetStatsManager(ssdk::util::ClientStatsManager::Ptr statsManager)
    {
        amf::AMFLock    lock(&m_Guard);
        m_StatsManager = statsManager;
    }

    AMF_RESULT AVSynchronizer::SubmitVideoInput(amf::AMFData* input)
    {
        AMF_RETURN_IF_FALSE(input != nullptr, AMF_INVALID_ARG, L"AVSynchronizer::SubmitVideoInput received NULL input");
        amf::AMFSurfacePtr surface(input);
        AMF_RETURN_IF_FALSE(surface != nullptr, AMF_INVALID_ARG, L"AVSynchronizer::SubmitVideoInput received input that is not an AMFSurface");
        ssdk::util::ClientStatsManager::Ptr statsManager;
        {
            amf::AMFLock    lock(&m_Guard);
            
            statsManager = m_StatsManager;
        }
        m_LastVideoPts = surface->GetPts();

        AMF_RESULT result = m_VideoPresenter != nullptr ? m_VideoPresenter->SubmitInput(surface) : AMF_OK;

        ssdk::transport_common::StreamID streamID = -1;
        surface->GetProperty(ssdk::video::STREAM_ID_PROPERTY, &streamID);
        if (streamID == -1)
        {
            AMFTraceError(AMF_FACILITY, L"SubmitVideoInput(): failed to read Stream ID property from frame");
        }

        amf_pts now = amf_high_precision_clock();
        amf_pts fullLatency = 0;
        amf_pts clientLatency = 0;

        amf_pts originPts = -1;
        surface->GetProperty(ssdk::video::ORIGIN_PTS_PROPERTY, &originPts);
        if (originPts == -1)
        {
            AMFTraceError(AMF_FACILITY, L"SubmitVideoInput(): failed to read Origin Time property from frame");
        }
        else
        {
            fullLatency = now - originPts;
        }

        amf_pts clienPts = -1;
        surface->GetProperty(ssdk::video::VIDEO_CLIENT_LATENCY_PTS, &clienPts);
        if (clienPts == -1)
        {
            AMFTraceError(AMF_FACILITY, L"SubmitVideoInput(): failed to read client ASDF property from frame");
        }
        else
        {
            clientLatency = now - clienPts;
        }

        if (statsManager != nullptr)
        {
            statsManager->UpdateClientLatency(clientLatency);
            statsManager->UpdateFullLatencyAndFramerate(fullLatency);
            statsManager->SendStatistics();
        }

        return result;
    }

    AMF_RESULT AVSynchronizer::FlushVideo()
    {
        return m_VideoPresenter != nullptr ? m_VideoPresenter->Flush() : AMF_OK;
    }

    AMF_RESULT AVSynchronizer::SubmitAudioInput(amf::AMFData* input)
    {
        AMF_RESULT result = AMF_OK;
        AMF_RETURN_IF_FALSE(input != nullptr, AMF_INVALID_ARG, L"AVSynchronizer::SubmitVideoInput received NULL input");
        amf::AMFAudioBufferPtr audioBuffer(input);
        AMF_RETURN_IF_FALSE(audioBuffer != nullptr, AMF_INVALID_ARG, L"AVSynchronizer::SubmitAudioInput received input that is not an AMFAudioBuffer");
        if (m_AudioPresenter != nullptr)
        {
            //  Compensation for audio samples arriving later than real time:
            //  An audio sample can be delayed on the network and arrive late. Since we generally try to
            //  buffer as little as possible not to increase latency and minimize desync with video,
            //  this will of course cause audio stutter. However, when the late sample arrives, if we still
            //  play it right after the gap, it would delay the whole audio stream even further. Since we
            //  have already stuttered, skipping the late sample won't make it sound worse, would just make the
            //  gap a little longer, but would allow the rest of the stream to catch up with real time.
            //  We allow for no more than 80ms of delay and no more than 10 consecutive samples to be skipped.
            bool playAudio = true;
            {
                static constexpr const amf_pts MAX_AUDIO_LATE_BY = 80 * AMF_MILLISECOND;
                static constexpr const int MAX_SEQ_DROPPED_AUDIO_PACKETS = 50;
                static constexpr const size_t AVERAGING_SAMPLE_SIZE = 100;

                amf::AMFLock    lock(&m_Guard);

                if (m_ResetAVSync == true)
                {
                    m_SequentiallyDroppedAudioSamplesCnt = 0;
                    m_DesyncToIgnore = 0;
                    m_ResetAVSync = false;
                }

                if (m_LastVideoPts != -1LL)
                {
                    //  Here we compensate for audio lagging behind video by more than a certain threshold
                    m_AverageAVDesyncQueue.push_back(m_LastVideoPts - audioBuffer->GetPts());
                    if (m_AverageAVDesyncQueue.size() > AVERAGING_SAMPLE_SIZE)
                    {
                        amf_pts averageAVDesync = 0;
                        m_AverageAVDesyncQueue.pop_front();
                        for (auto ptsDiff : m_AverageAVDesyncQueue)
                        {
                            averageAVDesync += ptsDiff;
                        }
                        // we need to cast to signed, because otherwise the division gets computed unsigned
                        averageAVDesync /= static_cast<amf_pts>(m_AverageAVDesyncQueue.size());
                        m_StatsManager->UpdateAudioStatistics(averageAVDesync); // lock aquired above
                        if (averageAVDesync > MAX_AUDIO_LATE_BY + m_DesyncToIgnore)
                        {
                            if (++m_SequentiallyDroppedAudioSamplesCnt < MAX_SEQ_DROPPED_AUDIO_PACKETS)
                            {
                                playAudio = false;
                                AMFTraceWarning(AMF_FACILITY, L"Dropping unsynced audio, desync=%5.2fms", float(averageAVDesync) / AMF_MILLISECOND);
                            }
                            else
                            {
                                AMFTraceWarning(AMF_FACILITY, L"Failed to resync audio and video, accepting desync of %5.2fms", float(averageAVDesync) / AMF_MILLISECOND);
                                m_DesyncToIgnore += averageAVDesync;
                                m_SequentiallyDroppedAudioSamplesCnt = 0;
                            }
                        }
                        else if (averageAVDesync < MAX_AUDIO_LATE_BY)
                        {
                            m_SequentiallyDroppedAudioSamplesCnt = 0;
                            m_DesyncToIgnore = 0;
                        }
                    }
                }
            }
            if (playAudio == true)
            {
                result = m_AudioPresenter->SubmitInput(audioBuffer);
            }
        }
        return result;
    }

    AMF_RESULT AVSynchronizer::FlushAudio()
    {
        return m_AudioPresenter != nullptr ? m_AudioPresenter->Flush() : AMF_OK;
    }

}
