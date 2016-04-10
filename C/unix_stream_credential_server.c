#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/un.h>			// sockaddr_un
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>

#define BUF_SIZE 256
#define UNIX_DOMAIN_PATH "/tmp/unix_domain_sockfd"
#define CONTROL_LEN (sizeof(struct cmsghdr) + sizeof(struct cmsgcred))

void sig_chld(int signo) {
	pid_t pid;
	int stat;

	while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
		printf("child %d terminate.\n", pid);
	}

	return;
}

ssize_t read_cred(int fd, void *ptr, size_t nbytes, struct cmsgcred *cmsgcreadptr) {
	int n;
	struct msghdr msg;
	struct iovec iov;
	char control[CONTROL_LEN];

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	iov.iov_base = ptr;
	iov.iov_len = nbytes;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = control;
	msg.msg_controllen = sizeof(control);
	msg.msg_flags = 0;

	if ((n = recvmsg(fd, &msg, 0)) < 0) {
		perror("recvmsg");
		return n;
	}

	cmsgcredptr->cmcred_ngroups = 0;	// 代表没有凭证返回
	if (cmsgcredptr && msg.msg_controllen > 0) {
		struct cmsghdr *cmptr = (struct cmsghdr*)control;

		if (cmptr->cmsg_len < CONTROL_LEN) {
			printf("control length = %d\n", cmptr->cmsg_len);
			exit(EXIT_FAILURE);
		}

		if (cmptr->cmsg_level != SOL_SOCKET) {
			printf("control level != SOL_SOCKET\n");
			exit(EXIT_FAILURE);
		}

		if (cmptr->cmsg_type != SCM_CREDS) {
			printf("control type != SCM_CREDS\n");
			exit(EXIT_FAILURE);
		}

		memcpy(cmsgcredptr, CMSG_DATA(cmptr), sizeof(struct cmsgcred));
	}
	
	return n;
}

void str_echo(int sockfd) {
	int i;
	ssize_t n;
	char recvline[BUF_SIZE] = {'\0'};
	char sendline[BUF_SIZE] = {'\0'};
	struct cmsgcred cred;

	while (1) {
		bzero(sendline, sizeof(sendline));
		bzero(recvline, sizeof(recvline));

		n = read_cred(sockfd, recvline, sizeof(recvline), &cred);
		if (n < 0) {
			if (EINTR == errno) {
				continue;
			}

			perror("read_cred");
			exit(EXIT_FAILURE);
		}

		if (cred.cmcred_ngroups == 0) {
			printf("no credentials returned\n");
		} else {
			printf("PID of sender = %d\n", cred.cmcred_pid);
			printf("real user ID = %d\n", cred.cmcred_uid);
			printf("real group ID = %d\n", cred.cmcred_gid);
			printf("effective user ID = %d\n", cred.cmcred_euid);
			printf("%d groups:", cred.cmcred_ngroups - 1);
			for (i = 1; i < cred.cmcred_ngroups; i++) {
				printf("%d", cred.cmcred_groups[i]);
			}
			printf("\n");
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
