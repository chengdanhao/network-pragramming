#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>

#define BUF_SIZE 256
#define PORT 10001

#define BACKLOG 10
#define CLIENTS_NUM 30

int main(int argc, char* argv[]) {
	ssize_t nread;
	size_t nwrite;
	socklen_t cli_len;
	char recv_buf[BUF_SIZE] = {'\0'};
	char send_buf[BUF_SIZE] = {'\0'};
	int master_sock, new_sock;
	int sd, max_sd, cli_socks[CLIENTS_NUM];
	struct sockaddr_in serv_addr, cli_addr;
	struct fd_set rfds;
	struct fd_set wfds;
	struct fd_set efds;

	master_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (master_sock < 0) {
		perror("socket");
		exit(-1);
	}

	bzero((char*)&serv_addr, 0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr = htonl(IN_ADDR);
	serv_addr.sin_port = htons(PORT);

	if (bind(master_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) {
		perror("bind");
		exit(-1);
	}

	if (listen(master_sock, BACKLOG)) {
		perror("listen");
		exit(-1);
	}

	cli_len = sizeof(cli_addr);
	while(1) {
		FD_ZERO(&rfds);
		FD_SET(master_sock, &rfds);

		maxfd = master_sock;
		
		// add child sockets to set
		for (i = 0; i < CLIENTS_NUM; i++) {
			sd = cli_socks[i];

			if (sd > 0) {
				FD_SET(sd, &rfds);
			}

			if (sd > max_sd) {
				max_sd = sd;
			}
		}

		/*
		 * > 0  : Success, the number of file descriptors contained in the three descriptor sets.
		 * = 0  : the timeout expires before anything interesting happens.  
		 * = -1 : errno is set to indicate the error; the file descriptor sets are unmodified, and timeout becomes undefined.
		 */
		switch (select(max_sd + 1, &rfds, NULL, NULL, NULL)) {
			case -1:
				perror("select");
				exit(-1);
			case 0:
				break;	// loop again
			default:
				// ...
				break;
		}

		new_sock = accept(master_sock, (struct sockaddr*)&cli_addr, &cli_len);
		if (new_sock < 0) {
			perror("accept");
			exit(-1);
		}
	}

	exit(0);
}
