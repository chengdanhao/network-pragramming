#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define PORT 10001
#define BUF_SIZE 256

void dg_cli(FILE* fp, int sockfd, struct sockaddr* servaddr, socklen_t servlen) {
	int n;
	char recv_msg[BUF_SIZE];
	char send_msg[BUF_SIZE];

	while (fgets(send_msg, sizeof(send_msg), fp) != NULL) {
		sendto(sockfd, send_msg, strlen(send_msg), 0, servaddr, servlen);
		
		n = recvfrom(sockfd, recv_msg, sizeof(recv_msg), 0, NULL, NULL);
	}
}

int main(int argc, char* argv[]) {

	int sockfd;
	struct sockaddr_in serv_addr;

	if (argc != 2) {
		printf("Usage: %s <IP/HOSTNAME>\n");
		exit(-1);
	}

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		exit(-1);
	}

	bzero(serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);

	dg_cli(stdin, sockfd, (struct sockaddr*)&serv_addr, siezof(serv_addr));

	exit(0);
}
