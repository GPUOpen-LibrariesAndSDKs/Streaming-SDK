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

#include "ClientImpl.h"
#include "transports/transport-common/ClientTransport.h"
#include "util/encryption/AESPSKCipher.h"
#include "sdk/util/stats/ClientStatsManager.h"

#include <set>
#include <map>

namespace ssdk::transport_amd
{
    using namespace ssdk::transport_common;

    static const amf_uint8 TURNAROUND_LATENCY_MESSAGE_PERIOD = 16; // ms

    class ClientTransportImpl : public ClientTransport, public ReceiverCallback
    {
    public:
        typedef std::shared_ptr<ClientTransportImpl> Ptr;

        class ClientInitParametersAMD : public ClientInitParameters
        {
        public:
            ClientInitParametersAMD() {};

            inline const std::string GetID() const noexcept { return m_ID; }
            inline void SetID(const std::string& id) noexcept { m_ID = id; }

            inline const std::string GetDisplayModel() const noexcept { return m_displayModel; }
            inline void SetDisplayModel(const std::string& model) noexcept { m_displayModel = model; }

            inline int32_t GetDisplayWidth() const noexcept { return m_displayWidth; }
            inline void SetDisplayWidth(int32_t width) noexcept { m_displayWidth = width; }

            inline int32_t GetDisplayHeight() const noexcept { return m_displayHeight; }
            inline void SetDisplayHeight(int32_t height) noexcept { m_displayHeight = height; }

            inline float GetFramerate() const noexcept { return m_framerate; }
            inline void SetFramerate(float framerate) noexcept { m_framerate = framerate; }

            inline uint32_t GetBitrate() const noexcept { return m_bitrate; }
            inline void SetBitrate(uint32_t bitrate) noexcept { m_bitrate = bitrate; }

            inline int32_t GetDisplayClientWidth() const noexcept { return displayClientWidth; }
            inline void SetDisplayClientWidth(int32_t width) noexcept { displayClientWidth = width; }

            inline int32_t GetDisplayClientHeight() const noexcept { return displayClientHeight; }
            inline void SetDisplayClientHeight(int32_t height) noexcept { displayClientHeight = height; }

            inline uint16_t GetDiscoveryPort() const noexcept { return m_discoveryPort; }
            inline void SetDiscoveryPort(uint16_t port) noexcept { m_discoveryPort = port; }

            int64_t GetDatagramSize() { return m_DatagramSize; };
            void SetDatagramSize(int64_t datagramSize) { m_DatagramSize = datagramSize; };

            inline ServerEnumCallback* GetServerEnumCallback() const noexcept { return m_ServerEnumCallback; }
            inline void SetServerEnumCallback(ServerEnumCallback* callback) noexcept { m_ServerEnumCallback = callback; }

            inline amf::AMFContextPtr GetContext() const noexcept { return m_pContext; } // fix
            inline void SetContext(const amf::AMFContextPtr& context) noexcept { m_pContext = context; }

            inline amf_uint GetLatencyMessagePeriod() const noexcept { return m_LatencyMessagePeriod; }
            inline void SetLatencyMessagePeriod(amf_uint period) noexcept { m_LatencyMessagePeriod = period; }

            inline const std::string GetPassphrase() const noexcept { return m_cipherPassphrase; }
            inline void SetPassphrase(const std::string& cipherPassphrase) noexcept { m_cipherPassphrase = cipherPassphrase; }

            inline void SetConnectionManagerCallback(ConnectionManagerCallback* callback) noexcept { m_CMCallback = callback; }
            inline void SetVideoSenderCallback(VideoSenderCallback* callback) noexcept { m_VSCallback = callback; }
            inline void SetVideoReceiverCallback(VideoReceiverCallback* callback) noexcept { m_VRCallback = callback; }
            inline void SetAudioSenderCallback(AudioSenderCallback* callback) noexcept { m_ASCallback = callback; }
            inline void SetAudioReceiverCallback(AudioReceiverCallback* callback) noexcept { m_ARCallback = callback; }
            inline void SetInputControllerCallback(InputControllerCallback* callback) noexcept { m_ICCallback = callback; }
            inline void SetAppCallback(ApplicationCallback* callback) noexcept { m_AppCallback = callback; }
            inline void SetCursorCallback(CursorCallback* callback) noexcept { m_CursorCallback = callback; }


        protected:
            std::string m_ID;
            std::string m_displayModel;
            int32_t m_displayWidth = 0;
            int32_t m_displayHeight = 0;
            float m_framerate = 0.0;
            uint32_t m_bitrate = 0;
            int32_t displayClientWidth = 0;
            int32_t displayClientHeight = 0;
            uint16_t m_discoveryPort = 0;
            ServerEnumCallback* m_ServerEnumCallback = nullptr;
            amf::AMFContextPtr m_pContext = nullptr;
            amf_uint m_LatencyMessagePeriod = TURNAROUND_LATENCY_MESSAGE_PERIOD;
            std::string m_cipherPassphrase;
            int64_t m_DatagramSize{ 65507 };
        };

        class ServerDescriptorAMD : public ServerDescriptor
        {
        public:
            typedef std::shared_ptr<ServerDescriptor>   Ptr;

            ServerDescriptorAMD(const std::string& name, const std::string& url, const std::string& description) :
                ServerDescriptor(name, url, description) {};

        protected:
            ServerDescriptorAMD() = default;
        };

        class TurnaroundLatencyThread : public amf::AMFThread
        {
        public:
            TurnaroundLatencyThread() {};
            void Init(ClientTransportImpl* pTransport, amf_uint period, StreamID streamID)
            {
                m_LastPts = amf_high_precision_clock();
                m_pTransport = pTransport;
                m_Period = period;
                m_StreamID = streamID;
            }
            //void Deinit();
            void SetLastPts(amf_pts pts) { amf::AMFLock lock(&m_Guard);  m_LastPts = pts; };
            void SetPeriod(amf_uint period) { amf::AMFLock lock(&m_Guard); m_Period = period; };

            virtual void Run();

        protected:
            ClientTransportImpl* m_pTransport = nullptr;
            amf_pts m_LastPts = 0; // in 100 nanosecs, time when last pose message was sent to server
            amf_uint m_Period = 0; // in ms, sleep period
            mutable amf::AMFCriticalSection m_Guard;
            StreamID m_StreamID;

        };

        typedef std::unique_ptr<char[]> SendData;

    public:
        ClientTransportImpl() {};
        virtual ~ClientTransportImpl() {};

        //	ClientTransport methods:
        virtual Result Start(const ClientInitParameters& params) override;
        virtual Result Shutdown() override;


        virtual Result FindServers() override;

        virtual Result Connect(const char* url) override;
        virtual Result Disconnect() override;

        virtual Result SubscribeToVideoStream(StreamID StreamID = DEFAULT_STREAM) override;
        virtual Result SubscribeToAudioStream(StreamID StreamID = DEFAULT_STREAM) override;
        virtual Result UnsubscribeFromVideoStream(StreamID StreamID = DEFAULT_STREAM) override;
        virtual Result UnsubscribeFromAudioStream(StreamID StreamID = DEFAULT_STREAM) override;

        virtual Result SendVideoInit(const char* codec, StreamID StreamID, InitID initID, const AMFSize& streamResolution, const AMFRect& viewport, uint32_t bitDepth, const void* initBlock, size_t initBlockSize) override;
        virtual Result SendVideoFrame(StreamID StreamID, const TransmittableVideoFrame& frame) override;

        virtual Result SendAudioInit(const char* codec, StreamID StreamID, InitID initID, amf::AMF_AUDIO_FORMAT format, uint32_t channels, uint32_t layout, uint32_t samplingRate, const void* initBlock, size_t initBlockSize) override;
        virtual Result SendAudioBuffer(StreamID StreamID, const TransmittableAudioBuffer& transmittableBuf) override;

        virtual Result SendControllerEvent(const char* controlID, const amf::AMFVariant& event) override;
        virtual Result SendTrackableDevicePose(const char* deviceID, const Pose& pose) override;

        virtual Result SendDeviceConnected(const char* deviceID, ssdk::ctls::CTRL_DEVICE_TYPE eType, const char* description) override;
        virtual Result SendDeviceDisconnected(const char* deviceID) override;

        // Statistics:
        virtual Result SendStats(StreamID streamID, const StatsSnapshot& statsSnapshot) override;

        virtual Result SendApplicationMessage(const void* msg, size_t size) override;

        // ReceiverCallback methods:
        virtual void OnMessageReceived(Session* session, Channel channel, int msgID, const void* msg, size_t messageSize) override;
        virtual void OnTerminate(Session* session, TerminationReason reason) override;

        // own methods
        void SetStatsManager(ssdk::util::ClientStatsManager::Ptr statsManager);
    protected:
        
        Result SendMessageWithData(Channel channel, Message* message, const void* data, size_t dataSize);
        void ProcessMessage(Session* session, Channel channel, int msgID, const void* msg, size_t messageSize);
        void OnServiceMessage(Session* session, const void* msg, size_t messageSize);
        void OnServiceConnectionRefused();
        void OnServiceForceIDRFrame(Session* session);
        void OnServiceServerStat(Session* session, const void* msg, size_t messageSize);
        void OnVideoOutMessage(Session* session, const void* msg, size_t messageSize);
        void OnVideoOutInit(Session* session, const void* msg, size_t messageSize);
        void OnVideoOutData(Session* session, const void* msg, size_t messageSize);
        void OnVideoOutCursor(Session* session, const void* msg, size_t messageSize);
        void OnAudioOutMessage(Session* session, const void* msg, size_t messageSize);
        void OnAudioOutInit(Session* session, const void* msg, size_t messageSize);
        void OnAudioOutData(Session* session, const void* msg, size_t messageSize);
        void OnAudioInMessage(Session* session, const void* msg, size_t messageSize);
        void OnSensorsInMessage(Session* session, const void* msg, size_t messageSize);
        void OnMiscOutMessage(Session* session, const void* msg, size_t messageSize);
        void OnSensorsOutMessage(Session* session, const void* msg, size_t messageSize);
        void OnUserDefinedMessage(Session* session, const void* msg, size_t messageSize);

        transport_common::Result SendMsg(Channel channel, const void* msg, size_t msgLen);
        Result SendVideoForceUpdate(StreamID streamID, uint64_t frameNumber, bool IDRframe);
    private:

        ClientInitParametersAMD m_clientInitParameters;
        ClientImpl::Ptr m_pClient = nullptr;
        amf::AMFContextPtr m_pContext = nullptr;
        ssdk::util::AESPSKCipher::Ptr m_pCipher = nullptr;
        ssdk::util::ClientStatsManager::Ptr m_StatsManager = nullptr;
        mutable amf::AMFCriticalSection m_CCCGuard; // CLient, Context, Cipher guard

        typedef std::set<StreamID> StreamIDSet;
        StreamIDSet                         m_SubscribedVideoStreams;
        StreamIDSet                         m_SubscribedAudioStreams;

        ClientSessionImpl::Ptr m_pSession = nullptr;
        mutable amf::AMFCriticalSection m_SessionGuard;
        TurnaroundLatencyThread m_TurnaroundLatencyThread;

        class FrameLossInfo
        {
        public:
            uint64_t m_lastFrameNumbers = 0;
            amf_pts m_LastIDRRequestTime = 0;
            bool m_IDRRequestSent = false;
            
        };
        typedef std::map<StreamID, FrameLossInfo> FrameLossInfoMap;
        FrameLossInfoMap m_FrameLossInfoMap;
    };
}
