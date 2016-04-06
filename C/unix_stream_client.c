#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/un.h>			// sockaddr_un
#include <sys/types.h>
#include <sys/socket.h>

#define BUF_SIZE 256
#define UNIX_DOMAIN_PATH "/tmp/unix_domain_sockfd"

void str_cli(FILE *fp, int sockfd) {
	char sendline[BUF_SIZE] = {'\0'};
	char recvline[BUF_SIZE] = {'\0'};

	while (NULL != fgets(sendline, sizeof(sendline) ,fp)) {
		if (write(sockfd, sendline, strlen(sendline)) < 0) {
			if (EINTR == errno) {
				continue;
			}

			perror("write");
			exit(EXIT_FAILURE);
		}

		bzero(recvline, sizeof(recvline));
		if (read(sockfd, recvline, sizeof(recvline)) < 0) {
			if (EINTR == errno) {
				continue;
			}

			perror("read");
			exit(EXIT_FAILURE);

		}

		fputs(recvline, stdout);
	}
}

int main(int argc, char* argv[]) {
	pid_t pid;
	int sockfd, connfd;
	socklen_t clilen;
	struct sockaddr_un servaddr, cliaddr;

	if ((sockfd = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_LOCAL;
	strcpy(servaddr.sun_path, UNIX_DOMAIN_PATH);

	if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
		perror("connect");
		exit(EXIT_FAILURE);
	}

	str_cli(stdin, sockfd);

	exit(EXIT_SUCCESS);
}
