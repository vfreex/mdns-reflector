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

#ifndef MDNS_REFLECTOR_LOGGING_H
#define MDNS_REFLECTOR_LOGGING_H

#include <stdbool.h>

extern bool log_to_syslog;

void log_setlevel(int level);

void log_msg(int priority, const char *fmt, ...);

void log_err(int priority, const char *fmt, ...);

#endif //MDNS_REFLECTOR_LOGGING_H
