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

#include "reflection_zone.h"
#include "logging.h"
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>

static int cmp_unsigned_int(const void *a, const void *b) {
    unsigned int av = *(unsigned int *) a;
    unsigned int bv = *(unsigned int *) b;
    if (av == bv)
        return 0;
    if (av < bv)
        return -1;
    return 1;
}

bool check_reflection_zone(const struct reflection_zone *rz_list) {
    if (!rz_list) {
        fputs("ERROR: At least 1 reflection zone must be specified.\n", stderr);
        return false;
    }
    size_t nifs = 0;
    for (const struct reflection_zone *rz = rz_list; rz; rz = rz->next) {
        if (rz->nifs < 2) {
            fputs("ERROR: At least 2 interfaces must be specified in each reflection zone.\n", stderr);
            return false;
        }
        nifs += rz->nifs;
    }
    unsigned int ifindices[nifs];
    size_t index = 0;
    for (const struct reflection_zone *rz = rz_list; rz; rz = rz->next) {
        for (struct reflection_if *rif = rz->first_if; rif; rif = rif->next) {
            ifindices[index++] = rif->ifindex;
        }
    }
    qsort(ifindices, nifs, sizeof(unsigned int), cmp_unsigned_int);
    for (size_t i = 1; i < nifs; ++i) {
        if (ifindices[i - 1] == ifindices[i]) {
            fputs("ERROR: Duplicate interfaces are not allowed.\n", stderr);
            return false;
        }
    }
    return true;
}

struct reflection_zone *new_reflection_zone(unsigned int zone_index, struct reflection_zone *rz_list) {
    struct reflection_zone *rz = malloc(sizeof(struct reflection_zone));
    if (!rz) {
        return NULL;
    }
    memset(rz, 0, sizeof(struct reflection_zone));
    rz->zone_index = zone_index;
    rz->next = rz_list;
    return rz;
}

struct reflection_if *new_reflection_if(unsigned int ifindex, const char *ifname, struct reflection_zone *rz) {
    struct reflection_if *rif = malloc(sizeof(struct reflection_if));
    if (!rif) {
        return NULL;
    }
    memset(rif, 0, sizeof(struct reflection_if));
    rif->recv_fd = -1;
    rif->send_fd = -1;
    rif->ifindex = ifindex;
    snprintf(rif->ifname, IF_NAMESIZE, "%s", ifname);
    rif->zone = rz;
    if (rz) {
        rif->next = rz->first_if;
        rz->first_if = rif;
        rz->nifs++;
    }
    return rif;
}
