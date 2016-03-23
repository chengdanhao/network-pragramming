#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define BUF_SIZE 256

int udp_server(const char *host, const char *serv, socklen_t *p_len) {
	int sockfd, n;
	struct addrinfo hints, *res, *p_res;

	bzero(&hints, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if (0 != (n = getaddrinfo(host, serv, &hints, &res))) {
		printf("udp_server error for %s, %s : %s\n",
				host, serv, gai_strerror(n));
		exit(EXIT_FAILURE);
	}

	p_res = res;

	while (NULL != res) {
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sockfd < 0) {
			continue;	// error, ignore this
		}

		if (0 == bind(sockfd, res->ai_addr, res->ai_addrlen)) {
			break;		// success
		}

		close(sockfd);

		res = res->ai_next;
	}

	if (NULL == res) {
		printf("udp_server error for %s, %s\n", host, serv);
		exit(EXIT_FAILURE);
	}

	if (NULL != p_len) {
		*p_len = res->ai_addrlen;
	}

	freeaddrinfo(p_res);

	return sockfd;
}

int main(int argc, char* argv[]) {
	int sockfd;
	ssize_t n;
	socklen_t len;
	time_t ticks;
	char buf[BUF_SIZE];
	char ip[BUF_SIZE];
	struct sockaddr_storage cliaddr;
	struct sockaddr *sa;
	struct sockaddr_in* sin;
	struct sockaddr_in6* sin6;

	if (2 == argc) {
		sockfd = udp_server(NULL, argv[1], NULL);
	} else if (3 == argc) {
		sockfd = udp_server(argv[1], argv[2], NULL);
	} else {
		printf("USAGE: %s [host/IP] <service/port>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	while (1) {
		len = sizeof(cliaddr);
		n = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&cliaddr, &len);
		buf[n] = '\0';

		sa = (struct sockaddr*)&cliaddr;
		switch (sa->sa_family) {
			case AF_INET:
				sin = (struct sockaddr_in*)&cliaddr;
				printf("[IPv4] datagram from %s\n", inet_ntop(AF_INET, &(sin->sin_addr), ip, sizeof(ip)));
				break;
			case AF_INET6:
				sin6 = (struct sockaddr_in6*)&cliaddr;
				printf("[IPv6] datagram from %s\n", inet_ntop(AF_INET6, &(sin6->sin6_addr), ip, sizeof(ip)));
				break;
			default:
				printf("unknown type.\n");
				break;
		}

		ticks = time(NULL);
		snprintf(buf, sizeof(buf), "%.24s\n", ctime(&ticks));
		sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr*)&cliaddr, len);
	}
}
