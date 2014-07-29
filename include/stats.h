#ifndef __STATS
#define __STATS

#include <server.h>

#define STATS "STATS"
#define MAX_EVENTS 10

#define ERROR_EXIT(str) \
do { \
	perror(str); \
	exit(-1); \
}while(0);

extern int stats_read_req(server_info_t *server, int efd);
extern int stats_write_res(server_info_t *server);


#endif
