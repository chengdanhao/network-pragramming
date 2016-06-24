#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define EXCHANGE_SERVER_IP "127.0.0.1"
#define EXCHANGE_SERVER_PORT 5002

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

int g_connfd;
char g_own_exchange_info_str[] = "test string";

void sock_init()
{
	struct sockaddr_in serv_addr;
	struct hostent* server;

	//while(1)
	{
		g_connfd = socket(AF_INET, SOCK_STREAM, 0);
		if (g_connfd < 0) {
			printf("%s : error socket\n", __func__);
			//continue;
		}

		// 这里的IP得好好想一想
		server = gethostbyname(EXCHANGE_SERVER_IP);
		if (NULL == server) {
			printf("%s : error gethostbyname\n", __func__);
			close(g_connfd);
			//continue;
		}

		memset((char*)&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		memcpy((char*)&(serv_addr.sin_addr.s_addr), (char*)server->h_addr, server->h_length);
		serv_addr.sin_port = htons(EXCHANGE_SERVER_PORT);

		if (connect(g_connfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
			printf("%s : error connect\n", __func__);
			close(g_connfd);
			//continue;
		}

		//break;
	}
}

void send_exchange_data()
{
	sock_init();
	while (1)
	{

		//sock_init();

		printf("%s str = %s\n", __func__, g_own_exchange_info_str);

		if (write(g_connfd, g_own_exchange_info_str, strlen(g_own_exchange_info_str)) < 0) {
			printf("error write\n");
			//goto finish;
			return;
		}

		sleep(2);

		//close(g_connfd);
	}
}

int main()
{
	send_exchange_data();
	return 0;
}
