Name:           mdns-reflector
Version:        0.0.1
Release:        0.dev.1%{?dist}
Summary:        lightweight and performant multicast DNS (mDNS) reflector

License:        GPLv3+
URL:            https://github.com/vfreex/mdns-reflector
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  systemd
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


Requires(post):    systemd
Requires(preun):   systemd
Requires(postun):  systemd


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
install -d $RPM_BUILD_ROOT%{_sysconfdir}/sysconfig
install misc/mdns-reflector $RPM_BUILD_ROOT%{_sysconfdir}/sysconfig/mdns-reflector
install -d $RPM_BUILD_ROOT%{_unitdir}
install -t $RPM_BUILD_ROOT%{_unitdir} misc/mdns-reflector.service


%files
%caps(cap_net_raw+ep) %{_bindir}/mdns-reflector
%config(noreplace) %{_sysconfdir}/sysconfig/mdns-reflector
%{_unitdir}/mdns-reflector.service
%{!?_licensedir:%global license %%doc}
%license COPYING
%doc README.md

%post
%systemd_post mdns-reflector.service

%preun
%systemd_preun mdns-reflector.service

%postun
%systemd_postun_with_restart mdns-reflector.service

%changelog
* Wed Aug 18 2021 Yuxiang Zhu <me@yux.im>
- Initial release
