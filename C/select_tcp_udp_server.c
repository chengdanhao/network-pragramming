#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
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

void sig_child(int signo) {

}

void process(int sockfd) {

}

int main(int argc, char* argv[]) {
	ssize_t nrecv;
	size_t nsend;
	socklen_t cli_len;
	pid_t pid;
	char tcp_recv_buf[BUF_SIZE] = {'\0'};
	char tcp_send_buf[BUF_SIZE] = {'\0'};
	char udp_recv_buf[BUF_SIZE] = {'\0'};
	char udp_send_buf[BUF_SIZE] = {'\0'};
	int i, cur_conn_cnt = 0;
	int listenfd, udpfd, new_sock;
	int sd, max_sd, cli_socks[MAX_CLIENTS_NUM];
	struct sockaddr_in serv_addr, cli_addr;
	int opt;
	fd_set rfds, active_fds;

	/*
	 * TCP SOCKET
	 */

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) {
		perror("[ERROR] TCP SOCKET");
		exit(-1);
	}

	// just a good habit
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ) {
		perror("[ERROR] setsockopt");
		exit(-1);
	}

	bzero((char*)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(PORT);

	if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) {
		perror("[ERROR] TCP BIND");
		exit(-1);
	}

	if (listen(listenfd, BACKLOG)) {
		perror("[ERROR] listen");
		exit(-1);
	}

	/*
	 * UDP SOCKET
	 */

	udpfd =  socket(AF_INET, SOCK_DGRAM, 0);
	if (listenfd < 0) {
		perror("[ERROR] UDP SOCKET");
		exit(-1);
	}

	bzero((char*)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(PORT);

	if (bind(udpfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) {
		perror("[ERROR] UDP BIND");
		exit(-1);
	}

	signal(SIGCHLD, sig_child);

	FD_ZERO(&active_fds);

	memset(cli_socks, -1, sizeof(cli_socks));
	max_sd = listenfd > udpfd ? listenfd : udpfd;

	while(1) {
		FD_SET(listenfd, &rfds);
		FD_SET(udpfd, &rfds);

		/*
		 * > 0  : Success, the number of file descriptors contained in the three descriptor sets.
		 * = 0  : the timeout expires before anything interesting happens.  
		 * = -1 : errno is set to indicate the error; the file descriptor sets are unmodified, and timeout becomes undefined.
		 * 
		 * the 1st argument can simply set to FD_SETSIZE
		 */
		switch (select(max_sd + 1, &rfds, NULL, NULL, /* &tv */NULL)) {
			case -1:
				perror("[ERROR] select");
				exit(-1);
			case 0:
				// timeout, just loop again
				printf("timeout.");
				break; // cotinue;
			default:
				// new TCP connection comes.
				if (FD_ISSET(listenfd, &rfds)) {
					cli_len = sizeof(cli_addr);
					new_sock = accept(listenfd, (struct sockaddr*)&cli_addr, &cli_len);
					if (new_sock < 0) {
						perror("[ERROR] accept");
						exit(-1);
					}

					// Do connection numbers reach the upper limit?
					if (cur_conn_cnt > MAX_CLIENTS_NUM - 1) {
						printf(REFUSE_MSG".\n");

						if (send(new_sock, REFUSE_MSG, strlen(REFUSE_MSG), 0) < 0) {
							perror("[ERROR] send refuse message");
						}

						close(new_sock);
						continue;
					} else {
						// not reach the upper limit, accept the socket.
						printf("new connection, sockfd = %d, ip = %s, port = %d.\n", 
								new_sock, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

						pid = fork();
						switch (pid) {
							case -1:
								perror("[ERROR] fork");
								exit(-1);
							case 0:
								// child
								close(listenfd);
								process(new_sock);
								exit(0);    // 这里必须要exit，仔细思考
							default:
								// parent
								close(new_sock);
						}
					}
				} // end of TCP


				// new UDP connection comes
				if (FD_ISSET(listenfd, &rfds)) {
					cli_len = sizeof(cli_addr);
					nrecv = recvfrom(udpfd, udp_recv_buf, sizeof(udp_recv_buf), 0, (struct sockaddr*)&cli_addr, &cli_len);
					if (nrecv < 0) {
						perror("recvfrom");
						close(udpfd);
						exit(-1);
					}

					printf("[server] udp_recv_buf = %s.\n", udp_recv_buf);

					sprintf(udp_send_buf, "ECHO BACK : %s", udp_recv_buf);

					// 最后一个参数不是指针
					nsend = sendto(udpfd, udp_send_buf, strlen(udp_send_buf), 0, (struct sockaddr*)&cli_addr, cli_len);
					printf("[server] udp_send_buf = %s.\n", udp_send_buf);


				} //end of UDP

				break;
		} // end of switch
	} // end of while(1)

	exit(0);
}
