# Run only in interactive sessions
if [ -z "${-##*i*}" ] && tty --silent &&
    [ -x /usr/share/console-scripts/configure_keyboard ]; then
	/usr/share/console-scripts/configure_keyboard ||:
fi
