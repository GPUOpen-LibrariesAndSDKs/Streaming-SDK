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

#include "UVDDecoderHEVC.h"
#include "amf/public/include/components/VideoDecoderUVD.h"
#include "amf/public/common/TraceAdapter.h"

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::video::UVDDecoderHEVC";

namespace ssdk::video
{
    UVDDecoderHEVC::UVDDecoderHEVC(amf::AMFContext* context, size_t instance) :
        UVDDecoder(context, instance)
    {
    }
    AMF_RESULT UVDDecoderHEVC::Init(amf::AMF_SURFACE_FORMAT format, const AMFSize& resolution, float frameRate, amf::AMFBuffer* initBlock)
    {
        if (m_Decoder == nullptr || format != m_Format)
        {
            m_Decoder = nullptr;
            AMF_RESULT result = g_AMFFactory.GetFactory()->CreateComponent(m_Context, format == amf::AMF_SURFACE_FORMAT::AMF_SURFACE_P010 ? AMFVideoDecoderHW_H265_MAIN10 : AMFVideoDecoderHW_H265_HEVC, &m_Decoder);
            AMF_RETURN_IF_FAILED(result, L"Failed to create a HEVC UVD decoder, result=%s", amf::AMFGetResultText(result));
            m_Format = format;
        }
        return UVDDecoder::Init(format, resolution, frameRate, initBlock);
    }
    const char* UVDDecoderHEVC::GetCodecName() const noexcept
    {
        return CODEC_HEVC;
    }

    bool UVDDecoderHEVC::IsHDRSupported() const noexcept
    {
        return true;
    }
}