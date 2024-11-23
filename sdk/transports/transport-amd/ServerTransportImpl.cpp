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

Copyright Advanced Micro Devices, Inc. All rights reserved.

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

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#include "ServerTransportImpl.h"
#include "transports/transport-amd/messages/service/StartStop.h"
#include "transports/transport-amd/messages/service/Update.h"
#include "transports/transport-amd/messages/video/VideoInit.h"
#include "transports/transport-amd/messages/video/VideoData.h"
#include "transports/transport-amd/messages/audio/AudioInit.h"
#include "transports/transport-amd/messages/audio/AudioData.h"
#include "amf/public/common/TraceAdapter.h"
#include "amf/public/common/ByteArray.h"
#include "transports/transport-amd/messages/video/Cursor.h"
#include "controllers/UserInput.h"
#include "transports/transport-amd/messages/service/GenericMessage.h"
#include "sdk/video/Defines.h"
#include <sstream>
#include <chrono>

// Default parameter values
static constexpr const int64_t DATAGRAM_MSG_INTERVAL = 10;
static constexpr const int64_t DATAGRAM_LOST_MSG_THRESHOLD = 10;
static constexpr const int64_t DATAGRAM_TURNING_POINT_THRESHOLD = 20;
static constexpr const int64_t SUBSCRIBER_CONNECTION_TIMEOUT = 30;

namespace ssdk::transport_amd
{
    static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::transport_amd::ServerTransport";

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ServerInitParametersAmd implementation
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ServerTransportImpl::ServerInitParametersAmd::ServerInitParametersAmd()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ServerTransport interface implementation
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ServerTransportImpl::ServerTransportImpl() :
        m_SessionMonitor(*this)
    {
    }

    ServerTransportImpl::~ServerTransportImpl()
    {
    }

    Result ServerTransportImpl::Start(const ServerInitParameters& params)
    {
        AMF_RETURN_IF_FALSE(m_Initialized == false, Result::OK, L"Already Initialized");
        AMFTraceInfo(AMF_FACILITY, L"Initialize()");

        amf_increase_timer_precision();
        {
            amf::AMFLock lock(&m_Guard);
            m_Stop = false;
        }

        try
        {
            m_InitParams = dynamic_cast<const ServerInitParametersAmd&>(params);
        }
        catch (const std::bad_cast& ex)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to cast ServerInitParameters: %s", ex.what());
            Result::FAIL;
        }

        m_pContext = m_InitParams.GetContext();

        if (m_InitParams.GetNetwork())
        {
            m_pServer = new ServerImpl::Implementation();
            AMF_RETURN_IF_FALSE(m_pServer != nullptr, Result::FAIL, L"Failed to create Server");
            m_pServer->SetOptionProvider(this);
            m_pServer->SetServerTransport(this);
#ifdef WIN32
            WORD wVersionRequested;
            WSADATA wsaData;
            int err;

            // Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h
            wVersionRequested = MAKEWORD(2, 2);
            err = ::WSAStartup(wVersionRequested, &wsaData);
#endif
            std::stringstream url;
            switch (m_InitParams.GetNetworkType())
            {
            case ServerTransport::NETWORK_TYPE::NETWORK_UDP:
                url << "UDP://";
                break;
            case ServerTransport::NETWORK_TYPE::NETWORK_TCP:
                url << "TCP://";
                break;
            }
            url << m_InitParams.GetBindInterface().c_str() << ':';
            url << m_InitParams.GetPort();

            std::string hostName = m_InitParams.GetHostName();
            if (hostName.length() == 0)
            {
                char hostNameDefault[256];
                int hostNameLength = sizeof(hostNameDefault);
                ::gethostname(hostNameDefault, hostNameLength);
                hostName = hostNameDefault;
            }
            m_pServer->SetName(hostName.c_str());
            m_pServer->SetDatagramSize(m_InitParams.GetDatagramSize());
            m_pServer->SetProperty(DATAGRAM_MSG_INTERVAL, 10);
            m_pServer->SetProperty(DATAGRAM_LOST_MSG_THRESHOLD, 10);
            m_pServer->SetProperty(DATAGRAM_TURNING_POINT_THRESHOLD, 10);

            Result serverStatus = m_pServer->Activate(url.str().c_str(), this);
            AMF_RETURN_IF_FALSE(serverStatus == Result::OK, Result::FAIL, L"Unable to start Server Service, Acticate() failed err = %d", (int)serverStatus);
        }
        else
        {
            AMFTraceInfo(AMF_FACILITY, L"Server started in test mode");
        }

        m_InitParams.SetAppInitTime((int64_t)std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
        m_Initialized = true;

        return Result::OK;
    }

    Result ServerTransportImpl::Shutdown()
    {
        amf_pts start = amf_high_precision_clock();
        AMFTraceInfo(AMF_FACILITY, L"Stop() start");

        Server::Ptr pServer;
        Session::Ptr pDiscoverySession;
        amf::AMFContextPtr pContext;
        {
            amf::AMFLock lock(&m_Guard);
            if (m_Initialized == false)
            {
                return Result::OK;
            }
            m_Initialized = false;
            pServer = m_pServer;
            m_pServer = nullptr;
            pContext = m_pContext;
            m_pContext = nullptr;
            pDiscoverySession = m_pDiscoverySession;
            m_pDiscoverySession = nullptr;
            m_Stop = true;
        }

        if (pServer != nullptr)
        {
            pServer->Deactivate();
        }

        DisconnectAllSessions();

        if (m_SessionMonitor.IsRunning() == true)
        {
            m_SessionMonitor.RequestStop();
            m_SessionMonitor.WaitForStop();
        }

        pDiscoverySession = nullptr;
        pContext = nullptr;
        pServer = nullptr; // GK: Kill server only after all of the sessions are destroyed

        amf_pts end = amf_high_precision_clock();
        AMFTraceInfo(AMF_FACILITY, L"Stop() end call duration = %5.2f", (end - start) / 10000.);

        {
            amf::AMFLock lock(&m_Guard);
            m_Ciphers.clear();
        }

#ifdef WIN32
        ::WSACleanup();
#endif
        return Result::OK;
    }

    void ServerTransportImpl::OnServiceStart(Session* session, const void* msg, size_t len, Subscriber::Ptr pSubscriber)
    {
        if (session != nullptr && pSubscriber != nullptr)
        {
            StartRequest start;
            if (start.ParseBuffer(msg, len) == false)
            {
                AMFTraceError(AMF_FACILITY, L"OnServiceStart from %S - Invalid JSON", pSubscriber->GetSubscriberIPAddress());
            }
            else
            {
                pSubscriber->WaitForVideoInitAck(start.IsVideoInitAckRequired());
                pSubscriber->WaitForAudioInitAck(start.IsAudioInitAckRequired());

                VideoSenderCallback* pVSCallback = m_InitParams.GetVideoSenderCallback();
                AudioSenderCallback* pASCallback = m_InitParams.GetAudioSenderCallback();
                SessionHandle hSession = session->GetSessionHandle();
                if (pVSCallback != nullptr && start.GetVideoStreamID() != INVALID_STREAM)
                {
                    pVSCallback->OnVideoStreamSubscribed(hSession, start.GetVideoStreamID());
                }
                if (pASCallback != nullptr && start.GetAudioStreamID() != INVALID_STREAM)
                {
                    pASCallback->OnAudioStreamSubscribed(hSession, start.GetAudioStreamID());
                }

                // Start sensors thread in server
                ConnectionManagerCallback* pCMcallback = m_InitParams.GetConnectionManagerCallback();
                if (pCMcallback != nullptr)
                {
                    pCMcallback->OnClientSubscribed(hSession);
                }

                // To start sensors thread in old client
                GenericMessage message(START_SENSOR);
                pSubscriber->TransmitMessage(Channel::SERVICE, message.GetSendData(), message.GetSendSize());
            }
        }
    }

    void ServerTransportImpl::OnServiceStop(Session* session)
    {
        if (DeleteSubscriber(session, TerminationReason::DISCONNECT) == false)
        {
            AMFTraceError(AMF_FACILITY, L"OnServiceStop: DeleteSubscriber failed");
        }
        else
        {
            AMFTraceInfo(AMF_FACILITY, L"OnServiceStop: Client connection with %S terminated, reason: STOP command", session->GetPeerUrl());
        }
    }

    void ServerTransportImpl::OnServiceUpdate(Session* session, const void* msg, size_t len, Subscriber::Ptr pSubscriber)
    {
        if (session != nullptr && pSubscriber != nullptr)
        {
            VideoSenderCallback* pVSCallback = m_InitParams.GetVideoSenderCallback();
            if (pVSCallback != nullptr)
            {
                UpdateRequest request;
                if (request.ParseBuffer(msg, len) == false)
                {
                    AMFTraceError(AMF_FACILITY, L"On Service Update from %S - invalid JSON", pSubscriber->GetSubscriberIPAddress());
                }
                else
                {
                    amf::AMFLock lock(&m_Guard);
                    SessionHandle hSession = session->GetSessionHandle();
                    StreamID streamID = request.GetStreamID();

                    // Framerate change request
                    float frameRate = request.GetFrameRate();
                    if (frameRate != 0.0f && frameRate != m_CurentFrameRate)
                    {
                        pVSCallback->OnFramerateChangeRecieverRequest(hSession, streamID, frameRate);
                        m_CurentFrameRate = frameRate;
                    }

                    // Bitrate change request
                    int64_t bitrate = request.GetBitrate();
                    if (bitrate > 0 && bitrate != m_CurrentBitrate)
                    {
                        pVSCallback->OnBitrateChangeRecieverRequest(hSession, streamID, bitrate);
                        m_CurrentBitrate = bitrate;
                    }

                    // Resolution change request
                    AMFSize resolution = request.GetResolution();
                    if (resolution.width > 0 && resolution.height > 0 && resolution != m_CurrentResolution)
                    {
                        pVSCallback->OnResolutionChangeRecieverRequest(hSession, streamID, resolution);
                        m_CurrentResolution = resolution;
                    }
                }
            }
        }
    }

    void ServerTransportImpl::OnServiceStatLatency(Session* session, const void* msg, size_t len)
    {
        Subscriber::Ptr pSubscriber = FindSubscriber(session);

        Statistics request;
        if (request.ParseBuffer(msg, len) == false)
        {
            AMFTraceError(AMF_FACILITY, L"OnServiceStatLatency - invalid JSON: %S", msg);
        }
        else
        {
            if (nullptr != pSubscriber)
            {
                pSubscriber->UpdateStatsFromClient(request);
                amf::AMFPropertyStoragePtr statistics;
                pSubscriber->GetSessionStatistics(&statistics);

                VideoStatsCallback* pStatsCallback = m_InitParams.GetVideoStatsCallback();
                if (pStatsCallback != nullptr)
                {
                    VideoStatsCallback::Stats   stats;

                    statistics->GetProperty(STATISTICS_UPDATE_TIME, &stats.lastStatsTime);
                    statistics->GetProperty(STATISTICS_VIDEO_FPS_AT_RX, &stats.receiverFramerate);
                    statistics->GetProperty(STATISTICS_FORCE_IDR_REQ_COUNT, &stats.keyFrameReqCount);
                    statistics->GetProperty(STATISTICS_WORST_SEND_TIME, &stats.avgSendTime);
                    statistics->GetProperty(STATISTICS_DECODER_QUEUE_DEPTH, &stats.decoderQueueDepth);
                    statistics->GetProperty(STATISTICS_FULL_LATENCY, &stats.fullLatency);
                    statistics->GetProperty(STATISTICS_CLIENT_LATENCY, &stats.clientLatency);
                    statistics->GetProperty(STATISTICS_SERVER_LATENCY, &stats.serverLatency);
                    statistics->GetProperty(STATISTICS_ENCODER_LATENCY, &stats.encoderLatency);
                    statistics->GetProperty(STATISTICS_NETWORK_LATENCY, &stats.networkLatency);
                    statistics->GetProperty(STATISTICS_DECODER_LATENCY, &stats.decoderLatency);
                    statistics->GetProperty(STATISTICS_VIDEO_FPS_AT_RX, &stats.receiverFramerate);

                    pStatsCallback->OnVideoStats(session->GetSessionHandle(), request.GetStreamID(), stats);
                }
            }
        }
    }

    size_t ServerTransportImpl::GetActiveConnectionCount() const noexcept
    {
        amf::AMFLock lock(&m_Guard);
        return m_Subscribers.size();
    }

    bool ServerTransportImpl::IsSessionActive(SessionHandle session) const noexcept
    {
        Sessions::const_iterator it = m_Sessions.find(session);
        Subscriber::Ptr pSubscriber = (it != m_Sessions.end()) ? FindSubscriber(it->second) : nullptr;
        return (pSubscriber != nullptr) ? true : false;
    }

    Result ServerTransportImpl::Disconnect(SessionHandle session) noexcept
    {
        Result result = (DeleteSubscriber(m_Sessions[session], TerminationReason::DISCONNECT) == true) ? Result::OK : Result::FAIL;
        if (result == Result::OK)
        {
            amf::AMFLock lock(&m_Guard);
            m_Sessions.erase(session);
        }
        return result;
    }

    Result ServerTransportImpl::DisconnectAllSessions() noexcept
    {
        Result result = Result::OK;
        amf::AMFLock lock(&m_Guard);
        ConnectionManagerCallback* pCMCallback = m_InitParams.GetConnectionManagerCallback();
        for (Subscribers::iterator it = m_Subscribers.begin(); it != m_Subscribers.end(); it++)
        {
            if (pCMCallback != nullptr)
            {
                SessionHandle hSession = it->first->GetSessionHandle();
                pCMCallback->OnClientDisconnected(hSession, ConnectionManagerCallback::DisconnectReason::CLIENT_DISCONNECTED);
            }
        }
        m_Subscribers.clear();
        m_Sessions.clear();

        return result;
    }

    void ServerTransportImpl::StopStreaming()
    {
        AMFTraceInfo(AMF_FACILITY, L"StopStreaming start");
/*
        Subscribers subscribers;
        {
            amf::AMFLock lock(&m_Guard);
            subscribers = m_Subscribers;
            m_Subscribers.clear();
            m_Sessions.clear();
        }
*/
        {
            amf::AMFLock lock(&m_Guard);
            ConnectionManagerCallback* pCMCallback = m_InitParams.GetConnectionManagerCallback();
            for (Subscribers::iterator it = m_Subscribers.begin(); it != m_Subscribers.end(); it++)
            {
                if (pCMCallback != nullptr)
                {
                    SessionHandle hSession = it->first->GetSessionHandle();
                    pCMCallback->OnClientDisconnected(hSession, ConnectionManagerCallback::DisconnectReason::CLIENT_DISCONNECTED);
                }
            }
            m_Subscribers.clear();
            m_Sessions.clear();
        }
    }

    void ServerTransportImpl::OnServiceMessage(Session* session, uint8_t opcode, const void* msg, size_t len, Subscriber::Ptr pSubscriber)
    {
        if (pSubscriber != nullptr)
        {
            switch (SERVICE_OP_CODE(opcode))
            {
            case SERVICE_OP_CODE::START:
                AMFTraceDebug(AMF_FACILITY, L"OnServiceMessage - SERVICE_OP_CODE_START from %S at %S", pSubscriber->GetID(), pSubscriber->GetSubscriberIPAddress());
                OnServiceStart(session, msg, len, pSubscriber);
                break;
            case SERVICE_OP_CODE::STOP:
                AMFTraceDebug(AMF_FACILITY, L"OnServiceMessage - SERVICE_OP_CODE_STOP from %S at %S", pSubscriber->GetID(), pSubscriber->GetSubscriberIPAddress());
                OnServiceStop(session);
                break;
            case SERVICE_OP_CODE::UPDATE:
                AMFTraceDebug(AMF_FACILITY, L"OnServiceMessage - SERVICE_OP_CODE_UPDATE from %S at %S", pSubscriber->GetID(), pSubscriber->GetSubscriberIPAddress());
                OnServiceUpdate(session, msg, len, pSubscriber);
                break;
            case SERVICE_OP_CODE::STAT_LATENCY:
                OnServiceStatLatency(session, msg, len);
                break;
            case SERVICE_OP_CODE::FORCE_IDR: // legacy
                AMFTraceDebug(AMF_FACILITY, L"OnServiceMessage - VIDEO_OP_CODE_FORCE_IDR from %S at %S", pSubscriber->GetID(), pSubscriber->GetSubscriberIPAddress());
                ForceKeyFrame(session, msg, len, pSubscriber);
                break;
            case SERVICE_OP_CODE::TRACKABLE_DEVICE_CAPS:
                AMFTraceInfo(AMF_FACILITY, L"OnServiceMessage - SERVICE_OP_CODE_TRACKABLE_DEVICE_CAPS from %S at %S", pSubscriber->GetID(), pSubscriber->GetSubscriberIPAddress());
                break;
            default:
                AMFTraceError(AMF_FACILITY, L"OnServiceMessage - invalid opcode %d from %S at %S", opcode, pSubscriber->GetID(), pSubscriber->GetSubscriberIPAddress());
            }
        }
        else
        {
            AMFTraceError(AMF_FACILITY, L"OnServiceMessage - invalid session");
        }
    }

    void ServerTransportImpl::ForceKeyFrame(Session* session, const void* msg, size_t len, Subscriber::Ptr pSubscriber)
    {
        AMFTraceInfo(AMF_FACILITY, L"Key/IDR frame requested");

        if (session != nullptr)
        {
            VideoForceUpdate request;
            if (request.ParseBuffer(msg, len) == false)
            {
                AMFTraceError(AMF_FACILITY, L"ForceKeyFrame() - Invalid JSON, ignored");
            }
            else
            {
                VideoSenderCallback* pVSCallback = m_InitParams.GetVideoSenderCallback();
                Subscribers::const_iterator it = m_Subscribers.find(session);
                if (pVSCallback != nullptr && it != m_Subscribers.end())
                {
                    pVSCallback->OnForceUpdateRequest(request.GetStreamID());
                }
                if (pSubscriber != nullptr)
                {
                    pSubscriber->ForceIDRFrame();
                }
            }
        }

    }

    void ServerTransportImpl::TransmitMessageToAllSubscribers(Channel channel, const void* msg, size_t msgLen)
    {
        Subscribers subscribers;
        {
            amf::AMFLock lock(&m_Guard);
            subscribers = m_Subscribers;
        }
        for (Subscribers::iterator it = subscribers.begin(); it != subscribers.end(); it++)
        {
            it->second->TransmitMessage(channel, msg, msgLen);
        }
    }

    void ServerTransportImpl::OnSensorsInMessage(Session* session, uint8_t opcode, const void* msg, size_t len, Subscriber::Ptr pSubscriber)
    {
        if (pSubscriber != nullptr)
        {
            switch (SENSOR_OP_CODE(opcode))
            {
            case SENSOR_OP_CODE::DEVICE_EVENT:
            {
                amf_pts now = amf_high_precision_clock();
                //  SUBSCRIBER_ROLE_MASTER means this client/subscriber can drive video by submitting head poses, or at least the timestamps
                //  that are used for measuring latency. Ideally we should only have one master, but we cannot prevent multiple clients
                //  from declaring themselves as masters, but sensor timestamps should only be submitted by one client. This code allows
                //  only the first master in the list to submit pose timestamps, ignoring the rest
                bool thisUserDrivesVideo = (pSubscriber->GetRole() == transport_common::ServerTransport::ConnectionManagerCallback::ClientRole::CONTROLLER);
                {
                    amf::AMFLock lock(&m_Guard);
                    if (m_LastSensorTime != 0LL)
                    {
                        m_AverageSensorFreq += now - m_LastSensorTime;
                        if (m_SensorDataCount % 100 == 0)
                        {
                            // AMFTraceDebug(AMF_FACILITY, L"Average sensor freq=%5.2f proc=%5.2f", m_AverageSensorFreq / 100. / 10000., m_AverageSensorProcTime / 100. / 10000.);
                            m_AverageSensorFreq = 0;
                            m_AverageSensorProcTime = 0;
                        }
                    }
                    m_LastSensorTime = now;
                    m_SensorDataCount++;
                    if (thisUserDrivesVideo == true)
                    {
                        for (Subscribers::const_iterator it = m_Subscribers.begin(); it != m_Subscribers.end() && it->second != pSubscriber; ++it)
                        {
                            if (it->second->GetRole() == transport_common::ServerTransport::ConnectionManagerCallback::ClientRole::CONTROLLER)
                            {
                                thisUserDrivesVideo = false;
                                break;
                            }
                        }
                    }
                }

                DeviceEvent data;
                data.ParseBuffer(msg, len);
                {
                    if (thisUserDrivesVideo == true)
                    {
                        InputControllerCallback* pICCallback = m_InitParams.GetInputControllerCallback();
                        if (pICCallback != nullptr)
                        {
                            // Get controller id & event ids from DeviceEvent object (json: name, value)
                            const DeviceEvent::DataCollection& coll = data.GetDataCollection();
                            if (coll.size() > 0)
                            {
                                const DeviceEvent::DataPair& dp = coll[0];

                                // Get control id
                                const char* controlID = dp.first.c_str();
                                std::string hmdHeadPoseID = ssdk::ctls::DEVICE_HMD;
                                hmdHeadPoseID += ssdk::ctls::DEVICE_POSE;

                                // event with control ID Device headpose contains origin time that is used to track turnaround latency
                                if (hmdHeadPoseID.compare(controlID) == 0)
                                {
                                    amf_pts originPts;
                                    if (dp.second[0].GetTimestampValue(originPts))
                                    {
                                        VideoStatsCallback* pStatsCallback = m_InitParams.GetVideoStatsCallback();
                                        if (pStatsCallback != nullptr)
                                        {
                                            pStatsCallback->OnOriginPts(session->GetSessionHandle(), data.GetStreamID(), originPts);
                                        }
                                    }
                                }

                                // Get event ids
                                const DeviceEvent::Data& deData = dp.second;
                                for (size_t i = 0; i < deData.size(); i++)
                                {
                                    // Get event
                                    amf::AMFVariant event;
                                    event.type = pICCallback->GetExpectedEventDataType(controlID);
                                    if (event.type != amf::AMF_VARIANT_TYPE::AMF_VARIANT_EMPTY)
                                    {
                                        deData[i].GetVariantValue(event);

                                        ssdk::ctls::CtlEvent ctlEvent;
                                        ctlEvent.value = event;
                                        deData[i].GetFlagsValue(ctlEvent.flags);

                                        // Call controller input event callback
                                        pICCallback->OnControllerInputEvent(session->GetSessionHandle(), controlID, ctlEvent);
                                    }
                                }
                            }
                        }
                    }
                }

                pSubscriber->OnEvent(data, len);
                m_AverageSensorProcTime += amf_high_precision_clock() - now;
            }

                break;
            case SENSOR_OP_CODE::TRACKABLE_DEVICE_DISCONNECTED:
                // Trackable device will be covered later
                break;
            case SENSOR_OP_CODE::TRACKABLE_DEVICE_CAPS:
                // Trackable device will be covered later
                break;
            default:
                AMFTraceError(AMF_FACILITY, L"OnSensorsInMessage - invalid opcode %d from %S at %S", opcode, pSubscriber->GetID(), pSubscriber->GetSubscriberIPAddress());
                break;
            }
        }
        else
        {
            AMFTraceError(AMF_FACILITY, L"On Sensors In Message - invalid session");
        }
    }

    void ServerTransportImpl::OnVideoOutMessage(Session* session, uint8_t opcode, const void* msg, size_t len, Subscriber::Ptr pSubscriber)
    {
        if (pSubscriber != nullptr)
        {
            VideoSenderCallback* pVSCallback = m_InitParams.GetVideoSenderCallback();
            switch (VIDEO_OP_CODE(opcode))
            {
            case VIDEO_OP_CODE::INIT_ACK:
            {
                // Call OnVideoBeginStreaming() when init_ack received from all subscribers
                VideoInitAck data;
                if (data.ParseBuffer(msg, len) == false)
                {
                    AMFTraceError(AMF_FACILITY, L"OnVideoOutMessage::INIT_ACK - Invalid JSON");
                }
                else if (data.GetAck() == true)
                {
                    pSubscriber->SetLastVideoInitAckReceived(data.GetID());
                    if (pVSCallback != nullptr)
                    {
                        pVSCallback->OnReadyToReceiveVideo(session->GetSessionHandle(), data.GetStreamID(), data.GetID());
                    }
                    AMFTraceInfo(AMF_FACILITY, L"OnVideoOutMessage - VIDEO_OP_CODE INIT_ACK received from client %S at %S for extra data ID %lld", pSubscriber->GetID(), pSubscriber->GetSubscriberIPAddress(), data.GetID());
                }
                else
                {
                    AMFTraceInfo(AMF_FACILITY, L"OnVideoOutMessage - VIDEO_OP_CODE INIT_NACK received from client %S at %S for extra data ID %lld - codec or resolution is not supported by the client", pSubscriber->GetID(), pSubscriber->GetSubscriberIPAddress(), data.GetID());
                }
            }
            break;
            case VIDEO_OP_CODE::FORCE_UPDATE:
                AMFTraceInfo(AMF_FACILITY, L"OnVideoOutMessage - VIDEO_OP_CODE FORCE_UPDATE received from client %S at %S", pSubscriber->GetID(), pSubscriber->GetSubscriberIPAddress());
                ForceKeyFrame(session, msg, len, pSubscriber);
                break;
            case VIDEO_OP_CODE::INIT_REQUEST:
                {
                    VideoInitRequest request;
                    if (request.ParseBuffer(msg, len) == false)
                    {
                        AMFTraceError(AMF_FACILITY, L"OnVideoOutMessage::INIT_REQUEST - Invalid JSON");
                    }
                    else
                    {
                        AMFTraceInfo(AMF_FACILITY, L"OnVideoOutMessage - VIDEO_OP_CODE INIT_REQUEST received from client %S at %S", pSubscriber->GetID(), pSubscriber->GetSubscriberIPAddress());
                        // Send extradata to client from SendVideoInit(). Application calls SendVideoInit() when receives OnVideoRequestInit() callback
                        if (pVSCallback != nullptr)
                        {
                            pVSCallback->OnVideoRequestInit(session->GetSessionHandle(), request.GetStreamID());
                        }
                    }
                }
                break;
            default:
                AMFTraceError(AMF_FACILITY, L"OnVideoOutMessage - invalid opcode %d from %S at %S", opcode, pSubscriber->GetID(), pSubscriber->GetSubscriberIPAddress());
                break;
            }
        }
        else
        {
            AMFTraceError(AMF_FACILITY, L"OnVideoOutMessage(): pSubscriber == nullptr");
        }
    }

    void ServerTransportImpl::OnAudioOutMessage(Session* session, uint8_t opcode, const void* msg, size_t len, Subscriber::Ptr pSubscriber)
    {
        AudioSenderCallback* pASCallback = m_InitParams.GetAudioSenderCallback();
        if (pSubscriber != nullptr)
        {
            switch (AUDIO_OP_CODE(opcode))
            {
            case AUDIO_OP_CODE::INIT_ACK:
            {
                // Call OnAudioBeginStreaming() when init_ack received from all subscribers
                AudioInitAck data;
                if (data.ParseBuffer(msg, len) == false)
                {
                    AMFTraceError(AMF_FACILITY, L"OnAudioOutMessage::INIT_ACK - Invalid JSON");
                }
                else if (data.GetAck() == true)
                {
                    pSubscriber->SetLastAudioInitAckReceived(data.GetID());
                    if (pASCallback != nullptr)
                    {
                        pASCallback->OnReadyToReceiveAudio(session->GetSessionHandle(), data.GetStreamID(), data.GetID());
                    }
                    AMFTraceInfo(AMF_FACILITY, L"OnAudioOutMessage - AUDIO_OP_CODE_INIT_ACK received from client %S at %S for extra data ID %lld", pSubscriber->GetID(), pSubscriber->GetSubscriberIPAddress(), data.GetID());
                }
                else
                {
                    AMFTraceInfo(AMF_FACILITY, L"OnAudioOutMessage - AUDIO_OP_CODE_INIT_NACK received from client %S at %S for extra data ID %lld - codec or resolution is not supported by the client", pSubscriber->GetID(), pSubscriber->GetSubscriberIPAddress(), data.GetID());
                }
            }
                break;
            case AUDIO_OP_CODE::INIT_REQUEST:
                {
                    AMFTraceInfo(AMF_FACILITY, L"OnAudioOutMessage - AUDIO_OP_CODE_INIT_REQUEST");
                    AudioInitRequest request;
                    if (request.ParseBuffer(msg, len) == false)
                    {
                        AMFTraceError(AMF_FACILITY, L"OnVideoOutMessage::INIT_REQUEST - Invalid JSON");
                    }
                    else
                    {
                        AMFTraceInfo(AMF_FACILITY, L"OnVideoOutMessage - VIDEO_OP_CODE INIT_REQUEST received from client %S at %S", pSubscriber->GetID(), pSubscriber->GetSubscriberIPAddress());
                        // Send extradata to client from SendVideoInit(). Application calls SendVideoInit() when receives OnVideoRequestInit() callback
                        if (pASCallback != nullptr)
                        {
                            pASCallback->OnAudioRequestInit(session->GetSessionHandle(), request.GetStreamID());
                        }
                    }
                }
                break;
            default:
                AMFTraceError(AMF_FACILITY, L"OnAudioOutMessage - invalid opcode %d from %S at %S", opcode, pSubscriber->GetID(), pSubscriber->GetSubscriberIPAddress());
                break;
            }
        }
    }

    Result ServerTransportImpl::SendVideoInit(SessionHandle session, const char* codec, StreamID streamID, InitID initID, const AMFSize& streamResolution,
        const AMFRect& viewport, uint32_t bitDepth, bool stereoscopic, bool foveated, const void* initBlock, size_t initBlockSize)
    {
        Result result;
        if (initID == -1 || codec == nullptr)
        {
            AMFTraceError(AMF_FACILITY, L"Invalid init block");
            result = Result::FAIL;
        }
        else
        {
            // Send extradata to client. Application calls SendVideoInit() when receives OnVideoRequestInit() callback
            VideoInit videoInit(initID, codec, streamResolution.width, streamResolution.height, viewport, foveated, bitDepth, streamID);
            AMFByteArray videoExtradata;
            videoExtradata.SetSize(videoInit.GetSendSize() + 1 + initBlockSize);
            uint8_t* data = videoExtradata.GetData();
            memcpy(data, videoInit.GetSendData(), videoInit.GetSendSize());
            data += videoInit.GetSendSize();
            *data++ = 0;
            if (initBlock != nullptr)
            {
                memcpy(data, initBlock, initBlockSize);
            }

            Subscriber::Ptr pSubscriber;
            {
                amf::AMFLock lock(&m_Guard);
                pSubscriber = FindSubscriber(m_Sessions[session]);
            }
            if (pSubscriber != nullptr)
            {
                pSubscriber->SetVideoInitSentTime(amf_high_precision_clock());
                pSubscriber->WaitForIDR(true);
                pSubscriber->SetEncoderStereo(stereoscopic);
                pSubscriber->TransmitMessage(Channel::VIDEO_OUT, videoExtradata.GetData(), videoExtradata.GetSize());
                AMFTraceInfo(AMF_FACILITY, L"SendVideoInit: Sent video init block to client %S at %S for stream %lld, init ID %lld", pSubscriber->GetID(), pSubscriber->GetSubscriberIPAddress(), streamID, initID);
                VideoSenderCallback* pVSCallback = m_InitParams.GetVideoSenderCallback();
                if (pSubscriber->WaitForVideoInitAck() == false && pVSCallback != nullptr)
                {
                    pVSCallback->OnReadyToReceiveVideo(session, streamID, initID);
                }
            }
            result = Result::OK;
        }
        return result;
    }

    Result ServerTransportImpl::SendVideoFrame(SessionHandle session, StreamID /*streamID*/, const TransmittableVideoFrame& frame)
    {
        // Calculate frame buffer
        size_t frameBufSize = frame.CalculateRequiredBufferSize();

        // Create video data
        amf_pts pts = frame.GetPts();
        amf_pts originPts = frame.GetOriginPts();
        amf_pts ptsServerLatency = 0, ptsEncoderLatency = 0, ptsLastSendDuration = 0;

        { // Todo: move this elsewhere, ServerTransportImpl should not depend on the application;  Don't forget to remove Defines.h include
            size_t subframeCnt = frame.GetSubframeCount();
            if (subframeCnt > 0)
            {
                amf::AMFBufferPtr inputFrame;
                frame.GetSubframeBuffer(0, &inputFrame);
                inputFrame->GetProperty(ssdk::video::VIDEO_ENCODER_LATENCY_PTS, &ptsEncoderLatency);
                inputFrame->GetProperty(ssdk::video::VIDEO_IN_PTS, &ptsServerLatency);
                ptsServerLatency = amf_high_precision_clock() - ptsServerLatency;
            }
        }

        uint32_t compressedFrameSize = uint32_t(frameBufSize);
        VideoFrame::ViewType eViewType = frame.GetViewType();
        VideoFrame::SubframeType eSubframeType = (frame.GetSubframeCount() > 0) ? frame.GetSubframeType(0) : VideoFrame::SubframeType::UNKNOWN;
        uint64_t uiFrameNum = frame.GetSequenceNumber();
        bool discontinuity = frame.IsDiscontinuity();
        VideoData videoData(pts, originPts, ptsServerLatency, ptsEncoderLatency, compressedFrameSize, eViewType, eSubframeType, ptsLastSendDuration, uiFrameNum, discontinuity);

        // Copy video data to buffer
        AMFByteArray bufToSend;
        bufToSend.SetSize(videoData.GetSendSize() + 1 + frameBufSize);
        amf_uint8* dataPtr = bufToSend.GetData();
        memcpy(dataPtr, videoData.GetSendData(), videoData.GetSendSize());

        // Shift the point to add frame buffer
        dataPtr += videoData.GetSendSize();
        *dataPtr++ = 0;

        // Construct & add frame buffer
        frame.ConstructFrame(dataPtr);

        // Find subscriber and send video data to client
        Result result = Result::FAIL;
        Subscriber::Ptr pSubscriber = FindSubscriber(m_Sessions[session]);
        amf::AMFLock lock(&m_Guard);
        if (pSubscriber != nullptr)
        {
            result = pSubscriber->TransmitMessage(Channel::VIDEO_OUT, bufToSend.GetData(), bufToSend.GetSize());
        }

        return result;
    }

    Result ServerTransportImpl::SendAudioInit(SessionHandle session, const char* codec, StreamID streamID, InitID initID, uint32_t channels,
                                              uint32_t layout, amf::AMF_AUDIO_FORMAT format, uint32_t samplingRate, const void* initBlock, size_t initBlockSize)
    {
        // Send extradata to client. Application calls SendAudioInit() when receives OnAudioRequestInit() callback
        AudioInit audioInit(initID, codec, format, samplingRate, channels, layout, streamID);
        AMFByteArray audioExtradata;
        audioExtradata.SetSize(audioInit.GetSendSize() + 1 + initBlockSize);
        amf_uint8* data = audioExtradata.GetData();
        memcpy(data, audioInit.GetSendData(), audioInit.GetSendSize());
        data += audioInit.GetSendSize();
        *data++ = 0;
        if (initBlock != nullptr)
        {
            memcpy(data, initBlock, initBlockSize);
        }

        Subscriber::Ptr pSubscriber;
        {
            amf::AMFLock lock(&m_Guard);
            pSubscriber = FindSubscriber(m_Sessions[session]);
        }
        if (pSubscriber != nullptr)
        {
            pSubscriber->SetAudioInitSentTime(amf_high_precision_clock());
            pSubscriber->TransmitMessage(Channel::AUDIO_OUT, audioExtradata.GetData(), audioExtradata.GetSize());
            AMFTraceInfo(AMF_FACILITY, L"OnAudioExtraData: Sent audio init block to client %S at %S for stream %lld, init ID %lld", pSubscriber->GetID(), pSubscriber->GetSubscriberIPAddress(), streamID, initID);
            AudioSenderCallback* pASCallback = m_InitParams.GetAudioSenderCallback();
            if (pSubscriber->WaitForAudioInitAck() == false && pASCallback != nullptr)
            {
                pASCallback->OnReadyToReceiveAudio(session, streamID, initID);
            }
        }

        return Result::OK;
    }

    Result ServerTransportImpl::SendAudioBuffer(SessionHandle session, StreamID streamID, const TransmittableAudioBuffer& buf)
    {
        // Get audio buffer
        amf::AMFBufferPtr buffer;
        if (const_cast<TransmittableAudioBuffer&>(buf).GetBuffer(&buffer) != AMF_RESULT::AMF_OK)
        {
            return Result::FAIL;
        }

        // Create audio data
        amf_pts pts = buf.GetPts();
        amf_pts duration = buf.GetDuration();
        size_t bufSize = buffer->GetSize();
        int64_t sequenceNumber = buf.GetSequenceNumber();
        bool discontinuity = buf.IsDiscontinuity();
        AudioData audioData(pts, duration, uint32_t(bufSize), sequenceNumber, discontinuity, streamID);

        // Copy audio data to buffer
        AMFByteArray bufToSend;
        bufToSend.SetSize(audioData.GetSendSize() + 1 + bufSize);
        amf_uint8* dataPtr = bufToSend.GetData();
        memcpy(dataPtr, audioData.GetSendData(), audioData.GetSendSize());

        // Shift the point to add audio buffer
        dataPtr += audioData.GetSendSize();
        *dataPtr++ = 0;

        // Add audio buffer
        memcpy(dataPtr, buffer->GetNative(), bufSize);

        // Find subscriber and send audio data to client
        Result result = Result::FAIL;
        Subscriber::Ptr pSubscriber = FindSubscriber(m_Sessions[session]);
        amf::AMFLock lock(&m_Guard);
        if (pSubscriber != nullptr)
        {
            result = pSubscriber->TransmitMessage(Channel::AUDIO_OUT, bufToSend.GetData(), bufToSend.GetSize());
        }

        return result;
    }

    Result ServerTransportImpl::SendControllerEvent(const char* controlID, const ssdk::ctls::CtlEvent* pEvent)
    {
        Result result = Result::FAIL;
        Subscribers subscribers;
        {
            amf::AMFLock lock(&m_Guard);
            subscribers = m_Subscribers;
        }
        for (Subscribers::iterator it = subscribers.begin(); it != subscribers.end(); it++)
        {
            result = it->second->SendEvent(controlID, pEvent);
        }
        return result;
    }

    Result ServerTransportImpl::SetCursor(const Cursor& cursor)
    {
        AMFByteArray buff;

        bool visible = false;
        bool monochrome = false;
        int width = 0;
        int height = 0;
        int pitch = 0;
        AMFPoint hs = {};
        AMFSize screenSize = { 0, 0 };

        // if null is passed in as bitmap, it means the cursor is invisible
        amf::AMFSurfacePtr pCursorBitmap;
        if (const_cast<Cursor&>(cursor).GetBitmap(&pCursorBitmap) == AMF_OK && nullptr != pCursorBitmap)
        {
            width = pCursorBitmap->GetPlaneAt(0)->GetWidth();
            height = pCursorBitmap->GetPlaneAt(0)->GetHeight();
            hs = cursor.GetHotspot();
            visible = true;

            amf::AMFPlane* p = pCursorBitmap->GetPlaneAt(0);
            pitch = p->GetWidth() * p->GetPixelSizeInBytes();

            monochrome = (cursor.GetType() == Cursor::Type::MONOCHROME);
            screenSize = cursor.GetServerDisplayResolution();
        }

        {
            amf::AMFLock lock(&m_Guard);

            CursorData cursorData(width, height, pitch, hs.x, hs.y, screenSize.width, screenSize.height, visible, monochrome);

            int bitmapSize = pitch * height;
            buff.SetSize(cursorData.GetSendSize() + 1 + bitmapSize);
            amf_uint8* dataPtr = buff.GetData();
            memcpy(dataPtr, cursorData.GetSendData(), cursorData.GetSendSize());

            dataPtr += cursorData.GetSendSize();
            *dataPtr++ = 0;

            if (bitmapSize > 0)
            {
                amf::AMFPlane* p = pCursorBitmap->GetPlaneAt(0);
                for (int i = 0; i < p->GetHeight(); i++)
                {
                    memcpy(dataPtr + (i * pitch), (amf_uint8*)p->GetNative() + (i * p->GetHPitch()), pitch);
                }
            }
        }
        TransmitMessageToAllSubscribers(Channel::VIDEO_OUT, buff.GetData(), buff.GetSize());

        return Result::OK;
    }

    Result ServerTransportImpl::MoveCursor(float x, float y)
    {
        ssdk::ctls::CtlEvent ev = {};
        std::string id = std::string(ssdk::ctls::DEVICE_MOUSE) + std::string(ssdk::ctls::DEVICE_MOUSE_POS);
        ev.value.type = amf::AMF_VARIANT_FLOAT_POINT2D;
        ev.value.floatPoint2DValue.x = x;
        ev.value.floatPoint2DValue.y = y;
        ev.flags = ssdk::ctls::DEVICE_MOUSE_POS_ABSOLUTE;

        SendControllerEvent(id.c_str(), &ev);

        return Result::OK;
    }

    Result ServerTransportImpl::HideCursor()
    {
        amf::AMFLock lock(&m_Guard);
        CursorData cursorData(0, 0, 0, 0, 0, 0, 0, false, false);

        AMFByteArray buff;
        buff.SetSize(cursorData.GetSendSize());
        amf_uint8* dataPtr = buff.GetData();
        memcpy(dataPtr, cursorData.GetSendData(), cursorData.GetSendSize());

        TransmitMessageToAllSubscribers(Channel::VIDEO_OUT, buff.GetData(), buff.GetSize());

        return Result::OK;
    }

    void ServerTransportImpl::ProcessMessage(Session* session, Channel channel, int msgID, const void* message, size_t messageSize, Subscriber::Ptr pSubscriber)
    {
        amf_pts now = amf_high_precision_clock();
        amf_pts decryptionTime = 0;
        {
            amf::AMFLock lock(&m_Guard);
            MsgID2TimeMap::iterator it = m_DecryptorSubmissionTimestamps.find(msgID);
            if (it != m_DecryptorSubmissionTimestamps.end())
            {
                decryptionTime = now - it->second;
                m_DecryptorSubmissionTimestamps.erase(it);
            }
        }
        if (pSubscriber != nullptr)
        {
            pSubscriber->AddDecryptionTime(decryptionTime);
        }

        uint8_t opcode = *((const uint8_t*)message);

        // All encrypted message will have a json following the opcode, the first character
        // will always be '{', use this as an extra signature check. If the password doesn't
        // match, decryption process should get something different.
        uint8_t signature = 0;
        if (messageSize > 1)
        {
            signature = *(((const uint8_t*)message) + 1);
        }
        if ('{' == signature || messageSize == 1)
        {
            switch (channel)
            {
            case Channel::SERVICE:
                OnServiceMessage(session, opcode, message, messageSize, pSubscriber);
                break;
            case Channel::SENSORS_IN:
                OnSensorsInMessage(session, opcode, message, messageSize, pSubscriber);
                break;
            case Channel::AUDIO_IN:
                if (pSubscriber != nullptr)
                {
                    pSubscriber->OnAudioInMessage(opcode, message, messageSize, session->GetSessionHandle(), m_InitParams.GetAudioReceiverCallback());
                }
                break;
            case Channel::VIDEO_OUT:
                OnVideoOutMessage(session, opcode, message, messageSize, pSubscriber);
                break;
            case Channel::AUDIO_OUT:
                OnAudioOutMessage(session, opcode, message, messageSize, pSubscriber);
                break;
            case Channel::USER_DEFINED:
            {
                if (pSubscriber != nullptr)
                {
                    pSubscriber->OnUserDefinedMessage(message, messageSize);
                }

                ApplicationCallback* pACallback = m_InitParams.GetAppCallback();
                if (pACallback != nullptr)
                {
                    pACallback->OnAppMessage(session->GetSessionHandle(), message, messageSize);
                }
            }
            break;
            default:
                AMFTraceError(AMF_FACILITY, L"ProcessMessage - invalid channel (%d)", (int)channel);
                break;
            }
        }
        else
        {
            AMFTraceError(AMF_FACILITY, L"ProcessMessage - password signature check failed. channel=%d, opcode=%d, messageSize=%d", (int)channel, (int)opcode, (int)messageSize);
        }
    }

    Result ServerTransportImpl::SendApplicationMessage(SessionHandle session, const void* msg, size_t size)
    {
        // Send message to client
        Result result = Result::FAIL;
        Subscriber::Ptr pSubscriber = FindSubscriber(m_Sessions[session]);
        amf::AMFLock lock(&m_Guard);
        if (pSubscriber != nullptr)
        {
            result = pSubscriber->TransmitMessage(Channel::USER_DEFINED, msg, size);
            AMFTraceInfo(AMF_FACILITY, L"SendApplicationMessage: Sent message Channel::USER_DEFINED to client %S at %S", pSubscriber->GetID(), pSubscriber->GetSubscriberIPAddress());
        }

        return result;
    }

    Result ServerTransportImpl::SetSessionSecurity(SessionHandle session, const char* cipherPassphrase)
    {
        amf::AMFLock lock(&m_Guard);

        if (cipherPassphrase != nullptr && m_Ciphers.find(session) == m_Ciphers.end())
        {
            m_Ciphers[session] = SessionSecurityParams(ssdk::util::AESPSKCipher::Ptr(new ssdk::util::AESPSKCipher(cipherPassphrase)));

            if (m_SessionMonitor.IsRunning() == false)
            {
                m_SessionMonitor.Start();
            }
        }
        return Result::OK;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReceiverCallback interface
    void ServerTransportImpl::OnMessageReceived(Session* session, Channel channel, int msgID, const void* message, size_t messageSize)
    {
        // If Shutdown() has been called, just return
        {
            amf::AMFLock lock(&m_Guard);
            if (m_Stop == true)
            {
                return;
            }
        }

        Subscriber::Ptr pSubscriber = FindSubscriber(session);

        // A Subscriber with nullptr id means AuthorizeConnectionFromDevice() hasn't been called
        // for current session. We don't process any message before that. Otherwise
        // encrypted messages are treated as if they're unencrypted, eventually crash the server.
        if (pSubscriber != nullptr && pSubscriber->GetID() != nullptr)
        {
            ssdk::util::AESPSKCipher::Ptr pCipher = nullptr;
            pSubscriber->GetCipher(&pCipher);
            amf::AMFContextPtr pContext;
            {
                amf::AMFLock lock(&m_Guard);
                pContext = m_pContext;
                m_DecryptorSubmissionTimestamps[msgID] = amf_high_precision_clock();
            }
            if (pCipher != nullptr && pContext != nullptr)
            {
                Decrypt(pContext, pCipher, session, channel, msgID, message, messageSize, pSubscriber);
            }
            else
            {
                ProcessMessage(session, channel, msgID, message, messageSize, pSubscriber);
            }
        }
    }

    void ServerTransportImpl::OnTerminate(Session* session, TerminationReason reason)
    {
        if (DeleteSubscriber(session, reason) == true)
        {
            AMFTraceInfo(AMF_FACILITY, L"Client connection from %S terminated, reason: %s", session->GetPeerUrl(), reason == TerminationReason::TIMEOUT ? L"Timeout" : L"Disconnected");
        }
        else
        {
            AMFTraceDebug(AMF_FACILITY, L"OnTerminate: DeleteSubscriber failed");
        }
    }

    /*  Legacy options structure:
    {
        "DatagramSize":1472,
        "MaxDatagramSize":65507,
        "Options":
        {
            "AudioStreams":
            [
                {"Bitrate":256000,"Bits":16,"Channels":2,"Codec":"AAC","Description":"Desktop audio, stereo, 44100Hz, AAC, 256000bps","Layout":3,"SamplingRate":44100,"Source":0}
            ],
            "Cipher":false,
            "Codecs":
            {
                "AudioCodecs":
                {
                    "AvailableCodecs":
                    [
                        {"Bitrate":256000,"Bits":16,"Channels":2,"Codec":"AAC","Layout":3,"SampleRate":44100}
                    ],
                    "Codec":
                    [
                        {"Bitrate":256000,"Bits":16,"Channels":2,"Codec":"AAC","Layout":3,"SampleRate":44100}
                    ],
                    "ServerCodecs":
                    [
                        {"Bitrate":256000,"Bits":16,"Channels":2,"Codec":"AAC","Layout":3,"SampleRate":44100}
                    ]
                },
                "VideoCodecs":
                {
                    "AvailableCodecs":
                    [
                        {"Bitrate":50000000,"Bpp":8,"Codec":"hevc","Framerate":60,"Resolution":[1920,1080]}
                    ],
                    "Codec":
                    [
                        {"Bitrate":50000000,"Bpp":8,"Codec":"hevc","Framerate":60,"Resolution":[1920,1080]}
                    ],
                    "ServerCodecs":
                    [
                        {"Bitrate":50000000,"Bpp":8,"Codec":"hevc","Framerate":60,"Resolution":[1920,1080]}
                    ]
                }
            },
            "EncoderSize":[1920,1080],
            "VideoStreams":
            [
                {"Bitrate":50000000,"Bits":8,"Codec":"hevc","Description":"Monitor 0, (1920x1080@60Hz, SDR)","Framerate":60,"Resolution":[1920,1080],"Source":0}
            ]
        },
        "Port":1235,
        "ProtocolMinVersion":3,
        "ProtocolVersion":3,
        "ServerName":"RemoteGameServer",
        "Transports":["UDP"]
    }

    */
    void ServerTransportImpl::FillLegacyOptions(amf::JSONParser* parser, HelloResponse::Options* options)
    {
        amf::JSONParser::Node::Ptr codecs;
        parser->CreateNode(&codecs);

        amf::JSONParser::Array::Ptr defaultStreamVideoCodec;
        parser->CreateArray(&defaultStreamVideoCodec);
        const transport_common::ServerTransport::ServerInitParameters::VideoStreams& videoStreams = m_InitParams.GetVideoStreams();
        if (videoStreams.size() > 0)
        {
            amf::JSONParser::Node::Ptr videoCodecs;
            parser->CreateNode(&videoCodecs);

            amf::JSONParser::Array::Ptr codecsArray;
            parser->CreateArray(&codecsArray);

            amf::JSONParser::Node::Ptr defaultStreamCodec;
            parser->CreateNode(&defaultStreamCodec);
            amf::SetStringValue(parser, defaultStreamCodec, "Codec", videoStreams[0].GetCodec());
            amf::SetSizeValue(parser, defaultStreamCodec, "Resolution", videoStreams[0].GetResolution());
            amf::SetFloatValue(parser, defaultStreamCodec, "Framerate", videoStreams[0].GetFramerate());
            amf::SetInt32Value(parser, defaultStreamCodec, "Bitrate", videoStreams[0].GetBitrate());
            amf::SetInt32Value(parser, defaultStreamCodec, "Bpp", videoStreams[0].GetColorDepth() == transport_common::VideoStreamDescriptor::ColorDepth::HDR_10 ? 10 : 8);

            codecsArray->AddElement(defaultStreamCodec);
            videoCodecs->AddElement("Codec", codecsArray);
            videoCodecs->AddElement("AvailableCodecs", codecsArray);
            videoCodecs->AddElement("ServerCodecs", codecsArray);
            codecs->AddElement("VideoCodecs", videoCodecs);

            amf::JSONParser::Array::Ptr defaultStreamResolution;
            amf::JSONParser::Value::Ptr defaultStreamWidth, defaultStreamHeight;
            parser->CreateValue(&defaultStreamWidth);
            defaultStreamWidth->SetValueAsInt32(videoStreams[0].GetResolution().width);
            parser->CreateValue(&defaultStreamHeight);
            defaultStreamHeight->SetValueAsInt32(videoStreams[0].GetResolution().height);
            parser->CreateArray(&defaultStreamResolution);
            defaultStreamResolution->AddElement(defaultStreamWidth);
            defaultStreamResolution->AddElement(defaultStreamHeight);
            options->SetArray("EncoderSize", defaultStreamResolution);
        }

        amf::JSONParser::Array::Ptr defaultStreamAudioCodec;
        parser->CreateArray(&defaultStreamAudioCodec);
        const transport_common::ServerTransport::ServerInitParameters::AudioStreams& audioStreams = m_InitParams.GetAudioStreams();
        if (audioStreams.size() > 0)
        {
            amf::JSONParser::Node::Ptr audioCodecs;
            parser->CreateNode(&audioCodecs);

            amf::JSONParser::Array::Ptr codecsArray;
            parser->CreateArray(&codecsArray);

            amf::JSONParser::Node::Ptr defaultStreamCodec;
            parser->CreateNode(&defaultStreamCodec);
            amf::SetStringValue(parser, defaultStreamCodec, "Codec", audioStreams[0].GetCodec());
            amf::SetInt32Value(parser, defaultStreamCodec, "SampleRate", audioStreams[0].GetSamplingRate());
            amf::SetInt32Value(parser, defaultStreamCodec, "Channels", audioStreams[0].GetNumOfChannels());
            amf::SetInt32Value(parser, defaultStreamCodec, "Layout", audioStreams[0].GetLayout());
            amf::SetInt32Value(parser, defaultStreamCodec, "Bitrate", audioStreams[0].GetBitrate());
            amf::SetInt32Value(parser, defaultStreamCodec, "Bits", audioStreams[0].GetBitsPerSample());

            codecsArray->AddElement(defaultStreamCodec);
            audioCodecs->AddElement("Codec", codecsArray);
            audioCodecs->AddElement("AvailableCodecs", codecsArray);
            audioCodecs->AddElement("ServerCodecs", codecsArray);
            codecs->AddElement("AudioCodecs", audioCodecs);
        }
        options->SetNode("Codecs", codecs);
    }

    void ServerTransportImpl::FillStreamDeclarations(amf::JSONParser* parser, HelloResponse::Options* options)
    {
        const transport_common::ServerTransport::ServerInitParameters::VideoStreams& videoStreams = m_InitParams.GetVideoStreams();
        if (videoStreams.size() > 0)
        {
            amf::JSONParser::Array::Ptr streams;
            parser->CreateArray(&streams);
            for (auto it : videoStreams)
            {
                amf::JSONParser::Node::Ptr stream;
                parser->CreateNode(&stream);
                amf::SetStringValue(parser, stream, "Codec", it.GetCodec());
                amf::SetStringValue(parser, stream, "Description", it.GetDescription());
                amf::SetInt32Value(parser, stream, "Source", it.GetSourceID());
                amf::SetInt64Value(parser, stream, "Bitrate", it.GetBitrate());
                amf::SetSizeValue(parser, stream, "Resolution", it.GetResolution());
                amf::SetFloatValue(parser, stream, "Framerate", it.GetFramerate());
                amf::SetInt32Value(parser, stream, "Bits", it.GetColorDepth() == VideoStreamDescriptor::ColorDepth::HDR_10 ? 10 : 8);

                streams->AddElement(stream);
            }
            options->SetArray("VideoStreams", streams);
        }
        const transport_common::ServerTransport::ServerInitParameters::AudioStreams& audioStreams = m_InitParams.GetAudioStreams();
        if (audioStreams.size() > 0)
        {
            amf::JSONParser::Array::Ptr streams;
            parser->CreateArray(&streams);
            for (auto it : audioStreams)
            {
                amf::JSONParser::Node::Ptr stream;
                parser->CreateNode(&stream);
                amf::SetStringValue(parser, stream, "Codec", it.GetCodec());
                amf::SetStringValue(parser, stream, "Description", it.GetDescription());
                amf::SetInt32Value(parser, stream, "Source", it.GetSourceID());
                amf::SetInt64Value(parser, stream, "Bitrate", it.GetBitrate());
                amf::SetInt32Value(parser, stream, "Channels", it.GetNumOfChannels());
                amf::SetInt32Value(parser, stream, "Layout", it.GetLayout());
                amf::SetInt32Value(parser, stream, "SamplingRate", it.GetSamplingRate());
                amf::SetInt32Value(parser, stream, "Bits", it.GetBitsPerSample());

                streams->AddElement(stream);
            }
            options->SetArray("AudioStreams", streams);
        }
    }

    transport_common::Result ServerTransportImpl::OnFillOptions(bool /*discovery*/, Session* /*session*/, HelloResponse::Options* options)
    {
        amf::JSONParser::Ptr parser;
        CreateJSONParser(&parser);

        amf::JSONParser::Node::Ptr codecs;
        parser->CreateNode(&codecs);

        amf::JSONParser::Array::Ptr defaultStreamVideoCodec;
        parser->CreateArray(&defaultStreamVideoCodec);
        FillLegacyOptions(parser, options);
        FillStreamDeclarations(parser, options);

        options->SetBool("Cipher", m_Ciphers.size() > 0);

        return Result::OK;
    }

    transport_common::Result ServerTransportImpl::OnDiscovery(Session* /*session*/)
    {
        //todo
        return Result::OK;
    }

    transport_common::Result ServerTransportImpl::OnClientConnected(Session* /*session*/)
    {
        //todo
        return Result::OK;
    }

    bool ServerTransportImpl::AuthorizeDiscoveryRequest(Session* session, const char* deviceID)
    {
        bool result = true;
        ConnectionManagerCallback* cb = m_InitParams.GetConnectionManagerCallback();
        if (cb != nullptr)
        {
            result = cb->OnDiscoveryRequest(deviceID) == ConnectionManagerCallback::ClientAction::ACCEPT;
            AMFTraceInfo(AMF_FACILITY, L"Discovery request from client ID %S at %S, platform %S was %s by server", deviceID, session->GetPeerUrl(), session->GetPeerPlatform(), result ? L"accepted" : L"rejected");
        }
        else
        {
            AMFTraceInfo(AMF_FACILITY, L"Discovery request from client ID %S at %S, platform %S was not handled by server and accepted by default", deviceID, session->GetPeerUrl(), session->GetPeerPlatform());
        }
        return result;
    }

    bool ServerTransportImpl::AuthorizeConnectionRequest(Session* session, const char* deviceID)
    {
        bool result = true;
        ConnectionManagerCallback* cb = m_InitParams.GetConnectionManagerCallback();
        if (cb != nullptr)
        {
            Subscriber::Ptr pSubscriber = FindSubscriber(session);
            if (pSubscriber == nullptr)
            {
                AMFTraceDebug(AMF_FACILITY, L"AuthorizeConnectionRequest: Subscriber not found");
            }
            else
            {
                pSubscriber->SetID(deviceID);
                result = cb->OnConnectionRequest(session->GetSessionHandle(), deviceID, pSubscriber->GetRole()) == ConnectionManagerCallback::ClientAction::ACCEPT;
                AMFTraceInfo(AMF_FACILITY, L"Connection request from client ID %S at %S, platform %S was %s by server", deviceID, session->GetPeerUrl(), session->GetPeerPlatform(), result ? L"accepted" : L"rejected");
            }
        }
        else
        {
            AMFTraceInfo(AMF_FACILITY, L"Connection request from client ID %S at %S, platform %S was not handled by server and accepted by default", deviceID, session->GetPeerUrl(), session->GetPeerPlatform());
        }
        return result;
    }


    bool ServerTransportImpl::Decrypt(amf::AMFContextPtr pContext, ssdk::util::AESPSKCipher::Ptr pCipher, Session* session, Channel channel, int msgID,
                                      const void* message, size_t messageSize, Subscriber::Ptr pSubscriber)
    {
        bool result = true;
        amf_pts completionTime;
        amf_pts submissionTime = amf_high_precision_clock();

        amf::AMFBufferPtr clearTextBuffer;

        size_t clearTextOfs = 0;
        size_t clearTextBufferSize = pCipher->GetClearTextBufferSize(messageSize);
        AMF_RESULT res = pContext->AllocBuffer(amf::AMF_MEMORY_HOST, clearTextBufferSize, &clearTextBuffer);
        if (res != AMF_OK)
        {
            AMFTraceError(L"Decryptor", L"OnMessageReceived()  - failed to allocate a buffer for decryption");
        }
        else
        {
            size_t clearTextSize;
            bool bSuccess = pCipher->Decrypt(message, messageSize, clearTextBuffer->GetNative(), &clearTextOfs, &clearTextSize);
            if (bSuccess != true)
            {
                AMFTraceError(L"Decryptor", L"OnMessageReceived()  - decryption failed");
            }
            // If password doesn't match, clearTextSize should be a meaningless number that's likely
            // larger than clearTextBufferSize. In the rare case where size get decrypted into
            // a number that's within the reasonable range, client and server would do an 8 bit
            // signature check again.
            else if (clearTextSize <= clearTextBufferSize)
            {
                const uint8_t* messagePtr = static_cast<const uint8_t*>(clearTextBuffer->GetNative());
                messagePtr += clearTextOfs;
                completionTime = amf_high_precision_clock();
                ProcessMessage(session, channel, msgID, messagePtr, clearTextSize, pSubscriber);
            }
        }

        submissionTime; completionTime;

        return result;
    }

    void ServerTransportImpl::NewClientSessionCreated(Session* session) noexcept
    {
        const std::string sPassphrase = m_InitParams.GetPassphrase();
        if (sPassphrase != "")
        {
            SetSessionSecurity(session->GetSessionHandle(), sPassphrase.c_str());
        }

        CreateSubscriber(session);
    }

    bool ServerTransportImpl::CreateSubscriber(Session* session)
    {
        bool result = false;
        if (session != nullptr)
        {
            if (FindSubscriber(session) == nullptr)
            {
                Subscriber::Ptr pSubscriber(new Subscriber(session, m_pContext));
                pSubscriber->SetCipher(FindCipherForSession(session->GetSessionHandle()));

                result = AddSubscriber(session, pSubscriber);
                session->RegisterReceiverCallback(this);
            }
        }
        return result;
    }

    bool ServerTransportImpl::AddSubscriber(Session* session, Subscriber::Ptr pSubscriber)
    {
        amf::AMFLock lock(&m_Guard);
        Subscribers::iterator it = m_Subscribers.find(session);
        if (it != m_Subscribers.end())
        {
            return false;
        }
        m_Subscribers[session] = pSubscriber;
        m_Sessions[session->GetSessionHandle()] = session;
        return true;
    }

    Subscriber::Ptr ServerTransportImpl::FindSubscriber(Session* session) const
    {
        amf::AMFLock lock(&m_Guard);
        Subscribers::const_iterator it = m_Subscribers.find(session);
        if (it != m_Subscribers.end())
        {
            return it->second;
        }
        return nullptr;
    }
    //-------------------------------------------------------------------------------------------------
    bool ServerTransportImpl::DeleteSubscriber(Session* session, TerminationReason reason)
    {
        amf::AMFLock lock(&m_Guard);

        Subscribers::iterator it = m_Subscribers.find(session);
        if (it != m_Subscribers.end())
        {
            Subscriber::Ptr pSubscriber = it->second;
            m_Subscribers.erase(it);

            ConnectionManagerCallback* pCMCallback = m_InitParams.GetConnectionManagerCallback();
            SessionHandle hSession = session->GetSessionHandle();
            if (pCMCallback != nullptr)
            {
                pCMCallback->OnClientDisconnected(hSession, reason == TerminationReason::DISCONNECT ? ConnectionManagerCallback::DisconnectReason::CLIENT_DISCONNECTED : ConnectionManagerCallback::DisconnectReason::TIMEOUT);
            }
            return true;
        }

        return false;
    }

    ssdk::util::AESPSKCipher::Ptr ServerTransportImpl::FindCipherForSession(SessionHandle session)
    {
        ssdk::util::AESPSKCipher::Ptr pCipher = nullptr;
        amf::AMFLock lock(&m_Guard);
        SessionToCipherMap::iterator it = m_Ciphers.find(session);
        if (it != m_Ciphers.end())
        {
            pCipher = it->second.GetCipher();
        }
        return pCipher;
    }

    //-------------------------------------------------------------------------------------------------
    void ServerTransportImpl::CleanupExpiredSessionIDs()
    {
        time_t now = time(nullptr);
        amf::AMFLock lock(&m_Guard);
        for (SessionToCipherMap::iterator it = m_Ciphers.begin(); it != m_Ciphers.end(); ++it)
        {
            if (it->second.GetCreationTime() - now > SUBSCRIBER_CONNECTION_TIMEOUT)
            {
                AMFTraceInfo(AMF_FACILITY, L"Stale session %d has expired", it->first);
                m_Ciphers.erase(it);
                it = m_Ciphers.begin();
            }
        }
    }
    //-------------------------------------------------------------------------------------------------
    void ServerTransportImpl::ExpiredSessionMonitor::Run()
    {
        while (StopRequested() == true)
        {
            m_Object.CleanupExpiredSessionIDs();
            amf_sleep(50);
        }
    }

} // namespace ssdk::transport_amd
