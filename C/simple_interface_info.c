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
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>

#define IFI_NAME 16
#define IFI_HADDR 8

#define IP_LEN 16
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
};

#define IFI_ALIAS 1

struct ifi_info *get_ifi_info(int, int);
void free_ifi_info(struct ifi_info *);

void sock_ntop_host(struct sockaddr* sa, char ip[]) {
	struct sockaddr_in *sin;
	struct sockaddr_in6 *sin6;

	switch (sa->sa_family) {
		case AF_INET:
			printf("AF_INET.\n");
			sin = (struct sockaddr_in*)sa;
			if (NULL == inet_ntop(AF_INET, &(sin->sin_addr), ip, IP_LEN)) {
				printf("inet_ntop : %s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}
			break;
		case AF_INET6:
			printf("AF_INET6.\n");
			sin6 = (struct sockaddr_in6*)sa;
			if (NULL == inet_ntop(AF_INET6, &(sin6->sin6_addr), ip, IP_LEN)) {
				printf("inet_ntop : %s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}
			break;
		default:
			printf("unknown type.\n");
			exit(EXIT_FAILURE);
	}
}

struct ifi_info* get_ifi_info(int family, int doaliases) {
	struct ifi_info *ifi, *ifihead, **ifipnext;
	int sockfd, len, lastlen, flags, myflags, idex = 0, hlen = 0;
	char *ptr, *buf, lastname[IFNAMSIZ], *cptr, *haddr, *sdlname;
	struct ifconf ifc;
	struct ifreq *ifr, ifrcopy;
	struct sockaddr_in *sinptr;
	struct sockaddr_in6 *sin6ptr;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0) < 0)) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	lastlen = 0;

	len = 100 * sizeof(struct ifreq);

	while (1) {
		if (NULL == (buf = (char *)malloc(len))) {
			perror("malloc");
			exit(EXIT_FAILURE);
		}

		ifc.ifc_len = len;
		ifc.ifc_buf = buf;
		if (ioctl(sockfd, SIOCGIFCONF, &ifc) < 0) {
			if (errno != EINVAL || lastlen != 0) {
				printf("ioctl error\n");
				exit(EXIT_FAILURE);
			} else {
				if (ifc.ifc_len == lastlen) {	// buffer is big enough
					break;
				}
				lastlen = ifc.ifc_len;
			}

			len += 10 * sizeof(struct ifreq);
			free(buf);
		}

		ifihead = NULL;
		ifipnext = &ifihead;
		lastname[0] = 0;
		sdlname = NULL;
		for (prt = buf; ptr < buf + ifc.ifc_len; ) {
			ifr = (struct ifreq *) ptr;

			switch (ifr->ifr_addr.sa_family) {
				case AF_INET6:
					len = sizeof(struct sockaddr_in6);
					break;
				case AF_INET:
				default:
					len = sizeof(struct sockaddr);
					break;
			}

			ptr += sizeof(ifr->ifr_name) + len;

			if (ifr->ifr_addr.sa_family != family) {
				continue;	// ignore if not desired address family
			}

			myflags = 0;
			if (NULL != (cptr = strchr(ifr->ifr_name, ":"))) {
				*cptr = 0;
			}
			if (0 == strncmp(lastname, ifr->ifr_name, IFNAMSIZ)) {
				if (0 == doaliases) {
					continue;
				}
				myflags = IFI_ALIAS;
			}
			memcpy(lastname, ifr->ifr_name, IFNAMSIZ);

			ifrcopy = *ifr;
			ioctl(sockfd, SIOCGIFFLAGS, &ifrcopy);
			flags = ifrcopy.ifr_flags;
			if (0 == (flags & IFF_UP)) {
				continue;
			}
		}
	}
}

int main(int argc, char *argv[]) {
	struct ifi_info *ifi, *ifihead;
	struct sockaddr *sa;
	unsigned char *ptr;
	int i, family, doaliases;
	char ip[16] = {'\0'};

	if (3 != argc) {
		printf("USAGE : %s <inet4|inet6> <doaliases>\n", argv[0]);
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
		printf("%s: ", ifi->ifi_name);
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
		printf("\n");

		if (ifi->ifi_mtu != 0) {
			printf("   MTU: %d\n", ifi->ifi_mtu);
		}

		if (NULL != (sa = ifi->ifi_addr)) {
			sock_ntop_host(sa, ip);
			printf("   IP addr: %s\n", ip);
		}
		if (NULL != (sa = ifi->ifi_brdaddr)) {
			sock_ntop_host(sa, ip);
			printf("   broadcast addr: %s\n", ip);
		}
		if (NULL != (sa = ifi->ifi_dstaddr)) {
			sock_ntop_host(sa, ip);
			printf("   destination addr: %s\n", ip);
		}
	}

	free_ifi_info(ifihead);
	exit(EXIT_SUCCESS);
}
