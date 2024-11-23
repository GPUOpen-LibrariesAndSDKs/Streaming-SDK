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

#include "SynchronousSlot.h"
#include "amf/public/common/TraceAdapter.h"

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::util::SynchronousSlot";

namespace ssdk::util
{
    SynchronousSlot::SynchronousSlot(const char* slotName, amf::AMFComponent* component, PipelineSlot::Ptr nextSlot, int maxSubmissionAttempt, int maxQueryOutputAttempts) :
        ComponentSlot(slotName, component, nextSlot),
        m_MaxSubmissionAttempts(maxSubmissionAttempt),
        m_MaxQueryOutputAttempts(maxQueryOutputAttempts)
    {
    }

    SynchronousSlot::~SynchronousSlot()
    {
        Stop();
    }

    AMF_RESULT SynchronousSlot::SubmitInput(amf::AMFData* input)
    {
        AMF_RESULT result = AMF_OK;
        AMF_RESULT submitInputResult = AMF_OK;
        AMF_RESULT queryOutputResult = AMF_OK;
        AMF_RETURN_IF_FALSE(m_Component != nullptr, AMF_NOT_INITIALIZED, L"Component is NULL in slot \"%S\"", m_Name.c_str());
        AMF_RETURN_IF_FALSE(m_NextSlot != nullptr, AMF_NOT_INITIALIZED, L"Slot's \"%S\" sink is NULL", m_Name.c_str());
        AMF_RETURN_IF_FALSE(input != nullptr, AMF_INVALID_ARG, L"Input to slot \"%S\" should not be NULL", m_Name.c_str());
        bool inputSubmitted = false;
        bool queryOutput = false;
        bool outputProduced = false;
        bool exitLoop = false;
        int submitInputAttempts = 0;
        int queryOutputAttempts = 0;
        do
        {
            if (IsTerminated() == true)
            {
                result = AMF_ACCESS_DENIED;
                exitLoop = true;
            }
            else
            {
                if (exitLoop == false && inputSubmitted == false)
                {
                    submitInputResult = m_Component->SubmitInput(input);
                    switch (submitInputResult)
                    {
                    case AMF_OK:
                        inputSubmitted = true;
                        queryOutput = true;
                        exitLoop = false;
                        break;
                    case AMF_NEED_MORE_INPUT:   //  Some components might want more than one input before they can produce output, so AMF_NEED_MORE_INPUT is NOT an error
                        inputSubmitted = true;
                        queryOutput = false;
                        exitLoop = true;
                        AMFTraceDebug(AMF_FACILITY, L"Need more Input in slot \"%S\", result=%s", m_Name.c_str(), amf::AMFGetResultText(submitInputResult));
                        break;
                    case AMF_INPUT_FULL:    //  Will need to QueryOutput and resubmit
                        inputSubmitted = false;
                        queryOutput = true;
                        exitLoop = false;
                        ++submitInputAttempts;
                        AMFTraceDebug(AMF_FACILITY, L"Input full in slot \"%S\", result=%s", m_Name.c_str(), amf::AMFGetResultText(submitInputResult));
                        break;
                    default:
                        AMFTraceError(AMF_FACILITY, L"Failed to submit input to component in slot \"%S\", result=%s", m_Name.c_str(), amf::AMFGetResultText(submitInputResult));
                        exitLoop = true;
                        result = submitInputResult;
                        break;
                    }
                }
                if (exitLoop == false && queryOutput == true)
                {
                    amf::AMFDataPtr output;
                    if ((exitLoop = IsTerminated()) == false)
                    {
                        do
                        {
                            queryOutputResult = m_Component->QueryOutput(&output);
                            if (queryOutputResult == AMF_OK)
                            {
                                if (output != nullptr)
                                {
                                    outputProduced = true;
                                    exitLoop = inputSubmitted;
                                    result = m_NextSlot->SubmitInput(output);
                                    if (result != AMF_OK)
                                    {
                                        AMFTraceError(AMF_FACILITY, L"Slot \"%S\" successfully queried the output but failed to submit it to slot \"%S\", result=%s", m_Name.c_str(), m_NextSlot->GetName().c_str(), amf::AMFGetResultText(result));
                                        exitLoop = true;
                                    }
                                }
                            }
                            else if (queryOutputResult != AMF_REPEAT)
                            {
                                ++queryOutputAttempts;
                                outputProduced = false;
                                amf_sleep(1);
                            }
                            else
                            {
                                AMFTraceError(AMF_FACILITY, L"Failed to query output from component in slot \"%S\", result=%s", m_Name.c_str(), amf::AMFGetResultText(queryOutputResult));
                                exitLoop = true;
                                outputProduced = false;
                                break;
                            }
                        } while (outputProduced == false && queryOutputAttempts < m_MaxQueryOutputAttempts);
                    }
                }
            }
        } while (exitLoop == false && submitInputAttempts < m_MaxSubmissionAttempts && queryOutputAttempts < m_MaxQueryOutputAttempts);
        if (inputSubmitted == false)
        {
            AMFTraceError(AMF_FACILITY, L"Slot \"%S\" failed to submit input after %d attempts, result=%s", m_Name.c_str(), submitInputAttempts, amf::AMFGetResultText(submitInputResult));
            result = AMF_FAIL;
        }
        if (outputProduced == false)
        {
            AMFTraceError(AMF_FACILITY, L"Slot \"%S\" failed to query output after %d attempts, result=%s", m_Name.c_str(), queryOutputAttempts, amf::AMFGetResultText(queryOutputResult));
            result = AMF_FAIL;
        }
        return result;
    }

    void SynchronousSlot::Start()
    {
        amf::AMFLock lock(&m_Guard);
        m_Terminated = false;
        if (m_NextSlot != nullptr)
        {
            m_NextSlot->Start();
        }
    }

    void SynchronousSlot::Stop()
    {
        amf::AMFLock lock(&m_Guard);
        m_Terminated = true;
        if (m_NextSlot != nullptr)
        {
            m_NextSlot->Stop();
        }
    }

    bool SynchronousSlot::IsTerminated() const noexcept
    {
        amf::AMFLock lock(&m_Guard);
        return m_Terminated;
    }

}