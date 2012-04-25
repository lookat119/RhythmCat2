# RhythmCat RPM SPEC File

Name: rhythmcat2
Version: 1.9.2
Release: 1
Summary: GTK+ frontend of RhythmCat2 Music Player
Source0: RhythmCat2-%{version}.tar.gz
License: GPLv3
Group: Application/Multimedia
URL: https://github.com/supercatexpert/RhythmCat2/

Requires: glib2 >= 2.32 gtk3 >= 3.4 gstreamer >= 0.10.30
Requires: gstreamer-plugins-base >= 0.10.30 gstreamer-plugins-good >= 0.10.30
Requires: json-c >= 0.9 librhythmcat2 >= 1.9.2
BuildRequires: glib2-devel >= 2.32 gtk3-devel >= 2.32 gstreamer-devel >= 0.10.30
BuildRequires: gstreamer-plugins-base-devel >= 0.10.30 json-c-devel >= 0.9
BuildRequires: desktop-file-utils

%global _unpackaged_files_terminate_build 0

%description
RhythmCat2 is a music player which can be used under Linux. It is not only a
normal music player, it has both command-line interface and graphic user
interface, and it can extend its features by plug-ins, like lyric show
in window, desktop lyric show, etc... The GTK+ UI can use custom GTK+ 3 themes,
and the UI layout can be customed partially.

%package -n librhythmcat2
Summary: Core library of RhythmCat Music Player
Group: System Environment/Libraries
Requires: glib2 >= 2.32 gstreamer >= 0.10.30 gstreamer-plugins-base >= 0.10.30
Requires: gstreamer-plugins-good >= 0.10.30 json-c >= 0.9
BuildRequires: glib2-devel >= 2.32 gstreamer-devel >= 0.10.30 json-c-devel >= 0.9
BuildRequires: gstreamer-plugins-base-devel >= 0.10.30 
%description -n librhythmcat2
This package provides core backend library of RhythmCat2 Music Player.

%package -n rhythmcat2-devel
Summary: Development files of the GTK+ frontend of RhythmCat2
Group: Development/Libraries
Requires: librhythmcat2 >= 1.9.2 rhythmcat2 >= 1.9.2
%description -n rhythmcat2-devel
This package is required to build plugins for RhythmCat2
(GTK+ frontend).

%package -n librhythmcat2-devel
Summary: Development files of LibRhythmCat2
Group: Development/Libraries
Requires: librhythmcat2 >= 1.9.2
%description -n librhythmcat2-devel
This package is required to build plugins for RhythmCat2
(core library).

%package -n rhythmcat2-doc
Summary: Development documents of the GTK+ frontend of RhythmCat2
Group: Documentation
BuildRequires: gtk-doc
%description -n rhythmcat2-doc
This package contains the API reference manual for RhythmCat2
(GTK+ frontend), which is useful when developing the plug-ins.

%package -n librhythmcat2-doc
Summary: Development documents of the GTK+ frontend of RhythmCat2
Group: Documentation
BuildRequires: gtk-doc
%description -n librhythmcat2-doc
This package contains the API reference manual for LibRhythmCat2,
which is useful when developing the plug-ins.

%package -n rhythmcat2-plugins-base
Summary: Base plug-ins for RhythmCat2
Group: Application/Multimedia
Requires: glib2 >= 2.32 gtk3 >= 3.4 gstreamer >= 0.10.30
Requires: gstreamer-plugins-base >= 0.10.30 gstreamer-plugins-good >= 0.10.30
Requires: json-c >= 0.9 librhythmcat2 >= 1.9.2 rhythmcat2 >= 1.9.2
BuildRequires: glib2-devel >= 2.32 gtk3-devel >= 2.32 gstreamer-devel >= 0.10.30
BuildRequires: gstreamer-plugins-base-devel >= 0.10.30 json-c-devel >= 0.9
%description -n rhythmcat2-plugins-base
This package provides base plug-ins, including: lyric show, desktop lyric, 
multimedia key supprot, MPRIS2 support, and notify popups.

%package -n rhythmcat2-plugins-extra
Summary: Extra plug-ins for RhythmCat2
Group: Application/Multimedia
Requires: glib2 >= 2.32 gtk3 >= 3.4 gstreamer >= 0.10.30
Requires: gstreamer-plugins-base >= 0.10.30 gstreamer-plugins-good >= 0.10.30
Requires: json-c >= 0.9 librhythmcat2 >= 1.9.2 rhythmcat2 >= 1.9.2 libcurl
BuildRequires: glib2-devel >= 2.32 gtk3-devel >= 2.32 gstreamer-devel >= 0.10.30
BuildRequires: gstreamer-plugins-base-devel >= 0.10.30 json-c-devel >= 0.9
BuildRequires: libcurl-devel
%description -n rhythmcat2-plugins-extra
This package provides extra plug-ins, including: lyric crawler, and lyric
crawler downloader modules.

%package -n rhythmcat2-plugins-python3-loader
Summary: Python3 plug-in loader for RhythmCat2
Group: Application/Multimedia
Requires: glib2 >= 2.32 gtk3 >= 3.4 gstreamer >= 0.10.30
Requires: gstreamer-plugins-base >= 0.10.30 gstreamer-plugins-good >= 0.10.30
Requires: json-c >= 0.9 librhythmcat2 >= 1.9.2 rhythmcat2 >= 1.9.2 python3 >= 3.2
Requires: python3-libs >= 3.2
BuildRequires: glib2-devel >= 2.32 gtk3-devel >= 2.32 gstreamer-devel >= 0.10.30
BuildRequires: gstreamer-plugins-base-devel >= 0.10.30 json-c-devel >= 0.9
BuildRequires: python3-devel >= 3.2
%description -n rhythmcat2-plugins-python3-loader
This package provides Python3 plug-in loader, which is used to load Python3
based plug-ins.

%prep
%setup -q -n RhythmCat2-%{version}

%build
%configure --enable-gtk-doc --with-native-plugins --with-python3-plugins
make %{_smp_mflags}

%install
rm -rf %{buildroot}
make DESTDIR=%{buildroot} install
# install desktop file and install
desktop-file-install --add-category="AudioVideo" --delete-original \
    --dir=%{buildroot}%{_datadir}/applications \
    %{buildroot}/%{_datadir}/applications/RhythmCat2.desktop
desktop-file-validate %{buildroot}/%{_datadir}/applications/RhythmCat2.desktop

%post
update-desktop-database &> /dev/null || :
update-mime-database %{_datadir}/mime &> /dev/null || :

%postun
update-desktop-database &> /dev/null || :
update-mime-database %{_datadir}/mime &> /dev/null || :

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%doc COPYING AUTHORS
%{_bindir}/RhythmCat2
%{_datadir}/applications/RhythmCat2.desktop
%{_datadir}/RhythmCat2
%{_datadir}/man/*

%files -n librhythmcat2
%{_libdir}/librhythmcat-2.0.a
%{_libdir}/librhythmcat-2.0.so*
%{_datadir}/locale
%post -n librhythmcat2 -p /sbin/ldconfig
%postun -n librhythmcat2 -p /sbin/ldconfig

%files -n rhythmcat2-devel
%{_includedir}/rhythmcat2
%{_libdir}/pkgconfig/rhythmcat-2.0.pc

%files -n librhythmcat2-devel
%{_includedir}/librhythmcat2
%{_libdir}/pkgconfig/librhythmcat-2.0.pc

%files -n rhythmcat2-doc
%{_datadir}/gtk-doc/html/RhythmCat-1.9.2

%files -n librhythmcat2-doc
%{_datadir}/gtk-doc/html/LibRhythmCat-1.9.2

%files -n rhythmcat2-plugins-base
%{_libdir}/RhythmCat2/plugins/lyricshow.so
%{_libdir}/RhythmCat2/plugins/desklrc.so
%{_libdir}/RhythmCat2/plugins/mediakey.so
%{_libdir}/RhythmCat2/plugins/mpris2.so
%{_libdir}/RhythmCat2/plugins/notify.so

%files -n rhythmcat2-plugins-extra
%{_libdir}/RhythmCat2/plugins/lrccrawler.so
%{_libdir}/RhythmCat2/plugins/CrawlerModules/lrccrawler-lrc123.so

%files -n rhythmcat2-plugins-python3-loader
%{_libdir}/RhythmCat2/plugins/python3.so

%changelog
* Mon Apr 23 2012 SuperCat <supercatexpert@gmail.com> - 1.9.2-1
- The 1.9.2-1 Unstable Version Package.

