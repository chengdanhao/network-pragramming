#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <resolv.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define DELAY 5            /*seconds */
int srv_fd, got_reply = 1;

void sig_handler(int signo)
{
	if (signo == SIGURG) {
		char c;
		recv(srv_fd, &c, sizeof(c), MSG_OOB);
		got_reply = (c == 's');			// got server heartbeat
		printf("[server is alive]\n");
	} else if (signo == SIGALRM) {		// send heartbeat periodically
		if (got_reply) {
			send(srv_fd, "c", 1, MSG_OOB);
			alarm(DELAY);
			got_reply = 0;
		} else {
			fprintf(stderr, "Lost connection to server!");
		}
	}
}

int main(int argc, char *argv[])
{
	struct sockaddr_in addr;
	struct sigaction act;
	int bytes;
	char line[100];

	if (argc != 3) {
		printf("usage: %s <ip> <port>\n", argv[0]);
		exit(0);
	}

	bzero(&act, sizeof(act));
	act.sa_handler = sig_handler;
	act.sa_flags = SA_RESTART;
	sigaction(SIGURG, &act, 0);
	sigaction(SIGALRM, &act, 0);

	srv_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fcntl(srv_fd, F_SETOWN, getpid()) != 0) {	// claim SIGIO/SIGURG signals
		perror("Can't claim SIGURG and SIGIO");
	}

	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(argv[2]));
	inet_aton(argv[1], &addr.sin_addr);

	if (connect(srv_fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
		alarm(DELAY);

		// simply echo back
		do {
			gets(line);
			send(srv_fd, line, strlen(line), 0);
			bytes = recv(srv_fd, line, sizeof(line), 0);
			printf("recv echo back [%s]\n", line);
		}
		while (bytes > 0);
	} else {
		perror("connect failed");
	}

	close(srv_fd);
	return 0;
}
