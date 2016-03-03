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

#define BACKLOG 5
#define MAX_CLIENTS_NUM 3

#define ACCEPT_MSG "Accept, connection established!"
#define REFUSE_MSG "Refuse, connection numbers reach the upper limit!"

int main(int argc, char* argv[]) {
	ssize_t nrecv;
	size_t nsend;
	socklen_t cli_len;
	char recv_buf[BUF_SIZE] = {'\0'};
	char send_buf[BUF_SIZE] = {'\0'};
	int i, cur_conn_cnt = 0, master_sock, new_sock;
	int sd, max_sd, cli_socks[MAX_CLIENTS_NUM];
	struct sockaddr_in serv_addr, cli_addr;
	struct timeval tv;
	int opt;
	fd_set rfds, active_fds;

	master_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (master_sock < 0) {
		perror("ERROR socket");
		exit(-1);
	}

	// just a good habit
	if (setsockopt(master_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ) {
		perror("ERROR setsockopt");
		exit(-1);
	}

	bzero((char*)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(PORT);

	if (bind(master_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) {
		perror("ERROR bind");
		exit(-1);
	}

	if (listen(master_sock, BACKLOG)) {
		perror("ERROR listen");
		exit(-1);
	}


	tv.tv_sec = 10;
	tv.tv_usec = 0;

	FD_ZERO(&active_fds);
	FD_SET(master_sock, &active_fds);

	memset(cli_socks, -1, sizeof(cli_socks));
	max_sd = master_sock;
	while(1) {
		// VERY IMPORTANT!
		rfds = active_fds;

		/*
		 * > 0  : Success, the number of file descriptors contained in the three descriptor sets.
		 * = 0  : the timeout expires before anything interesting happens.  
		 * = -1 : errno is set to indicate the error; the file descriptor sets are unmodified, and timeout becomes undefined.
		 * 
		 * the 1st argument can simply set to FD_SETSIZE
		 */
		switch (select(max_sd + 1, &rfds, NULL, NULL, /* &tv */NULL)) {
			case -1:
				perror("ERROR select");
				exit(-1);
			case 0:
				// timeout, just loop again
				printf("timeout.");
				break; // cotinue;
			default:
				// new connection comes.
				if (FD_ISSET(master_sock, &rfds)) {
					cli_len = sizeof(cli_addr);
					new_sock = accept(master_sock, (struct sockaddr*)&cli_addr, &cli_len);
					if (new_sock < 0) {
						perror("ERROR accept");
						exit(-1);
					}

					// Do connection numbers reach the upper limit?
					if (cur_conn_cnt > MAX_CLIENTS_NUM - 1) {
						printf(REFUSE_MSG".\n");

						if (send(new_sock, REFUSE_MSG, strlen(REFUSE_MSG), 0) < 0) {
							perror("ERROR send refuse message");
						}

						close(new_sock);
						continue;
					} else {
					// not reach the upper limit, accept the socket.
					printf("new connection, sockfd = %d, ip = %s, port = %d.\n", 
							new_sock, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));



					for (i = 0; i < MAX_CLIENTS_NUM; i++) {
							if (-1 == cli_socks[i]) {
							cli_socks[i] = new_sock;
							printf("add sock %d to cli_socks[%d].\n", new_sock, i);
							break;
						}
					}

					// current connection count

					// renew the value of maxsd
					if (new_sock > max_sd) {
						max_sd = new_sock;
						}
						FD_SET(new_sock, &active_fds);
						cur_conn_cnt++;	// current connection count
						if (send(new_sock, ACCEPT_MSG, strlen(ACCEPT_MSG), 0) < 0) {
							perror("ERROR send accept message");
						}
					}
				}

				// client sockets
				for (i = 0; i < cur_conn_cnt; i++) {
					sd = cli_socks[i];
					if (sd < 0) {
						continue;
					}

					if (0 == FD_ISSET(sd, &rfds)) {
						continue;
					}

					bzero(recv_buf, sizeof(recv_buf));
					nrecv = recv(sd, recv_buf, sizeof(recv_buf) - 1, 0);
					switch (nrecv) {
						case -1:
							perror("ERROR recv message");
							close(sd);
							FD_CLR(sd, &active_fds);
							cli_socks[i] = -1;
							break;
						case 0:
							printf("sock %d close the connection.", sd);
							close(sd);
							FD_CLR(sd, &active_fds);
							cli_socks[i] = -1;
							break;
						default:
							printf("recv (%ld) message: %s.\n", nrecv, recv_buf);
							break;
					}

					bzero(send_buf, sizeof(send_buf));
					sprintf(send_buf, "ECHO BACK : %s", recv_buf);

					if (send(sd, send_buf, strlen(send_buf), 0) < 0) {
						perror("ERROR echo back");
					}

				}

				break;
		} // end of switch
	} // end of while(1)

	exit(0);
}
