#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define PORT 10001
#define BUF_LEN 256

void str_cli(FILE* fp, int sockfd) {
	ssize_t nread, nwrite;
	char send_buf[BUF_LEN];
	char recv_buf[BUF_LEN];

	while (NULL != fgets(send_buf, sizeof(send_buf), fp)) {
		// send message
		nwrite = write(sockfd, send_buf, strlen(send_buf));
		if (nwrite < 0) {
			perror("write");
			exit(EXIT_FAILURE);
		}

		// recv response
		bzero(recv_buf, sizeof(recv_buf));
		nread = read(sockfd, recv_buf, sizeof(recv_buf) - 1);
		if (0 == nread) {
			printf("server has closed.\n");
			close(sockfd);
			exit(EXIT_FAILURE);
		} else if (nread < 0) {
			perror("read");
			exit(EXIT_FAILURE);
		}

		fputs(recv_buf, stdout);
	}

}

int main(int argc, char* argv[]) {

	int sockfd;
	struct sockaddr_in servaddr;
	struct hostent* server;

	if (argc < 2) {
		printf("Usage: %s <HOSTNAME>.\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	server = gethostbyname(argv[1]);
	if (NULL == server) {
		perror("gethostbyname");
		exit(EXIT_FAILURE);
	}

	bzero((char*)&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	bcopy((char*)server->h_addr, (char*)&(servaddr.sin_addr.s_addr), server->h_length);
	servaddr.sin_port = htons(PORT);

	// connect
	if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
		perror("connect");
		exit(EXIT_FAILURE);
	}

	str_cli(stdin, sockfd);

	exit(EXIT_SUCCESS);
}
