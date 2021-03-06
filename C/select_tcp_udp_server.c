#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>

#define BUF_SIZE 256
#define PORT 10001

#define BACKLOG 5

void sig_child(int sig_no) {
	pid_t pid;
	int stat;

	if (SIGCHLD == sig_no) {
		printf("recv SIGCHLD signal.\n");
	}

	while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
		printf("process (%d) terminate.\n", pid);
	}
}

void process(int sock) {
	int n;
	char recv_buf[BUF_SIZE] = {'\0'};
	char* response = "I got your message";

	while (1) {
		bzero(recv_buf, sizeof(recv_buf));
		n = read(sock, recv_buf, sizeof(recv_buf) - 1);
		if (0 == n) {
			printf("client has been closed.\n");
			break;
		} else if (n < 0 && EINTR == errno) {
			continue;
		} else if (n < 0) {
			perror("read");
			close(sock);
			exit(-1);
		}

		printf("[TCP]received message: %s.\n", recv_buf);

		n = write(sock, response, strlen(response));
		if (n < 0) {
			perror("write");
			exit(-1);
		}

		printf("[TCP]send message: %s.\n", response);
	}

	close(sock);
}

int main(int argc, char* argv[]) {
	int opt;
	pid_t pid;
	fd_set rfds;
	ssize_t nrecv;
	ssize_t nsend;
	socklen_t cli_len;
	int max_sd, listenfd, udpfd, new_sock;
	struct sockaddr_in serv_addr, cli_addr;
	char tcp_recv_buf[BUF_SIZE] = {'\0'};
	char tcp_send_buf[BUF_SIZE] = {'\0'};
	char udp_recv_buf[BUF_SIZE] = {'\0'};
	char udp_send_buf[BUF_SIZE] = {'\0'};

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
	if (udpfd < 0) {
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
				if (EINTR == errno) {	// without this condition, it will throw error interrupted system call
					continue;
				} else {
					perror("[ERROR] select");
					exit(-1);
				}
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

					printf("new tcp connection, sockfd = %d, ip = %s, port = %d.\n",
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
				} // end of TCP


				// new UDP connection comes
				if (FD_ISSET(udpfd, &rfds)) {
					cli_len = sizeof(cli_addr);
					nrecv = recvfrom(udpfd, udp_recv_buf, sizeof(udp_recv_buf), 0, (struct sockaddr*)&cli_addr, &cli_len);
					if (nrecv < 0) {
						perror("recvfrom");
						close(udpfd);
						exit(-1);
					}

					printf("[UDP] received message = %s.\n", udp_recv_buf);

					sprintf(udp_send_buf, "ECHO BACK : %s", udp_recv_buf);

					// 最后一个参数不是指针
					nsend = sendto(udpfd, udp_send_buf, strlen(udp_send_buf), 0, (struct sockaddr*)&cli_addr, cli_len);
					printf("[UDP] send message = %s.\n", udp_send_buf);


				} //end of UDP

				break;
		} // end of switch
	} // end of while(1)

	exit(0);
}
