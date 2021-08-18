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

#ifndef MDNS_REFLECTOR_MCAST_H
#define MDNS_REFLECTOR_MCAST_H

#include <stdint.h>
#include <sys/socket.h>

/// Join a multicast group.
/// \param fd socket fd
/// \param sa multicast group address
/// \param sa_len length of multicast group address
/// \param ifindex interface index, or 0 to let the kernel decide
/// \return 0 ON success or -1 ON error
int mcast_join(int fd, const struct sockaddr_storage *sa, socklen_t sa_len, uint32_t ifindex);

#endif //MDNS_REFLECTOR_MCAST_H
