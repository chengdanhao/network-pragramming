#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <time.h>

void connect_alarm(int signo)
{
	printf("connect timeout.\n");
	return;
}

int connect_timeout(int sockfd, struct sockaddr* sa, socklen_t len, int nsec) {
	sighandler_t sigfunc;
	int n;

	sigfunc = signal(SIGALRM, connect_alarm);
	if (SIG_ERR == sigfunc) {
		perror("signal");
		exit(EXIT_FAILURE);
	}

	/*
	 * alarm() returns the number of seconds remaining until
	 * any previously scheduled alarm was due to be delivered,
	 * or zero if there was no pre‐viously scheduled alarm.
	 */
	if (0 != alarm(nsec)) {
		printf("alarm was already set.\n");
		exit(EXIT_FAILURE);
	}
	
	if ((n = connect(sockfd, sa, len)) < 0) {
		close(sockfd);
		if (EINTR == errno) {
			errno = ETIMEDOUT;
		}
	}

	alarm(0);	// turn off the alarm
	signal(SIGALRM, sigfunc);

	return n;
}
