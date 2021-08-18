# mDNS Reflector

mDNS Reflector (mdns-reflector) is a lightweight and performant multicast DNS (mDNS) reflector with modern design.
It reflects mDNS queries and responses among multiple LANs, which allows you to run untrusted IoT devices
in a separate LAN but can still be discovered by other LANs.
Supports zone based reflection and IPv6.

It provides a command line interface (CLI) familiar to the discontinued [mdns-repeater][].

## License
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

[mdns-repeater]: https://bitbucket.org/geekman/mdns-repeater/
