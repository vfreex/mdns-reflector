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

#include "logging.h"
#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

bool log_to_syslog;
static int log_level;

void log_setlevel(int level) {
    log_level = level;
}

void log_msg(int priority, const char *fmt, ...) {
    if (priority > log_level)
        return;
    char buffer[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);
    if (!log_to_syslog) {
        fprintf(stderr, "%s\n", buffer);
    } else {
        syslog(priority, "%s", buffer);
    }
}

void log_err(int priority, const char *fmt, ...) {
    if (priority > log_level)
        return;
    char buffer[1024];
    va_list ap;
    va_start(ap, fmt);
    int printed = vsnprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);
    assert(printed >= 0);
    size_t len = (size_t) printed;
    if (len < sizeof(buffer) - 2) {
        buffer[len++] = ':';
        buffer[len++] = ' ';
        buffer[len] = '\0';
        strerror_r(errno, buffer + len, sizeof(buffer) - len);
    }
    if (!log_to_syslog) {
        fprintf(stderr, "%s\n", buffer);
    } else {
        syslog(priority, "%s", buffer);
    }

}
