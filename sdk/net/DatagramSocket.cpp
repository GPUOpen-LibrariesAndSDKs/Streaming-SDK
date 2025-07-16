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
#include "DatagramSocket.h"
#include "amf/public/common/TraceAdapter.h"
#include <sstream>

#if defined(WIN32)
#include <iphlpapi.h>
#include <ws2tcpip.h>
#elif defined(__linux__) || defined(__android__) || defined (__APPLE__)
#include <sys/types.h>
#include <ifaddrs.h>
#endif

#if defined(__APPLE__)
#include <net/if.h>
#endif


static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::net::DatagramSocket";

#define NIC_LIST_UPDATE_INTERVAL    1   //  In seconds. A call to query all network adapters on Windows is relatively slow - up to 25ms on slower CPUs like Carrizo
                                        //

//#define TRACE_ME

#if defined(__linux__) || defined(__android__) //|| defined (__APPLE__)
#if defined(__linux__) ||  __ANDROID_API__ <= 22

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#if defined(__linux__)
#include <linux/netdevice.h>
#endif

typedef struct NetlinkList
{
    struct NetlinkList *m_next;
    struct nlmsghdr *m_data;
    unsigned int m_size;
} NetlinkList;

static int netlink_socket(void)
{
    int l_socket = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if(l_socket < 0)
    {
        return -1;
    }

    struct sockaddr_nl l_addr;
    memset(&l_addr, 0, sizeof(l_addr));
    l_addr.nl_family = AF_NETLINK;
    if(bind(l_socket, (struct sockaddr *)&l_addr, sizeof(l_addr)) < 0)
    {
        close(l_socket);
        return -1;
    }

    return l_socket;
}

static int netlink_send(int p_socket, int p_request)
{
    char l_buffer[NLMSG_ALIGN(sizeof(struct nlmsghdr)) + NLMSG_ALIGN(sizeof(struct rtgenmsg))];
    memset(l_buffer, 0, sizeof(l_buffer));
    struct nlmsghdr *l_hdr = (struct nlmsghdr *)l_buffer;
    struct rtgenmsg *l_msg = (struct rtgenmsg *)NLMSG_DATA(l_hdr);

    l_hdr->nlmsg_len = NLMSG_LENGTH(sizeof(*l_msg));
    l_hdr->nlmsg_type = p_request;
    l_hdr->nlmsg_flags = NLM_F_ROOT | NLM_F_MATCH | NLM_F_REQUEST;
    l_hdr->nlmsg_pid = 0;
    l_hdr->nlmsg_seq = p_socket;
    l_msg->rtgen_family = AF_UNSPEC;

    struct sockaddr_nl l_addr;
    memset(&l_addr, 0, sizeof(l_addr));
    l_addr.nl_family = AF_NETLINK;
    return (sendto(p_socket, l_hdr, l_hdr->nlmsg_len, 0, (struct sockaddr *)&l_addr, sizeof(l_addr)));
}

static int netlink_recv(int p_socket, void *p_buffer, size_t p_len)
{
    struct msghdr l_msg;
    struct iovec l_iov = { p_buffer, p_len };
    struct sockaddr_nl l_addr;
    int l_result;

    for(;;)
    {
        l_msg.msg_name = (void *)&l_addr;
        l_msg.msg_namelen = sizeof(l_addr);
        l_msg.msg_iov = &l_iov;
        l_msg.msg_iovlen = 1;
        l_msg.msg_control = NULL;
        l_msg.msg_controllen = 0;
        l_msg.msg_flags = 0;
        int l_result = recvmsg(p_socket, &l_msg, 0);

        if(l_result < 0)
        {
            if(errno == EINTR)
            {
                continue;
            }
            return -2;
        }

        if(l_msg.msg_flags & MSG_TRUNC)
        { // buffer was too small
            return -1;
        }
        return l_result;
    }
}

static struct nlmsghdr *getNetlinkResponse(int p_socket, int *p_size, int *p_done)
{
    size_t l_size = 4096;
    void *l_buffer = NULL;

    for(;;)
    {
        free(l_buffer);
        l_buffer = malloc(l_size);

        int l_read = netlink_recv(p_socket, l_buffer, l_size);
        *p_size = l_read;
        if(l_read == -2)
        {
            free(l_buffer);
            return NULL;
        }
        if(l_read >= 0)
        {
            pid_t l_pid = getpid();
            struct nlmsghdr *l_hdr;
            for(l_hdr = (struct nlmsghdr *)l_buffer; NLMSG_OK(l_hdr, (unsigned int)l_read); l_hdr = (struct nlmsghdr *)NLMSG_NEXT(l_hdr, l_read))
            {
                if((pid_t)l_hdr->nlmsg_pid != l_pid || (int)l_hdr->nlmsg_seq != p_socket)
                {
                    continue;
                }

                if(l_hdr->nlmsg_type == NLMSG_DONE)
                {
                    *p_done = 1;
                    break;
                }

                if(l_hdr->nlmsg_type == NLMSG_ERROR)
                {
                    free(l_buffer);
                    return NULL;
                }
            }
            return (nlmsghdr *)l_buffer;
        }

        l_size *= 2;
    }
}

static NetlinkList *newListItem(struct nlmsghdr *p_data, unsigned int p_size)
{
    NetlinkList *l_item = (NetlinkList *)malloc(sizeof(NetlinkList));
    l_item->m_next = NULL;
    l_item->m_data = p_data;
    l_item->m_size = p_size;
    return l_item;
}

static void freeResultList(NetlinkList *p_list)
{
    NetlinkList *l_cur;
    while(p_list)
    {
        l_cur = p_list;
        p_list = p_list->m_next;
        free(l_cur->m_data);
        free(l_cur);
    }
}

static NetlinkList *getResultList(int p_socket, int p_request)
{
    if(netlink_send(p_socket, p_request) < 0)
    {
        return NULL;
    }

    NetlinkList *l_list = NULL;
    NetlinkList *l_end = NULL;
    int l_size;
    int l_done = 0;
    while(!l_done)
    {
        struct nlmsghdr *l_hdr = getNetlinkResponse(p_socket, &l_size, &l_done);
        if(!l_hdr)
        { // error
            freeResultList(l_list);
            return NULL;
        }

        NetlinkList *l_item = newListItem(l_hdr, l_size);
        if(!l_list)
        {
            l_list = l_item;
        }
        else
        {
            l_end->m_next = l_item;
        }
        l_end = l_item;
    }
    return l_list;
}

static size_t maxSize(size_t a, size_t b)
{
    return (a > b ? a : b);
}

static size_t calcAddrLen(sa_family_t p_family, int p_dataSize)
{
    switch(p_family)
    {
        case AF_INET:
            return sizeof(struct sockaddr_in);
        case AF_INET6:
            return sizeof(struct sockaddr_in6);
        case AF_PACKET:
            return maxSize(sizeof(struct sockaddr_ll), offsetof(struct sockaddr_ll, sll_addr) + p_dataSize);
        default:
            return maxSize(sizeof(struct sockaddr), offsetof(struct sockaddr, sa_data) + p_dataSize);
    }
}

static void makeSockaddr(sa_family_t p_family, struct sockaddr *p_dest, void *p_data, size_t p_size)
{
    switch(p_family)
    {
        case AF_INET:
            memcpy(&((struct sockaddr_in*)p_dest)->sin_addr, p_data, p_size);
            break;
        case AF_INET6:
            memcpy(&((struct sockaddr_in6*)p_dest)->sin6_addr, p_data, p_size);
            break;
        case AF_PACKET:
            memcpy(((struct sockaddr_ll*)p_dest)->sll_addr, p_data, p_size);
            ((struct sockaddr_ll*)p_dest)->sll_halen = p_size;
            break;
        default:
            memcpy(p_dest->sa_data, p_data, p_size);
            break;
    }
    p_dest->sa_family = p_family;
}

static void addToEnd(struct ifaddrs **p_resultList, struct ifaddrs *p_entry)
{
    if(!*p_resultList)
    {
        *p_resultList = p_entry;
    }
    else
    {
        struct ifaddrs *l_cur = *p_resultList;
        while(l_cur->ifa_next)
        {
            l_cur = l_cur->ifa_next;
        }
        l_cur->ifa_next = p_entry;
    }
}

static void interpretLink(struct nlmsghdr *p_hdr, struct ifaddrs **p_links, struct ifaddrs **p_resultList)
{
    struct ifinfomsg *l_info = (struct ifinfomsg *)NLMSG_DATA(p_hdr);

    size_t l_nameSize = 0;
    size_t l_addrSize = 0;
    size_t l_dataSize = 0;

    size_t l_rtaSize = NLMSG_PAYLOAD(p_hdr, sizeof(struct ifinfomsg));
    struct rtattr *l_rta;
    for(l_rta = (struct rtattr *)(((char *)l_info) + NLMSG_ALIGN(sizeof(struct ifinfomsg))); RTA_OK(l_rta, l_rtaSize); l_rta = RTA_NEXT(l_rta, l_rtaSize))
    {
        void *l_rtaData = RTA_DATA(l_rta);
        size_t l_rtaDataSize = RTA_PAYLOAD(l_rta);
        switch(l_rta->rta_type)
        {
            case IFLA_ADDRESS:
            case IFLA_BROADCAST:
                l_addrSize += NLMSG_ALIGN(calcAddrLen(AF_PACKET, l_rtaDataSize));
                break;
            case IFLA_IFNAME:
                l_nameSize += NLMSG_ALIGN(l_rtaSize + 1);
                break;
            case IFLA_STATS:
                l_dataSize += NLMSG_ALIGN(l_rtaSize);
                break;
            default:
                break;
        }
    }

    struct ifaddrs *l_entry = (struct ifaddrs *)malloc(sizeof(struct ifaddrs) + l_nameSize + l_addrSize + l_dataSize);
    memset(l_entry, 0, sizeof(struct ifaddrs));
    l_entry->ifa_name = const_cast<char*>(""); //todo: this is bad!

    char *l_name = ((char *)l_entry) + sizeof(struct ifaddrs);
    char *l_addr = l_name + l_nameSize;
    char *l_data = l_addr + l_addrSize;

    l_entry->ifa_flags = l_info->ifi_flags;

    l_rtaSize = NLMSG_PAYLOAD(p_hdr, sizeof(struct ifinfomsg));
    for(l_rta = (struct rtattr *)(((char *)l_info) + NLMSG_ALIGN(sizeof(struct ifinfomsg))); RTA_OK(l_rta, l_rtaSize); l_rta = RTA_NEXT(l_rta, l_rtaSize))
    {
        void *l_rtaData = RTA_DATA(l_rta);
        size_t l_rtaDataSize = RTA_PAYLOAD(l_rta);
        switch(l_rta->rta_type)
        {
            case IFLA_ADDRESS:
            case IFLA_BROADCAST:
            {
                size_t l_addrLen = calcAddrLen(AF_PACKET, l_rtaDataSize);
                makeSockaddr(AF_PACKET, (struct sockaddr *)l_addr, l_rtaData, l_rtaDataSize);
                ((struct sockaddr_ll *)l_addr)->sll_ifindex = l_info->ifi_index;
                ((struct sockaddr_ll *)l_addr)->sll_hatype = l_info->ifi_type;
                if(l_rta->rta_type == IFLA_ADDRESS)
                {
                    l_entry->ifa_addr = (struct sockaddr *)l_addr;
                }
                else
                {
                    l_entry->ifa_broadaddr = (struct sockaddr *)l_addr;
                }
                l_addr += NLMSG_ALIGN(l_addrLen);
                break;
            }
            case IFLA_IFNAME:
                strncpy(l_name, (char*)l_rtaData, l_rtaDataSize);
                l_name[l_rtaDataSize] = '\0';
                l_entry->ifa_name = l_name;
                break;
            case IFLA_STATS:
                memcpy(l_data, l_rtaData, l_rtaDataSize);
                l_entry->ifa_data = l_data;
                break;
            default:
                break;
        }
    }

    addToEnd(p_resultList, l_entry);
    p_links[l_info->ifi_index - 1] = l_entry;
}

static void interpretAddr(struct nlmsghdr *p_hdr, struct ifaddrs **p_links, struct ifaddrs **p_resultList)
{
    struct ifaddrmsg *l_info = (struct ifaddrmsg *)NLMSG_DATA(p_hdr);

    size_t l_nameSize = 0;
    size_t l_addrSize = 0;

    int l_addedNetmask = 0;

    size_t l_rtaSize = NLMSG_PAYLOAD(p_hdr, sizeof(struct ifaddrmsg));
    struct rtattr *l_rta;
    for(l_rta = (struct rtattr *)(((char *)l_info) + NLMSG_ALIGN(sizeof(struct ifaddrmsg))); RTA_OK(l_rta, l_rtaSize); l_rta = RTA_NEXT(l_rta, l_rtaSize))
    {
        void *l_rtaData = RTA_DATA(l_rta);
        size_t l_rtaDataSize = RTA_PAYLOAD(l_rta);
        if(l_info->ifa_family == AF_PACKET)
        {
            continue;
        }

        switch(l_rta->rta_type)
        {
            case IFA_ADDRESS:
            case IFA_LOCAL:
                if((l_info->ifa_family == AF_INET || l_info->ifa_family == AF_INET6) && !l_addedNetmask)
                { // make room for netmask
                    l_addrSize += NLMSG_ALIGN(calcAddrLen(l_info->ifa_family, l_rtaDataSize));
                    l_addedNetmask = 1;
                }
                break;
            case IFA_BROADCAST:
                l_addrSize += NLMSG_ALIGN(calcAddrLen(l_info->ifa_family, l_rtaDataSize));
                break;
            case IFA_LABEL:
                l_nameSize += NLMSG_ALIGN(l_rtaSize + 1);
                break;
            default:
                break;
        }
    }

    struct ifaddrs *l_entry = (struct ifaddrs *)malloc(sizeof(struct ifaddrs) + l_nameSize + l_addrSize);
    memset(l_entry, 0, sizeof(struct ifaddrs));
    l_entry->ifa_name = p_links[l_info->ifa_index - 1]->ifa_name;

    char *l_name = ((char *)l_entry) + sizeof(struct ifaddrs);
    char *l_addr = l_name + l_nameSize;

    l_entry->ifa_flags = l_info->ifa_flags | p_links[l_info->ifa_index - 1]->ifa_flags;

    l_rtaSize = NLMSG_PAYLOAD(p_hdr, sizeof(struct ifaddrmsg));
    for(l_rta = (struct rtattr *)(((char *)l_info) + NLMSG_ALIGN(sizeof(struct ifaddrmsg))); RTA_OK(l_rta, l_rtaSize); l_rta = RTA_NEXT(l_rta, l_rtaSize))
    {
        void *l_rtaData = RTA_DATA(l_rta);
        size_t l_rtaDataSize = RTA_PAYLOAD(l_rta);
        switch(l_rta->rta_type)
        {
            case IFA_ADDRESS:
            case IFA_BROADCAST:
            case IFA_LOCAL:
            {
                size_t l_addrLen = calcAddrLen(l_info->ifa_family, l_rtaDataSize);
                makeSockaddr(l_info->ifa_family, (struct sockaddr *)l_addr, l_rtaData, l_rtaDataSize);
                if(l_info->ifa_family == AF_INET6)
                {
                    if(IN6_IS_ADDR_LINKLOCAL((struct in6_addr *)l_rtaData) || IN6_IS_ADDR_MC_LINKLOCAL((struct in6_addr *)l_rtaData))
                    {
                        ((struct sockaddr_in6 *)l_addr)->sin6_scope_id = l_info->ifa_index;
                    }
                }

                if(l_rta->rta_type == IFA_ADDRESS)
                { // apparently in a point-to-point network IFA_ADDRESS contains the dest address and IFA_LOCAL contains the local address
                    if(l_entry->ifa_addr)
                    {
                        l_entry->ifa_dstaddr = (struct sockaddr *)l_addr;
                    }
                    else
                    {
                        l_entry->ifa_addr = (struct sockaddr *)l_addr;
                    }
                }
                else if(l_rta->rta_type == IFA_LOCAL)
                {
                    if(l_entry->ifa_addr)
                    {
                        l_entry->ifa_dstaddr = l_entry->ifa_addr;
                    }
                    l_entry->ifa_addr = (struct sockaddr *)l_addr;
                }
                else
                {
                    l_entry->ifa_broadaddr = (struct sockaddr *)l_addr;
                }
                l_addr += NLMSG_ALIGN(l_addrLen);
                break;
            }
            case IFA_LABEL:
                strncpy(l_name, (char*)l_rtaData, l_rtaDataSize);
                l_name[l_rtaDataSize] = '\0';
                l_entry->ifa_name = l_name;
                break;
            default:
                break;
        }
    }

    if(l_entry->ifa_addr && (l_entry->ifa_addr->sa_family == AF_INET || l_entry->ifa_addr->sa_family == AF_INET6))
    {
        unsigned l_maxPrefix = (l_entry->ifa_addr->sa_family == AF_INET ? 32 : 128);
        unsigned l_prefix = (l_info->ifa_prefixlen > l_maxPrefix ? l_maxPrefix : l_info->ifa_prefixlen);
        char l_mask[16] = {0};
        unsigned i;
        for(i=0; i<(l_prefix/8); ++i)
        {
            l_mask[i] = 0xff;
        }
        l_mask[i] = 0xff << (8 - (l_prefix % 8));

        makeSockaddr(l_entry->ifa_addr->sa_family, (struct sockaddr *)l_addr, l_mask, l_maxPrefix / 8);
        l_entry->ifa_netmask = (struct sockaddr *)l_addr;
    }

    addToEnd(p_resultList, l_entry);
}

static void interpret(int p_socket, NetlinkList *p_netlinkList, struct ifaddrs **p_links, struct ifaddrs **p_resultList)
{
    pid_t l_pid = getpid();
    for(; p_netlinkList; p_netlinkList = p_netlinkList->m_next)
    {
        unsigned int l_nlsize = p_netlinkList->m_size;
        struct nlmsghdr *l_hdr;
        for(l_hdr = p_netlinkList->m_data; NLMSG_OK(l_hdr, l_nlsize); l_hdr = NLMSG_NEXT(l_hdr, l_nlsize))
        {
            if((pid_t)l_hdr->nlmsg_pid != l_pid || (int)l_hdr->nlmsg_seq != p_socket)
            {
                continue;
            }

            if(l_hdr->nlmsg_type == NLMSG_DONE)
            {
                break;
            }

            if(l_hdr->nlmsg_type == RTM_NEWLINK)
            {
                interpretLink(l_hdr, p_links, p_resultList);
            }
            else if(l_hdr->nlmsg_type == RTM_NEWADDR)
            {
                interpretAddr(l_hdr, p_links, p_resultList);
            }
        }
    }
}

static unsigned countLinks(int p_socket, NetlinkList *p_netlinkList)
{
    unsigned l_links = 0;
    pid_t l_pid = getpid();
    for(; p_netlinkList; p_netlinkList = p_netlinkList->m_next)
    {
        unsigned int l_nlsize = p_netlinkList->m_size;
        struct nlmsghdr *l_hdr;
        for(l_hdr = p_netlinkList->m_data; NLMSG_OK(l_hdr, l_nlsize); l_hdr = NLMSG_NEXT(l_hdr, l_nlsize))
        {
            if((pid_t)l_hdr->nlmsg_pid != l_pid || (int)l_hdr->nlmsg_seq != p_socket)
            {
                continue;
            }

            if(l_hdr->nlmsg_type == NLMSG_DONE)
            {
                break;
            }

            if(l_hdr->nlmsg_type == RTM_NEWLINK)
            {
                ++l_links;
            }
        }
    }

    return l_links;
}

#if defined(__android__)
static int getifaddrs(struct ifaddrs **ifap)
{
    if(!ifap)
    {
        return -1;
    }
    *ifap = NULL;

    int l_socket = netlink_socket();
    if(l_socket < 0)
    {
        return -1;
    }

    NetlinkList *l_linkResults = getResultList(l_socket, RTM_GETLINK);
    if(!l_linkResults)
    {
        close(l_socket);
        return -1;
    }

    NetlinkList *l_addrResults = getResultList(l_socket, RTM_GETADDR);
    if(!l_addrResults)
    {
        close(l_socket);
        freeResultList(l_linkResults);
        return -1;
    }

    unsigned l_numLinks = countLinks(l_socket, l_linkResults) + countLinks(l_socket, l_addrResults);
    struct ifaddrs *l_links[l_numLinks];
    memset(l_links, 0, l_numLinks * sizeof(struct ifaddrs *));

    interpret(l_socket, l_linkResults, l_links, ifap);
    interpret(l_socket, l_addrResults, l_links, ifap);

    freeResultList(l_linkResults);
    freeResultList(l_addrResults);
    close(l_socket);
    return 0;
}

static void freeifaddrs(struct ifaddrs *ifa)
{
    struct ifaddrs *l_cur;
    while(ifa)
    {
        l_cur = ifa;
        ifa = ifa->ifa_next;
        free(l_cur);
    }
}
#endif

#endif
#endif

namespace ssdk::net
{
    DatagramSocket::AddressVector   DatagramSocket::m_MyNICs;
    time_t DatagramSocket::m_NICsUpdatedTime = 0;
    time_t DatagramSocket::m_NICsUpdateInterval = NIC_LIST_UPDATE_INTERVAL;
    amf::AMFCriticalSection DatagramSocket::m_Guard;


    DatagramSocket::DatagramSocket(AddressFamily addrFamily, Protocol protocol) :
        Socket(addrFamily, Socket::Type::TYPE_DATAGRAM, protocol)
    {
//        EnumerateNICs();
    }

    DatagramSocket::Result DatagramSocket::SendTo(const void* buf, size_t size, const Socket::Address& to, size_t* bytesSent, int flags)
    {
        DatagramSocket::Result result = DatagramSocket::Result::OK;
        int sentCount = 0;

        if (m_Socket == INVALID_SOCKET)
        {
            result = Socket::Result::SOCKET_NOT_OPEN;
            AMFTraceError(AMF_FACILITY, L"SendTo() err=%s", GetErrorString(result));
        }
        else if (buf == nullptr)
        {
            result = Socket::Result::INVALID_ARG;
            AMFTraceError(AMF_FACILITY, L"SendTo() buffer is NULL err=%s", GetErrorString(result));
        }
        else if (size == 0)
        {
            result = Socket::Result::INVALID_ARG;
            AMFTraceError(AMF_FACILITY, L"SendTo() buffer size is 0 err=%s", GetErrorString(result));
        }
        else if (to.GetAddressFamily() != m_AddrFamily)
        {
            result = Socket::Result::INVALID_ARG;
            AMFTraceError(AMF_FACILITY, L"SendTo() invalid address family err=%s", GetErrorString(result));
        }
        else if ((sentCount = ::sendto(m_Socket, reinterpret_cast<const char*>(buf), (int)size, flags, &to.ToSockAddr(), (int)to.GetSize())) <= 0)
        {
            result = GetError(GetSocketOSError());
            AMFTraceError(AMF_FACILITY, L"SendTo() sendto() failed err=%s", GetErrorString(result));
        }
        else if (bytesSent != nullptr)
        {
            //AMFTraceDebug(AMF_FACILITY, L"DatagramSocket::SendTo() bytesSent=%d", sentCount);
            *bytesSent = sentCount;
        }
        return result;
    }

    DatagramSocket::Result DatagramSocket::ReceiveFrom(void* buf, size_t size, Socket::Address* from, size_t* bytesReceived, int flags)
    {
        DatagramSocket::Result result = DatagramSocket::Result::OK;
        int receivedCount = 0;

        if (m_Socket == INVALID_SOCKET)
        {
            result = Socket::Result::SOCKET_NOT_OPEN;
            AMFTraceError(AMF_FACILITY, L"ReceiveFrom() err=%s", GetErrorString(result));
        }
        else if (buf == nullptr)
        {
            result = Socket::Result::INVALID_ARG;
            AMFTraceError(AMF_FACILITY, L"ReceiveFrom() buffer is NULL err=%s", GetErrorString(result));
        }
        else if (size == 0)
        {
            result = Socket::Result::INVALID_ARG;
            AMFTraceError(AMF_FACILITY, L"ReceiveFrom() buffer size is 0 err=%s", GetErrorString(result));
        }
        else
        {
            struct sockaddr* fromAddr = nullptr;
            socklen_t addrSize = 0;
            if (from != nullptr)
            {
                fromAddr = &(from->ToSockAddr());
                addrSize = (int)from->GetSize();
            }
            if ((receivedCount = ::recvfrom(m_Socket, reinterpret_cast<char*>(buf), (int)size, flags, fromAddr, &addrSize)) <= 0)
            {
                int errcode = GetSocketOSError();
                result = GetError(errcode);
                AMFTraceError(AMF_FACILITY, L"ReceiveFrom() recvfrom() failed: err=%s socketerr=%d", GetErrorString(result), errcode);
            }
            else if (bytesReceived != nullptr)
            {
                //AMFTraceDebug(AMF_FACILITY, L"DatagramSocket::ReceiveFrom() receivedCount=%d", receivedCount);
                *bytesReceived = receivedCount;
            }
        }
        return result;
    }

    void DatagramSocket::SetNICDataExpiration(time_t expirationSec)
    {
        amf::AMFLock    lock(&m_Guard);
        m_NICsUpdatedTime = expirationSec;
    }

    bool DatagramSocket::EnumerateNICs()
    {
        bool result = true;
        time_t now = time(nullptr);
        amf::AMFLock    lock(&m_Guard);
        if (now - m_NICsUpdatedTime >= NIC_LIST_UPDATE_INTERVAL)
        {
            m_NICsUpdatedTime = now;
            m_MyNICs.clear();
#if defined(WIN32)
            DWORD rv, size = 100 * 1024;    //  100k buffer by default, should be enough for absolute majority of cases
            PIP_ADAPTER_ADDRESSES adapterAddresses;
            ULONG family = AF_INET;
            ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_INCLUDE_ALL_INTERFACES;
            bool retry = false;
            std::unique_ptr<uint8_t[]> buf;
            do
            {
                buf.reset(new uint8_t[size]);
                adapterAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buf.get());
                rv = GetAdaptersAddresses(family, flags, nullptr, adapterAddresses, &size);
                switch (rv)
                {
                case ERROR_BUFFER_OVERFLOW:
                    retry = true;
                    result = false;
                    break;
                case ERROR_SUCCESS:
                    result = true;
                    retry = false;
                    break;
                default:
                    AMFTraceError(AMF_FACILITY, L"GetAdaptersAddresses() failed, error code %d", rv);
                    result = false;
                    retry = false;
                    break;
                }
            } while (retry == true);

            if (result == true)
            {
                for (PIP_ADAPTER_ADDRESSES aa = adapterAddresses; aa != nullptr; aa = aa->Next)
                {
                    if (aa->OperStatus == IfOperStatusUp && aa->FirstUnicastAddress != nullptr)
                    {
                        for (PIP_ADAPTER_UNICAST_ADDRESS multicastAddr = aa->FirstUnicastAddress; multicastAddr != nullptr; multicastAddr = multicastAddr->Next)
                        {
                            unsigned long mask;
                            ConvertLengthToIpv4Mask(multicastAddr->OnLinkPrefixLength, &mask);
                            if (mask != 0xFFFFFFFF)
                            {
                                *(reinterpret_cast<unsigned long*>(&reinterpret_cast<sockaddr_in*>(multicastAddr->Address.lpSockaddr)->sin_addr)) |= (~mask);

                                Socket::Address address(*multicastAddr->Address.lpSockaddr);
                                m_MyNICs.push_back(address);
                                std::string addressStr = address.GetParsedAddressAsString();
                                AMFTraceDebug(AMF_FACILITY, L"Added broadcast interface %S, address %S", aa->AdapterName, addressStr.c_str());
                            }
                        }
                    }
                }
            }

#elif defined(__linux__) || defined(__android__) || defined (__APPLE__)
            struct ifaddrs *addrs, *tmp;

            getifaddrs(&addrs);
            tmp = addrs;

            while (tmp)
            {
#if !defined (__APPLE__)
                if (tmp->ifa_addr != nullptr && tmp->ifa_addr->sa_family == AF_INET && tmp->ifa_ifu.ifu_broadaddr != nullptr)
#else
                if (tmp->ifa_addr != nullptr && tmp->ifa_addr->sa_family == AF_INET && (tmp->ifa_flags & IFF_BROADCAST) && tmp->ifa_dstaddr != nullptr)
#endif
                {
#if !defined (__APPLE__)
                    Socket::Address address(*tmp->ifa_ifu.ifu_broadaddr);
#else
                    Socket::Address address(*tmp->ifa_dstaddr);
#endif

                    m_MyNICs.push_back(address);
                    AMFTraceDebug(AMF_FACILITY, L"Added broadcast interface %S, address %S", tmp->ifa_name, address.GetParsedAddressAsString().c_str());
                }
                tmp = tmp->ifa_next;
            }

            freeifaddrs(addrs);

#endif


        }
        return result;
    }

    DatagramSocket::Result DatagramSocket::Broadcast(const void* buf, size_t size, unsigned short port, size_t* bytesSent, int flags)
    {
        DatagramSocket::Result result = DatagramSocket::Result::OK;
        AddressVector myNICs;

        EnumerateNICs();
        {
            amf::AMFLock    lock(&m_Guard);
            myNICs = m_MyNICs;
        }
        for (AddressVector::const_iterator it = myNICs.begin(); it != myNICs.end(); ++it)
        {
            Socket::IPv4Address address(*it);
            address.SetIPPort(port);
            result = SendTo(buf, size, address, bytesSent, flags);
        }

        return result;
    }
}
