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

#include "SimpleStreamingClient.h"

#include "sdk/controllers/client/android/GameControllerAndroid.h"
#include "sdk/controllers/client/android/KeyboardControllerAndroid.h"
#include "sdk/controllers/client/android/TouchControllerAndroid.h"
#include "sdk/controllers/client/android/MouseControllerAndroid.h"
#include "sdk/controllers/client/android/MouseCursorAndroid.h"

#include "amf/public/common/AMFFactory.h"
#include "amf/public/common/Thread.h"
#include "amf/public/common/TraceAdapter.h"
#include "amf/public/common/AMFSTL.h"

static constexpr const wchar_t* AMF_FACILITY = L"SimpleStreamingClient";

static SimpleStreamingClient*   m_This = nullptr;
static bool                     m_AMFInitalized = false;
static constexpr const int64_t  DATAGRAM_SIZE = 1024;
static constexpr const amf_uint DEFAULT_TURNAROUND_LATENCY_PERIOD = 16;

SimpleStreamingClient& SimpleStreamingClient::GetInstance()
{
    if (m_This == nullptr)
    {
        m_This = new SimpleStreamingClient;
    }
    return *m_This;
}

void SimpleStreamingClient::Destroy()
{
    if (m_This != nullptr)
    {
        delete m_This;
        m_This = nullptr;
    }
}

SimpleStreamingClient::SimpleStreamingClient()
{
}

SimpleStreamingClient::~SimpleStreamingClient()
{
    Terminate();
}

SimpleStreamingClient::Result SimpleStreamingClient::Init(ssdk::util::JavaObjectHolder&& activity, ssdk::util::ANativeWindowHolder&& decodeWindow, ssdk::util::ANativeWindowHolder&& cursorWindow,
                                 int displayWidth, int displayHeight, int displayRefreshRate, int displayViewportLeft, int displayViewportTop, int displayViewportRight, int displayViewportBottom,
                                 const std::string& vendor, const std::string& model, const std::string& deviceID, const std::string& writablePath,
                                 const std::string& serverUrl, int port,
                                 bool encryption, const std::string& password, bool enableLogging)
{
    SimpleStreamingClient::Result result = SimpleStreamingClient::Result::OK;
    if (this == m_This)
    {
        m_DisplayResolution.width = displayWidth;
        m_DisplayResolution.height = displayHeight;
        m_DisplayRefreshRate = displayRefreshRate;

        m_DisplayViewport.left = displayViewportLeft;
        m_DisplayViewport.top = displayViewportTop;
        m_DisplayViewport.right = displayViewportRight;
        m_DisplayViewport.bottom = displayViewportBottom;

        m_Vendor = vendor;
        m_Model = model;
        m_DeviceID = deviceID;
        m_WritablePath = writablePath;
        m_ServerUrl = serverUrl;
        const char* url = serverUrl.c_str();
        m_Port = port;
        m_Encryption = encryption;
        m_Password = password;
        m_Logging = enableLogging;

        m_Activity = std::move(activity);

        if (InitAMF() != true)
        {
            result = SimpleStreamingClient::Result::FAILED_INIT_AMF;
        }
        else if (InitApp() != true)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to initialize application");
            result = SimpleStreamingClient::Result::FAILED_INIT_APP;
        }
        else if (InitVideo(std::move(decodeWindow)) != true)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to initialize video components");
            result = SimpleStreamingClient::Result::FAILED_INIT_VIDEO;
        }
        else if (InitAudio() != true)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to initialize audio components");
            result = SimpleStreamingClient::Result::FAILED_INIT_AUDIO;
        }
        else if (InitStatistics() != true)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to initialize statistics manager");
            result = SimpleStreamingClient::Result::FAILED_INIT_STATS;
        }
        else if (InitNetwork() != true)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to initialize network");
            result = SimpleStreamingClient::Result::FAILED_INIT_NET;
        }
        else if (InitControllers() != true)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to initialize controllers");
            result = SimpleStreamingClient::Result::FAILED_INIT_CTRL;
        }
        else
        {
            AMFTraceInfo(AMF_FACILITY, L"Initialization successful");
            result = SimpleStreamingClient::Result::OK;
        }
    }
    return result;
}

SimpleStreamingClient::Result SimpleStreamingClient::Terminate()
{
    SimpleStreamingClient::Result result = SimpleStreamingClient::Result::OK;

    TerminateControllers();
    TerminateNetwork();
    TerminateStatistics();
    TerminateAudio();
    TerminateVideo();
    TerminateApp();
    TerminateAMF();
    return result;
}

bool SimpleStreamingClient::InitApp()
{
    bool result = true;
    //  videoPresenter must be nullptr since on Android the decoder puts out the image directly onto the screen
    m_AudioPresenter = ssdk::audio::AudioPresenterAndroid::Ptr(new ssdk::audio::AudioPresenterAndroid());
    m_AVSynchronizer = ssdk::util::AVSynchronizer::Ptr(new ssdk::util::AVSynchronizer(nullptr, m_AudioPresenter));
    m_MouseCursor = ssdk::ctls::MouseCursor::Ptr(new ssdk::ctls::MouseCursorAndroid(m_Activity));
    m_ControllerManager = ssdk::ctls::ControllerManager::Ptr(new ssdk::ctls::ControllerManager());
    return result;
}

void SimpleStreamingClient::TerminateApp()
{
    m_ControllerManager = nullptr;
    m_AVSynchronizer = nullptr;
    m_AudioPresenter = nullptr;
}

bool SimpleStreamingClient::InitAMF()
{
    bool result = true;
    // AMF initialization; before using SDK, AMF needs to be initialized
    // AMFFactory and AMFTrace contain static variables that don't get reset when the AMF runtime .so
    // is unloaded, so we only need to load it once
    if (m_AMFInitalized == false)
    {
        if (g_AMFFactory.Init() != AMF_OK)
        {
            result = false;
        }
        else
        {
            int32_t globalLogLevel;
#ifdef _DEBUG
            globalLogLevel = AMF_TRACE_DEBUG;
#else
            globalLogLevel = AMF_TRACE_INFO;
#endif

            g_AMFFactory.GetDebug()->AssertsEnable(false);
            g_AMFFactory.GetTrace()->TraceEnableAsync(false);
            g_AMFFactory.GetTrace()->SetGlobalLevel(globalLogLevel);
            g_AMFFactory.GetTrace()->SetWriterLevel(AMF_TRACE_WRITER_DEBUG_OUTPUT, AMF_TRACE_DEBUG);
            g_AMFFactory.GetTrace()->SetWriterLevel(AMF_TRACE_WRITER_CONSOLE, globalLogLevel);
            if (m_Logging == true)
            {
                g_AMFFactory.GetTrace()->SetWriterLevel(AMF_TRACE_WRITER_FILE, globalLogLevel);
                std::string logPath = m_WritablePath + "/SimpleStreamingClient.log";
                g_AMFFactory.GetTrace()->SetPath(amf::amf_from_utf8_to_unicode(logPath.c_str()).c_str());
            }
            m_AMFInitalized = true;
        }
    }

    if (result ==  true)
    {
        AMF_RESULT amfResult = g_AMFFactory.GetFactory()->CreateContext(&m_Context);
        if (m_Context == nullptr)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to create AMFContext, result=%s", amf::AMFGetResultText(amfResult));
            result = false;
        }
        else
        {
            amf_increase_timer_precision();
            result = true;
            AMFTraceDebug(AMF_FACILITY, L"AMF runtime initialized successfully");
        }

    }
    return result;
}

void SimpleStreamingClient::TerminateAMF()
{
    AMFTraceDebug(AMF_FACILITY, L"Terminating and unloading AMF...");
    if (m_AMFInitalized == true)
    {
        amf_restore_timer_precision();
        m_Context = nullptr;
//        g_AMFFactory.Terminate();
//        m_AMFInitalized = false;
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
        static_cast<ssdk::transport_amd::ClientTransportImpl *>(m_Transport.get())->SetStatsManager(m_StatsManager);
        m_pConnectThread = ConnectThread::Ptr(new ConnectThread(m_Transport));

        ssdk::transport_amd::ClientTransportImpl::ClientInitParametersAMD initParams;

        initParams.SetInputControllerCallback(m_ControllerManager.get());
        initParams.SetCursorCallback(m_MouseCursor.get());
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
        initParams.SetFramerate((float)m_DisplayRefreshRate);

        // set paramters specified from command line

        //  Server URL
        if (m_ServerUrl.empty() == false)
        {
            m_pConnectThread->SetServerUrl(m_ServerUrl);
        }
        //  Discovery port
        initParams.SetDiscoveryPort(uint16_t(m_Port));
        initParams.SetDatagramSize(DATAGRAM_SIZE);

        //  Unique device ID
        initParams.SetID(m_DeviceID);

        //  Encryption
        if (m_Encryption == true)
        {
            initParams.SetPassphrase(m_Password);
        }

        // hardcoded default paramters
        initParams.SetDisplayModel(m_Model);
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

void SimpleStreamingClient::TerminateNetwork()
{
    AMFTraceDebug(AMF_FACILITY, L"Terminating network...");
    if (m_pConnectThread != nullptr)
    {
        m_pConnectThread->RequestStop();
        m_pConnectThread->Wake();
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

bool SimpleStreamingClient::InitVideo(ssdk::util::ANativeWindowHolder&& decodeWindow)
{
    bool result = true;
    AMF_RESULT amfRes = AMF_OK;
    m_VideoPipeline = ssdk::video::VideoReceiver::Ptr(new ssdk::video::VideoReceiver(m_Context, m_AVSynchronizer));
    if ((amfRes = m_VideoPipeline->Init(std::move(decodeWindow), m_DisplayResolution, m_DisplayViewport, m_DisplayRefreshRate)) != AMF_OK)
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
    }

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
    if (m_AudioPresenter != nullptr)
    {
        m_AudioPresenter->Terminate();
    }
    AMFTraceDebug(AMF_FACILITY, L"Audio input pipeline terminated");
}

bool SimpleStreamingClient::InitControllers()
{
    if (m_ControllerManager != nullptr)
    {
        m_ControllerManager->SetClientTransport(m_Transport);
        m_ControllerManager.get()->AddController(ssdk::ctls::ControllerBase::Ptr(new ssdk::ctls::GameController(m_ControllerManager, m_Activity, 0)));
        m_ControllerManager.get()->AddController(ssdk::ctls::ControllerBase::Ptr(new ssdk::ctls::KeyboardControllerAndroid(m_ControllerManager)));
        m_ControllerManager.get()->AddController(ssdk::ctls::ControllerBase::Ptr(new ssdk::ctls::TouchController(m_ControllerManager, m_VideoPipeline)));
        m_ControllerManager.get()->AddController(ssdk::ctls::ControllerBase::Ptr(new ssdk::ctls::MouseControllerAndroid(m_ControllerManager, m_VideoPipeline)));
        m_ControllerManager->SetCursor(m_MouseCursor);
    }
    return true;
}

void SimpleStreamingClient::TerminateControllers()
{
    if (m_ControllerManager != nullptr)
    {
        m_ControllerManager->SetCursor(nullptr);
        m_ControllerManager->ReleaseModifiers();
        m_ControllerManager->Stop();
    }
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

///////////////////////////////////////////////////////

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

    if (m_ServerURL.empty() == true && StopRequested() == false)
    {
        result = m_Transport->FindServers();
        // m_ServerURL populated in OnServerDiscovered
        if (result != ssdk::transport_common::Result::OK)
        {
            AMFTraceError(AMF_FACILITY, L"Connect Thread: Server discovery did not succeed");
            amf_sleep(10);
        }
        else
        {
            AMFTraceInfo(AMF_FACILITY, L"Connect Thread: Server discovery successful: (%S)", m_ServerURL.c_str());
        }
    }

    //  When discovery is enabled, m_ServerURL will be populated by the call to m_Transport->FindServers()
    if (m_ServerURL.empty() == true)
    {
        AMFTraceError(AMF_FACILITY, L"Connect Thread can't connect: server URL is not provided as parameter or through discovery");
        result = ssdk::transport_common::Result::FAIL;
        amf_sleep(10);
    }
    else if (StopRequested() == false)
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
                amf::AMFLock waitForWakeup(&m_WakeUpEvent);
            }
        }
    }
    else
    {
        AMFTraceDebug(AMF_FACILITY, L"Discovery thread termination requested, exiting...");
    }
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

void SimpleStreamingClient::ConnectThread::Wake()
{
    m_WakeUpEvent.SetEvent();
}

// ------------------------------------------------------------------------------------------------------------------------
// Implementation of ConnectionManagerCallback interface
// ------------------------------------------------------------------------------------------------------------------------
void SimpleStreamingClient::OnConnectionEstablished(const ssdk::transport_amd::ClientTransport::ServerDescriptor& server)
{
    AMFTraceInfo(AMF_FACILITY, L"Connection to server %S at %S established", server.GetName().c_str(), server.GetUrl().c_str());
    if (m_ControllerManager != nullptr)
    {
        m_ControllerManager->Start();
    }
}

void SimpleStreamingClient::OnConnectionTerminated(TerminationReason reason)
{
    const wchar_t* reasonStr = nullptr;
    switch (reason)
    {
    case TerminationReason::CLOSED_BY_CLIENT:
        reasonStr = L"Closed by client";
        break;
    case TerminationReason::CLOSED_BY_SERVER:
        reasonStr = L"Closed by server";
        break;
    case TerminationReason::TIMEOUT:
        reasonStr = L"Timeout";
        break;
    default:
        reasonStr = L"Unknown";
        break;
    }
    AMFTraceInfo(AMF_FACILITY, L"Connection to server terminated, reason: %s", reasonStr);
    if (m_ControllerManager != nullptr)
    {
        m_ControllerManager->Stop();
    }
    if (m_pConnectThread != nullptr && m_pConnectThread->IsRunning() == true)
    {
        m_pConnectThread->Wake();
    }
}

bool SimpleStreamingClient::DispatchInputEvent(const WindowMessage& msg)
{
    bool result = false;
    if (m_ControllerManager != nullptr)
    {
        result = m_ControllerManager->ProcessMessage(msg);
    }
    return result;
}