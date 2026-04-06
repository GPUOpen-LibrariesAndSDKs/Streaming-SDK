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

#include "VideoReceiverPipeline.h"

#include "util/pipeline/SynchronousSlot.h"
#include "util/pipeline/AsynchronousSlot.h"

#include "amf/public/include/components/HQScaler.h"
#include "amf/public/include/components/VQEnhancer.h"
#include "amf/public/common/TraceAdapter.h"

static constexpr const wchar_t* const AMF_FACILITY = L"VideoPipeline";

namespace ssdk::video
{
    VideoReceiverPipeline::VideoReceiverPipeline(amf::AMFContext* context) :
        ssdk::util::AVPipeline(context)
    {
    }


    AMF_RESULT VideoReceiverPipeline::Init(bool preserveAspectRatio)
    {
        amf::AMFLock    lock(&m_Guard);
        m_PreserveAspectRatio = preserveAspectRatio;
        return AMF_OK;
    }

    AMF_RESULT VideoReceiverPipeline::OnInputChanged(const char* codec, ssdk::transport_common::InitID initID, const AMFSize& streamResolution, const AMFRect& viewport, uint32_t bitDepth, bool stereoscopic, bool foveated, amf::AMFBuffer* initBlock)
    {
        AMF_RESULT result = AMF_OK;
        if (codec == nullptr)
        {
            result = AMF_INVALID_ARG;
            AMFTraceError(AMF_FACILITY, L"Codec cannot be nullptr");
        }
        else
        {
            m_Codec = codec;

            m_Viewport = viewport;
            m_Stereoscopic = stereoscopic;
            m_Foveated = foveated;
            m_BitDepth = bitDepth;
            m_StreamResolution = streamResolution;
            m_InitBlock = initBlock;
            SetInitID(initID);
        }
        return result;
    }

    AMF_RESULT VideoReceiverPipeline::GetLastInitBlock(amf::AMFBuffer** initBlock) const
    {
        AMF_RESULT result = AMF_OK;
        if (initBlock == nullptr)
        {
            result = AMF_INVALID_ARG;
        }
        else
        {
            amf::AMFLock lock(&m_Guard);
            *initBlock = m_InitBlock;
            if (*initBlock != nullptr)
            {
                (*initBlock)->Acquire();
            }
        }
        return result;
    }
}
