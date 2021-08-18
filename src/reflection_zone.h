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

#ifndef MDNS_REFLECTOR_REFLECTION_ZONE_H
#define MDNS_REFLECTOR_REFLECTION_ZONE_H

#include <stdbool.h>
#include <net/if.h>

struct reflection_if {
    int recv_fd;
    int send_fd;
    unsigned int ifindex;
    struct reflection_zone *zone;
    char ifname[IF_NAMESIZE];
    struct reflection_if *next;
};

struct reflection_zone {
    unsigned int zone_index;
    size_t nifs;
    struct reflection_if *first_if;
    struct reflection_zone *next;
};

bool check_reflection_zone(const struct reflection_zone *rz_list);

struct reflection_zone *new_reflection_zone(unsigned int zone_index, struct reflection_zone *rz_list);

struct reflection_if *new_reflection_if(unsigned int ifindex, const char *ifname, struct reflection_zone *rz);

#endif //MDNS_REFLECTOR_REFLECTION_ZONE_H
