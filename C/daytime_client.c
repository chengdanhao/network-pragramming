#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAXLINE 256

int main(int argc, char** argv)
{
	int socketfd, n;
	char recvline[MAXLINE + 1];
	struct sockaddr_in servaddr;

	if(argc != 2)
	{  
		printf("USAGE : %s <ip address>\n");
		exit(EXIT_FAILURE);
	}   

	if((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}  

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(13);
	if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
	{
		perror("inet_pton");
		exit(EXIT_FAILURE);
	} 

	if(connect(socketfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
	{
		perror("connect");
		exit(EXIT_FAILURE);
	} 

	while((n = read(socketfd, recvline, MAXLINE)) > 0)
	{
		recvline[n] = 0;
		if(EOF == fputs(recvline, stdout))
		{
			perror("fputs");
			exit(EXIT_FAILURE);
		} 
	}

	if(n < 0)
	{
		perror("read");
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS); 
}
