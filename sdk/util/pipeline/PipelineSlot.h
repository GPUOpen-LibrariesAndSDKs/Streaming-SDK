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

#pragma once

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#include "amf/public/include/components/Component.h"
#include "amf/public/common/Thread.h"

#include <memory>
#include <string>

namespace ssdk::util
{
    class PipelineSlot
    {
    public:
        typedef std::shared_ptr<PipelineSlot>   Ptr;

    protected:
        PipelineSlot(const char* slotName);
        virtual ~PipelineSlot();

    public:
        virtual void Start() = 0;
        virtual void Stop() = 0;

        virtual AMF_RESULT SubmitInput(amf::AMFData* input) = 0;
        virtual AMF_RESULT Flush() = 0;

        inline const std::string& GetName() const noexcept { return m_Name; }

    protected:
        std::string             m_Name;
    };

    class ComponentSlot : public PipelineSlot
    {
    public:
        typedef std::shared_ptr<ComponentSlot>   Ptr;

        static constexpr const int MAX_SUBMIT_INPUT_ATTEMPTS = 100;

    protected:
        ComponentSlot(const char* slotName, amf::AMFComponent* component, PipelineSlot::Ptr nextSlot);
    public:
        virtual AMF_RESULT Flush() override;

    protected:
        mutable amf::AMFCriticalSection     m_Guard;
        PipelineSlot::Ptr                   m_NextSlot;
        amf::AMFComponentPtr                m_Component;
    };

    class SinkSlot : public PipelineSlot
    {
    public:
        typedef std::shared_ptr<SinkSlot>   Ptr;

    protected:
        SinkSlot(const char* slotName);
    };
}
