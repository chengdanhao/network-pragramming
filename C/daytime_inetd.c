/*
 * [ bash ]
   daniel@v460:~/network-programming$ cat /etc/services | tail -1
   mydaytime	9999/tcp

   daniel@v460:~/network-programming$ cat /etc/inetd.conf | tail -1
   mydaytime stream tcp nowait daniel /home/daniel/code/network-pragramming/C/mydaytime mydaytime

   root@v460:/home/daniel/code/network-pragramming/C# ps aux | grep inetd
   root       667  0.0  0.0  12696  1888 ?        Ss   20:59   0:00 /usr/sbin/inetd -i
   root      3016  0.0  0.0  12892  1720 pts/0    S+   22:33   0:00 grep inetd
   root@v460:/home/daniel/code/network-pragramming/C# kill -s HUP 667

   root@v460:/home/daniel/code/network-pragramming/C# netstat -na | grep 9999
   tcp        0      0 0.0.0.0:9999            0.0.0.0:*               LISTEN   

   root@v460:/home/daniel/code/network-pragramming/C# telnet localhost 9999
   Trying ::1...
   Trying 127.0.0.1...
   Connected to localhost.
   Escape character is '^]'.
   Thu Mar 31 22:35:31 2016
   Connection closed by foreign host.


 * [ /var/log/daemon.log ]
   Mar 31 22:34:38 v460 inetd[3022]: execv /home/daniel/code/network-pragramming/C/mydaytime: No such file or directory
   Mar 31 22:35:31 v460 mydaytime[3049]: AF_INET.
   Mar 31 22:35:31 v460 mydaytime[3049]: connect to 127.0.0.1
   Mar 31 22:35:31 v460 mydaytime[3049]: time is Thu Mar 31 22:35:31 2016
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <time.h>

#define BUF_SIZE 256
#define SYS_LOG(...) syslog(LOG_INFO | LOG_DAEMON, __VA_ARGS__)

void daemon_inetd(const char *pname, int facility) {
	openlog(pname, LOG_PID, facility);
}

void print_connection_info(struct sockaddr_storage* ss) {
	char str[BUF_SIZE] = {'\0'};
	struct sockaddr *sa;
	struct sockaddr_in *sin;
	struct sockaddr_in6 *sin6;

	sa = (struct sockaddr*)ss;
	switch (sa->sa_family) {
		case AF_INET:
			SYS_LOG("AF_INET.\n");
			sin = (struct sockaddr_in*)ss;
			if (NULL == inet_ntop(AF_INET, &(sin->sin_addr), str, sizeof(str))) {
				SYS_LOG("inet_ntop : %s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}
			break;
		case AF_INET6:
			SYS_LOG("AF_INET6.\n");
			sin6 = (struct sockaddr_in6*)ss;
			if (NULL == inet_ntop(AF_INET6, &(sin6->sin6_addr), str, sizeof(str))) {
				SYS_LOG("inet_ntop : %s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}
			break;
		default:
			SYS_LOG("unknown type.\n");
			exit(EXIT_FAILURE);
	}

	SYS_LOG("connect to %s\n", str);
}

int main(int argc, char* argv[]) {
	time_t ticks;
	ssize_t n;
	socklen_t len;
	struct sockaddr_storage ss;
	char time_buf[BUF_SIZE] = {'\0'};

	if (1 != argc) {
		SYS_LOG("USAGE : %s\n", argv[0]);
	}

	daemon_inetd(argv[0], 0);

	len = sizeof(ss);
	if (getpeername(0, (struct sockaddr*)&ss, &len)) {
		SYS_LOG("getpeername : %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	print_connection_info(&ss);

	ticks = time(NULL);
	snprintf(time_buf, sizeof(time_buf), "%.24s\r\n", ctime(&ticks));
	SYS_LOG("time is %s\n", time_buf);

	n = write(0, time_buf, strlen(time_buf));
	if (n < 0 && EINTR == errno) {
		SYS_LOG("write");
		exit(EXIT_FAILURE);
	}

	close(0);

	exit(EXIT_SUCCESS);
}
