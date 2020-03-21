Name:		tmux-xdg
Version:	0.0.1
Release:        1%{?dist}
Summary:	A wrapper for tmux to enforce XDG Base Directory Specification compliance
License:	GPLv3
Source0:	%{name}-%{version}.tar.xz

BuildRequires:	meson
BuildRequires:	cmake
BuildRequires:	gcc
Requires:	tmux

%description
A wrapper for tmux to enforce XDG Base Directory Specification compliance


%prep
%autosetup


%build
%meson
%meson_build

%install
%meson_install


%files
/usr/local/bin/tmux

%changelog
* Sat Mar 21 2020 Edd Salkield <edd@salkield.uk> - 0.0.1-1
- Initial packaging
