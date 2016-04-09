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

ssize_t write_fd(int fd, void *ptr, size_t nbytes, int sendfd) {
	struct msghdr msg;
	struct iovec iov;

	union {
		struct cmsghdr cm;
		char control[CMSG_SPACE(sizeof(int))];
	} control_un;
	struct cmsghdr *cmptr;

	msg.msg_control = control_un.control;
	msg.msg_controllen = sizeof(control_un.control);
	cmptr = CMSG_FIRSTHDR(&msg);
	cmptr->cmsg_len = CMSG_LEN(sizeof(int));
	cmptr->cmsg_level = SOL_SOCKET;
	cmptr->cmsg_type = SCM_RIGHTS;
	*((int *) CMSG_DATA(cmptr)) = sendfd;

	msg.msg_name = NULL;
	msg.msg_namelen = 0;

	iov.iov_base = ptr;
	iov.iov_len = nbytes;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	return (sendmsg(fd, &msg, 0));
}

int main(int argc, char* argv[]) {
	int fd;
	char send_buf[BUF_SIZE] = "hellowrold";

	if (4 != argc) {
		printf("USAGE : %s <sockfd> <filename> <mode>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if ((fd = open(argv[2], atoi(argv[3]))) < 0) {
		perror("open");
		exit(errno > 0 ? errno : 255);
	}

	fprintf(stdout, "<recv> %s %s %s %s\n", argv[0], argv[1], argv[2], argv[3]);
	fprintf(stdout, "<send> %s %d\n", send_buf, fd);

	if (write_fd(atoi(argv[1]), send_buf, strlen(send_buf), fd) < 0) {
		perror("write_fd");
		exit(errno > 0 ? errno : 255);
	}

	exit(EXIT_SUCCESS);
}
