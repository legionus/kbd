if [ -z "${DISPLAY-}" ]; then
	if [ -x /usr/share/console-scripts/vt_activate_user_map ]; then
		/usr/share/console-scripts/vt_activate_user_map ||:
	fi

	if [ -x /usr/share/console-scripts/vt_activate_unicode ]; then
		/usr/share/console-scripts/vt_activate_unicode ||:
	fi
fi
