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

#include "RemoteDesktopServer.h"

#include "sdk/video/Defines.h"
#include "sdk/video/encoders/GPUEncoderH264.h"
#include "sdk/video/encoders/GPUEncoderHEVC.h"
#include "sdk/video/encoders/GPUEncoderAV1.h"

#include "sdk/audio/encoders/AudioEncoderAAC.h"
#include "sdk/audio/encoders/AudioEncoderOPUS.h"

#include "amf/public/include/components/DisplayCapture.h"
#include "amf/public/include/components/AudioCapture.h"

#include "amf/public/common/CurrentTimeImpl.h"
#include "amf/public/common/AMFFactory.h"
#include "amf/public/common/Thread.h"
#include "amf/public/common/TraceAdapter.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <sys/stat.h>
#include <time.h>
#include <cstdio>

static constexpr const wchar_t* const AMF_FACILITY = L"RemoteDesktopServer";

//  Command line parameters
static constexpr const wchar_t* PARAM_NAME_SHUTDOWN = L"shutdown";

static constexpr const wchar_t* PARAM_NAME_PROTOCOL = L"protocol";
static constexpr const wchar_t* PARAM_NAME_PORT = L"port";
static constexpr const wchar_t* PARAM_NAME_DATAGRAM_SIZE = L"datagramSize";
static constexpr const wchar_t* PARAM_NAME_LOGFILE = L"LOGFILE";
static constexpr const wchar_t* PARAM_NAME_BIND_INTERFACE = L"BindInterface";
static constexpr const wchar_t* PARAM_NAME_SERVER_HOSTNAME = L"Hostname";
static constexpr const wchar_t* PARAM_NAME_MAX_CONNECTIONS = L"Connections";

static constexpr const wchar_t* PARAM_NAME_ENCRYPTION = L"encrypted";
static constexpr const wchar_t* PARAM_NAME_PASSPHRASE = L"pass";

static constexpr const wchar_t* PARAM_NAME_MONITOR_ID = L"MonitorID";
static constexpr const wchar_t* PARAM_NAME_CAPTURE_MODE = L"CaptureMode";
static constexpr const wchar_t* PARAM_NAME_CAPTURE_RATE = L"FrameRate";
static constexpr const wchar_t* PARAM_NAME_VIDEO_CODEC = L"VideoCodec";
static constexpr const wchar_t* PARAM_NAME_RESOLUTION = L"Resolution";
static constexpr const wchar_t* PARAM_NAME_VIDEO_BITRATE = L"VideoBitrate";
static constexpr const wchar_t* PARAM_NAME_PRESERVE_ASPECT_RATIO = L"PreserveAspectRatio";
static constexpr const wchar_t* PARAM_NAME_HDR = L"Hdr";

static constexpr const wchar_t* PARAM_NAME_AUDIO_CODEC = L"AudioCodec";
static constexpr const wchar_t* PARAM_NAME_AUDIO_BITRATE = L"AudioBitrate";
static constexpr const wchar_t* PARAM_NAME_AUDIO_SAMPLING_RATE = L"AudioSamplingRate";
static constexpr const wchar_t* PARAM_NAME_AUDIO_CHANNELS = L"AudioChannels";

static constexpr const wchar_t* PARAM_NAME_QOS_ADJUST_FRAMERATE = L"QOSFramerate";
static constexpr const wchar_t* PARAM_NAME_QOS_ADJUST_BITRATE = L"QOSBitrate";

static constexpr const wchar_t* PARAM_NAME_QOS_MIN_FRAMERATE = L"QOSMinFramerate";
static constexpr const wchar_t* PARAM_NAME_QOS_MIN_BITRATE = L"QOSMinBitrate";

//  Values for PARAM_NAME_CAPTURE_MODE
static constexpr const wchar_t* CAPTURE_MODE_FRAMERATE = L"FRAMERATE";
static constexpr const wchar_t* CAPTURE_MODE_PRESENT = L"PRESENT";
static constexpr const wchar_t* CAPTURE_MODE_ASAP = L"ASAP";

//  Values for PARAM_NAME_PROTOCOL
static constexpr const wchar_t* PROTOCOL_UDP = L"UDP";
static constexpr const wchar_t* PROTOCOL_TCP = L"TCP";

//  Values for PARAM_NAME_VIDEO_CODEC
static constexpr const wchar_t* VIDEO_CODEC_AV1 = L"AV1";
static constexpr const wchar_t* VIDEO_CODEC_HEVC = L"HEVC";
static constexpr const wchar_t* VIDEO_CODEC_AVC = L"AVC";
static constexpr const wchar_t* VIDEO_CODEC_H265 = L"H265";
static constexpr const wchar_t* VIDEO_CODEC_H264 = L"H264";

//  Values for PARAM_NAME_AUDIO_CODEC
static constexpr const wchar_t* AUDIO_CODEC_AAC = L"AAC";
static constexpr const wchar_t* AUDIO_CODEC_OPUS = L"OPUS";

//  Values for PARAM_NAME_AUDIO_CHANNELS
static constexpr const wchar_t* AUDIO_CHANNELS_MONO = L"1";
static constexpr const wchar_t* AUDIO_CHANNELS_STEREO = L"2";
static constexpr const wchar_t* AUDIO_CHANNELS_STEREO_SUB = L"2.1";
static constexpr const wchar_t* AUDIO_CHANNELS_STEREO_CENTER = L"3";
static constexpr const wchar_t* AUDIO_CHANNELS_STEREO_CENTER_SUB = L"3.1";
static constexpr const wchar_t* AUDIO_CHANNELS_SURROUND = L"5";
static constexpr const wchar_t* AUDIO_CHANNELS_SURROUND_SUB = L"5.1";

// Default parameter values
static constexpr const unsigned short DEFAULT_PORT = 1235;
static constexpr const int64_t  DEFAULT_DATAGRAM_SIZE = 65507;
static constexpr const wchar_t* DEFAULT_LOG_FILENAME = L"./RemoteDesktopServer.log";
static constexpr const char*    DEFAULT_BIND_INTERFACE = "*";
static constexpr const char*    DEFAULT_SERVER_HOSTNAME = "RemoteDesktopServer";

// Default QoS values
static constexpr int64_t QOS_DEFAULT_SECONDS_BEFORE_PANIC = 5;

static constexpr int64_t QOS_DEFAULT_THRESHOLD_IDR = 2;
static constexpr int64_t QOS_DEFAULT_THRESHOLD_IDR_PANIC = 5;

static constexpr int64_t QOS_DEFAULT_MAX_ENCODER_QUEUE_DEPTH = 4;
static constexpr int64_t QOS_DEFAULT_MAX_DECODER_QUEUE_DEPTH = 2;

static constexpr int64_t QOS_DEFAULT_FRAMERATE = 60;
static constexpr int64_t QOS_DEFAULT_FRAMERATE_MIN = 15; 
static constexpr int64_t QOS_DEFAULT_FRAMERATE_STEP = 5;
static constexpr int64_t QOS_DEFAULT_FRAMERATE_ADJUSTMENT_PERIOD_SECONDS = 5;

static constexpr int64_t QOS_DEFAULT_BITRATE = 25000000;
static constexpr int64_t QOS_DEFAULT_BITRATE_MIN = 1000000;
static constexpr int64_t QOS_DEFAULT_BITRATE_STEP = QOS_DEFAULT_BITRATE / 10;
static constexpr int64_t QOS_DEFAULT_BITRATE_ADJUSTMENT_PERIOD_SECONDS = 10;

RemoteDesktopServer::RemoteDesktopServer()
{
    SetParamDescription(PARAM_NAME_SHUTDOWN, ParamCommon, L"Shutdown a server instance listening on the specified port", ParamConverterInt64);
    SetParamDescription(PARAM_NAME_PROTOCOL, ParamCommon, L"Specify a transport protocol [UDP, TCP], default = UDP", nullptr);
    SetParamDescription(PARAM_NAME_PORT, ParamCommon, L"Specify a UDP/TCP port the server is listening on, default = 1235", ParamConverterInt64);
    SetParamDescription(PARAM_NAME_DATAGRAM_SIZE, ParamCommon, L"Specify UDP datagram size, default = 65507", ParamConverterInt64);
    SetParamDescription(PARAM_NAME_LOGFILE, ParamCommon, L"Specify log file path, default = ./RemoteDesktopServer.log", nullptr);
    SetParamDescription(PARAM_NAME_BIND_INTERFACE, ParamCommon, L"IP address or interface name the server is accepting connections from, * for any", nullptr);
    SetParamDescription(PARAM_NAME_SERVER_HOSTNAME, ParamCommon, L"Display name of server, \"RemoteDesktopServer\" will be used if empty", nullptr);
    SetParamDescription(PARAM_NAME_MAX_CONNECTIONS, ParamCommon, L"Specify the number of concurrent connections, default = 1", ParamConverterInt64);


    SetParamDescription(PARAM_NAME_ENCRYPTION, ParamCommon, L"Enable AES encryption of traffic between client and server (true, false), default = false", ParamConverterBoolean);
    SetParamDescription(PARAM_NAME_PASSPHRASE, ParamCommon, L"Specify a passphrase for AES encryption", nullptr);

    SetParamDescription(PARAM_NAME_MONITOR_ID, ParamCommon, L"Specify a 0-based index of the monitor to capture video from, default = 0", ParamConverterInt64);
    SetParamDescription(PARAM_NAME_CAPTURE_MODE, ParamCommon, L"Specify capture mode: [framerate, present, asap], default = framerate", nullptr);
    SetParamDescription(PARAM_NAME_CAPTURE_RATE, ParamCommon, L"Specify capture frame rate for the \"framerate\" capture mode, default = 60", ParamConverterInt64);
    SetParamDescription(PARAM_NAME_RESOLUTION, ParamCommon, L"Encoded stream resolution, (w,h) default = 1920,1080", ParamConverterSize);
    SetParamDescription(PARAM_NAME_VIDEO_CODEC, ParamCommon, L"Specify video codec: [av1, hevc, avc], default = hevc", nullptr);
    SetParamDescription(PARAM_NAME_VIDEO_BITRATE, ParamCommon, L"Video bitrate in bits per second, default = 50000000", ParamConverterInt64);
    SetParamDescription(PARAM_NAME_HDR, ParamCommon, L"Enable HDR on video (true, false), default = false", ParamConverterBoolean);
    SetParamDescription(PARAM_NAME_PRESERVE_ASPECT_RATIO, ParamCommon, L"Preserve aspect ratio of the server display (true, false), default = true", ParamConverterBoolean);

    SetParamDescription(PARAM_NAME_AUDIO_CODEC, ParamCommon, L"Specify audio codec: [aac, opus], default = aac", nullptr);
    SetParamDescription(PARAM_NAME_AUDIO_BITRATE, ParamCommon, L"Audio bitrate in bits per second, default = 256000", ParamConverterInt64);
    SetParamDescription(PARAM_NAME_AUDIO_SAMPLING_RATE, ParamCommon, L"Audio sampling rate in Hz, default = 44100", ParamConverterInt64);
    SetParamDescription(PARAM_NAME_AUDIO_CHANNELS, ParamCommon, L"Specify audio channel layout: [1 - mono, 2 - stereo, 2.1 - stereo+sub, 3 - stereo+center, 3.1 - stereo+center+sub, 5.1 - full surround], default = 2 (stereo)", nullptr);

    SetParamDescription(PARAM_NAME_QOS_ADJUST_BITRATE, ParamCommon, L"Enables QoS bitrate adjustment (true, false), default = true", ParamConverterBoolean);
    SetParamDescription(PARAM_NAME_QOS_ADJUST_FRAMERATE, ParamCommon, L"Enables QoS framerate adjustment (true, false), default = true", ParamConverterBoolean);
    SetParamDescription(PARAM_NAME_QOS_MIN_FRAMERATE, ParamCommon, L"Minimum framerate set by QoS, default = 15", ParamConverterInt64);
    SetParamDescription(PARAM_NAME_QOS_MIN_BITRATE, ParamCommon, L"Minimum video bitrate set by QoS (in bits per second), default = 1000000", ParamConverterInt64);


}

RemoteDesktopServer::Status RemoteDesktopServer::Init()
{
    Status result = Status::FAIL;
    if (InitAMF() != true)
    {
        std::cerr << "Failed to initialize AMF. Check AMD graphics driver installation.\n";
    }
    else
    {
        if (parseCmdLineParameters(this) != true)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to parse command line parameters");
        }
        else
        {
            if (InitDirectories() == true)
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

                int64_t port = DEFAULT_PORT;
                if (GetParam(PARAM_NAME_SHUTDOWN, port) == AMF_OK)
                {
                    m_Port = uint16_t(port);
                    result = Status::SKIP;
                }
                else if (GetParam(PARAM_NAME_PORT, port) == AMF_OK)
                {
                    m_Port = uint16_t(port);
                }

                if (result != Status::SKIP)
                {
                    std::remove(GetSemaphoreFileName().c_str());    //  Remove the shutdown semaphore file
                    m_CurrentTime = (new amf::AMFCurrentTimeImpl());
                    m_AVStreamer = AVStreamer::Ptr(new AVStreamer);
                    m_pControllerManager = ssdk::ctls::svr::ControllerManager::Ptr(new ssdk::ctls::svr::ControllerManager());
                    if (OnAppInit() != true)
                    {
                        AMFTraceError(AMF_FACILITY, L"Failed to initialize application");
                    }
                    else if (InitVideoCapture() != true)
                    {
                        AMFTraceError(AMF_FACILITY, L"Failed to create/initialize display capture");
                    }
                    else if (InitVideoCodec() != true)
                    {
                        AMFTraceError(AMF_FACILITY, L"Failed to create video codec");
                    }
                    else if (InitAudioCapture() != true)
                    {
                        AMFTraceError(AMF_FACILITY, L"Failed to create/initialize audio capture");
                    }
                    else if (InitAudioCodec() != true)
                    {
                        AMFTraceError(AMF_FACILITY, L"Failed to create audio codec");
                    }
                    else if (InitCursorCapture() != true)
                    {
                        AMFTraceError(AMF_FACILITY, L"Failed to create/initialize mouse cursor capture");
                    }
                    else if (InitNetwork() != true)
                    {
                        AMFTraceError(AMF_FACILITY, L"Failed to create/initialize network");
                    }
                    else if (InitStreamer() != true)
                    {
                        AMFTraceError(AMF_FACILITY, L"Failed to initialize A/V Streamer");
                    }
                    else if (InitControllers() != true)
                    {
                        AMFTraceError(AMF_FACILITY, L"Failed to create/initialize controllers");
                    }
                    else
                    {
                        result = Status::OK;
                    }
                }
            }
            else
            {
                AMFTraceError(AMF_FACILITY, L"Failed to create shutdown directory");
            }
        }
    }
    return result;
}

void RemoteDesktopServer::Terminate()
{
    TerminateControllers();
    TerminateStreamer();
    TerminateCursorCapture();
    TerminateAudioCodec();
    TerminateAudioCapture();
    TerminateVideoCodec();
    TerminateVideoCapture();
    TerminateNetwork();
    OnAppTerminate();
    m_CurrentTime = nullptr;
    m_AVStreamer = nullptr;
    TerminateAMF();
}

bool RemoteDesktopServer::RunServer()
{
    /*
    * The main thread of the server process is checking for the presence of the shutdown semaphore file at a predefined location.
    * To shutdown a running server another instance is launched with the -shutdown <port> command line. It will create a semaphore
    * file at a predefined location, which is monitored here. Once a file with the extension containing the UDP/TCP port number is found,
    * the running daemon's main loop would break here and the server would terminate. This mechanism would not work in the interactive mode
    * supported on Windows only (not needed on Linux)
    */
    std::string fileName = GetSemaphoreFileName();
    do
    {
        std::ifstream file;
        file.open(fileName, std::ios::binary | std::ios::in);
        if (file.is_open() == true)
        {
            file.close();
            std::remove(fileName.c_str());
            break;
        }
        else
        {
            amf_sleep(1000);
        }
    } while (true);
    return true;
}

std::string RemoteDesktopServer::GetSemaphoreFileName() const
{
    std::string filePath(m_ShutdownDir);
    std::stringstream fileName;
    fileName << "shutdown." << m_Port;
    filePath += fileName.str();
    return filePath;
}

void RemoteDesktopServer::ShutdownRunningServer()
{
    //  Just create an empty semaphore file which contains the port as extension to indicate that a server listening on this port must shut down.
    std::ofstream file;
    file.open(GetSemaphoreFileName(), std::ios::binary | std::ios::trunc | std::ios::out);
    file.close();
}

bool RemoteDesktopServer::OnAppInit()
{
    GetParam(PARAM_NAME_MAX_CONNECTIONS, m_MaxConcurrentConnections);
    return true;
}

void RemoteDesktopServer::OnAppTerminate()
{
}

bool RemoteDesktopServer::InitAMF()
{
    bool result = false;
    if (g_AMFFactory.Init() == AMF_OK)
    {
        g_AMFFactory.Init();
        g_AMFFactory.GetDebug()->AssertsEnable(false);

        if (g_AMFFactory.GetAMFDLLHandle() != nullptr)
        {
            amf_increase_timer_precision();
#ifdef _DEBUG
            int32_t traceLevel = AMF_TRACE_DEBUG;
#else
            int32_t traceLevel = AMF_TRACE_INFO;
#endif
            g_AMFFactory.GetTrace()->TraceEnableAsync(false);
            g_AMFFactory.GetTrace()->SetGlobalLevel(traceLevel);
            g_AMFFactory.GetTrace()->SetWriterLevel(AMF_TRACE_WRITER_CONSOLE, AMF_TRACE_ERROR);
            g_AMFFactory.GetTrace()->SetWriterLevel(AMF_TRACE_WRITER_DEBUG_OUTPUT, traceLevel);
            g_AMFFactory.GetTrace()->SetWriterLevel(AMF_TRACE_WRITER_FILE, AMF_TRACE_INFO);

            static constexpr const wchar_t* componentNames[] =
            {
                 L"AMFEncoderVCE", L"UVEEncoderTrace", L"EncoderVCEPropertySet", L"EncoderH264PropertySet", L"EncoderVulkanH264PropertySet",
                 L"VCEEncoderTrace", L"AMFEncoderVulkan", L"AMFEncoderHEVC", L"EncoderUVEHEVCImpl", L"encoderuveh264impl", L"AMFEncoderUVE",
                 L"AMFEncoderCoreBaseImpl", L"AMFEncoderCoreHevc", L"AMFVideoConverterImpl", L"AMFEncoderCoreImpl", L"AMFScreenCaptureImpl",
                 L"EncodeQueuePalImpl", L"AMFAudioCaptureImpl", L"AMFEncoderCoreAv1", L"AMFEncoderCoreH264"
            };
            for (auto& it : componentNames)
            {
                g_AMFFactory.GetTrace()->SetWriterLevelForScope(AMF_TRACE_WRITER_DEBUG_OUTPUT, it, AMF_TRACE_WARNING);
                g_AMFFactory.GetTrace()->SetWriterLevelForScope(AMF_TRACE_WRITER_FILE, it, AMF_TRACE_WARNING);
            }

            AMF_RESULT amfResult = g_AMFFactory.GetFactory()->CreateContext(&m_Context);
            if (m_Context == nullptr)
            {
                AMFTraceError(AMF_FACILITY, L"Failed to create AMFContext, result=%s", amf::AMFGetResultText(amfResult));
                result = false;
            }
            else
            {
                amf_increase_timer_precision();
                m_AMFInitalized = true;
                result = true;
                AMFTraceDebug(AMF_FACILITY, L"AMF runtime initialized successfully");
            }
        }
    }
    return result;
}

void RemoteDesktopServer::TerminateAMF()
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

bool RemoteDesktopServer::InitVideoCapture()
{
    bool result = false;
    AMF_RESULT amfResult = AMF_FAIL;

    if (m_VideoCapture == nullptr)  //  Check for nullptr since capture can be created by the platform-specific derived class, e.g. MS Desktop Duplication
    {
        amfResult = g_AMFFactory.GetFactory()->CreateComponent(m_Context, AMFDisplayCapture, &m_VideoCapture);
    }

    if (amfResult != AMF_OK)
    {
        AMFTraceError(AMF_FACILITY, L"Failed to create the display capture component, result=%s", amf::AMFGetResultText(amfResult));
    }
    else
    {
        int64_t monitorID = 0;
        GetParam(PARAM_NAME_MONITOR_ID, monitorID);
        amfResult = m_VideoCapture->SetProperty(AMF_DISPLAYCAPTURE_MONITOR_INDEX, monitorID);
        m_VideoStreamDescriptor.SetSourceID(static_cast<int32_t>(monitorID));

        std::wstring captureMode = CAPTURE_MODE_FRAMERATE;
        if (GetParamWString(PARAM_NAME_CAPTURE_MODE, captureMode) == AMF_OK)
        {
            captureMode = ::toUpper(captureMode);
        }
        if (captureMode == CAPTURE_MODE_FRAMERATE)
        {
            amfResult = m_VideoCapture->SetProperty(AMF_DISPLAYCAPTURE_MODE, amf_int64(AMF_DISPLAYCAPTURE_MODE_ENUM::AMF_DISPLAYCAPTURE_MODE_KEEP_FRAMERATE));
            switch (amfResult)
            {
            case AMF_OK:
                {
                    int64_t frameRate = 60;
                    GetParam(PARAM_NAME_CAPTURE_RATE, frameRate);
                    amfResult = m_VideoCapture->SetProperty(AMF_DISPLAYCAPTURE_FRAMERATE, AMFConstructRate(int32_t(frameRate), 1));
                    switch (amfResult)
                    {
                    case AMF_OK:
                        AMFTraceInfo(AMF_FACILITY, L"Capture mode is set to \"%s\" at %d fps", CAPTURE_MODE_FRAMERATE, int32_t(frameRate));
                        m_VideoStreamDescriptor.SetFramerate(static_cast<float>(frameRate));
                        result = true;
                        break;
                    case AMF_NOT_FOUND:
                    case AMF_ACCESS_DENIED:
                        AMFTraceWarning(AMF_FACILITY, L"Failed to set video capture frame rate to %d fps", int32_t(frameRate));
                        result = true;
                        break;
                    default:
                        break;
                    }
                }
                break;
            case AMF_NOT_FOUND:
            case AMF_ACCESS_DENIED:
                AMFTraceWarning(AMF_FACILITY, L"Selected video capture method does not support capture at a fixed frame rate, -%s %S ignored", PARAM_NAME_CAPTURE_MODE, CAPTURE_MODE_FRAMERATE);
                result = true;
                break;
            default:
                AMFTraceError(AMF_FACILITY, L"Error setting the AMF_DISPLAYCAPTURE_MODE property on the display capture component, result=%s", amf::AMFGetResultText(amfResult));
                break;
            }
        }
        else if (captureMode == CAPTURE_MODE_PRESENT)
        {
            amfResult = m_VideoCapture->SetProperty(AMF_DISPLAYCAPTURE_MODE, amf_int64(AMF_DISPLAYCAPTURE_MODE_ENUM::AMF_DISPLAYCAPTURE_MODE_WAIT_FOR_PRESENT));
        }
        else if (captureMode == CAPTURE_MODE_ASAP)
        {
            amfResult = m_VideoCapture->SetProperty(AMF_DISPLAYCAPTURE_MODE, amf_int64(AMF_DISPLAYCAPTURE_MODE_ENUM::AMF_DISPLAYCAPTURE_MODE_GET_CURRENT_SURFACE));
        }
        if (result != true)
        {
            if (amfResult == AMF_OK)
            {
                AMFTraceInfo(AMF_FACILITY, L"Capture mode is set to %S", captureMode.c_str());
            }
            else
            {
                AMFTraceError(AMF_FACILITY, L"Failed to set video capture mode to %S, result=%s", captureMode.c_str(), amf::AMFGetResultText(amfResult));
            }
        }

        if ((amfResult = m_VideoCapture->SetProperty(AMF_DISPLAYCAPTURE_CURRENT_TIME_INTERFACE, m_CurrentTime)) != AMF_OK)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to set the current time interface on the video capture component, result=%s", amf::AMFGetResultText(amfResult));
        }
        else
        {   //  All properties are configured
            if ((amfResult = m_VideoCapture->Init(amf::AMF_SURFACE_UNKNOWN, 0, 0)) != AMF_OK)
            {
                AMFTraceError(AMF_FACILITY, L"Failed to initialize display capture, result=%s", amf::AMFGetResultText(amfResult));
                result = false;
            }
            else
            {
                AMFTraceInfo(AMF_FACILITY, L"Video capture successfully initialized");
                result = true;
            }
        }
    }

    return result;
}

void RemoteDesktopServer::TerminateVideoCapture()
{
    if (m_VideoCapture != nullptr)
    {
        m_VideoCapture->Terminate();
        m_VideoCapture = nullptr;
    }
}

bool RemoteDesktopServer::InitAudioCapture()
{
    bool result = false;
    AMF_RESULT amfResult = AMF_FAIL;

    if ((amfResult = AMFCreateComponentAudioCapture(m_Context, &m_AudioCapture)) != AMF_OK)
    {
        result = false;
        AMFTraceError(AMF_FACILITY, L"Failed to create an audio capture component, result = %s", amf::AMFGetResultText(amfResult));
    }
    else if ((amfResult = m_AudioCapture->SetProperty(AUDIOCAPTURE_CURRENT_TIME_INTERFACE, m_CurrentTime)) != AMF_OK)
    {
        result = false;
        AMFTraceError(AMF_FACILITY, L"Failed to set the current time interface on the audio capture component, result=%s", amf::AMFGetResultText(amfResult));
    }
    else
    {
        m_AudioCapture->SetProperty(AUDIOCAPTURE_SOURCE, false);    //  Capture from the desktop

        int64_t samplingRate = 44100;
        GetParam(PARAM_NAME_AUDIO_SAMPLING_RATE, samplingRate);
        m_AudioStreamDescriptor.SetSamplingRate(static_cast<int32_t>(samplingRate));

        std::wstring channels = AUDIO_CHANNELS_STEREO;
        GetParamWString(PARAM_NAME_AUDIO_CHANNELS, channels);
        int32_t numOfChannels = 0;
        int32_t layout = 0;
        std::string layoutDesc;
        if (channels == AUDIO_CHANNELS_MONO)
        {
            numOfChannels = 1;
            layout = amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_FRONT_CENTER;
            layoutDesc = "mono";
            result = true;
        }
        else if (channels == AUDIO_CHANNELS_STEREO)
        {
            numOfChannels = 2;
            layout = amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_FRONT_LEFT | amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_FRONT_RIGHT;
            layoutDesc = "stereo";
            result = true;
        }
        else if (channels == AUDIO_CHANNELS_STEREO_SUB)
        {
            numOfChannels = 3;
            layout = amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_FRONT_LEFT | amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_FRONT_RIGHT | amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_LOW_FREQUENCY;
            layoutDesc = "stereo-2.1";
            result = true;
        }
        else if (channels == AUDIO_CHANNELS_STEREO_CENTER)
        {
            numOfChannels = 3;
            layout = amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_FRONT_CENTER | amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_FRONT_LEFT | amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_FRONT_RIGHT;
            layoutDesc = "LCR (3.0-channel)";
            result = true;
        }
        else if (channels == AUDIO_CHANNELS_STEREO_CENTER_SUB)
        {
            numOfChannels = 4;
            layout = amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_FRONT_CENTER | amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_FRONT_LEFT | amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_FRONT_RIGHT |
                        amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_LOW_FREQUENCY;
            layoutDesc = "LCR (3.1-channel)";
            result = true;
        }
        else if (channels == AUDIO_CHANNELS_SURROUND)
        {
            numOfChannels = 5;
            layout = amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_FRONT_CENTER | amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_FRONT_LEFT | amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_FRONT_RIGHT |
                        amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_BACK_LEFT | amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_BACK_RIGHT;
            layoutDesc = "surround (5.0-channel)";
            result = true;
        }
        else if (channels == AUDIO_CHANNELS_SURROUND_SUB)
        {
            numOfChannels = 6;
            layout = amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_FRONT_CENTER | amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_FRONT_LEFT | amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_FRONT_RIGHT |
                        amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_BACK_LEFT | amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_BACK_RIGHT | amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_LOW_FREQUENCY;
            layoutDesc = "surround (5.1-channel)";
            result = true;
        }
        else
        {
            result = false;
            AMFTraceError(AMF_FACILITY, L"Invalid audio channel layout: %s", channels.c_str());
        }
        if (result == true)
        {
            m_AudioCapture->SetProperty(AUDIOCAPTURE_CHANNELS, numOfChannels);
            m_AudioStreamDescriptor.SetNumOfChannels(numOfChannels);
            m_AudioCapture->SetProperty(AUDIOCAPTURE_CHANNEL_LAYOUT, layout);
            m_AudioStreamDescriptor.SetLayout(layout);

            std::stringstream desc;
            desc << "Desktop audio, " << layoutDesc << ", " << samplingRate << "Hz";
            m_AudioStreamDescriptor.SetDescription(desc.str());

            m_AudioCapture->SetProperty(AUDIOCAPTURE_LOWLATENCY, true);
            if ((amfResult = m_AudioCapture->Init(amf::AMF_SURFACE_UNKNOWN, 0, 0)) != AMF_OK)
            {
                AMFTraceError(AMF_FACILITY, L"Failed to initialize audio capture, result=%s", amf::AMFGetResultText(amfResult));
                result = false;
            }
            else
            {
                AMFTraceInfo(AMF_FACILITY, L"Audio capture successfully initialized");
                result = true;
            }
        }
    }
    return result;
}

void RemoteDesktopServer::TerminateAudioCapture()
{
    if (m_AudioCapture != nullptr)
    {
        m_AudioCapture->Terminate();
        m_AudioCapture = nullptr;
    }
}

bool RemoteDesktopServer::InitCursorCapture()
{   //  Cursor capture is platform-specific and is overriden in the derived class. Add common platform-agnostic initialization here if necessary
    m_LastCursor = nullptr;
    return true;
}

void RemoteDesktopServer::TerminateCursorCapture()
{
    m_LastCursor = nullptr;
    m_CursorCapture = nullptr;
}

bool RemoteDesktopServer::InitVideoCodec()
{
    bool result = true;
    std::wstring codec = VIDEO_CODEC_HEVC;
    if (GetParamWString(PARAM_NAME_VIDEO_CODEC, codec) == AMF_OK)
    {
        codec = ::toUpper(codec);
    }
    ssdk::video::VideoEncodeEngine::Ptr encoder;
    if (codec == VIDEO_CODEC_AVC || codec == VIDEO_CODEC_H264)
    {
        encoder = ssdk::video::VideoEncodeEngine::Ptr(new ssdk::video::GPUEncoderH264(m_Context));
    }
    else if (codec == VIDEO_CODEC_HEVC || codec == VIDEO_CODEC_H265)
    {
        encoder = ssdk::video::VideoEncodeEngine::Ptr(new ssdk::video::GPUEncoderHEVC(m_Context));
    }
    else if (codec == VIDEO_CODEC_AV1)
    {
        encoder = ssdk::video::VideoEncodeEngine::Ptr(new ssdk::video::GPUEncoderAV1(m_Context));
    }
    else
    {
        AMFTraceError(AMF_FACILITY, L"Invalid codec: %S", codec.c_str());
        result = false;
    }
    m_VideoEncoder = encoder;
    if (result == true)
    {
        m_VideoStreamDescriptor.SetCodec(encoder->GetCodecName());

        bool hdr = false;
        GetParam(PARAM_NAME_HDR, hdr);

        m_VideoEncoder->EnableHDR(hdr);

        m_VideoStreamDescriptor.SetColorDepth(hdr == true ? ssdk::transport_common::VideoStreamDescriptor::ColorDepth::HDR_10 : ssdk::transport_common::VideoStreamDescriptor::ColorDepth::SDR_8);

        AMFSize resolution = { 1920, 1080 };
        GetParam(PARAM_NAME_RESOLUTION, resolution);
        m_VideoStreamDescriptor.SetResolution(resolution);

        int64_t videoBitrate = 50000000;
        GetParam(PARAM_NAME_VIDEO_BITRATE, videoBitrate);
        m_VideoStreamDescriptor.SetBitrate(static_cast<int32_t>(videoBitrate));

        std::stringstream desc;
        desc << "Monitor " << m_VideoStreamDescriptor.GetSourceID() << ", (" << resolution.width << 'x' << resolution.height << '@' << m_VideoStreamDescriptor.GetFramerate() << "Hz, " << (hdr ? "HDR" : "SDR") << ')';
        m_VideoStreamDescriptor.SetDescription(desc.str());
    }
    return result;
}

void RemoteDesktopServer::TerminateVideoCodec()
{
    m_VideoEncoder = nullptr;
}

bool RemoteDesktopServer::InitAudioCodec()
{
    bool result = true;
    std::wstring codec = AUDIO_CODEC_AAC;
    if (GetParamWString(PARAM_NAME_AUDIO_CODEC, codec) == AMF_OK)
    {
        codec = ::toUpper(codec);
    }
    ssdk::audio::AudioEncodeEngine::Ptr encoder;
    if (codec == AUDIO_CODEC_AAC)
    {
        encoder = ssdk::audio::AudioEncodeEngine::Ptr(new ssdk::audio::AudioEncoderAAC(m_Context));
    }
    else if (codec == AUDIO_CODEC_OPUS)
    {
        encoder = ssdk::audio::AudioEncodeEngine::Ptr(new ssdk::audio::AudioEncoderOPUS(m_Context));
    }
    else
    {
        AMFTraceError(AMF_FACILITY, L"Invalid codec: %S", codec.c_str());
        result = false;
    }
    m_AudioEncoder = encoder;
    if (result == true)
    {
        m_AudioStreamDescriptor.SetCodec(encoder->GetCodecName());

        int64_t audioBitrate = 256000;
        GetParam(PARAM_NAME_AUDIO_BITRATE, audioBitrate);
        m_AudioStreamDescriptor.SetBitrate(static_cast<int32_t>(audioBitrate));

        std::stringstream adjustedDesc;
        adjustedDesc << m_AudioStreamDescriptor.GetDescription() << ", " << encoder->GetCodecName() << ", " << audioBitrate << "bps";
        m_AudioStreamDescriptor.SetDescription(adjustedDesc.str());
    }
    return result;
}

void RemoteDesktopServer::TerminateAudioCodec()
{
    m_AudioEncoder = nullptr;
}

bool RemoteDesktopServer::InitNetwork()
{
    bool result = true;
    // error checking.  verify necessery components implementing VideoSenderCallback & AudioSenderCallback are instantiated

    m_Transport = ssdk::transport_common::ServerTransport::Ptr(new ssdk::transport_amd::ServerTransportImpl);
    ssdk::transport_amd::ServerTransportImpl::ServerInitParametersAmd initParams;

    // todo: set stub callbacks

    initParams.SetContext(m_Context);

    // set paramters specified from command line
    int64_t port = DEFAULT_PORT;
    GetParam(PARAM_NAME_PORT, port);
    initParams.SetPort(port);

    std::wstring protocol;
    GetParamWString(PARAM_NAME_PROTOCOL, protocol);
    protocol = ::toUpper(protocol);
    ssdk::transport_common::ServerTransport::NETWORK_TYPE networkType = ssdk::transport_common::ServerTransport::NETWORK_TYPE::NETWORK_UDP;
    if (protocol == PROTOCOL_TCP)// todo verify this works
    {
        networkType = ssdk::transport_common::ServerTransport::NETWORK_TYPE::NETWORK_TCP;
    }
    initParams.SetNetworkType(networkType);

    initParams.SetNetwork(true);

    int64_t datagramSize = DEFAULT_DATAGRAM_SIZE;
    GetParam(PARAM_NAME_DATAGRAM_SIZE, datagramSize);
    initParams.SetDatagramSize(datagramSize);

    std::string bindInterface = DEFAULT_BIND_INTERFACE;
    GetParamString(PARAM_NAME_BIND_INTERFACE, bindInterface);
    initParams.SetBindInterface(bindInterface);

    std::string hostName = DEFAULT_SERVER_HOSTNAME;
    GetParamString(PARAM_NAME_SERVER_HOSTNAME, hostName);
    initParams.SetHostName(hostName);

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

    if (result == true)
    {
        //  Add default video and audio stream descriptors
        initParams.AddVideoStream(m_VideoStreamDescriptor);
        initParams.AddAudioStream(m_AudioStreamDescriptor);

        //  Set application callbacks
        initParams.SetCallback(static_cast<ssdk::transport_common::ServerTransport::ConnectionManagerCallback*>(this));
        initParams.SetCallback(static_cast<ssdk::transport_common::ServerTransport::VideoSenderCallback*>(m_AVStreamer.get()));
        initParams.SetCallback(static_cast<ssdk::transport_common::ServerTransport::VideoStatsCallback*> (m_AVStreamer.get()));
        initParams.SetCallback(static_cast<ssdk::transport_common::ServerTransport::AudioSenderCallback*>(m_AVStreamer.get()));
        initParams.SetCallback(static_cast<ssdk::transport_common::ServerTransport::InputControllerCallback*>(m_pControllerManager.get()));

        //  Start the server
        ssdk::transport_common::Result networkResult = m_Transport->Start(initParams);
        if (networkResult != ssdk::transport_common::Result::OK)
        {
            AMFTraceError(AMF_FACILITY, L"Cannot initialize the network stack: failed to start server");
            result = false;
        }
        else
        {
            AMFTraceDebug(AMF_FACILITY, L"Server started");
        }
    }

    return result;
}

void RemoteDesktopServer::TerminateNetwork()
{
    if (m_Transport != nullptr)
    {
        m_Transport->Shutdown();
        m_Transport = nullptr;
    }
    m_CurConnectionsCnt = 0;
    AMFTraceDebug(AMF_FACILITY, L"Server terminated");
}

bool RemoteDesktopServer::InitStreamer()
{
    bool result = false;
    AMF_RESULT amfResult = AMF_OK;


    bool enableQoSBitrate = true;
    GetParam(PARAM_NAME_QOS_ADJUST_BITRATE, enableQoSBitrate);
    bool enableQoSFramerate = true;
    GetParam(PARAM_NAME_QOS_ADJUST_FRAMERATE, enableQoSFramerate);

    if (enableQoSBitrate || enableQoSFramerate)
    {
        m_QoS = ssdk::util::QoS::Ptr(new ssdk::util::QoS(m_AVStreamer.get(), ssdk::transport_common::DEFAULT_STREAM));
    }
    else
    {
        m_QoS = nullptr;
    }

    m_VideoTransmitterAdapter = ssdk::video::TransmitterAdapter::Ptr(new ssdk::video::TransmitterAdapter(m_Transport, ssdk::transport_common::DEFAULT_STREAM, m_QoS));
    m_VideoOutput = ssdk::video::MonoscopicVideoOutput::Ptr(new ssdk::video::MonoscopicVideoOutput(*m_VideoTransmitterAdapter, m_VideoEncoder, m_Context, m_MemoryType));

    m_AudioTransmitterAdapter = ssdk::audio::TransmitterAdapter::Ptr(new ssdk::audio::TransmitterAdapter(m_Transport, ssdk::transport_common::DEFAULT_STREAM));
    m_AudioOutput = ssdk::audio::AudioOutput::Ptr(new ssdk::audio::AudioOutput(*m_AudioTransmitterAdapter, m_AudioEncoder, m_Context));

    bool hdr = false;
    GetParam(PARAM_NAME_HDR, hdr);

    bool preserveAspectRatio = true;
    GetParam(PARAM_NAME_PRESERVE_ASPECT_RATIO, preserveAspectRatio);

    int64_t audioCaptureSamplingRate = 0;
    m_AudioCapture->GetProperty(AUDIOCAPTURE_SAMPLERATE, &audioCaptureSamplingRate);
    int64_t audioChannels = 0;
    m_AudioCapture->GetProperty(AUDIOCAPTURE_CHANNELS, &audioChannels);
    int64_t audioChannelLayout = 0;
    m_AudioCapture->GetProperty(AUDIOCAPTURE_CHANNEL_LAYOUT, &audioChannelLayout);
    int64_t audioCaptureFormat = 0;
    m_AudioCapture->GetProperty(AUDIOCAPTURE_FORMAT, &audioCaptureFormat);

    if ((amfResult = m_VideoOutput->Init(hdr == true ? m_VideoEncoder->GetPreferredHDRFormat() : m_VideoEncoder->GetPreferredSDRFormat(), m_VideoStreamDescriptor.GetResolution(), m_VideoStreamDescriptor.GetResolution(),
        m_VideoStreamDescriptor.GetBitrate(), m_VideoStreamDescriptor.GetFramerate(), hdr, preserveAspectRatio, 0)) != AMF_OK)
    {
        AMFTraceError(AMF_FACILITY, L"Failed to initialize video output, result=%s", amf::AMFGetResultText(amfResult));
        result = false;
    }
    else if ((amfResult = m_AudioOutput->Init(amf::AMF_AUDIO_FORMAT(audioCaptureFormat), int32_t(audioCaptureSamplingRate), int32_t(audioChannels), int32_t(audioChannelLayout),
                                              m_AudioStreamDescriptor.GetSamplingRate(), m_AudioStreamDescriptor.GetNumOfChannels(), m_AudioStreamDescriptor.GetLayout(), m_AudioStreamDescriptor.GetBitrate())) != AMF_OK)
    {
        AMFTraceError(AMF_FACILITY, L"Failed to initialize audio output, result=%s", amf::AMFGetResultText(amfResult));
        result = false;
    }
    else if (m_AVStreamer == nullptr)
    {
        AMFTraceError(AMF_FACILITY, L"AVStreamer has not been created");
        result = false;
    }
    else
    {
        if (m_QoS != nullptr)
        {
            ssdk::util::QoS::InitParams qosInitParams;
            qosInitParams.timeBeforePanic = QOS_DEFAULT_SECONDS_BEFORE_PANIC * AMF_SECOND;

            qosInitParams.thresholdIDR = QOS_DEFAULT_THRESHOLD_IDR;
            qosInitParams.panicThresholdIDR = QOS_DEFAULT_THRESHOLD_IDR_PANIC;

            qosInitParams.maxEncoderQueueDepth = QOS_DEFAULT_MAX_ENCODER_QUEUE_DEPTH;
            qosInitParams.maxDecoderQueueDepth = QOS_DEFAULT_MAX_DECODER_QUEUE_DEPTH;

            if (enableQoSBitrate == true && enableQoSFramerate == true)
            {
                qosInitParams.strategy = ssdk::util::QoS::QoSStrategy::ADJUST_BOTH;
            }
            else if (enableQoSBitrate == true)
            {
                qosInitParams.strategy = ssdk::util::QoS::QoSStrategy::ADJUST_VIDEOBITRATE; 
            }
            else // enableQoSFramerate == true
            {
                qosInitParams.strategy = ssdk::util::QoS::QoSStrategy::ADJUST_FRAMERATE;
            }
            
            int64_t frameRate = QOS_DEFAULT_FRAMERATE;
            GetParam(PARAM_NAME_CAPTURE_RATE, frameRate);
            qosInitParams.maxFramerate = (float)frameRate;

            int64_t minFrameRate = QOS_DEFAULT_FRAMERATE_MIN;
            GetParam(PARAM_NAME_QOS_MIN_FRAMERATE, minFrameRate);
            qosInitParams.minFramerate = (float)minFrameRate;

            qosInitParams.framerateStep = (float)QOS_DEFAULT_FRAMERATE_STEP;
            qosInitParams.framerateAdjustmentPeriod = QOS_DEFAULT_FRAMERATE_ADJUSTMENT_PERIOD_SECONDS * AMF_SECOND;
        
            int64_t videoBitrate = QOS_DEFAULT_BITRATE;
            GetParam(PARAM_NAME_VIDEO_BITRATE, videoBitrate);
            qosInitParams.maxBitrate = videoBitrate;

            int64_t minVideoBitrate = QOS_DEFAULT_BITRATE_MIN;
            GetParam(PARAM_NAME_QOS_MIN_BITRATE, minVideoBitrate);
            qosInitParams.minBitrate = minVideoBitrate;
            
            qosInitParams.bitrateStep = QOS_DEFAULT_BITRATE_STEP;
            qosInitParams.bitrateAdjustmentPeriod = QOS_DEFAULT_BITRATE_ADJUSTMENT_PERIOD_SECONDS * AMF_SECOND;

            result = m_QoS->Init(qosInitParams);
        }

        result = m_AVStreamer->Init(m_VideoCapture, m_VideoOutput, m_VideoTransmitterAdapter, m_AudioCapture, m_AudioOutput, m_AudioTransmitterAdapter, m_QoS);
        if (result == true)
        {
            AMFTraceInfo(AMF_FACILITY, L"Successfully initialized video and audio pipelines");
            m_AVStreamerInitialized = true;
        }
        else
        {
            AMFTraceError(AMF_FACILITY, L"Failed to initialize A/V Streamer");
        }
    }

    return result;
}

void RemoteDesktopServer::TerminateStreamer()
{
    m_AVStreamerInitialized = false;
    if (m_AVStreamer != nullptr)
    {
        m_AVStreamer->Terminate();  //  The AVStreamer object is created in the main Init() and destroyed in Terminate(), do not NULL the pointer here
    }
    if (m_VideoOutput != nullptr)
    {
        m_VideoOutput->Terminate();
        m_VideoOutput = nullptr;
    }
    m_VideoTransmitterAdapter = nullptr;

    if (m_AudioOutput != nullptr)
    {
        m_AudioOutput->Terminate();
        m_AudioOutput = nullptr;
    }
    m_AudioTransmitterAdapter = nullptr;
}

bool RemoteDesktopServer::InitControllers()
{
    if (m_pControllerManager == nullptr)
    {
        return false;
    }

    m_pControllerManager->SetServerTransport(m_Transport);
    m_pControllerManager->SetCursorCapture(m_CursorCapture);

    return true;
}

void RemoteDesktopServer::TerminateControllers()
{
    if (m_pControllerManager != nullptr)
    {
        m_pControllerManager->TerminateControllers();
        m_pControllerManager = nullptr;
    }
    AMFTraceDebug(AMF_FACILITY, L"Controller Manager terminated");
}

ssdk::transport_common::ServerTransport::ConnectionManagerCallback::ClientAction RemoteDesktopServer::OnDiscoveryRequest(const char* clientID)
{
    ssdk::transport_common::ServerTransport::ConnectionManagerCallback::ClientAction action = ssdk::transport_common::ServerTransport::ConnectionManagerCallback::ClientAction::REFUSE;
    amf::AMFLock lock(&m_Guard);
    if (m_AVStreamerInitialized == true && m_CurConnectionsCnt < m_MaxConcurrentConnections)
    {
        action = ssdk::transport_common::ServerTransport::ConnectionManagerCallback::ClientAction::ACCEPT;
        AMFTraceInfo(AMF_FACILITY, L"Server received and accepted a discovery request from client \"%S\"", clientID);
    }
    else
    {
        AMFTraceInfo(AMF_FACILITY, L"Server received and rejected a discovery request from client %S because the number of concurrent connections of %lld has been exceeded", clientID, m_MaxConcurrentConnections);
    }
    return action;
}

ssdk::transport_common::ServerTransport::ConnectionManagerCallback::ClientAction RemoteDesktopServer::OnConnectionRequest(ssdk::transport_common::SessionHandle session, const char* clientID, ClientRole role)
{
    ssdk::transport_common::ServerTransport::ConnectionManagerCallback::ClientAction action = ssdk::transport_common::ServerTransport::ConnectionManagerCallback::ClientAction::REFUSE;
    amf::AMFLock lock(&m_Guard);
    if (m_AVStreamerInitialized == true && m_CurConnectionsCnt++ < m_MaxConcurrentConnections)
    {
        action = ssdk::transport_common::ServerTransport::ConnectionManagerCallback::ClientAction::ACCEPT;
        AMFTraceInfo(AMF_FACILITY, L"Server received and accepted a connection request from client \"%S\", session ID: %lld as %s", clientID, session, role == ClientRole::CONTROLLER ? L"controller" : L"viewer");
    }
    else
    {
        AMFTraceInfo(AMF_FACILITY, L"Server received and rejected a connection request from client \"%S\", session ID: %lld because the number of concurrent connections of %lld has been exceeded", clientID, session, m_MaxConcurrentConnections);
    }
    return action;
}

void RemoteDesktopServer::OnClientSubscribed(ssdk::transport_common::SessionHandle /*session*/)
{
    if (m_CurConnectionsCnt > 0 && m_pControllerManager != nullptr && m_pControllerManager->Started() == false)
    {
        m_pControllerManager->Start();
    }
}

void RemoteDesktopServer::OnClientDisconnected(ssdk::transport_common::SessionHandle session, DisconnectReason reason)
{
    ssdk::ctls::svr::ControllerManager::Ptr pControllerManager;
    bool stop = false;
    {
        amf::AMFLock lock(&m_Guard);
        pControllerManager = m_pControllerManager;
        if (m_CurConnectionsCnt > 0)
        {
            --m_CurConnectionsCnt;
            if (m_AVStreamerInitialized == true)
            {
                m_AVStreamer->OnSessionDisconnected(session);
            }
            AMFTraceInfo(AMF_FACILITY, L"Client with session ID: %lld %s, remaining clients: %lld", session, reason == DisconnectReason::CLIENT_DISCONNECTED ? L"disconnected" : L"timed out", m_CurConnectionsCnt);

            if (m_CurConnectionsCnt == 0 && m_pControllerManager != nullptr && m_pControllerManager->Started() == true)
            {
                stop = true;
            }
        }
#ifdef _DEBUG
        else
        {
            AMFTraceDebug(AMF_FACILITY, L"An extra OnClientDisconnected call has been received from session %lld, reason %s, look for a bug", session, reason == DisconnectReason::CLIENT_DISCONNECTED ? L"DISCONNECT" : L"TIMEOUT");
        }
#endif
    }
    if (stop == true && pControllerManager != nullptr)
    {
        pControllerManager->Stop();
    }
}
