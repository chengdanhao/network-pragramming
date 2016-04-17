#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 10001
#define BUF_LEN 256

void str_echo(int sockfd) {
	ssize_t n;
	char buf[BUF_LEN];

	bzero(buf, sizeof(buf));
	n = read(sockfd, buf, sizeof(buf) - 1);
	printf("echo back : <%ld> %s.\n", n, buf);
	write(sockfd, buf, n);
}

int main() {
	pid_t pid;
	int listenfd, connfd, on;
	socklen_t clilen;
	char recv_buf[BUF_LEN];
	struct sockaddr_in serv_addr, cli_addr;

	// create socket
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	// fill struct
	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(PORT);

	// bind
	if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	// listen
	listen(listenfd, 5);

	clilen = sizeof(cli_addr);
	for (;;) { 
		// accept
		connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);
		if (connfd < 0) {
			if (EINTR == errno){
				continue;
			}
			perror("accept");
			exit(EXIT_FAILURE);
		}

		printf("new connect from %d.\n", connfd);

		if (0 == (pid = fork())) {
			close(listenfd);
			str_echo(connfd);
			exit(EXIT_SUCCESS);
		}

		close(connfd);
	}

	exit(EXIT_SUCCESS);
}
