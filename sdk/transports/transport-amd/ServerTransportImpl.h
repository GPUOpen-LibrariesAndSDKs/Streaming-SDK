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

#pragma once

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#include "transports/transport-common/ServerTransport.h"
#include "TransportSession.h"
#include "amf/public/common/Thread.h"
#include "TransportServerImpl.h"
#include "Subscriber.h"

#include <unordered_map>

namespace ssdk::transport_amd
{
    using namespace ssdk::transport_common;

    class ServerTransportImpl :
        public ServerTransport,
        public ReceiverCallback,
        public OnFillOptionsCallback,
        public OnClientConnectCallback
    {
    public:
        //  Transport-specific server parameters:
        class ServerInitParametersAmd : public ServerTransport::ServerInitParameters
        {
        public:
            ServerInitParametersAmd();
            virtual ~ServerInitParametersAmd() = default;

            inline const amf::AMFContextPtr GetContext() { return m_pContext; };
            void SetContext(amf::AMFContextPtr pContext) { m_pContext = pContext; };

            inline const bool GetNetwork() { return m_bNetwork; };
            void SetNetwork(bool bNetwork) { m_bNetwork = bNetwork; };

            inline const ServerTransport::NETWORK_TYPE GetNetworkType() { return m_NetworkType; };
            void SetNetworkType(ServerTransport::NETWORK_TYPE networkType) { m_NetworkType = networkType; };

            inline const int64_t GetPort() { return m_Port; };
            void SetPort(int64_t port) { m_Port = port; };

            inline const int64_t GetDatagramSize() { return m_DatagramSize; };
            void SetDatagramSize(int64_t datagramSize) { m_DatagramSize = datagramSize; };

            inline const std::string GetBindInterface() { return m_BindInterface; };
            void SetBindInterface(const std::string& bindInterface) { m_BindInterface = bindInterface; };

            inline const std::string GetHostName() { return m_HostName; };
            void SetHostName(const std::string& hostName) { m_HostName = hostName; };

            inline const int64_t GetAppInitTime() { return m_AppInitTime; };
            void SetAppInitTime(int64_t initTime) { m_AppInitTime = initTime; };

            inline const std::string GetPassphrase() const noexcept { return m_cipherPassphrase; }
            inline void SetPassphrase(const std::string& cipherPassphrase) noexcept { m_cipherPassphrase = cipherPassphrase; }

        protected:
            amf::AMFContextPtr  m_pContext;
            bool                m_bNetwork{ true };
            ServerTransport::NETWORK_TYPE m_NetworkType{ ServerTransport::NETWORK_TYPE::NETWORK_UDP };
            int64_t             m_Port{ 1235 };
            int64_t             m_DatagramSize{ 65507 };
            std::string         m_BindInterface;
            std::string         m_HostName{ "" };
            int64_t             m_AppInitTime{};
            std::string         m_cipherPassphrase;
        };

        ServerTransportImpl();
        virtual ~ServerTransportImpl();

    public:
        //  Start/stop:
        virtual Result Start(const ServerInitParameters& params) override;                                  // Start the server with specified parameters
        virtual Result Shutdown() override;                                                                 // Shutdown the server

        //  Session management:
        virtual size_t GetActiveConnectionCount() const noexcept override;                                  // Query the number of active sessions
        virtual bool   IsSessionActive(SessionHandle session) const noexcept override;                      // Check if a session identified by SessionHandle is connected
        virtual Result Disconnect(SessionHandle session) noexcept override;                                 // Forcefully disconnect a specific session
        virtual Result DisconnectAllSessions() noexcept override;                                           // Forcefully disconnect all sessions

        void    NewClientSessionCreated(Session* session) noexcept;                                         // Called when new client session is created

        //  Send various types of messages:
        //  Video:
        virtual Result SendVideoInit(SessionHandle session, const char* codec, StreamID streamID, InitID initID, const AMFSize& streamResolution,
                                     const AMFRect& viewport, uint32_t bitDepth, bool stereoscopic, bool foveated, const void* initBlock, size_t initBlockSize) override;
        virtual Result SendVideoFrame(SessionHandle session, StreamID streamID, const TransmittableVideoFrame& frame) override;

        //  Audio:
        virtual Result SendAudioInit(SessionHandle session, const char* codec, StreamID streamID, InitID initID, uint32_t channels,
                                     uint32_t layout, amf::AMF_AUDIO_FORMAT format, uint32_t samplingRate, const void* initBlock, size_t initBlockSize) override;
        virtual Result SendAudioBuffer(SessionHandle session, StreamID streamID, const TransmittableAudioBuffer& buf) override;

        //  Controllers:
        virtual Result SendControllerEvent(const char* controlID, const ssdk::ctls::CtlEvent* pEvent) override;

        //  Cursor:
        //  Cursor-related messages reflect the state of the server and therefore are broadcast to all active sessions
        virtual Result SetCursor(const Cursor& cursor) override;                // Send the cursor bitmap, invoke whenever the mouse pointer on the server changes its shape
        virtual Result MoveCursor(float x, float y) override;                   // Send absolute mouse coordinates in the [0.0:1.0] range, with {0, 0} representing the top left corner
        virtual Result HideCursor() override;                                   // Send a notification to clients to hide the cursor, calling SetCursor should show it again

        //  Application-specific communications:
        virtual Result SendApplicationMessage(SessionHandle session, const void* msg, size_t size) override;

        virtual Result SetSessionSecurity(SessionHandle session, const char* cipherPassphrase) override; // When cipher is not set or set to nullptr, no encryption is used

        // ReceiverCallback interface
        virtual void OnMessageReceived(Session* session, Channel channel, int msgID, const void* message, size_t messageSize) override;
        virtual void OnTerminate(Session* session, TerminationReason reason) override;

        // OnFillOptionsCallback interface
        virtual transport_common::Result AMF_STD_CALL OnFillOptions(bool discovery, Session* session, HelloResponse::Options* options) override;

        // OnClientConnectCallback interface
        virtual transport_common::Result AMF_STD_CALL OnDiscovery(Session* session) override;
        virtual transport_common::Result AMF_STD_CALL OnClientConnected(Session* session) override;
        virtual bool                     AMF_STD_CALL AuthorizeDiscoveryRequest(Session* session, const char* deviceID) override;
        virtual bool                     AMF_STD_CALL AuthorizeConnectionRequest(Session* session, const char* deviceID) override;


    protected:
        void OnServiceStart(Session* session, const void* msg, size_t len, Subscriber::Ptr pSubscriber);
        void OnServiceStop(Session* session);
        void OnServiceUpdate(Session* session, const void* msg, size_t len, Subscriber::Ptr pSubscriber);
        void OnServiceStatLatency(Session* session, const void* msg, size_t len);
        void StopStreaming();
        void OnServiceMessage(Session* session, uint8_t opcode, const void* msg, size_t len, Subscriber::Ptr pSubscriber);
        void OnSensorsInMessage(Session* session, uint8_t opcode, const void* msg, size_t len, Subscriber::Ptr pSubscriber);
        void OnVideoOutMessage(Session* session, uint8_t opcode, const void* msg, size_t len, Subscriber::Ptr pSubscriber);
        void OnAudioOutMessage(Session* session, uint8_t opcode, const void* msg, size_t len, Subscriber::Ptr pSubscriber);
        void ProcessMessage(Session* session, Channel channel, int msgID, const void* message, size_t messageSize, Subscriber::Ptr pSubscriber);
        void TransmitMessageToAllSubscribers(Channel channel, const void* msg, size_t msgLen);

        bool Decrypt(amf::AMFContextPtr pContext, ssdk::util::AESPSKCipher::Ptr pCipher, Session* session, Channel channel, int msgID,
                     const void* message, size_t messageSize, Subscriber::Ptr pSubscriber);

        bool CreateSubscriber(Session* session);
        bool AddSubscriber(Session* session, Subscriber::Ptr pSubscriber);
        Subscriber::Ptr FindSubscriber(Session* session) const;
        bool DeleteSubscriber(Session* session, TerminationReason reason);

        ssdk::util::AESPSKCipher::Ptr FindCipherForSession(SessionHandle session);
        void CleanupExpiredSessionIDs();

        void ForceKeyFrame(Session* session, const void* msg, size_t len, Subscriber::Ptr pSubscriber);

        void FillLegacyOptions(amf::JSONParser* parser, HelloResponse::Options* options);
        void FillStreamDeclarations(amf::JSONParser* parser, HelloResponse::Options* options);

        // Ciphers class
        class SessionSecurityParams
        {
        public:
            SessionSecurityParams() : m_CreationTime(time(nullptr)) {}
            SessionSecurityParams(ssdk::util::AESPSKCipher::Ptr pCipher) : m_pCipher(pCipher), m_CreationTime(time(nullptr)) {}

            ssdk::util::AESPSKCipher::Ptr GetCipher() const { return m_pCipher; }
            time_t GetCreationTime() const { return m_CreationTime; }
        private:
            ssdk::util::AESPSKCipher::Ptr m_pCipher;
            time_t m_CreationTime;
        };
        typedef std::map<SessionHandle, SessionSecurityParams> SessionToCipherMap;
        SessionToCipherMap m_Ciphers;

        // Session monitor thread classe
        class ExpiredSessionMonitor : public amf::AMFThread
        {
        public:
            ExpiredSessionMonitor(ServerTransportImpl& obj) : m_Object(obj) {}

            virtual void Run() override;

        private:
            ServerTransportImpl& m_Object;
        };
        ExpiredSessionMonitor m_SessionMonitor;

    private:
        typedef std::map<Session*, Subscriber::Ptr> Subscribers;
        typedef std::map<SessionHandle, Session*> Sessions;
        typedef std::unordered_map<int, amf_pts> MsgID2TimeMap;

        mutable amf::AMFCriticalSection m_Guard;
        mutable amf::AMFCriticalSection m_KeyFrameGuard;
        ServerInitParametersAmd         m_InitParams;
        Subscribers                     m_Subscribers;
        Sessions                        m_Sessions;
        amf::AMFContextPtr              m_pContext;
        ServerImpl::Ptr                 m_pServer;
        Session::Ptr	                m_pDiscoverySession;
        bool                            m_Initialized{ false };
        bool                            m_Stop{ false };
        MsgID2TimeMap                   m_DecryptorSubmissionTimestamps;
        float                           m_CurentFrameRate = 0.0f;
        int64_t                         m_CurrentBitrate = 0;
        AMFSize                         m_CurrentResolution = { 0, 0 };
        amf_pts                         m_AverageSensorFreq = 0;
        amf_pts                         m_AverageSensorProcTime = 0;
        amf_pts                         m_LastSensorTime = 0;
        amf_int64                       m_SensorDataCount = 0;
    }; // class ServerTransportImpl
} // namespace ssdk::transport_amd
