#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]) {
	char** p_addrlist = NULL;
	char** p_alias = NULL;
	struct hostent *p_hostent = NULL;
	char ip_addr[INET_ADDRSTRLEN] = {'\0'};	// man page tells it

	if (argc != 2) {
		printf("USAGE: %s <name/addr>.\n");
		exit(-1);
	}

	if (NULL == (p_hostent = gethostbyname(argv[1]))) {
		// perror在这里失效
		printf("gethostbyname error for host : %s, error : %s.\n",
				argv[1], hstrerror(h_errno));
		exit(-1);
	}

	printf("official hostname : %s\n", p_hostent->h_name);
	
	for (p_alias = p_hostent->h_aliases; *p_alias != NULL; p_alias++) {
		printf("\t alias : %s\n", *p_alias);
	}

	switch (p_hostent->h_addrtype) {
		case AF_INET:
			p_addrlist = p_hostent->h_addr_list;
			while (NULL != *p_addrlist) {
				printf("\taddress : %s\n", 
						inet_ntop(p_hostent->h_addrtype, *p_addrlist, ip_addr, sizeof(ip_addr)));
				p_addrlist++;
			}
			break;
		default:
			printf("unknown address type.\n");
			break;
	}

	exit(0);
}
