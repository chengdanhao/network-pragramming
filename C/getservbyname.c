#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#define BUF_SIZE 256

int main(int argc, char* argv[]) {
	struct in_addr inetaddr;
	struct in_addr* inetaddrlist[2];
	struct in_addr** p_addrlist;
	struct servent* p_servent;
	int sockfd, n;
	struct sockaddr_in servaddr;
	char recvline[BUF_SIZE] = {'\0'};
	struct hostent *p_hostent = NULL;
	char ip_addr[INET_ADDRSTRLEN] = {'\0'};	// according to man page,we use INET_ADDRSTRLEN

	if (argc != 3) {
		printf("USAGE: %s <hostname> <service>.\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if (NULL == (p_hostent = gethostbyname(argv[1]))) {
		// gethostbyname的错误保存在全局的h_errno里面
		printf("gethostbyname error(%s), try to user inet_aton.\n", hstrerror(h_errno));

		if (0 == inet_aton(argv[1], &inetaddr)) {
			perror("inet_aton");
			exit(EXIT_FAILURE);
		} else {
			inetaddrlist[0] = &inetaddr;
			inetaddrlist[1] = NULL;
			p_addrlist = inetaddrlist;
		}
	} else {
		printf("official hostname : %s\n", p_hostent->h_name);
		p_addrlist = (struct in_addr**)p_hostent->h_addr_list;
	}

	// 为什么./a.out baidu.com smb会报Success的错误
	if (NULL == (p_servent = getservbyname(argv[2], "tcp"))) {
		perror("getservbyname");
		exit(EXIT_FAILURE);
	}

	while (NULL != *p_addrlist) {
		if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			perror("socket");
			exit(EXIT_FAILURE);
		}

		bzero(&servaddr, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_port = p_servent->s_port;
		memcpy(&servaddr.sin_addr, *p_addrlist, sizeof(struct in_addr));

		printf("trying to connect %s\n", inet_ntop(AF_INET, *p_addrlist, ip_addr, sizeof(ip_addr)));

		if (0 == connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) {
			break;
		}

		perror("connect error");

		p_addrlist++;
	}

	if (NULL == *p_addrlist) {
		printf("unable to connect.\n");
		exit(EXIT_FAILURE);
	}

	if (read(sockfd, recvline, BUF_SIZE - 1) < 0) {
		perror("read");
		exit(EXIT_FAILURE);
	}

	fputs(recvline, stdout);

	exit(EXIT_SUCCESS);
}
