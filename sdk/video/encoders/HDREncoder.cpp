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

#include "HDREncoder.h"
#include "amf/public/common/TraceAdapter.h"

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::video::HDREncoder";

namespace ssdk::video
{
    HDREncoder::HDREncoder(amf::AMFContext* context) :
        VideoEncodeEngine(context)
    {
    }

    AMF_RESULT HDREncoder::SubmitInput(amf::AMFSurface* input)
    {
        AMF_RESULT result = AMF_OK;
        if (input != nullptr)
        {
            // get the surface HDR metadata
            amf::AMFVariant varBufMaster;
            if ((input->GetProperty(AMF_VIDEO_COLOR_HDR_METADATA, &varBufMaster) == AMF_OK) &&
                (varBufMaster.type == amf::AMF_VARIANT_INTERFACE))
            {
                // if the format supports HDR
                if (input->GetFormat() == amf::AMF_SURFACE_P010 ||
                    input->GetFormat() == amf::AMF_SURFACE_RGBA_F16 ||
                    input->GetFormat() == amf::AMF_SURFACE_R10G10B10A2)
                {
                    result = SetHDRMetadata(varBufMaster);
                }
            }
            if (result != AMF_OK)
            {
                AMFTraceError(AMF_FACILITY, L"Failed to set HDR metadata, result %s, frame not submitted", amf::AMFGetResultText(result));
            }
            else
            {
                result = VideoEncodeEngine::SubmitInput(input);
            }
        }
        return result;
    }

    AMF_RESULT HDREncoder::EnableHDR(bool enable) noexcept
    {
        if (m_HDREnabled != enable)
        {
            m_HDREnabled = enable;
            ToggleHDR(enable);
        }
        return AMF_OK;
    }

}