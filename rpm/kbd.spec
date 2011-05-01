Name: kbd
Serial: 0
Version: 1.15.3
Release: alt0.20110502

Group: Terminals
Summary: Tools for managing the Linux console
License: GPL
Url: ftp://ftp.kernel.org/pub/linux/utils/kbd/
# HOWTO at http://www.win.tue.nl/~aeb/linux/kbd/

Packager: Alexey Gladkov <legion@altlinux.ru>

ExclusiveOS: Linux
ExcludeArch: s390 s390x

Provides: console-tools_or_kbd = %name%serial:%version-%release
Conflicts: console-tools_or_kbd < %name%serial:%version-%release
Conflicts: console-tools_or_kbd > %name%serial:%version-%release

# for compatibility:
Requires: console-vt-tools
Obsoletes: console-tools
Provides: console-tools = %version

# due to file extarctions from this pkg to other
# (console-data, console-common-scripts)
Conflicts: interactivesystem < 1:sisyphus-alt12
Conflicts: console-common-scripts <= 0.2.2-alt1.4

Source0: kbd-%version.tar

# Debian kbd-1.12-10 patches (according to debian/patches/series):
Patch22: po_makefile.diff
Patch23: man_pages.diff

#NMU Patches:
Patch100: kbd-1.12-alt-unicode_start_vs_setfont.patch

# Automatically added by buildreq on Mon Jan 07 2008 (-bi)
BuildRequires: flex libpam-devel cvs

%description
This package contains tools for managing the Linux console
(Linux console, virtual terminals on it, keyboard, etc.)--mainly,
what they do is loading console fonts and keyboard maps.

Basic tools for controlling the vitual terminals are in console-vt-tools package.

A set of various fonts and keyboard maps is provided by console-data package.

%description -l ru_RU.UTF-8
Этот пакет содержит инструменты для управления консолью Linux
(консолью, виртуальными терминалами на ней, клавиатурой и т.п.), в основном, они
занимаются загрузкой консольных шрифтов и раскладок клавиатуры.

Простые инструменты для управления виртуальными терминалами находятся
в пакете console-vt-tools.

Набор разнообразных шрифтов и описаний раскладок предоставляется
пакетом console-data.

%package -n %name-data
Group: Terminals
Summary: Linux console data files

Obsoletes: console-data
Provides: console-data = %version

%description -n %name-data
This package contains various console fonts and keyboard maps.

%package -n %name-docs
Group: Documentation
Summary: Documentation for kbd

%description -n %name-docs
Documentation for kbd

%package -n console-vt-tools
Group: Terminals
Summary: Tools to control VTs (virtual terminals) on the Linux console
Serial: 0

# due to the same files before pkg split:
Conflicts: kbd < 0:1.12-alt1
Conflicts: console-tools < 0:0.2.3-ipl30mdk

%description -n console-vt-tools
console-vt-tools perform simple control operations on the VTs on Linux console
(like switching between them). 

Usually, several VTs are used ontop of the Linux console.
These scripts are useful for writing scripts that need to control them.

%package -n console-scripts
Group: System/Configuration/Other
Summary: Console configuration activation and management
BuildArch: noarch

Requires: %name %name-data
Obsoletes: console-common-scripts
Provides: console-common-scripts = %version

Conflicts: startup < 0.9.7-alt1
Conflicts: console-tools < 0.2.3-ipl29mdk

%description -n console-scripts
This package is required if you have an interactive system with console.
The package is dedicated to both system-wide and per-user Linux console/other VT 
configuration.

It is responsible for the ways the console configuration is managed, 
stored and used (activated), either at system-boot time 
or user session startup. 

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
BuildArch: noarch

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
%patch100 -p1 -b .unicode_start_vs_setfont

%build
%autoreconf -I m4
%configure \
	--bindir=/bin \
	--datadir=/lib/%name \
	--mandir=%_mandir \
	--enable-nls \
	--enable-optional-progs \
	#

%make_build

%install
%makeinstall \
	bindir="%buildroot/bin" \
	datadir="%buildroot/lib/%name" \
	localedir="%buildroot/%_datadir/locale" \
	gnulocaledir="%buildroot/%_datadir/locale" \
	#

# Backward compatibility link
mkdir -p -- %buildroot/%_bindir %buildroot/%_libdir
ln -s -- /lib/%name %buildroot/%_libdir/%name
for binary in setfont dumpkeys kbd_mode unicode_start unicode_stop chvt openvt deallocvt fgconsole; do
	t=$(relative /bin/$binary %_bindir/$binary)
	ln -s $t %buildroot/%_bindir/$binary
done

# Set up kbdrate to be userhelpered.
mkdir -p %buildroot/sbin \
	%buildroot/%_sysconfdir/security/console.apps \
	%buildroot/%_sysconfdir/pam.d

install -p -m640 rpm/util-linux-2.9w-kbdrate.pamd %buildroot/%_sysconfdir/pam.d/kbdrate
install -p -m640 rpm/util-linux-2.9w-kbdrate.apps %buildroot/%_sysconfdir/security/console.apps/kbdrate

mv %buildroot/bin/kbdrate %buildroot/sbin/
ln -s -- %_usr/lib/consolehelper/helper %buildroot/bin/kbdrate

mkdir -p \
	%buildroot/%_initdir \
	%buildroot/%_datadir/console-scripts \
	%buildroot/%_sysconfdir/profile.d

install -p -m755 rpm/console-scripts/setsys* %buildroot/sbin/
install -p -m755 rpm/console-scripts/{keytable,consolesaver} %buildroot/%_initdir/

for f in configure_keyboard vt_activate_*; do
	install -p -m755 rpm/console-scripts/$f %buildroot/%_datadir/console-scripts/
done

for f in console.*sh configure_keyboard.*sh; do
	install -p -m644 rpm/console-scripts/$f  %buildroot/%_sysconfdir/profile.d/
done

%find_lang %name

# Compatibility forever!
cp -ar -- rpm/kbd-data-extra/* %buildroot/lib/%name/

old_path="$(pwd)"
cd %buildroot/lib/%name/keymaps
	rm -rf -- amiga atari sun
%ifnarch ppc
	rm -rf -- mac ppc
%endif
cd %buildroot/lib/%name/keymaps/i386/include
	rm -f euro1.inc windowkeys.map*

	gzip -9 *.inc
	ln -s windowkeys-compose.inc.gz windowkeys.map.gz
	for f in *.map.gz; do
		fn="${f%%.map.gz}"
		ln -s "$f" "$fn.inc.gz"
	done
cd %buildroot/lib/%name/keymaps/i386/qwerty
	gzip -9 *.map

	if [ -f by-cp1251.kmap ]; then
		mv by-cp1251.kmap by-cp1251.map
		gzip -9 by-cp1251.map
	fi

	ln -s ua.map.gz  ua-KOI8-U.map.gz
	ln -s ua.map.gz  ua-KOI8-R.map.gz # it has also a Russian KOI8-R layout
	ln -s ua-cp1251.map.gz  ua-CP1251.map.gz
	ln -s by-cp1251.map.gz  by-CP1251.map.gz
	ln -s ru_alt-KOI8-R.map.gz ru_alt.map.gz
	ln -s ru_cplk-KOI8-R.map.gz ru_cplk.map.gz
	ln -s ru_ct_sh-KOI8-R.map.gz ru_ct_sh.map.gz
	ln -s ru_ctrl-KOI8-R.map.gz ru_ctrl.map.gz
        ln -s ruwin_alt-KOI8-R.map.gz ruwin_alt.map.gz
	ln -s ruwin_cplk-KOI8-R.map.gz ruwin_cplk.map.gz
        ln -s ruwin_ct_sh-KOI8-R.map.gz ruwin_ct_sh.map.gz
	ln -s ruwin_ctrl-KOI8-R.map.gz ruwin_ctrl.map.gz
cd "$old_path"

# Config files:
mkdir -p -- %buildroot/%_sysconfdir/sysconfig/console
cd %buildroot/%_sysconfdir/sysconfig
touch consolefont keyboard console/setterm

# Set default font
echo 'SYSFONT=UniCyr_8x16' > consolefont

%triggerpostun -n %name-data -- console-data
[ $2 = 0 ] || exit 0
[ ! -d '%_libdir/%name' ] ||
	rm -rf -- '%_libdir/%name'
[ -e '%_libdir/%name' ] ||
	ln -s -- '/lib/%name' '%_libdir/%name'

%triggerpostun -n console-scripts -- console-common-scripts <= 0.2.2-alt1.4, console-scripts < 0:1.13.99-alt4
/sbin/chkconfig keytable on
/sbin/chkconfig consolesaver on
	
%post -n console-scripts
%post_service keytable
%post_service consolesaver
cd %_sysconfdir/sysconfig
for f in consolefont keyboard console/setterm; do
	[ ! -s "$f" ] || continue
	if [ -f "$f".rpmsave ]; then
		cp -pfv "$f".rpmsave "$f"
	elif [ -f "$f".rpmnew ]; then
		cp -pfv "$f".rpmnew "$f"
	fi
done

%preun -n console-scripts
%preun_service keytable
%preun_service consolesaver

%files -f %name.lang
/bin/*
/sbin/*
%_bindir/*
%_mandir/*/*
%exclude /bin/chvt
%exclude /bin/openvt
%exclude /bin/deallocvt
%exclude /bin/fgconsole
%exclude /bin/kbdrate
%exclude /sbin/kbdrate
%exclude /sbin/setsysfont
%exclude /sbin/setsyskeytable
%exclude %_bindir/chvt
%exclude %_bindir/openvt
%exclude %_bindir/deallocvt
%exclude %_bindir/fgconsole
%exclude %_man1dir/chvt*
%exclude %_man1dir/openvt*
%exclude %_man1dir/deallocvt*
%exclude %_man1dir/fgconsole*
%exclude %_man8dir/kbdrate.*

%files -n %name-data
/lib/%name
%ghost %_libdir/%name

%files -n %name-docs
%doc ChangeLog CREDITS README doc/*.txt doc/kbd.FAQ*.html doc/font-formats/*.html doc/utf

%files -n console-vt-tools
/bin/chvt
/bin/openvt
/bin/deallocvt
/bin/fgconsole
%_bindir/chvt
%_bindir/openvt
%_bindir/deallocvt
%_bindir/fgconsole
%_man1dir/chvt*
%_man1dir/openvt*
%_man1dir/deallocvt*
%_man1dir/fgconsole*

%files -n console-scripts
/sbin/setsysfont
/sbin/setsyskeytable
%attr(755,root,root) %config %_initdir/*
%attr(755,root,root) %config %_sysconfdir/profile.d/*.sh
%attr(755,root,root) %config %_sysconfdir/profile.d/*.csh
%_datadir/console-scripts
%dir %_sysconfdir/sysconfig/console
%config(noreplace) %verify(not md5 mtime size) %_sysconfdir/sysconfig/consolefont
%config(noreplace) %verify(not md5 mtime size) %_sysconfdir/sysconfig/keyboard
%config(noreplace) %verify(not md5 mtime size) %_sysconfdir/sysconfig/console/setterm

# as in ALT util-linux:
%files -n kbdrate
/sbin/kbdrate
%_man8dir/kbdrate.*

%files -n kbdrate-usermode
%config(noreplace) %_sysconfdir/pam.d/kbdrate
%config(noreplace) %_sysconfdir/security/console.apps/kbdrate
/bin/kbdrate

%changelog
* Mon May 02 2011 Alexey Gladkov <legion@altlinux.ru> 0:1.15.3-alt0.20110502
- New snapshot.
- openvt: Fix -v option.

* Wed Apr 27 2011 Alexey Gladkov <legion@altlinux.ru> 0:1.15.3-alt0.20110427
- New snapshot (1.15.3pre).

* Mon Mar 14 2011 Alexey Gladkov <legion@altlinux.ru> 0:1.15.2-alt2
- Change default console font to UniCyr_8x16 (ALT#25225).

* Wed Apr 07 2010 Alexey Gladkov <legion@altlinux.ru> 0:1.15.2-alt1
- New release version (1.15.2).
- Use automake to build translations.
- Fix colemak installation.
- psffontop: Fix possible alignment issues, wrt -Wcast-align.
- vcstime: Fix build warning.
- loadkeys -u: Switch to Unicode mode, if necessary (Michael Schutte).
- Use either /dev/vcs[a] or /dev/vcs[a]0 (Michael Schutte)
- Keymaps:
  + Add "mobii" specific keymap (Richard Zidlicky).
- Fonts:
  + Add georgian font.

* Fri Oct 09 2009 Alexey Gladkov <legion@altlinux.ru> 0:1.15.1-alt1
- New release version (1.15.1).
- Add default font (ALT#17171).
- loadkeys: Auto-convert "traditional"/Unicode keysyms.
- loadkeys: Support bidirectional conversion of keysyms.
- loadkeys: Support Unicode compose tables.
- showconsolefont: Print adequate space chars.
- dumpkeys: Use U+... in "compose" lines if KDGKBDIACRUC is available.
- Add support for Brl_dot9 and Brl_dot10.
- Never handle plain ASCII characters as Unicode.
- Enable UNUMBERs in compose definitions.
- Keymaps:
  + bg_pho-utf8 keycode 38 assignment fix.
  + Add keymap for Colemak.
  + Add keymap for German Intel based Macs.
  + Add UK keymap for the Sun Type-6 keyboard.
  + Add French keymap Dvorak.
  + Add Kirghiz keymap.
  + Add Tajik keymap.

* Sun Mar 22 2009 Alexey Gladkov <legion@altlinux.ru> 0:1.15-alt1
- New release version (1.15).
- Fix build warnings.
- Add warning for on U+xxxx keysym specifications >= 0xf000.
- Update 8859-7_to_uni.trans.
- Keymaps:
  + bg_pho-utf8 keycode 38 assignment fix.
  + be-latin1 keycode 7 assignment fix (thx Herton Ronaldo Krzesinski).
- Fonts:
  + iso07u-16.psfu: Update font (thx Lefteris Dimitroulakis).
  + UniCyrExt_8x16.psf, lat1-16.psfu: Add U+2010, U+2012, U+2013, U+2018, 
    U+2019, U+2212 to the embedded character table.
- Use "BuildArch: noarch" for console-scripts and
  kbdrate-usermode (thx Dmitry V. Levin).
- console-scripts/console.{csh,sh}: Do nothing if $DISPLAY is
  set (thx Dmitry V. Levin).

* Fri Oct 24 2008 Alexey Gladkov <legion@altlinux.ru> 0:1.14.1.20081023-alt1
- New release version (1.14.1).
- Print an error message when calls to exec* functions fail (thx Michael Schutte).
- openvt: Document the -f switch (thx Michael Schutte).
- loadkeys: dump binary keymap (thx Michel Stempin).
- Keymaps:
  + Add turkish F (trf) keyboard map.
  + Add qwertz/cz.map keymap.
  + Add Norwegian dvorak keymap.
  + Fix turkish Q (trq) keymap (thx Ozgur Murat Homurlu).
  + Understand the CapsShift modifiers (thx Michael Schutte).
  + ruwin_*: Use qwerty-layout.inc.
- vt_activate_user_map: Allow absolute path in SYSFONTACM variable.
- unicode_{start,stop}: To run loadkeys is allowed only to root.
- unicode_start: Without any arguments utility will only set
  unicode mode.

* Fri Jan 18 2008 Alexey Gladkov <legion@altlinux.ru> 0:1.13.99-alt5
- Compatibility:
  + Fix windowskeys;
  + Add DISABLE_WINDOWS_KEY handle;
  + Add mode keymaps from console-tools.
- Update rpm triggers.

* Tue Jan 15 2008 Alexey Gladkov <legion@altlinux.ru> 0:1.13.99-alt4
- Fix for kernel-2.6.24 and newer. Since 2.6.24-rc1 the console by
  default in UTF-8 mode.

* Sat Jan 12 2008 Alexey Gladkov <legion@altlinux.ru> 0:1.13.99-alt3
- Add OLPC keymaps.
- Fix kbdrate-usermode for x86_64.
- Fix postinstall to load previous configs.

* Fri Jan 11 2008 Alexey Gladkov <legion@altlinux.ru> 0:1.13.99-alt2
- New console-scripts obsolete console-common-scripts.
- Improve compatibility with console-data.

* Thu Jan 03 2008 Alexey Gladkov <legion@altlinux.ru> 0:1.13.99-alt1
- New prerelease version (1.13.99).
- Add new subpackage: kbd-data.
- Add '.acm' suffix for compatibility with console-tools.
- Add Terminus font.
- Add unicode cyrillic fonts.
- Add more romanian keymaps.
- Add another ukrainian keymap.
- Add Belarusian (Belarus) keymaps.
- Add Kazakh keymap.
- Add Kyrgyz keymap.
- Add Bashkir (Russia) keymap.
- Add Tatar keymaps.
- Add more russian keymaps.
- Migrate to autoconf.
- Apply patches from other linux vendors.
- Fix some build warnings.
- /usr/lib/kbd moved to /lib/kbd.

* Mon May 21 2007 Alexey Gladkov <legion@altlinux.ru> 0:1.12.1-alt2
- Fix openvt: set the session controlling terminal.

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
