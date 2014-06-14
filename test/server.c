#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>   
#include <netinet/in.h>       
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define BUF_SIZE            1024
#define PORT_NUM            8888
#define PEND_CONNECTIONS     100
#define TRUE                   1
#define FALSE                  0

#define SEND_STR	"pawan\n"
#define SEND_STR_LEN	sizeof(SEND_STR)

void *my_thread(void * arg)
{
	unsigned int    myClient_s;
	char           buf[BUF_SIZE];
	char           *temp;
	unsigned int   buf_len;

	myClient_s = *(unsigned int *)arg;
	while (1) {
		if(-1 == send(myClient_s, SEND_STR, SEND_STR_LEN, 0)) {
			perror("send");
			break;
		}
		if (-1 == recv(myClient_s, buf, BUF_SIZE, 0)) {
			perror("recv");
			break;
		}
		printf("%s", buf);
	}
	close(myClient_s);
	pthread_exit(NULL);
}

int main(void)
{
	unsigned int          server_s;
	struct sockaddr_in    server_addr;
	struct sockaddr_in    client_addr;
	int                   addr_len;
	pthread_attr_t        attr;
	pthread_t             threads;
	unsigned int	      client_s;

	server_s = socket(AF_INET, SOCK_STREAM, 0);

	int x = 1;
	if (setsockopt(server_s, SOL_SOCKET, SO_REUSEADDR, &x, sizeof(x)) == -1) 
	{ 
		perror("Setsockopt");
		exit(FALSE);
	} 
	signal(SIGPIPE, SIG_IGN);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT_NUM);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(server_s, (struct sockaddr *)&server_addr, sizeof(server_addr));

	listen(server_s, PEND_CONNECTIONS);

	pthread_attr_init(&attr);
	while(TRUE)
	{
		printf("my server is ready ...\n");  

		addr_len = sizeof(client_addr);
		if (FALSE == (client_s = accept(server_s, (struct sockaddr *)&client_addr, &addr_len))) {
			perror("accept");
			continue;
		}

		printf("a new client arrives ...\n");  
		{
			unsigned int ids;
			ids = client_s;
			if (pthread_create (&threads, &attr, my_thread, &ids))
				break;
		}
	}
	close (server_s);
	return (TRUE);
}
