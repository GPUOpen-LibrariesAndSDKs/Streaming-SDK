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


#include "VideoEncodeEngine.h"

#include "amf/public/include/core/VulkanAMF.h"
#include "amf/public/common/TraceAdapter.h"

#ifdef _WIN32
#include <dxgiformat.h>
#include <d3d11.h>
#include <d3d12.h>
#endif

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::video::VideoEncodeEngine";

namespace ssdk::video
{
    VideoEncodeEngine::VideoEncodeEngine(amf::AMFContext* context) :
        m_Context(context)
    {
    }

    VideoEncodeEngine::~VideoEncodeEngine()
    {
        Terminate();
    }

    AMF_RESULT VideoEncodeEngine::Init(const AMFSize& encoderResolution, int64_t bitrate, float frameRate, int64_t intraRefreshPeriod, const ColorParameters& inputColorParams, size_t instance)
    {
        m_Bitrate = bitrate;
        m_Resolution = encoderResolution;
        m_Framerate = frameRate;
        m_IntraRefreshPeriod = intraRefreshPeriod;
        m_InputColorParams = inputColorParams;
        m_Instance = instance;
        m_Initialized = true;

        return AMF_OK;
    }

    void VideoEncodeEngine::Terminate()
    {
        if (m_Encoder != nullptr)
        {
            m_Encoder->Terminate();
        }
        m_Initialized = false;
    }

    AMF_RESULT VideoEncodeEngine::SubmitInput(amf::AMFSurface* inputSurface)
    {
        AMF_RETURN_IF_FALSE(m_Encoder != nullptr, AMF_NOT_INITIALIZED, L"EncoderEngine::SubmitInput(): video encoder not initialized");
        return m_Encoder->SubmitInput(inputSurface);
    }

    AMF_RESULT VideoEncodeEngine::QueryOutput(amf::AMFBuffer** output, transport_common::VideoFrame::SubframeType& frameType)
    {
        AMF_RETURN_IF_FALSE(m_Encoder != nullptr, AMF_NOT_INITIALIZED, L"EncoderEngine::QueryOutput(): video encoder not initialized");
        amf::AMFDataPtr outputData;
        AMF_RESULT result = m_Encoder->QueryOutput(&outputData);
        if (outputData != nullptr)
        {
            amf::AMFBufferPtr outputBuffer(outputData);
            DetermineFrameType(outputBuffer, frameType);
            (*output) = outputBuffer.Detach();
        }
        else if (result != AMF_OK && result != AMF_REPEAT)
        {
            AMFTraceError(AMF_FACILITY, L"Video encoder QueryOutput() call failed with result = %s", amf::AMFGetResultText(result));
        }
        return result;
    }

    AMF_RESULT VideoEncodeEngine::Flush()
    {
        AMF_RETURN_IF_FALSE(m_Encoder != nullptr, AMF_NOT_INITIALIZED, L"EncoderEngine::Flush(): video encoder not initialized");
        return m_Encoder->Flush();
    }

    AMF_RESULT VideoEncodeEngine::Drain()
    {
        AMF_RETURN_IF_FALSE(m_Encoder != nullptr, AMF_NOT_INITIALIZED, L"EncoderEngine::Drain(): video encoder not initialized");
        return m_Encoder->Drain();
    }

    AMF_RESULT VideoEncodeEngine::SetProperty(const wchar_t* name, const amf::AMFVariant& value)
    {
        AMF_RETURN_IF_FALSE(m_Encoder != nullptr, AMF_NOT_INITIALIZED, L"EncoderEngine::SetProperty(): video encoder not initialized");
        return m_Encoder->SetProperty(name, value);
    }

    AMF_RESULT VideoEncodeEngine::GetProperty(const wchar_t* name, amf::AMFVariant& value) const
    {
        AMF_RETURN_IF_FALSE(m_Encoder != nullptr, AMF_NOT_INITIALIZED, L"EncoderEngine::GetProperty(): video encoder not initialized");
        return m_Encoder->GetProperty(name, &value);
    }

    bool VideoEncodeEngine::IsFormatSupported(amf::AMF_SURFACE_FORMAT defaultFormat, bool /*hdr*/, amf::AMFSurface* pSurface) const
    {
        bool result = false;
        amf::AMF_SURFACE_FORMAT format;
        if (pSurface != nullptr)
        {
            format = pSurface->GetFormat(); //  Encoder can be initialized before the first frame, in this case we check the default format, otherwise take it from the input surface
            amf::AMF_MEMORY_TYPE memType = pSurface->GetMemoryType();

            //  EFC is not supported for sRGB formats, force use of VideoConverter
#ifdef _WIN32
            if (memType == amf::AMF_MEMORY_DX11)
            {
                D3D11_TEXTURE2D_DESC desc = {};
                ((ID3D11Texture2D*)pSurface->GetPlaneAt(0)->GetNative())->GetDesc(&desc);
                if (desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB || desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB ||
                    desc.Format == DXGI_FORMAT_B8G8R8X8_UNORM_SRGB || (desc.Format == DXGI_FORMAT_R10G10B10A2_UNORM))
                {
                    return false;
                }
            }
            else if (memType == amf::AMF_MEMORY_DX12)
            {
                D3D12_RESOURCE_DESC desc = ((ID3D12Resource*)pSurface->GetPlaneAt(0)->GetNative())->GetDesc();
                if (desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB || desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB ||
                    desc.Format == DXGI_FORMAT_B8G8R8X8_UNORM_SRGB || (desc.Format == DXGI_FORMAT_R10G10B10A2_UNORM))
                {
                    return false;
                }
            }
            else
#endif
            if (memType == amf::AMF_MEMORY_VULKAN)
            {
                amf::AMFVulkanSurface* pSurfaceVulkan = (amf::AMFVulkanSurface*)pSurface->GetPlaneAt(0)->GetNative();
                if (pSurfaceVulkan->eFormat == VK_FORMAT_R8G8B8A8_SRGB || pSurfaceVulkan->eFormat == VK_FORMAT_B8G8R8A8_SRGB ||
                    pSurfaceVulkan->eFormat == VK_FORMAT_A2R10G10B10_UNORM_PACK32)
                {
                    return false;
                }
            }
        }
        else
        {
            format = defaultFormat;
        }
        //  Check whether the encoder accepts an input format through Capability Manager
        amf::AMFCapsPtr pCaps;
        if (m_Encoder->GetCaps(&pCaps) == AMF_OK && pCaps != nullptr)
        {
            amf::AMFIOCapsPtr inputCaps;
            if (pCaps->GetInputCaps(&inputCaps) == AMF_OK && inputCaps != nullptr)
            {
                int32_t numOfFormats = inputCaps->GetNumOfFormats();
                for (int32_t i = 0; i < numOfFormats; i++)
                {
                    amf::AMF_SURFACE_FORMAT curFormat = amf::AMF_SURFACE_UNKNOWN;
                    bool isNative = false;
                    inputCaps->GetFormatAt(i, &curFormat, &isNative);
                    if (curFormat == format)
                    {
                        result = true;
                        break;
                    }
                }
            }
        }
        return result;
    }

    AMF_COLOR_RANGE_ENUM VideoEncodeEngine::GetSupportedColorRange() const noexcept
    {
        return AMF_COLOR_RANGE_ENUM::AMF_COLOR_RANGE_FULL;
    }

    AMF_RESULT VideoEncodeEngine::GetNumOfEncoderInstancesPrivate(const wchar_t* codec, const wchar_t* propName, size_t& numOfInstances) const
    {
        AMF_RESULT result = AMF_OK;
        numOfInstances = 0;
        AMF_RETURN_IF_FALSE(m_Encoder != nullptr, AMF_FAIL, L"%s Encoder is not initialized", codec);

        amf::AMFCapsPtr pEncoderCaps;
        result = m_Encoder->GetCaps(&pEncoderCaps);
        AMF_RETURN_IF_FAILED(result, L"Failed to query the %s encoder capabilities", codec);
        int64_t instanceCnt = 1;
        pEncoderCaps->GetProperty(propName, &instanceCnt);
        numOfInstances = static_cast<size_t>(instanceCnt);
        return AMF_OK;
    }

    uint64_t VideoEncodeEngine::AlignValue(uint64_t value, uint64_t alignment)
    {
        return ((value + (alignment - 1)) & ~(alignment - 1));
    }

}
