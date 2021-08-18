Name:           mdns-reflector
Version:        0.0.1
Release:        0.dev.1%{?dist}
Summary:        lightweight and performant multicast DNS (mDNS) reflector

License:        GPLv3+
URL:            https://github.com/vfreex/mdns-reflector
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  make
%if 0%{?rhel} == 7
BuildRequires:  cmake3
%global cmake %{cmake3}
%global cmake_build %{cmake3_build}
%global cmake_install %{cmake3_install}
%else
BuildRequires:  cmake
%endif


%description
mDNS Reflector (mdns-reflector) is a lightweight and performant multicast DNS (mDNS) reflector with a modern design.
It reflects mDNS queries and responses among multiple LANs, which allows you to run untrusted IoT devices
in a separate LAN but those devices can still be discovered in other LANs.

mDNS Reflector supports zone based reflection and IPv6.


%prep
%setup -q


%build
%cmake .
%cmake_build


%install
%cmake_install


%files
%{_bindir}/mdns-reflector
%{!?_licensedir:%global license %%doc}
%license COPYING
%doc README.md


%changelog
* Wed Aug 18 2021 Yuxiang Zhu <me@yux.im>
- Initial release
