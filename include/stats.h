#ifndef __STATS
#define __STATS

#include <server.h>

#define STATS "STATS"

extern int stats_read_req(server_info_t *server, int efd);
extern int stats_write_res(server_info_t *server);
#endif
