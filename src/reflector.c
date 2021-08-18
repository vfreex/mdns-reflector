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

#include "reflector.h"
#include "logging.h"
#include "reflection_zone.h"
#include "options.h"
#include "mcast.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <signal.h>

#if defined(__linux__)

#include <sys/epoll.h>

#elif defined(__unix__) || defined(__APPLE__)
#include <sys/event.h>
#endif
#define MDNS_PORT 5353
#define MDNS_ADDR4 (u_int32_t)0xe00000fb  /* 224.0.0.251 */
#define MDNS_ADDR6_INIT \
{{{ 0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfb }}}

int new_recv_socket(const struct sockaddr_storage *sa, socklen_t sa_len, uint32_t ifindex) {
    const int ON = 1;
    int fd;
    switch (sa->ss_family) {
        case AF_INET6:
            fd = socket(AF_INET6, SOCK_DGRAM, 0);
            if (fd == -1) {
                log_err(LOG_ERR, "IPv6 socket");
                return -1;
            }
            if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &ON, sizeof(ON)) == -1) {
                log_err(LOG_ERR, "IPv6 setsockopt IPV6_V6ONLY");
                goto cleanup;
            }
            if (setsockopt(fd, IPPROTO_IPV6, IPV6_RECVPKTINFO, &ON, sizeof(ON)) == -1) {
                log_err(LOG_ERR, "IPv6 setsockopt IPV6_PKTINFO");
                goto cleanup;
            }
#if defined(SO_BINDTODEVICE)
            {
                struct ifreq ifr;
                if (if_indextoname(ifindex, ifr.ifr_ifrn.ifrn_name) == NULL) {
                    log_err(LOG_ERR, "if_indextoname");
                    goto cleanup;
                }
                if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) == -1) {
                    log_err(LOG_ERR, "setsockopt SO_BINDTODEVICE");
                    goto cleanup;
                }
            }
#elif defined(IPV6_BOUND_IF)
            if (setsockopt(fd, IPPROTO_IPV6, IPV6_BOUND_IF, &ifindex, sizeof(ifindex)) == -1) {
                log_err(LOG_ERR, "IPv6 setsockopt IPV6_BOUND_IF");
                goto cleanup;
            }
#endif
            break;
        case AF_INET:
            fd = socket(AF_INET, SOCK_DGRAM, 0);
            if (fd == -1) {
                log_err(LOG_ERR, "IPv4 socket");
                return -1;
            }
            if (setsockopt(fd, IPPROTO_IP, IP_PKTINFO, &ON, sizeof(ON)) == -1) {
                log_err(LOG_ERR, "IPv4 setsockopt IP_PKTINFO");
                goto cleanup;
            }
#if defined(SO_BINDTODEVICE)
            {
                struct ifreq ifr;
                if (if_indextoname(ifindex, ifr.ifr_ifrn.ifrn_name) == NULL) {
                    log_err(LOG_ERR, "if_indextoname");
                    goto cleanup;
                }
                if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) == -1) {
                    log_err(LOG_ERR, "setsockopt SO_BINDTODEVICE");
                    goto cleanup;
                }
            }
#elif defined(IP_BOUND_IF)
            if (setsockopt(fd, IPPROTO_IP, IP_BOUND_IF, &ifindex, sizeof(ifindex)) == -1) {
                log_err(LOG_ERR, "IPv4 setsockopt IP_BOUND_IF");
                goto cleanup;
            }
#endif
            break;
        default:
            errno = EAFNOSUPPORT;
            return -1;
    }
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &ON, sizeof(ON)) == -1) {
        log_err(LOG_ERR, "setsockopt SO_REUSEADDR");
        goto cleanup;
    }
#if defined(SO_REUSEPORT) && !defined(__linux__)
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &ON, sizeof(ON)) == -1) {
        log_err(LOG_ERR, "setsockopt SO_REUSEPORT");
        goto cleanup;
    }
#endif
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        log_err(LOG_ERR, "fcntl F_GETFL");
        goto cleanup;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        log_err(LOG_ERR, "fcntl F_SETFL");
        goto cleanup;
    }
    if (bind(fd, (struct sockaddr *) sa, sa_len) == -1) {
        log_err(LOG_ERR, "bind");
        goto cleanup;
    }
    return fd;
    cleanup:
    close(fd);
    return -1;
}

int new_send_socket(const struct sockaddr_storage *sa, socklen_t sa_len, uint32_t ifindex) {
    const int ON = 1;
    const int OFF = 0;
    int fd;
    switch (sa->ss_family) {
        case AF_INET6:
            fd = socket(AF_INET6, SOCK_DGRAM, 0);
            if (fd == -1) {
                log_err(LOG_ERR, "IPv6 socket");
                return -1;
            }
            if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &ON, sizeof(ON)) == -1) {
                log_err(LOG_ERR, "setsockopt IPV6_V6ONLY");
                goto cleanup;
            }
            if (setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex)) == -1) {
                log_err(LOG_ERR, "setsockopt IPV6_MULTICAST_IF");
                goto cleanup;
            }
            if (setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &OFF, sizeof(ON)) == -1) {
                log_err(LOG_ERR, "setsockopt IPV6_MULTICAST_LOOP");
                goto cleanup;
            }
            break;
        case AF_INET:
            fd = socket(AF_INET, SOCK_DGRAM, 0);
            if (fd == -1) {
                log_err(LOG_ERR, "IPv4 socket");
                return -1;
            }
            {
                struct ifreq ifreq;
                if (if_indextoname(ifindex, ifreq.ifr_name) == NULL) {
                    goto cleanup;
                }
                struct sockaddr_in *src_addr4 = (struct sockaddr_in *) &ifreq.ifr_addr;
                if (ioctl(fd, SIOCGIFADDR, &ifreq) == -1) {
                    goto cleanup;
                }
                if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &src_addr4->sin_addr, sizeof(src_addr4->sin_addr)) ==
                    -1) {
                    log_err(LOG_ERR, "setsockopt IP_MULTICAST_IF");
                    goto cleanup;
                }
            }
            if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &OFF, sizeof(ON)) == -1) {
                log_err(LOG_ERR, "setsockopt IP_MULTICAST_LOOP");
                goto cleanup;
            }
            break;
        default:
            errno = EAFNOSUPPORT;
            return -1;
    }
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &ON, sizeof(ON)) == -1) {
        log_err(LOG_ERR, "setsockopt SO_REUSEADDR");
        goto cleanup;
    }
#if defined(SO_REUSEPORT) && !defined(__linux__)
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &ON, sizeof(ON)) == -1) {
        log_err(LOG_ERR, "setsockopt SO_REUSEPORT");
        goto cleanup;
    }
#endif
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        log_err(LOG_ERR, "fcntl F_GETFL");
        goto cleanup;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        log_err(LOG_ERR, "fcntl F_SETFL");
        goto cleanup;
    }
    if (bind(fd, (struct sockaddr *) sa, sa_len) == -1) {
        log_err(LOG_ERR, "bind");
        goto cleanup;
    }
    return fd;
    cleanup:
    close(fd);
    return -1;
}

static const char *sockaddr_storage_to_string(const struct sockaddr_storage *sa) {
    static __thread char buffer[INET6_ADDRSTRLEN + 2 + 1 + 5 + 1 + 10];
    uint16_t port;
    if (sa->ss_family == AF_INET6) {
        struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *) sa;
        char addr_buf[INET6_ADDRSTRLEN];
        if (inet_ntop(AF_INET6, &sa6->sin6_addr, addr_buf, sizeof(addr_buf)) == NULL) {
            return NULL;
        }
        port = ntohs(sa6->sin6_port);
        if (IN6_IS_ADDR_LINKLOCAL(&sa6->sin6_addr) || IN6_IS_ADDR_MC_LINKLOCAL(&sa6->sin6_addr)) {
            snprintf(buffer, sizeof(buffer), "[%s%%%u]:%u", addr_buf, sa6->sin6_scope_id, port);
        } else {
            snprintf(buffer, sizeof(buffer), "[%s]:%u", addr_buf, port);
        }
    } else if (sa->ss_family == AF_INET) {
        struct sockaddr_in *sa4 = (struct sockaddr_in *) sa;
        char addr_buf[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &sa4->sin_addr, addr_buf, sizeof(addr_buf)) == NULL) {
            return NULL;
        }
        port = ntohs(sa4->sin_port);
        snprintf(buffer, sizeof(buffer), "%s:%u", addr_buf, port);
    } else {
        return NULL;
    }
    return buffer;
}

bool stopping;

#define PACKET_MAX 10240
#define MAX_EVENTS 10

static void signal_handler(int sig) {
    switch (sig) {
        case SIGTERM:
            stopping = true;
            break;
    }
}

int run_event_loop(struct options *options) {
    int r = -1;
    signal(SIGTERM, signal_handler);
#if defined(EVFILT_READ)
    int kq;
    if ((kq = kqueue()) == -1) {
        log_err(LOG_ERR, "kqueue");
        return -1;
    }
    struct kevent ev, events[MAX_EVENTS];
#elif defined(EPOLLIN)
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        log_err(LOG_ERR, "epoll_create1");
        return -1;
    }
    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
#endif

    // Create recv_socks and send_socks for IPv6 reflection zones.
    struct sockaddr_in6 sa6 = {
            .sin6_family=AF_INET6,
            .sin6_port = htons(MDNS_PORT),
            .sin6_addr = IN6ADDR_ANY_INIT,
    };
    struct sockaddr_in6 sa_group6 = {
            .sin6_family=AF_INET6,
            .sin6_port = htons(MDNS_PORT),
            .sin6_addr = MDNS_ADDR6_INIT,
    };
    for (const struct reflection_zone *rz = options->rz_list6; rz; rz = rz->next) {
        for (struct reflection_if *rif = rz->first_if; rif; rif = rif->next) {
            rif->send_fd = new_send_socket((struct sockaddr_storage *) &sa6, sizeof(sa6), rif->ifindex);
            if (rif->send_fd < 0) {
                log_err(LOG_ERR, "Failed to setup IPv6 send socket for interface %s", rif->ifname);
                goto end;
            }
            rif->recv_fd = new_recv_socket((struct sockaddr_storage *) &sa6, sizeof(sa6), rif->ifindex);
            if (rif->recv_fd < 0) {
                log_err(LOG_ERR, "Failed to setup IPv6 recv socket for interface %s", rif->ifname);
                goto end;
            }
#if defined(EVFILT_READ)
            EV_SET(&ev, rif->recv_fd, EVFILT_READ, EV_ADD, 0, 0, rif);
            if (kevent(kq, &ev, 1, NULL, 0, NULL) == -1) {
                log_err(LOG_ERR, "kevent");
                goto end;
            }
#elif defined(EPOLLIN)
            ev.data.ptr = rif;
            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, rif->recv_fd, &ev) == -1) {
                log_err(LOG_ERR, "epoll_ctl EPOLL_CTL_ADD");
                goto end;
            }
#endif
            sa_group6.sin6_scope_id = rif->ifindex;
            if (mcast_join(rif->recv_fd, (struct sockaddr_storage *) &sa_group6, sizeof(sa_group6), rif->ifindex) <
                0) {
                log_err(LOG_ERR, "Failed to join interface %s to IPv6 multicast group", rif->ifname);
                goto end;
            }
        }
    }

    // Create recv_socks and send_socks for IPv4 reflection zones.
    struct sockaddr_in sa4 = {
            .sin_family = AF_INET,
            .sin_port = htons(MDNS_PORT),
            .sin_addr.s_addr = htonl(INADDR_ANY),
    };
    struct sockaddr_in sa_group4 = {
            .sin_family = AF_INET,
            .sin_port = htons(MDNS_PORT),
            .sin_addr.s_addr = htonl(MDNS_ADDR4),
    };
    for (const struct reflection_zone *rz = options->rz_list4; rz; rz = rz->next) {
        for (struct reflection_if *rif = rz->first_if; rif; rif = rif->next) {
            rif->send_fd = new_send_socket((struct sockaddr_storage *) &sa4, sizeof(sa4), rif->ifindex);
            if (rif->send_fd < 0) {
                log_err(LOG_ERR, "Failed to setup IPv4 send socket for interface %s", rif->ifname);
                goto end;
            }
            rif->recv_fd = new_recv_socket((struct sockaddr_storage *) &sa4, sizeof(sa4), rif->ifindex);
            if (rif->recv_fd < 0) {
                log_err(LOG_ERR, "Failed to setup IPv4 recv socket for interface %s", rif->ifname);
                goto end;
            }
#if defined(EVFILT_READ)
            EV_SET(&ev, rif->recv_fd, EVFILT_READ, EV_ADD, 0, 0, rif);
            if (kevent(kq, &ev, 1, NULL, 0, NULL) == -1) {
                log_err(LOG_ERR, "kevent");
                goto end;
            }
#elif defined(EPOLLIN)
            ev.data.ptr = rif;
            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, rif->recv_fd, &ev) == -1) {
                log_err(LOG_ERR, "epoll_ctl EPOLL_CTL_ADD");
                goto end;
            }
#endif
            if (mcast_join(rif->recv_fd, (struct sockaddr_storage *) &sa_group4, sizeof(sa_group4), rif->ifindex) <
                0) {
                log_err(LOG_ERR, "Failed to join interface %s to IPv4 multicast group", rif->ifname);
                goto end;
            }
        }
    }

    struct sockaddr_storage peer_addr;
    char peer_addr_str[INET6_ADDRSTRLEN + 2 + 1 + 5 + 1 + 10];
    char buffer[PACKET_MAX];
    char cmbuf[0x100];
    struct iovec iov = {
            .iov_base = buffer,
            .iov_len = sizeof(buffer),
    };
    struct msghdr mh = {
            .msg_name = &peer_addr,
            .msg_namelen = sizeof(peer_addr),
            .msg_iov = &iov,
            .msg_iovlen = 1,
            .msg_control = cmbuf,
            .msg_controllen = sizeof(cmbuf),
    };

    while (!stopping) {
#if defined(EVFILT_READ)
        log_msg(LOG_DEBUG, "kevent");
        int nevents = kevent(kq, NULL, 0, events, MAX_EVENTS, NULL);
        if (nevents == -1) {
            log_err(LOG_ERR, "kevent");
            goto end;
        }
#elif defined(EPOLLIN)
        log_msg(LOG_DEBUG, "epoll_wait");
        int nevents = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nevents == -1) {
            log_err(LOG_ERR, "epoll_wait");
            goto end;
        }
#endif
        for (int i = 0; i < nevents; ++i) {
#if defined(EVFILT_READ)
            int fd = (int) events[i].ident;
            struct reflection_if *rif = events[i].udata;
#elif defined(EPOLLIN)
            struct reflection_if *rif = events[i].data.ptr;
            int fd = rif->recv_fd;
#endif
            for (;;) {
                log_msg(LOG_DEBUG, "recvmsg");
                ssize_t recv_size = recvmsg(fd, &mh, 0);
                if (recv_size == -1) {
                    if (errno == EWOULDBLOCK)
                        break;
                    log_err(LOG_ERR, "recvfrom");
                    goto end;
                }
                if (options->log_level <= LOG_INFO) {
                    snprintf(peer_addr_str, sizeof(peer_addr_str), "%s", sockaddr_storage_to_string(&peer_addr));
                    log_msg(LOG_INFO, "received %u bytes from interface %s with source IP %s",
                            recv_size, rif->ifname, peer_addr_str);
                }
                if (recv_size >= PACKET_MAX) {
                    log_msg(LOG_WARNING, "ignoring because it is too large (limit is %d bytes)", PACKET_MAX);
                    continue;
                }
                // Send to other interfaces.
                struct sockaddr *dst;
                socklen_t dst_len;
                if (peer_addr.ss_family == AF_INET6) {
                    dst = (struct sockaddr *) &sa_group6;
                    dst_len = sizeof(sa_group6);
                } else if (peer_addr.ss_family == AF_INET) {
                    dst = (struct sockaddr *) &sa_group4;
                    dst_len = sizeof(sa_group4);
                } else {
                    log_msg(LOG_WARNING, "ignoring packet from unknown address family: %d", peer_addr.ss_family);
                    continue;
                }
                for (struct reflection_if *dst_rif = rif->zone->first_if; dst_rif; dst_rif = dst_rif->next) {
                    if (dst_rif == rif)
                        continue;
                    if (peer_addr.ss_family == AF_INET6)
                        sa_group6.sin6_scope_id = dst_rif->ifindex;
                    log_msg(LOG_INFO, "forwarding to interface %s", dst_rif->ifname);
                    if (sendto(dst_rif->send_fd, buffer, (size_t) recv_size, 0, dst, dst_len) == -1) {
                        if (errno == EWOULDBLOCK)
                            continue;  // send queue overwhelmed; skipping
                        log_err(LOG_ERR, "sendto");
                        goto end;
                    }
                    log_msg(LOG_DEBUG, "sent");
                }
            }
        }
    }

    r = 0;
    end:
    for (const struct reflection_zone *rz = options->rz_list6; rz; rz = rz->next) {
        for (struct reflection_if *rif = rz->first_if; rif; rif = rif->next) {
            close(rif->recv_fd);
            close(rif->send_fd);
        }
    }
    for (const struct reflection_zone *rz = options->rz_list4; rz; rz = rz->next) {
        for (struct reflection_if *rif = rz->first_if; rif; rif = rif->next) {
            close(rif->recv_fd);
            close(rif->send_fd);
        }
    }
#if defined(EVFILT_READ)
    close(kq);
#elif defined(EPOLLIN)
    close(epoll_fd);
#endif
    return r;
}
