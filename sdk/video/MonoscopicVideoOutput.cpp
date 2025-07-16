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
// Copyright (c) 2018 Advanced Micro Devices, Inc. All rights reserved.
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

#include "MonoscopicVideoOutput.h"
#include "Defines.h"
#include "transports/transport-common/Video.h"


#include "amf/public/include/components/VideoConverter.h"
#include "amf/public/include/components/DisplayCapture.h"
#include "amf/public/common/TraceAdapter.h"

#include <queue>

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::video::MonoscopicVideoOutput";

namespace ssdk::video
{
    MonoscopicVideoOutput::MonoscopicVideoOutput(TransmitterAdapter& transportAdapter,
        VideoEncodeEngine::Ptr encoder, amf::AMFContext* context, amf::AMF_MEMORY_TYPE memoryType) :
        m_TransportAdapter(transportAdapter),
        m_Encoder(std::move(encoder)),
        m_Context(context),
        m_MemoryType(memoryType),
        m_EncoderPoller(this)
    {
    }

    MonoscopicVideoOutput::~MonoscopicVideoOutput()
    {
        Terminate();
        //  Encoder and context are passed to constructor, release only in destructor after a call to Terminate()
        m_Encoder = nullptr;
        m_Context = nullptr;    //  Context should be the last thing to be released
    }

    AMF_RESULT MonoscopicVideoOutput::Init(amf::AMF_SURFACE_FORMAT format, const AMFSize& inputResolution, const AMFSize& streamResolution, int64_t bitrate, float frameRate, bool hdr, bool preserveAspectRatio, int64_t intraRefreshPeriod, bool forceEFCOff)
    {
        AMF_RESULT result = AMF_OK;
        amf::AMFLock lock(&m_Guard);
        if (m_Initialized == false)
        {
            m_NeedsCSC = (inputResolution != streamResolution) || (hdr == true && m_Encoder->IsHDRSupported() == false) || m_Encoder->GetSupportedColorRange() == AMF_COLOR_RANGE_ENUM::AMF_COLOR_RANGE_STUDIO || forceEFCOff == true;
            m_ForceEFCOff = forceEFCOff;
            ColorParameters encoderInputColorParams;
            DetermineEncoderInputColorParams(nullptr, m_NeedsCSC, hdr, encoderInputColorParams);
            if ((result = InitializeEncoder(streamResolution, bitrate, frameRate, encoderInputColorParams, intraRefreshPeriod)) != AMF_OK)
            {
                AMFTraceError(AMF_FACILITY, L"Failed to initialize the encoder, result=%s", amf::AMFGetResultText(result));
            }
            else
            {
                if (m_NeedsCSC == true)
                {
                    AMFTraceInfo(AMF_FACILITY, L"EFC is disabled");
                    result = InitializeConverter(format, inputResolution, streamResolution, encoderInputColorParams);
                    AMF_RETURN_IF_FAILED(result, L"Failed to reinitialize video converter, result %s", amf::AMFGetResultText(result));
                }
                else
                {
                    AMFTraceInfo(AMF_FACILITY, L"EFC is enabled");
                }
                m_CurrentInputColorParams.SetFormat(format);
                m_InputResolution = inputResolution;
                m_StreamResolution = streamResolution;
                m_Bitrate = bitrate;
                m_FrameRate = frameRate;
                m_HDR = hdr;
                m_PreserveAspectRatio = preserveAspectRatio;
                m_SequenceNumber = 0;
                m_FramesSubmitted = 0;

                m_Initialized = true;
            }
        }
        return result;
    }

    AMF_RESULT MonoscopicVideoOutput::InitializeEncoder(const AMFSize& streamResolution, int64_t bitrate, float frameRate, const ColorParameters& colorParams, int64_t intraRefreshPeriod)
    {
        AMF_RESULT result = AMF_OK;
        AMF_RETURN_IF_INVALID_POINTER(m_Encoder, L"Encoder engine is not instantiated");
        m_Encoder->Terminate();
        result = m_Encoder->Init(streamResolution, bitrate, frameRate, intraRefreshPeriod, colorParams);
        AMF_RETURN_IF_FAILED(result, L"Failed to initialize video encoder, result = %s", amf::AMFGetResultText(result));
        amf::AMFBufferPtr extraData;
        if (m_Encoder->GetExtraData(&extraData) == AMF_OK)
        {
            SaveExtraData(extraData);
        }

        return result;
    }

    AMF_RESULT MonoscopicVideoOutput::InitializeConverter(amf::AMF_SURFACE_FORMAT format, const AMFSize& inputResolution, const AMFSize& streamResolution, const ColorParameters& encoderInputColorParams)
    {
        AMF_RESULT result = AMF_OK;
        AMFTraceDebug(AMF_FACILITY, L"InitializeConverter()");
        if (m_Converter == nullptr)
        {
            result = g_AMFFactory.GetFactory()->CreateComponent(m_Context, AMFVideoConverter, &m_Converter);
            AMF_RETURN_IF_FAILED(result, L"Failed to create video converter %s", AMFVideoConverter);
        }

        m_Converter->SetProperty(AMF_VIDEO_CONVERTER_MEMORY_TYPE, m_MemoryType);
        m_Converter->SetProperty(AMF_VIDEO_CONVERTER_COMPUTE_DEVICE, m_MemoryType);
        m_Converter->SetProperty(AMF_VIDEO_CONVERTER_SCALE, AMF_VIDEO_CONVERTER_SCALE_BILINEAR);
        AMF_COLOR_RANGE_ENUM outputColorRange = m_Encoder->GetSupportedColorRange();
        if (outputColorRange == AMF_COLOR_RANGE_ENUM::AMF_COLOR_RANGE_FULL)
        {
            m_Converter->SetProperty(AMF_VIDEO_CONVERTER_COLOR_PROFILE, AMF_VIDEO_CONVERTER_COLOR_PROFILE_FULL_709);
        }
        else
        {
            m_Converter->SetProperty(AMF_VIDEO_CONVERTER_COLOR_PROFILE, AMF_VIDEO_CONVERTER_COLOR_PROFILE_709);

        }
        m_Converter->SetProperty(AMF_VIDEO_CONVERTER_OUTPUT_COLOR_RANGE, outputColorRange);
        m_Converter->SetProperty(AMF_VIDEO_CONVERTER_INPUT_COLOR_RANGE, AMF_COLOR_RANGE_ENUM::AMF_COLOR_RANGE_FULL);

        m_Converter->SetProperty(AMF_VIDEO_CONVERTER_LINEAR_RGB, false);

        if (m_PreserveAspectRatio == true)
        {
            m_Converter->SetProperty(AMF_VIDEO_CONVERTER_KEEP_ASPECT_RATIO, true);
            m_Converter->SetProperty(AMF_VIDEO_CONVERTER_FILL, true);
            m_Converter->SetProperty(AMF_VIDEO_CONVERTER_FILL_COLOR, AMFConstructColor(0, 0, 0, 0xff));
        }

        m_Converter->SetProperty(AMF_VIDEO_CONVERTER_OUTPUT_FORMAT, encoderInputColorParams.GetFormat());
        m_Converter->SetProperty(AMF_VIDEO_CONVERTER_OUTPUT_COLOR_PRIMARIES, encoderInputColorParams.GetColorPrimaries());
        m_Converter->SetProperty(AMF_VIDEO_CONVERTER_OUTPUT_TRANSFER_CHARACTERISTIC, encoderInputColorParams.GetXferCharacteristic());
        m_Converter->SetProperty(AMF_VIDEO_CONVERTER_INPUT_TONEMAPPING, AMF_VIDEO_CONVERTER_TONEMAPPING_LINEAR); // L"InputTonemapping", 2
        m_Converter->SetProperty(AMF_VIDEO_CONVERTER_OUTPUT_TONEMAPPING, AMF_VIDEO_CONVERTER_TONEMAPPING_LINEAR); // L"OutputTonemapping", 2

        m_Converter->SetProperty(AMF_VIDEO_CONVERTER_OUTPUT_SIZE, streamResolution);

        result = m_Converter->Init(format, inputResolution.width, inputResolution.height);
        AMF_RETURN_IF_FAILED(result, L"Failed to initialize video converter: input resolution: %dx%d, format: %s, output resolution %dx%d, format: %s",
            inputResolution.width, inputResolution.height, AMFSurfaceGetFormatName(format),
            streamResolution.width, streamResolution.height, AMFSurfaceGetFormatName(encoderInputColorParams.GetFormat()));
        AMFTraceInfo(AMF_FACILITY, L"Video Converter successfully initialized: input resolution: %dx%d, format: %s, output resolution %dx%d, format: %s, xfer: %d, primaries: %d",
            inputResolution.width, inputResolution.height, AMFSurfaceGetFormatName(format), streamResolution.width, streamResolution.height,
            AMFSurfaceGetFormatName(encoderInputColorParams.GetFormat()), encoderInputColorParams.GetXferCharacteristic(), encoderInputColorParams.GetColorPrimaries());

        return result;
    }

    AMF_RESULT MonoscopicVideoOutput::Terminate()
    {
        AMF_RESULT result = AMF_OK;
        //  Terminate threads:
        m_EncoderPoller.RequestStop();
        m_EncoderPoller.WaitForStop();

        amf::AMFLock lock(&m_Guard);
        m_Converter = nullptr;
        m_SequenceNumber = 0;
        m_FramesSubmitted = 0;
        m_Initialized = false;
        return result;
    }

    void MonoscopicVideoOutput::ForceKeyFrame()
    {
        amf::AMFLock lock(&m_Guard);
        m_ForceKeyFrame = true;
    }

    AMFSize MonoscopicVideoOutput::GetEncodedResolution() const noexcept
    {
        amf::AMFLock lock(&m_Guard);
        return m_StreamResolution;
    }

    AMF_RESULT MonoscopicVideoOutput::SetEncodedResolution(const AMFSize& resolution)
    {
        amf::AMFLock lock(&m_Guard);
        m_StreamResolution = resolution;
        return AMF_OK;
    }

    int64_t MonoscopicVideoOutput::GetBitrate() const noexcept
    {
        amf::AMFLock lock(&m_Guard);
        return m_Bitrate;
    }

    AMF_RESULT MonoscopicVideoOutput::SetBitrate(int64_t bitrate)
    {
        {
            amf::AMFLock lock(&m_Guard);
            m_Bitrate = bitrate;
        }
        AMF_RESULT result = m_Encoder->UpdateBitrate(bitrate);
        if (result != AMF_OK)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to set video encoder bitrate rate to %lld", bitrate);
        }
        return result;
    }

    float MonoscopicVideoOutput::GetFramerate() const noexcept
    {
        amf::AMFLock lock(&m_Guard);
        return m_FrameRate;
    }

    AMF_RESULT MonoscopicVideoOutput::SetFramerate(float framerate)
    {
        {
            amf::AMFLock lock(&m_Guard);
            m_FrameRate = framerate;
        }
        AMF_RESULT result = m_Encoder->UpdateFramerate(AMFConstructRate(uint32_t(framerate), 1));
        if (result != AMF_OK)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to set video encoder frame rate to %5.2f", framerate);
        }
        return result;
    }

    bool MonoscopicVideoOutput::IsKeyFrameRequested() noexcept
    {
        amf::AMFLock lock(&m_Guard);
        bool forceKeyFrame = m_ForceKeyFrame;
        m_ForceKeyFrame = false;
        return forceKeyFrame;
    }

    AMF_RESULT MonoscopicVideoOutput::SubmitInput(amf::AMFSurface* input, amf_pts originPts, amf_pts videoInPts)
    {
        AMF_RESULT result = AMF_OK;

        bool needsCSC = false;
        amf::AMFComponentPtr converter;
        amf::AMFDataPtr converterOutput;
        {
            amf::AMFLock lock(&m_Guard);
            if (m_EncoderPoller.IsRunning() == false)
            {
                m_EncoderPoller.Start();
            }
            //  Check if the pipeline needs to be reinitialized
            bool resolutionChanged = UpdateInputResolutionIfChanged(input);
            bool colorPrimariesChanged = UpdateColorParametersIfChanged(input);
            bool inputFormatChanged = UpdateInputFormatIfChanged(input);

            //  Check if anything (resolution, format or HDR data) has changed and reinitialize the pipeline if necessary
            if (resolutionChanged == true || colorPrimariesChanged == true || inputFormatChanged == true)
            {   //  Reinitialize the pipeline
                AMFSize inputResolution = GetSurfaceResolution(input);
                amf::AMF_SURFACE_FORMAT format = input->GetFormat();

                bool bDCC = false;
                input->GetProperty(AMF_DISPLAY_CAPTURE_DCC, &bDCC);

                //  VideoConverter is required when:
                needsCSC = (inputResolution != m_StreamResolution) ||      //  scaling is required
                    m_ForceEFCOff == true ||    //  EFC is forced off by initialization parameters
                    (m_HDR == true && m_Encoder->IsHDRSupported() == false) ||  //  we're asking for HDR support, but the codec does not support it
                    m_Encoder->IsFormatSupported(m_HDR == true ? m_Encoder->GetPreferredHDRFormat() : m_Encoder->GetPreferredSDRFormat(), m_HDR, input) == false || //  when the encoder cannot accept the input format directly
                    bDCC == true ||   //  when DCC is enabled
                    m_Encoder->GetSupportedColorRange() == AMF_COLOR_RANGE_ENUM::AMF_COLOR_RANGE_STUDIO;    //  When full color range is not supported

                ColorParameters encoderInputColorParams;
                DetermineEncoderInputColorParams(input, needsCSC, m_HDR, encoderInputColorParams);
                result = InitializeEncoder(m_StreamResolution, m_Bitrate, m_FrameRate, encoderInputColorParams, m_IntraRefreshPeriod);
                AMF_RETURN_IF_FAILED(result, L"Failed to reinitialize video encoder, aborting SubmitInput()");
                if (needsCSC == true)
                {
                    result = InitializeConverter(format, inputResolution, m_StreamResolution, encoderInputColorParams);
                    AMF_RETURN_IF_FAILED(result, L"Failed to reinitialize video converter, aborting SubmitInput()");
                }
                m_NeedsCSC = needsCSC;
                AMFTraceInfo(AMF_FACILITY, L"Video pipeline reinitialized, DCC: %s, EFC: %s, resolution %s, color primaries %s, input format %s",
                             bDCC == true ? L"yes" : L"no", needsCSC == false ? L"yes" : L"no",
                             resolutionChanged == true ? L"changed" : L"did not change", colorPrimariesChanged == true ? L"changed" : L"did not change", inputFormatChanged == true ? L"changed" : L"did not change");
            }   //  Reinitialization successfull
            else
            {   //  No reinitialization necessary
                needsCSC = m_NeedsCSC;   //  We only want to reinitialize under a lock, but not the actual submission
            }
            converter = m_Converter;    //  Also saving a pointer to the converter in a local variable to avoid unnecessary locking, only using locals below.
                                        //  The encoder is always present because it's passed to a constructor, so no need to worry about it changing on the fly

            //  Adjust the frame rate setting on the encoder to reflect the actual measured frame rate when frames are captured as Present or ASAP
            amf_pts now = amf_high_precision_clock();
            if (m_FrameCnt == 0)
            {
                m_FrameCntStartTime = now;
            }
            ++m_FrameCnt;
            constexpr static amf_pts FPS_MEASUREMENT_PERIOD = 3;
            amf_pts timeSinceHistoryStart = now - m_FrameCntStartTime;
            if (timeSinceHistoryStart > FPS_MEASUREMENT_PERIOD * AMF_SECOND)
            {
                AMFRate fps = { uint32_t(m_FrameCnt / (timeSinceHistoryStart / AMF_SECOND)), 1 };
                float prevFps = m_Encoder->GetFramerate();
                if (abs(fps.num - prevFps) > prevFps * 0.1) //  We don't want to change the encoder's frame rate too frequently by a small amount because it can reduce image quality, but when the frame changes significantly, we want to reflect that for a more optimal compression
                {
                    m_Encoder->UpdateFramerate(fps);
                }
                m_FrameCnt = 0;
            }
        }

        input->SetProperty(ORIGIN_PTS_PROPERTY, originPts);
        input->SetProperty(VIDEO_IN_PTS, videoInPts);
        if (needsCSC == true && converter != nullptr)
        {   //  CSC/scaling is needed
            amf::AMFVariant varBufMaster;
            AMF_RESULT  res = input->GetProperty(AMF_VIDEO_COLOR_HDR_METADATA, &varBufMaster);
            converter->SetProperty(AMF_VIDEO_CONVERTER_OUTPUT_HDR_METADATA, (res == AMF_OK) && (varBufMaster.type == amf::AMF_VARIANT_INTERFACE) ? varBufMaster : static_cast<amf::AMFInterface*>(nullptr));

            //  Do CSC/scaling to encoder's desired format and resolution:
            amf::AMFDataPtr converterInput(input);
            result = converter->SubmitInput(converterInput);
            AMF_RETURN_IF_FAILED(result, L"Failed to submit a frame to video converter, result=%s", amf::AMFGetResultText(result));
            do
            {
                result = converter->QueryOutput(&converterOutput);   //  Converter output is going to be encoder input
            } while ((result != AMF_OK && result != AMF_FAIL) || converterOutput == nullptr);
            if (converterOutput != nullptr && varBufMaster.type == amf::AMF_VARIANT_INTERFACE)    //  VideoConverter does not propagate HDR metadata
            {
                converterOutput->SetProperty(AMF_VIDEO_COLOR_HDR_METADATA, varBufMaster);
            }
        }
        else
        {   //  CSC/scaling is not needed, submit input to encoder directly (EFC)
            converterOutput = input;
        }

        if (converterOutput != nullptr)
        {
            if (IsKeyFrameRequested() == true)
            {
                m_Encoder->ForceKeyFrame(converterOutput);
                AMFTraceInfo(AMF_FACILITY, L"Key/IDR frame requested from encoder");
            }
            //  Submit a frame to the encoder:
            do
            {
                amf::AMFSurfacePtr encoderInput(converterOutput);
                encoderInput->SetProperty(VIDEO_ENCODER_IN_PTS, amf_high_precision_clock());
                result = m_Encoder->SubmitInput(encoderInput);
                switch (result)
                {
                case AMF_OK:
                    m_FramesSubmitted++;
                    break;
                case AMF_INPUT_FULL:
                    amf_sleep(1);
                    break;
                default:
                    AMF_RETURN_IF_FAILED(result, L"Failed to submit a frame to video encoder, result=%s", amf::AMFGetResultText(result));
                    break;
                }
            } while (result == AMF_INPUT_FULL);
        }
        return result;
    }

    uint32_t MonoscopicVideoOutput::GetColorDepthInBits() const noexcept
    {
        return m_HDR == true && m_Encoder->IsHDREnabled() == true ? 10 : 8;
    }

    AMFRect MonoscopicVideoOutput::CalculateViewport() const noexcept
    {
        AMFRect viewport = {};
        double inputAspectRatio = double(m_InputResolution.width) / double(m_InputResolution.height);
        double streamAspectRatio = double(m_StreamResolution.width) / double(m_StreamResolution.height);
        if (inputAspectRatio == streamAspectRatio)
        {
            viewport = AMFConstructRect(0, 0, m_StreamResolution.width, m_StreamResolution.height);
        }
        else if (inputAspectRatio < streamAspectRatio)  //  Input is narrower than stream, black bars on the sides
        {
            int32_t viewportWidth = static_cast<int32_t>(m_StreamResolution.height * inputAspectRatio);
            viewport.left = (m_StreamResolution.width - viewportWidth) / 2;
            viewport.right = m_StreamResolution.width - viewport.left;
            viewport.top = 0;
            viewport.bottom = m_StreamResolution.height;
        }
        else    // if (inputAspectRatio > streamAspectRatio)    //  Input is wider than stream, black bars above and below
        {
            int32_t viewportHeight = static_cast<int32_t>(m_StreamResolution.width / inputAspectRatio);
            viewport.top = (m_StreamResolution.height - viewportHeight) / 2;
            viewport.bottom = m_StreamResolution.height - viewport.top;
            viewport.left = 0;
            viewport.right = m_StreamResolution.width;
        }
        return viewport;
    }

    void MonoscopicVideoOutput::PollForEncoderOutput()
    {
        AMF_RESULT result = AMF_OK;
        uint32_t bitsPerPixel;
        AMFRect viewPort;
        amf::AMFBufferPtr extraData;
        transport_common::InitID initID;
        AMFSize resolution;
        {
            amf::AMFLock lock(&m_Guard);
            bitsPerPixel = GetColorDepthInBits();
            viewPort = CalculateViewport();
            resolution = m_StreamResolution;
            extraData = m_ExtraData;
            initID = m_InitID;
        }
        if (GetExtraData(&extraData, initID) == true)
        {   //  Encoder has been reinitialized, send the new init block to all clients
            m_TransportAdapter.SendVideoInit(m_Encoder->GetCodecName(),initID, resolution, viewPort, bitsPerPixel, false, false, extraData);
        }

        amf::AMFBufferPtr compressedFrame;
        transport_common::VideoFrame::SubframeType frameType;
        result = m_Encoder->QueryOutput(&compressedFrame, frameType);
        if (compressedFrame != nullptr)
        {   //  Encoder has produced some output - prepare and send the frame to all clients
            amf_pts originPts = 0;
            compressedFrame->GetProperty(ORIGIN_PTS_PROPERTY, &originPts);
            amf_pts encoderInPts = 0;
            if (compressedFrame->GetProperty(VIDEO_ENCODER_IN_PTS, &encoderInPts) == AMF_OK)
            {
                amf_pts encoderOutPts = amf_high_precision_clock();
                compressedFrame->SetProperty(VIDEO_ENCODER_LATENCY_PTS, encoderOutPts - encoderInPts);
            }

            transport_common::TransmittableVideoFrame frame(transport_common::TransmittableVideoFrame::ViewType::MONOSCOPIC, originPts, m_SequenceNumber++, false);
            frame.AddSubframe(frameType, compressedFrame);


            ssdk::util::QoS::VideoOutputStats videoOutputStats;
            videoOutputStats.encoderQueueDepth = m_FramesSubmitted - m_SequenceNumber;
            videoOutputStats.encoderTargetBitrate = m_Bitrate;// is always the same as m_Encoder->GetBitrate();
            videoOutputStats.encoderTargetFramerate = m_FrameRate; // is always the same as m_Encoder->GetFramerate()
            videoOutputStats.bandwidth = frame.CalculateRequiredBufferSize();

            m_TransportAdapter.SendVideoFrame(frame, videoOutputStats);

#ifdef _DEBUG
            if (frameType == transport_common::VideoFrame::SubframeType::IDR)
            {
                AMFTraceDebug(AMF_FACILITY, L"IDR/Key frame generated");
            }
#endif
        }
        else
        {
            amf_sleep(1);
        }
    }

    void MonoscopicVideoOutput::SaveExtraData(amf::AMFBuffer* buffer)
    {
        amf::AMFLock lock(&m_Guard);
        m_ExtraData = buffer;
        m_InitID = amf_high_precision_clock();
    }

    bool MonoscopicVideoOutput::GetExtraData(amf::AMFBuffer** buffer, amf_pts& id)
    {
        amf::AMFLock lock(&m_Guard);
        *buffer = m_ExtraData;
        (*buffer)->Acquire();
        id = m_InitID;
        bool result = m_LastRetrievedExtraDataID != m_InitID;
        m_LastRetrievedExtraDataID = m_InitID;
        return result;
    }

    void MonoscopicVideoOutput::EncoderPoller::Run()
    {
        while (StopRequested() == false)
        {
            m_pPipeline->PollForEncoderOutput();
        }
    }

    bool MonoscopicVideoOutput::UpdateColorParametersIfChanged(amf::AMFSurface* input)
    {
        bool result = false;
        int64_t colorPrimaries = AMF_COLOR_PRIMARIES_UNDEFINED;
        input->GetProperty(AMF_VIDEO_COLOR_PRIMARIES, &colorPrimaries);

        if (colorPrimaries != m_CurrentInputColorParams.GetColorPrimaries())
        {
            AMFTraceInfo(AMF_FACILITY, L"Video Input color primaries changed from %d to %d", m_CurrentInputColorParams.GetColorPrimaries(), colorPrimaries);
            m_CurrentInputColorParams.SetColorPrimaries(AMF_COLOR_PRIMARIES_ENUM(colorPrimaries));
            result = true;
        }

        int64_t transfer = AMF_COLOR_TRANSFER_CHARACTERISTIC_UNDEFINED;
        input->GetProperty(AMF_VIDEO_COLOR_TRANSFER_CHARACTERISTIC, &transfer);
        if (transfer != m_CurrentInputColorParams.GetXferCharacteristic())
        {
            AMFTraceInfo(AMF_FACILITY, L"Video transfer characteristic changed from %d to %d", m_CurrentInputColorParams.GetXferCharacteristic(), transfer);
            m_CurrentInputColorParams.SetXferCharacteristic(static_cast<AMF_COLOR_TRANSFER_CHARACTERISTIC_ENUM>(transfer));
        }

        return result;
    }

    bool MonoscopicVideoOutput::UpdateInputResolutionIfChanged(amf::AMFSurface* input)
    {
        bool result = false;
        AMFSize frameResolution(GetSurfaceResolution(input));
        if (frameResolution != m_InputResolution)
        {
            AMFTraceInfo(AMF_FACILITY, L"Video Input resolution changed from %dx%d to %dx%d", m_InputResolution.width, m_InputResolution.height, frameResolution.width, frameResolution.height);
            m_InputResolution = frameResolution;
            result = true;
        }

        int64_t orientation = amf::AMF_ROTATION_ENUM::AMF_ROTATION_NONE;
        input->GetProperty(AMF_SURFACE_ROTATION, &orientation);
        if (orientation != m_Orientation)
        {
            AMFTraceInfo(AMF_FACILITY, L"Video Input orientation changed from %dx to %dx degrees", int32_t(m_Orientation) * 90, int32_t(orientation) * 90);
            m_Orientation = static_cast<amf::AMF_ROTATION_ENUM>(orientation);
            result = true;
        }

        return result;
    }

    AMFSize MonoscopicVideoOutput::GetSurfaceResolution(amf::AMFSurface* input) const
    {
        return AMFSize(input->GetPlaneAt(0)->GetWidth(), input->GetPlaneAt(0)->GetHeight());
    }

    bool MonoscopicVideoOutput::UpdateInputFormatIfChanged(amf::AMFSurface* input)
    {
        bool result = false;
        amf::AMF_SURFACE_FORMAT format = input->GetFormat();
        if (format != m_CurrentInputColorParams.GetFormat())
        {
            AMFTraceInfo(AMF_FACILITY, L"Video Input format changed from %s to %s", amf::AMFSurfaceGetFormatName(m_CurrentInputColorParams.GetFormat()), amf::AMFSurfaceGetFormatName(format));
            m_CurrentInputColorParams.SetFormat(format);
            result = true;
        }
        return result;
    }

    void MonoscopicVideoOutput::DetermineEncoderInputColorParams(amf::AMFSurface* input, bool needsConverter, bool hdr, ColorParameters& inputColorParams)
    {
        amf::AMF_SURFACE_FORMAT preferredHDRFormat = m_Encoder->GetPreferredHDRFormat();
        amf::AMF_SURFACE_FORMAT preferredSDRFormat = m_Encoder->GetPreferredSDRFormat();
        //  When input is not known yet or when not using EFC, the encoder input needs to be configured with defaults
        //  and VideoConverter output to be configured to match encoder input
        if (input == nullptr || needsConverter == true)
        {
            if (hdr == true)
            {
                inputColorParams.SetXferCharacteristic(AMF_COLOR_TRANSFER_CHARACTERISTIC_SMPTE2084);
                inputColorParams.SetColorPrimaries(AMF_COLOR_PRIMARIES_BT2020);
                inputColorParams.SetColorProfile(AMF_VIDEO_CONVERTER_COLOR_PROFILE_FULL_2020);
                inputColorParams.SetFormat(preferredHDRFormat);
            }
            else
            {
                inputColorParams.SetXferCharacteristic(AMF_COLOR_TRANSFER_CHARACTERISTIC_BT709);
                inputColorParams.SetColorPrimaries(AMF_COLOR_PRIMARIES_BT709);
                inputColorParams.SetColorProfile(AMF_VIDEO_CONVERTER_COLOR_PROFILE_FULL_709);
                inputColorParams.SetFormat(preferredSDRFormat);
            }
        }
        else if (needsConverter == false)
        {
            int64_t transfer = AMF_COLOR_TRANSFER_CHARACTERISTIC_UNDEFINED;
            input->GetProperty(AMF_VIDEO_COLOR_TRANSFER_CHARACTERISTIC, &transfer);
            if (transfer != AMF_COLOR_TRANSFER_CHARACTERISTIC_UNDEFINED)
            {
                inputColorParams.SetXferCharacteristic((AMF_COLOR_TRANSFER_CHARACTERISTIC_ENUM)transfer);
            }
            int64_t primaries = AMF_COLOR_PRIMARIES_UNDEFINED;
            input->GetProperty(AMF_VIDEO_COLOR_PRIMARIES, &primaries);
            if (primaries != AMF_COLOR_PRIMARIES_UNDEFINED)
            {
                inputColorParams.SetColorPrimaries((AMF_COLOR_PRIMARIES_ENUM)primaries);
            }
            inputColorParams.SetColorProfile(primaries == AMF_COLOR_PRIMARIES_BT2020 ? AMF_VIDEO_CONVERTER_COLOR_PROFILE_FULL_2020 : AMF_VIDEO_CONVERTER_COLOR_PROFILE_FULL_709);
            inputColorParams.SetFormat(input->GetFormat());
        }

    }

}
