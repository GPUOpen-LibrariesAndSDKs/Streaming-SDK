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

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#include "TransportServerImpl.h"
#include "FlowCtrlProtocol.h"
#include "ServerSessionImpl.h"
#include "Misc.h"
#include "amf/public/common/TraceAdapter.h"

#include <iostream>

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::transport_amd::ServerImpl";

static constexpr const time_t DISCONNECT_TIMEOUT = 5;
static constexpr const size_t MAX_CONCURRENT_CONNECTIONS = 10;


namespace ssdk::transport_amd
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Server properties
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Datagram messages monitoring
    const wchar_t* DATAGRAM_MSG_INTERVAL = L"DGramInterval";                    // amf_int64; default = 10; the interval in seconds for monitoring lost messages due to UDP Datagram size limitations
    const wchar_t* DATAGRAM_LOST_MSG_THRESHOLD = L"DGramLostMsgCountThreshold"; // amf_int64; default = 10; the interval in seconds for monitoring lost messages due to UDP Datagram size limitations
    const wchar_t* DATAGRAM_TURNING_POINT_THRESHOLD = L"DGramDecisionThreshold";// amf_int64; default = 10; the interval in seconds for monitoring lost messages due to UDP Datagram size limitations

    //----------------------------------------------------------------------------------------------
    // Statistics properties
    //----------------------------------------------------------------------------------------------
    const wchar_t* STATISTICS_FULL_LATENCY              = L"FullLatency";          // amf_float; complete latency for the video frame, from captured on server to presented on client (ms)
    const wchar_t* STATISTICS_SERVER_LATENCY            = L"ServerLatency";        // amf_float; complete latency on the server side (ms)
    const wchar_t* STATISTICS_ENCODER_LATENCY           = L"EncoderLatency";       // amf_float; latency added by encoding the video frame (ms)
    const wchar_t* STATISTICS_CLIENT_LATENCY            = L"ClientLatency";        // amf_float; complete latency on the client side (ms)
    const wchar_t* STATISTICS_NETWORK_LATENCY           = L"NetworkLatency";       // amf_float; latency added by network transport (ms)
    const wchar_t* STATISTICS_DECODER_LATENCY           = L"DecoderLatency";       // amf_float; latency added by decoding on video frame (ms)
    const wchar_t* STATISTICS_ENCODER_QUEUE_DEPTH       = L"EncoderQueueDepth";    // amf_int64; the number of frames pending in the encoder queue on the server
    const wchar_t* STATISTICS_DECODER_QUEUE_DEPTH       = L"DecoderQueueDepth";    // amf_int64; the number of frames pending in the decoder queue on the client
    const wchar_t* STATISTICS_ENCRYPTION_LATENCY        = L"EncryptionLatency";    // amf_float; latency added by encryption (ms)
    const wchar_t* STATISTICS_DECRYPTION_LATENCY        = L"DecryptionLatency";    // amf_float; latency added by decryption (ms)
    const wchar_t* STATISTICS_CLIENT_ENCRYPTION_LATENCY = L"ClEncryptionLatency";  // amf_float; latency added by encryption on the client (ms)
    const wchar_t* STATISTICS_CLIENT_DECRYPTION_LATENCY = L"ClDecryptionLatency";  // amf_float; latency added by decryption on the client (ms)
    const wchar_t* STATISTICS_UPDATE_TIME               = L"StatsUpdateTime";      // amf_pts; time of the last update in 100s of ns

    const wchar_t* STATISTICS_BANDWIDTH_VIDEO_OUT       = L"BandwidthVideoOut";    // amf_float; video out bandwidth (bps)
    const wchar_t* STATISTICS_BANDWIDTH_VIDEO_IN        = L"BandwidthVideoIn";     // amf_float; video in bandwidth (bps)
    const wchar_t* STATISTICS_VIDEO_SEND_TIME           = L"VideoSendTime";        // amf_float; average time in send_to in ms
    const wchar_t* STATISTICS_VIDEO_FPS_AT_TX           = L"VideoFpsTx";           // amf_float; real fps measured at transmission point
    const wchar_t* STATISTICS_VIDEO_FPS_AT_RX           = L"VideoFpsRx";           // amf_float; real fps measured at receive point
    const wchar_t* STATISTICS_BANDWIDTH_AUDIO_OUT       = L"BandwidthAudioOut";    // amf_float; audio out bandwidth (bps)
    const wchar_t* STATISTICS_BANDWIDTH_AUDIO_IN        = L"BandwidthAudioIn";     // amf_float; audio in bandwidth (bps)
    const wchar_t* STATISTICS_AUDIO_SEND_TIME           = L"AudioSendTime";        // amf_float; average time in send_to in ms
    const wchar_t* STATISTICS_BANDWIDTH_CTRL_OUT        = L"BandwidthCtrlOut";     // amf_float; sensors out bandwidth (bps)
    const wchar_t* STATISTICS_BANDWIDTH_CTRL_IN         = L"BandwidthCtrlIn";      // amf_float; controllers/kbd/mouse/touchscreen bandwidth (bps)
    const wchar_t* STATISTICS_CTRL_SEND_TIME            = L"CtrlSendTime";         // amf_float; average time in send_to in ms
    const wchar_t* STATISTICS_BANDWIDTH_USER_OUT        = L"BandwidthUserOut";     // amf_float; user-defined messages out bandwidth (bps)
    const wchar_t* STATISTICS_BANDWIDTH_USER_IN         = L"BandwidthUserIn";      // amf_float; user-defined messages in bandwidth (bps)
    const wchar_t* STATISTICS_USER_SEND_TIME            = L"UserSendTime";         // amf_float; average time in send_to in ms
    const wchar_t* STATISTICS_TOTAL_SEND_TIME           = L"TotalSendTime";        // amf_float; average time in send_to in ms
    const wchar_t* STATISTICS_BANDWIDTH_TOTAL_OUT       = L"BandwidthTotalOut";    // amf_float; total messages out bandwidth (bps)
    const wchar_t* STATISTICS_BANDWIDTH_TOTAL_IN        = L"BandwidthTotalIn";     // amf_float; total messages in bandwidth (bps)
    const wchar_t* STATISTICS_BANDWIDTH_ESTIMATE        = L"BandwidthEstimate";    // amf_float; currently used total network bandwidth (bps)
    const wchar_t* STATISTICS_FORCE_IDR_REQ_COUNT       = L"ForceIDRReqCnt";       // amf_int64; count of ForceIDR requests since last statistics
    const wchar_t* STATISTICS_SLOW_SEND_COUNT           = L"SlowSendCnt";          // amf_int64; count of sends that took longer than 50ms
    const wchar_t* STATISTICS_WORST_SEND_TIME           = L"WorstSendTime";        // amf_float; worst send time in ms

    const wchar_t* STATISTICS_AV_DESYNC                 = L"AVDesync";             // amf_float; average audio-video desync (video-audio) in ms

    const wchar_t* STATISTICS_LOCAL_UPDATE_TIME         = L"StatsLocalUpdateTime";  // amf_pts; time of the last update in 100s of ns

    const wchar_t* STATISTICS_TICK_PERIOD               = L"StatsTickPeriod";      // ANSClient only. amf_pts. Controls the minimum period of ticks being sent to the client. Default: 0 (once a ms)

    const wchar_t* STATISTICS_SERVER_GPU_CLK            = L"ServerGpuClk";         // amf_float; current server gpu core clock (mhz)
    const wchar_t* STATISTICS_SERVER_GPU_USAGE          = L"ServerGpuUsage";       // amf_float; current server gpu utilisation in percentage (%)
    const wchar_t* STATISTICS_SERVER_GPU_TEMP           = L"ServerGpuTemp";        // amf_float; current server gpu core temperature (C)
    const wchar_t* STATISTICS_SERVER_CPU_CLK            = L"ServerCpuClk";         // amf_float; current server cpu core clock (MHz)
    const wchar_t* STATISTICS_SERVER_CPU_TEMP           = L"ServerCpuTemp";        // amf_float; current server cpu core temperature (C)
    const wchar_t* SERVER_STATISTICS_PERIOD             = L"ServerStatisticsPeriod";// amf_pts; default = 3 seconds. the length of time period that server gpu statistics are sent


    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ServerImpl::ServerImpl() :
        m_Connected(false),
        m_Port(DEFAULT_PORT),	//	Default port
        m_MaxFragmentSize(FlowCtrlProtocol::MAX_DATAGRAM_SIZE),
        m_pOptionsProvider(nullptr),
        m_PairingModeEnabled(false)
    {
    }

    transport_common::Result AMF_STD_CALL ServerImpl::SetName(const char* name)
    {
        transport_common::Result result = transport_common::Result::OK;
        if (name == nullptr)
        {
            result = transport_common::Result::INVALID_ARG;
        }
        else
        {
            amf::AMFLock lock(&m_CritSect);
            m_Name = name;
        }
        return result;
    }

    const std::string& ServerImpl::GetName() const noexcept
    {
        return m_Name;
    }

    uint16_t ServerImpl::GetPort() const noexcept
    {
        return m_Port;
    }

    transport_common::Result	AMF_STD_CALL ServerImpl::SetDatagramSize(size_t size)
    {
        transport_common::Result result = transport_common::Result::OK;
        amf::AMFLock lock(&m_CritSect);
        if (m_UDPServer != nullptr && m_UDPServer->IsServerRunning() == true)
        {
            result = transport_common::Result::CANT_SET_WHILE_RUNNING;
        }
        else
        {
            m_MaxFragmentSize = size;
        }
        return result;
    }

    size_t ServerImpl::GetDatagramSize() const throw()
    {
        return m_MaxFragmentSize;
    }

    //  Run the server in a thread
    transport_common::Result AMF_STD_CALL ServerImpl::Activate(const char* url, OnClientConnectCallback* connectCallback)
    {
        transport_common::Result result = transport_common::Result::OK;

        amf::AMFLock lock(&m_CritSect);
        if ((m_UDPServer != nullptr && m_UDPServer->IsServerRunning() == true) ||
            (m_TCPServer != nullptr && m_TCPServer->IsServerRunning() == true))
        {
            result = transport_common::Result::ALREADY_RUNNING;
            AMFTraceWarning(AMF_FACILITY, L"StartService() already running");
        }
        else
        {
            m_pConnectCallback = connectCallback;

            net::Url urlTemp(url, "udp", m_Port);
            m_Port = urlTemp.GetPort();

            m_UDPServer = UDPServer::Ptr(new UDPServer(url, m_Port, m_MaxFragmentSize, *this/*, connectCallback*/));
            result = m_UDPServer->StartServer();
            if (result != transport_common::Result::OK)
            {
                AMFTraceError(AMF_FACILITY, L"StartService() Failed to start UDP server");
                m_pConnectCallback = nullptr;
            }
            else if (urlTemp.GetProtocol() == "TCP" || urlTemp.GetProtocol() == "tcp")
            {
                m_TCPServer = TCPServer::Ptr(new TCPServer(url, m_Port, m_MaxFragmentSize, *this/*, connectCallback*/));
                result = m_TCPServer->StartServer();
                if (result != transport_common::Result::OK)
                {
                    AMFTraceError(AMF_FACILITY, L"StartService() Failed to start TCP server. url=%S", urlTemp.GetUrl().c_str());
                    m_TCPServer = nullptr;
                    m_UDPServer = nullptr;
                    m_pConnectCallback = nullptr;
                }
            }
            if (result == transport_common::Result::OK)
            {
                AMFTraceInfo(AMF_FACILITY, L"StartService() Network services started. url=%S", urlTemp.GetUrl().c_str());
            }
        }

        return result;
    }

    transport_common::Result AMF_STD_CALL ServerImpl::Deactivate()
    {
        transport_common::Result result = transport_common::Result::NOT_RUNNING;
        {
            amf::AMFLock    lock(&m_CritSect);
            if ((m_UDPServer != nullptr && m_UDPServer->IsServerRunning() == true) ||
                (m_TCPServer != nullptr && m_TCPServer->IsServerRunning() == true))
            {
                result = transport_common::Result::OK;
            }
        }
        if (result == transport_common::Result::OK)
        {
            if (m_UDPServer != nullptr)
            {
                AMFTraceInfo(AMF_FACILITY, L"StopService() UDP server shutdown");
                m_UDPServer->StopServer();
                m_UDPServer = nullptr;
            }
            if (m_TCPServer != nullptr)
            {
                AMFTraceInfo(AMF_FACILITY, L"StopService() TCP server shutdown");
                m_TCPServer->StopServer();
                m_TCPServer = nullptr;
            }
            amf::AMFLock    lock(&m_CritSect);
            m_pConnectCallback = nullptr;
        }
        return result;
    }

    bool ServerImpl::IsConnected() const noexcept
    {
        amf::AMFLock    lock(&m_CritSect);
        return m_Connected;
    }

    void ServerImpl::SetConnected(bool val) noexcept
    {
        amf::AMFLock    lock(&m_CritSect);
        m_Connected = val;
    }

    transport_common::Result     ServerImpl::SetOptionProvider(OnFillOptionsCallback* provider)
    {
        amf::AMFLock    lock(&m_CritSect);
        m_pOptionsProvider = provider;
        return transport_common::Result::OK;
    }

    void ServerImpl::SetSessionTimeoutEnabled(bool timeoutEnabled)
    {
        if (m_UDPServer != nullptr)
        {
            m_UDPServer->SetSessionTimeoutEnabled(timeoutEnabled);
        }
        if (m_TCPServer != nullptr)
        {
            m_TCPServer->SetSessionTimeoutEnabled(timeoutEnabled);
        }
    }

    void ServerImpl::FillOptions(bool discovery, Session* session, HelloResponse::Options* options)
    {
        if (m_pOptionsProvider != nullptr && options->IsPresent() == true)
        {
            m_pOptionsProvider->OnFillOptions(discovery, session, options);
        }
    }

    bool ServerImpl::AuthorizeDiscoveryRequest(Session* session, const std::string& deviceID)
    {
        amf::AMFLock lock(&m_CritSect);
        return m_pConnectCallback->AuthorizeDiscoveryRequest(session, deviceID.empty() == false ? deviceID.c_str() : nullptr);
    }

    bool ServerImpl::AuthorizeConnectionRequest(Session* session, const std::string& deviceID)
    {
        amf::AMFLock lock(&m_CritSect);
        return m_pConnectCallback->AuthorizeConnectionRequest(session, deviceID.empty() == false ? deviceID.c_str() : nullptr);
    }
}