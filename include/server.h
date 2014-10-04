#ifndef __SERVER
#define __SERVER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/epoll.h>  
#include <fcntl.h>
#include <malloc.h>
#include <assert.h>

#define MAX_CLIENT 100
#define BUF_LEN 65536
#define MAX_EVENTS 10

#define true 1
#define false 0

typedef unsigned short int u16bits;
typedef unsigned int u32bits;

#define ERROR_EXIT(str) \
do { \
	perror(str); \
	exit(-1); \
}while(0);

#define ASSERT(cond) assert((cond))

#define LB_SERVER		0x1
#define STATS_SERVER		0x2
#define STATS_CONN		0x4
#define BACKEND_SERVER		0x8
#define CONN_CLOSED		0x10

typedef struct session_info {
	int buf_len;
	int buf_read;
	char buf[BUF_LEN];
	struct server_info *server;
} session_info_t;

#define session_info_s sizeof(session_info_t)

typedef struct server_info {
	int fd;
	u32bits id;
	u32bits read_events;
	u32bits write_events;
	u32bits cur_conn;
	u32bits tot_unhandled_conn;
	u32bits server_flags;
	u32bits usage_flags;
	struct server_info *next;
	struct server_info **prev;
	session_info_t *session;
} server_info_t;

#define server_info_s sizeof(server_info_t)

typedef struct bserver_info {
	u16bits port;
	u16bits cur_conn;
	char *ip_str;
	struct bserver_info *bnext;
	server_info_t *bserver;
} bserver_info_t;

#define bserver_info_s sizeof(bserver_info_t)

extern int read_event_handler(server_info_t *server, int efd);
extern int write_event_handler(server_info_t *server);
extern int init_listening_socket(u16bits port);
extern server_info_t *create_server_info(int fd);
extern server_info_t *create_backend_server();
extern server_info_t *create_client_info(int efd, int fd);
extern bserver_info_t *create_bserver_info(char *ip, u16bits port);
extern void init_backend_server(int efd);
extern void init_epoll_events(server_info_t *server, int efd);
extern int attach_backend_lbserver(int efd, server_info_t *client_info);
extern bserver_info_t *get_next_lbserver();
extern void insert_client_info(server_info_t *client_info);
extern void remove_server_info(server_info_t *client_info);
extern void insert_into_cpool(server_info_t *client_info);
extern void remove_server_cpool(server_info_t *client_info);
extern void remove_form_cpool(server_info_t *client_info);
extern void close_server_conn(int efd, server_info_t *server);
extern void close_client_conn(server_info_t *client);
extern void close_client_pconn(server_info_t *client);
extern void close_conn(int efd, server_info_t *server);
extern void insert_backend_server(server_info_t *server);
extern void mark_pending_event_invalid(void *sptr);


extern server_info_t *lb_server;
extern server_info_t *stats_server;
extern server_info_t *client_info_head;
extern bserver_info_t *bserver_head;

extern struct epoll_event cur_events[MAX_EVENTS];
extern int event_count;
extern u32bits server_id;

#endif
