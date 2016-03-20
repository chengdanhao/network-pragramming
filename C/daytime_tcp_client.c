#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <errno.h>

#define BUF_SIZE 256

int tcp_connect(const char* host, const char* serv) {
	int sockfd, n;
	struct addrinfo hints, *res, *p_res;

	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((n == getaddrinfo(host, serv, &hints, &res)) != 0) {
		printf("getaddrinfo error for %s, %s: %s.\n",
				host, serv, gai_strerror(n));
		exit(EXIT_FAILURE);
	}

	p_res = res;

	while (NULL != res) {
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sockfd < 0) {
			continue;			// ignore this one
		}

		if (0 == connect(sockfd, res->ai_addr, res->ai_addrlen)) {
			break;				// successs
		} else {
			close(sockfd);		// ignore this one
		}

		res = res->ai_next;
	}

	if (NULL == res) {
		printf("tcp_connect error for %s, %s.\n", host, serv);
	}

	freeaddrinfo(p_res);

	return sockfd;
}

int main(int argc, char* argv[]) {
	int sockfd, n;
	socklen_t len;
	struct sockaddr_storage ss;
	struct sockaddr_in* sin;
	char str[BUF_SIZE] = {'\0'};
	char recv_line[BUF_SIZE] = {'\0'};

	if (argc != 3) {
		printf("USAGE: %s <hostname/IPaddress> <service/port>.\n", argv[0]);
		exit(EXIT_SUCCESS);
	}

	sockfd = tcp_connect(argv[1], argv[2]);

	len = sizeof(ss);

	if (getpeername(sockfd, (struct sockaddr*)&ss, &len) < 0) {
		perror("getpeername");
		exit(EXIT_FAILURE);
	}


	sin = (struct sockaddr_in*)&ss;
	/*
	if (sin->sin_family != AF_INET) {
		printf("%s: the type must be AF_INET.\n", __FUNCTION__);
		exit(EXIT_FAILURE);
	}
	*/

	// warning: comparison between pointer and integer
	// addr header file arpa/inet.h
	switch (sin->sin_family) {
		case AF_INET:
			printf("AF_INET.\n");
			if (NULL == inet_ntop(AF_INET, &(sin->sin_addr), str, sizeof(str))) {
				perror("inet_ntop");
				exit(EXIT_FAILURE);
			}
			break;
		case AF_INET6:
			printf("AF_INET6.\n");
			if (NULL == inet_ntop(AF_INET6, &(sin->sin_addr), str, sizeof(str))) {
				perror("inet_ntop");
				exit(EXIT_FAILURE);
			}
			break;
		default:
			printf("unknown type.\n");
			exit(EXIT_FAILURE);
	}

	// it's ok :)
	// printf("connect to %s\n", inet_ntop(AF_INET, &(sin->sin_addr), str, sizeof(str)));
	printf("connect to %s\n", str);

	while ((n = read(sockfd, recv_line, BUF_SIZE)) > 0) {
		recv_line[n] = '\0';
		fputs(recv_line, stdout);
	}

	exit(EXIT_SUCCESS);
}
