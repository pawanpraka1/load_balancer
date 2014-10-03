#include <server.h>

server_info_t *lb_server;
server_info_t *stats_server;

server_info_t *client_info_head;
server_info_t *server_info_head;

struct epoll_event cur_events[MAX_EVENTS];
int event_count;
u32bits server_id;

void insert_into_cpool(server_info_t *client_info)
{
	if (lb_server->cpool)
		lb_server->cpool->prev = &(client_info->cpool);
	client_info->cpool = lb_server->cpool;
	lb_server->cpool = client_info;
	client_info->cpool_prev = &lb_server->cpool;
	client_info->usage_flags |= CLIENT_CONN_PENDING;
}

void remove_server_cpool(server_info_t *client_info)
{
	*(client_info->cpool_prev) = client_info->cpool;
	if (client_info->cpool)
		client_info->cpool->cpool_prev = client_info->prev;
}

void insert_client_info(server_info_t *client_info)
{
	if (client_info_head)
		client_info_head->prev = &(client_info->next);
	client_info->next = client_info_head;
	client_info->prev = &client_info_head; 
	client_info_head = client_info;
	client_info->id = server_id++;
}

void remove_server_info(server_info_t *server)
{
	*(server->prev) = server->next;
	if (server->next)
		server->next->prev = server->prev;
}

server_info_t *create_server_info(int fd)
{
	server_info_t *server;
	if (!(server = (server_info_t *)malloc(server_info_s)))
		ERROR_EXIT("malloc");;
	bzero((char *)server, server_info_s);
	server->fd = fd;
	if (server_info_head)
		server_info_head->prev = &(server->next);
	server->next = server_info_head;
	server_info_head = server;
	server->prev = &server_info_head;
	return server;
}

server_info_t *create_client_info(int efd, int fd)
{
	server_info_t *client_info;
	struct epoll_event event;
	if (!(client_info = (server_info_t *)malloc(server_info_s))) {
		close(fd);
		return NULL;
	}
	bzero((char *)client_info, server_info_s);

	if (!(client_info->session = (session_info_t *)malloc(session_info_s))) {
		close(fd);
		free(client_info);
		return NULL;
	}
	bzero((char *)client_info->session, session_info_s);

	event.data.ptr = (void *)client_info;
	event.events = EPOLLIN;
	if (0 > epoll_ctl (efd, EPOLL_CTL_ADD, fd, &event)) {
		close(fd);
		free(client_info->session);
		free(client_info);
		return NULL; 
	}
	client_info->fd = fd;
	return client_info;
}

int init_listening_socket(u16bits port)
{
	struct sockaddr_in s_addr;
	int sock_fd;
	if ((sock_fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0)) < 0)
		ERROR_EXIT("socket");

	int optval = 1;
	if (0 > setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)))
		ERROR_EXIT("setsockopt SO_REUSEADDR");
	bzero((char *) &s_addr, sizeof(s_addr));
	s_addr.sin_family = AF_INET;
	s_addr.sin_port = htons(port);
	s_addr.sin_addr.s_addr = htons(INADDR_ANY);

	if (0 > bind(sock_fd, (struct sockaddr *) &s_addr, sizeof(s_addr)))
		ERROR_EXIT("bind");

	if (0 > listen(sock_fd, MAX_CLIENT))
		ERROR_EXIT("listen");

	return sock_fd;
}

void init_epoll_events(server_info_t *server, int efd)
{
	struct epoll_event event;
	event.data.ptr = (void *)server;
	event.events = EPOLLIN;

	if (0 > epoll_ctl (efd, EPOLL_CTL_ADD, server->fd, &event))
		ERROR_EXIT("epoll_ctl");
}

int main()
{
	struct sockaddr_in c_addr, s_addr;
	int sock_fd, stats_fd, efd;
	struct epoll_event event;

	sock_fd = init_listening_socket(5555);
	stats_fd = init_listening_socket(3333);
	init_backend_server(efd);

	if (0 > (efd = epoll_create1 (EPOLL_CLOEXEC)))
		ERROR_EXIT("epoll_create1");

	server_info_t *server = create_server_info(stats_fd);
	server->server_flags |= STATS_SERVER;
	init_epoll_events(server, efd);
	stats_server = server;

	server = create_server_info(sock_fd);
	server->server_flags |= LB_SERVER;
	init_epoll_events(server, efd);
	lb_server = server;

	while (1) {
		event_count = epoll_wait (efd, cur_events, MAX_EVENTS, -1);
		while (event_count--) {
			if (0 == cur_events[event_count].events)
				continue;
			server = (server_info_t *)cur_events[event_count].data.ptr;

			if ((cur_events[event_count].events & EPOLLERR) ||
					(cur_events[event_count].events & EPOLLHUP) ||
					(!(cur_events[event_count].events & (EPOLLIN | EPOLLOUT)))) {
				close_conn(efd, server);
				continue;
			}


			ASSERT(!(server->server_flags & (server->server_flags - 1)));

			if (cur_events[event_count].events & EPOLLIN)
				read_event_handler(server, efd);
			else if(cur_events[event_count].events & EPOLLOUT)
				write_event_handler(server);
		}
	}
	return 0;
}
