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

#include "CodecDescriptor.h"
#include "Video.h"
#include "Audio.h"
#include "Controller.h"

#include "amf/public/include/core/Variant.h"
#include "amf/public/include/core/PropertyStorageEx.h"

namespace ssdk::transport_common
{
    enum class Result
    {
        OK = 0,
        FAIL,
        INVALID_ARG,
        PORT_BUSY,
        SESSION_TIMEOUT,
        SESSION_TERMINATING,
        SESSION_NOT_READY,
        NOT_WAITING_FOR_INCOMING,
        ALREADY_RUNNING,
        NOT_RUNNING,
        INVALID_MESSAGE,
        SERVERS_NOT_ENUMERATED,
        SERVER_NOT_AVAILABLE,
        CANT_SET_WHILE_RUNNING,
        CONNECTION_REFUSED,
        CONNECTION_TIMEOUT,
        CONNECTION_INVALID_VERSION,
        ALREADY_EXISTS,
        NOT_FOUND
    };

    class Transport
    {
    protected:
        Transport() = default;

    public:
        virtual ~Transport() = default;
    };

    typedef int64_t StreamID;
    constexpr const StreamID DEFAULT_STREAM = 0;
    constexpr const StreamID INVALID_STREAM = -1;

    typedef int64_t InitID;
    constexpr const InitID INVALID_INIT_ID = -1;

    typedef int64_t SessionHandle;
    constexpr const SessionHandle INVALID_SESSION_HANDLE = -1;
}
