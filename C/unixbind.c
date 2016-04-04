#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>

int main(int argc, char* argv[]) {
	int sockfd;
	socklen_t len;
	struct sockaddr_un addr1, addr2;

	if (2 != argc) {
		printf("USAGE : %s <pathname>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if ((sockfd = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	unlink(argv[1]);

	bzero(&addr1, sizeof(addr1));
	addr1.sun_family = AF_LOCAL;
	strncpy(addr1.sun_path, argv[1], sizeof(addr1.sun_path) - 1);


	if (bind(sockfd, (struct sockaddr*)&addr1, SUN_LEN(&addr1)) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}


	len = sizeof(addr2);
	if (getsockname(sockfd, (struct sockaddr*)&addr2, &len) < 0) {
		perror("getsockname");
		exit(EXIT_FAILURE);
	}

	printf("bound name = %s, returned len = %d\n", addr2.sun_path, len);

	exit(EXIT_FAILURE);
}
