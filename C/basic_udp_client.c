#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h> //gethostbyname

#define PORT 10001
#define BUF_SIZE 256

void dg_cli(FILE* fp, int sockfd, struct sockaddr* servaddr, socklen_t servlen) {
	int n;
	char recv_msg[BUF_SIZE];
	char send_msg[BUF_SIZE];

	while (fgets(send_msg, sizeof(send_msg), fp) != NULL) {

		// 客户可以显式地调用bind，这里没有，内核为它选择一个随机端口
		sendto(sockfd, send_msg, strlen(send_msg), 0, servaddr, servlen);
		printf("[client] send_msg = %s.\n", send_msg);

		bzero((void *)recv_msg, sizeof(recv_msg));

		// 第五和第六个参数为NULL，这将告知内核我们不关心数据报由谁发送。
		// 风险：任何进程都可以想本客户端的IP和端口发送数据报
		n = recvfrom(sockfd, recv_msg, sizeof(recv_msg), 0, NULL, NULL);
		printf("[client] recv_msg = %s.\n", recv_msg);
	}
}

int main(int argc, char* argv[]) {

	int sockfd;
	struct sockaddr_in serv_addr;
	struct hostent* server;


	if (argc != 2) {
		printf("Usage: %s <IP/HOSTNAME>\n", argv[0]);
		exit(-1);
	}

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		exit(-1);
	}


	server = gethostbyname(argv[1]);
	if (NULL == server) {
		perror("gethostbyname");
		exit(-1);
	}

	bzero((char*)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char*)server->h_addr, (char*)&(serv_addr.sin_addr.s_addr), server->h_length);
	serv_addr.sin_port = htons(PORT);

	/*
	   bzero((void *)&serv_addr, sizeof(serv_addr));
	   serv_addr.sin_family = AF_INET;
	   serv_addr.sin_port = htons(PORT);
	   inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);
	 */

	dg_cli(stdin, sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

	exit(0);
}
