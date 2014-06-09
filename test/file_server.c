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


#define OK_TEXT     "HTTP/1.0 200 OK\nContent-Type:text/html\n\n"
#define NOTOK_404   "HTTP/1.0 404 Not Found\nContent-Type:text/html\n\n"
#define MESS_404    "<html><body><h1>FILE NOT FOUND</h1></body></html>"

#define BUF_SIZE            8096
#define PORT_NUM            8888
#define PEND_CONNECTIONS     100 
#define TRUE                   1
#define FALSE                  0

unsigned int    client_s;


void *my_thread(unsigned int myClient_s)
{

	char           in_buf[BUF_SIZE] = {0};
	char           out_buf[BUF_SIZE];
	char           *temp;
	char           file_name[BUF_SIZE];
	unsigned int   fh;
	unsigned int   buf_len;
	unsigned int   retcode;
	unsigned int   req_len = 0;

	while (1) {
		retcode = recv(client_s, &in_buf[req_len], BUF_SIZE - req_len, 0);
		if (retcode > 0)
			req_len += retcode;
		else if (retcode <= 0)
			break;
		if (strstr(in_buf, "\r\n\r\n"))
			break;
	}
	if (retcode <= 0)
	{   printf("recv error detected ...\n"); }

	else
	{
		printf("received data = %s\n", in_buf);
		strtok(in_buf, " ");
		temp = strtok(NULL, " ");

		snprintf(file_name, BUF_SIZE, "%s", temp + 1); 
		fh = open(file_name, O_RDONLY);

		if (fh == -1)
		{
			printf("File %s not found - sending an HTTP 404 \n", file_name);
			strcpy(out_buf, NOTOK_404);
			send(client_s, out_buf, strlen(out_buf), 0);
			strcpy(out_buf, MESS_404);
			send(client_s, out_buf, strlen(out_buf), 0);
		}
		else
		{
			printf("File %s is being sent \n", file_name);
			strcpy(out_buf, OK_TEXT);
			send(client_s, out_buf, strlen(out_buf), 0);

			buf_len = 1;  
			while (buf_len > 0)  
			{
				buf_len = read(fh, out_buf, BUF_SIZE);
				if (buf_len > 0)   
				{ 
					send(client_s, out_buf, buf_len, 0);      
				}
				sleep(1);
			}

			close(fh);
			close(client_s);
			printf("File %s is sent done\n", file_name);
		}
	}
}

int main(void)
{
	unsigned int          server_s;
	struct sockaddr_in    server_addr;
	struct sockaddr_in    client_addr;
	int                   addr_len;

	server_s = socket(AF_INET, SOCK_STREAM, 0);

	int x = 1;
	if (setsockopt(server_s, SOL_SOCKET, SO_REUSEADDR, &x, sizeof(x)) == -1) 
	{ 
		perror("Setsockopt");
		exit(FALSE);
	} 

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT_NUM);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(server_s, (struct sockaddr *)&server_addr, sizeof(server_addr));

	listen(server_s, PEND_CONNECTIONS);

	while(TRUE)
	{
		addr_len = sizeof(client_addr);
		if (0 > (client_s = accept(server_s, (struct sockaddr *)&client_addr, &addr_len)))
		{
			printf("ERROR - Unable to create socket \n");
			exit(FALSE);
		}
		else
		{
			printf("got the connection\n");
			if (fork() == 0) {
				close(server_s);
				my_thread(client_s);
				exit(0);
			}
			close(client_s);
		}
	}

	close (server_s);  // close the primary socket
	return (TRUE);
}
