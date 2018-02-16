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
#include <signal.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <netinet/tcp.h>

#define BUF_SIZE		1024
#define PORT_NUM		8888
#define PEND_CONNECTIONS	100
#define NUM_EV			1024

char *send_str = "How are you?";
int lport = PORT_NUM;

void server_get_args(int argc, char * argv[])
{
	int c;
	while ((c = getopt(argc, argv, "s:p:")) != EOF)
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

int read_data(int fd)
{
	char buf[BUF_SIZE];
	int len;

	do {
		len = read(fd, buf, BUF_SIZE);
		if (0 >= len) {
			if (0 == len) {
				printf("connection closed by peer\n");
			} else {
				if (EAGAIN == errno)
					return 0;
				perror("read");
			}
			return -1;
		}

		if (0 > write(1, buf, len)) {
			perror("write");
		}
	} while (1);

	return 0;
}

int write_data(int fd)
{
	int len, buf_len;

	buf_len = strlen(send_str);

	do {
		len = write(fd, send_str, buf_len);

		if (0 > len) {
			if (EAGAIN == errno)
				return 0;
			perror("write");
			return -1;
		}
	} while (1);

	return 0;
}

int main(int argc, char *argv[])
{
	unsigned int          server_s;
	struct sockaddr_in    server_addr;
	struct sockaddr_in    client_addr;
	int                   addr_len;
	unsigned int	      client_s;

	server_s = socket(AF_INET, SOCK_STREAM, 0);
	if (0 > server_s) {
		perror("Setsockopt");
		exit(-1);
	}

	int epollfd = epoll_create1(EPOLL_CLOEXEC);
	if (0 > epollfd) {
		perror("epoll_create1");
		exit(-1);
	}
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = server_s;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, server_s, &ev) == -1) {
		perror("epoll_ctl: listen_sock");
		exit(-1);
	}
	int x = 1;
	if (setsockopt(server_s, SOL_SOCKET, SO_REUSEADDR, &x, sizeof(x)) == -1) {
		perror("Setsockopt");
		exit(-1);
	}

	signal(SIGPIPE, SIG_IGN);
	
	server_get_args(argc, argv);

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(lport);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (0 > bind(server_s, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
		perror("bind");
		exit(-1);
	}

	if (0 > listen(server_s, PEND_CONNECTIONS)) {
		perror("listen");
		exit(-1);
	}

	struct epoll_event nev[NUM_EV];
	while(1) {
		int nfds = epoll_wait(epollfd, nev, NUM_EV, -1);
		if (nfds == -1) {
			if (EINTR == errno)
				continue;
			perror("epoll_wait");
			exit(-1);
		}
		for (int i = 0; i < nfds; i++) {
			struct epoll_event *rev = &nev[i];
			if (rev->data.fd == server_s) {
				addr_len = sizeof(client_addr);
				if (0 > (client_s = accept(server_s, (struct sockaddr *)&client_addr, &addr_len))) {
					perror("accept");
					continue;
				}
				int flags = fcntl(client_s, F_GETFL, 0);
				if (-1 == flags) {
					perror("fcntl get");
					close(client_s);
					continue;
				}
				flags |= O_NONBLOCK;
				if (0 > fcntl(client_s, F_SETFL, flags)) {
					perror("fcntl");
					exit(-1);
				}

				ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
				ev.data.fd = client_s;
				if (epoll_ctl(epollfd, EPOLL_CTL_ADD, client_s,
				    &ev) == -1) {
					perror("epoll_ctl: conn_sock");
					exit(-1);
				}
				printf("a new client arrives ...\n");
			} else {
				if ((rev->events & EPOLLERR) ||
				    (rev->events & EPOLLHUP)) {
					printf("got the epoll error 0x%x (%x, %x, %x, %x, %x)\n", rev->events, EPOLLERR, EPOLLHUP, EPOLLIN, EPOLLOUT, EPOLLRDHUP);
					if (0 > close(rev->data.fd))
						perror("close err");
					continue;
				}
				if (rev->events & EPOLLIN) {
					if (0 > read_data(rev->data.fd)) {
						if (0 > close(rev->data.fd))
							perror("close");
					}
				}
				if (rev->events & EPOLLOUT) {
					if (0 > write_data(rev->data.fd)) {
						if (0 > close(rev->data.fd))
							perror("close");
					}
				}
			}
		}
	}
	close (server_s);
	return (0);
}
