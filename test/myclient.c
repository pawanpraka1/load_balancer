#include <netinet/in.h>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>

#define BUF_SIZE 1024
#define MSG "Hi Pawan Prakash Sharma!\n"

int main()
{
	int sock, len;
	char buf[BUF_SIZE];
	struct sockaddr_in server_addr;
	struct hostent *host;

	signal(SIGPIPE, SIG_IGN);
	host = gethostbyname("localhost");
	if (NULL == host) {
		perror("gethostbyname");
		exit(-1);
	}
	if (0 > (sock = socket(AF_INET, SOCK_STREAM, 0))) {
		perror("socket");
		exit(-1);
	}

	memset(&server_addr,'\0', sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(7842);
	server_addr.sin_addr = *((struct in_addr *)host->h_addr);

	if (0 > connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr))) {
		perror("connect");
		exit(-1);
	}
	while (1) {
		if(-1 == send(sock, MSG, sizeof(MSG) - 1, 0)) {
			perror("send");
			break;
		}
		if (0 > (len = recv(sock, buf, BUF_SIZE, 0))) {
			perror("recv");
			break;
		}
		if (0 == len) {
			printf("connection closed by peer\n");
			break;
		}
		if (0 > write(1, buf, len)) {
			perror("write");
			break;
		}
		sleep(1);
	}
	close(sock);
	return(0);
}
