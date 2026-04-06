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

Copyright (c) Advanced Micro Devices, Inc. All rights reserved.

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

#include "sdk/video/VideoDispatcher.h"
#include "sdk/audio/AudioDispatcher.h"
#include "sdk/transports/transport-amd/ClientTransportImpl.h"
#include "sdk/video/android/VideoReceiverAndroid.h"
#include "sdk/audio/android/AudioPresenterAndroid.h"
#include "sdk/util/pipeline/AVSynchronizer.h"
#include "sdk/util/stats/ClientStatsManager.h"
#include "sdk/util/jni/ANativeWindowHolder.h"
#include "sdk/util/jni/JavaObjectHolder.h"
#include "sdk/controllers/client/ControllerManager.h"

#include "amf/public/include/core/Context.h"

#include <string>
#include <memory>

class SimpleStreamingClient :
    public ssdk::transport_amd::ClientTransport::ConnectionManagerCallback
{
public:
    typedef std::unique_ptr<SimpleStreamingClient>  Ptr;

    enum class Result
    {
        OK,
        FAILED_INIT_AMF,
        FAILED_INIT_APP,
        FAILED_INIT_VIDEO,
        FAILED_INIT_AUDIO,
        FAILED_INIT_NET,
        FAILED_INIT_CTRL,
        FAILED_INIT_CURSOR,
        FAILED_INIT_STATS
    };
protected:
    SimpleStreamingClient();
    ~SimpleStreamingClient();

public:
    static SimpleStreamingClient& GetInstance();
    static void Destroy();

    Result Init(ssdk::util::JavaObjectHolder&& activity, ssdk::util::ANativeWindowHolder&& decodeWindow, ssdk::util::ANativeWindowHolder&& cursorWindow,
              int displayWidth, int displayHeight, int displayRefreshRate, int displayViewportLeft, int displayViewportTop, int displayViewportRight, int displayViewportBottom,
              const std::string& vendor, const std::string& model, const std::string& deviceID, const std::string& writablePath,
              const std::string& serverUrl, int port,
              bool encryption, const std::string& password, bool enableLogging);
    Result Terminate();

    // Implementation of ConnectionManagerCallback interface
    virtual void OnConnectionEstablished(const ssdk::transport_amd::ClientTransport::ServerDescriptor& server) override;        // Called when a connection to a server was established and acknowledged by the server
    virtual void OnConnectionTerminated(TerminationReason reason) override;                                                     // Called when a connection was terminated for any reason

    bool DispatchInputEvent(const WindowMessage& msg);

private:
    bool InitAMF();
    void TerminateAMF();

    bool InitApp();
    void TerminateApp();

    bool InitNetwork();
    void TerminateNetwork();

    bool InitVideo(ssdk::util::ANativeWindowHolder&& decodeWindow);
    void TerminateVideo();

    bool InitAudio();
    void TerminateAudio();

    bool InitControllers();
    void TerminateControllers();

    bool InitStatistics();
    void TerminateStatistics();

private:
    class ConnectThread :
        public ssdk::transport_amd::ServerEnumCallback,
        public amf::AMFThread
    {
    public:
        typedef std::unique_ptr<ConnectThread> Ptr;

    public:
        ConnectThread(ssdk::transport_common::ClientTransport::Ptr Transport) : m_Transport(Transport) {};

        virtual void Run() override;

        void SetServerUrl(std::string serverURL)
        {
            m_ServerURL = serverURL;
        }

        //	ServerEnumCallback methods:
        virtual ssdk::transport_amd::ServerEnumCallback::DiscoveryCtrl AMF_STD_CALL OnServerDiscovered(ssdk::transport_amd::ServerParameters* server) override;
        virtual ssdk::transport_amd::ServerEnumCallback::DiscoveryCtrl AMF_STD_CALL OnConnectionRefused() override;

        void Wake();

    protected:
        void FindServerAndConnect();

    protected:
        ssdk::transport_common::ClientTransport::Ptr m_Transport;
        std::string                 m_ServerURL;
        amf::AMFEvent               m_WakeUpEvent;
    };

    AMFSize         m_DisplayResolution = {};
    AMFRect         m_DisplayViewport = {};
    int             m_DisplayRefreshRate = 0;

    std::string     m_Vendor;
    std::string     m_Model;
    std::string     m_DeviceID;

    std::string     m_WritablePath;

    std::string     m_ServerUrl;
    int             m_Port = 1235;

    bool            m_Encryption = false;
    std::string     m_Password;

    bool            m_Logging = false;

    amf::AMFContextPtr                      m_Context;
    amf::AMF_MEMORY_TYPE                    m_MemoryType = amf::AMF_MEMORY_TYPE::AMF_MEMORY_UNKNOWN;

    ssdk::util::AVSynchronizer::Ptr         m_AVSynchronizer;
    ssdk::video::VideoDispatcher::Ptr       m_VideoDispatcher;
    ssdk::video::VideoReceiver::Ptr         m_VideoPipeline;

    ssdk::audio::AudioDispatcher::Ptr       m_AudioDispatcher;
    ssdk::audio::AudioReceiverPipeline::Ptr m_AudioPipeline;
    ssdk::audio::AudioPresenterAndroid::Ptr m_AudioPresenter;

    ssdk::ctls::ControllerManager::Ptr      m_ControllerManager;
    ssdk::ctls::MouseCursor::Ptr            m_MouseCursor;

    ssdk::transport_common::ClientTransport::Ptr m_Transport;
    ConnectThread::Ptr                      m_pConnectThread;

    ssdk::util::ClientStatsManager::Ptr     m_StatsManager;

    ssdk::util::JavaObjectHolder            m_Activity;
};
