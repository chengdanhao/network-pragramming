#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUF_SIZE 256

int tcp_listen(const char* host, const char* serv, socklen_t* p_addrlen) {
	int listenfd, n, len;
	const int on = 1;
	struct addrinfo hints, *res, *p_res;

	bzero(&hints, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if (0 != (n = getaddrinfo(host, serv, &hints, &res))) {
		printf("%s error for %s, %s : %s.\n", host, serv, gai_strerror(n));
		exit(EXIT_FAILURE);
	}

	p_res = res;

	while (NULL != res) {
		listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (listenfd < 0) {
			continue;			// error, try next one
		}

		if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0 ) {
			perror("setsockopt");
			exit(EXIT_FAILURE);
		}

		if (0 == bind(listenfd, res->ai_addr, res->ai_addrlen)) {
			break;				// success
		}

		close(listenfd);		// error, try next one

		res = res->ai_next;
	}

	if (listen(listenfd, 10) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	if (NULL != p_addrlen) {
		*p_addrlen = res->ai_addrlen;
	}

	freeaddrinfo(p_res);

	return listenfd;
}

int main(int argc, char* argv[]) {
	int listenfd, connfd, n;
	socklen_t len;
	char ip[BUF_SIZE] = {'\0'};
	char time_buf[BUF_SIZE] = {'\0'};
	time_t ticks;
	struct sockaddr_storage cliaddr;

	if (argc != 2) {
		printf("USAGE: %s <service/port>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	listenfd = tcp_listen(NULL, argv[1], NULL);

	while (1) {
		len = sizeof(cliaddr);
		connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);
		if (connfd < 0) {
			if (EINTR == errno){
				continue;
			}
			perror("accept");
			exit(EXIT_FAILURE);
		}

		printf("connection from %s\n", inet_ntop(AF_INET, &cliaddr, ip, sizeof(ip)));

		ticks = time(NULL);
		snprintf(time_buf, sizeof(time_buf), "%.24s\r\n", ctime(&ticks));
		n = write(connfd, time_buf, strlen(time_buf));
		if (n < 0) {
			if (EINTR == errno){
				continue;
			}
			perror("write");
			exit(EXIT_FAILURE);
		}

		close(connfd);
	}

	exit(EXIT_SUCCESS);
}
