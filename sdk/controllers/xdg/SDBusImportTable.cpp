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

#if defined(__linux)

#include "SDBusImportTable.h"
#include "public/common/TraceAdapter.h"
#include "public/common/Thread.h"

using namespace amf;

#define GET_SO_ENTRYPOINT(m, h, f) m = reinterpret_cast<decltype(&f)>(amf_get_proc_address(h, #f)); \
    AMF_RETURN_IF_FALSE(nullptr != m, AMF_FAIL, L"Failed to acquire entrypoint %S", #f);

namespace ssdk::ctls
{
    //-------------------------------------------------------------------------------------------------
    SDBusImportTable::SDBusImportTable()
    {}

    //-------------------------------------------------------------------------------------------------
    SDBusImportTable::~SDBusImportTable()
    {
        UnloadFunctionsTable();
    }

    //-------------------------------------------------------------------------------------------------
    AMF_RESULT SDBusImportTable::LoadFunctionsTable()
    {
        if (nullptr == m_hLibSystemdSO)
        {
            m_hLibSystemdSO = amf_load_library(L"libsystemd.so.0");
            AMF_RETURN_IF_FALSE(nullptr != m_hLibSystemdSO, AMF_FAIL, L"Failed to load libsystemd.so.0");
        }

        GET_SO_ENTRYPOINT(m_pSDB_Open_System, m_hLibSystemdSO, sd_bus_open_system);
        GET_SO_ENTRYPOINT(m_pSDB_Open_User, m_hLibSystemdSO, sd_bus_open_user);

        GET_SO_ENTRYPOINT(m_pSDB_Process, m_hLibSystemdSO, sd_bus_process);
        GET_SO_ENTRYPOINT(m_pSDB_Wait, m_hLibSystemdSO, sd_bus_wait);

        GET_SO_ENTRYPOINT(m_pSDB_Get_Unique_Name, m_hLibSystemdSO, sd_bus_get_unique_name);

        GET_SO_ENTRYPOINT(m_pSDB_Message_New_Method_Call, m_hLibSystemdSO, sd_bus_message_new_method_call);

        GET_SO_ENTRYPOINT(m_pSDB_Message_Append, m_hLibSystemdSO, sd_bus_message_append);

        GET_SO_ENTRYPOINT(m_pSDB_Message_Read, m_hLibSystemdSO, sd_bus_message_read);
        GET_SO_ENTRYPOINT(m_pSDB_Message_Skip, m_hLibSystemdSO, sd_bus_message_skip);
        GET_SO_ENTRYPOINT(m_pSDB_Message_Peek_Type, m_hLibSystemdSO, sd_bus_message_peek_type);

        GET_SO_ENTRYPOINT(m_pSDB_Message_Open_Container, m_hLibSystemdSO, sd_bus_message_open_container);
        GET_SO_ENTRYPOINT(m_pSDB_Message_Close_Container, m_hLibSystemdSO, sd_bus_message_close_container);


        GET_SO_ENTRYPOINT(m_pSDB_Message_Enter_Container, m_hLibSystemdSO, sd_bus_message_enter_container);
        GET_SO_ENTRYPOINT(m_pSDB_Message_Exit_Container, m_hLibSystemdSO, sd_bus_message_exit_container);

        GET_SO_ENTRYPOINT(m_pSDB_Match_Signal, m_hLibSystemdSO, sd_bus_match_signal);

        GET_SO_ENTRYPOINT(m_pSDB_Call, m_hLibSystemdSO, sd_bus_call);
        GET_SO_ENTRYPOINT(m_pSDB_Get_Property_Trivial, m_hLibSystemdSO, sd_bus_get_property_trivial);

        GET_SO_ENTRYPOINT(m_pSDB_Ref, m_hLibSystemdSO, sd_bus_ref);
        GET_SO_ENTRYPOINT(m_pSDB_Message_Ref, m_hLibSystemdSO, sd_bus_message_ref);
        GET_SO_ENTRYPOINT(m_pSDB_Slot_Ref, m_hLibSystemdSO, sd_bus_slot_ref);

        GET_SO_ENTRYPOINT(m_pSDB_Unref, m_hLibSystemdSO, sd_bus_unref);
        GET_SO_ENTRYPOINT(m_pSDB_Message_Unref, m_hLibSystemdSO, sd_bus_message_unref);
        GET_SO_ENTRYPOINT(m_pSDB_Slot_Unref, m_hLibSystemdSO, sd_bus_slot_unref);

        return AMF_OK;
    }

    void SDBusImportTable::UnloadFunctionsTable()
    {
        if (nullptr != m_hLibSystemdSO)
        {
            amf_free_library(m_hLibSystemdSO);
            m_hLibSystemdSO = nullptr;

            // sets everything else to nullptr
            *this = SDBusImportTable();
        }

    }
    //-----------------------------------------------------------------------------------------

} // namespace ssdk::ctls

#endif // __linux
