#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stats.h>

int main()
{
	struct sockaddr_in s_addr;
	int sock_fd;

	char buf[BUF_LEN];

	if ((sock_fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0)) < 0)
		ERROR_EXIT("socket");

	bzero((char *) &s_addr, sizeof(s_addr));
	s_addr.sin_family = AF_INET;
	s_addr.sin_port = htons(3333);
	s_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (0 > connect(sock_fd, (struct sockaddr *) &s_addr, sizeof(s_addr)))
		ERROR_EXIT("connect");
	int wlen = 0, len = snprintf(buf, BUF_LEN, STATS);
	int efd, event_count;

	if (0 > (efd = epoll_create1 (EPOLL_CLOEXEC)))
		ERROR_EXIT("epoll_create1");
	struct epoll_event event;
	event.data.u32 = sock_fd;
	event.events = EPOLLOUT;
	struct epoll_event cur_events[MAX_EVENTS];

	if (0 > epoll_ctl (efd, EPOLL_CTL_ADD, sock_fd, &event))
		ERROR_EXIT("epoll_ctl");
	while (1) {
		event_count = epoll_wait (efd, cur_events, MAX_EVENTS, -1);
		while (event_count--) {
			if (cur_events[event_count].events & EPOLLOUT) {
				int wbyte;
				if (0 > (wbyte = write(sock_fd, buf + wlen, len - wlen)))
					ERROR_EXIT("write");
				wlen += wbyte;
				if (len == wlen) {
					event.data.u32 = sock_fd;
					event.events = EPOLLIN;
					if (0 > epoll_ctl (efd, EPOLL_CTL_MOD, sock_fd, &event))
						ERROR_EXIT("epoll_ctl MOD");
				}
			} else if (cur_events[event_count].events & EPOLLIN) {
				if (0 > (len = read(sock_fd, buf, BUF_LEN))) {
					ERROR_EXIT("read");
				}
				if (len) {
					if (0 > write(1, buf, len))
						ERROR_EXIT("write");
				} else {
					if (0 > close(sock_fd))
						ERROR_EXIT("close");
					exit(0);
				}
			}
		}
	}
	return 0;
}
