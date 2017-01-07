#include <netinet/in.h>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include <netdb.h>
#include <unistd.h>

#define BUF_SIZE 1024

int main()
{
	int sock,b;
	char p[BUF_SIZE];
	struct sockaddr_in server_addr;
	struct hostent *host;
	host=gethostbyname("localhost");
	if((sock=socket(AF_INET,SOCK_STREAM,0))==-1)
	{
		perror("socket");
		exit(0);
	}
	server_addr.sin_family=AF_INET;
	server_addr.sin_port=htons(7842);
	server_addr.sin_addr = *((struct in_addr *)host->h_addr);
	memset(&(server_addr.sin_zero),'\0',8);
	if((connect(sock,(struct sockaddr *)&server_addr,sizeof(struct sockaddr)))==-1)
	{
		perror("connect\n");
		exit(0);
	}
	while (1) {
		if(-1 == send(sock, "Hi\n", 4, 0))
			perror("send");

		if (0 > (b = recv(sock, p, BUF_SIZE - 1, 0)))
			perror("recv");
		p[b] = 0;
		printf("%s\n", p);
		sleep(1);
	}
	close(sock);
	return(0);
}
