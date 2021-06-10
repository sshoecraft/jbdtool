#ifndef __WIN32
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "debug.h"

int become_daemon(void) {
	pid_t pid;

	/* Fork the process */
	DLOG(LOG_DEBUG,"become_daemon: forking process...");
	pid = fork();

	/* If pid < 0, error */
	if (pid < 0) {
		DLOG(LOG_DEBUG|LOG_SYSERR,"become_daemon: fork");
		return 1;
	}

	/* If pid > 0, parent */
	else if (pid > 0) {
		DLOG(LOG_DEBUG,"become_daemon: parent exiting...");
		_exit(0);
	}

	/* Set the session ID */
	setsid();

	/* Fork again */
	pid = fork();
	if (pid < 0)
		return 1;
	else if (pid != 0) {
		DLOG(LOG_DEBUG,"become_daemon(2): parent exiting...");
		_exit(0);
	}

	/* Set the umask */
	umask(022);

	/* Close stdin,stdout,stderr */
	close(0);
	close(1);
	close(2);
	return 0;
}
#endif
