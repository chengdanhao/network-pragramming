#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define EXCHANGE_SERVER_PORT 5002
#define BUF_SIZE 256

int main(int argc, char* argv[]) {
	int sockfd;
	socklen_t len;
	int n;
	char recv_msg[BUF_SIZE] = {'\0'};
	char send_msg[BUF_SIZE] = {'\0'};
	struct sockaddr_in serv_addr, cli_addr;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		exit(-1);
	}

	bzero((void *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(EXCHANGE_SERVER_PORT);

	if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("bind");
		exit(-1);
	}

	while (1) {
		len = sizeof(cli_addr);	// 每次都要更新
		bzero((void *)recv_msg, sizeof(recv_msg));
		bzero((void *)send_msg, sizeof(send_msg));

		n = recvfrom(sockfd, recv_msg, sizeof(recv_msg), 0, (struct sockaddr*)&cli_addr, &len);
		if (-1 == n || 0 == n) {
			perror("recvfrom");
			close(sockfd);
			exit(-1);
		}

		printf("[server] recv msg from %s:%d, msg = %s.\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port) , recv_msg);

		//n = sendto(sockfd, send_msg, strlen(send_msg), 0, cliaddr, len);
	}

	exit(0);
}
