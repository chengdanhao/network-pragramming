#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>

#define IFI_NAME 16
#define IFI_HADDR 8
#define BUF_SIZE 256

struct ifi_info {
	char ifi_name[IFI_NAME];
	short ifi_index;
	short ifi_mtu;
	unsigned char ifi_haddr[IFI_HADDR];
	unsigned short ifi_hlen;
	short ifi_flags;
	short ifi_myflags;
	struct sockaddr *ifi_addr;
	struct sockaddr *ifi_brdaddr;
	struct sockaddr *ifi_dstaddr;
	struct ifi_info *ifi_next;
}

#define IFI_ALIAS 1

struct ifi_info *get_ifi_info(int, int);
void free_ifi_info(struct ifi_info *);

void sock_ntop_host(struct sockaddr* sa) {
	char str[BUF_SIZE] = {'\0'};
	struct sockaddr_in *sin;
	struct sockaddr_in6 *sin6;

	switch (sa->sa_family) {
		case AF_INET:
			printf("AF_INET.\n");
			sin = (struct sockaddr_in*)ss;
			if (NULL == inet_ntop(AF_INET, &(sin->sin_addr), str, sizeof(str))) {
				printf("inet_ntop : %s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}
			break;
		case AF_INET6:
			printf("AF_INET6.\n");
			sin6 = (struct sockaddr_in6*)ss;
			if (NULL == inet_ntop(AF_INET6, &(sin6->sin6_addr), str, sizeof(str))) {
				printf("inet_ntop : %s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}
			break;
		default:
			printf("unknown type.\n");
			exit(EXIT_FAILURE);
	}

	printf("connect to %s\n", str);
}

int main(int argc, char *argv[]) {
	struct ifi_info *ifi, *ifihead;
	struct sockaddr *sa;
	unsigned char *ptr;
	int i, family, doaliases;

	if (3 != argc) {
		printf("USAGE : %s <inet4|inet6> <doaliases>\n");
		exit(EXIT_FAILURE);
	}

	if (!strcmp(argv[1], "inet4")) {
		family = AF_INET;
	} else if (!strcmp(argv[1], "inet6")) {
		family = AF_INET6;
	} else {
		printf("invalid address family\n");
		exit(EXIT_FAILURE);
	}

	doaliases = atoi(argv[2]);

	for (ifihead = ifi = get_ifi_info(family, doaliases);
		ifi != NULL; ifi = ifi->ifi_next) {
		printf("%s: ", ifi->ifi_next);
		if (0 != ifi->ifi_index) {
			printf("(%d) ", ifi->ifi_index);
		}

		printf("<");
		if (ifi->ifi_flags & IFF_UP)			printf("UP ");
		if (ifi->ifi_flags & IFF_BROADCAST)		printf("BCAST ");
		if (ifi->ifi_flags & IFF_MULTICAST)		printf("MCAST ");
		if (ifi->ifi_flags & IFF_LOOPBACK)		printf("LOOP ");
		if (ifi->ifi_flags & IFF_POINTOPOINT)	printf("P2P ");
		printf(">\n");

		if ((i = ifi->ifi_hlen) > 0) {
			ptr = ifi->ifi_haddr;
			do {
				printf("%s%x", (i == ifi->ifi_hlen) ? "   " : ":", *ptr++);
			} while (--i > 0);
		}
		printf('\n');

		if (ifi->ifi_mtu != 0) {
			printf("   MTU: %d\n", ifi->ifi_mtu);
		}

		if (NULL != (sa = ifi->ifi_addr)) {
			printf("   IP addr: %s\n", sock_ntop_host(*sa));
		}
		if (NULL != (sa = ifi->ifi_brdaddr)) {
			printf("   broadcast addr: %s\n", sock_ntop_host(*sa));
		}
		if (NULL != (sa = ifi->ifi_dstaddr)) {
			printf("   destination addr: %s\n", sock_ntop_host(*sa));
		}


	}

	free_ifi_info(ifihead);
	exit(EXIT_SUCCESS);
}
