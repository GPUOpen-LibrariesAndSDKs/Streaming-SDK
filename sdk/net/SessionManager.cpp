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
#include "SessionManager.h"
#include "amf/public/common/TraceAdapter.h"
#include <vector>
#include <sstream>
#ifndef _WIN32
#include <unistd.h>
#endif

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::net::SessionManager";

namespace ssdk::net
{
    SessionManager::SessionManager()
    {
    }

    SessionManager::~SessionManager()
    {
		TerminateSessions();
    }

    SessionManager::Result SessionManager::RegisterSession(Session* session)
    {
		amf::AMFLock lock(&m_Guard);
        SessionManager::Result result = Result::OK;
        if (m_Sessions.insert(session).second != true)
        {
            result = Result::SESSION_ALREADY_EXISTS;
			AMFTraceError(AMF_FACILITY, L"RegisterSession - Session already registered");
        }
        return result;
    }

	void SessionManager::CleanupTimedoutSessions(time_t disconnectTimeout)
	{
		amf::AMFLock lock(&m_Guard);
		if (m_Sessions.size() > 0)
		{
			time_t now = time(nullptr);
			SessionSet sessionsToBeRemain;
			for (SessionSet::const_iterator sessionIt = m_Sessions.begin(); sessionIt != m_Sessions.end(); ++sessionIt)
			{
                if ((*sessionIt)->IsTerminated() == false)
                {
                    time_t lastMessageReceived = (*sessionIt)->GetLastRequestTime();
                    if (now - lastMessageReceived > disconnectTimeout && m_sessionTimeoutEnabled == true)
                    {
                        AMFTraceInfo(AMF_FACILITY, L"Session timed out");
                        (*sessionIt)->OnSessionTimeout();
                    }
                    else
                    {
                        sessionsToBeRemain.insert(*sessionIt);
                    }
                }
                else
                {
                    (*sessionIt)->OnSessionClose();
                }
			}
			m_Sessions = sessionsToBeRemain;
		}

	}

	void SessionManager::TerminateSessions()
	{
		amf::AMFLock lock(&m_Guard);
		for (SessionSet::const_iterator sessionIt = m_Sessions.begin(); sessionIt != m_Sessions.end(); ++sessionIt)
		{
			(*sessionIt)->Terminate();
		}
		m_Sessions.clear();
	}

	std::string SessionManager::GetHostName() const
    {
        char hostName[256];
        ::gethostname(hostName, sizeof(hostName)-1);
        return hostName;
    }

    void SessionManager::SetSessionTimeoutEnabled(bool timeoutEnabled)
    {
        m_sessionTimeoutEnabled = timeoutEnabled;
    }

}
