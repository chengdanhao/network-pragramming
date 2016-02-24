#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 		//read, write
#include <sys/socket.h>
#include <arpa/inet.h>		// hotnl, hotns

#define PORT 10001
#define BUF_SIZE 256

void process(int sock);

int main(int argc, char* argv[]) {
	int master_sock, new_sock, n;
	struct sockaddr_in serv_addr, cli_addr;
	socklen_t cli_len;
	pid_t pid;

	master_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (master_sock < 0) {
		perror("socket");
		exit(1);
	}

	bzero((char*)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(master_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("bind");
		exit(1);
	}

	if (listen(master_sock, 10) < 0) {
		perror("listen");
		exit(1);
	}

	cli_len = sizeof(cli_addr);

	while(1) {
		new_sock = accept(master_sock, (struct sockaddr*)&cli_addr, &cli_len);
		if (new_sock < 0) {
			perror("accpet");
			exit(1);
		}

		pid = fork();
		switch (pid) {
			case -1:
				perror("fork");
				exit(1);
			case 0:
				// child
				close(master_sock);
				process(new_sock);
				exit(0);	// 这里必须要exit，仔细思考
			default:
				// parent
				close(new_sock);
		}
	}

	return 0;
}

void process(int sock) {
	int n;
	char recv_buf[BUF_SIZE] = {'\0'};
	char* response = "I got your message";

	n = read(sock, recv_buf, sizeof(recv_buf) - 1);
	if (n < 0) {
		perror("read");
		exit(1);
	}

	printf("Received message: %s.\n", recv_buf);

	n = write(sock, response, strlen(response));
	if (n < 0) {
		perror("write");
		exit(1);
	}
}
