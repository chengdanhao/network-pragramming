#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/un.h>			// sockaddr_un
#include <sys/types.h>
#include <sys/socket.h>

#define BUF_SIZE 256
#define UNIX_DOMAIN_PATH "/tmp/unix_domain_sockfd"

void dg_echo(int sockfd, struct sockaddr* pcliaddr, socklen_t clilen)
{
	int n;
	char recv_msg[BUF_SIZE] = {'\0'};
	char send_msg[BUF_SIZE] = {'\0'};

	while (1) {
		bzero(recv_msg, sizeof(recv_msg));
		bzero(send_msg, sizeof(send_msg));

		n = recvfrom(sockfd, recv_msg, sizeof(recv_msg), 0, pcliaddr, &clilen);
		if (n < 0) {
			if (EINTR == errno) {
				continue;
			}
			
			perror("recvfrom");
			close(sockfd);
			exit(EXIT_FAILURE);
		}

		sprintf(send_msg, "ECHO BACK : %s", recv_msg);

		n = sendto(sockfd, send_msg, strlen(send_msg), 0, pcliaddr, clilen);
		if (n < 0) {
			if (EINTR == errno) {
				continue;
			}
			
			perror("sendto");
			close(sockfd);
			exit(EXIT_FAILURE);
		}
	}
}

int main(int argc, char* argv[]) {
	int sockfd;
	struct sockaddr_un servaddr, cliaddr;

	if ((sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	unlink(UNIX_DOMAIN_PATH);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_LOCAL;
	strcpy(servaddr.sun_path, UNIX_DOMAIN_PATH);

	if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	dg_echo(sockfd, (struct sockaddr*)&cliaddr, sizeof(cliaddr));

	exit(EXIT_SUCCESS);
}
