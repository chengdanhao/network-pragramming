#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define BUF_LEN 256
#define PORT 10001

int main(int argc, char* argv[]) {

	int master_sock, n;
	struct sockaddr_in serv_addr;
	struct hostent* server;
	char send_buf[BUF_LEN];
	char recv_buf[BUF_LEN];

	if (argc < 2) {
		printf("Usage: %s <HOSTNAME>.\n", argv[0]);
		exit(-1);
	}

	// socket
	master_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (master_sock < 0) {
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

	// connect
	if (connect(master_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("connect");
		exit(-1);
	}

	while (1) {
		printf("Please enter the mssage: ");
		bzero(send_buf, sizeof(send_buf));
		fgets(send_buf, sizeof(send_buf) - 1, stdin);

		if (0 == strcmp(send_buf, "exit\n")) {
			printf("exit!\n");
			close(master_sock);
			exit(0);
		}

		// send message
		n = write(master_sock, send_buf, strlen(send_buf));
		if (n < 0) {
			perror("write");
			exit(-1);
		}

		// recv response
		bzero(recv_buf, sizeof(recv_buf));
		n = read(master_sock, recv_buf, sizeof(recv_buf) - 1);
		if (0 == n) {
			printf("server has closed.\n");
			close(master_sock);
			exit(0);
		} else if (n < 0) {
			perror("read");
			exit(-1);
		}

		printf("Receive message: %s.\n", recv_buf);
	}

	return 0;
}
