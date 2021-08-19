/*
    This file is part of mDNS Reflector (mdns-reflector), a lightweight and performant multicast DNS (mDNS) reflector.
    Copyright (C) 2021 Yuxiang Zhu <me@yux.im>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "mcast.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>

int mcast_join(int fd, const struct sockaddr_storage *sa, socklen_t sa_len, uint32_t ifindex) {
#if defined(MCAST_JOIN_GROUP)
    int level;
    switch (sa->ss_family) {
        case AF_INET6:
            level = IPPROTO_IPV6;
            break;
        case AF_INET:
            level = IPPROTO_IP;
            break;
        default:
            errno = EAFNOSUPPORT;
            return -1;
    }
    struct group_req req;
    req.gr_interface = ifindex;
    memcpy(&req.gr_group, sa, sa_len);
#if defined(__unix__) && !defined(__linux__) || defined(__APPLE__)
    // At least FreeBSD and macOS requires ss_len, otherwise we'll get an `Invalid argument` error.
    req.gr_group.ss_len = (uint8_t) sa_len;
#endif
    if (setsockopt(fd, level, MCAST_JOIN_GROUP, &req, sizeof(struct group_req)) == -1)
        return -1;
#else
    (void) sa_len;
    switch (sa->ss_family) {
        case AF_INET6: {
            struct ipv6_mreq mreq6;
            mreq6.ipv6mr_interface = ifindex;
            memcpy(&mreq6.ipv6mr_multiaddr, &((struct sockaddr_in6 *) sa)->sin6_addr, sizeof(struct in6_addr));
            if (setsockopt(fd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq6, sizeof(mreq6)) == -1)
                return -1;
            break;
        }
        case AF_INET: {
            struct ip_mreq mreq4;
            struct ifreq ifreq;
            if (ifindex > 0) {
                if (if_indextoname(ifindex, ifreq.ifr_name) == NULL)
                    return -1;
                if (ioctl(fd, SIOCGIFADDR, &ifreq) == -1)
                    return -1;
                memcpy(&mreq4.imr_interface, &((struct sockaddr_in *) &ifreq.ifr_ifru.ifru_addr)->sin_addr,
                        sizeof(struct in_addr));
            } else {
                mreq4.imr_interface.s_addr = htonl(INADDR_ANY);
            }
            memcpy(&mreq4.imr_multiaddr, &((struct sockaddr_in *) sa)->sin_addr, sizeof(struct in_addr));
            if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq4, sizeof(mreq4)) == -1)
                return -1;
            break;
        }
        default:
            errno = EAFNOSUPPORT;
            return -1;
    }
#endif
    return 0;
}
