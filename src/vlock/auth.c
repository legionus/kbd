
/*

  PAM authentication routine for vlock, the VT locking program for linux.

  Copyright (C) 1994-1998 Michael K. Johnson <johnsonm@redhat.com>
  Copyright (C) 2002, 2005 Dmitry V. Levin <ldv@altlinux.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>

#include "pam_auth.h"
#include "vlock.h"
#include "nls.h"

/* Unrecognized PAM error timeout. */
#define	ERROR_TIMEOUT	10

int
get_password (pam_handle_t * pamh, const char *username, const char *tty)
{
	uid_t   uid = getuid ();

	for (;;)
	{
		int     rc;
		const char *msg;

		if (!pamh)
		{
			pamh = init_pam (username, tty, 1);
			if (!pamh)
			{
				/* Log the fact of failure. */
				syslog (LOG_WARNING,
					"Authentication problems on %s for %s by (uid=%u)",
					tty, username, uid);
				puts (_("Please try again later.\n\n\n"));
				fflush (stdout);
				sleep (ERROR_TIMEOUT);
				continue;
			}
		}

		if (o_lock_all)
		{
			printf (_("The entire console display is now completely locked by %s.\n"),
				username);
		} else
		{
			printf (_("The %s is now locked by %s.\n"), tty,
				username);
			if (is_vt)
				puts (_("Use Alt-function keys to switch to other virtual consoles."));
		}
		fflush (stdout);

		/*
		 * No need to request a delay on failure via pam_fail_delay(3):
		 * authentication module should do it for us.
		 */
		rc = pam_authenticate (pamh, 0);

		switch (rc)
		{
			case PAM_SUCCESS:
				pam_end (pamh, rc);
				/* Log the fact of console unlocking. */
				syslog (LOG_NOTICE,
					"Unlocked %s on %s for %s by (uid=%u)",
					locked_name (), tty, username, uid);
				return EXIT_SUCCESS;

			case PAM_INCOMPLETE:
				/*
				 * EOF encountered on read?
				 * If not on VT, check stdin.
				 */
				if (is_vt || isatty (STDIN_FILENO))
				{
					/* Ignore error. */
					sleep (1);
					break;
				}

				/* Cancel locking. */
				pam_end (pamh, rc);
				syslog (LOG_NOTICE,
					"Cancelled lock of %s on %s for %s by (uid=%u)",
					locked_name (), tty, username, uid);
				return EXIT_FAILURE;

			case PAM_MAXTRIES:
			case PAM_ABORT:
				msg = pam_strerror (pamh, rc);
				/* Log the fact of failure. */
				syslog (LOG_WARNING, "%s", msg);
				printf ("%s.\n\n\n", msg);
				fflush (stdout);
				msg = 0;
				pam_end (pamh, rc);
				pamh = 0;
				sleep (ERROR_TIMEOUT);
				break;

			default:
				printf ("%s.\n\n\n", pam_strerror (pamh, rc));
				fflush (stdout);
		}
	}
}
