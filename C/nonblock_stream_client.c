#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>

#define MAX_SIZE 1024

char* gf_time() {
	struct timeval tv;
	static char str[30];
	char *ptr;

	if (gettimeofday(&tv, NULL) < 0) {
		perror("gettimeofday");
		exit(EXIT_FAILURE);
	}

	/* 12:54:05,119738 */
	ptr = ctime(&tv.tv_sec);
	strcpy(str, &ptr[11]);
	snprintf(str + 8, sizeof(str) - 8, ".%06ld", tv.tv_usec);

	return str;
}

int max(int a, int b, int c) {
	int d = 0;

	return (d = (a > b ? a : b)) > c ? d : c;
}

void str_cli(FILE *fp, int sockfd) {
	int n, nread, nwritten;
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

		if (FD_ISSET(STDIN_FILENO, &rset)) {
			if (nread = read(STDIN_FILENO, toiptr, &to[MAX_SIZE] - toiptr) < 0) {
				if (EWOULDBLOCK != errno) {
					printf("read error on stdin\n");
					exit(EXIT_FAILURE);
				}
			} else if (0 == nread) {
				fprintf(stderr, "%s, EOF on stdin\n", gf_time());
				stdineof = 1;
				if (tooptr == toiptr) {
					shutdown(sockfd, SHUT_WR);
				}
			} else {
				fprintf(stderr, "%s : read %d bytes form stdin\n", gf_time(), nread);
				toiptr += nread;
				FD_SET(sockfd, &wset);
			}
		}

		if (FD_ISSET(sockfd, &rset)) {
			if (nread = read(sockfd, friptr, &fr[MAX_SIZE] - friptr) < 0) {
				if (EWOULDBLOCK != errno) {
					printf("read error on socket\n");
					exit(EXIT_FAILURE);
				}
			} else if (0 == nread) {
				fprintf(stderr, "%s, EOF on socket\n", gf_time());
				if (stdineof) {
					return;		// normal termination
				} else {
					printf("str_cli : server terminated prematurely\n");
					exit(EXIT_FAILURE);
				}
			} else {
				fprintf(stderr, "%s : read %d bytes form socket\n", gf_time(), nread);
				friptr += nread;
				FD_SET(STDOUT_FILENO, &wset);
			}
		}

		if (FD_ISSET(STDOUT_FILENO, &wset) && (n = friptr - froptr) > 0) {
			if ((nwritten = write(STDOUT_FILENO, froptr, n)) < 0) {
				if (EWOULDBLOCK != errno) {
					printf("write error to stdout\n");
					exit(EXIT_FAILURE);
				}
			} else {
				fprintf(stderr, "%s wrote %d bytes to stdout\n", gf_time(), nwritten);
				froptr += nwritten;
				if (froptr == friptr) {
					froptr = friptr = fr;
				}
			}
		}

		if (FD_ISSET(sockfd, &wset) && (n = toiptr - tooptr) > 0) {
			if ((nwritten = write(sockfd, tooptr, n)) < 0) {
				if (EWOULDBLOCK != errno) {
					printf("write error to sockfd\n");
					exit(EXIT_FAILURE);
				}
			} else {
				fprintf(stderr, "%s wrote %d bytes to stdout\n", gf_time(), nwritten);
				froptr += nwritten;
				if (tooptr == toiptr) {
					tooptr = toiptr = to;
					if (stdineof) {
						shutdown(sockfd, SHUT_WR);
					}
				}
			}
		}
	}
}

int main(int argc, char* argv[]) {

	exit(EXIT_SUCCESS);
}

