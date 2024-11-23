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

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "Url.h"
#include "amf/public/common/TraceAdapter.h"
#include <cstdlib>
#include <sstream>
#include <cctype>

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::net::Url";

namespace ssdk::net
{
    static const char REALM[] = "Url";
    static const char PROTOCOL_DELIM[] = "://";
    static const size_t PROTOCOL_DELIM_LEN = sizeof(PROTOCOL_DELIM) / sizeof(const char) - 1;
    static const char PORT_DELIM[] = ":";
    static const size_t PORT_DELIM_LEN = sizeof(PORT_DELIM) / sizeof(const char) - 1;

    Url::Url() :
        m_Port(0)
    {
    }

    Url::Url(const std::string& url, const std::string& defaultProtocol, unsigned short defaultPort)
    {
        Init(url, defaultProtocol, defaultPort);
    }

    void Url::Trace(const std::wstring& message, const std::string& url, Url::Result error) const
    {
        std::wstring errorMessage;

        switch (error)
        {
        case Result::INVALID_URL:
			errorMessage = L"Invalid URL";
            break;
        case Result::INVALID_PROTOCOL:
			errorMessage = L"Invalid protocol";
            break;
        case Result::INVALID_HOST:
			errorMessage = L"Invalid host";
            break;
        case Result::INVALID_PORT:
			errorMessage = L"Invalid port";
            break;
        default:
			errorMessage = L"Unknown error";
            break;
        }
		AMFTraceError(AMF_FACILITY, L"%s URL: \'%S\', error: %s", message.c_str(), url.c_str(), errorMessage.c_str());
    }

    Url::Result Url::Init(const std::string& url, const std::string& defaultProtocol, unsigned short defaultPort)
    {
        Url::Result result = Result::INVALID_URL;
        if (url.length() > 0)
        {
            if ((result = FindProtocol(url, defaultProtocol, &m_Protocol)) != Result::OK)
            {
                Trace(L"Url::Init()", url, result);
            }
            else if ((result = FindPort(url, defaultPort, &m_PortStr, &m_Port)) != Result::OK)
            {
                Trace(L"Url::Init()", url, result);
            }
            else if ((result = FindHost(url, &m_Host)) != Result::OK)
            {
                Trace(L"Url::Init()", url, result);
            }
        }
        return result;
    }

    bool Url::operator==(const Url& rhs) const
    {
        return (m_Protocol == rhs.m_Protocol &&
                m_Host == rhs.m_Host &&
                m_PortStr == rhs.m_PortStr);
    }

    bool Url::operator!=(const Url& rhs) const
    {
        return !(operator==(rhs));
    }

    std::string Url::CreateUrl(const std::string& protocol, const std::string& host, unsigned short port)
    {
        std::stringstream url;
        url << protocol;
        url << "://";
        url << host;
        url << ":";
        url << port;
        return url.str();
    }

    Url::Result Url::FindProtocol(const std::string& url, const std::string& defaultProtocol, std::string* protocol) const
    {
        Url::Result result = Result::OK;
        std::string::size_type protocolDelimPos = url.find(PROTOCOL_DELIM);
        if (protocolDelimPos != url.npos)
        {
            protocol->assign(url, 0, protocolDelimPos);
            result = Result::OK;
        }
        else
        {
            protocol->assign(defaultProtocol);
            result = Result::OK;
        }
        for (std::string::iterator it = protocol->begin(); it != protocol->end(); ++it)
        {
            *it = (char) std::toupper(*it);
        }
        return result;
    }

    Url::Result Url::FindPort(const std::string& url, unsigned short defaultPort, std::string* portStr, unsigned short* port) const
    {
        Url::Result result = Result::INVALID_PORT;
        std::string::size_type colonPos = url.rfind(PORT_DELIM);
        if (colonPos != url.npos && url.compare(colonPos, PROTOCOL_DELIM_LEN, PROTOCOL_DELIM) != 0)
        {
            if (colonPos < url.length() - 1)
            {
                portStr->assign(url, colonPos + 1, url.length() - colonPos);
                if (portStr->length() > 0)
                {
                    int numPort = std::atoi(portStr->c_str());
                    if (numPort > 0)
                    {
                        *port = static_cast<unsigned short>(numPort);
                        result = Result::OK;
                    }
                }
                else
                {
                    *port = defaultPort;
                    if (portStr != nullptr)
                    {
                        std::stringstream portStream;
                        portStream << defaultPort;
                        *portStr = portStream.str();
                    }
                    result = Result::OK;
                }
            }
        }
        else
        {
            *port = defaultPort;
            if (portStr != nullptr)
            {
                std::stringstream portStream;
                portStream << defaultPort;
                *portStr = portStream.str();
            }
            result = Result::OK;
        }
        return result;
    }

    Url::Result Url::FindHost(const std::string& url, std::string* host) const
    {
        Url::Result result = Result::INVALID_HOST;
        std::string::size_type startPos = url.find(PROTOCOL_DELIM);
        if (startPos == url.npos)
        {
            startPos = 0;
        }
        else
        {
            startPos += PROTOCOL_DELIM_LEN;
        }
        std::string::size_type endPos = url.rfind(PORT_DELIM);
        if (startPos >= endPos)
        {
			endPos = url.npos;
		}
        if (url.at(startPos) == '*' && (endPos - startPos == 1 || endPos == url.npos))
        {
            host->assign("*");
            result = Result::OK;
        }
        else
        {
            static const char forbiddenChars[] = ",:\"\'{}[]()<>!@#$%^&*";
            host->assign(url, startPos, endPos - startPos);
            if (host->find_first_of(forbiddenChars) == host->npos)
            {
                result = Result::OK;
            }
        }
        return result;
    }

	const std::string Url::GetUrl() const throw()
	{
		std::string url;
		url += m_Protocol;
		url += "://";
		url += m_Host;
		url += ":";
		url += m_PortStr;
		return url;
	}

    void Url::SetPort(unsigned short port) throw()
    {
        m_Port = port;
        std::stringstream portStr;
        portStr << port;
        m_PortStr = portStr.str();
    }

    Url::Result Url::SetUrl(const std::string& url)
	{
        return Init(url, "", 0);
	}
}
