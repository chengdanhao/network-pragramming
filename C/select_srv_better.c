#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define MAX_CONNECTIONS 10
#define RECV_BUF_SIZE 256
#define LISTEN_BACKLOG 20
#define EXCHANGE_SERVER_IP "127.0.0.1"
#define EXCHANGE_SERVER_PORT 5002

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

int main()
{
	socklen_t cli_len;
	int i, maxi = -1, nready, opt = 1, master_sock, new_sock, nrecv = 0;
	int sockfd, max_sd, cli_socks[MAX_CONNECTIONS];
	u8 recv_buf[RECV_BUF_SIZE] = {0};
	struct sockaddr_in serv_addr, cli_addr;
	fd_set rfds, active_fds;

	if ((master_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("%s : error socket.\n", __func__);
		goto exit;
	}

	if (setsockopt(master_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
	{
		printf("%s : error setsockopt.\n", __func__);
		goto exit;
	}

	memset((char*)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(EXCHANGE_SERVER_PORT);

	if (bind(master_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)))
	{
		printf("%s : error bind.\n", __func__);
		goto exit;
	}

	if (listen(master_sock, LISTEN_BACKLOG))
	{
		printf("%s : error listen.\n", __func__);
		goto exit;
	}

	FD_ZERO(&active_fds);
	FD_SET(master_sock, &active_fds);

	for (u8 i = 0; i < MAX_CONNECTIONS; i++)
	{
		cli_socks[i] = -1;
	}

	max_sd = master_sock;

	printf(">>>> recv server start\n");
	while(1) 
	{
		rfds = active_fds;
		
		/*
		 * > 0  : Success, the number of file descriptors contained in the three descriptor sets.
		 * = 0  : the timeout expires before anything interesting happens.  
		 * = -1 : errno is set to indicate the error; the file descriptor sets are unmodified, and timeout becomes undefined.
		 */
		nready = select(max_sd + 1, &rfds, NULL, NULL, NULL);
		if (-1 == nready || 0 == nready)
		{
			continue;
		}

		/* MASTER SOCKETS */
		if (FD_ISSET(master_sock, &rfds))
		{
			cli_len = sizeof(cli_addr);
			new_sock = accept(master_sock, (struct sockaddr*)&cli_addr, &cli_len);
			if (new_sock < 0)
			{
				printf("%s : error accept.\n", __func__);
				close(new_sock);
			}

			printf("new connection, sockfd = %d, ip = %s, port = %d.\n", 
						new_sock, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

			for (i = 0; i < MAX_CONNECTIONS; i++)
			{
				if (cli_socks[i] < 0)
				{
					cli_socks[i] = new_sock;

					FD_SET(new_sock, &active_fds);

					if (i > maxi)
					{
						maxi = i;
						printf(">>>> maxi = %d\n", maxi);
					}

					if (new_sock > max_sd)
					{
						max_sd = new_sock;
					}

					break;
				}
			}

			// do connection numbers reach the upper limit?
			if (i == MAX_CONNECTIONS)
			{
				printf("reach the max connections limit\n");
				close(new_sock);
				continue;
			}

			if (--nready <= 0)
			{
				continue;
			}
		}

		/* CLIENT SOCKETS */
		for (i = 0; i <= maxi; i++)
		{
			sockfd = cli_socks[i];
			printf("socks[%d / %d] = %d\n", i, maxi,  cli_socks[i]);

			if (sockfd < 0)
			{
				continue;
			}

			if (FD_ISSET(sockfd, &rfds))
			{
				memset(recv_buf, 0, sizeof(recv_buf));
				printf("---- client sockfd = %d\n", sockfd);
				nrecv = read(sockfd, recv_buf, sizeof(recv_buf) - 1);
				if (0 == nrecv || -1 == nrecv)
				{
					getpeername(sockfd , (struct sockaddr*)&cli_addr , (socklen_t*)&cli_len);
					printf("client %s:%d disconnected\n" , inet_ntoa(cli_addr.sin_addr) , ntohs(cli_addr.sin_port));
					close(sockfd);
					FD_CLR(sockfd, &active_fds);
					cli_socks[i] = -1;
				}
				else
				{
					printf("receive message: %s.\n", recv_buf);
					//handle_received_data(recv_buf);
					//close(sockfd);
				}

				if (--nready <=0)
				{
					break;
				}
			}
		}
	}

exit:
	return 0;
}

