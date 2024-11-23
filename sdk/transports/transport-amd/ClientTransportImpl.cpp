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

#include "ClientTransportImpl.h"

#include "messages/audio/AudioInit.h"
#include "messages/audio/AudioData.h"
#include "messages/sensors/DeviceEvent.h"
#include "messages/service/StartStop.h"
#include "messages/service/Stats.h"
#include "messages/video/Cursor.h"
#include "messages/video/VideoInit.h"
#include "messages/video/VideoData.h"
#include "messages/sensors/TrackableDeviceCaps.h"

#include "controllers/UserInput.h"

#include "util/stats/ClientStatsManager.h"

#include "amf/public/common/TraceAdapter.h"


/*
todos:
merge ServerEnumCallback with ConnectionManagerCallback and ServerParameters with ServerDescriptor
merge ReceiverCallback::TerminationReason and ConnectionManagerCallback::TerminationReason

missing method for client to request different resolution or framerate via UpdateRequest - reevaluate if needed when multiple StreamIDs are supported
OnAudioInMessage - Complete when implementing client to server microphone stream
*/

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static constexpr const wchar_t* const AMF_FACILITY = L"ClientTransportImpl";
#define DISCOVERY_TIMEOUT  3 // sec

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace ssdk::transport_amd
{
    // todo find a better place to keep these tags
    static constexpr const wchar_t* ANS_STATISTICS_SERVER_GPU_CLK = L"ServerGpuClk";        // int32_t; current server gpu core clock (mhz)
    static constexpr const wchar_t* ANS_STATISTICS_SERVER_GPU_USAGE = L"ServerGpuUsage";    // int32_t; current server gpu utilisation in percentage (%)
    static constexpr const wchar_t* ANS_STATISTICS_SERVER_GPU_TEMP = L"ServerGpuTemp";      // int32_t; current server gpu core temperature (C)
    static constexpr const wchar_t* ANS_STATISTICS_SERVER_CPU_CLK = L"ServerCpuClk";        // int32_t; current server cpu core clock (MHz)
    static constexpr const wchar_t* ANS_STATISTICS_SERVER_CPU_TEMP = L"ServerCpuTemp";      // int32_t; current server cpu core temperature (C)

    static constexpr const wchar_t* ID_SESSION_ID = L"ANS_SessionID";

    void ClientTransportImpl::TurnaroundLatencyThread::Run()
    {
        AMFTraceInfo(AMF_FACILITY, L"TurnaroundLatencyThread started");

        if (nullptr == m_pTransport)
        {
            AMFTraceInfo(AMF_FACILITY, L"TurnaroundLatencyThread stopped prematurely becase transport is missing");
            return;
        }

        while (StopRequested() == false)
        {
            // m_LastPts and m_Period may be updated while thread sleeps
            amf_pts lastPts;
            amf_pts period;

            {
                amf::AMFLock lock(&m_Guard);
                lastPts = m_LastPts;
                period = m_Period;
            }

            amf_pts elapsedMS = static_cast<amf_ulong>((amf_high_precision_clock() - lastPts) / 10000);
            while (elapsedMS < period)
            {
                amf_sleep(1);
                {
                    amf::AMFLock lock(&m_Guard);
                    lastPts = m_LastPts;
                    period = m_Period;
                }
                elapsedMS = static_cast<amf_ulong>((amf_high_precision_clock() - lastPts) / 10000); // in ms
            }

            // after m_Period ms passed since last pose message, send an empty pose message with current pts.
            amf::Pose pose;
            DeviceEvent::Pose headPose(pose, 0, 0);
            amf_pts pts = amf_high_precision_clock();

            std::string hmdHeadPoseID = ssdk::ctls::DEVICE_HMD;
            hmdHeadPoseID += ssdk::ctls::DEVICE_POSE;

            DeviceEvent deviceEvent(m_StreamID);
            deviceEvent.AddValue(hmdHeadPoseID, headPose, 0, pts);
            {
                amf::AMFLock lock(&m_Guard);
                m_LastPts = pts;
            }
            deviceEvent.Prepare();

            m_pTransport->SendMsg(Channel::SENSORS_IN, deviceEvent.GetSendData(), deviceEvent.GetSendSize());
        }

        m_pTransport = nullptr;

        AMFTraceInfo(AMF_FACILITY, L"TurnaroundLatencyThread terminated");
    }

    Result ClientTransportImpl::Start(const ClientInitParameters& params)
    {
        Result result = Result::FAIL;

        if (nullptr != m_pClient)
        {
            Shutdown();
        }

        AMFTraceInfo(AMF_FACILITY, L"Start()");

        amf::AMFLock lock(&m_CCCGuard);
        try
        {
            m_clientInitParameters = dynamic_cast<const ClientInitParametersAMD&>(params);
        }
        catch (const std::bad_cast& ex)
        {
            AMFTraceError(AMF_FACILITY, L"Start() Failed to cast ClientInitParameters: %s", ex.what());
            return Result::FAIL;
        }

        std::string cipherPassphrase = m_clientInitParameters.GetPassphrase();
        m_pContext = m_clientInitParameters.GetContext();
        if (nullptr == m_pContext)
        {
            AMFTraceError(AMF_FACILITY, L"Start() Failed to get context from ClientInitParameters");
        }
        else
        {
            m_pClient = new ClientImpl();
            m_pClient->SetDatagramSize(m_clientInitParameters.GetDatagramSize());
            m_pCipher = cipherPassphrase.empty() ? nullptr : ssdk::util::AESPSKCipher::Ptr(new ssdk::util::AESPSKCipher(cipherPassphrase.c_str()));
            result = Result::OK;
        }

        return result;
    }

    Result ClientTransportImpl::Shutdown()
    {
        Result result = Result::OK;

        AMFTraceInfo(AMF_FACILITY, L"Shutdown()");

        result = Disconnect();

        amf::AMFLock lock(&m_CCCGuard);
        m_pClient = nullptr;
        m_pContext = nullptr;
        m_pCipher = nullptr;
        return result;
    }


    Result ClientTransportImpl::FindServers()
    {
        ClientImpl::Ptr pClient = nullptr;
        {
            amf::AMFLock lock(&m_CCCGuard);
            pClient = m_pClient;
        }
        AMF_RETURN_IF_FALSE(nullptr != pClient, Result::FAIL, L"FindServers() Client object missing");

        Result result = Result::FAIL;
        AMFTraceInfo(AMF_FACILITY, L"FindServers()");

        unsigned short port = m_clientInitParameters.GetDiscoveryPort();
        std::string strID = m_clientInitParameters.GetID();
        const char* ID = strID.empty() ? nullptr : strID.c_str();
        amf_size count = 0;
        ServerEnumCallback* serverEnumCallBack = m_clientInitParameters.GetServerEnumCallback();

        result = pClient->EnumerateServers(ID, port, DISCOVERY_TIMEOUT, &count, serverEnumCallBack);

        AMFTraceInfo(AMF_FACILITY, Result::OK == result ? L"FindServers() Servers enumerated successfully " : L"FindServers() Enumerating servers failed");

        return result;
    }

    Result ClientTransportImpl::Connect(const char* url)
    {
        ClientImpl::Ptr pClient = nullptr;
        ssdk::util::AESPSKCipher::Ptr pCipher;
        {
            amf::AMFLock lock(&m_CCCGuard);
            pClient = m_pClient;
            pCipher = m_pCipher;
        }
        AMF_RETURN_IF_FALSE(nullptr != pClient, Result::FAIL, L"Connect() Client object missing");

        Result result = Result::FAIL;
        AMFTraceInfo(AMF_FACILITY, L"Connect()");

        amf::AMFLock lock(&m_SessionGuard);

        ServerParameters* serverParameters = nullptr;
        std::string strID = m_clientInitParameters.GetID();
        const char* ID = strID.empty() ? nullptr : strID.c_str();

        // for backwards compatability, encrypted communication will not work if client doesn't have ID_SESSION_ID property set.
        if (nullptr != pCipher)
        {
            amf::AMFVariant emptySessionID("");
            pClient->SetProperty(ID_SESSION_ID, emptySessionID);
        }

        result = pClient->ConnectToServerAndQueryParameters(url, ID, (Session**)&m_pSession, &serverParameters);

        if (nullptr != m_clientInitParameters.GetConnectionManagerCallback())
        {
            if (Result::OK == result)
            {
                ServerDescriptorAMD serverDesriptor(serverParameters->GetName(), serverParameters->GetUrl(), "Test server descriptor description");//todo: description paramter
                m_clientInitParameters.GetConnectionManagerCallback()->OnConnectionEstablished(serverDesriptor);
            }
            else if (Result::CONNECTION_TIMEOUT == result)
            {
                m_clientInitParameters.GetConnectionManagerCallback()->OnConnectionTerminated(ConnectionManagerCallback::TerminationReason::TIMEOUT);
            }
        }

        if (Result::OK != result)
        {
            AMFTraceError(AMF_FACILITY, L"Connect() ConnectToServerAndQueryParameters(%S)  - failed with result %d", url, result);
        }
        else if (nullptr == m_pSession)
        {
            AMFTraceError(AMF_FACILITY, L"Connect() Session object missing");
            result = Result::FAIL;
        }
        else if (nullptr == serverParameters)
        {
            AMFTraceError(AMF_FACILITY, L"Connect() Server paramters missing");
            result = Result::FAIL;
        }
        else
        {
            AMFTraceInfo(AMF_FACILITY, L"Connect() ConnectToServerAndQueryParameters(%S)  - succeeded", url);
            m_pSession->RegisterReceiverCallback(this);
            result = pClient->Activate();

            if (Result::OK != result)
            {
                AMFTraceError(AMF_FACILITY, L"Client failed to activate");
            }
            else
            {
                AMFTraceInfo(AMF_FACILITY, L"Connect() Client (re)activated");
            }
        }

        return result;
    }

    Result ClientTransportImpl::Disconnect()
    {
        Result result = Result::OK;

        AMFTraceInfo(AMF_FACILITY, L"Disconnect()");

        m_TurnaroundLatencyThread.RequestStop();
        m_TurnaroundLatencyThread.WaitForStop();

        if (nullptr != m_clientInitParameters.GetConnectionManagerCallback())
        {
            m_clientInitParameters.GetConnectionManagerCallback()->OnConnectionTerminated(ConnectionManagerCallback::TerminationReason::CLOSED_BY_CLIENT);
        }

        //  Unsubscribe from all video and audio streams
        for (auto& it : m_SubscribedVideoStreams)
        {
            StopRequest stopRequest(it, INVALID_STREAM);
            result = SendMsg(Channel::SERVICE, stopRequest.GetSendData(), stopRequest.GetSendSize());
            if (result != Result::OK)
            {
                AMFTraceError(AMF_FACILITY, L"Disconnect() Failed to send Stop Request err=%d", (int)result);
            }
        }
        m_SubscribedVideoStreams.clear();
        for (auto& it : m_SubscribedAudioStreams)
        {
            StopRequest stopRequest(INVALID_STREAM, it);
            result = SendMsg(Channel::SERVICE, stopRequest.GetSendData(), stopRequest.GetSendSize());
            if (result != Result::OK)
            {
                AMFTraceError(AMF_FACILITY, L"Disconnect() Failed to send Stop Request err=%d", (int)result);
            }
        }
        m_SubscribedAudioStreams.clear();
        m_FrameLossInfoMap.clear();

        ClientSessionImpl::Ptr session;
        {
            amf::AMFLock lock(&m_SessionGuard);
            session = m_pSession;
            m_pSession = nullptr;
        }
        if (nullptr != session)
        {
            session->Terminate();
        }

        ClientImpl::Ptr pClient = nullptr;
        {
            amf::AMFLock lock(&m_CCCGuard);
            pClient = m_pClient;
        }
        if (nullptr != pClient)
        {
            pClient->Deactivate();
        }

        return result;
    }

    Result ClientTransportImpl::SubscribeToVideoStream(StreamID StreamID)
    {
        Result result = Result::FAIL;

        StartRequest startRequest(
            m_clientInitParameters.GetDisplayModel(),
            static_cast<int32_t>(m_clientInitParameters.GetDisplayWidth()),
            static_cast<int32_t>(m_clientInitParameters.GetDisplayHeight()),
            m_clientInitParameters.GetFramerate(),
            static_cast<int64_t>(m_clientInitParameters.GetBitrate()),
            0,//InterpupillaryDistanceValue,
            0,//someAspectRatioValue,
            0,//someSeparateEyeProcessingValue,
            0,//someVideoCodecValue,
            0,//someNonLinearScalingSupportedValue,
            0,//someAudioCodecValue,
            0,//someAudioChannelsValue,
            0,//someAudioChannelLayoutValue,
            static_cast<int32_t>(m_clientInitParameters.GetDisplayClientWidth()),
            static_cast<int32_t>(m_clientInitParameters.GetDisplayClientHeight()),
            StreamID, INVALID_STREAM
        );
        result = SendMsg(Channel::SERVICE, startRequest.GetSendData(), startRequest.GetSendSize());
        if (Result::OK != result)
        {
            AMFTraceError(AMF_FACILITY, L"SubscribeToVideoStream(%lld) Failed to send Start Request err=%d", StreamID, (int)result);
        }
        else
        {
            AMFTraceInfo(AMF_FACILITY, L"SubscribeToVideoStream(%lld) sent Start Request", StreamID);
            m_SubscribedVideoStreams.insert(StreamID);

            // start latency thread
            amf_uint latencyMessagePeriod = m_clientInitParameters.GetLatencyMessagePeriod();
            float framerate = m_clientInitParameters.GetFramerate();
            if (0 == latencyMessagePeriod)
            {
                latencyMessagePeriod = framerate > 0 ? static_cast<amf_uint>(std::round(1000.0f / framerate)) : TURNAROUND_LATENCY_MESSAGE_PERIOD;
            }
            m_TurnaroundLatencyThread.Init(this, latencyMessagePeriod, StreamID);
            m_TurnaroundLatencyThread.Start();
        }

        return result;
    }

    Result ClientTransportImpl::SubscribeToAudioStream(StreamID StreamID)
    {
        Result result = Result::FAIL;

        StartRequest startRequest(
            m_clientInitParameters.GetDisplayModel(),
            static_cast<int32_t>(m_clientInitParameters.GetDisplayWidth()),
            static_cast<int32_t>(m_clientInitParameters.GetDisplayHeight()),
            m_clientInitParameters.GetFramerate(),
            static_cast<int64_t>(m_clientInitParameters.GetBitrate()),
            0,//InterpupillaryDistanceValue,
            0,//someAspectRatioValue,
            0,//someSeparateEyeProcessingValue,
            0,//someVideoCodecValue,
            0,//someNonLinearScalingSupportedValue,
            0,//someAudioCodecValue,
            0,//someAudioChannelsValue,
            0,//someAudioChannelLayoutValue,
            static_cast<int32_t>(m_clientInitParameters.GetDisplayClientWidth()),
            static_cast<int32_t>(m_clientInitParameters.GetDisplayClientHeight()),
            INVALID_STREAM, StreamID
        );
        result = SendMsg(Channel::SERVICE, startRequest.GetSendData(), startRequest.GetSendSize());
        if (Result::OK != result)
        {
            AMFTraceError(AMF_FACILITY, L"SubscribeToAudioStream(%lld) Failed to send Start Request err=%d", StreamID, (int)result);
        }
        else
        {
            m_SubscribedAudioStreams.insert(StreamID);
            AMFTraceInfo(AMF_FACILITY, L"SubscribeToAudioStream(%lld) sent Start Request", StreamID);
        }
        return result;
    }

    Result ClientTransportImpl::UnsubscribeFromVideoStream(StreamID StreamID)
    {
        Result result = Result::FAIL;
        StopRequest stop(StreamID, INVALID_STREAM);
        result = SendMsg(Channel::SERVICE, stop.GetSendData(), stop.GetSendSize());
        if (Result::OK != result)
        {
            AMFTraceError(AMF_FACILITY, L"UnsubscribeFromVideoStream(%lld) Failed to send Stop Request err=%d", StreamID, (int)result);
        }
        else
        {
            m_SubscribedVideoStreams.erase(StreamID);
            m_FrameLossInfoMap.erase(StreamID);
            AMFTraceInfo(AMF_FACILITY, L"UnsubscribeFromVideoStream(%lld) sent Stop Request", StreamID);
        }

        return result;
    }

    Result ClientTransportImpl::UnsubscribeFromAudioStream(StreamID StreamID)
    {
        Result result = Result::FAIL;
        StopRequest stop(INVALID_STREAM, StreamID);
        result = SendMsg(Channel::SERVICE, stop.GetSendData(), stop.GetSendSize());
        if (Result::OK != result)
        {
            AMFTraceError(AMF_FACILITY, L"UnsubscribeFromVideoStream(%lld) Failed to send Stop Request err=%d", StreamID, (int)result);
        }
        else
        {
            m_SubscribedAudioStreams.erase(StreamID);
            AMFTraceInfo(AMF_FACILITY, L"UnsubscribeFromVideoStream(%lld) sent Start Request", StreamID);
        }

        return result;
    }

    //  Video:
    Result ClientTransportImpl::SendVideoInit(const char* codec, StreamID StreamID, InitID initID, const AMFSize& streamResolution, const AMFRect& viewport, uint32_t bitDepth, const void* initBlock, size_t initBlockSize)
    {
        VideoInit videoInit(initID, codec, streamResolution.width, streamResolution.height, viewport, false, bitDepth, StreamID);
        return SendMessageWithData(Channel::VIDEO_IN, &videoInit, initBlock, initBlockSize);
    }

    Result ClientTransportImpl::SendVideoFrame(StreamID /*StreamID*/, const TransmittableVideoFrame& /*frame*/)
    {
        // todo
        Result result = Result::FAIL;

        /*
        VideoData::VideoData(frame., amf_pts ptsSensor, amf_pts ptsServerLatency, amf_pts ptsEncoderLatency,
                         uint32_t compressedFrameSize, ssdk::transport_common::VideoFrame::ViewType eFrameType, ssdk::transport_common::VideoFrame::SubframeType eEncodedFrameType,
                         amf_pts ptsLastSendDuration, amf_uint64 uiFrameNum, bool discontinuity, const char* userMsg)
        */
        return result;
    }

    //  Audio:
    Result ClientTransportImpl::SendAudioInit(const char* codec, StreamID StreamID, InitID initID, amf::AMF_AUDIO_FORMAT format, uint32_t channels, uint32_t layout, uint32_t samplingRate, const void* initBlock, size_t initBlockSize)
    {
        AudioInit audioInit(initID, codec, format, samplingRate, channels, layout, StreamID);
        return SendMessageWithData(Channel::AUDIO_IN, &audioInit, initBlock, initBlockSize);
    }

    Result ClientTransportImpl::SendAudioBuffer(StreamID StreamID, const TransmittableAudioBuffer& transmittableBuf)
    {
        Result result = Result::FAIL;

        amf::AMFBufferPtr buf;
        const_cast<TransmittableAudioBuffer&>(transmittableBuf).GetBuffer(&buf);
        if (nullptr == buf)
        {
            result = Result::INVALID_ARG;
        }
        else
        {
            amf_pts pts = buf->GetPts();
            amf_pts duration = buf->GetDuration();
            uint32_t size = (uint32_t)buf->GetSize();
            AudioData audioData(pts, duration, size, transmittableBuf.GetSequenceNumber(), transmittableBuf.IsDiscontinuity(), StreamID);

            result = SendMessageWithData(Channel::AUDIO_IN, &audioData, buf->GetNative(), buf->GetSize());
        }

        return result;
    }

    //  Controllers:
    Result ClientTransportImpl::SendControllerEvent(const char* controlID, const amf::AMFVariant& event)
    {
        DeviceEvent devEvent;
        devEvent.AddValue(controlID, event, 0);
        devEvent.Prepare();

        return SendMsg(Channel::SENSORS_IN, devEvent.GetSendData(), devEvent.GetSendSize());
    }

    Result ClientTransportImpl::SendTrackableDevicePose(const char* /*deviceID*/, const Pose& /*pose*/)
    {
        // todo:
        Result result = Result::FAIL;

        return result;
    }

    //  Devices
    Result ClientTransportImpl::SendDeviceConnected(const char* deviceID, ssdk::ctls::CTRL_DEVICE_TYPE eType, const char* description)
    {
        Result result = Result::FAIL;

        TrackableDeviceCaps::DeviceClass eClass = TrackableDeviceCaps::DeviceClass::UNKNOWN;
        switch (eType)
        {
        case ssdk::ctls::CTRL_DEVICE_TYPE::UNKNOWN:
            eClass = TrackableDeviceCaps::DeviceClass::UNKNOWN;
            break;
        case ssdk::ctls::CTRL_DEVICE_TYPE::DISPLAY:
            eClass = TrackableDeviceCaps::DeviceClass::HMD;
            break;
        case ssdk::ctls::CTRL_DEVICE_TYPE::VR_CONTROLLER:
            eClass = TrackableDeviceCaps::DeviceClass::VR_CONTROLLER;
            break;
        case ssdk::ctls::CTRL_DEVICE_TYPE::MOUSE:
            eClass = TrackableDeviceCaps::DeviceClass::MOUSE;
            break;
        case ssdk::ctls::CTRL_DEVICE_TYPE::TOUCHSCREEN:
            eClass = TrackableDeviceCaps::DeviceClass::TOUCHSCREEN;
            break;
        case ssdk::ctls::CTRL_DEVICE_TYPE::KEYBOARD:
            eClass = TrackableDeviceCaps::DeviceClass::KEYBOARD;
            break;
        case ssdk::ctls::CTRL_DEVICE_TYPE::GAME_CONTROLLER:
            eClass = TrackableDeviceCaps::DeviceClass::GAME_CONTROLLER;
            break;
        }

        TrackableDeviceCaps controllerConnectedRequest(std::string(deviceID),
            eClass,
            description,
            nullptr, nullptr,
            false);

        result = SendMsg(Channel::SERVICE, controllerConnectedRequest.GetSendData(), controllerConnectedRequest.GetSendSize());

        return result;
    }

    Result ClientTransportImpl::SendDeviceDisconnected(const char* /*deviceID*/)
    {
        // todo: send TRACKABLE_DEVICE_DISCONNECTED
        Result result = Result::FAIL;

        return result;
    }

    Result ClientTransportImpl::SendStats(StreamID streamID, const StatsSnapshot& statsSnapshot)
    {
        Result result = Result::FAIL;
                        
        const ssdk::util::ClientStatsManager::StatsSnapshotImpl statsSnapshotImpl = dynamic_cast<const ssdk::util::ClientStatsManager::StatsSnapshotImpl&>(statsSnapshot);
        Statistics statsMessage(streamID, 
                                statsSnapshotImpl.GetFullLatency(),
                                statsSnapshotImpl.GetClientLatency(),
                                statsSnapshotImpl.GetServerLatency(),
                                statsSnapshotImpl.GetEncoderLatency(),
                                statsSnapshotImpl.GetNetworkLatency(),
                                statsSnapshotImpl.GetDecoderLatency(),
                                0/*float encryption time*/,
                                statsSnapshotImpl.GetDecryptLatency(),
                                statsSnapshotImpl.GetDecoderQueueDepth(),
                                statsSnapshotImpl.GetAVDesync(),
                                statsSnapshotImpl.GetFramerate());

        result = SendMsg(Channel::SERVICE, statsMessage.GetSendData(), statsMessage.GetSendSize());
        if (Result::OK != result)
        {
            AMFTraceError(AMF_FACILITY, L"SendStats(%lld) Failed to send Stat Latency Message err=%d", streamID, (int)result);
        }
        return result;
    }

    //  Application-specific communications:
    Result ClientTransportImpl::SendApplicationMessage(const void* msg, size_t size)
    {
        return SendMsg(Channel::USER_DEFINED, msg, size);
    }

    // ReceiverCallback methods:
    void ClientTransportImpl::OnMessageReceived(Session* session, Channel channel, int msgID, const void* msg, size_t messageSize)
    {
        ssdk::util::AESPSKCipher::Ptr pCipher = nullptr;
        amf::AMFContextPtr pContext = nullptr;
        ssdk::util::ClientStatsManager::Ptr statsManager;
        {
            amf::AMFLock lock(&m_CCCGuard);
            pCipher = m_pCipher;
            pContext = m_pContext;
            statsManager = m_StatsManager;
        }

        if (nullptr == session)
        {
            AMFTraceError(AMF_FACILITY, L"OnMessageReceived() - received session pointer is nullptr");
        }
        else if (nullptr == pCipher)
        {
            ProcessMessage(session, channel, msgID, msg, messageSize);
        }
        else
        {
            amf::AMFBufferPtr clearTextBuffer;

            size_t clearTextOfs = 0;
            size_t clearTextBufferSize = pCipher->GetClearTextBufferSize(messageSize);
            AMF_RESULT res = pContext->AllocBuffer(amf::AMF_MEMORY_HOST, clearTextBufferSize, &clearTextBuffer);

            if (res != AMF_OK)
            {
                AMFTraceError(AMF_FACILITY, L"Decrypt() - failed to allocate a buffer");
            }
            else
            {
                amf_pts startDecryptPts = amf_high_precision_clock();
                size_t clearTextSize;
                bool bSuccess = pCipher->Decrypt(msg, messageSize, clearTextBuffer->GetNative(), &clearTextOfs, &clearTextSize);
                amf_pts decryptPts = amf_high_precision_clock() - startDecryptPts;
                if (!bSuccess)
                {
                    AMFTraceError(AMF_FACILITY, L"OnMessageReceived() - decryption failed");
                }
                // If password doesn't match, clearTextSize should be a meaningless number that's likely
                // larger than clearTextBufferSize. In the rare case where size get decrypted into
                // a number that's within the reasonable range, client and server would do an 8 bit
                // signature check again.
                else if (clearTextSize <= clearTextBufferSize)
                {
                    const uint8_t* messagePtr = static_cast<const uint8_t*>(clearTextBuffer->GetNative());
                    messagePtr += clearTextOfs;
                    if (statsManager != nullptr)
                    {
                        statsManager->UpdateDecryptStatistics(decryptPts);
                    }
                    ProcessMessage(session, channel, msgID, messagePtr, clearTextSize);
                }
                else
                {
                    AMFTraceError(AMF_FACILITY, L"OnMessageReceived() - decryption failed");
                }
            }
        }
    }

    void ClientTransportImpl::OnTerminate(Session* /*session*/, ReceiverCallback::TerminationReason reason)
    {
        ConnectionManagerCallback::TerminationReason terminationReason = ConnectionManagerCallback::TerminationReason::CLOSED_BY_SERVER;;
        switch (reason)
        {
        case ReceiverCallback::TerminationReason::DISCONNECT:
        {
            terminationReason = ConnectionManagerCallback::TerminationReason::CLOSED_BY_SERVER;
            break;
        }
        case ReceiverCallback::TerminationReason::TIMEOUT:
        {
            terminationReason = ConnectionManagerCallback::TerminationReason::TIMEOUT;
            break;
        }
        default:
            AMFTraceError(AMF_FACILITY, L"OnTerminate - encountered unexpected termination reason(%d)", reason);
            break;
        }

        if (nullptr != m_clientInitParameters.GetConnectionManagerCallback())
        {
            m_clientInitParameters.GetConnectionManagerCallback()->OnConnectionTerminated(terminationReason);
        }

        m_TurnaroundLatencyThread.RequestStop();
        m_TurnaroundLatencyThread.WaitForStop();
    }

    //  Own methods:
    void ClientTransportImpl::SetStatsManager(ssdk::util::ClientStatsManager::Ptr statsManager)
    {
        amf::AMFLock    lock(&m_CCCGuard);
        m_StatsManager = statsManager;
    }

    Result ClientTransportImpl::SendMessageWithData(Channel channel, Message* msg, const void* data, size_t dataSize)
    {
        AMF_RETURN_IF_FALSE(nullptr != msg, Result::FAIL, L"SendMessageWithData() message missing");

        Result result = Result::FAIL;

        size_t  messageSize = msg->GetSendSize() + 1 + dataSize;

        SendData buffer = SendData(new char[messageSize]);
        char* rawPtr = buffer.get();

        memcpy(rawPtr, msg->GetSendData(), msg->GetSendSize());

        rawPtr += msg->GetSendSize();
        *rawPtr++ = 0;

        if (dataSize > 0 && nullptr != data)
        {
            memcpy(rawPtr, data, dataSize);
        }

        result = SendMsg(channel, rawPtr, messageSize);

        return result;
    }

    void ClientTransportImpl::ProcessMessage(Session* session, Channel channel, int msgID, const void* msg, size_t messageSize)
    {
        ClientImpl::Ptr pClient = nullptr;
        {
            amf::AMFLock lock(&m_CCCGuard);
            pClient = m_pClient;
        }

        if (nullptr == pClient)
        {
            AMFTraceError(AMF_FACILITY, L"ProcessMessage - Client is missing");
        }
        else
        {
            // ClientImpl processes received SERVICE_OP_CODE::HELLO and SERVICE_OP_CODE::CONNECTION_REFUSED messages frist
            pClient->OnMessageReceived(session, channel, msgID, msg, messageSize);
        }

        switch (channel)
        {
        case Channel::SERVICE:
            OnServiceMessage(session, msg, messageSize);
            break;
        case Channel::VIDEO_OUT:
            OnVideoOutMessage(session, msg, messageSize);
            break;
        case Channel::AUDIO_OUT:
            OnAudioOutMessage(session, msg, messageSize);
            break;
        case Channel::AUDIO_IN:
            OnAudioInMessage(session, msg, messageSize); // Todo
            break;
        case Channel::SENSORS_IN:
            OnSensorsInMessage(session, msg, messageSize); // Todo
            break;
        case Channel::MISC_OUT:
            OnMiscOutMessage(session, msg, messageSize);
            break;
        case Channel::SENSORS_OUT:
            OnSensorsOutMessage(session, msg, messageSize);
            break;
        case Channel::USER_DEFINED:
            OnUserDefinedMessage(session, msg, messageSize);
            break;
        default:
            AMFTraceError(AMF_FACILITY, L"ProcessMessage() received unexpected channel code %d", channel);
            break;
        }
    }

    void ClientTransportImpl::OnServiceMessage(Session* session, const void* msg, size_t messageSize)
    {
        uint8_t opCode = *((const uint8_t*)msg);
        switch (SERVICE_OP_CODE(opCode))
        {
        case SERVICE_OP_CODE::CONNECTION_REFUSED:
            AMFTraceInfo(AMF_FACILITY, L"OnServiceMessage() - CONNECTION_REFUSED from %S", session->GetPeerUrl());
            OnServiceConnectionRefused();
            break;
        case SERVICE_OP_CODE::SERVER_STAT:
            AMFTraceInfo(AMF_FACILITY, L"OnServiceMessage() - SERVER_STAT from %S", session->GetPeerUrl());
            OnServiceServerStat(session, msg, messageSize);
            break;
        case SERVICE_OP_CODE::CODECS_UPDATE:
        case SERVICE_OP_CODE::TRACKABLE_DEVICE_CAPS:
            // This message is ignored here, and sensors thread is managed by controller manager
            break;
        default:
            AMFTraceInfo(AMF_FACILITY, L"OnServiceMessage() - invalid opcode %d from %S", opCode, session->GetPeerUrl());
            break;
        }
    }
    void ClientTransportImpl::OnServiceConnectionRefused()
    {
        if (nullptr != m_clientInitParameters.GetConnectionManagerCallback())
        {
            ConnectionManagerCallback::TerminationReason terminationReason = ConnectionManagerCallback::TerminationReason::CLOSED_BY_SERVER;
            m_clientInitParameters.GetConnectionManagerCallback()->OnConnectionTerminated(terminationReason);
        }

        m_TurnaroundLatencyThread.RequestStop();
        m_TurnaroundLatencyThread.WaitForStop();
    }

    void ClientTransportImpl::OnServiceForceIDRFrame(Session* /*session*/)
    {
    }

    void ClientTransportImpl::OnServiceServerStat(Session* /*session*/, const void* msg, size_t messageSize)
    {
        ServerStat request;
        if (request.ParseBuffer(msg, messageSize) == false)
        {
            AMFTraceError(AMF_FACILITY, L"OnServiceServerStat - invalid JSON: %S", msg);
        }
        else if (nullptr != m_clientInitParameters.GetVideoReceiverCallback())
        {
            amf::AMFPropertyStoragePtr stats = new amf::AMFInterfaceImpl<amf::AMFPropertyStorageImpl<amf::AMFPropertyStorage>>();

            stats->SetProperty(ANS_STATISTICS_SERVER_GPU_CLK, request.GetGpuClk());
            stats->SetProperty(ANS_STATISTICS_SERVER_GPU_USAGE, request.GetGpuUsage());
            stats->SetProperty(ANS_STATISTICS_SERVER_GPU_TEMP, request.GetGpuTemp());
            stats->SetProperty(ANS_STATISTICS_SERVER_CPU_CLK, request.GetCpuClk());
            stats->SetProperty(ANS_STATISTICS_SERVER_CPU_TEMP, request.GetCpuTemp());

            StreamID videoStreamID = request.GetStreamID();

            m_clientInitParameters.GetVideoReceiverCallback()->OnVideoServerStats(videoStreamID, stats);
        }
    }

    void ClientTransportImpl::OnVideoOutMessage(Session* session, const void* msg, size_t messageSize)
    {
        uint8_t opCode = *((const uint8_t*)msg);

        switch (VIDEO_OP_CODE(opCode))
        {
        case VIDEO_OP_CODE::INIT:
            AMFTraceDebug(AMF_FACILITY, L"OnVideoOutMessage() - INIT from %S", session->GetPeerUrl());
            OnVideoOutInit(session, msg, messageSize);
            break;
        case VIDEO_OP_CODE::DATA:
            //AMFTraceDebug(AMF_FACILITY, L"OnVideoOutMessage() - DATA from %S", session->GetPeerUrl());
            OnVideoOutData(session, msg, messageSize);
            break;
        case VIDEO_OP_CODE::QOS:// NOT IMPLEMENTED BY LEGACY SERVER
            break;
        case VIDEO_OP_CODE::CURSOR:
            OnVideoOutCursor(session, msg, messageSize);
            break;
        default:
            AMFTraceError(AMF_FACILITY, L"OnVideoOutMessage() - invalid opcode %d from %S", opCode, session->GetPeerUrl());
            break;
        }
    }

    void ClientTransportImpl::OnVideoOutInit(Session* /*session*/, const void* msg, size_t messageSize)
    {
        VideoInit videoInit;
        if (videoInit.ParseBuffer(msg, messageSize) == false)
        {
            AMFTraceError(AMF_FACILITY, L"OnVideoOutInit - invalid JSON: %S", msg);
        }
        else
        {
            bool result = false;
            InitID initID = videoInit.GetID();
            StreamID videoStreamID = videoInit.GetStreamID();
            if (nullptr != m_clientInitParameters.GetVideoReceiverCallback())
            {
                std::string codecID = videoInit.GetCodecID();
                AMFSize resolution = AMFConstructSize(videoInit.GetWidth(), videoInit.GetHeight());
                AMFRect viewPort = videoInit.GetViewport();
                int32_t bitDepth = videoInit.GetBitDepth();
                bool foveated = videoInit.GetNonLinearScaling();
                size_t initBlockOfs = strlen((char*)msg + 1) + 2;// +2 - opcode + trailing zero after JSON
                const void* initBlock = static_cast<const amf_uint8*>(msg) + initBlockOfs;
                AMFTraceInfo(AMF_FACILITY, L"Recieved Video Init message, initID = %lld, codec %S, resolution: %dx%d, vieport(%d,%d,%d,%d), depth: %d",
                    initID, codecID.c_str(), videoInit.GetWidth(), videoInit.GetHeight(), viewPort.left, viewPort.top, viewPort.right, viewPort.bottom, bitDepth);

                result = m_clientInitParameters.GetVideoReceiverCallback()->OnVideoInit(codecID.c_str(), videoStreamID,
                    initID, resolution, viewPort, bitDepth, /* bool stereoscopic*/ false, foveated, initBlock, messageSize - initBlockOfs);
            }

            VideoInitAck videoInitAck(initID, result, videoStreamID);
            SendMsg(Channel::VIDEO_OUT, videoInitAck.GetSendData(), videoInitAck.GetSendSize());
            AMFTraceInfo(AMF_FACILITY, L"OnVideoOutInit - sent video VideoInitAck message, initID = %lld, result = %d", initID, result);
        }
    }

    // todo:  add server code to limit frequency of IDR requests sent to 4 per second.
    void ClientTransportImpl::OnVideoOutData(Session* /*session*/, const void* msg, size_t messageSize)
    {
        amf::AMFContextPtr pContext = nullptr;
        ssdk::util::ClientStatsManager::Ptr statsManager = nullptr;
        {
            amf::AMFLock lock(&m_CCCGuard);
            pContext = m_pContext;
            statsManager = m_StatsManager;
        }

        VideoData videoData;
        if (videoData.ParseBuffer(msg, messageSize) == false)
        {
            AMFTraceError(AMF_FACILITY, L"OnVideoOutData - invalid JSON: %S", msg);
        }
        else if (nullptr != m_clientInitParameters.GetVideoReceiverCallback())
        {
            ReceivableVideoFrame frame(pContext, videoData.GetViewType(), videoData.GetOriginPts(), videoData.GetFrameNum(), videoData.GetDiscontinuity());

            size_t frameBlockOfs = strlen((char*)msg + 1) + 2;// skip opt code
            const void* frameBlock = static_cast<const amf_uint8*>(msg) + frameBlockOfs;
            transport_common::VideoFrame::SubframeType subFrameType = videoData.GetSubframeType();
            frame.RegisterSubframe(0, messageSize - frameBlockOfs, subFrameType);
            if (frame.ParseBuffer(frameBlock) != AMF_OK)
            {
                AMFTraceError(AMF_FACILITY, L"OnVideoOutData - invalid frame: %S", msg);
            }
            else
            {
                // check if this is the first frame for stream StreamID
                StreamID streamID = videoData.GetStreamID();
                uint64_t frameNumber = videoData.GetFrameNum();
                bool frameProcessed = false;
                if (m_FrameLossInfoMap.find(streamID) == m_FrameLossInfoMap.end())
                {
                    if (subFrameType == transport_common::VideoFrame::SubframeType::IDR)
                    {
                        m_clientInitParameters.GetVideoReceiverCallback()->OnVideoFrame(streamID, frame);
                        frameProcessed = true;
                    }
                    else
                    {
                        AMFTraceError(AMF_FACILITY, L"The first video frame for Stream ID %llu received after connection is not decodable (type %d) likely due to IDR frame loss", streamID, static_cast<int>(subFrameType));
                        SendVideoForceUpdate(streamID, frameNumber, true);
                    }
                }
                else // not the first frame received
                {
                    if (subFrameType == transport_common::VideoFrame::SubframeType::IDR)
                    {
                        m_FrameLossInfoMap[streamID].m_IDRRequestSent = false;
                        m_clientInitParameters.GetVideoReceiverCallback()->OnVideoFrame(streamID, frame);
                        frameProcessed = true;
                    }
                    else
                    {
                        if (m_FrameLossInfoMap[streamID].m_IDRRequestSent == false)
                        {
                            uint64_t lastFrameNumber = m_FrameLossInfoMap[streamID].m_lastFrameNumbers;
                            if (frameNumber == (lastFrameNumber + 1)) // check that frame arrived in order
                            {
                                m_clientInitParameters.GetVideoReceiverCallback()->OnVideoFrame(streamID, frame); 
                                frameProcessed = true;
                            }
                            else
                            {
                                SendVideoForceUpdate(streamID, frameNumber, false); // frame received out of order
                            }
                        }
                        else // if IDR frame was requested and non-idr frame arrived, request idr frame again
                        {
                            SendVideoForceUpdate(streamID, frameNumber, true); // IDR frame lost or not yet arrived
                        }
                    }
                }
                m_FrameLossInfoMap[streamID].m_lastFrameNumbers = frameNumber;
                if (frameProcessed == true && statsManager != nullptr)
                {
                    statsManager->UpdateServerLatency(videoData.GetServerLatency());
                    statsManager->UpdateEncoderLatency(videoData.GetEncoderLatency());
                }
            }
        }
    }
    
    void ClientTransportImpl::OnVideoOutCursor(Session* /*session*/, const void* msg, size_t messageSize)
    {
        amf::AMFContextPtr pContext = nullptr;
        {
            amf::AMFLock lock(&m_CCCGuard);
            pContext = m_pContext;
        }

        CursorData cursorData;

        if (cursorData.ParseBuffer(msg, messageSize) == false)
        {
            AMFTraceError(AMF_FACILITY, L"OnVideoOutCursor - invalid JSON: %S", msg);
        }
        else if (nullptr != m_clientInitParameters.GetCursorCallback())
        {
            bool visible = cursorData.GetVisible();

            if (false == visible)
            {
                m_clientInitParameters.GetCursorCallback()->OnCursorHidden();
            }
            else if (nullptr == pContext)
            {
                AMFTraceError(AMF_FACILITY, L"OnVideoOutCursor - Context is missing");
            }
            else
            {
                int width = cursorData.GetWidth();
                int height = cursorData.GetHeight();
                int pitch = cursorData.GetPitch();

                amf::AMFSurfacePtr pSurface = nullptr;
                if (0 < width && 0 < height && 0 < pitch)
                {
                    AMF_RESULT result = pContext->AllocSurface(amf::AMF_MEMORY_HOST, amf::AMF_SURFACE_BGRA, width, height, &pSurface);
                    if (AMF_OK == result)
                    {
                        size_t messageLen = strlen((char*)msg + 1) + 1;// skip opt code
                        const void* videoDataPtr = static_cast<const amf_uint8*>(msg) + messageLen + 1;

                        amf_uint8* dst = (amf_uint8*)pSurface->GetPlaneAt(0)->GetNative();
                        int dstPitch = pSurface->GetPlaneAt(0)->GetHPitch();
                        for (int i = 0; i < height; i++)
                        {
                            memcpy(dst + dstPitch * i,
                                (amf_uint8*)videoDataPtr + pitch * i,
                                pitch);
                        }
                    }
                    AMFPoint hotspot = { cursorData.GetHotspotX(), cursorData.GetHotspotY() };
                    AMFSize resolution = { cursorData.GetCaptureResolutionX(), cursorData.GetCaptureResolutionY() };
                    Cursor::Type type = cursorData.GetMonochrome() ? Cursor::Type::MONOCHROME : Cursor::Type::COLOR;

                    // consider passing pContext and cursorData to Cursor cosntructor the same way we do for VideoData
                    // instead of allocating memory in the transport class.
                    Cursor cursor(pSurface, hotspot, resolution, type);
                    m_clientInitParameters.GetCursorCallback()->OnCursorChanged(cursor);
                }
            }
        }
    }

    void ClientTransportImpl::OnAudioOutMessage(Session* session, const void* msg, size_t messageSize)
    {
        uint8_t opCode = *((const uint8_t*)msg);

        switch (AUDIO_OP_CODE(opCode))
        {
        case AUDIO_OP_CODE::INIT:
            OnAudioOutInit(session, msg, messageSize);
            break;
        case AUDIO_OP_CODE::DATA:
            OnAudioOutData(session, msg, messageSize);
            break;
        default:
            AMFTraceError(AMF_FACILITY, L"OnAudioOutMessage() - invalid opcode %d from %S", opCode, session->GetPeerUrl());
            break;
        }
    }


    void ClientTransportImpl::OnAudioOutInit(Session* /*session*/, const void* msg, size_t messageSize)
    {
        AudioInit audioInit;
        if (audioInit.ParseBuffer(msg, messageSize) == false)
        {
            AMFTraceError(AMF_FACILITY, L"OnAudioOutInit - invalid JSON: %S", msg);
        }
        else
        {
            InitID initID = audioInit.GetID();
            StreamID streamID = audioInit.GetStreamID();
            bool result = false;
            if (nullptr != m_clientInitParameters.GetAudioReceiverCallback())
            {
                std::string codecID = audioInit.GetCodecID();
                int32_t sampleRate = audioInit.GetSampleRate();
                int32_t channels = audioInit.GetChannels();
                int32_t layout = audioInit.GetChannelLayout();
                amf::AMF_AUDIO_FORMAT format = audioInit.GetFormat();

                size_t initBlockSize = strlen((char*)msg + 1) + 1;
                const void* initBlock = static_cast<const amf_uint8*>(msg) + initBlockSize + 1;


                result = m_clientInitParameters.GetAudioReceiverCallback()->OnAudioInit(codecID.c_str(), streamID, initID, channels, layout, sampleRate, format, initBlock, initBlockSize);
            }
            AudioInitAck audioInitAck(initID, result, streamID);
            SendMsg(Channel::AUDIO_OUT, audioInitAck.GetSendData(), audioInitAck.GetSendSize());
        }
    }

    void ClientTransportImpl::OnAudioOutData(Session* /*session*/, const void* msg, size_t messageSize)
    {
        amf::AMFContextPtr pContext = nullptr;
        {
            amf::AMFLock lock(&m_CCCGuard);
            pContext = m_pContext;
        }

        AudioData audioData;
        if (audioData.ParseBuffer(msg, messageSize) == false)
        {
            AMFTraceError(AMF_FACILITY, L"OnAudioOutData - invalid JSON: %S", msg);
        }
        else if (nullptr == pContext)
        {
            AMFTraceError(AMF_FACILITY, L"OnAudioOutData - Context is missing");
        }
        else if (nullptr != m_clientInitParameters.GetAudioReceiverCallback())
        {
            size_t messageLen = strlen((char*)msg + 1) + 1;
            const void* audioDataPtr = static_cast<const amf_uint8*>(msg) + messageLen + 1;
            size_t audioDataSize = messageSize - messageLen - 1;
            amf_size sizeToDecode = audioData.IsSizePresent() ? (amf_size)audioData.GetSize() : audioDataSize;

            amf::AMFBufferPtr pBuffer;
            if (AMF_OK != pContext->AllocBuffer(amf::AMF_MEMORY_HOST, sizeToDecode, &pBuffer))
            {
                AMFTraceError(AMF_FACILITY, L"AllocBuffer(%d) failed", (int)sizeToDecode);
                return;
            }

            memcpy(pBuffer->GetNative(), audioDataPtr, sizeToDecode);

            pBuffer->SetPts(audioData.GetPts());
            if (audioData.GetDuration() > 0)
            {
                pBuffer->SetDuration(audioData.GetDuration());
            }

            // consider passing m_pContext and AudioData to ReceivableAudioBuffer cosntructor the same way we do for VideoData
            // instead of allocating memory in the transport class.
            ReceivableAudioBuffer buffer(pBuffer, audioData.GetSequenceNumber(), audioData.GetDiscontinuity());

            StreamID streamID = audioData.GetStreamID();

            m_clientInitParameters.GetAudioReceiverCallback()->OnAudioBuffer(streamID, buffer);
        }
    }

    void ClientTransportImpl::OnAudioInMessage(Session* session, const void* msg, size_t /*messageSize*/)
    {
        // Todo later
        uint8_t opCode = *((const uint8_t*)msg);
        switch (AUDIO_OP_CODE(opCode))
        {
        case AUDIO_OP_CODE::INIT_REQUEST:
            break;// todo
        case AUDIO_OP_CODE::INIT_ACK:
            break;// todo
        default:
            AMFTraceInfo(AMF_FACILITY, L"OnAudioInMessage() - invalid opcode %d from %S", opCode, session->GetPeerUrl());
            break;
        }
    }

    void ClientTransportImpl::OnSensorsOutMessage(Session* session, const void* msg, size_t messageSize)
    {
        uint8_t opCode = *((const uint8_t*)msg);
        switch (SENSOR_OP_CODE(opCode))
        {
        case SENSOR_OP_CODE::DEVICE_EVENT:
        {
            DeviceEvent data;
            data.ParseBuffer(msg, messageSize);
            InputControllerCallback* pICCallback = m_clientInitParameters.GetInputControllerCallback();
            if (pICCallback != nullptr)
            {
                // Get controller id & event ids from DeviceEvent object (json: name, value)
                const DeviceEvent::DataCollection& coll = data.GetDataCollection();
                if (coll.size() > 0)
                {
                    const DeviceEvent::DataPair& dp = coll[0];

                    // Get control id
                    const char* controlID = dp.first.c_str();

                    // Get events
                    const DeviceEvent::Data& deData = dp.second;
                    for (size_t i = 0; i < deData.size(); i++)
                    {
                        // Get event
                        amf::AMFVariant event(AMFConstructFloatPoint2D(0, 0));
                        deData[i].GetVariantValue(event);

                        // Call controller input event callback
                        pICCallback->OnControllerInputEvent(controlID, event);
                    }
                }
            }
        }
            break;
        //case SENSOR_OP_CODE::TRACKABLE_DEVICE_CAPS:
        //    break;// todo
        //case SENSOR_OP_CODE::TRACKABLE_DEVICE_DISCONNECTED:
        //    break;// todo
        default:
            AMFTraceInfo(AMF_FACILITY, L"OnSensorsInMessage() - invalid opcode %d from %S", opCode, session->GetPeerUrl());
            break;
        }
    }

    void ClientTransportImpl::OnMiscOutMessage(Session* session, const void* msg, size_t /*messageSize*/)
    {
        uint8_t opCode = *((const uint8_t*)msg);
        AMFTraceInfo(AMF_FACILITY, L"OnMiscOutMessage() - mis message received with opcode %d from %S", opCode, session->GetPeerUrl());
    }

    void ClientTransportImpl::OnSensorsInMessage(Session* /*session*/, const void* /*msg*/, size_t /*messageSize*/)
    {
        // Todo later

        //uint8_t opCode = *((const uint8_t*)message);
        //switch (SENSOR_OP_CODE(opCode))
        //{
        //default:
        //    AMFTraceInfo(AMF_FACILITY, L"OnSensorsInMessage() - invalid opcode %d from %S", opCode, session->GetPeerUrl());
        //    break;
        //}
    }

    void ClientTransportImpl::OnUserDefinedMessage(Session* /*session*/, const void* msg, size_t messageSize)
    {
        if (nullptr != m_clientInitParameters.GetAppCallback())
        {
            m_clientInitParameters.GetAppCallback()->OnAppMessage(msg, messageSize);
        }
    }

    transport_common::Result ClientTransportImpl::SendMsg(Channel channel, const void* msg, size_t msgLen)
    {
        transport_common::Result result = transport_common::Result::FAIL;

        ClientSessionImpl::Ptr pSession = nullptr;
        {
            amf::AMFLock lock(&m_SessionGuard);
            pSession = m_pSession;
        }

        AMF_RETURN_IF_FALSE(nullptr != pSession, Result::FAIL, L"SendMsg(): transport contains nullptr session pointer");

        ssdk::util::AESPSKCipher::Ptr pCipher;
        amf::AMFContextPtr pContext;
        {
            amf::AMFLock lock(&m_CCCGuard);
            pCipher = m_pCipher;
            pContext = m_pContext;
        }

        if (nullptr == pCipher) // send unencoded message
        {
            result = pSession->Send(channel, static_cast<uint8_t*>(const_cast<void*>(msg)), msgLen);
        }
        else // encode and send message
        {
            size_t cipherTextBufferSize = pCipher->GetCipherTextBufferSize(msgLen);
            if (cipherTextBufferSize == 0)
            {
                AMFTraceError(AMF_FACILITY, L"SendMsg(): Failed to allocate a zero-length buffer, msgLen: %ld", msgLen);
            }
            else if (nullptr == pContext)
            {
                AMFTraceError(AMF_FACILITY, L"SendMsg(): Context is missing, can't allocate buffer");
            }
            else
            {
                AMF_RESULT res;
                amf::AMFBufferPtr pSendBuffer;
                res = pContext->AllocBuffer(amf::AMF_MEMORY_HOST, cipherTextBufferSize, &pSendBuffer);
                if (res != AMF_OK)
                {
                    AMFTraceError(AMF_FACILITY, L"SendMsg(): Failed to allocate a buffer size: %ld, msgLen: %ld", cipherTextBufferSize, msgLen);
                }
                else
                {
                    uint8_t* cipherText = static_cast<uint8_t*>(pSendBuffer->GetNative());
                    size_t cipherTextSize;
                    if (pCipher->Encrypt(msg, msgLen, cipherText, &cipherTextSize))
                    {
                        result = pSession->Send(channel, cipherText, cipherTextBufferSize);
                    }
                    else
                    {
                        AMFTraceError(AMF_FACILITY, L"SendMsg(): ANSCipher::Encrypt failed");
                        result = Result::INVALID_ARG;
                    }
                }
            }
        }

        return result;
    }

    Result ClientTransportImpl::SendVideoForceUpdate(StreamID streamID, uint64_t frameNumber, bool IDRframe)
    {
        Result result = Result::FAIL;
        amf_pts now = amf_high_precision_clock();
        
        // if this is a first frame, m_FrameLossInfoMap[streamID] will return default LastFrameInfo object with m_LastIDRRequestTime = 0
        if (now - m_FrameLossInfoMap[streamID].m_LastIDRRequestTime > 0.5 * AMF_SECOND)
        {
            VideoForceUpdate videoForceUpdate(streamID);
            result = SendMsg(Channel::VIDEO_OUT, videoForceUpdate.GetSendData(), videoForceUpdate.GetSendSize());
            AMFTraceInfo(AMF_FACILITY, L"Lost %S, video frame detected for StreamID %llu, (received %llu, expected: %llu);  Sending VideoForceUpdate message;  Time since last VideoForceUpdate message = %5.2f seconds",
                IDRframe ? "IDR" : "", streamID, frameNumber, m_FrameLossInfoMap[streamID].m_lastFrameNumbers, (float)(now - m_FrameLossInfoMap[streamID].m_LastIDRRequestTime) / (float)AMF_SECOND);
            
            m_FrameLossInfoMap[streamID].m_LastIDRRequestTime = now;
            m_FrameLossInfoMap[streamID].m_IDRRequestSent = true;
        }
        return result;
    }
}
