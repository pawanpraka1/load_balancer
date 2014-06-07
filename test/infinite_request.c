#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#define BUF_LEN 2048

#define ERROR_EXIT(str) \
do { \
	perror(str); \
	exit(-1); \
}while(0);

int main()
{
	struct sockaddr_in s_addr;
	int sock_fd;

	char buf[BUF_LEN];

	if ((sock_fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0)) < 0)
		ERROR_EXIT("socket");

	bzero((char *) &s_addr, sizeof(s_addr));
	s_addr.sin_family = AF_INET;
	s_addr.sin_port = htons(5555);
	s_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (0 > connect(sock_fd, (struct sockaddr *) &s_addr, sizeof(s_addr)))
		ERROR_EXIT("connect");
	int len = snprintf(buf, BUF_LEN, "STATS");
	while (1) {
		if (0 > send(sock_fd, buf, len, 0))
			ERROR_EXIT("send");
		sleep(1);
	}
	close(sock_fd);
	return 0;
}
