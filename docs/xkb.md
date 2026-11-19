# XKB layout conversion for the Linux console

kbd manages the Linux virtual console. It does not configure keyboard
layouts for X11 or Wayland sessions.

When kbd is built with XKB support, `loadkeys` can use XKB layout
descriptions as input and convert them into Linux kernel console keymaps.
This is useful when an XKB layout exists but no equivalent kbd keymap is
available under the keymap data directory.

## Build requirements

XKB conversion is available when kbd is built with libxkbcommon support.
The configure option is:

```sh
./configure --enable-xkb
```

The default configure mode is `auto`: XKB support is enabled when
libxkbcommon is found and disabled otherwise.

## Basic usage

Load an XKB layout into the Linux virtual console:

```sh
loadkeys --xkb-layout us
```

Use a model, layout variant, or XKB options:

```sh
loadkeys --xkb-model pc105 --xkb-layout us --xkb-variant intl
loadkeys --xkb-layout us,ru --xkb-options grp:caps_toggle
```

Check that a layout can be converted without loading it:

```sh
loadkeys --parse --xkb-layout de
```

Write the converted keymap in kbd text format:

```sh
loadkeys --tkeymap --xkb-layout us --xkb-variant intl > us-intl.map
```

Select the locale used to look up XKB compose data:

```sh
loadkeys --xkb-layout us --xkb-variant intl --xkb-locale en_US.UTF-8
```

If `--xkb-locale` is not specified, `loadkeys` tries `LC_ALL`,
`LC_CTYPE`, `LANG`, and then `C`.

## What gets converted

The converter resolves the requested XKB model, layout, variant and
options through the standard `evdev` XKB rules. It then converts the
resulting symbols, modifier behavior, compose entries, keypad mappings
and virtual console switching mappings into the kernel keymap format
where the Linux console can represent them.

Not every XKB semantic has a direct Linux console equivalent. XKB is a
richer input model than the kernel console keymap interface, so conversion
necessarily preserves the behavior that can be expressed through the
Linux virtual console.

## Unsupported group switching options

`grp:shifts_toggle` is rejected with an error, including when it appears
in a comma-separated option list or is inherited from `XKB_DEFAULT_OPTIONS`.
The converter does not silently remove it.

This option gives each Shift key two roles: holding Shift and switching
groups when the other Shift is held. The Linux console looks up key
releases in the current modifier table, without remembering the action
used at key press. After switching groups, releasing Shift can therefore
select a group-switch action instead of releasing the modifier, leaving
Shift stuck. Use another switching option, such as `grp:caps_toggle`.

This check rejects the named option; it does not detect every custom XKB
configuration that assigns similar roles to modifier keys.

## Temporary group switching

`grp:switch` uses RightAlt to select the next group while the key is held.
For example:

```sh
loadkeys --xkb-layout us,ru --xkb-options grp:switch
```

The converter maps `Mode_switch` (also named `ISO_Group_Shift`) to the
console action `CtrlR`. This uses the independent `KG_CTRLR` bit; ordinary
left and right Control keys still use `Control` (`KG_CTRL`). The existing
`KG_SHIFTL` and `KG_SHIFTR` bits continue to select the locked group.

When a keymap contains `Mode_switch`, the converter adds tables with
`KG_CTRLR` set: tables 128–191 alongside tables 0–63, or tables 128–255 when
absolute or previous-group selection also requires tables 64–127. These
tables select the next XKB group, wrapping at the total number of groups.
Per-key layout fallback is still resolved by libxkbcommon. Releasing the
switch clears `KG_CTRLR` and returns to the group selected by the locked
group bits.

The console looks up key releases in the current table, without remembering
the action used at key press. In the extra tables, every key that can produce
`Mode_switch` therefore keeps the `CtrlR` action under all modifiers. This
allows RightAlt to be released after pressing Shift or changing the locked
group. In the ordinary tables, original assignments are preserved, including
`Shift+RightAlt = Compose` where XKB defines it. If the locked group changes
while RightAlt is held, releasing RightAlt returns to that newly selected
group.

This represents a single held group switch. Multiple `Mode_switch` keys
share the same bit: holding several does not advance several groups, and
while one is held, the others also act as group switches rather than their
alternate bindings. Arbitrary XKB actions attached to `Mode_switch` are not
interpreted; conversion assumes the usual `SetGroup(group=+1)` behavior.

The existing limitations of other group actions and of releasing an ordinary
modifier after its binding changes between groups still apply.

## Absolute group selection

`grp:shift_caps_switch` makes CapsLock select group 1 and Shift+CapsLock
select group 2. Repeating either combination leaves the same group selected;
it does not toggle Shift or CapsLock. With a held `grp:switch` key, this
changes the locked group, and the temporary next-group offset still applies.

The standard XKB interpretations of `ISO_First_Group` and `ISO_Last_Group`
are `LockGroup(group=1)` and `LockGroup(group=2)`. Despite its name,
`ISO_Last_Group` selects group 2 even in a keymap with three or four groups.
Custom interpretations of these keysyms are not evaluated by the converter.

Console lock actions can toggle only one bit at a time. With one or two
groups, the existing group bits suffice. With three or four groups, some
absolute selections require both group bits to change. The converter then
reserves `KG_CTRLL` as an additional locked bit: when set, it inverts both
`KG_SHIFTL` and `KG_SHIFTR` in the group index used to select the XKB layout.
Toggling `ShiftL_Lock`, `ShiftR_Lock`, or `CtrlL_Lock` can therefore reach any
group in a single action. Selecting the current group uses `VoidSymbol`.
Both physical Control keys continue to use the ordinary `KG_CTRL` bit.

The additional tables 64–127 are generated only for keymaps with more than
two groups and an absolute or previous-group selector. When combined with
temporary group switching, all 256 console tables are used. `KG_CTRLR`
retains its independent role as the held group-switch bit.

## Previous group selection

`ISO_Prev_Group` follows the standard XKB interpretation
`LockGroup(group=-1)`: it selects the previous locked group, wrapping from
the first group to the last. With one layout it produces `VoidSymbol`.
With three or four layouts it uses the additional `KG_CTRLL` tables
described above so that every backward transition takes one action.

When `grp:switch` is held, the temporary next-group offset remains applied;
releasing it reveals the newly selected locked group. Custom XKB actions
attached to `ISO_Prev_Group` are not interpreted. Supporting this keysym
does not remove the modifier-release limitation of `grp:shifts_toggle`.

## Level 5 modifiers

Eight-level XKB layouts such as `de(neo)` and `de(noted)` use
`ISO_Level5_Shift`, `ISO_Level5_Latch`, and `ISO_Level5_Lock` alongside
`ISO_Level3_Shift` (`AltGr`). The converter keeps `LevelThree` on `AltGr`
(`KG_ALTGR`) and assigns `LevelFive` to an unused console modifier bit:

- By default, `ISO_Level5_Shift` and `ISO_Level5_Latch` map to `CtrlL`
  (`KG_CTRLL`), and `ISO_Level5_Lock` maps to `CtrlL_Lock`. Ordinary left
  and right Control keys continue to use `Control` (`KG_CTRL`).
- When more than two layouts are combined with absolute or previous-group
  selection (`ISO_First_Group`, `ISO_Last_Group`, or `ISO_Prev_Group`),
  `KG_CTRLL` is reserved for single-step group locking; `LevelFive` then
  uses `CtrlR` (`KG_CTRLR`) and `CtrlR_Lock`.
- Combining `LevelFive` with more than two layouts, absolute or
  previous-group selection, and temporary group switching (`Mode_switch`)
  at the same time requires 9 modifier/group bits (512 tables), which
  exceeds the kernel limit of 256 keymap tables (`MAX_NR_KEYMAPS`) and is
  rejected with an error.

## Scope and limitations

- XKB conversion affects the Linux virtual console keymap only.
- It does not change X11, Wayland, desktop environment, or terminal
  emulator keyboard settings.
- Loading a console keymap changes the kernel keyboard translation table
  shared by all Linux virtual consoles.
- Unicode characters at `U+F000` and above cannot be represented as console
  key bindings and are omitted during conversion.
- Compose data may depend on the selected locale and installed XKB data.
- A converted keymap should be tested with `--parse` or `--tkeymap`
  before it is loaded on systems where console input is critical.

See also [`loadkeys(1)`](https://kbd-project.org/manpages/man1/loadkeys.1.html)
and [`keymaps(5)`](https://kbd-project.org/manpages/man5/keymaps.5.html).

See [XKB compose conversion](xkb-compose.md) for compose filtering,
priorities, conflict resolution, and kernel table limits.
