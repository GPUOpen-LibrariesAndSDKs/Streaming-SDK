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

#include <string>

namespace ssdk::net
{
    class Url
    {
    public:
        enum class Result
        {
            OK,
            INVALID_URL,
            INVALID_PROTOCOL,
            INVALID_HOST,
            INVALID_PORT
        };

    public:
        Url();
        Url(const std::string& url, const std::string& defaultProtocol, unsigned short defaultPort);

        Result Init(const std::string& url, const std::string& defaultProtocol, unsigned short defaultPort);

        const std::string GetUrl() const throw();
        Result SetUrl(const std::string& url);

        inline const std::string& GetProtocol() const throw() { return m_Protocol; }
		inline void SetProtocol(const std::string& protocol) throw() { m_Protocol = protocol; }

        inline const std::string& GetHost() const throw() { return m_Host; }
		inline void SetHost(const std::string& host) throw() { m_Host = host; }

        inline unsigned short GetPort() const throw() { return m_Port; }
		void SetPort(unsigned short port) throw();

        inline const std::string& GetPortAsString() const throw() { return m_PortStr; }
         bool operator==(const Url& rhs) const;
         bool operator!=(const Url& rhs) const;

        static std::string CreateUrl(const std::string& protocol, const std::string& host, unsigned short port);

    private:
        Result FindProtocol(const std::string& url, const std::string& defaultProtocol, std::string* protocol) const;
        Result FindPort(const std::string& url, unsigned short defaultPort, std::string* portStr, unsigned short* port) const;
        Result FindHost(const std::string& url, std::string* host) const;

        void Trace(const std::wstring& message, const std::string& url, Url::Result error) const;

    private:
        std::string    m_Protocol;
        std::string    m_Host;
        std::string    m_PortStr;
        unsigned short m_Port;
    };
}
