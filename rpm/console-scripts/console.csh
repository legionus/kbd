if ($?DISPLAY == 0) then
	if ( -x /usr/share/console-scripts/vt_activate_user_map ) then
	        /usr/share/console-scripts/vt_activate_user_map ||:
	endif

	if ( -x /usr/share/console-scripts/vt_activate_unicode ) then
	        /usr/share/console-scripts/vt_activate_unicode ||:
	endif
endif
