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
#pragma once

#include "transports/transport-amd/messages/sensors/DeviceEvent.h"
#include "controllers/ControllerTypes.h"
#include "transports/transport-common/Transport.h"
#include "controllers/UserInput.h"
#include "amf/public/common/Thread.h"

namespace ssdk::ctls::svr
{
    //----------------------------------------------------------------------------------------------
    class ControllerManager;
    typedef std::shared_ptr<ControllerManager> ControllerManagerPtr;
    //-------------------------------------------------------------------------------------------------
    // ControllerBase for Server
    //-------------------------------------------------------------------------------------------------
    class ControllerBase
    {
    public:
        ControllerBase(ControllerManagerPtr pControllerManager) :
            m_pControllerManager(pControllerManager),
            m_eState(CTRL_DEVICE_STATE::DISCONNECTED)
        {
        }

        typedef std::shared_ptr<ControllerBase> Ptr;

        virtual const char* GetID() const { return m_ID.c_str(); }
        virtual CTRL_DEVICE_TYPE GetType() const { return CTRL_DEVICE_TYPE::UNKNOWN; }
        virtual CTRL_DEVICE_STATE GetState() const { return m_eState; }
        virtual void Update() {};

        virtual void ProcessInputEvent(const char* /*eventID*/, const ssdk::ctls::CtlEvent& /*event*/) {};
        virtual void OnConnectionEstablished() { m_eState = CTRL_DEVICE_STATE::CONNECTED; };
        virtual void OnConnectionTerminated() { m_eState = CTRL_DEVICE_STATE::DISCONNECTED; };

    protected:
        CTRL_DEVICE_STATE       m_eState;
        std::string             m_ID;
        ControllerManagerPtr    m_pControllerManager;
    };

} // namespace ssdk::ctls::svr
