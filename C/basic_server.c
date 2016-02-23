#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define PORT 10001
#define BUF_LEN 256

int main() {
	int master_sock, new_sock;
	socklen_t clilen;
	char recv_buf[BUF_LEN];
	struct sockaddr_in serv_addr, cli_addr;
	int n;

	master_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (master_sock < 0) {
		perror("creating socket error");
		exit(1);
	}

	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(PORT);

	if (bind(master_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) {
		perror("bind error");
		exit(1);
	}

	listen(master_sock, 5);

	clilen = sizeof(cli_addr);
	new_sock = accept(master_sock, (struct sockaddr*)&cli_addr, &clilen);
	if (new_sock < 0) {
		perror("accept error");
		exit(1);
	}

	bzero(recv_buf, BUF_LEN);
	n = read(new_sock, recv_buf, BUF_LEN);
	if (n < 0) {
		perror("reading from socket");
		exit(1);
	}

	printf("message is : %s.\n", recv_buf);

	char *response = "I got your message";
	n = write(new_sock, response, sizeof(response));
	if (n < 0) {
		perror("writing to socket error");
		exit(1);
	}

	return 0;
}
