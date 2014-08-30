//#include<sys/socket.h>
#include <netinet/in.h>//sockaddr_in,INADDR_ANY
#include<stdlib.h>//exit
#include<string.h>//memeset
#include<stdio.h>//scanf
#include <netdb.h> //host->h_addr
int main()
{
	int sock,b;
	char p[10];
	struct sockaddr_in server_addr;
	struct hostent *host;
	host=gethostbyname("localhost");
	if((sock=socket(AF_INET,SOCK_STREAM,0))==-1)
	{
		perror("socket");
		exit(0);
	}
	server_addr.sin_family=AF_INET;
	server_addr.sin_port=htons(5555);
	server_addr.sin_addr = *((struct in_addr *)host->h_addr);
	memset(&(server_addr.sin_zero),'\0',8);
	if((connect(sock,(struct sockaddr *)&server_addr,sizeof(struct sockaddr)))==-1)
	{
		perror("connect\n");
		exit(0);
	}
	while (1) {
		if (0 > (b = recv(sock, p, 1024, 0)))
			perror("recv");
		p[b]='\0';
		printf("%s",p);
		if(-1 == send(sock, "bye\n", 4, 0))
			perror("send");
		sleep(1);
	}
	close(sock);
	return(0);
}
