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

#include "PipelineSlot.h"
#include "amf/public/common/TraceAdapter.h"

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::util::PipelineSlot";

namespace ssdk::util
{
    PipelineSlot::PipelineSlot(const char* slotName) :
        m_Name(slotName)
    {
    }

    PipelineSlot::~PipelineSlot()
    {
        AMFTraceDebug(AMF_FACILITY, L"Slot \"%S\" destroyed", m_Name.c_str());
    }

    ComponentSlot::ComponentSlot(const char* slotName, amf::AMFComponent* component, PipelineSlot::Ptr nextSlot) :
        PipelineSlot(slotName),
        m_Component(component),
        m_NextSlot(nextSlot)
    {
    }

    AMF_RESULT ComponentSlot::Flush()
    {
        amf::AMFLock lock(&m_Guard);

        AMF_RESULT result = AMF_OK;
        AMF_RESULT thisResult = AMF_OK;
        if (m_Component != nullptr)
        {
            thisResult = m_Component->Flush();
            if (thisResult != AMF_OK)
            {
                AMFTraceError(AMF_FACILITY, L"Component in slot \"%S\" failed to flush, result = %s", m_Name.c_str(), amf::AMFGetResultText(thisResult));
            }
        }
        if (m_NextSlot != nullptr)
        {
            result = m_NextSlot->Flush();
            if (result == AMF_OK)   //  If either this slot or any down the pipeline failed to flush, result will contain an error code
            {
                result = thisResult;
            }
        }
        return result;
    }

    SinkSlot::SinkSlot(const char* slotName) :
        PipelineSlot(slotName)
    {
    }
}