Name:           microsocks
Version:        1.0.1
Release:        4%{?dist}
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

%pre
# Create microsocks user/group 
# TODO: control user via configure build flag
getent group microsocks  >/dev/null || groupadd -r microsocks
getent passwd microsocks >/dev/null || \
    useradd --system --gid microsocks --shell /sbin/nologin \
    --comment "microsocks service account" --no-create-home \
    microsocks
exit 0


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
* Tue Mar 25 2020 OnceUponALoop <firas.alshafei@us.abb.com> 1.0.1-3
- Fix bug in systemd unit file not sourcing sysconfig arguments

* Tue Mar 24 2020 OnceUponALoop <firas.alshafei@us.abb.com> 1.0.1-3
- Create microsocks nologin service user
- Update systemd unit to run as service user

* Mon Mar 23 2020 OnceUponALoop <firas.alshafei@us.abb.com> 1.0.1-2
- First release
- Transition to GNU Autotools
- Update usage message

