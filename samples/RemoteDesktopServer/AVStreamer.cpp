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

#include "AVStreamer.h"

#include "amf/public/common/TraceAdapter.h"
#include "amf/public/include/components/DisplayCapture.h"

static constexpr const wchar_t* const AMF_FACILITY = L"AVStreamer";

AVStreamer::AVStreamer() :
    m_VideoCaptureThread(*this),
    m_AudioCaptureThread(*this)
{
}

AVStreamer::~AVStreamer()
{
    Terminate();
}

bool AVStreamer::Init(amf::AMFComponent* videoCapture, ssdk::video::MonoscopicVideoOutput::Ptr videoOutput, ssdk::video::TransmitterAdapter::Ptr videoTransmitterAdapter,
                      amf::AMFComponent* audioCapture, ssdk::audio::AudioOutput::Ptr audioOutput, ssdk::audio::TransmitterAdapter::Ptr audioTransmitterAdapter, ssdk::util::QoS::Ptr QoS)
{
    amf::AMFLock    lock(&m_Guard);

    m_VideoCapture = videoCapture;
    m_VideoOutput = videoOutput;
    m_VideoTransmitterAdapter = videoTransmitterAdapter;

    m_AudioCapture = audioCapture;
    m_AudioOutput = audioOutput;
    m_AudioTransmitterAdapter = audioTransmitterAdapter;
    m_QoS = QoS;

    return true;
}

void AVStreamer::Terminate()
{
    m_VideoCaptureThread.RequestStop();
    m_VideoCaptureThread.WaitForStop();
    m_AudioCaptureThread.RequestStop();
    m_AudioCaptureThread.WaitForStop();

    amf::AMFLock    lock(&m_Guard);
    m_AudioTransmitterAdapter = nullptr;
    m_AudioOutput = nullptr;
    m_AudioCapture = nullptr;

    m_VideoTransmitterAdapter = nullptr;
    m_VideoOutput = nullptr;
    m_VideoCapture = nullptr;
    m_QoS = nullptr;
}

void AVStreamer::OnSessionDisconnected(ssdk::transport_common::SessionHandle session)
{
    {
        amf::AMFLock    lock(&m_Guard);
        m_SessionsVideo.erase(session);
        m_VideoTransmitterAdapter->UnregisterSession(session);
        if (m_QoS != nullptr)
        {
            m_QoS->UnregisterSession(session);
        }
        m_SessionsAudio.erase(session);
        m_AudioTransmitterAdapter->UnregisterSession(session);
    }
    StopVideoCaptureIfNecessary();
    StopAudioCaptureIfNecessary();
}


void AVStreamer::StopVideoCaptureIfNecessary()
{
    bool stop = false;
    {
        amf::AMFLock    lock(&m_Guard);
        stop = m_SessionsVideo.size() == 0;
    }

    if (stop == true && m_VideoCaptureThread.IsRunning() == true)
    {
        m_VideoCaptureThread.RequestStop();
        m_VideoCaptureThread.WaitForStop();
        AMFTraceInfo(AMF_FACILITY, L"Video capture stopped");
    }
}

void AVStreamer::StopAudioCaptureIfNecessary()
{
    bool stop = false;
    {
        amf::AMFLock    lock(&m_Guard);
        stop = m_SessionsAudio.size() == 0;
    }

    if (stop == true && m_AudioCaptureThread.IsRunning() == true)
    {
        m_AudioCaptureThread.RequestStop();
        m_AudioCaptureThread.WaitForStop();
        AMFTraceInfo(AMF_FACILITY, L"Audio capture stopped");
    }
}


//  Video callbacks
void AVStreamer::OnVideoStreamSubscribed(ssdk::transport_common::SessionHandle session, ssdk::transport_common::StreamID streamID)
{
    amf::AMFLock    lock(&m_Guard);
    if (streamID != ssdk::transport_common::DEFAULT_STREAM)
    {
        AMFTraceWarning(AMF_FACILITY, L"Client session %lld is trying to subscribe to an invalid video stream %lld, request ignored", session, streamID);
    }
    else if (m_VideoTransmitterAdapter == nullptr)
    {
        AMFTraceError(AMF_FACILITY, L"AVStreamer not initialized");
    }
    else
    {
        m_SessionsVideo.emplace(session);
        bool result = m_VideoTransmitterAdapter->RegisterSession(session);
        if (result == true)
        {
            if (m_VideoCaptureThread.IsRunning() == false)
            {
                m_VideoCaptureThread.Start();
                AMFTraceInfo(AMF_FACILITY, L"Video capture started");
            }
            m_VideoTransmitterAdapter->SendInitToSession(session);
            AMFTraceInfo(AMF_FACILITY, L"Successfully registered session %lld for video stream %lld", session, streamID);
        }
        else
        {
            AMFTraceWarning(AMF_FACILITY, L"Session %lld is already registered for video stream %lld", session, streamID);
        }
    }
}

void AVStreamer::OnVideoStreamUnsubscribed(ssdk::transport_common::SessionHandle session, ssdk::transport_common::StreamID streamID)        // Called when a client unsibscribed from a specific video stream
{
    {
        amf::AMFLock    lock(&m_Guard);
        if (streamID != ssdk::transport_common::DEFAULT_STREAM)
        {
            AMFTraceWarning(AMF_FACILITY, L"Client session %lld is trying to unsubscribe from an invalid video stream %lld, request ignored", session, streamID);
        }
        else if (m_VideoTransmitterAdapter == nullptr)
        {
            AMFTraceError(AMF_FACILITY, L"AVStreamer not initialized");
        }
        else
        {
            m_VideoTransmitterAdapter->UnregisterSession(session);
            m_SessionsVideo.erase(session);
            if (m_QoS != nullptr)
            {
                m_QoS->UnregisterSession(session);
            }
            AMFTraceInfo(AMF_FACILITY, L"Successfully unregistered session %lld for video stream %lld", session, streamID);
        }
    }
    StopVideoCaptureIfNecessary();
}

void AVStreamer::OnReadyToReceiveVideo(ssdk::transport_common::SessionHandle session, ssdk::transport_common::StreamID streamID, ssdk::transport_common::InitID initID)  // Called when a client is ready to accept video frames after video codec reinitialization.
{
    amf::AMFLock    lock(&m_Guard);
    if (streamID != ssdk::transport_common::DEFAULT_STREAM)
    {
        AMFTraceWarning(AMF_FACILITY, L"Client session %lld is trying to update an invalid video stream %lld, init ID %lld, request ignored", session, streamID, initID);
    }
    else if (m_VideoTransmitterAdapter == nullptr)
    {
        AMFTraceError(AMF_FACILITY, L"AVStreamer not initialized");
    }
    else
    {
        m_VideoTransmitterAdapter->UpdateSession(session, initID);
        m_VideoOutput->ForceKeyFrame();
        AMFTraceInfo(AMF_FACILITY, L"Successfully updated session %lld for video stream %lld with init ID %lld", session, streamID, initID);
    }
}

void AVStreamer::OnForceUpdateRequest(ssdk::transport_common::StreamID streamID)                // Called when a key/IDR frame must be submitted. This can be requested by the client or decided by the server
{
    amf::AMFLock    lock(&m_Guard);
    if (streamID != ssdk::transport_common::DEFAULT_STREAM)
    {
        AMFTraceWarning(AMF_FACILITY, L"Client requested a key/IDR-frame for an invalid video stream %lld, request ignored", streamID);
    }
    else if (m_VideoOutput == nullptr)
    {
        AMFTraceError(AMF_FACILITY, L"AVStreamer not initialized");
    }
    else
    {
        m_VideoOutput->ForceKeyFrame();
        AMFTraceInfo(AMF_FACILITY, L"Key/IDR frame forced for video stream %lld", streamID);
    }
}

void AVStreamer::OnVideoRequestInit(ssdk::transport_common::SessionHandle session, ssdk::transport_common::StreamID streamID)               // Called when the video init block needs to be sent/resent
{
    amf::AMFLock    lock(&m_Guard);
    if (streamID != ssdk::transport_common::DEFAULT_STREAM)
    {
        AMFTraceWarning(AMF_FACILITY, L"Client %lld requested an init block for an invalid video stream %lld, request ignored", session, streamID);
    }
    else if (m_VideoTransmitterAdapter == nullptr)
    {
        AMFTraceError(AMF_FACILITY, L"AVStreamer not initialized");
    }
    else
    {
        m_VideoTransmitterAdapter->SendInitToSession(session);
        AMFTraceInfo(AMF_FACILITY, L"Video init block for stream %lld sent to session ID %lld", streamID, session);
    }
}

void AVStreamer::OnBitrateChangeRecieverRequest(ssdk::transport_common::SessionHandle session, ssdk::transport_common::StreamID streamID, int64_t bitrate)                   // Called when a video receiver requested bitrate change
{
    //  Client has an option to request codec parameter change. If you would prefer not to allow this, simply leave this function empty
    ssdk::video::MonoscopicVideoOutput::Ptr videoOutput;
    {
        amf::AMFLock lock(&m_Guard);
        videoOutput = m_VideoOutput;;
    }

    if (streamID != ssdk::transport_common::DEFAULT_STREAM)
    {
        AMFTraceWarning(AMF_FACILITY, L"Client %lld requested video bitrate change for an invalid video stream %lld, request ignored", session, streamID);
    }
    else if (videoOutput == nullptr)
    {
        AMFTraceError(AMF_FACILITY, L"AVStreamer not initialized");
    }
    else
    {
        AMF_RESULT result = videoOutput->SetBitrate(bitrate);
        if (result == AMF_OK)
        {
            AMFTraceInfo(AMF_FACILITY, L"Video bitrate for stream %lld has been changed to %lld by request from client session %lld", streamID, bitrate, session);
        }
        else
        {
            AMFTraceError(AMF_FACILITY, L"Failed to set video bitrate for stream %lld to %lld by request from client session %lld, result=%s", streamID, bitrate, session, amf::AMFGetResultText(result));
        }
    }
}

void AVStreamer::OnFramerateChangeRecieverRequest(ssdk::transport_common::SessionHandle session, ssdk::transport_common::StreamID streamID, float framerate)                 // Called when a video receiver requested framerate change
{
    //  Client has an option to request codec parameter change. If you would prefer not to allow this, simply leave this function empty
    ssdk::video::MonoscopicVideoOutput::Ptr videoOutput;
    amf::AMFComponentPtr                    videoCapture;
    {
        amf::AMFLock lock(&m_Guard);
        videoOutput = m_VideoOutput;;
        videoCapture = m_VideoCapture;
    }

    if (streamID != ssdk::transport_common::DEFAULT_STREAM)
    {
        AMFTraceWarning(AMF_FACILITY, L"Client %lld requested video frame rate change for an invalid video stream %lld, request ignored", session, streamID);
    }
    else if (videoOutput == nullptr)
    {
        AMFTraceError(AMF_FACILITY, L"AVStreamer not initialized");
    }
    else if (videoCapture == nullptr)
    {
        AMFTraceError(AMF_FACILITY, L"Video capture not initialized");
    }
    else
    {
        AMF_RESULT result = AMF_FAIL;
        result = videoCapture->SetProperty(AMF_DISPLAYCAPTURE_FRAMERATE, AMFConstructRate(int32_t(framerate), 1));

        if (result == AMF_OK)
        {
            AMFTraceInfo(AMF_FACILITY, L"Video capture frame rate for stream %lld has been changed to %5.2f by request from client session %lld", streamID, framerate, session);
        }
        else
        {
            AMFTraceError(AMF_FACILITY, L"Failed to set video capture frame rate for stream %lld to %5.2f by request from client session %lld, result=%s", streamID, framerate, session, amf::AMFGetResultText(result));
        }

        result = videoOutput->SetFramerate(framerate);
        if (result == AMF_OK)
        {
            AMFTraceInfo(AMF_FACILITY, L"Encoder frame rate for stream %lld has been changed to %5.2f by request from client session %lld", streamID, framerate, session);
        }
        else
        {
            AMFTraceError(AMF_FACILITY, L"Failed to set encoder frame rate for stream %lld to %5.2f by request from client session %lld, result=%s", streamID, framerate, session, amf::AMFGetResultText(result));
        }
    }
}

void AVStreamer::OnResolutionChangeRecieverRequest(ssdk::transport_common::SessionHandle session, ssdk::transport_common::StreamID streamID, const AMFSize& resolution)      // Called when a video receiver requested resolution change
{
    //  Client has an option to request codec parameter change. If you would prefer not to allow this, simply leave this function empty
    ssdk::video::MonoscopicVideoOutput::Ptr videoOutput;
    {
        amf::AMFLock lock(&m_Guard);
        videoOutput = m_VideoOutput;;
    }

    if (streamID != ssdk::transport_common::DEFAULT_STREAM)
    {
        AMFTraceWarning(AMF_FACILITY, L"Client %lld requested video resolution change for an invalid video stream %lld, request ignored", session, streamID);
    }
    else if (videoOutput == nullptr)
    {
        AMFTraceError(AMF_FACILITY, L"AVStreamer not initialized");
    }
    else
    {
        AMF_RESULT result = videoOutput->SetEncodedResolution(resolution);
        if (result == AMF_OK)
        {
            AMFTraceInfo(AMF_FACILITY, L"Video resolution for stream %lld has been changed to %dx%d by request from client session %lld", streamID, resolution.width, resolution.height, session);
        }
        else
        {
            AMFTraceError(AMF_FACILITY, L"Failed to set video resolution for stream %lld to to %dx%d by request from client session %lld, result=%s", streamID, resolution.width, resolution.height, session, amf::AMFGetResultText(result));
        }
    }
}

void AVStreamer::OnQoSEvent(ssdk::transport_common::StreamID streamID, ssdk::util::QoS::QoSEvent event, const amf::AMFVariantStruct* value)
{
    switch (event)
    {
    case ssdk::util::QoS::QoSEvent::PANIC:
        AMFTraceDebug(AMF_FACILITY, L"OnQoSEvent(PANIC), reason %lld", value->int64Value);
        break;
    case ssdk::util::QoS::QoSEvent::PANIC_ENDED:
        AMFTraceDebug(AMF_FACILITY, L"OnQoSEvent(PANIC_ENDED)");
        break;
    case ssdk::util::QoS::QoSEvent::FPS_CHANGE:
        AMFTraceDebug(AMF_FACILITY, L"OnQoSEvent(FPS_CHANGE) %5.2f);", value->floatValue);
        OnFramerateChangeByQoS(value->floatValue, streamID);
        break;
    case ssdk::util::QoS::QoSEvent::FPS_REACHED_LOW_LIMIT:
        AMFTraceDebug(AMF_FACILITY, L"OnQoSEvent(FPS_REACHED_LOW_LIMIT) %5.2f", value->floatValue);
        break;
    case ssdk::util::QoS::QoSEvent::FPS_REACHED_HIGH_LIMIT:
        AMFTraceDebug(AMF_FACILITY, L"OnQoSEvent(FPS_REACHED_HIGH_LIMIT)%5.2f", value->floatValue);
        break;
    case ssdk::util::QoS::QoSEvent::VIDEO_BITRATE_CHANGED:
        AMFTraceDebug(AMF_FACILITY, L"OnQoSEvent(VIDEO_BITRATE_CHANGED (%lld)", value->int64Value);
        OnBitrateChangeByQoS(value->int64Value, streamID);
        break;
    case ssdk::util::QoS::QoSEvent::VIDEO_BITRATE_REACHED_LOW_LIMIT:
        AMFTraceDebug(AMF_FACILITY, L"OnQoSEvent(VIDEO_BITRATE_REACHED_LOW_LIMIT) (%lld)", value->int64Value);
        break;
    case ssdk::util::QoS::QoSEvent::VIDEO_BITRATE_REACHED_HIGH_LIMIT:
        AMFTraceDebug(AMF_FACILITY, L"OnQoSEvent(VIDEO_BITRATE_REACHED_HIGH_LIMIT) (%lld)", value->int64Value);
        break;
    case ssdk::util::QoS::QoSEvent::VIDEO_ENCODER_QUEUE_THRESHOLD_EXCEEDED:
        AMFTraceDebug(AMF_FACILITY, L"OnQoSEvent(VIDEO_ENCODER_QUEUE_THRESHOLD_EXCEEDED)(%lld)", value->int64Value);
        break;
    default:
        AMFTraceDebug(AMF_FACILITY, L"OnQoSEvent(unknown!)");
        break;
    }
}

void AVStreamer::OnBitrateChangeByQoS(int64_t bitrate, ssdk::transport_common::StreamID streamID)               // Called when QoS requested bitrate change
{
    ssdk::video::MonoscopicVideoOutput::Ptr videoOutput;
    {
        amf::AMFLock lock(&m_Guard);
        videoOutput = m_VideoOutput;;
    }

    if (streamID != ssdk::transport_common::DEFAULT_STREAM)
    {
        AMFTraceError(AMF_FACILITY, L"QoS requested video bitrate change for an invalid video stream %lld, request ignored", streamID);
    }
    else if (videoOutput == nullptr)
    {
        AMFTraceError(AMF_FACILITY, L"AVStreamer not initialized");
    }
    else
    {
        AMF_RESULT result = videoOutput->SetBitrate(bitrate);
        if (result == AMF_OK)
        {
            AMFTraceDebug(AMF_FACILITY, L"Video bitrate for stream %lld has been changed to %lld by QoS", streamID, bitrate);
        }
        else
        {
            AMFTraceError(AMF_FACILITY, L"QoS failed to set video bitrate for stream %lld to %lld by request from client session %lld, result=%s", streamID, bitrate, amf::AMFGetResultText(result));
        }
    }
}

void AVStreamer::OnFramerateChangeByQoS(float framerate, ssdk::transport_common::StreamID streamID)             // Called when QoS requested framerate change
{
    ssdk::video::MonoscopicVideoOutput::Ptr videoOutput;
    amf::AMFComponentPtr                    videoCapture;
    {
        amf::AMFLock lock(&m_Guard);
        videoOutput = m_VideoOutput;;
        videoCapture = m_VideoCapture;
    }

    if (streamID != ssdk::transport_common::DEFAULT_STREAM)
    {
        AMFTraceError(AMF_FACILITY, L"QoS requested video frame rate change for an invalid video stream %lld, request ignored", streamID);
    }
    else if (videoOutput == nullptr)
    {
        AMFTraceError(AMF_FACILITY, L"AVStreamer not initialized");
    }
    else if (videoCapture == nullptr)
    {
        AMFTraceError(AMF_FACILITY, L"Video capture not initialized");
    }
    else
    {
        AMF_RESULT result = AMF_FAIL;
        result = videoCapture->SetProperty(AMF_DISPLAYCAPTURE_FRAMERATE, AMFConstructRate(int32_t(framerate), 1));

        if (result == AMF_OK)
        {
            AMFTraceDebug(AMF_FACILITY, L"Video capture frame rate for stream %lld has been changed to %5.2f by QoS", streamID, framerate);
        }
        else
        {
            AMFTraceError(AMF_FACILITY, L"QoS failed to set video capture frame rate for stream %lld to %5.2f, result=%s", streamID, framerate, amf::AMFGetResultText(result));
        }

        result = videoOutput->SetFramerate(framerate);
        if (result == AMF_OK)
        {
            AMFTraceDebug(AMF_FACILITY, L"Encoder frame rate for stream %lld has been changed to %5.2f by QoS", streamID, framerate);
        }
        else
        {
            AMFTraceError(AMF_FACILITY, L"QoS failed to set encoder frame rate for stream %lld to %5.2f, result=%s", streamID, framerate, amf::AMFGetResultText(result));
        }
    }
}

void AVStreamer::OnVideoStats(ssdk::transport_common::SessionHandle session, ssdk::transport_common::StreamID /*streamID*/, const ssdk::transport_common::ServerTransport::VideoStatsCallback::Stats& stats)                                                               // Called when statistics is updated for a specific client/stream
{
    if (m_SessionsVideo.find(session) != m_SessionsVideo.end())
    {
        if (m_QoS != nullptr)
        {
            m_QoS->UpdateSessionStats(session, stats.lastStatsTime, stats.receiverFramerate, stats.keyFrameReqCount, stats.avgSendTime, stats.decoderQueueDepth);
        }
        AMFTraceInfo(AMF_FACILITY, L"Average latency (ms): full %5.2f, client %5.2f, server %5.2f, encoder %5.2f, network %5.2f, decoder %5.2f, Frame rate: %5.2f fps",
            stats.fullLatency,
            stats.clientLatency,
            stats.serverLatency,
            stats.encoderLatency,
            stats.networkLatency,
            stats.decoderLatency,
            stats.receiverFramerate);
    }
}

void AVStreamer::OnOriginPts(ssdk::transport_common::SessionHandle /*session*/, ssdk::transport_common::StreamID /*streamID*/, amf_pts originPts)
{
    amf::AMFLock    lock(&m_Guard);
    m_LastOriginPts = originPts;
    m_TimeOfLastOriginPts = amf_high_precision_clock();
}

//  Audio callbacks:
void AVStreamer::OnAudioStreamSubscribed(ssdk::transport_common::SessionHandle session, ssdk::transport_common::StreamID streamID)                           // Called when a client subscribes to a specific video stream
{
    amf::AMFLock    lock(&m_Guard);
    if (streamID != ssdk::transport_common::DEFAULT_STREAM)
    {
        AMFTraceWarning(AMF_FACILITY, L"Client session %lld is trying to subscribe to an invalid video stream %lld, request ignored", session, streamID);
    }
    else if (m_AudioTransmitterAdapter == nullptr)
    {
        AMFTraceError(AMF_FACILITY, L"AVStreamer not initialized");
    }
    else
    {
        m_SessionsAudio.emplace(session);
        bool result = m_AudioTransmitterAdapter->RegisterSession(session);
        if (result == true)
        {
            if (m_AudioCaptureThread.IsRunning() == false)
            {
                m_AudioCaptureThread.Start();
                AMFTraceInfo(AMF_FACILITY, L"Audio capture started");
            }
            m_AudioTransmitterAdapter->SendInitToSession(session);
            AMFTraceInfo(AMF_FACILITY, L"Successfully registered session %lld for audio stream %lld", session, streamID);
        }
        else
        {
            AMFTraceWarning(AMF_FACILITY, L"Session %lld is already registered for audio stream %lld", session, streamID);
        }
    }
}

void AVStreamer::OnAudioStreamUnsubscribed(ssdk::transport_common::SessionHandle session, ssdk::transport_common::StreamID streamID)                         // Called when a client unsibscribed from a specific video stream
{
    {
        amf::AMFLock    lock(&m_Guard);
        if (streamID != ssdk::transport_common::DEFAULT_STREAM)
        {
            AMFTraceWarning(AMF_FACILITY, L"Client session %lld is trying to unsubscribe from an invalid audio stream %lld, request ignored", session, streamID);
        }
        else if (m_AudioTransmitterAdapter == nullptr)
        {
            AMFTraceError(AMF_FACILITY, L"AVStreamer not initialized");
        }
        else
        {
            m_AudioTransmitterAdapter->UnregisterSession(session);
            m_SessionsAudio.erase(session);
            AMFTraceInfo(AMF_FACILITY, L"Successfully unregistered session %lld for audio stream %lld", session, streamID);
        }
    }
    StopAudioCaptureIfNecessary();
}

void AVStreamer::OnReadyToReceiveAudio(ssdk::transport_common::SessionHandle session, ssdk::transport_common::StreamID streamID, ssdk::transport_common::InitID initID)     // Called when an ACK or a NACK is received from the client. The id parameter contains the same value as initID passed to SendAudioInit
{
    amf::AMFLock    lock(&m_Guard);
    if (streamID != ssdk::transport_common::DEFAULT_STREAM)
    {
        AMFTraceWarning(AMF_FACILITY, L"Client session %lld is trying to update an invalid audio stream %lld, init ID %lld, request ignored", session, streamID, initID);
    }
    else if (m_AudioTransmitterAdapter == nullptr)
    {
        AMFTraceError(AMF_FACILITY, L"AVStreamer not initialized");
    }
    else
    {
        m_AudioTransmitterAdapter->UpdateSession(session, initID);
        AMFTraceInfo(AMF_FACILITY, L"Successfully updated session %lld for audio stream %lld with init ID %lld", session, streamID, initID);
    }
}

void AVStreamer::OnAudioRequestInit(ssdk::transport_common::SessionHandle session, ssdk::transport_common::StreamID streamID)                                // Called when the audio init block needs to be sent/resent
{
    amf::AMFLock    lock(&m_Guard);
    if (streamID != ssdk::transport_common::DEFAULT_STREAM)
    {
        AMFTraceWarning(AMF_FACILITY, L"Client %lld requested an init block for an invalid audio stream %lld, request ignored", session, streamID);
    }
    else if (m_AudioTransmitterAdapter == nullptr)
    {
        AMFTraceError(AMF_FACILITY, L"AVStreamer not initialized");
    }
    else
    {
        m_AudioTransmitterAdapter->SendInitToSession(session);
        AMFTraceInfo(AMF_FACILITY, L"Audio init block for stream %lld sent to session ID %lld", streamID, session);
    }
}

void AVStreamer::CaptureVideo()
{
    int64_t lastOriginPts = m_LastOriginPts;
    int64_t timeOfLastOriginPts = m_TimeOfLastOriginPts;
    amf::AMFDataPtr capturedData;
    if (m_VideoCapture->QueryOutput(&capturedData) == AMF_OK)
    {
        amf::AMFSurfacePtr frame(capturedData);
        m_TimestampCalibrator.SubmitVideo(frame);   //  This synchronizes low latency video timestamp to audio. See notes in sdk/util/pipeline/TimestampCalibrator.h for detailed explanations
        m_VideoOutput->SubmitInput(frame, lastOriginPts, timeOfLastOriginPts);
    }
}

void AVStreamer::CaptureAudio()
{
    amf::AMFDataPtr capturedData;
    if (m_AudioCapture->QueryOutput(&capturedData) == AMF_OK)
    {
        amf::AMFAudioBufferPtr buffer(capturedData);
        m_TimestampCalibrator.SubmitAudio(buffer);
        m_AudioOutput->SubmitInput(buffer);
    }
}

AVStreamer::VideoCaptureThread::VideoCaptureThread(AVStreamer& streamer) :
    m_Streamer(streamer)
{
}

void AVStreamer::VideoCaptureThread::Run()
{
    while (StopRequested() == false)
    {
        m_Streamer.CaptureVideo();
    }
}

AVStreamer::AudioCaptureThread::AudioCaptureThread(AVStreamer& streamer) :
    m_Streamer(streamer)
{
}

void AVStreamer::AudioCaptureThread::Run()
{
    while (StopRequested() == false)
    {
        m_Streamer.CaptureAudio();
    }
}
