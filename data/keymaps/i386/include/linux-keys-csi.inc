# Modifier-aware CSI variant of the function and navigation keys.
#
# This is an overlay for the keys defined in linux-keys-bare.inc.  It
# rebinds the function keys (F6-F12) and the navigation keys (Home, End,
# Insert, Delete, PageUp, PageDown) to the Csi_* keysyms, which produce
# standard CSI sequences and let the kernel encode the canonical modifier
# state (Shift, Control, Alt) into the emitted sequence.
#
# These keysyms are fully compatible with standard Linux console terminfo
# (TERM=linux). For F1-F5, see linux-keys-csi-terminfo.inc.
#
# This requires a kernel console with KT_CSI support.  The default include
# files select this overlay with include-if, so kernels without KT_CSI keep
# the legacy linux-keys-bare bindings.
#
# Designed to be loaded as an overlay on top of an already loaded keymap:
# it deliberately has no "keymaps" line so that loadkeys merges it rather
# than clearing unrelated tables.

#
# Function keys F6-F12.
#
# The plain entry selects the CSI sequence; the kernel adds the canonical
# modifier parameter at runtime. The shift and control entries are set
# explicitly to override the legacy F18-F36 bindings from linux-keys-bare.inc
# while Alt and Control+Alt are left untouched so that console switching keeps
# working.
#
plain   keycode  64 = Csi_F6
shift   keycode  64 = Csi_F6
control keycode  64 = Csi_F6
plain   keycode  65 = Csi_F7
shift   keycode  65 = Csi_F7
control keycode  65 = Csi_F7
plain   keycode  66 = Csi_F8
shift   keycode  66 = Csi_F8
control keycode  66 = Csi_F8
plain   keycode  67 = Csi_F9
shift   keycode  67 = Csi_F9
control keycode  67 = Csi_F9
plain   keycode  68 = Csi_F10
shift   keycode  68 = Csi_F10
control keycode  68 = Csi_F10
plain   keycode  87 = Csi_F11
shift   keycode  87 = Csi_F11
control keycode  87 = Csi_F11
plain   keycode  88 = Csi_F12
shift   keycode  88 = Csi_F12
control keycode  88 = Csi_F12

#
# Navigation keys.
#
# A single plain entry is enough: the kernel falls back to it for any held
# canonical modifier and encodes that modifier into the sequence. Shift+PageUp
# and Shift+PageDown are left as the console scroll actions defined in
# linux-keys-bare.inc.
#
plain keycode 110 = Csi_Insert
plain keycode 102 = Csi_Home
plain keycode 104 = Csi_PgUp
plain keycode 111 = Csi_Delete
plain keycode 107 = Csi_End
plain keycode 109 = Csi_PgDn

include-if feature (KT_CSI_TERMINFO) "linux-keys-csi-terminfo"
