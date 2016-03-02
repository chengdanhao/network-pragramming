#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define PORT 10001
#define BUF_SIZE 256

void dg_echo(int sockfd, struct sockaddr* cliaddr, socklen_t clilen)
{
	socklen_t len;
	int n;
	char recv_msg[BUF_SIZE] = {'\0'};
	char send_msg[BUF_SIZE] = {'\0'};

	while (1) {
		len = clilen;	// 每次都要更新
		bzero((void *)recv_msg, sizeof(recv_msg));
		bzero((void *)send_msg, sizeof(send_msg));

		n = recvfrom(sockfd, recv_msg, sizeof(recv_msg), 0, cliaddr, &len);
		// printf("[server] recv msg from %s, msg = %s.\n", inet_ntoa(((struct sockaddr_in*)cliaddr)->sin_addr), recv_msg);
		printf("[server] recv_msg = %s.\n", recv_msg);

		if (n < 0) {
			perror("recvfrom");
			close(sockfd);
			exit(-1);
		}

		// 如果n=0如果做判断？EOF还是长度为0的包？

		sprintf(send_msg, "ECHO BACK : %s", recv_msg);

		// 最后一个参数不是指针
		n = sendto(sockfd, send_msg, strlen(send_msg), 0, cliaddr, len);
		printf("[server] send_msg = %s.\n", send_msg);
	}
}

int main(int argc, char* argv[]) {
	int sockfd;
	struct sockaddr_in serv_addr, cli_addr;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		exit(-1);
	}

	bzero((void *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(PORT);

	if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("bind");
		exit(-1);
	}

	dg_echo(sockfd, (struct sockaddr*)&cli_addr, sizeof(cli_addr));

	exit(0);
}
