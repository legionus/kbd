Name: kbd
Serial: 0
Version: 1.12.1
Release: alt1

Group: Terminals
Summary: Tools for managing the Linux console (variant: kbd)
License: GPL
Url: ftp://ftp.win.tue.nl/pub/linux-local/utils/kbd/
# HOWTO at http://www.win.tue.nl/~aeb/linux/kbd/

Packager: Alexey Gladkov <legion@altlinux.ru>

ExclusiveOS: Linux
ExcludeArch: s390 s390x

Provides: console-tools_or_kbd = %name%serial:%version-%release
Conflicts: console-tools_or_kbd < %name%serial:%version-%release
Conflicts: console-tools_or_kbd > %name%serial:%version-%release

# due to file extarctions from this pkg to other
# (console-data, console-common-scripts)
Conflicts: interactivesystem < 1:sisyphus-alt12

# for compatibility:
Requires: console-vt-tools

#TODO: the feature to build kernels: loadkeys-mktable-macros

Source0: ftp://ftp.kernel.org/pub/linux/utils/kbd/kbd-%version.tar.bz2

# Debian kbd-1.12-10 patches (according to debian/patches/series):
Patch22: po_makefile.diff
Patch23: man_pages.diff

#NMU Patches:
Patch100: kbd-1.12-alt-unicode_start_vs_setfont.patch

# due to grep, sed, xargs
BuildRequires(install): grep, sed, findutils
# due to provided PAM config file: 
# libpam-devel is required to compute what pkg provides
BuildRequires(install): libpam-devel

# Automatically added by buildreq on Tue Apr 12 2005
BuildRequires: flex

%description
This package contains tools for managing the Linux console
(Linux console, virtual terminals on it, keyboard, etc.)--mainly,
what they do is loading console fonts and keyboard maps.

Basic tools for controlling the vitual terminals are in console-vt-tools package.

A set of various fonts and keyboard maps is provided by console-data package.

%description -l ru_RU.KOI8-R
Этот пакет содержит инструменты для управления консолью Linux
(консолью, виртуальными терминалами на ней, клавиатурой и т.п.), в основном, они
занимаются загрузкой консольных шрифтов и раскладок клавиатуры.

Простые инструменты для управления виртуальными терминалами находятся
в пакете console-vt-tools.

Набор разнообразных шрифтов и описаний раскладок предоставляется
пакетом console-data.

%package -n kbd-docs
Group: Documentation
Summary: Documentation for kbd

%description -n kbd-docs
Documentation for kbd

%package -n console-vt-tools
Group: Terminals
Summary: Tools to control VTs (virtual terminals) on the Linux console
# Separate pkg is made to distinguish dependencies to vt-tools and other
# (for interchangebility with console-tools).

# due to the same files before pkg split:
Conflicts: kbd < 0:1.12-alt1
Conflicts: console-tools < 0:0.2.3-ipl30mdk

# console-tools also produces a console-vt-tools pkg with its own version.
# So if you want to specify correct "obsoletion" order, 
# you should make correct declarations here, incl. a Serial:
Serial: 0

%description -n console-vt-tools
console-vt-tools perform simple control operations on the VTs on Linux console
(like switching between them). 

Usually, several VTs are used ontop of the Linux console.
These scripts are useful for writing scripts that need to control them.

%package -n kbdrate
Group: System/Configuration/Hardware
Summary: Reset the keyboard repeat rate and delay time

# due to the parent package change (util-linux --> kbd)
%define kbdrate_serial 1
Serial: %kbdrate_serial

# due to kbdrate
Conflicts: util-linux < 2.11h-alt2

# FIX: kbdrate l10n messages are in the common kbd catalogue

%description -n kbdrate
This package is used to change the keyboard repeat rate and delay time.
The delay is the amount of time that a key must be depressed before it
will start to repeat.

%package -n kbdrate-usermode
Group: System/Configuration/Hardware
Summary: Usermode bindings for kbdrate

# due to the parent package change (util-linux --> kbd)
Serial: %kbdrate_serial

Requires: kbdrate = %kbdrate_serial:%version-%release
Requires: consolehelper

%description -n kbdrate-usermode
Kbdrate package is used to change the keyboard repeat rate and delay time.
The delay is the amount of time that a key must be depressed before it
will start to repeat.

This package contains usermode bindings for kbdrate.


%prep
%setup -q

# Debian:
%patch22 -p1 -b .po_make
%patch23 -p1 -b .man

%patch100 -p1 -b .unicode_start_vs_setfont

%build
# We don't use %% {configure} because the ./configure included here does not
# understand most of the options.
export CFLAGS="-Wextra"
./configure --prefix=/  --datadir=%_libdir/kbd --mandir=%_mandir 

# Override CFLAGS because this configure ignores them anyway, and LDFLAGS
# because it defaults to -s, but that's a build policy decision.
%make_build

%install
%define _makeinstall_target install-progs install-man

# Basic install.
%makeinstall DESTDIR="%buildroot" \
	\
	BINDIR="%buildroot/%_bindir" \
	LOADKEYS_BINDIR="%buildroot/bin" \
	MANDIR="%buildroot/%_mandir" \
	gnulocaledir="%buildroot/%_datadir/locale" \
	localedir="%buildroot/%_datadir/locale"

# Move binaries which we use before /usr is mounted from %_bindir to /bin.
for binary in setfont dumpkeys kbd_mode unicode_start unicode_stop ; do
	mv %buildroot/%_bindir/$binary %buildroot/bin
	ln -s /bin/$binary %buildroot/%_bindir/$binary
done

# Set up kbdrate to be userhelpered.
mkdir -p %buildroot/sbin \
	%buildroot/%_sysconfdir/security/console.apps \
	%buildroot/%_sysconfdir/pam.d

install -p -m640 rpm/util-linux-2.9w-kbdrate.pamd %buildroot/%_sysconfdir/pam.d/kbdrate
install -p -m640 rpm/util-linux-2.9w-kbdrate.apps %buildroot/%_sysconfdir/security/console.apps/kbdrate

mv %buildroot/%_bindir/kbdrate %buildroot/sbin/
%__ln_s %_libdir/helper/consolehelper %buildroot/%_bindir/kbdrate

%find_lang %name

%files -f %name.lang
/bin/*
/sbin/*
%_bindir/*
%_mandir/*/*
%exclude %_bindir/chvt
%exclude %_bindir/openvt
%exclude %_bindir/deallocvt
%exclude %_bindir/fgconsole
%exclude /sbin/kbdrate
%exclude %_bindir/kbdrate
%exclude %_man1dir/chvt*
%exclude %_man1dir/openvt*
%exclude %_man1dir/deallocvt*
%exclude %_man1dir/fgconsole*
%exclude %_man8dir/kbdrate.*

%files -n kbd-docs
%doc CHANGES CREDITS README doc/*.txt doc/kbd.FAQ*.html doc/font-formats/*.html doc/utf

%files -n console-vt-tools
%_bindir/chvt
%_bindir/openvt
%_bindir/deallocvt
%_bindir/fgconsole
%_man1dir/chvt*
%_man1dir/openvt*
%_man1dir/deallocvt*
%_man1dir/fgconsole*

# as in ALT util-linux:
%files -n kbdrate
/sbin/kbdrate
%_man8dir/kbdrate.*

%files -n kbdrate-usermode
%config(noreplace) %_sysconfdir/pam.d/kbdrate
%config(noreplace) %_sysconfdir/security/console.apps/kbdrate
%_bindir/kbdrate

%changelog
* Wed May 09 2007 Alexey Gladkov <legion@altlinux.ru> 0:1.12.1-alt1
- New bugfix version
- Move documentation into kbd-docs.

* Fri Mar 03 2006 Stanislav Ievlev <inger@altlinux.org> 0:1.12-alt3
- for sisyphus

* Wed Mar 01 2006 Stanislav Ievlev <inger@altlinux.org> 0:1.12-alt2.2
- shutup loadkeys
- added stty fix

* Thu Jan 12 2006 ALT QA Team Robot <qa-robot@altlinux.org> 0:1.12-alt2.1.1
- Rebuilt for new style PAM dependencies generated by rpm-build-4.0.4-alt55.

* Tue Aug 16 2005 Stanislav Ievlev <inger@altlinux.org> 0:1.12-alt2.1
- NMU: fixed unicode_start (remove extra setfont's - was conflict)

* Tue Apr 12 2005 Ivan Zakharyaschev <imz@altlinux.ru> 0:1.12-alt2
- Buildreqs: add more (libpam-devel to compute what package provides)

* Fri Apr  8 2005 Ivan Zakharyaschev <imz@altlinux.ru> 0:1.12-alt1
- Adapting for ALT (base on RH).
- For console-{tools,data} compatibility (among other goals):
  + added Debian kbd patches (TODO: Unicode);
  + ported ALT console-tools patches (TODO: Unicode);
  + do not package data;
  + declare the conflict with console-tools (and any other variants);
  + a subpackage for console-vt-tools (alternative: the same from console-tools)
  (little PROBLEM: l10n messages for it are in kbd package)
  (this is done to avoid dependency complexities on vttools).
- Package kbdrate{,-usermode} (substitute subpackages of util-linux)
  (little PROBLEM: l10n messages for kbdrate are in kbd package).

* Sun Feb 20 2005 Miloslav Trmac <mitr@redhat.com> - 1.12-5
- Put "Meta_acute" back in German keymaps, just ignore it in (loadkeys -u)
  (patch by Jochen Schmitt)
- Don't ship patch backup files, simpler way

* Sat Feb 19 2005 Miloslav Trmac <mitr@redhat.com> - 1.12-4
- Don't ship a patch backup file
- Mention in setfont.8 that 512-glyph fonts reduce the number of available
  colors (#140935, patch by Dmitry Butskoj)
- Remove "Meta_acute" from German keymaps (#143124)
- Make the %%triggerun script condition more precise, ignore failure of the
  script

* Mon Feb 14 2005 Adrian Havill <havill@redhat.com>
- rebuilt

* Tue Jun 15 2004 Elliot Lee <sopwith@redhat.com>
- rebuilt

* Thu Feb 26 2004 Adrian Havill <havill@redhat.com>
- update to 1.12

* Fri Feb 13 2004 Elliot Lee <sopwith@redhat.com>
- rebuilt

* Wed Jan 14 2004 Bill Nottingham <notting@redhat.com> 1.08-12
- remove speakup patch at request of author

* Fri Oct 10 2003 Bill Nottingham <notting@redhat.com> 1.08-11
- remove keytable init script (#106783)

* Tue Aug 12 2003 Adrian Havill <havill@rtedhat.com> 1.08-10.1
- bump for RHEL

* Tue Aug 12 2003 Adrian Havill <havill@rtedhat.com> 1.08-10
- apply the rukbd patch (#78218)

* Thu Jul 31 2003 Adrian Havill <havill@redhat.com> 1.08-9
- don't print "plus before..." warnings about non-supported capslock
  in unimode <Andries.Brouwer@cwi.nl> (#81855)

* Wed Jul 30 2003 Adrian Havill <havill@redhat.com> 1.08-8
- replaced Russian keyboard map with working UTF-8 equivalent (#79338)

* Thu Jul 24 2003 Adrian Havill <havill@redhat.com> 1.08-7
- make euro/latin-9 the default instead of latin-1 and 7-bit (#97013)
- fix swedish keymap; se, not sv (#88791)
- add fr-latin0 legacy alias of fr-latin-9 (#88324)
- add ".map" ext to filename param of init script (#90562)

* Wed Jun 04 2003 Elliot Lee <sopwith@redhat.com>
- rebuilt

* Thu Mar 06 2003 Florian La Roche <Florian.LaRoche@redhat.de>
- build new rpm

* Fri Feb 21 2003 Florian La Roche <Florian.LaRoche@redhat.de>
- ExcludeArch mainframe

* Thu Jan 30 2003 Bill Nottingham <notting@redhat.com> 1.08-4
- remove condrestart from initscript

* Wed Jan 22 2003 Tim Powers <timp@redhat.com>
- rebuilt

* Fri Dec  6 2002 Nalin Dahyabhai <nalin@redhat.com> 1.08-2
- only output terminal unicode init sequence if both stdout and stderr are
  connected to terminals, so that it doesn't show up when script outputs
  get piped to files

* Fri Nov 22 2002 Nalin Dahyabhai <nalin@redhat.com> 1.08-1
- update to 1.08
- drop updates which went mainline

* Mon Nov 11 2002 Nalin Dahyabhai <nalin@redhat.com> 1.06-27
- add detached signature
- remove directory names from PAM configuration so that the same config file
  can be used for any arch on multilib systems

* Wed Sep  4 2002 Bill Nottingham <notting@redhat.com> 1.06-26
- don't munge /etc/sysconfig/i18n

* Tue Sep  3 2002 Bill Nottingham <notting@redhat.com> 1.06-25
- don't run setsysfont in upgrade trigger on console-tools

* Thu Aug 29 2002 Jakub Jelinek <jakub@redhat.com> 1.06-24
- use cyr-sun16 cyrillic chars in latarcyrheb-sun16 font
  instead of old LatArCyrHeb-16 chars
- add Euro character to latarcyrheb-sun16
- use latarcyrheb-sun16 by default in unicode_start script

* Tue Aug 27 2002 Jakub Jelinek <jakub@redhat.com> 1.06-23
- add back lat[02]-sun16 fonts plus latarcyrheb-sun16 font

* Thu Aug 22 2002 Karsten Hopp <karsten@redhat.de>
- needs to conflict with older util-linux packages
  (kbdrate moved between packages)

* Tue Aug 13 2002 Bill Nottingham <notting@redhat.com> 1.06-21
- remove Evil Hack in favor of slightly-less-evil-hack in initscripts

* Tue Jul  9 2002 Bill Nottingham <notting@redhat.com> 1.06-20
- fix speakup keymap names

* Tue Jul 09 2002 Phil Knirsch <pknirsch@redhat.com> 1.06-19
- Evil hack to make setfont work correctly on all consoles (#68018)

* Thu Jun 27 2002 Bill Nottingham <notting@redhat.com> 1.06-18
- move unicode_stop to /bin too
- fix path to loadkeys in keytable.init
- add in speakup keymaps

* Fri Jun 21 2002 Tim Powers <timp@redhat.com>
- automated rebuild

* Tue Jun 11 2002 Nalin Dahyabhai <nalin@redhat.com> 1.06-16
- fix incorrect path in console.apps configuration file

* Thu May 30 2002 Bill Nottingham <notting@redhat.com> 1.06-14
- move some more stuff to /bin (unicode_start and dependencies)

* Thu May 23 2002 Tim Powers <timp@redhat.com>
- automated rebuild

* Mon Feb 25 2002 Bernhard Rosenkraenzer <bero@redhat.com> 1.06-12
- Rebuild in new environment

* Wed Jan 30 2002 Bernhard Rosenkraenzer <bero@redhat.com> 1.06-11
- Oops, actually list the pam files in %files

* Tue Jan 29 2002 Bernhard Rosenkraenzer <bero@redhat.com> 1.06-10
- Add and consolehelper'ify kbdrate

* Tue Jan 29 2002 Bernhard Rosenkraenzer <bero@redhat.com> 1.06-9
- Re-remove kbdrate

* Thu Jan 24 2002 Bernhard Rosenkraenzer <bero@redhat.com> 1.06-7
- Fix build in current environment
- Get rid of kbdrate, it's in util-linux these days

* Wed Jul 18 2001 Matt Wilson <msw@redhat.com>
- added a patch (Patch4) that allows --tty= in setfont
- modified patch not to break translations

* Tue Jul  3 2001 Bernhard Rosenkraenzer <bero@redhat.com> 1.06-4
- Add cyrillic patches from leon@geon.donetsk.ua (#47144)

* Tue Jun 26 2001 Bernhard Rosenkraenzer <bero@redhat.com> 1.06-3
- Fix "Alt+AltGr=Compose" in qwertz-keyboards

* Mon Jun 25 2001 Bernhard Rosenkraenzer <bero@redhat.com> 1.06-2
- Fix "make install" and init script (#45327)

* Sat Jun 16 2001 Than Ngo <than@redhat.com>
- update to 1.0.6
- use %%{_tmppath}
- use find_lang
- support new gettext
- remove some patch files, which are included in 1.0.6
- fix to use RPM_OPT_FLAGS

* Thu May  3 2001 Bernhard Rosenkraenzer <bero@redhat.com> 1.05-3
- Fix up resizecons

* Wed May  2 2001 Bernhard Rosenkraenzer <bero@redhat.com> 1.05-2
- Build everything, obsoletes console-tools
- s/Copyright:/License:/
- License is GPL, not just distributable
- Add our compose mappings from old console-tools
- Add triggerpostun -- console-tools magic to get sane fonts and mappings

* Tue Apr 17 2001 Erik Troan <ewt@redhat.com>
- initial packaging for kbdrate
