# Modifier-aware CSI variant of function keys F1-F5.
#
# These keys produce CSI sequences (\e[11~ through \e[15~). Standard Linux
# console terminfo (TERM=linux) expects legacy sequences (\e[[A through \e[[E)
# for F1-F5, so these keysyms require an updated terminfo entry (for example,
# installed by kbd-terminfo-fixup).
#
# Selected via include-if feature (KT_CSI_TERMINFO).
#
plain   keycode  59 = Csi_F1
shift   keycode  59 = Csi_F1
control keycode  59 = Csi_F1
plain   keycode  60 = Csi_F2
shift   keycode  60 = Csi_F2
control keycode  60 = Csi_F2
plain   keycode  61 = Csi_F3
shift   keycode  61 = Csi_F3
control keycode  61 = Csi_F3
plain   keycode  62 = Csi_F4
shift   keycode  62 = Csi_F4
control keycode  62 = Csi_F4
plain   keycode  63 = Csi_F5
shift   keycode  63 = Csi_F5
control keycode  63 = Csi_F5
