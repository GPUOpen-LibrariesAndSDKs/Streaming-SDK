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

#include "AsynchronousSlot.h"
#include "amf/public/common/TraceAdapter.h"

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::util::AsynchronousSlot";

namespace ssdk::util
{
    AsynchronousSlot::AsynchronousSlot(const char* slotName, amf::AMFComponent* component, PipelineSlot::Ptr nextSlot, int maxSubmissionAttempt) :
        ComponentSlot(slotName, component, nextSlot),
        m_MaxSubmissionAttempts(maxSubmissionAttempt),
        m_Pump(*this)
    {
    }

    AsynchronousSlot::~AsynchronousSlot()
    {
        Stop();
    }

    AMF_RESULT AsynchronousSlot::SubmitInput(amf::AMFData* input)
    {
        AMF_RESULT result = AMF_OK;
        AMF_RETURN_IF_FALSE(m_Component != nullptr, AMF_NOT_INITIALIZED, L"Component is NULL in slot \"%S\"", m_Name.c_str());
        AMF_RETURN_IF_FALSE(input != nullptr, AMF_INVALID_ARG, L"Input to slot \"%S\" should not be NULL", m_Name.c_str());

        bool resubmit = false;
        int resubmitCount = 0;
        do
        {
            if (IsTerminated() == true)
            {
                resubmit = false;
                result = AMF_NOT_INITIALIZED;
            }
            else
            {
                result = m_Component->SubmitInput(input);
                switch (result)
                {
                case AMF_OK:
                case AMF_NEED_MORE_INPUT:
                    resubmit = false;
                    break;
                case AMF_INPUT_FULL:
                    resubmit = true;
                    ++resubmitCount;
                    amf_sleep(1);
                    break;
                default:
                    resubmit = false;
                    AMFTraceError(AMF_FACILITY, L"Failed to submit input to component in slot \"%S\", result=%s", m_Name.c_str(), amf::AMFGetResultText(result));
                    break;
                }
            }
        } while (resubmit == true && resubmitCount < m_MaxSubmissionAttempts);
        if (resubmitCount == m_MaxSubmissionAttempts)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to submit input to component in slot \"%S\" after %d attempts, result=%s", m_Name.c_str(), resubmitCount, amf::AMFGetResultText(result));
        }

        return result;
    }

    void AsynchronousSlot::Start()
    {
//        amf::AMFLock lock(&m_Guard);
        m_Terminated = false;
        if (m_NextSlot != nullptr)
        {
            m_NextSlot->Start();
        }
        if (m_Pump.IsRunning() == false)
        {
            m_Pump.Start();
        }
    }

    void AsynchronousSlot::Stop()
    {
        if (m_Pump.IsRunning() == true)
        {
            m_Pump.RequestStop();
            m_Pump.WaitForStop();
        }
        if (m_NextSlot != nullptr)
        {
            m_NextSlot->Stop();
        }
//        amf::AMFLock lock(&m_Guard);
        m_Terminated = true;
    }

    bool AsynchronousSlot::IsTerminated() const noexcept
    {
//        amf::AMFLock lock(&m_Guard);
        return m_Terminated;
    }

    void AsynchronousSlot::PollForOutput()
    {
        amf::AMFDataPtr output;
        AMF_RESULT result = m_Component->QueryOutput(&output);
        if (result == AMF_OK)
        {
            if (output != nullptr)
            {   //  Got output, pass it on to the next slot down the pipeline
                if (m_NextSlot != nullptr)
                {
                    result = m_NextSlot->SubmitInput(output);
                    if (result != AMF_OK)
                    {
                        AMFTraceError(AMF_FACILITY, L"Slot \"%S\" successfully queried the output but failed to submit it to slot \"%S\", result=%s", m_Name.c_str(), m_NextSlot->GetName().c_str(), amf::AMFGetResultText(result));
                    }
                }
            }
            else
            {   //  No output, try again after 1ms to avoid burning CPU cycles
                amf_sleep(1);
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    AsynchronousSlot::OutPump::OutPump(AsynchronousSlot& slot) :
        m_Slot(slot)
    {
    }

    void AsynchronousSlot::OutPump::Run()
    {
        while (StopRequested() == false)
        {
            m_Slot.PollForOutput();
        }
    }
}