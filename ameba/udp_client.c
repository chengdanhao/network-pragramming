#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h> //gethostbyname

#define EXCHANGE_SERVER_IP "127.0.0.1"
#define EXCHANGE_SERVER_PORT 5002
#define BUF_SIZE 256

int main(int argc, char* argv[]) {
	int sockfd;
	char send_msg[BUF_SIZE] = "test udp message";
	struct sockaddr_in serv_addr;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		exit(-1);
	}

	bzero((void *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(EXCHANGE_SERVER_PORT);
	// inet_pton(AF_INET, EXCHANGE_SERVER_IP, &serv_addr.sin_addr);
	inet_aton(EXCHANGE_SERVER_IP, &serv_addr.sin_addr);

	while (1) {
		// 客户可以显式地调用bind，这里没有，内核为它选择一个随机端口
		sendto(sockfd, send_msg, strlen(send_msg), 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
		printf("[client] send_msg = %s.\n", send_msg);
		sleep(2);
	}

	exit(0);
}
