#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define EXCHANGE_SERVER_IP "127.0.0.1"
#define EXCHANGE_SERVER_PORT 5002
#define STR "test string"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;


int main()
{
	int sockfd = 0;
	struct sockaddr_in serv_addr;
	struct hostent* server;

	while (1) 
	{
		printf(">>>> sockfd = %d\n", sockfd);

		printf(">>>> recreate socket\n");
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0)
		{
			printf("%s : error socket\n", __func__);
			goto exit;

		}

		server = gethostbyname(EXCHANGE_SERVER_IP);
		if (NULL == server)
		{
			printf("%s : error gethostbyname\n", __func__);
			goto exit;

		}

		memset((char*)&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		memcpy((char*)&(serv_addr.sin_addr.s_addr), (char*)server->h_addr, server->h_length);
		serv_addr.sin_port = htons(EXCHANGE_SERVER_PORT);

		if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
		{
			perror("connect");
			goto exit;

		}

		printf(">>>> directly send\n");
		if (write(sockfd, STR, strlen(STR)) < 0) {
			printf("error write\n");
			goto exit;

		}

		printf("send msg : %s\n", STR);
exit:
		close(sockfd);
		printf("\n");

		usleep(10);
	}

	return 0;
}
