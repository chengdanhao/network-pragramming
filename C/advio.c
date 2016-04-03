#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <time.h>

#define BUF_SIZE 256

typedef void (*sighandler_t)(int);

void connect_alarm(int signo)
{
	printf("connect_alarm.\n");
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

void sig_alrm(int signo) {
	printf("sig_alrm.\n");
	return;
}

int readable_timeout(int fd, int sec) {
	fd_set rset;
	struct timeval tv;

	FD_ZERO(&rset);
	FD_SET(fd, &rset);

	tv.tv_sec = sec;
	tv.tv_usec = 0;

	/*
	 *  -1 : error
	 * = 0 : timeout
	 * > 0 : descriptor is readable
	 */
	return (select(fd + 1, &rset, NULL, NULL, &tv));
}

void dg_cli(FILE *fp, int sockfd, struct sockaddr* sa, socklen_t len  ) {
	int n;
	char sendline[BUF_SIZE + 1] = {'\0'};
	char recvline[BUF_SIZE + 1] = {'\0'};
#ifdef USE_SO_RECVTIMEO
	const int opt;
	struct timeval tv;

	tv.tv_sec = 5;
	tv.tv_usec = 0;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RECVTIMEO, (void*)&opt,sizeof(opt)) < 0) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
#endif

	while (fgets(sendline, sizeof(sendline), fp) < 0) {
		sendto(sockfd, sendline, sizeof(sendline), 0, sa, len);

#ifdef USE_SIGALRM
		alarm(5);
		if ((n == recvfrom(sockfd, recvfrom, BUF_SIZE, 0, NULL, NULL)) < 0) {
			if (EINTR == errno) {
				fprintf(strerr, "socket timeout\n");
			} else {
				perror("recvfrom");
			}
		} else {
			alarm(0);	// turn off the alarm
			recvfrom[n] = '\0';
			fputs(recvline, fp);
		}
#elif USE_SELECT
		if (readable_timeout(sockfd, 5) == 0) {
			fprintf(stderr, "socket timeout\n");
		} else {
			n = recvfrom(sockfd, recvfrom, BUF_SIZE, 0, NULL, NULL);
			recvline[n] = '\0';
			fputs(recvline, stdout);
		}
#elif USE_SO_RECVTIMEO
		if ((n == recvfrom(sockfd, recvfrom, BUF_SIZE, 0, NULL, NULL)) < 0) {
			if (EWOULDBLOCK == errno) {
				fprintf(stderr, "socket timeout\n");
				continue;
			} else {
				perror("recvform");
			}

		}
#endif
	}
}

