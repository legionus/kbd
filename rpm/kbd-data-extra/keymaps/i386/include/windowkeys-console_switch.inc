# Differs from the original windowkeys.inc by assigning a console 
# switching function to the Menu key (instead of assigning Compose 
# function to it). Here, Menu swithes between the current and the last 
# console (the one you used before using the current).

# Suggested by Konstantin Volkov <goldhead@altlinux.ru>, who, in his 
# turn, has seen this in Black Cat distro.

keymaps 0-255 # to affect all groups

# Extra 105 windows keys

keycode 125 = Decr_Console
keycode 126 = Incr_Console
keycode 127 = Last_Console
