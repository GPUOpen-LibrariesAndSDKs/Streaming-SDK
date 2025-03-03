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

#include <string>
#include <memory>
#include <vector>

namespace ssdk::transport_common
{
    class ClientTransport : public Transport
    {
    public:
        typedef std::shared_ptr<ClientTransport>    Ptr;

        class ServerDescriptor
        {
        public:
            typedef std::shared_ptr<ServerDescriptor>   Ptr;

        protected:
            ServerDescriptor() = default;
            ServerDescriptor(const std::string& name, const std::string& url, const std::string& description);
        public:
            virtual ~ServerDescriptor() = default;

            inline const std::string& GetUrl() const noexcept { return m_Url; }
            inline const std::string& GetName() const noexcept { return m_Name; }
            inline const std::string& GetDescription() const noexcept { return m_Description; }

            inline const std::vector<VideoStreamDescriptor>& GetVideoStreams() const noexcept { return m_VideoStreams; }
            inline std::vector<VideoStreamDescriptor>& GetVideoStreams() noexcept { return m_VideoStreams; }
            inline void AddVideoStream(const VideoStreamDescriptor& streamDescriptor) noexcept { m_VideoStreams.push_back(streamDescriptor); }

            inline const std::vector<AudioStreamDescriptor>& GetAudioStreams() const noexcept { return m_AudioStreams; }
            inline std::vector<AudioStreamDescriptor>& GetAudioStreams() noexcept { return m_AudioStreams; }
            inline void AddAudioStream(const AudioStreamDescriptor& streamDescriptor) noexcept { m_AudioStreams.push_back(streamDescriptor); }

        protected:
            std::string     m_Url;
            std::string     m_Name;
            std::string     m_Description;
            std::vector<VideoStreamDescriptor>  m_VideoStreams;
            std::vector<AudioStreamDescriptor>  m_AudioStreams;
        };

        class ConnectionManagerCallback
        {
        public:
            enum class DiscoveryAction      //  What to do when a server is found
            {
                STOP,                       //  Stop discovery
                CONTINUE                    //  Continue discovery
            };

            enum class TerminationReason
            {
                CLOSED_BY_SERVER,           //  Connection was closed by the server
                CLOSED_BY_CLIENT,           //  Connection was closed by client
                TIMEOUT                     //  Connection timed out
            };

        public:
            virtual DiscoveryAction OnServerDiscovered(const ServerDescriptor& server) = 0;      //  Called every time a new server is found
            virtual void OnDiscoveryComplete() = 0;                                             //  Called when the discovery process is complete

            virtual void OnConnectionEstablished(const ServerDescriptor& server) = 0;           //  Called when a connection to a server was established and acknowledged by the server
            virtual void OnConnectionTerminated(TerminationReason reason) = 0;                  //  Called when a connection was terminated for any reason
        };

        //  VideoSenderCallback: implement when server sends video to clients
        class VideoSenderCallback
        {
        public:
            virtual void OnVideoStreamSubscribed(StreamID streamID) = 0;                                             // Called when a client subscribes to a specific video stream
            virtual void OnVideoStreamUnsubscribed(StreamID streamID) = 0;                                           // Called when a client unsibscribed from a specific video stream
            virtual void OnReadyToReceiveVideo(StreamID streamID, int64_t initID) = 0;                               // Called when a receiver is ready to receive video.
                                                                                                                    // The id parameter contains the same value as initID passed to SendVideoInit

            virtual void OnBitrateChangeRecieverRequest(StreamID streamID, int64_t bitrate) = 0;                       // Called when a video receiver requested bitrate change
            virtual void OnFramerateChangeRecieverRequest(StreamID streamID, float framerate) = 0;                     // Called when a video receiver requested framerate change
            virtual void OnResolutionChangeRecieverRequest(StreamID streamID, const AMFSize& resolution) = 0;          // Called when a video receiver requested resolution change

            virtual void OnKeyFrameRequest(StreamID streamID) = 0;                                                   // Called when a client requested a key/IDR frame

            virtual void OnBitrateChangeByQoS(int64_t bitrate, StreamID streamID) = 0;                               // Called when QoS requested bitrate change
            virtual void OnFramerateChangeByQoS(float framerate, StreamID streamID) = 0;                             // Called when QoS requested framerate change
            virtual void OnResolutionChangeByQoS(const AMFSize& resolution, StreamID streamID) = 0;                  // Called when QoS requested resolution change

            virtual void OnVideoStats(StreamID streamID, amf::AMFPropertyStorage* stats) = 0;                      // Called when statistics is updated for a specific client/stream
        };

        //  VideoReceiverCallback: implement when client receives video from server
        class VideoReceiverCallback
        {
        public:
            virtual bool OnVideoInit(const char* codec, StreamID streamID, int64_t initID,
                const AMFSize& streamResolution, const AMFRect& viewport, uint32_t bitDepth,
                bool stereoscopic, bool foveated, const void* initBlock, size_t initBlockSize) = 0;                 // Called when a video codec init block is received
            virtual void OnVideoFrame(StreamID streamID, const ReceivableVideoFrame& frame) = 0;                    // Called when a video frame is received
            virtual void OnVideoServerStats(StreamID streamID, amf::AMFPropertyStorage* stats) = 0;                 // Called when statistics is updated for a specific server
        };

        //  AudioSenderCallback: implement when server sends audio to clients
        class AudioSenderCallback
        {
        public:
            virtual void OnAudioStreamSubscribed(StreamID streamID) = 0;                                             // Called when a client subscribes to a specific video stream
            virtual void OnAudioStreamUnsubscribed(StreamID streamID) = 0;                                           // Called when a client unsibscribed from a specific video stream
            virtual void OnReadyToReceiveAudio(StreamID streamID, int64_t initID) = 0;                               // Called when the receiver is ready to receive audio. The id parameter contains the same value as initID passed to SendAudioInit
        };

        //  AudioReceiverCallback: implement when server receives audio from clients, i.e. virtual microphone
        class AudioReceiverCallback
        {
        public:
            virtual bool OnAudioInit(const char* codec, StreamID streamID, int64_t initID,
                uint32_t channels, uint32_t layout, uint32_t samplingRate, amf::AMF_AUDIO_FORMAT format,
                const void* initBlock, size_t initBlockSize) = 0;                                                   // Called when an audio codec init block is received
            virtual void OnAudioBuffer(StreamID streamID, const ReceivableAudioBuffer& buffer) = 0;                 // Called when an audio buffer is received
        };

        //  InputControllerCallback: implement when server receives data from input devices (kbd, mice, game controllers, etc and trackable devices such as HMDs, VR controllers)
        class InputControllerCallback
        {
        public:
            virtual void OnControllerEnabled(const char* deviceID, ControllerType type) = 0;
            virtual void OnControllerDisabled(const char* deviceID) = 0;
            virtual void OnControllerInputEvent(const char* controlID, const amf::AMFVariant& event) = 0;
        };

        class CursorCallback
        {
        public:
            virtual void OnCursorChanged(const Cursor& cursor) = 0;
            virtual void OnCursorHidden() = 0;
        };

        //  ApplicationCallback: implement when the application defines any custom messages
        class ApplicationCallback
        {
        public:
            virtual void OnAppMessage(const void* buf, size_t size) = 0;
        };


        class ClientInitParameters
        {
        protected:
            ClientInitParameters() = default;
        public:
            virtual ~ClientInitParameters() = default;

            inline ConnectionManagerCallback* GetConnectionManagerCallback() const noexcept { return m_CMCallback; }
            inline VideoSenderCallback* GetVideoSenderCallback() const noexcept { return m_VSCallback; }
            inline VideoReceiverCallback* GetVideoReceiverCallback() const noexcept { return m_VRCallback; }
            inline AudioSenderCallback* GetAudioSenderCallback() const noexcept { return m_ASCallback; }
            inline AudioReceiverCallback* GetAudioReceiverCallback() const noexcept { return m_ARCallback; }
            inline InputControllerCallback* GetInputControllerCallback() const noexcept { return m_ICCallback; }
            inline ApplicationCallback* GetAppCallback() const noexcept { return m_AppCallback; }
            inline CursorCallback* GetCursorCallback() const noexcept { return m_CursorCallback; }

        protected:
            ConnectionManagerCallback* m_CMCallback = nullptr;
            VideoSenderCallback* m_VSCallback = nullptr;
            VideoReceiverCallback* m_VRCallback = nullptr;
            AudioSenderCallback* m_ASCallback = nullptr;
            AudioReceiverCallback* m_ARCallback = nullptr;
            InputControllerCallback* m_ICCallback = nullptr;
            ApplicationCallback* m_AppCallback = nullptr;
            CursorCallback* m_CursorCallback = nullptr;
        };

        class StatsSnapshot
        {
        public:
            virtual ~StatsSnapshot() = default;
        };

    public:
        virtual Result Start(const ClientInitParameters& params) = 0;
        virtual Result Shutdown() = 0;


        virtual Result FindServers() = 0;

        virtual Result Connect(const char* url) = 0;
        virtual Result Disconnect() = 0;
        virtual Result SubscribeToVideoStream(StreamID StreamID = DEFAULT_STREAM) = 0;
        virtual Result SubscribeToAudioStream(StreamID StreamID = DEFAULT_STREAM) = 0;
        virtual Result UnsubscribeFromVideoStream(StreamID StreamID = DEFAULT_STREAM) = 0;
        virtual Result UnsubscribeFromAudioStream(StreamID StreamID = DEFAULT_STREAM) = 0;

        //  Video:
        virtual Result SendVideoInit(const char* codec, StreamID StreamID, InitID initID, const AMFSize& streamResolution, const AMFRect& viewport, uint32_t bitDepth, const void* initBlock, size_t initBlockSize) = 0;
        virtual Result SendVideoFrame(StreamID StreamID, const TransmittableVideoFrame& frame) = 0;

        //  Audio:
        virtual Result SendAudioInit(const char* codec, StreamID StreamID, InitID initID, amf::AMF_AUDIO_FORMAT format, uint32_t channels, uint32_t layout, uint32_t samplingRate, const void* initBlock, size_t initBlockSize) = 0;
        virtual Result SendAudioBuffer(StreamID StreamID, const TransmittableAudioBuffer& buf) = 0;

        //  Controllers:
        virtual Result SendControllerEvent(const char* controlID, const amf::AMFVariant& event) = 0;
        virtual Result SendTrackableDevicePose(const char* deviceID, const Pose& pose) = 0;

        //  Devices:
        virtual Result SendDeviceConnected(const char* deviceID, ssdk::ctls::CTRL_DEVICE_TYPE eType, const char* description) = 0;
        virtual Result SendDeviceDisconnected(const char* deviceID) = 0;

        // Statistics:
        virtual Result SendStats(StreamID streamID, const StatsSnapshot& statsSnapshot) = 0;

        //  Application-specific communications:
        virtual Result SendApplicationMessage(const void* msg, size_t size) = 0;

    };
}
