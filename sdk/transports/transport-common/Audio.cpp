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

#include "Audio.h"
#include "amf/public/common/TraceAdapter.h"

#include <sstream>

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::transport_common::Audio";

namespace ssdk::transport_common
{
    AudioStreamDescriptor::AudioStreamDescriptor(const char* description, const char* codec, int32_t channels, int32_t layout, int32_t bitrate,
                                                 int32_t bitsPerSample, amf::AMF_AUDIO_FORMAT format, int32_t samplingRate, const char* language) :
        m_Codec(codec),
        m_Bitrate(bitrate),
        m_Channels(channels),
        m_Layout(layout),
        m_SamplingRate(samplingRate),
        m_BitsPerSample(bitsPerSample),
        m_Format(format)
    {
        if (language != nullptr)
        {
            m_Language = language;
        }

        if (description != nullptr)
        {
            m_Description = description;
        }
        else
        {
            std::stringstream str;
            str << m_Language << (m_Language.length() > 0 ? ", " : "") << m_SamplingRate << "Hz, " << float(bitrate) / 1024 << "kbps, ";
            switch (layout)
            {
            case amf::AMFACL_SPEAKER_FRONT_CENTER:
                str << "mono";
                break;
            case amf::AMFACL_SPEAKER_FRONT_LEFT | amf::AMFACL_SPEAKER_FRONT_RIGHT:
                str << "stereo 2.0";
                break;
            case amf::AMFACL_SPEAKER_FRONT_LEFT | amf::AMFACL_SPEAKER_FRONT_RIGHT | amf::AMFACL_SPEAKER_LOW_FREQUENCY:
                str << "stereo 2.1";
                break;
            case amf::AMFACL_SPEAKER_FRONT_RIGHT | amf::AMFACL_SPEAKER_FRONT_CENTER | amf::AMFACL_SPEAKER_FRONT_LEFT:
                str << "stereo 3.0";
                break;
            case amf::AMFACL_SPEAKER_FRONT_RIGHT | amf::AMFACL_SPEAKER_FRONT_CENTER | amf::AMFACL_SPEAKER_FRONT_LEFT | amf::AMFACL_SPEAKER_LOW_FREQUENCY:
                str << "stereo 3.1";
                break;
            case amf::AMFACL_SPEAKER_FRONT_RIGHT | amf::AMFACL_SPEAKER_FRONT_CENTER | amf::AMFACL_SPEAKER_FRONT_LEFT | amf::AMFACL_SPEAKER_BACK_LEFT | amf::AMFACL_SPEAKER_BACK_RIGHT:
                str << "surround 5.0";
                break;
            case amf::AMFACL_SPEAKER_FRONT_RIGHT | amf::AMFACL_SPEAKER_FRONT_CENTER | amf::AMFACL_SPEAKER_FRONT_LEFT | amf::AMFACL_SPEAKER_BACK_LEFT | amf::AMFACL_SPEAKER_BACK_RIGHT | amf::AMFACL_SPEAKER_LOW_FREQUENCY:
                str << "surround 5.1";
                break;
            default:
                str << channels << "-ch surround";
                break;
            }
            m_Description = str.str();
        }
    }

    AudioBuffer::AudioBuffer(amf::AMFBuffer* buffer, int64_t sequenceNumber, bool discontinuity) :
        m_Buf(buffer),
        m_SequenceNumber(sequenceNumber),
        m_Discontinuity(discontinuity)
    {
    }

    AMF_RESULT AudioBuffer::GetBuffer(amf::AMFBuffer** buf)
    {
        AMF_RETURN_IF_FALSE(buf != nullptr, AMF_INVALID_ARG, L"AudioBuffer::GetBuffer(buf): buf should not be NULL");
        *buf = m_Buf;
        (*buf)->Acquire();
        return AMF_OK;
    }

    amf_pts AudioBuffer::GetPts() const noexcept
    {
        return m_Buf != nullptr ? m_Buf->GetPts() : -1;
    }

    amf_pts AudioBuffer::GetDuration() const noexcept
    {
        return m_Buf != nullptr ? m_Buf->GetDuration() : -1;
    }

    TransmittableAudioBuffer::TransmittableAudioBuffer(amf::AMFBuffer* buffer, int64_t sequenceNumber, bool discontinuity) :
        AudioBuffer(buffer, sequenceNumber, discontinuity)
    {
    }

    ReceivableAudioBuffer::ReceivableAudioBuffer(amf::AMFBuffer* buffer, int64_t sequenceNumber, bool discontinuity) :
        AudioBuffer(buffer, sequenceNumber, discontinuity)
    {
    }
}