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
#include <unistd.h>

#define BUF_SIZE            1024
#define PORT_NUM            8888
#define PEND_CONNECTIONS     100
#define TRUE                   1
#define FALSE                  0


char *send_str = "pawan\n";
int lport = 8888;

void server_get_args(int argc, char * argv[])
{
	int c;
	while ((c=getopt(argc, argv, "s:p:")) != EOF)
	{
		switch(c)
		{
			case 's':
				send_str = optarg;
				break;
			case 'p':
				lport = atoi(optarg); 
				break;
			default:
				//usage();
				;
		}
	}
}


void *my_thread(void * arg)
{
	unsigned int    myClient_s;
	char           buf[BUF_SIZE];
	char           *temp;
	unsigned int   buf_len;

	myClient_s = *(unsigned int *)arg;
	while (1) {
		if (-1 == recv(myClient_s, buf, BUF_SIZE, 0)) {
			perror("recv");
			break;
		}

		printf("%s", buf);

		if(-1 == send(myClient_s, send_str, strlen(send_str), 0)) {
			perror("send");
			break;
		}
	}
	close(myClient_s);
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
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
	
	server_get_args(argc, argv);

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(lport);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(server_s, (struct sockaddr *)&server_addr, sizeof(server_addr));

	listen(server_s, PEND_CONNECTIONS);

	pthread_attr_init(&attr);
	while(TRUE)
	{
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
