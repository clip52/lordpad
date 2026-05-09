Name:           lordpad
Version:        0.9
Release:        1%{?dist}
Summary:        Editor de código nativo Linux Qt6

License:        GPLv3+
URL:            https://github.com/clip52/notepad-fedora
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qttools-devel
BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires:  uchardet-devel
BuildRequires:  hunspell-devel
BuildRequires:  pkgconfig
BuildRequires:  desktop-file-utils
BuildRequires:  libappstream-glib

Requires:       qt6-qtbase
Requires:       uchardet
Requires:       hunspell
Requires:       hicolor-icon-theme

%description
LordPad é um editor de código nativo Linux baseado em Qt6, com Scintilla e
Lexilla para edição e realce de sintaxe, suporte a LSP, integração Git, mais de
60 painéis (REST, GraphQL, SQLite, DB shells, Docker, kubectl, systemd, túneis
SSH, SSHFS, cofre de senhas, viewers de CSV/MD/SQL/Hex/JSON, etc.) e plugin
host em Python.

%prep
%autosetup -n %{name}-%{version}

%build
cd PowerEditor-qt
%cmake -DCMAKE_BUILD_TYPE=Release
%cmake_build

%install
cd PowerEditor-qt
%cmake_install

%check
desktop-file-validate %{buildroot}%{_datadir}/applications/%{name}.desktop
appstream-util validate-relax --nonet %{buildroot}%{_datadir}/metainfo/%{name}.metainfo.xml

%files
%license LICENSE
%doc README.md docs/
%{_bindir}/lordpad
%{_datadir}/applications/lordpad.desktop
%{_datadir}/metainfo/lordpad.metainfo.xml
%{_datadir}/icons/hicolor/scalable/apps/lordpad.svg
%{_datadir}/lordpad/cheatsheets/

%changelog
* Wed May 07 2026 Lord Clip <provisionamento@gmail.com> - 0.9-1
- LordPad 0.9: rebrand do notepadpp-qt; 70+ milestones de funcionalidade.
