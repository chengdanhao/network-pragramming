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
#include <sys/wait.h>
#include <sys/types.h>

#define BUF_SIZE 256

ssize_t read_fd(int fd, void *ptr, size_t nbytes, int *recvfd) {
	struct msghdr msg;
	struct iovec iov;
	ssize_t n;

	union {
		struct cmsghdr cm;
		char control[CMSG_SPACE(sizeof(int))];
	}control_un;
	struct cmsghdr* cmptr;
	
	msg.msg_control = control_un.control;
	msg.msg_controllen = sizeof(control_un.control);
	
	msg.msg_name = NULL;
	msg.msg_namelen = 0;

	iov.iov_base = ptr;
	iov.iov_len = nbytes;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	if ((n = recvmsg(fd, &msg, 0)) <= 0) {
		perror("recvmsg");
		return n;
	}

	if (NULL != (cmptr = CMSG_FIRSTHDR(&msg))
		&& CMSG_LEN(sizeof(int)) == cmptr->cmsg_len) {
		if (SOL_SOCKET != cmptr->cmsg_level) {
			printf("SOL_SOCKET != cmptr->cmsg_level");
			exit(EXIT_FAILURE);
		}

		if (SCM_RIGHTS != cmptr->cmsg_type) {
			printf("SCM_RIGHTS != cmptr->cmsg_type");
			exit(EXIT_FAILURE);

		}

		*recvfd = *((int *) CMSG_DATA(cmptr));
	} else {
		*recvfd = -1;
	}


	return n;
}

int my_open(const char *pathname, int mode) {
	pid_t pid;
	int fd, sockfd[2], status;
	char recv_buf[BUF_SIZE], argsockfd[10], argmode[10];

	if (socketpair(AF_LOCAL, SOCK_STREAM, 0, sockfd) < 0) {
		perror("socketpair");
		exit(EXIT_FAILURE);
	}

	pid = fork();
	if (pid > 0) {				// parent process
		// close the end we don't need
		close(sockfd[0]);

		// http://www.cnblogs.com/xiedan/archive/2009/10/25/1589584.html
		if (waitpid(pid, &status, 0) < 0) {
			perror("waitpid");
			exit(EXIT_FAILURE);
		}

		if (0 == WIFEXITED(status)) {
			printf("child did not terminate\n");
			exit(EXIT_FAILURE);
		}

		// only when WIFEXITED return true, we can use WEXITSTATUS
		if (0 == (status = WEXITSTATUS(status))) {	// exit success
			bzero(recv_buf, sizeof(recv_buf));
			read_fd(sockfd[1], recv_buf, sizeof(recv_buf), &fd);
			printf("sockfd[1] = %d, recv_buf = %s, fd = %d.\n", sockfd[1], recv_buf, fd);
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
		execl("./openfile", "openfile", argsockfd, pathname, argmode, (char*) NULL);
		
		// 只有出错才会走到这里
		printf("execl error\n");
		exit(EXIT_FAILURE);
	} else {					// fork error
		perror("fork");
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char* argv[]) {
	int fd, n;
	char buff[BUF_SIZE];
	
	if (2 != argc) {
		printf("USAGE : %s <pathname>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// 把my_open替换为open，程序直接输出到标准输出
	if ((fd = my_open(argv[1], O_RDONLY)) < 0) {
		perror("my_open");
		exit(EXIT_FAILURE);
	}

	while ((n = read(fd, buff, BUF_SIZE)) > 0) {
		write(STDOUT_FILENO, buff, n);
	}

	exit(EXIT_SUCCESS);
}
