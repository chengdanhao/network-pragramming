#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 		//read, write
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>			// errno
#include <sys/socket.h>
#include <arpa/inet.h>		// hotnl, hotns

#define PORT 10001
#define BUF_SIZE 256

void process(int sock);
void sig_child(int sig_no);
void sig_int_parent(int sig_no);
void sig_int_child(int sig_no);

int master_sock, new_sock;

int main(int argc, char* argv[]) {
	int n;
	struct sockaddr_in serv_addr, cli_addr;
	socklen_t cli_len;
	pid_t pid;
	struct sigaction child_int;

	signal(SIGCHLD, sig_child);
	signal(SIGINT, sig_int_parent);

	master_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (master_sock < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	bzero((char*)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(master_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	if (listen(master_sock, 10) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	cli_len = sizeof(cli_addr);

	while(1) {
		new_sock = accept(master_sock, (struct sockaddr*)&cli_addr, &cli_len);
		if (new_sock < 0) {
			if (EINTR == errno){
				continue;
			}
			perror("accept");
			exit(EXIT_FAILURE);
		}

		pid = fork();
		switch (pid) {
			case -1:
				perror("fork");
				close(new_sock);
				exit(EXIT_FAILURE);
			case 0:
				// child
				child_int.sa_handler = sig_int_child;
				child_int.sa_flags = 0;
				sigaction(SIGINT, &child_int, NULL);

				close(master_sock);
				process(new_sock);
				exit(EXIT_SUCCESS);	// 这里必须要exit，仔细思考
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
		exit(EXIT_FAILURE);
	}

	printf("Received message: %s.\n", recv_buf);

	n = write(sock, response, strlen(response));
	if (n < 0) {
		perror("write");
		exit(EXIT_FAILURE);
	}
}

void sig_child(int sig_no) {
	pid_t pid;
	int stat;

	if (SIGCHLD == sig_no) {
		printf("recv SIGCHLD signal.\n");
	}

	while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
		printf("process (%d) terminate.\n", pid);
	}

	exit(EXIT_SUCCESS);
}

void sig_int_parent(int sig_no) {
	printf("parent recv SIGINT signal.\n");

	close(master_sock);
	exit(EXIT_SUCCESS);
}

void sig_int_child(int sig_no) {
	printf("child recv SIGINT signal.\n");

	close(new_sock);
	exit(EXIT_SUCCESS);
}
