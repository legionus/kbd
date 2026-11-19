# XKB compose conversion for the Linux console

The converter in [src/xkbsupport.c](../src/xkbsupport.c) selects a subset of
an XKB compose table for the Linux virtual console. This document describes
that selection, including the cases where XKB behavior cannot be preserved.
For command-line usage, see [XKB layout conversion](xkb.md).

## What the kernel can represent

A kernel compose entry contains three character values: `diacr`, `base`,
and `result`. The kernel looks up the pair `(diacr, base)` and returns the
first matching result. It does not retain XKB keysyms, physical keycodes,
modifier states, or the original compose sequence.

The table is shared by the converted layouts. `MAX_DIACR` is currently 256,
and `KDSKBDIACR`/`KDSKBDIACRUC` accept up to `MAX_DIACR - 1` (255) entries;
it is not a separate table for each layout or modifier.
XKB sequences and output strings do not necessarily fit this representation.
Importing a compose table therefore does not reproduce the full XKB compose
state machine.

## Compose source and reachable symbols

`loadkeys` selects the locale from `--xkb-locale`, then the first nonempty
value of `LC_ALL`, `LC_CTYPE`, and `LANG`, falling back to `C`. The converter
calls `xkb_compose_table_new_from_locale()`. File lookup, including
`XCOMPOSEFILE` and user compose files, is delegated to libxkbcommon.

If no compose table can be created, conversion warns and continues without
importing compose rules. Failure to create the iterator or allocate storage
while importing an available table is an error instead.

Before importing compose rules, `xkeymap_walk()` records the XKB keysyms it
successfully resolves while generating console key bindings. It visits the
console groups and combinations of Shift, AltGr, Control, and Alt, using
XKB's per-key layout fallback. For a level with multiple keysyms, only the
first is used. The first recorded libkeymap code for a keysym is retained.

This recorded set is the meaning of "reachable" below. It is not every
keysym named in the XKB files, nor a simulation of arbitrary keyboard event
sequences. Codes are recorded before CapsLock tagging and the final
Control/Meta adjustments to a binding.

## Candidate filtering

`xkeymap_compose_candidate_from_entry()` rejects an entry when:

- Its sequence does not contain exactly two keysyms.
- Its result keysym is `NoSymbol`. A UTF-8 result string alone is not used;
  the converter does not extract a character from that string.
- Either input keysym is absent from the reachable set.
- An input contains a `KT_DEAD` index outside the known accent table.
- The result is not already reachable and `xkeymap_get_code()` cannot
  resolve it.

A three-key sequence such as `<Multi_key> <apostrophe> <a>` is not shortened
by removing `Multi_key`; it is rejected by the length check. An exact
two-key sequence need not start with a dead key to be considered.

A result does not have to be directly available on the keyboard. If it is
reachable, its recorded code is reused; otherwise it follows the normal
XKB semantic, name, Unicode, and hexadecimal lookup path. The importer does
not add a separate character-only validation step for the resolved result.

For named characters, an 8-bit lookup can borrow a byte from a different
charset. The converter checks whether that byte still denotes the named
character in the current charset. If it does not, it keeps the Unicode
name resolution instead. This prevents, for example, `ccaron` from becoming
`egrave` when an ISO-8859-2 byte is interpreted as Latin-1. Existing correct
byte representations and kernel action types are preserved.

The candidate filter does not issue a diagnostic for each rejected entry,
although symbol-conversion helpers can emit warnings. These rejections are
separate from dropping otherwise eligible rules because of conflicts or
the kernel table limit.

## Convert actions to compose characters

Key bindings and compose inputs use different representations. For example,
`dead_acute` is a key action (`K_DACUTE`, 0x401), but the kernel converts it
to an apostrophe (0x27) before looking up a compose rule. Storing 0x401 as
the compose accent would never match that action.

`xkeymap_compose_input_code()` converts both inputs:

- `KT_DEAD` uses the same accent-character mapping as Linux `k_dead()`.
- `KT_DEAD2` uses the action's byte value, expressed as a libkeymap Unicode
  code so later charset conversion preserves its character meaning.
- Other codes are retained for subsequent libkeymap conversion.

Some especially important mappings are:

| XKB keysym | Console symbol | Compose accent |
| --- | --- | --- |
| `dead_acute` | `dead_acute` | apostrophe, U+0027 |
| `dead_circumflex` | `dead_circumflex` | `^`, U+005E |
| `dead_caron` | `dead_kcaron` | `c`, U+0063 |
| `dead_breve` | `dead_kbreve` | `U`, U+0055 |
| `dead_doubleacute` | `dead_kdoubleacute` | `=`, U+003D |
| `dead_ogonek` | `dead_kogonek` | `k`, U+006B |

The four `dead_k*` names select distinct kernel actions. The unprefixed
libkeymap names are historical aliases for other accents, retained for old
console keymaps. XKB conversion explicitly bypasses those aliases.

For the complete mapping, see `xkeymap_compose_input_code()` and Linux
`drivers/tty/vt/keyboard.c`, especially `k_dead()`, `k_dead2()`, and
`handle_diacr()`.

## Candidate priorities

`xkeymap_score_compose_candidate()` adds the following weights, starting
from zero. Higher scores are selected first.

| Condition | Weight |
| --- | ---: |
| First keysym has a canonical name beginning with `dead_` | +1000 |
| Preferred dead-key/letter combination listed below | +300 |
| Base is an ASCII lowercase letter | +250 |
| Base is an ASCII uppercase letter | +200 |
| Base is an ASCII digit or space | -200 |
| Other nonzero base value | -50 |
| Result value is U+0001 through U+00FF | +200 |
| Result value is U+0100 through U+017F | +100 |
| Other nonzero result value | +25 |
| Result keysym is directly reachable | -100 |

The base categories are mutually exclusive, as are the result categories.
Base and result values come from the converted libkeymap codes through
`xkeymap_compose_code_to_unicode()`, not directly from the original XKB
keysyms. This helper decodes libkeymap Unicode values and uses the low byte
of `KT_LATIN`/`KT_LETTER`; it returns zero for other action types. It is not
a general charset-to-Unicode conversion.

If the second keysym is itself a dead key, its base value is treated as
zero for scoring, and it receives no preferred-letter bonus. For example,
the accent code `c` for `dead_caron` must not make a repeated dead key look
like the ordinary lowercase letter `c`.

The preferred letter sets apply in both uppercase and lowercase:

| First keysym | Letters |
| --- | --- |
| `dead_grave`, `dead_macron` | AEIOU |
| `dead_acute` | ACEILNORSUYZ |
| `dead_circumflex` | ACEGHIJOSUWY |
| `dead_tilde` | AINOU |
| `dead_diaeresis` | AEIOUY |
| `dead_abovering` | AU |
| `dead_cedilla` | ACEST |
| `dead_caron` | CDELNRSTZ |
| `dead_ogonek` | AE |
| `dead_breve` | AGU |
| `dead_doubleacute` | OU |
| `dead_abovedot` | CEGILZ |

These are selection heuristics favoring traditional console dead-key
combinations and a compact Latin repertoire. They are not additional
eligibility requirements: other scripts and combinations remain candidates.

Scores use signed arithmetic. A negative total sorts below zero and below
positive totals, both when choosing between results for the same XKB
sequence and when ordering candidates for the kernel table. Penalties do
not wrap around into high priorities.

## Resolve duplicates, then apply the size limit

There are two distinct comparisons:

1. `xkeymap_select_compose_candidates()` groups candidates by their original
   two XKB input keysyms. It keeps the highest score for each sequence;
   ties use the smaller numeric result keysym. The remaining candidates
   are sorted by descending score, then ascending numeric first input,
   second input, and result keysym.
2. `xkeymap_append_compose_candidates()` compares the final kernel input
   pair. Before comparing, it converts both inputs with `lk_convert_code()`
   to the encoding that `lk_append_compose()` will store: Unicode when
   requested and supported, otherwise 8-bit. The result is deliberately
   excluded from this comparison. The first candidate in priority order
   wins, even if a later candidate would produce a different result.

Comparing after encoding conversion also catches different libkeymap codes
that become the same stored character. Keeping multiple results for one
pair would waste slots: the kernel would only use the first one.

Each surviving pair is inserted through `lk_append_compose()`, which
converts all three fields. Only the first `MAX_DIACR - 1` unique pairs are
inserted. The converter continues counting unique pairs beyond the limit
and warns if the count exceeds it. The reported count is after filtering
and duplicate removal, not the number of entries in the original XKB table.
The limit is not apportioned between layouts or languages.

### Collisions that cannot preserve both XKB meanings

`dead_caron + c` and `dead_caron + dead_caron` both become `('c', 'c')`.
The kernel cannot tell which original sequence was typed. Scoring favors
the letter combination, so the retained rule produces `č`. Repeating the
dead key then produces that same result. The corresponding breve collision
is `('U', 'U')`, where the retained letter rule produces `Ŭ`.

This is a deliberate loss of one XKB meaning. Duplicate removal makes the
choice explicit; it cannot restore information lost when the inputs were
converted to kernel characters.

## Text dumps and regression checks

`lk_dump_diacs()` writes non-ASCII Unicode inputs as `U+XXXX`, for example:

```text
compose '\'' U+03b1 to alphaaccent
```

Printing U+03B1 with `%c` would emit only byte 0xB1. Even values in
U+0080..U+00FF need explicit Unicode notation to avoid interpretation under
a different charset when the dump is read. ASCII quoting and escaping, and
8-bit-mode output, retain their existing form.

Useful tests and implementation references:

- [libkeymap-test55.c](../tests/libkeymap/libkeymap-test55.c): sequence
  selection, input-pair conflicts after encoding conversion, dead-key
  actions, and repeated-dead-key priorities.
- `libkeymap-test56` through `libkeymap-test60`: converted XKB snapshots
  under [tests/data/xkb](../tests/data/xkb).
- [libkeymap-test61.c](../tests/libkeymap/libkeymap-test61.c): conversion
  without an available compose table.
- [libkeymap-test53.c](../tests/libkeymap/libkeymap-test53.c): dump/reparse
  checks, including numeric comparison of Unicode compose fields.
- [diacr.c](../src/libkeymap/diacr.c), [dump.c](../src/libkeymap/dump.c), and
  [parser.y](../src/libkeymap/parser.y): storage, serialization, and parsing.

Run the focused XKB tests with `make -C tests check CHECK_KEYWORDS=xkb`.
For changes to libkeymap encoding or dumping, use
`make -C tests check CHECK_KEYWORDS=libkeymap` as well. A matching snapshot
alone does not prove console behavior: check the numeric input pairs and
remember that the kernel chooses its first matching rule.
