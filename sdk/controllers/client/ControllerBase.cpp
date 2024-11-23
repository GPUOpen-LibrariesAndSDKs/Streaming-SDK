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
// Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
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

#include "ControllerBase.h"
#include "ControllerManager.h"

namespace ssdk::ctls
{
    static constexpr const wchar_t* AMF_FACILITY = L"ControllerBase";

    //-------------------------------------------------------------------------------------------------
    ControllerBase::ControllerBase(ControllerManagerPtr pControllerManager) :
        m_pControllerManager(pControllerManager)
    {
    }
    ControllerBase::~ControllerBase()
    {
        if (m_pConfig != nullptr)
        {
            m_pConfig->Detach(this);
        }
    }

    //-------------------------------------------------------------------------------------------------
    void AMF_STD_CALL ControllerBase::SetConfig(const ControllerConfigPtr& pConfig)
    {
        amf::AMFLock lock(&m_GuardConfig);

        if (m_pConfig != nullptr)
        {
            m_pConfig->Detach(this);
            m_pConfig.reset();
        }
        if (pConfig != nullptr)
        {
            m_pConfig = pConfig;
            m_pConfig->Notify();
        }
    }

    ControllerConfigPtr AMF_STD_CALL ControllerBase::GetConfig() const
    {
        amf::AMFLock lock(&m_GuardConfig);

        return m_pConfig;
    }

    //-------------------------------------------------------------------------------------------------
    // Called by Subject (Config) to update controller once config has been changed
    void AMF_STD_CALL ControllerBase::OnConfigUpdated()
    {
        Update();
    }

    //-------------------------------------------------------------------------------------------------
    // ControllerConfig class implementation
    //------------------------------------------------------------------------------------------------
    void AMF_STD_CALL ControllerConfig::Attach(ControllerBase* observer)
    {
        amf::AMFLock lock(&m_Guard);

        m_Observers.push_back(observer);
    }

    void AMF_STD_CALL ControllerConfig::Detach(ControllerBase* observer)
    {
        amf::AMFLock lock(&m_Guard);

        m_Observers.remove(observer);
    }

    //-------------------------------------------------------------------------------------------------
    // Notifies observers (controllers) that subject (config) has been updated
    void AMF_STD_CALL ControllerConfig::Notify()
    {
        amf::AMFLock lock(&m_Guard);

        std::list<ControllerBase*>::iterator it = m_Observers.begin();
        while (it != m_Observers.end())
        {
            (*it)->OnConfigUpdated();
            ++it;
        }
    }
}// namespace ssdk::ctls
