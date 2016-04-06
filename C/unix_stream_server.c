#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/un.h>			// sockaddr_un
#include <sys/types.h>
#include <sys/socket.h>

#define BUF_SIZE 256
#define UNIX_DOMAIN_PATH "/tmp/unix_domain_sockfd"

void sig_chld(int signo) {
	pid_t pid;
	int stat;

	while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
		printf("child %d terminate.\n", pid);
	}

	return;
}

void str_echo(int sockfd) {
	ssize_t n;
	char recvline[BUF_SIZE] = {'\0'};
	char sendline[BUF_SIZE] = {'\0'};

	while (1) {
		bzero(sendline, sizeof(sendline));
		bzero(recvline, sizeof(recvline));

		n = read(sockfd, recvline, sizeof(recvline));
		if (n < 0) {
			if (EINTR == errno) {
				continue;
			}

			perror("read");
			exit(EXIT_FAILURE);
		}

		sprintf(sendline, "[response] %s", recvline);

		if (write(sockfd, sendline, strlen(sendline)) < 0) {
			perror("write");
			exit(EXIT_FAILURE);
		}
	}
}

int main(int argc, char* argv[]) {
	pid_t pid;
	int listenfd, connfd;
	socklen_t clilen;
	struct sockaddr_un servaddr, cliaddr;

	if ((listenfd = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	unlink(UNIX_DOMAIN_PATH);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_LOCAL;
	strcpy(servaddr.sun_path, UNIX_DOMAIN_PATH);

	if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	listen(listenfd ,5);

	signal(SIGCHLD, sig_chld);

	while (1) {
		clilen = sizeof(cliaddr);
		if ((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen)) < 0) {
			if (EINTR == errno) {
				continue;
			}

			perror("accept");
			exit(EXIT_FAILURE);
		}

		pid = fork();
		if (pid == 0) {			// child
			close(listenfd);
			str_echo(connfd);
			exit(EXIT_SUCCESS);
		} else if (pid > 0) {	// parent
			close(connfd);
		} else {
			perror("fork");
			exit(EXIT_FAILURE);
		}
	}

	exit(EXIT_SUCCESS);
}
