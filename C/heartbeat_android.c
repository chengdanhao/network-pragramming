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
#include <cutils/log.h>

#define LOG_TAG "heartbeat"
#define hb_debug(...) ALOGD(__VA_ARGS__)
#define hb_error(s) ALOGE(s" : %s\n", strerror(errno))

#define BUF_SIZE 256
#define ALARM_INTERVAL 3
#define PEER_EXPIRE_CNT 10

int port = 5003;
char ip[16] = "192.168.88.254";

int conn_fd = 0;
int got_reply = 1;
int expire_cnt = 0;
int already_reset = 0;

int connect_server()
{
	struct sockaddr_in addr;

	conn_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fcntl(conn_fd, F_SETOWN, getpid()) != 0) {	// claim SIGIO/SIGURG signals
		hb_error("Can't claim SIGURG and SIGIO");
	}

	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_aton(ip, &addr.sin_addr);

	if (connect(conn_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
		hb_error("connect");
		close(conn_fd);
		return -1;;
	}

	return 0;
}

void sig_handler(int signo)
{
	char c;

	if (signo == SIGURG) {
		recv(conn_fd, &c, sizeof(c), MSG_OOB);
		got_reply = (c == 's');			// got server heartbeat
		already_reset = 0;

		hb_debug("[server is alive]\n");
	} else if (signo == SIGALRM) {		// send heartbeat periodically
		alarm(ALARM_INTERVAL);

		if (got_reply) {
			send(conn_fd, "c", 1, MSG_OOB);
			got_reply = 0;
		} else {
			close(conn_fd);

			if (0 == already_reset)
			{
				hb_debug("RESET PEER : lost connection to server!\n");
				already_reset = 1;
			}
		}
	}
}

int main()
{
	struct sigaction act;
	char buf[BUF_SIZE];

	bzero(&act, sizeof(act));
	act.sa_handler = sig_handler;
	act.sa_flags = SA_RESTART;
	sigaction(SIGURG, &act, 0);
	sigaction(SIGALRM, &act, 0);

	while (1) {
		if (0 == connect_server()) {
			alarm(ALARM_INTERVAL);
			got_reply = 1;
			recv(conn_fd, buf, sizeof(buf), 0);
			close(conn_fd);
			got_reply = 0;
		} else if (PEER_EXPIRE_CNT == expire_cnt++ && 0 == already_reset) {
			hb_debug("RESET PEER : maybe not start up.\n");
			expire_cnt = 0;
		}

		// SIGARLM will make sleep invalid, but we should better and
		// this line to avoid FAST while loop
		sleep(3);
	}

	close(conn_fd);
	return 0;
}
