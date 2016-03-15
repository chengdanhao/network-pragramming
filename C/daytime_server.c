#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define MAX_LINE 256

int listenfd, connfd;

void sig_int(int signo) {
	close(listenfd);
	exit(EXIT_SUCCESS);
}

int main(int argc, char** argv)
{
	struct sockaddr_in servadd;
	char buff[MAX_LINE];
	time_t ticks;
	int on = 1;

	signal(SIGINT, sig_int);

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	if((setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	bzero(&servadd, sizeof(servadd));
	servadd.sin_family = AF_INET;
	servadd.sin_addr.s_addr = htonl(INADDR_ANY);
	servadd.sin_port = htons(13);

	if (bind(listenfd, (struct sockaddr*)&servadd, sizeof(servadd))) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	listen(listenfd, 0);

	while (1)
	{
		if ((connfd = accept(listenfd, (struct sockaddr*)NULL, NULL)) < 0) {
			perror("accept");
			exit(EXIT_SUCCESS);
		}

		ticks = time(NULL);
		snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
		write(connfd, buff, strlen(buff));
		close(connfd);
	}
}
