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

void dg_cli(FILE* fp, int sockfd, struct sockaddr* pservaddr, socklen_t servlen) {
	int n;
	char recv_msg[BUF_SIZE] = {'\0'};
	char send_msg[BUF_SIZE] = {'\0'};

	while (NULL != fgets(send_msg, sizeof(send_msg), fp)) {
		n = sendto(sockfd, send_msg, strlen(send_msg), 0, pservaddr, servlen);
		if (n < 0) {
			if (EINTR == errno) {
				continue;
			}
			
			perror("sendto");
			close(sockfd);
			exit(EXIT_FAILURE);
		}
		
		bzero(recv_msg, sizeof(recv_msg));

		n = recvfrom(sockfd, recv_msg, sizeof(recv_msg), 0, NULL, NULL);
		if (n < 0) {
			if (EINTR == errno) {
				continue;
			}
			
			perror("recvfrom");
			close(sockfd);
			exit(EXIT_FAILURE);
		}

		printf("%s.\n", recv_msg);
	}
}

int main(int argc, char* argv[]) {
	int sockfd;
	socklen_t clilen;
	struct sockaddr_un servaddr, cliaddr;

	if ((sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	bzero(&cliaddr, sizeof(cliaddr));
	cliaddr.sun_family = AF_LOCAL;
	strcpy(cliaddr.sun_path, tmpnam(NULL));

	if (bind(sockfd, (struct sockaddr*)&cliaddr, sizeof(cliaddr)) < 0) {
		perror("bind");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_LOCAL;
	strcpy(servaddr.sun_path, UNIX_DOMAIN_PATH);

	dg_cli(stdin, sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

	exit(EXIT_SUCCESS);
}
