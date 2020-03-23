Name:           microsocks
Version:        1.0.2
Release:        1%{?dist}
Summary:        Lightweight socks5 server

Group:          Applications/Internet
License:        MIT
URL:            https://github.com/rofl0r/microsocks
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  make
BuildRequires:  systemd
BuildRequires:  autoconf
BuildRequires:  automake
BuildRequires:  autoconf-archive
Requires:       glibc
Requires:       systemd

%description
A lightweight, multithreaded, and efficient socks5 server.

%prep
%setup -q
autoreconf -i


%build
%configure -docdir=%{_docdir}/%{pkgname} %{?configure_args}
make %{?_smp_mflags}

%check
make check

%install
rm -rf %{buildroot}
make install-strip DESTDIR=%{buildroot}

%clean
rm -rf %{buildroot}

%files
# binary
%{_bindir}/microsocks

# systemd service
%{_unitdir}/microsocks.service

# systemd config file
%config(noreplace) %{_sysconfdir}/sysconfig/microsocks

# documentation 
%doc README.md

# license info
%license LICENSE


%changelog
* Mon Mar 23 2020 OnceUponALoop <firas.alshafei@us.abb.com> 1.0.2-1
- First release
- Transition to GNU Autotools
- Update usage message




