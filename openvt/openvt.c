/*
 *       openvt.c open a vt to run a new command (or shell).
 *       
 *	 Copyright (c) 1994 by Jon Tombs <jon@gtex02.us.es>
 *
 *       This program is free software; you can redistribute it and/or
 *       modify it under the terms of the GNU General Public License
 *       as published by the Free Software Foundation; either version
 *       2 of the License, or (at your option) any later version.
 */

/*
 * Added the not-in-use check, aeb@cwi.nl, 940924.
 *
 *   [Accidentally starting a process on a VT that is in use
 *    yields unfortunate effects: two processes reading the keyboard.
 *    This can be a disaster if the old process was in scancode mode.]
 *
 * Added the -u (`as user') stuff for use from inittab,
 *  Joshua Spoerri <josh@cooper.edu>, 1996-07-18
 *
 * Fixed some bugs; made it a bit more robust; renamed to openvt
 *  aeb@cwi.nl, 1998-06-06.
 */

#include "openvt.h"

const char *version = "openvt 1.4a - (c) Jon Tombs 1994";

#ifndef VTNAME
#error vt device name must be defined in openvt.h
#endif


int 
main(int argc, char *argv[])
{

   int opt, pid;
   struct vt_stat vtstat;
   int vtno     = -1;
   int fd       = -1;
   char optc	= FALSE;
   char show    = FALSE;
   char login   = FALSE;
   char force   = FALSE;
   char verbose = FALSE;
   char do_wait	= FALSE;
   char as_user = FALSE;
   char vtname[sizeof VTNAME + 2]; /* allow 999 possible VTs */
   char *cmd = NULL, *def_cmd = NULL, *username = NULL;

   /*
    * I don't like using getopt for this, but otherwise this gets messy.
    * POSIX/Gnu getopt forces the use of -- to separate child/program
    * options. RTFM.
    */
   while ((opt = getopt(argc, argv, "c:lsvfuw")) != -1) {
      switch (opt) {
	case 'c':
	  optc = 1;		/* vtno was specified by the user */
	  vtno = (int) atol(optarg);
	  if (vtno <= 0 || vtno > 63) {	  
	    fprintf(stderr, "openvt: %s illegal vt number\n", optarg); 
	    return 5;
	  }
	  /* close security holes - until we can do this safely */
	  (void) setuid(getuid());
	  break;
	case 'l':
	  login = TRUE;
	  break;
	case 's':
	  show = TRUE;
	  break;
	case 'v':
	  verbose = TRUE;
	  break;
        case 'f':
	  force = TRUE;
	  break;
	case 'w':
	  do_wait = TRUE;
	  break;
	case 'u':
          /* we'll let 'em get away with the meaningless -ul combo */
          if(getuid()) {
		fprintf(stderr,"%s: only root can use the -u flag.\n",argv[0]);
		exit(1);
          }
	  as_user = TRUE;
	  break;
	default:
	  usage(1);
	
      }
   }

   fd = getfd();
   if (fd < 0) {
      fprintf(stderr,
	      "Couldnt get a file descriptor referring to the console\n");
      return(2);
   }

   if (ioctl(fd, VT_GETSTATE, &vtstat) < 0) {
	perror("openvt: can't get VTstate\n");
        return(4);
   }

   if (vtno == -1) {
     if ((ioctl(fd, VT_OPENQRY, &vtno) < 0) || (vtno == -1)) {
        perror("openvt: Cannot find a free VT\n");
        return(3);
     }
   } else if (!force) {
     if (vtno >= 16) {
        fprintf(stderr, "openvt: cannot check whether vt %d is free\n", vtno);
	fprintf(stderr, "        use `openvt -f' to force.\n");
	return(7);
     }
     if (vtstat.v_state & (1 << vtno)) {
        fprintf(stderr, "openvt: vt %d is in use; command aborted\n", vtno);
	fprintf(stderr, "        use `openvt -f' to force.\n");
	return(7);
     }
   }

   sprintf(vtname, VTNAME, vtno);

   /* Can we open the vt we want? */
   if ((fd = open(vtname, O_RDWR)) == -1) {
      int errsv = errno;
      if (!optc) {
	      /* We found vtno ourselves - it is free according
		 to the kernel, but we cannot open it. Maybe X
		 used it and did a chown.  Try a few vt's more
		 before giving up. Note: the 16 is a kernel limitation. */
	      int i;
	      for (i=vtno+1; i<16; i++) {
		      if((vtstat.v_state & (1<<i)) == 0) {
			      sprintf(vtname, VTNAME, i);
			      if ((fd = open(vtname, O_RDWR)) >= 0) {
				      vtno = i;
				      goto got_vtno;
			      }
		      }
	      }
	      sprintf(vtname, VTNAME, vtno);
      }
      fprintf(stderr, "openvt: Unable to open %s: %s\n",
	      vtname, strerror(errsv));
      return(5);
   }
got_vtno:
   close(fd);

   /* Maybe we are suid root, and the -c option was given.
      Check that the real user can access this VT.
      We assume getty has made any in use VT non accessable */
   if (access(vtname, R_OK | W_OK) < 0) {
      fprintf(stderr, "openvt: Cannot open %s read/write (%s)\n", vtname,
	     strerror(errno));
      return (5);
   }

   if (as_user)
	   username = authenticate_user(vtstat.v_active);
   else {
	   if (!geteuid()) {
		   uid_t uid = getuid();
		   chown(vtname, uid, getgid());
		   setuid(uid);
	   }

	   if (!(argc > optind)) {
		   def_cmd = getenv("SHELL");
		   if (def_cmd == NULL)
			   usage(0);
		   cmd = malloc(strlen(def_cmd + 2));
	   } else {
		   cmd = malloc(strlen(argv[optind] + 2));
	   }

	   if (login)
		   strcpy(cmd, "-");
	   else
		   cmd[0] = '\0';

	   if (def_cmd)
		   strcat(cmd, def_cmd);
	   else
		   strcat(cmd, argv[optind]);

	   if (login) 
		   argv[optind] = cmd++;
   }

   if (verbose)
	fprintf(stderr,	"openvt: using VT %s\n", vtname);
	
   fflush(stderr);

   if((pid = fork()) == 0) {
      /* leave current vt */
#ifdef   ESIX_5_3_2_D
      if (setpgrp() < 0) {
#else
      if (setsid() < 0) {
#endif      
        fprintf(stderr, "openvt: Unable to set new session (%s)\n",
	strerror(errno));
      }
      close(0);			/* so that new vt becomes stdin */

      /* and grab new one */
      if ((fd = open(vtname, O_RDWR)) == -1) { /* strange ... */
	fprintf(stderr, "\nopenvt: could not open %s R/W (%s)\n",
		vtname, strerror(errno));
	fflush(stderr);
        _exit (1);		/* maybe above user limit? */
      }

      if (show) {
         if (ioctl(fd, VT_ACTIVATE, vtno)) {
	    fprintf(stderr, "\nopenvt: could not activate vt %d (%s)\n",
		    vtno, strerror(errno));
	    fflush(stderr);
	    _exit (1); /* probably fd does not refer to a tty device file */
	 }

	 if (ioctl(fd, VT_WAITACTIVE, vtno)){
	    fprintf(stderr, "\nopenvt: activation interrupted? (%s)\n",
		    strerror(errno));
	    fflush(stderr);
	    _exit (1);
	 }
      }
      close(1);
      close(2);
      if (fd > 2)
	 close(fd);	 
      dup(fd);
      dup(fd);

      if(as_user)
	 execlp("login", "login", "-f", username, NULL);
      else if (def_cmd)
         execlp(cmd, def_cmd, NULL);
      else
	 execvp(cmd, &argv[optind]);
      _exit(127);		/* exec failed */
   }

   if ( pid < 0 ) {
      perror("openvt: fork() error");
      return(6);
   }

   if ( do_wait ) {
      wait(NULL);
      if (show) { /* Switch back... */
	 (void) ioctl(fd, VT_ACTIVATE, vtstat.v_active);
	 /* wait to be really sure we have switched */
	 (void) ioctl(fd, VT_WAITACTIVE, vtstat.v_active);
      }
   }

   return 0;
}


void usage(int stat)
{
   fprintf(stderr,
     "Usage: openvt [-c vtnumber] [-l] [-u] [-s] [-v] [-w] -- command_line\n");
   exit (stat);
}

/*
 * Support for Spawn_Console: openvt running from init
 * added by Joshua Spoerri, Thu Jul 18 21:13:16 EDT 1996
 *
 *  -u     Figure  out  the  owner  of the current VT, and run
 *         login as that user.  Suitable to be called by init.
 *         Shouldn't be used with -c or -l.
 *         Sample inittab line:
 *                kb::kbrequest:/usr/bin/openvt -us
 *
 * It is the job of authenticate_user() to find out who
 * produced this keyboard signal.  It is called only as root.
 *
 * Note that there is a race condition: curvt may not be the vt
 * from which the keyboard signal was produced.
 * (Possibly the signal was not produced at the keyboard at all,
 * but by a "kill -SIG 1".  However, only root can do this.)
 *
 * Conclusion: do not use this in high security environments.
 * Or fix the code below to be more suspicious.
 *
 * Maybe it is better to just start a login at the new vt,
 * instead of pre-authenticating the user with "login -f".
 */

char *
authenticate_user(int curvt) {
	DIR *dp;
	struct dirent *dentp;
	struct stat buf;
	dev_t console_dev;
	ino_t console_ino;
	uid_t console_uid;
	char filename[NAME_MAX+12];
	struct passwd *pwnam;

	if (!(dp=opendir("/proc"))) {
		perror("/proc");
		exit(1);
	}
	
	/* get the current tty */
	sprintf(filename,"/dev/tty%d", curvt);
	if (stat(filename,&buf)) {
		perror(filename);
		exit(1);
	}
	console_dev=buf.st_dev;
	console_ino=buf.st_ino;
	console_uid=buf.st_uid;

	/* get the owner of current tty */
	if (!(pwnam = getpwuid(console_uid))) {
		perror("can't getpwuid");
		exit(1);
	}

	/* check to make sure that user has a process on that tty */
	/* this will fail for example when X is running on the tty */
	while ((dentp=readdir(dp))) {
		sprintf(filename,"/proc/%s/fd/0",dentp->d_name);
		if (stat(filename,&buf))
			continue;
		if(buf.st_dev == console_dev && buf.st_ino == console_ino
				&& buf.st_uid == console_uid)
			goto got_a_process;
	}

	fprintf(stderr,"couldn't find owner of current tty!\n");
	exit(1);

   got_a_process:
	return pwnam->pw_name;
}
