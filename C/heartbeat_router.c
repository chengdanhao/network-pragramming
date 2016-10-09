#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <resolv.h>
#include <signal.h>
#include <sys/wait.h>

#define BUF_SIZE 256
#define ALARM_INTERVAL 2
#define PEER_EXPIRE_CNT 30
#define LISTEN_PORT 5003
#define FILENAME "/tmp/heartbeat"

int cli_fd;
pid_t child_pid;
int already_reset = 0;
struct sigaction act;

void write_expire_cnt(int v) {
	int fd;
	char str[25];

	sprintf(str, "%d", v);
	fd = open(FILENAME, O_RDWR|O_CREAT|O_TRUNC);
	write(fd, str, strlen(str) + 1);
	close(fd);
}

int read_expire_cnt()
{
	int fd;
	char str[25];

	fd = open(FILENAME, O_RDONLY);
	read(fd, str, sizeof(str));
	close(fd);
	return (atoi(str));
}

void sig_handler(int signo)
{
	char c;
	int expire_cnt;

	if (signo == SIGURG) {
		recv(cli_fd, &c, sizeof(c), MSG_OOB);
		if (c == 'c') {
			printf("recv SIGURG\n");
			send(cli_fd, "s", 1, MSG_OOB);
		}

		write_expire_cnt(0);
		already_reset = 0;
	} else if (signo == SIGALRM) {
		alarm(ALARM_INTERVAL);
		expire_cnt = read_expire_cnt();
		printf("expire_cnt = %d, already_reset = %d.\n", expire_cnt, already_reset);

		if (PEER_EXPIRE_CNT == expire_cnt && 0 == already_reset) {
			printf("RESET PEER : maybe not start up.\n");
			close(cli_fd);

			if (child_pid > 0) {
				kill(child_pid, SIGKILL);
			}

			system("echo 1 > /sys/android/reboot");
			write_expire_cnt(0);
			already_reset = 1;
		}

		write_expire_cnt(++expire_cnt);
	} else if (signo == SIGCHLD) {
		wait(0);
	}
}

void servlet(void)
{
	char buf[BUF_SIZE];

	bzero(&act, sizeof(act));
	act.sa_handler = sig_handler;
	act.sa_flags = SA_RESTART;
	sigaction(SIGURG, &act, 0);

	if (fcntl(cli_fd, F_SETOWN, getpid()) != 0) {	// claim SIGIO/SIGURG signals
		perror("Can't claim SIGIO and SIGURG");
	}

	recv(cli_fd, buf, sizeof(buf), 0);
	printf("RESET PEER : socket have been shutdown\n");
	system("echo 1 > /sys/android/reboot");

	close(cli_fd);
	exit(0);
}

int main(int argc, char *argv[])
{
	int opt = 1;
	int listen_fd;
	struct sockaddr_in addr;
	pid_t pid;

	bzero(&act, sizeof(act));
	act.sa_handler = sig_handler;
	act.sa_flags = SA_NOCLDSTOP | SA_RESTART;
	sigaction(SIGCHLD, &act, 0);
	sigaction(SIGALRM, &act, 0);

	listen_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ) {
		perror("setsockopt");
		exit(-1);
	}

	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(LISTEN_PORT);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
		perror("bind()");
	}

	listen(listen_fd, 15);

	alarm(ALARM_INTERVAL);

	for (;;) {
		printf("waiting for a scoket.\n");
		cli_fd = accept(listen_fd, 0, 0);
		printf("accept %d socket.\n", cli_fd);
		if (cli_fd > 0) {
			if ((pid = fork()) == 0) {
				close(listen_fd);
				servlet();
			} else {
				child_pid = pid;
				close(cli_fd);
			}
		} else {
			perror("accept()");
		}
	}

	close(listen_fd);
	return 0;
}
