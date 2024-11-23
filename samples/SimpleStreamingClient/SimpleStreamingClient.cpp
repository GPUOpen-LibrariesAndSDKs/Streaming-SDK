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

#include "SimpleStreamingClient.h"

#include "amf/public/common/AMFFactory.h"
#include "amf/public/common/Thread.h"
#include "amf/public/common/TraceAdapter.h"

#include "amf/public/common/AMFSTL.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>

static constexpr const wchar_t* const AMF_FACILITY = L"SimpleStreamingClient";

const wchar_t* PARAM_NAME_URL = L"server";
const wchar_t* PARAM_NAME_PORT = L"port";
const wchar_t* PARAM_NAME_ENCRYPTION = L"encrypted";
const wchar_t* PARAM_NAME_PASSPHRASE = L"pass";
const wchar_t* PARAM_NAME_UVD_INSTANCE = L"uvd";
const wchar_t* PARAM_NAME_PRESERVE_ASPECT = L"PreserveAspectRatio";
const wchar_t* PARAM_NAME_HQUPSCALE = L"HQUpscale";
const wchar_t* PARAM_NAME_VIDEO_DENOISER = L"VideoDenoiser";
const wchar_t* PARAM_NAME_DEVICE_ID = L"DeviceID";
const wchar_t* PARAM_NAME_RELATIVE_MOUSE_CAPTURE = L"RelativeMouse";
const wchar_t* PARAM_NAME_DATAGRAM_SIZE = L"datagramSize";
const wchar_t* PARAM_NAME_SHOW_CURSOR = L"ShowCursor";
const wchar_t* PARAM_NAME_LOGFILE = L"LOGFILE";

// Default parameter values
static constexpr const unsigned short DEFAULT_PORT = 1235;
static constexpr const int64_t  DATAGRAM_SIZE = 65507;
static constexpr const amf_uint DEFAULT_TURNAROUND_LATENCY_PERIOD = 16;
static constexpr const char* DEFAULT_DISPLAY_MODEL = "Test Display Model";
static constexpr const wchar_t* DEFAULT_LOG_FILENAME = L"./SimpleStreamingClient.log";

SimpleStreamingClient* SimpleStreamingClient::m_This = nullptr;

SimpleStreamingClient::SimpleStreamingClient()
{
    if (m_This == nullptr)
    {
        m_This = this;
    }
    SetParamDescription(PARAM_NAME_URL, ParamCommon, L"Server url, server auto discovered when not specified", nullptr);
    SetParamDescription(PARAM_NAME_PORT, ParamCommon, L"Specify a UDP/TCP port the server is listening on, default = 1235", ParamConverterInt64);
    SetParamDescription(PARAM_NAME_ENCRYPTION, ParamCommon, L"Enable AES encryption of traffic between client and server (true, false), default=false", ParamConverterBoolean);
    SetParamDescription(PARAM_NAME_PASSPHRASE, ParamCommon, L"Specify a passphrase for AES encryption", nullptr);
    SetParamDescription(PARAM_NAME_UVD_INSTANCE, ParamCommon, L"UVD engine instance, default = 0", ParamConverterInt64);
    SetParamDescription(PARAM_NAME_PRESERVE_ASPECT, ParamCommon, L"Preserve video aspect ratio (true, false), default=true", ParamConverterBoolean);
    SetParamDescription(PARAM_NAME_HQUPSCALE, ParamCommon, L"Enable high quality upscaler (true, false), default=true", ParamConverterBoolean);
    SetParamDescription(PARAM_NAME_VIDEO_DENOISER, ParamCommon, L"Enable video denoiser (true, false), default=true", ParamConverterBoolean);
    SetParamDescription(PARAM_NAME_DEVICE_ID, ParamCommon, L"Client device ID, GUID will be generated if empty", nullptr);
    SetParamDescription(PARAM_NAME_RELATIVE_MOUSE_CAPTURE, ParamCommon, L"Enable relative mouse movement capture (true, false) default = true", ParamConverterBoolean);
    SetParamDescription(PARAM_NAME_DATAGRAM_SIZE, ParamCommon, L"Specify UDP datagram size, default = 65507", ParamConverterInt64);
    SetParamDescription(PARAM_NAME_SHOW_CURSOR, ParamCommon, L"Show cursor sent by server, (true, false), default = true", ParamConverterBoolean);
    SetParamDescription(PARAM_NAME_LOGFILE, ParamCommon, L"Specify log file path, default = ./SimpleStreamingClient.log", nullptr);
}

SimpleStreamingClient::~SimpleStreamingClient()
{
    m_This = nullptr;
}

SimpleStreamingClient* SimpleStreamingClient::GetInstance()
{
    return m_This;
}

bool SimpleStreamingClient::Init()
{
    bool result = false;
    if (this == m_This)
    {
        if (InitAMF() == true)
        {
            if (parseCmdLineParameters(this) != true)
            {
                AMFTraceError(AMF_FACILITY, L"parseCmdLineParameters() failed");
            }
            else
            {
                if (OnAppInit() != true)
                {
                    AMFTraceError(AMF_FACILITY, L"Failed to initialize application");
                }
                else if (InitVideo() != true)
                {
                    AMFTraceError(AMF_FACILITY, L"Failed to initialize video components");
                }
                else if (InitAudio() != true)
                {
                    AMFTraceError(AMF_FACILITY, L"Failed to initialize audio components");
                }
                else if (InitStatistics() != true)
                {
                    AMFTraceError(AMF_FACILITY, L"Failed to initialize statistics manager");
                }
                else if (InitCursor() != true)
                {
                    AMFTraceError(AMF_FACILITY, L"Failed to initialize cursor");
                }
                else if (InitNetwork() != true)
                {
                    AMFTraceError(AMF_FACILITY, L"Failed to initialize network");
                }
                else if (InitControllers() != true)
                {
                    AMFTraceError(AMF_FACILITY, L"Failed to initialize controllers");
                }
                else
                {
                    result = true;
                }
            }
        }
        else
        {
            std::cerr << "Failed to initialize AMF. Check AMD graphics driver installation or, if running on a non-AMD platform, ensure that amfrt64.dll and amfrtlte64.dll are next to the executable or in the PATH\n";
        }
    }
    return result;
}

bool SimpleStreamingClient::Terminate()
{
    bool result;
    if (this == m_This)
    {
        if (m_AMFInitalized == true)
        {
            //  Termination order is important, do not change!
            TerminateStatistics();
            TerminateNetwork();
            TerminateAudio();
            TerminateVideo();
            TerminateControllers();
            OnAppTerminate();
        
            TerminateAMF();
        }
        result = true;
    }
    else
    {
        result = false;
    }
    return result;
}

void SimpleStreamingClient::ResizePresenter(uint32_t width, uint32_t height)
{
    m_PresenterResolution.width = width;
    m_PresenterResolution.height = height;
    m_VideoPresenter->SetFrameSize(width, height);
    AMFTraceInfo(AMF_FACILITY, L"Video presenter resolution set to %ux%u", width, height);
}

bool SimpleStreamingClient::OnAppInit()
{
    // trace to file
    std::wstring logFilePath = DEFAULT_LOG_FILENAME;
    GetParamWString(PARAM_NAME_LOGFILE, logFilePath);
    if (logFilePath != L"null")
    {
        if (logFilePath.length() != 0)
        {
            g_AMFFactory.GetTrace()->SetPath(logFilePath.c_str());
        }
        g_AMFFactory.GetTrace()->EnableWriter(AMF_TRACE_WRITER_FILE, true);
    }
    else
    {
        g_AMFFactory.GetTrace()->EnableWriter(AMF_TRACE_WRITER_FILE, false);
    }

    return true;
}

void SimpleStreamingClient::OnAppTerminate()
{
}

bool SimpleStreamingClient::InitAMF()
{
    bool ret = true;
    AMF_RESULT result = AMF_OK;

    if ((result = g_AMFFactory.Init()) == AMF_OK)
    {
        g_AMFFactory.GetDebug()->AssertsEnable(false);

        g_AMFFactory.GetTrace()->TraceEnableAsync(false);
        g_AMFFactory.GetTrace()->SetGlobalLevel(AMF_TRACE_INFO);
        g_AMFFactory.GetTrace()->SetWriterLevel(AMF_TRACE_WRITER_FILE, AMF_TRACE_INFO);
        g_AMFFactory.GetTrace()->EnableWriter(AMF_TRACE_WRITER_CONSOLE, false);
        g_AMFFactory.GetTrace()->SetWriterLevel(AMF_TRACE_WRITER_DEBUG_OUTPUT, AMF_TRACE_DEBUG);

        g_AMFFactory.GetTrace()->SetWriterLevelForScope(AMF_TRACE_WRITER_DEBUG_OUTPUT, L"AMFDecoderUVDImpl", AMF_TRACE_ERROR);
        g_AMFFactory.GetTrace()->SetWriterLevelForScope(AMF_TRACE_WRITER_DEBUG_OUTPUT, L"AMFVideoConverterImpl", AMF_TRACE_ERROR);
        g_AMFFactory.GetTrace()->SetWriterLevelForScope(AMF_TRACE_WRITER_DEBUG_OUTPUT, L"VideoEnhancerImpl", AMF_TRACE_ERROR);
        g_AMFFactory.GetTrace()->SetWriterLevelForScope(AMF_TRACE_WRITER_DEBUG_OUTPUT, L"AMFVideoStreamParserImpl", AMF_TRACE_ERROR);
        g_AMFFactory.GetTrace()->SetWriterLevelForScope(AMF_TRACE_WRITER_DEBUG_OUTPUT, L"AMFHQScalerImpl", AMF_TRACE_ERROR);


        result = g_AMFFactory.GetFactory()->CreateContext(&m_Context);
        if (m_Context == nullptr)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to create AMFContext, result=%s", amf::AMFGetResultText(result));
            ret = false;
        }
        else
        {
            amf_increase_timer_precision();
            m_AMFInitalized = true;
            ret = true;
            AMFTraceDebug(AMF_FACILITY, L"AMF runtime initialized successfully");
        }
    }
    else
    {
        ret = false;
    }
    return ret;
}

void SimpleStreamingClient::TerminateAMF()
{
    AMFTraceDebug(AMF_FACILITY, L"Terminating and unloading AMF...");
    if (m_AMFInitalized == true)
    {
        amf_restore_timer_precision();
        m_Context = nullptr;
        g_AMFFactory.Terminate();
        m_AMFInitalized = false;
    }
}

bool SimpleStreamingClient::InitNetwork()
{
    bool result = true;
    if (m_VideoDispatcher == nullptr)
    {
        AMFTraceError(AMF_FACILITY, L"Cannot initialize the network stack before video pipeline is created");
        result = false;
    }
    else if (m_AudioDispatcher == nullptr)
    {
        AMFTraceError(AMF_FACILITY, L"Cannot initialize the network stack before audio pipeline is created");
        result = false;
    }
    else
    {
        m_Transport = ssdk::transport_common::ClientTransport::Ptr(new ssdk::transport_amd::ClientTransportImpl);
        static_cast<ssdk::transport_amd::ClientTransportImpl*>(m_Transport.get())->SetStatsManager(m_StatsManager);
        m_pConnectThread = ConnectThread::Ptr(new ConnectThread(m_Transport));

        ssdk::transport_amd::ClientTransportImpl::ClientInitParametersAMD initParams;
        
        // set temporary stub callbacks
        initParams.SetInputControllerCallback(m_ControllerManager.get());
        initParams.SetCursorCallback(m_ControllerManager->GetCursor().get());
        initParams.SetConnectionManagerCallback(this);

        // set known paramteres
        initParams.SetContext(m_Context);
        initParams.SetVideoReceiverCallback(m_VideoDispatcher.get());
        initParams.SetAudioReceiverCallback(m_AudioDispatcher.get());
        initParams.SetServerEnumCallback(m_pConnectThread.get());
        initParams.SetDisplayWidth(0);
        initParams.SetDisplayHeight(0);
        initParams.SetDisplayClientWidth(0);
        initParams.SetDisplayClientHeight(0);
        initParams.SetFramerate((float)m_DisplayRefreshRate.num / (float)m_DisplayRefreshRate.den);

        // set paramters specified from command line

        //  Server URL
        std::string url;
        if (GetParamString(PARAM_NAME_URL, url) == AMF_OK)
        {
            m_pConnectThread->SetServerUrl(url);
        }

        //  Discovery port
        int64_t port = DEFAULT_PORT;
        GetParam(PARAM_NAME_PORT, port);
        initParams.SetDiscoveryPort(uint16_t(port));

        int64_t datagramSize = DATAGRAM_SIZE;
        GetParam(PARAM_NAME_DATAGRAM_SIZE, datagramSize);
        initParams.SetDatagramSize(datagramSize);

        //  Unique device ID
        std::string deviceID;
        GetParamString(PARAM_NAME_DEVICE_ID, deviceID);
        if (deviceID.empty())
        {
            deviceID = GenerateGUID();
        }
        initParams.SetID(deviceID);

        //  Encryption
        bool encryption = false;
        GetParam(PARAM_NAME_ENCRYPTION, encryption);
        if (encryption == true)
        {
            std::string passphrase;
            if (GetParamString(PARAM_NAME_PASSPHRASE, passphrase) == AMF_OK)
            {
                initParams.SetPassphrase(passphrase);
            }
            else
            {
                AMFTraceError(AMF_FACILITY, L"Cannot initialize the network stack because encryption password was not provided");
                result = false;
            }
        }

        // hardcoded default paramters
        initParams.SetDisplayModel(DEFAULT_DISPLAY_MODEL);
        initParams.SetBitrate(0);
        initParams.SetLatencyMessagePeriod(DEFAULT_TURNAROUND_LATENCY_PERIOD);

        if (result == true)
        {
            m_StatsManager->SetTransport(m_Transport.get());

            ssdk::transport_common::Result networkResult = m_Transport->Start(initParams);
            if (networkResult != ssdk::transport_common::Result::OK)
            {
                AMFTraceError(AMF_FACILITY, L"Cannot initialize the network stack: failed to create transport client ");
            }
            else
            {
                m_pConnectThread->Start();
                AMFTraceDebug(AMF_FACILITY, L"Started connection thread ");
            }

            result = networkResult == ssdk::transport_common::Result::OK;
        }
    }
    return result;
}

void SimpleStreamingClient::ConnectThread::Run()
{
    while (StopRequested() == false)
    {
        FindServerAndConnect();
    }
}

void SimpleStreamingClient::ConnectThread::FindServerAndConnect()
{
    ssdk::transport_common::Result result = ssdk::transport_common::Result::OK;

    if (m_ServerURL == "")
    {
        result = m_Transport->FindServers();
        // m_ServerURL populated in OnServerDiscovered
        if (result != ssdk::transport_common::Result::OK)
        {
            AMFTraceError(AMF_FACILITY, L"Connect Thread: Server discovery did not succeed");
        }
        else
        {
            AMFTraceInfo(AMF_FACILITY, L"Connect Thread: Server discovery successful: (%S)", m_ServerURL.c_str());
        }
    }

    if (m_ServerURL == "")
    {
        AMFTraceError(AMF_FACILITY, L"Connect Thread can't connect: server URL is not provided as parameter or through discovery");
        result = ssdk::transport_common::Result::FAIL;
    }
    else
    {
        result = m_Transport->Connect(m_ServerURL.c_str());
        if (result != ssdk::transport_common::Result::OK)
        {
            AMFTraceError(AMF_FACILITY, L"Connect Thread: Failed to connect to server (%S), result = %d", m_ServerURL.c_str(), result);
        }
        else
        {
            result = m_Transport->SubscribeToAudioStream();
            if (result != ssdk::transport_common::Result::OK)
            {
                AMFTraceError(AMF_FACILITY, L"Connect Thread: Failed to subscribe to server (%S) audio stream, result = %d", m_ServerURL.c_str(), result);
            }
            else
            {
                AMFTraceDebug(AMF_FACILITY, L"Connect Thread connected succesfully to server (%S) audio stream", m_ServerURL.c_str());
            }
            result = m_Transport->SubscribeToVideoStream();
            if (result != ssdk::transport_common::Result::OK)
            {
                AMFTraceError(AMF_FACILITY, L"Connect Thread: Failed to subscribe to server (%S) video stream, result = %d", m_ServerURL.c_str(), result);
            }
            else
            {
                AMFTraceDebug(AMF_FACILITY, L"Connect Thread connected succesfully to server (%S) video stream", m_ServerURL.c_str());
                RequestStop();
            }
        }
    }

    amf_sleep(10);
}

//	ServerEnumCallback methods start
ssdk::transport_amd::ServerEnumCallback::DiscoveryCtrl SimpleStreamingClient::ConnectThread::OnServerDiscovered(ssdk::transport_amd::ServerParameters* server)
{
    m_ServerURL = server->GetUrl();
    AMFTraceInfo(AMF_FACILITY, L"Server discovered: %S", m_ServerURL.c_str());
    return ssdk::transport_amd::ServerEnumCallback::DiscoveryCtrl::ABORT;
}

ssdk::transport_amd::ServerEnumCallback::DiscoveryCtrl SimpleStreamingClient::ConnectThread::OnConnectionRefused()
{
    return ssdk::transport_amd::ServerEnumCallback::ServerEnumCallback::DiscoveryCtrl::CONTINUE;
}



void SimpleStreamingClient::TerminateNetwork()
{
    if (m_pConnectThread != nullptr)
    {
        m_pConnectThread->RequestStop();
        m_pConnectThread->WaitForStop();
        m_pConnectThread = nullptr;
    }
    
    if (m_Transport != nullptr)
    {
        m_Transport->Shutdown();
        m_Transport = nullptr;
    }
    AMFTraceDebug(AMF_FACILITY, L"Network terminated");
}

bool SimpleStreamingClient::InitVideo()
{
    bool result = false;
    if (m_VideoPresenter == nullptr)
    {
        AMFTraceError(AMF_FACILITY, L"Video presenter cannot be NULL");
        result = false;
    }
    else if (m_AVSynchronizer == nullptr)
    {
        AMFTraceError(AMF_FACILITY, L"AVSynchronizer cannot be NULL");
        result = false;
    }
    else
    {
        m_VideoPresenter->Terminate();
        AMF_RESULT amfRes = m_VideoPresenter->Init(m_PresenterResolution.width, m_PresenterResolution.height);
        if (amfRes != AMF_OK)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to initialize video presenter, resolution %dx%d, result=%s", m_PresenterResolution.width, m_PresenterResolution.height, amf::AMFGetResultText(amfRes));
            result = false;
        }
        else
        {
            //  Presenter is created and configured in the child class which overrides this method:
            m_DisplayRefreshRate = m_VideoPresenter->GetDisplayRefreshRate();
            m_PresenterFormat = m_VideoPresenter->GetInputFormat();
            m_VideoPipeline = ssdk::video::VideoReceiverPipeline::Ptr(new ssdk::video::VideoReceiverPipeline(m_Context, m_VideoPresenter, m_AVSynchronizer));
            int64_t uvdInstance = 0;
            GetParam(PARAM_NAME_UVD_INSTANCE, uvdInstance);
            bool reserveAspectRation = true;
            GetParam(PARAM_NAME_PRESERVE_ASPECT, reserveAspectRation);
            bool hqUpscale = true;
            GetParam(PARAM_NAME_HQUPSCALE, hqUpscale);
            bool videoDenoiser = true;
            GetParam(PARAM_NAME_VIDEO_DENOISER, videoDenoiser);
            if ((amfRes = m_VideoPipeline->Init(uvdInstance, reserveAspectRation, hqUpscale, videoDenoiser)) != AMF_OK)
            {
                AMFTraceError(AMF_FACILITY, L"Failed to initialize video pipeline, result=%s", amf::AMFGetResultText(amfRes));
                result = false;
            }
            else
            {
                m_VideoDispatcher = ssdk::video::VideoDispatcher::Ptr(new ssdk::video::VideoDispatcher(m_Context, m_VideoPipeline));
                m_VideoDispatcher->RegisterPipelineForStream(ssdk::transport_common::DEFAULT_STREAM, m_VideoPipeline);
                AMFTraceDebug(AMF_FACILITY, L"Video input pipeline initialized");
                result = true;
            }
        }
    }
    return result;
}

void SimpleStreamingClient::TerminateVideo()
{
    if (m_VideoDispatcher != nullptr)
    {
        m_VideoDispatcher->UnregisterPipelineForStream(0);
        m_VideoDispatcher = nullptr;
    }
    if (m_VideoPipeline != nullptr)
    {
        m_VideoPipeline->Terminate();
        m_VideoPipeline = nullptr;
        m_StatsManager = nullptr;
    }
    if (m_VideoPresenter != nullptr)
    {
        m_VideoPresenter->Terminate();
    }
    AMFTraceDebug(AMF_FACILITY, L"Video input pipeline terminated");
}

bool SimpleStreamingClient::InitAudio()
{
    bool result = false;
    if (m_AudioPresenter == nullptr)
    {
        AMFTraceError(AMF_FACILITY, L"Video presenter cannot be NULL");
        result = false;
    }
    else if (m_AVSynchronizer == nullptr)
    {
        AMFTraceError(AMF_FACILITY, L"AVSynchronizer cannot be NULL");
        result = false;
    }
    else
    {
        AMF_RESULT amfRes = m_AudioPresenter->Init();
        if (amfRes != AMF_OK)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to initialize audio presenter, result=%s", amf::AMFGetResultText(amfRes));
            result = false;
        }
        else
        {
            m_AudioPipeline = ssdk::audio::AudioReceiverPipeline::Ptr(new ssdk::audio::AudioReceiverPipeline(m_Context, m_AudioPresenter, m_AVSynchronizer));
            amfRes = m_AudioPipeline->Init();
            if (amfRes != AMF_OK)
            {
                AMFTraceError(AMF_FACILITY, L"Failed to initialize audio pipeline, result=%s", amf::AMFGetResultText(amfRes));
            }
            else
            {
                m_AudioDispatcher = ssdk::audio::AudioDispatcher::Ptr(new ssdk::audio::AudioDispatcher(m_Context, m_AudioPipeline));
                m_AudioDispatcher->RegisterPipelineForStream(0, m_AudioPipeline);
                AMFTraceDebug(AMF_FACILITY, L"Audio input pipeline initialized");
                result = true;
            }
        }
    }
    return result;
}

void SimpleStreamingClient::TerminateAudio()
{
    m_AudioDispatcher = nullptr;
    if (m_AudioPipeline != nullptr)
    {
        m_AudioPipeline->Terminate();
        m_AudioPipeline = nullptr;
    }
    AMFTraceDebug(AMF_FACILITY, L"Audio input pipeline terminated");
}

bool SimpleStreamingClient::InitCursor()
{
    m_ControllerManager = ssdk::ctls::ControllerManager::Ptr(new ssdk::ctls::ControllerManager());
    return (m_ControllerManager != nullptr);
}

bool SimpleStreamingClient::InitControllers()
{
    if (m_ControllerManager == nullptr)
    {
        return false;
    }

    m_ControllerManager->SetClientTransport(m_Transport);

    AMFTraceDebug(AMF_FACILITY, L"Controller Manager initialized");

    return true;
}

void SimpleStreamingClient::TerminateControllers()
{
    m_ControllerManager = nullptr;

    AMFTraceDebug(AMF_FACILITY, L"Controller Manager terminated");
}

bool SimpleStreamingClient::InitStatistics()
{
    bool result = false;
    if (m_VideoPipeline == nullptr)
    {
        AMFTraceError(AMF_FACILITY, L"InitStatistics: Video pipeline cannot be NULL");
        result = false;
    }
    else if (m_AVSynchronizer == nullptr)
    {
        AMFTraceError(AMF_FACILITY, L"InitStatistics: AVSynchronizer cannot be NULL");
        result = false;
    }
    else
    {
        m_StatsManager = ssdk::util::ClientStatsManager::Ptr(new ssdk::util::ClientStatsManager());
        m_AVSynchronizer->SetStatsManager(m_StatsManager);
        m_VideoPipeline->SetStatsManager(m_StatsManager);
        result = true;
    }

    AMFTraceDebug(AMF_FACILITY, L"Statistics collector initialized");

    return result;
}

void SimpleStreamingClient::TerminateStatistics()
{
    if (m_VideoPipeline != nullptr)
    {
        m_VideoPipeline->SetStatsManager(nullptr);
    }

    if (m_AVSynchronizer != nullptr)
    {
        m_AVSynchronizer->SetStatsManager(nullptr);
    }

    if (m_Transport != nullptr)
    {
        static_cast<ssdk::transport_amd::ClientTransportImpl*>(m_Transport.get())->SetStatsManager(nullptr);
    }

    if (m_StatsManager != nullptr)
    {
        m_StatsManager->SetTransport(nullptr);
        m_StatsManager = nullptr;
    }
    
    AMFTraceDebug(AMF_FACILITY, L"Statistics collector terminated");
}

// ------------------------------------------------------------------------------------------------------------------------
// Implementation of ConnectionManagerCallback interface
// ------------------------------------------------------------------------------------------------------------------------
ssdk::transport_amd::ClientTransport::ConnectionManagerCallback::DiscoveryAction SimpleStreamingClient::OnServerDiscovered(const ssdk::transport_amd::ClientTransport::ServerDescriptor& /*server*/)
{
    return ssdk::transport_amd::ClientTransport::ConnectionManagerCallback::DiscoveryAction::STOP;
}

void SimpleStreamingClient::OnDiscoveryComplete()
{
}

void SimpleStreamingClient::OnConnectionEstablished(const ssdk::transport_amd::ClientTransport::ServerDescriptor& /*server*/)
{
    if (m_ControllerManager != nullptr)
    {
        m_ControllerManager->Start();
    }
}

void SimpleStreamingClient::OnConnectionTerminated(TerminationReason /*reason*/)
{
    if (m_ControllerManager != nullptr)
    {
        m_ControllerManager->Stop();
    }

    if (m_pConnectThread != nullptr && m_pConnectThread->IsRunning() == false)
    {
        m_pConnectThread->Start();
    }
}
