#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <errno.h>

#define MAX_SIZE 1024

int max(int a, int b, int c) {
	int d = 0;

	return (d = (a > b ? a : b)) > c ? d : c;
}

void str_cli(FILE *fp, int sockfd) {
	int val, stdineof, maxfd;
	char *toiptr, *tooptr;
	char *friptr, *froptr;
	char to[MAX_SIZE], fr[MAX_SIZE];
	fd_set rset, wset;

	val = fcntl(sockfd, F_GETFL, 0);
	fcntl(sockfd, F_SETFL, val | O_NONBLOCK);

	val = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(sockfd, F_SETFL, val | O_NONBLOCK);

	val = fcntl(STDOUT_FILENO, F_GETFL, 0);
	fcntl(sockfd, F_SETFL, val | O_NONBLOCK);

	toiptr = tooptr = to;
	friptr = froptr = fr;
	stdineof = 0;

	maxfd = max(STDIN_FILENO, STDOUT_FILENO, sockfd) + 1;
	while (1) {
		FD_ZERO(&rset);
		FD_ZERO(&wset);

		if (0 == stdineof && toiptr < &to[MAX_SIZE]) {
			FD_SET(STDIN_FILENO, &rset);	/* read from stdin */
		}

		if (friptr < &fr[MAX_SIZE]) {
			FD_SET(sockfd, &rset);			/* read from socket */
		}

		if (tooptr != toiptr) {
			FD_SET(sockfd, &wset);			/* write to socket */
		}

		if (froptr != friptr) {
			FD_SET(STDOUT_FILENO, &wset);	/* write to stdout */
		}

		select (maxfd, &rset, &wset, NULL, NULL);
	}
}

int main(int argc, char* argv[]) {

	exit(EXIT_SUCCESS);
}
