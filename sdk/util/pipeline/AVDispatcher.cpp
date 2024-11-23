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

#include "AVDispatcher.h"
#include "amf/public/common/TraceAdapter.h"

static constexpr const wchar_t* const AMF_FACILITY = L"AVispatcher";

namespace ssdk::util
{
    AVDispatcher::AVDispatcher(amf::AMFContext* context) :
        m_Context(context)
    {
    }

    AVDispatcher::~AVDispatcher()
    {
    }

    void AVDispatcher::RegisterPipelineForStream(ssdk::transport_common::StreamID streamID, ssdk::util::AVPipeline::Ptr pipeline)
    {
        amf::AMFLock lock(&m_Guard);
        m_StreamHandlers[streamID] = pipeline;
    }

    bool AVDispatcher::UnregisterPipelineForStream(ssdk::transport_common::StreamID streamID)
    {
        amf::AMFLock lock(&m_Guard);
        return m_StreamHandlers.erase(streamID) != 0;
    }

    AMF_RESULT AVDispatcher::AllocAMFBufferForBlock(const void* initBlock, size_t initBlockSize, amf::AMFBuffer** buf)
    {
        AMF_RETURN_IF_FALSE(buf != nullptr, AMF_INVALID_ARG, L"AVDispatcher::AllocAMFBufferFor(): buf cannot be NULL");
        AMF_RESULT result = AMF_OK;
        amf::AMFBufferPtr initBlockBuf;
        if (initBlock != nullptr && initBlockSize > 0)
        {
            result = m_Context->AllocBuffer(amf::AMF_MEMORY_HOST, initBlockSize, &initBlockBuf);
            if (initBlockBuf != nullptr)
            {
                memcpy(initBlockBuf->GetNative(), initBlock, initBlockSize);
                *buf = initBlockBuf;
                (*buf)->Acquire();
                result = AMF_OK;
            }
        }
        return result;
    }
}