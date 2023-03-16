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

----

## Installation of mDNS Reflector using prebuilt binaries
### Docker
```sh
docker pull yuxzhu/mdns-reflector:latest
```
### Podman
```sh
podman pull yuxzhu/mdns-reflector:latest
```
### Fedora / CentOS 8 / CentOS Stream 8
```sh
dnf copr enable -y yux/networking
dnf install -y mdns-reflector
```
### CentOS 7
```sh
curl -fLo /etc/yum.repos.d/yux-networking-epel-7.repo  https://copr.fedorainfracloud.org/coprs/yux/networking/repo/epel-7/yux-networking-epel-7.repo
yum install -y mdns-reflector
```
### Debian / Ubuntu
Available in Debian 'experimental' branch:
 - https://packages.debian.org/experimental/mdns-reflector

It can happen that the version in experimental is outdated.
In that case experienced users can either build the .deb package themselves
 - by using this the Debian Salsa repo: https://salsa.debian.org/debian-edu-pkg-team/mdns-reflector
 - by using the included debian/ folder.
### OpenWRT
Coming soon.

----

## Installing mDNS Reflector from Source
```sh
git clone https://github.com/vfreex/mdns-reflector
cd mdns-reflector && mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=release ..
make install
```

----

## Usage

`mdns-reflector` is easy to use. Just run the following command on a multi-homed host:

```sh
mdns-reflector -fn eth0 eth1
```

where `eth0` and `eth1` are the interfaces that you want to reflect mDNS for.

Run `mdns-reflector -h` for help.

Similarly, run with Docker in the foreground:

```sh
docker run --net=host yuxzhu/mdns-reflector:latest mdns-reflector -fn eth0 eth1
```

Or run with Docker as a daemon:

```sh
docker run -d --restart=always --net=host yuxzhu/mdns-reflector:latest mdns-reflector -fn eth0 eth1
```
----

## Systemd service

You can enable the systemd service with:
```sh
systemctl enable --now mdns-reflector
```

The main configuration file for the service daemon is located at:
`/etc/mdns-reflector/mdns-reflector.conf`

Although you can add files to `/etc/mdns-reflector/conf.d/` to overwrite the
main configuration file.

**Quick Tip:** Adding a number and an underscore like `10_` as a prefix to your
filename is good practice and ensures a predictable priority of the config
files. `99_*` files have a higher priority than `00_*` files which means that
the settings in file `20_example.conf` would overwrite settings from `10_example.conf`.

## Multiple systemd services

mDNS Reflector supports multiple services running in parallel.
You can archieve this by creating files like `/etc/mdns-reflector/%i.conf`.

Now enable the systemd service with this specific config file:
```sh
systemctl enable --now mdns-reflector@$(systemd-escape "%i")
```

**NOTE:** Replace `%i` with your config filename *without* the `.conf` at the end.
The `.conf` filename suffix is still necessary though.

### Important information about multiple services running in parallel
Let us define a few networks to make an example frame `<iface_name>: <network_name>`:
  - eth_home:  `Homenetwork`
  - eth_smart: `Smarthome-Devices`
  - eth_print: `Printers`

We want the devices in `Printers` to announce their services into the
`Homenetwork`. Likewise the devices in `Smarthome-Devices` should also announce
their services into the `Homenetwork`.

mDNS Reflector does **not** support an interface occuring in **more than one**
reflection planes. For example:
```sh
mdns-reflector eth_home eth_smart -- eth_home eth_print # Wrong!
```

At the moment this  is only possible if multiple mDNS Reflector services are
running in parallel.

Uni-directional service announcement is not supported at the moment but is planned.

So, for this example you'll need two config files.

`/etc/mdns-reflector/smarthomestuff.conf`:
```
INTERFACES=eth_home eth_smart
More stuff...
```

`/etc/mdns-reflector/printerstuff.conf`:
```
INTERFACES=eth_home eth_print
More stuff...
```

Enabling the service of course:
```sh
systemctl enable --now mdns-reflector@$(systemd-escape "smarthomestuff")
systemctl enable --now mdns-reflector@$(systemd-escape "printerstuff")
```

This would make all mDNS services visible in `Homenetwork`. But beware this also
makes all services from devices withing the `Homenetwork` visible to the
`Smarthome-Devices` and `Printers` networks.

Services within the `Smarthome-Devices` network **won't** be announced to the
`Printers` network and vice-versa.

----

## License
Copyright (C) 2021-2023 Yuxiang Zhu <me@yux.im>

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
