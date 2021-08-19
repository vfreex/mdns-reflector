# mDNS Reflector

mDNS Reflector (mdns-reflector) is a lightweight and performant multicast DNS (mDNS) reflector with a modern design.
It reflects mDNS queries and responses among multiple LANs, which allows you to run untrusted IoT devices
in a separate LAN but those devices can still be discovered in other LANs.

## Highlights
- Low footprint, no dynamic memory allocation during reflecting
- Rootless - can be run as either root or non-root
- Supports zone based reflection
- Supports both IPv4 and IPv6
- Leverages epoll on Linux and kqueue on BSD and macOS

It provides a command line interface (CLI) familiar to the discontinued [mdns-repeater][].

## Installation
### Install from Binary
#### Docker
```sh
docker pull yuxzhu/mdns-reflector:latest
```
#### Podman
```sh
podman pull yuxzhu/mdns-reflector:latest
```
#### Fedora / CentOS 8 / CentOS Stream 8
```sh
dnf copr enable -y yux/networking
dnf install -y mdns-reflector
```
#### CentOS 7
```sh
curl -fLo /etc/yum.repos.d/yux-networking-epel-7.repo  https://copr.fedorainfracloud.org/coprs/yux/networking/repo/epel-7/yux-networking-epel-7.repo
yum install -y mdns-reflector
```
#### Debian / Ubuntu
Coming soon.

#### OpenWRT
Coming soon.

### Install from Source
```sh
git clone https://github.com/vfreex/mdns-reflector
cd mdns-reflector && mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=release ..
make install
```

## Usage

`mdns-reflector` is easy to use. Just run the following command on a multi-homed host:

```sh
mdns-reflector -f eth0 eth1
```

where `eth0` and `eth1` are the interfaces that you want to reflect mDNS for.

Run `mdns-reflector -h` for help.

Similarly, run with Docker in the foreground:

```sh
docker run --net=host yuxzhu/mdns-reflector:latest -f eth0 eth1
```

Or run with Docker as a daemon:

```sh
docker run -d --restart=always --net=host yuxzhu/mdns-reflector:latest -f eth0 eth1
```

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
