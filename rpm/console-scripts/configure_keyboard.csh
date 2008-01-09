# Run only in interactive sessions
tty --silent
if ( $status == 0 && -x /usr/share/console-scripts/configure_keyboard ) then
	/usr/share/console-scripts/configure_keyboard ||:
endif
