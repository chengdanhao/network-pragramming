#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <errno.h>

#define BUF_SIZE 256

int udp_client(const char* host, const char* serv,
		struct sockaddr** p_sa, socklen_t* p_len) {
	int sockfd, n;
	struct addrinfo hints, *res, *p_res;

	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if (0 != (n = getaddrinfo(host, serv, &hints, &res))) {
		printf("udp_client error for %s, %s : %s",
				host, serv, gai_strerror(n));
		exit(EXIT_FAILURE);
	}

	p_res = res;

	while (NULL != res) {

		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sockfd >= 0) {
			break; // success
		}

		res = res->ai_next;
	}

	if (NULL == res) {
		printf("udp_client error for %s, %s.\n", host, serv);
		// Q:这里不需要考虑内存释放吗？
		// A:进程若是退出，进程相关的任何资源都会释放，这个你不用担心。
		exit(EXIT_FAILURE);
	}

	*p_sa = (struct sockaddr*)malloc(res->ai_addrlen);
	memcpy(*p_sa, res->ai_addr, res->ai_addrlen);
	*p_len = res->ai_addrlen;

	freeaddrinfo(p_res);

	return sockfd;
}

int main(int argc, char* argv[]) {
	int sockfd, n;
	char recv_line[BUF_SIZE];
	char ip[BUF_SIZE];
	socklen_t len;
	struct sockaddr* sa;
	struct sockaddr_in* sin;
	struct sockaddr_in6* sin6;

	if (argc != 3) {
		printf("USAGE: %s <hostname/IP> <service/port>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	sockfd = udp_client(argv[1], argv[2], &sa, &len);

	switch (sa->sa_family) {
		case AF_INET:
			sin = (struct sockaddr_in*)sa;
			printf("[IPv4]sending to %s\n", inet_ntop(AF_INET, &(sin->sin_addr), ip, sizeof(ip)));
			break;
		case AF_INET6:
			sin6 = (struct sockaddr_in6*)sa;
			printf("[IPv6]sending to %s\n", inet_ntop(AF_INET6, &(sin6->sin6_addr), ip, sizeof(ip)));
			break;
		default:
			printf("unknown type.\n");
			break;
	}

	sendto(sockfd , "hello", sizeof("hello"), 0, sa, len);

	n = recvfrom(sockfd, recv_line, sizeof(recv_line), 0, NULL, NULL);
	recv_line[n] = '\0';
	fputs(recv_line, stdout);

	exit(EXIT_SUCCESS);
}
