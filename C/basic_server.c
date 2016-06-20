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

	// create socket
	master_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (master_sock < 0) {
		perror("socket");
		exit(1);
	}

	// fill struct
	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(PORT);

	// bind
	if (bind(master_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) {
		perror("bind");
		exit(1);
	}

	// listen
	listen(master_sock, 5);

	// accept
	clilen = sizeof(cli_addr);
	new_sock = accept(master_sock, (struct sockaddr*)&cli_addr, &clilen);
	if (new_sock < 0) {
		perror("accept");
		exit(1);
	}

	bzero(recv_buf, sizeof(recv_buf));
	n = read(new_sock, recv_buf, sizeof(recv_buf) - 1);
	if (n < 0) {
		perror("read");
		exit(1);
	}

	printf("Receive message : %s.\n", recv_buf);

	char *response = recv_buf;
	n = write(new_sock, response, strlen(response));
	if (n < 0) {
		perror("write");
		exit(1);
	}

	return 0;
}
