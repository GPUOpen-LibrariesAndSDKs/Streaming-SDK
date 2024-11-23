#pragma once
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

#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// Windows Header Files
#include <tchar.h>
#include <windows.h>

#else

#endif

#include "sdk/video/VideoDispatcher.h"

#include "sdk/audio/AudioDispatcher.h"

#include "sdk/transports/transport-amd/ClientTransportImpl.h"
#include "sdk/video/MonoscopicVideoInput.h"
#include "sdk/video/VideoReceiverPipeline.h"
#include "sdk/audio/AudioInput.h"
#include "sdk/audio/AudioReceiverPipeline.h"
#include "sdk/util/pipeline/AVSynchronizer.h"
#include "sdk/util/stats/ClientStatsManager.h"

#include "amf/public/include/core/Context.h"
#include "amf/public/samples/CPPSamples/common/CmdLineParser.h"
#include "amf/public/samples/CPPSamples/common/VideoPresenter.h"
#include "amf/public/samples/CPPSamples/common/AudioPresenter.h"

#include "sdk/controllers/client/ControllerManager.h"

#include <memory>
#include <string>

extern const wchar_t* PARAM_NAME_URL;
extern const wchar_t* PARAM_NAME_PORT;
extern const wchar_t* PARAM_NAME_ENCRYPTION;
extern const wchar_t* PARAM_NAME_PASSPHRASE;
extern const wchar_t* PARAM_NAME_UVD_INSTANCE;
extern const wchar_t* PARAM_NAME_PRESERVE_ASPECT;
extern const wchar_t* PARAM_NAME_HQUPSCALE;
extern const wchar_t* PARAM_NAME_VIDEO_DENOISER;
extern const wchar_t* PARAM_NAME_DEVICE_ID;
extern const wchar_t* PARAM_NAME_RELATIVE_MOUSE_CAPTURE;
extern const wchar_t* PARAM_NAME_SHOW_CURSOR;
extern const wchar_t* PARAM_NAME_LOGFILE;

class SimpleStreamingClient :
    public ParametersStorage,
    public ssdk::transport_amd::ClientTransport::ConnectionManagerCallback
{
public:
    typedef std::unique_ptr<SimpleStreamingClient>  Ptr;

protected:
    SimpleStreamingClient();

public:
    virtual ~SimpleStreamingClient();

    static SimpleStreamingClient* GetInstance();

    virtual bool Init();
    virtual bool Terminate();

    virtual void ResizePresenter(uint32_t width, uint32_t height);

    virtual void RunMessageLoop() = 0;

protected:

    class ConnectThread : public ssdk::transport_amd::ServerEnumCallback, public amf::AMFThread
    {
    public:
        typedef std::unique_ptr<ConnectThread> Ptr;
        ConnectThread(ssdk::transport_common::ClientTransport::Ptr Transport) : m_Transport(Transport) {};
            
        virtual void Run();

        void SetServerUrl(std::string serverURL)
        {
            m_ServerURL = serverURL;
        }

        //	ServerEnumCallback methods:
        virtual ssdk::transport_amd::ServerEnumCallback::DiscoveryCtrl AMF_STD_CALL OnServerDiscovered(ssdk::transport_amd::ServerParameters* server) override;
        virtual ssdk::transport_amd::ServerEnumCallback::DiscoveryCtrl AMF_STD_CALL OnConnectionRefused() override;

    protected:
        void FindServerAndConnect();

    protected:
        ssdk::transport_common::ClientTransport::Ptr m_Transport;
        std::string m_ServerURL;
    };
    virtual bool OnAppInit();
    virtual void OnAppTerminate();

    virtual bool InitAMF();
    virtual void TerminateAMF();

    virtual bool InitNetwork();
    virtual void TerminateNetwork();

    virtual bool InitVideo();
    virtual void TerminateVideo();

    virtual bool InitAudio();
    virtual void TerminateAudio();

    virtual bool InitControllers();
    virtual bool InitCursor();
    virtual void TerminateControllers();

    virtual bool InitStatistics();
    virtual void TerminateStatistics();

    virtual std::string GenerateGUID() const = 0;

    // Implementation of ConnectionManagerCallback interface
    virtual DiscoveryAction OnServerDiscovered(const ssdk::transport_amd::ClientTransport::ServerDescriptor& server) override;  // Called every time a new server is found
    virtual void OnDiscoveryComplete() override;                                                                                // Called when the discovery process is complete
    virtual void OnConnectionEstablished(const ssdk::transport_amd::ClientTransport::ServerDescriptor& server) override;        // Called when a connection to a server was established and acknowledged by the server
    virtual void OnConnectionTerminated(TerminationReason reason) override;                                                     // Called when a connection was terminated for any reason

protected:
    static SimpleStreamingClient*           m_This;
    bool                                    m_AMFInitalized = false;

    amf::AMFContextPtr                      m_Context;
    amf::AMF_MEMORY_TYPE                    m_MemoryType = amf::AMF_MEMORY_TYPE::AMF_MEMORY_UNKNOWN;

    ssdk::util::AVSynchronizer::Ptr         m_AVSynchronizer;

    ssdk::video::VideoDispatcher::Ptr       m_VideoDispatcher;
    ssdk::video::VideoReceiverPipeline::Ptr m_VideoPipeline;
    ssdk::util::ClientStatsManager::Ptr     m_StatsManager;
    VideoPresenterPtr                       m_VideoPresenter;
    amf::AMF_SURFACE_FORMAT                 m_PresenterFormat = amf::AMF_SURFACE_FORMAT::AMF_SURFACE_UNKNOWN;
    AMFRate                                 m_DisplayRefreshRate = {};
    AMFSize                                 m_PresenterResolution = {};

    ssdk::audio::AudioDispatcher::Ptr       m_AudioDispatcher;
    ssdk::audio::AudioReceiverPipeline::Ptr m_AudioPipeline;
    AudioPresenterPtr                       m_AudioPresenter;

    ssdk::transport_common::ClientTransport::Ptr m_Transport;
    ConnectThread::Ptr                      m_pConnectThread;

    ssdk::ctls::ControllerManager::Ptr      m_ControllerManager;
};
