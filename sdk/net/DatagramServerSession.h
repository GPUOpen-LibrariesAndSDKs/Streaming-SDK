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

#include "Session.h"

namespace ssdk::net
{
    class DatagramServerSession :
        public Session
    {
    public:
		DatagramServerSession(const Socket::Address& peer);
        virtual ~DatagramServerSession();

        typedef amf::AMFInterfacePtr_T<DatagramServerSession> Ptr;


        AMF_DECLARE_IID(0xeec5524f, 0x857b, 0x4f49, 0x95, 0x67, 0x60, 0x8, 0x3, 0x9f, 0xe3, 0x2f);

        // interface
        virtual bool   AMF_STD_CALL OnTickNotify() = 0;
		virtual Result AMF_STD_CALL OnDataReceived(const void* request, size_t requestSize, const Socket::Address& receivedFrom) = 0;
        virtual Socket::Result  AMF_STD_CALL Send(const void* buf, size_t size, size_t* const bytesSent, int flags) = 0;
    };
}
