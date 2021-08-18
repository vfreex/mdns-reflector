# This file is part of mDNS Reflector (mdns-reflector), a lightweight and performant multicast DNS (mDNS) reflector.
# Copyright (C) 2021 Yuxiang Zhu <me@yux.im>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

add_executable(mdns-reflector)
target_sources(mdns-reflector
    PRIVATE
        main.c mcast.c  logging.c daemon.c reflector.c reflection_zone.c
    PUBLIC
        mcast.h logging.h daemon.h reflector.h reflection_zone.h options.h
)
target_compile_options(mdns-reflector PRIVATE -Wall -Wextra -Wpedantic -Wconversion -D__APPLE_USE_RFC_3542)
target_compile_definitions(mdns-reflector PRIVATE)
target_include_directories(mdns-reflector PRIVATE ${MDNS_REFLECTOR_INCLUDE})

install(TARGETS mdns-reflector
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
        )
