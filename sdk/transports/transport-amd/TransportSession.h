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

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#include "Channels.h"
#include "amf/public/include/core/Interface.h"
#include "transports/transport-common/ServerTransport.h"

namespace ssdk::transport_amd
{
    class Session;
    //---------------------------------------------------------------------------------------------
    //  A callback interface which receives session's events. Must be implemented by the user and
    //  registered with a session upon its creation through Session::RegisterReceiverCallback()
    //  You can (and should) register separate callbacks for different channels, although a common
    //  callback is also possible
    //---------------------------------------------------------------------------------------------
    class ReceiverCallback
    {
    public:
        enum class TerminationReason
        {
            DISCONNECT,
            TIMEOUT
        };

        //  Called when the client receives a new message. The message parameter points to a buffer containing the
        //  entire message. Unless buffers are allocated by the user using the BufferAllocator interface (below),
        //  DO NOT make any assumptions about the lifecycle of the buffer. If you need to preserve the data, either copy
        //  it to your own buffer, or manage buffers yourself through BufferAllocator.
        virtual void            AMF_STD_CALL OnMessageReceived(Session* session, Channel channel, int msgID, const void* message, size_t messageSize) = 0;
        virtual void            AMF_STD_CALL OnTerminate(Session* session, TerminationReason reason) = 0;
    };

    //---------------------------------------------------------------------------------------------
    class Session : public amf::AMFInterface
    {
    public:
        typedef amf::AMFInterfacePtr_T<Session> Ptr;
        AMF_DECLARE_IID(0xa394e28d, 0xa77, 0x47f9, 0x9b, 0xe4, 0xf5, 0x88, 0xf3, 0x67, 0xeb, 0x78);

        //  Channel methods:
        virtual void                AMF_STD_CALL UpgradeProtocol(uint32_t version) = 0;
        virtual void                AMF_STD_CALL RegisterReceiverCallback(ReceiverCallback* callback) = 0;
        virtual void                AMF_STD_CALL Terminate() = 0;
        virtual bool                AMF_STD_CALL IsTerminated() const noexcept = 0;
        virtual transport_common::Result AMF_STD_CALL Send(Channel channel, const void* msg, size_t msgLen) = 0;
        virtual const char*         AMF_STD_CALL GetPeerPlatform() const noexcept = 0;
        virtual const char*         AMF_STD_CALL GetPeerUrl() const noexcept = 0;
        virtual ssdk::transport_common::SessionHandle AMF_STD_CALL GetSessionHandle() const noexcept = 0;
    };
    //---------------------------------------------------------------------------------------------
}
