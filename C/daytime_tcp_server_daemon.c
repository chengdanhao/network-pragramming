#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUF_SIZE 256
#define MAX_FD 64

#define SYSLOG(...) syslog(LOG_INFO | LOG_LOCAL2, __VA_ARGS__)

int daemon_init(const char* ident, int facility) {
	int i;
	pid_t pid;

	pid = fork();
	if (pid < 0) {
		SYSLOG("daemon_init first fork");
		return -1;		// fork error
	} else if (pid > 0) {
		exit(EXIT_SUCCESS);		// parent terminate
	}

	/* child 1 continues ... */

	if (setsid() < 0) {			// become session leader
		SYSLOG("daemon_init setsid");
		return -1;
	}

	signal(SIGHUP, SIG_IGN);

	pid = fork();
	if (pid < 0) {
		SYSLOG("daemon_init second fork");
		return -1;		// fork error
	} else if (pid > 0) {
		exit(EXIT_SUCCESS);		// parent terminate
	}

	/* child 2 continues ... */
	chdir("/");					// change working directory

	for (i = 0; i < MAX_FD; i++) {
		close(i);
	}


	// redirecting stdin, stdout and stderr to /dev/null
	open("/dev/null", O_RDONLY);
	open("/dev/null", O_RDWR);
	open("/dev/null", O_RDWR);

	openlog(ident, LOG_PID, facility);

	return 0;
}

int tcp_listen(const char* host, const char* serv, socklen_t* p_addrlen) {
	int listenfd, n, len;
	const int on = 1;
	struct addrinfo hints, *res, *p_res;

	bzero(&hints, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if (0 != (n = getaddrinfo(host, serv, &hints, &res))) {
		SYSLOG("%s error for %s, %s : %s.\n", host, serv, gai_strerror(n));
		exit(EXIT_FAILURE);
	}

	p_res = res;

	while (NULL != res) {
		listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (listenfd < 0) {
			continue;			// error, try next one
		}

		if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0 ) {
			SYSLOG("setsockopt");
			exit(EXIT_FAILURE);
		}

		if (0 == bind(listenfd, res->ai_addr, res->ai_addrlen)) {
			break;				// success
		}

		close(listenfd);		// error, try next one

		res = res->ai_next;
	}

	if (listen(listenfd, 10) < 0) {
		SYSLOG("listen");
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
	socklen_t len, addrlen;
	char ip[BUF_SIZE] = {'\0'};
	char time_buf[BUF_SIZE] = {'\0'};
	time_t ticks;
	struct sockaddr_storage cliaddr;
	struct sockaddr* sa;
	struct sockaddr_in* sin4;
	struct sockaddr_in6* sin6;

	if (argc < 2 || argc > 3) {
		SYSLOG("USAGE: %s [host] <service/port>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	daemon_init(argv[0], 0);

	if (2 == argc) {
		// host为NULL，表示从本地搜索相应服务
		listenfd = tcp_listen(NULL, argv[1], &addrlen);
	} else if (3 == argc) {
		// host不为NULL，从host指定主机上寻找相应服务
		listenfd = tcp_listen(argv[1], argv[2], &addrlen);
	}

	while (1) {
		len = sizeof(cliaddr);
		connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);
		if (connfd < 0) {
			if (EINTR == errno){
				continue;
			}
			SYSLOG("accept");
			exit(EXIT_FAILURE);
		}

		sa = (struct sockaddr*)&cliaddr;

		switch (sa->sa_family) {
			case AF_INET:
				sin4 = (struct sockaddr_in*)&cliaddr;
				SYSLOG("connection from %s\n", inet_ntop(AF_INET, &(sin4->sin_addr), ip, sizeof(ip)));
				break;
			case AF_INET6:
				sin6 = (struct sockaddr_in6*)&cliaddr;
				SYSLOG("connection from %s\n", inet_ntop(AF_INET6, &(sin6->sin6_addr), ip, sizeof(ip)));
				break;
			default:
				SYSLOG("unknown type.\n");
				//exit(EXIT_FAILURE);
				continue;
		}

		ticks = time(NULL);
		snprintf(time_buf, sizeof(time_buf), "%.24s\r\n", ctime(&ticks));
		n = write(connfd, time_buf, strlen(time_buf));
		if (n < 0) {
			if (EINTR == errno){
				continue;
			}
			SYSLOG("write");
			exit(EXIT_FAILURE);
		}

		close(connfd);
	}

	exit(EXIT_SUCCESS);
}
