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
int expire_cnt = 0;
int already_reset = 0;

#define ALARM_INTERVAL 2
#define PEER_EXPIRE_CNT 3

void sig_handler(int signo)
{
	if (signo == SIGURG) {
		char c;

		recv(cli_fd, &c, sizeof(c), MSG_OOB);
		if (c == 'c') {
			printf("recv SIGURG\n");
			send(cli_fd, "s", 1, MSG_OOB);
		}

		expire_cnt = 0;
		already_reset = 0;
	} else if (signo == SIGALRM) {
		alarm(ALARM_INTERVAL);

		if (PEER_EXPIRE_CNT == expire_cnt++ && 0 == already_reset) {
			printf("RESET PEER : maybe not start up.\n");
			expire_cnt = 0;
		}
	} else if (signo == SIGCHLD) {
		wait(0);
	}
}

int main(int count, char *strings[])
{
	struct sockaddr_in addr;
	struct sigaction act;
	int listen_fd;
	int opt = 1;
	int bytes;
	char buf[1024];

	if (count != 2) {
		printf("usage: %s <port>\n", strings[0]);
		exit(0);
	}

	bzero(&act, sizeof(act));
	act.sa_handler = sig_handler;
	act.sa_flags = SA_NOCLDSTOP | SA_RESTART;
	sigaction(SIGCHLD, &act, 0);
	sigaction(SIGURG, &act, 0);
	sigaction(SIGALRM, &act, 0);

	listen_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ) {
		perror("setsockopt");
		exit(-1);
	}

	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(strings[1]));
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
		perror("bind()");
	}

	listen(listen_fd, 15);

	alarm(ALARM_INTERVAL);

	for (;;) {
		cli_fd = accept(listen_fd, 0, 0);
		if (cli_fd > 0) {
			if (fcntl(cli_fd, F_SETOWN, getpid()) != 0) {	// claim SIGIO/SIGURG signals
				perror("Can't claim SIGIO and SIGURG");
			}

			recv(cli_fd, buf, sizeof(buf), 0);
			printf("RESET PEER : socket have been shutdown\n");
			already_reset = 1;

			close(cli_fd);
		} else {
			perror("accept()");
		}
	}

	close(listen_fd);
	return 0;
}
