#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <resolv.h>
#include <signal.h>
#include <sys/wait.h>

int cli_fd;
struct sigaction act;

void sig_handler(int signo)
{
	if (signo == SIGURG) {
		char c;
		recv(cli_fd, &c, sizeof(c), MSG_OOB);
		if (c == 'c') {
			send(cli_fd, "s", 1, MSG_OOB);
		}
	} else if (signo == SIGCHLD) {
		wait(0);
	}
}

void servlet(void)
{
	int bytes;
	char buf[1024];

	bzero(&act, sizeof(act));
	act.sa_handler = sig_handler;
	act.sa_flags = SA_RESTART;
	sigaction(SIGURG, &act, 0);

	if (fcntl(cli_fd, F_SETOWN, getpid()) != 0) {	// claim SIGIO/SIGURG signals
		perror("Can't claim SIGIO and SIGURG");
	}

	// simply echo back
	do {
		bzero(buf, sizeof(buf));
		bytes = recv(cli_fd, buf, sizeof(buf), 0);
		if (bytes > 0) {
			send(cli_fd, buf, bytes, 0);
			printf("send echo back [%s]\n", buf);
		}
	} while (bytes > 0);

	close(cli_fd);
	exit(0);
}

int main(int count, char *strings[])
{
	int listen_fd;
	struct sockaddr_in addr;

	if (count != 2) {
		printf("usage: %s <port>\n", strings[0]);
		exit(0);
	}

	bzero(&act, sizeof(act));
	act.sa_handler = sig_handler;
	act.sa_flags = SA_NOCLDSTOP | SA_RESTART;
	if (sigaction(SIGCHLD, &act, 0) != 0) {
		perror("sigaction()");
	}

	listen_fd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(strings[1]));
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
		perror("bind()");
	}

	listen(listen_fd, 15);

	for (;;) {
		cli_fd = accept(listen_fd, 0, 0);
		if (cli_fd > 0) {
			if (fork() == 0) {
				close(listen_fd);
				servlet();
			} else {
				close(cli_fd);
			}
		} else {
			perror("accept()");
		}
	}

	close(listen_fd);
	return 0;
}
