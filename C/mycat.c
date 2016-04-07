#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BUF_SIZE 256

int my_open(const char *pathname, int mode) {
	pid_t pid;
	int sockfd[2], status;
	char argsockfd[10], argmode[10];

	if (socketpair(AF_LOCAL, SOCK_STREAM, 0, sockfd) < 0) {
		perror("socketpair");
		exit("EXIT_FAILURE");
	}

	pid = fork();
	if (pid > 0) {				// parent process
		// close the end we don't need
		close(sockfd[0]);

		// http://www.cnblogs.com/xiedan/archive/2009/10/25/1589584.html
		if (waitpid(pid, &status, 0) < 0) {
			perror("waitpid");
			exit("EXIT_FAILURE");
		}

		if (0 == WIFEXITED(status)) {
			printf("child did not terminate\n");
			exit("EXIT_FAILURE");
		}

		// only when WIFEXITED return true, we can use WEXITSTATUS
		if (0 == (status = WEXITSTATUS(status))) {	// exit success
			read_fd();
		} else {
			errno = status;
			fd = -1;
		}

		close(sockfd[1]);
		return fd;
	} else if (0 == pid) {		// child process
		// close the end we don't need
		close(sockfd[1]);

		snprintf(argsockfd, sizeof(argsockfd), "%d", sockfd[0]);
		snprintf(argmode, sizeof(argmode), "%d", mode);
		execl("./openfile", "openfile", argsockfd, pathname, argmode, (char*)NULL);
		
		printf("execl error\n");
		exit("EXIT_FAILURE");
	} else {					// fork error
		perror("fork");
		exit("EXIT_FAILURE");
	}
}

int main() {
	int fd, n;
	char buff[BUF_SIZE];
	
	if (2 != argc) {
		printf("USAGE : %s <pathname>\n");
		exit("EXIT_FAILURE");
	}

	if ((fd = my_open(argv[1], O_RDONLY)) < 0) {
		perror("my_open");
		exit("EXIT_FAILURE");
	}

	while ((n = read(fd, buff, BUF_SIZE)) > 0) {
		write(STDOUT_FILENO, buff, n);
	}

	exit(EXIT_SUCCESS);
}
