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
// Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
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

#pragma once

#include "AVStreamer.h"

#include "sdk/video/MonoscopicVideoOutput.h"
#include "sdk/video/VideoTransmitterAdapter.h"

#include "sdk/audio/AudioOutput.h"
#include "sdk/audio/AudioTransmitterAdapter.h"

#include "sdk/transports/transport-common/ServerTransport.h"
#include "sdk/transports/transport-amd/ServerTransportImpl.h"

#include "amf/public/include/core/Context.h"
#include "amf/public/include/core/CurrentTime.h"
#include "amf/public/include/components/Component.h"
#include "amf/public/include/components/CursorCapture.h"
#include "amf/public/common/Thread.h"
#include "amf/public/samples/CPPSamples/common/CmdLineParser.h"

#include "sdk/controllers/server/ControllerManagerSvr.h"

#include <string>
#include <memory>

extern const wchar_t* PARAM_NAME_MONITOR_ID;

class RemoteDesktopServer :
    public ParametersStorage,
    public ssdk::transport_common::ServerTransport::ConnectionManagerCallback
{
public:
    typedef std::unique_ptr<RemoteDesktopServer>    Ptr;

    enum class EStatus
    {
        OK,
        FAIL,
        SKIP
    };

protected:
    RemoteDesktopServer();
public:
    virtual ~RemoteDesktopServer() = default;

    virtual EStatus Init(int argc = 0, const char** argv = nullptr);
    virtual void Terminate();

    virtual bool RunServer();

    virtual void ShutdownRunningServer();

    //  ssdk::transport_common::ServerTransport::ConnectionManagerCallback methods
    virtual ClientAction OnDiscoveryRequest(const char* clientID) override;                                                                  // Called when a discovery request is received, server can either accept or refuse request
    virtual ClientAction OnConnectionRequest(ssdk::transport_common::SessionHandle session, const char* clientID, ClientRole role) override; // Called when a client tries to establish a connection, server can either accept or refuse request
    virtual void OnClientSubscribed(ssdk::transport_common::SessionHandle session) override;                                                 // Called when the client is subscribed and ready to receive streaming
    virtual void OnClientDisconnected(ssdk::transport_common::SessionHandle session, DisconnectReason reason) override;                      // Called when the client is disconnected

protected:
    virtual bool OnAppInit();
    virtual void OnAppTerminate();

    bool InitAMF();
    void TerminateAMF();

    virtual bool InitDirectories() = 0;

    virtual bool InitVideoCapture();
    virtual void TerminateVideoCapture();

    virtual bool InitAudioCapture();
    virtual void TerminateAudioCapture();

    virtual bool InitCursorCapture();
    virtual void TerminateCursorCapture();

    virtual bool InitVideoCodec();
    virtual void TerminateVideoCodec();

    virtual bool InitAudioCodec();
    virtual void TerminateAudioCodec();

    virtual bool InitNetwork();
    virtual void TerminateNetwork();

    virtual bool InitStreamer();
    virtual void TerminateStreamer();

    virtual bool InitControllers();
    virtual void TerminateControllers();

    std::string GetSemaphoreFileName() const;

protected:
    mutable amf::AMFCriticalSection                     m_Guard;

    bool                                                m_AMFInitalized = false;

    std::string                                         m_ShutdownDir;
    uint16_t                                            m_Port = 0;

    amf::AMFContextPtr                                  m_Context;
    amf::AMF_MEMORY_TYPE                                m_MemoryType = amf::AMF_MEMORY_TYPE::AMF_MEMORY_UNKNOWN;

    int64_t                                             m_MaxConcurrentConnections = 1;
    int64_t                                             m_CurConnectionsCnt = 0;

    amf::AMFCurrentTimePtr                              m_CurrentTime;

    ssdk::transport_common::ServerTransport::Ptr        m_Transport;

    amf::AMFComponentPtr                                m_VideoCapture;
    ssdk::video::VideoEncodeEngine::Ptr                 m_VideoEncoder;
    ssdk::video::TransmitterAdapter::Ptr                m_VideoTransmitterAdapter;
    ssdk::video::MonoscopicVideoOutput::Ptr             m_VideoOutput;
    ssdk::transport_common::VideoStreamDescriptor       m_VideoStreamDescriptor;


    amf::AMFComponentPtr                                m_AudioCapture;
    ssdk::audio::AudioEncodeEngine::Ptr                 m_AudioEncoder;
    ssdk::audio::TransmitterAdapter::Ptr                m_AudioTransmitterAdapter;
    ssdk::audio::AudioOutput::Ptr                       m_AudioOutput;
    ssdk::transport_common::AudioStreamDescriptor       m_AudioStreamDescriptor;

    ssdk::util::QoS::Ptr                                m_QoS;

    AVStreamer::Ptr                                     m_AVStreamer;
    bool                                                m_AVStreamerInitialized = false;

    amf::AMFCursorCapturePtr                            m_CursorCapture;
    amf::AMFSurfacePtr                                  m_LastCursor;

    ssdk::ctls::svr::ControllerManager::Ptr             m_pControllerManager;
};