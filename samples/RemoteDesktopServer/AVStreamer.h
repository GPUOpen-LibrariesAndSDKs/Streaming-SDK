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

#include "sdk/video/MonoscopicVideoOutput.h"
#include "sdk/audio/AudioOutput.h"
#include "sdk/transports/transport-common/ServerTransport.h"
#include "sdk/util/pipeline/TimestampCalibrator.h"
#include "sdk/util/QoS/QoS.h"


#include "amf/public/include/core/Context.h"
#include "amf/public/include/core/CurrentTime.h"
#include "amf/public/include/components/Component.h"

#include "amf/public/common/Thread.h"

#include <string>
#include <memory>
#include <set>

class AVStreamer :
    public ssdk::transport_common::ServerTransport::AudioSenderCallback,
    public ssdk::transport_common::ServerTransport::VideoSenderCallback,
    public ssdk::transport_common::ServerTransport::VideoStatsCallback,
    public ssdk::util::QoS::QoSCallback

{
public:
    typedef std::shared_ptr<AVStreamer>     Ptr;

public:
    AVStreamer();
    ~AVStreamer();

    bool Init(amf::AMFComponent* videoCapture, ssdk::video::MonoscopicVideoOutput::Ptr videoOutput, ssdk::video::TransmitterAdapter::Ptr videoTransmitterAdapter,
              amf::AMFComponent* audioCapture, ssdk::audio::AudioOutput::Ptr audioOutput, ssdk::audio::TransmitterAdapter::Ptr audioTransmitterAdapter, ssdk::util::QoS::Ptr QoS);

    void Terminate();

    void OnSessionDisconnected(ssdk::transport_common::SessionHandle session);

    //  ssdk::transport_common::ServerTransport::VideoSenderCallback methods:
    virtual void OnVideoStreamSubscribed(ssdk::transport_common::SessionHandle session, ssdk::transport_common::StreamID streamID) override;         // Called when a client subscribes to a specific video stream
    virtual void OnVideoStreamUnsubscribed(ssdk::transport_common::SessionHandle session, ssdk::transport_common::StreamID streamID) override;       // Called when a client unsibscribed from a specific video stream
    virtual void OnReadyToReceiveVideo(ssdk::transport_common::SessionHandle session, ssdk::transport_common::StreamID streamID, ssdk::transport_common::InitID initID) override;                                  // Called when a client is ready to accept video frames after video codec reinitialization.

    virtual void OnForceUpdateRequest(ssdk::transport_common::StreamID streamID) override;               // Called when a key/IDR frame must be submitted. This can be requested by the client or decided by the server
    virtual void OnVideoRequestInit(ssdk::transport_common::SessionHandle session, ssdk::transport_common::StreamID streamID) override;              // Called when the video init block needs to be sent/resent

    virtual void OnBitrateChangeRecieverRequest(ssdk::transport_common::SessionHandle session, ssdk::transport_common::StreamID streamID, int64_t bitrate) override;                  // Called when a video receiver requested bitrate change
    virtual void OnFramerateChangeRecieverRequest(ssdk::transport_common::SessionHandle session, ssdk::transport_common::StreamID streamID, float framerate) override;                // Called when a video receiver requested framerate change
    virtual void OnResolutionChangeRecieverRequest(ssdk::transport_common::SessionHandle session, ssdk::transport_common::StreamID streamID, const AMFSize& resolution) override;     // Called when a video receiver requested resolution change

    //  ssdk::transport_common::ServerTransport::VideoStatsCallback methods
    virtual void OnVideoStats(ssdk::transport_common::SessionHandle session, ssdk::transport_common::StreamID streamID,
                              const ssdk::transport_common::ServerTransport::VideoStatsCallback::Stats& stats) override;                                // Called when statistics is updated for a specific client/stream
    virtual void OnOriginPts(ssdk::transport_common::SessionHandle session, ssdk::transport_common::StreamID streamID, amf_pts originPts) override;     // Called when origin timestamp is updated for a specific client/stream

    //  ssdk::util::QoS::QoSCallback methods:
    virtual void OnQoSEvent(ssdk::transport_common::StreamID streamID, ssdk::util::QoS::QoSEvent event, const amf::AMFVariantStruct* value) override;

    //  ssdk::transport_common::ServerTransport::AudioSenderCallback methods:
    virtual void OnAudioStreamSubscribed(ssdk::transport_common::SessionHandle session, ssdk::transport_common::StreamID streamID) override;                          // Called when a client subscribes to a specific video stream
    virtual void OnAudioStreamUnsubscribed(ssdk::transport_common::SessionHandle session, ssdk::transport_common::StreamID streamID) override;                        // Called when a client unsibscribed from a specific video stream
    virtual void OnReadyToReceiveAudio(ssdk::transport_common::SessionHandle session, ssdk::transport_common::StreamID streamID, ssdk::transport_common::InitID initID) override;                                                   // Called when an ACK or a NACK is received from the client. The id parameter contains the same value as initID passed to SendAudioInit

    virtual void OnAudioRequestInit(ssdk::transport_common::SessionHandle session, ssdk::transport_common::StreamID streamID) override;                               // Called when the audio init block needs to be sent/resent

private:
    void CaptureVideo();
    void CaptureAudio();

    void StopVideoCaptureIfNecessary();
    void StopAudioCaptureIfNecessary();

    void OnBitrateChangeByQoS(int64_t bitrate, ssdk::transport_common::StreamID streamID);                                                 // Called when QoS requested bitrate change
    void OnFramerateChangeByQoS(float framerate, ssdk::transport_common::StreamID streamID);                                               // Called when QoS requested framerate change


    class VideoCaptureThread : public amf::AMFThread
    {
    public:
        VideoCaptureThread(AVStreamer& streamer);

        virtual void Run() override;

    private:
        AVStreamer& m_Streamer;
    };

    class AudioCaptureThread : public amf::AMFThread
    {
    public:
        AudioCaptureThread(AVStreamer& streamer);

        virtual void Run() override;

    private:
        AVStreamer& m_Streamer;
    };


private:
    mutable amf::AMFCriticalSection                     m_Guard;
    amf::AMFCurrentTimePtr                              m_CurrentTime;

    amf::AMFComponentPtr                                m_VideoCapture;
    ssdk::video::MonoscopicVideoOutput::Ptr             m_VideoOutput;
    ssdk::video::TransmitterAdapter::Ptr                m_VideoTransmitterAdapter;

    amf::AMFComponentPtr                                m_AudioCapture;
    ssdk::audio::AudioOutput::Ptr                       m_AudioOutput;
    ssdk::audio::TransmitterAdapter::Ptr                m_AudioTransmitterAdapter;
    ssdk::util::QoS::Ptr                                m_QoS;

    ssdk::util::TimestampCalibrator                     m_TimestampCalibrator;

    int64_t                                             m_LastOriginPts = 0;
    int64_t                                             m_TimeOfLastOriginPts = 0;

    typedef std::set<ssdk::transport_common::SessionHandle>  SessionCollection;
    SessionCollection                                   m_SessionsVideo;
    SessionCollection                                   m_SessionsAudio;

    VideoCaptureThread                                  m_VideoCaptureThread;
    AudioCaptureThread                                  m_AudioCaptureThread;
};
