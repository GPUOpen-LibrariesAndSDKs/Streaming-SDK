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

#include "Transport.h"
#include "controllers/ControllerTypes.h"

namespace ssdk::transport_amd
{
    class Session;
}

namespace ssdk::transport_common
{
    //----------------------------------------------------------------------------------------------
    // ServerTransport class
    //----------------------------------------------------------------------------------------------
    class ServerTransport : public Transport
    {
    public:
        typedef std::shared_ptr<ServerTransport>    Ptr;

        enum class NETWORK_TYPE
        {
            NETWORK_UDP = 0,
            NETWORK_TCP = 1
        };

        //  Application notification callbacks:
        //  ConnectionManagerCallback: all server applications must implement this interface
        class ConnectionManagerCallback
        {
        public:
            //  Client roles:
            enum class ClientRole
            {
                CONTROLLER,     //  Client can send and receive events from input devices, such as input controllers and trackers
                VIEWER          //  Client is a passive viewer, not sending any input events
            };

            enum class ClientAction
            {
                REFUSE,
                ACCEPT
            };

            enum class DisconnectReason
            {
                CLIENT_DISCONNECTED,
                TIMEOUT
            };

        public:
            virtual ClientAction OnDiscoveryRequest(const char* clientID) = 0;                                          // Called when a discovery request is received, server can either accept or refuse request
            virtual ClientAction OnConnectionRequest(SessionHandle session, const char* clientID, ClientRole role) = 0; // Called when a client tries to establish a connection, server can either accept or refuse request
            virtual void OnClientSubscribed(SessionHandle session) = 0;                                                 // Called when the client is subscribed and ready to receive streaming
            virtual void OnClientDisconnected(SessionHandle session, DisconnectReason reason) = 0;                      // Called when the client is disconnected
        };

        //  VideoSenderCallback: implement when server sends video to clients
        class VideoSenderCallback
        {
        public:
            virtual void OnVideoStreamSubscribed(SessionHandle session, StreamID streamID) = 0;         // Called when a client subscribes to a specific video stream
            virtual void OnVideoStreamUnsubscribed(SessionHandle session, StreamID streamID) = 0;       // Called when a client unsibscribed from a specific video stream
            virtual void OnReadyToReceiveVideo(SessionHandle session, StreamID streamID, InitID initID) = 0;  // Called when a client is ready to accept video frames after video codec reinitialization.

            virtual void OnForceUpdateRequest(StreamID streamID) = 0;               // Called when a key/IDR frame must be submitted. This can be requested by the client or decided by the server
            virtual void OnVideoRequestInit(SessionHandle session, StreamID streamID) = 0;              // Called when the video init block needs to be sent/resent

            virtual void OnBitrateChangeRecieverRequest(SessionHandle session, StreamID streamID, int64_t bitrate) = 0;                  // Called when a video receiver requested bitrate change
            virtual void OnFramerateChangeRecieverRequest(SessionHandle session, StreamID streamID, float framerate) = 0;                // Called when a video receiver requested framerate change
            virtual void OnResolutionChangeRecieverRequest(SessionHandle session, StreamID streamID, const AMFSize& resolution) = 0;     // Called when a video receiver requested resolution change
        };

        //  VideoSenderCallback: implement when client sends video stats to server
        class VideoStatsCallback
        {
        public:
            struct Stats
            {
                amf_pts     lastStatsTime;
                float       receiverFramerate;
                float       avgSendTime;
                int64_t     decoderQueueDepth;
                int64_t     keyFrameReqCount;
                float       fullLatency;
                float       clientLatency;
                float       decoderLatency;
                float       serverLatency;
                float       encoderLatency;
                float       networkLatency;
            };
        public:
            virtual void OnVideoStats(SessionHandle session, StreamID streamID, const Stats& stats) = 0;        // Called when statistics is updated for a specific client/stream
            virtual void OnOriginPts(SessionHandle session, StreamID streamID, amf_pts originPts) = 0;          // Called when sensor timestamp is updated for a specific client/stream
        };

        //  VideoReceiverCallback: implement when server receives video from clients, i.e. virtual webcam, AR feed, etc
        class VideoReceiverCallback
        {
        public:
            virtual void OnVideoInit(SessionHandle session, StreamID streamID, const char* codec, InitID initID,
                    const AMFSize& streamResolution, const AMFRect& viewport, uint32_t bitDepth, bool stereoscopic, bool foveated,
                    const void* initBlock, size_t initBlockSize) = 0;                                                                   // Called when a video codec init block is received
            virtual void OnVideoFrame(SessionHandle session, StreamID streamID, const ReceivableVideoFrame& frame) = 0;                 // Called when a video frame is received
        };

        //  AudioSenderCallback: implement when server sends audio to clients
        class AudioSenderCallback
        {
        public:
            virtual void OnAudioStreamSubscribed(SessionHandle session, StreamID streamID) = 0;                          // Called when a client subscribes to a specific video stream
            virtual void OnAudioStreamUnsubscribed(SessionHandle session, StreamID streamID) = 0;                        // Called when a client unsibscribed from a specific video stream
            virtual void OnReadyToReceiveAudio(SessionHandle session, StreamID streamID, InitID initID) = 0;             // Called when an ACK or a NACK is received from the client. The id parameter contains the same value as initID passed to SendAudioInit

            virtual void OnAudioRequestInit(SessionHandle session, StreamID streamID) = 0;                               // Called when the audio init block needs to be sent/resent
        };

        //  AudioReceiverCallback: implement when server receives audio from clients, i.e. virtual microphone
        class AudioReceiverCallback
        {
        public:
            virtual void OnAudioInit(SessionHandle session, StreamID streamID, const char* codec, InitID initID,
                    uint32_t channels, uint32_t layout, uint32_t samplingRate,
                    const void* initBlock, size_t initBlockSize) = 0;                                                                       // Called when an audio codec init block is received
            virtual void OnAudioBuffer(SessionHandle session, StreamID streamID, const ReceivableAudioBuffer& buffer) = 0;                  // Called when an audio buffer is received
        };

        //  InputControllerCallback: implement when server receives data from input devices (kbd, mice, game controllers, etc and trackable devices such as HMDs, VR controllers)
        class InputControllerCallback
        {
        public:
            virtual void OnControllerEnabled(SessionHandle session, const char* deviceID, ControllerType type) = 0;
            virtual void OnControllerDisabled(SessionHandle session, const char* deviceID) = 0;
            virtual void OnControllerInputEvent(SessionHandle session, const char* controlID, const ssdk::ctls::CtlEvent& event) = 0;
            virtual amf::AMF_VARIANT_TYPE GetExpectedEventDataType(const char* controlID) = 0;
            virtual void OnTrackableDevicePoseChange(SessionHandle session, const char* deviceID, const Pose& pose) = 0;
        };

        //  ApplicationCallback: implement when the application defines any custom messages
        class ApplicationCallback
        {
        public:
            virtual void OnAppMessage(SessionHandle session, const void* buf, size_t size) = 0;
        };

        //  Base class for transport-specific server parameters:
        class ServerInitParameters
        {
        public:
            typedef std::vector<VideoStreamDescriptor> VideoStreams;
            typedef std::vector<AudioStreamDescriptor> AudioStreams;

        protected:
            ServerInitParameters() = default;
            virtual ~ServerInitParameters() = default;

        public:
            inline void SetCallback(ConnectionManagerCallback* cb) noexcept { m_CMCallback = cb; }
            inline void SetCallback(VideoSenderCallback* cb) noexcept { m_VSCallback = cb; }
            inline void SetCallback(VideoStatsCallback* cb) noexcept { m_StatsCallback = cb; }
            inline void SetCallback(VideoReceiverCallback* cb) noexcept { m_VRCallback = cb; }
            inline void SetCallback(AudioSenderCallback* cb) noexcept { m_ASCallback = cb; }
            inline void SetCallback(AudioReceiverCallback* cb) noexcept { m_ARCallback = cb; }
            inline void SetCallback(InputControllerCallback* cb) noexcept { m_ICCallback = cb; }
            inline void SetCallback(ApplicationCallback* cb) noexcept { m_AppCallback = cb; }

            inline ConnectionManagerCallback* GetConnectionManagerCallback() const noexcept { return m_CMCallback; }
            inline VideoSenderCallback* GetVideoSenderCallback() const noexcept { return m_VSCallback; }
            inline VideoStatsCallback* GetVideoStatsCallback() const noexcept { return m_StatsCallback; }
            inline VideoReceiverCallback* GetVideoReceiverCallback() const noexcept { return m_VRCallback; }
            inline AudioSenderCallback* GetAudioSenderCallback() const noexcept { return m_ASCallback; }
            inline AudioReceiverCallback* GetAudioReceiverCallback() const noexcept { return m_ARCallback; }
            inline InputControllerCallback* GetInputControllerCallback() const noexcept { return m_ICCallback; }
            inline ApplicationCallback* GetAppCallback() const noexcept { return m_AppCallback; }

            inline const VideoStreams& GetVideoStreams() const noexcept { return m_VideoStreams; }
            inline void AddVideoStream(const VideoStreamDescriptor& streamDescriptor) noexcept { m_VideoStreams.push_back(streamDescriptor); }

            inline const AudioStreams& GetAudioStreams() const noexcept { return m_AudioStreams; }
            inline void AddAudioStream(const AudioStreamDescriptor& streamDescriptor) noexcept { m_AudioStreams.push_back(streamDescriptor); }

        protected:
            ConnectionManagerCallback*  m_CMCallback = nullptr;
            VideoSenderCallback*        m_VSCallback = nullptr;
            VideoStatsCallback*         m_StatsCallback = nullptr;
            VideoReceiverCallback*      m_VRCallback = nullptr;
            AudioSenderCallback*        m_ASCallback = nullptr;
            AudioReceiverCallback*      m_ARCallback = nullptr;
            InputControllerCallback*    m_ICCallback = nullptr;
            ApplicationCallback*        m_AppCallback = nullptr;
            std::vector<VideoStreamDescriptor>  m_VideoStreams;
            std::vector<AudioStreamDescriptor>  m_AudioStreams;
        };

    public:
        //  Start/stop:
        virtual Result Start(const ServerInitParameters& params) = 0;               //  Start the server with specified parameters
        virtual Result Shutdown() = 0;                                              //  Shutdown the server

        //  Session management:
        virtual size_t GetActiveConnectionCount() const noexcept = 0;               //  Query the number of active sessions
        virtual bool   IsSessionActive(SessionHandle session) const noexcept = 0;   //  Check if a session identified by SessionHandle is connected
        virtual Result Disconnect(SessionHandle session) noexcept = 0;              //  Forcefully disconnect a specific session
        virtual Result DisconnectAllSessions() noexcept = 0;                        //  Forcefully disconnect all sessions

        //  Send various types of messages:
        //  Video:
        virtual Result SendVideoInit(SessionHandle session, const char* codec, StreamID streamID, InitID initID,
                                     const AMFSize& streamResolution, const AMFRect& viewport, uint32_t bitDepth,
                                     bool stereoscopic, bool foveated, const void* initBlock, size_t initBlockSize) = 0;
        virtual Result SendVideoFrame(SessionHandle session, StreamID streamID, const TransmittableVideoFrame& frame) = 0;

        //  Audio:
        virtual Result SendAudioInit(SessionHandle session, const char* codec, StreamID streamID, InitID initID, uint32_t channels,
                                     uint32_t layout, amf::AMF_AUDIO_FORMAT format, uint32_t samplingRate, const void* initBlock, size_t initBlockSize) = 0;
        virtual Result SendAudioBuffer(SessionHandle session, StreamID streamID, const TransmittableAudioBuffer& buf) = 0;

        //  Controllers:
        virtual Result SendControllerEvent(const char* controlID, const ssdk::ctls::CtlEvent* pEvent) = 0;

        //  Cursor:
        //  Cursor-related messages reflect the state of the server and therefore are broadcast to all active sessions
        virtual Result SetCursor(const Cursor& cursor) = 0;                 //  Send the cursor bitmap, invoke whenever the mouse pointer on the server changes its shape
        virtual Result MoveCursor(float x, float y) = 0;                    //  Send absolute mouse coordinates in the [0.0:1.0] range, with {0, 0} representing the top left corner
        virtual Result HideCursor() = 0;                                    //  Send a notification to clients to hide the cursor, calling SetCursor should show it again

        //  Application-specific communications:
        virtual Result SendApplicationMessage(SessionHandle session, const void* msg, size_t size) = 0;

        virtual Result SetSessionSecurity(SessionHandle session, const char* cipherPassphrase) = 0;   // When cipher is not set or set to nullptr, no encryption is used
    };

}