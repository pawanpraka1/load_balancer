#include <server.h>

bserver_info_t *bserver_head;
bserver_info_t *cur_lbserver;

server_info_t *get_next_lbserver()
{
	cur_lbserver = cur_lbserver->bnext;
	if (NULL == cur_lbserver)
		cur_lbserver = bserver_head;
	return cur_lbserver;
}

int attach_backend_lbserver(int efd, server_info_t *client_info)
{
	struct epoll_event event;
	server_info_t *tserver;
	if (!(tserver = create_backend_server()))
		return -1;
	do {
		event.data.ptr = (void *)client_info;
		event.events = EPOLLIN | EPOLLOUT;
		if (0 > epoll_ctl (efd, EPOLL_CTL_MOD, client_info->fd, &event)) {
			ASSERT(0);
			break;
		}
		event.data.ptr = (void *)tserver;
		event.events = EPOLLIN | EPOLLOUT;
		if (0 > epoll_ctl (efd, EPOLL_CTL_ADD, tserver->fd, &event)) {
			ASSERT(0);
			break;
		}
		client_info->session->server = tserver;
		tserver->session->server = client_info;
		cur_lbserver->cur_conn++;
		insert_backend_server(tserver);
		get_next_lbserver();
		return 0;
	} while (0);

	free(tserver);
	return -1;
}

void attach_pending_connection(int efd)
{
	struct epoll_event event;
	server_info_t *cinfo, *sinfo;
	if (lb_server->cur_pending_conn) {
		if (!(sinfo = create_backend_server()))
			return;
		cinfo = lb_server->cpool;
		ASSERT(cinfo->usage_flags & CLIENT_CONN_PENDING);

		event.data.ptr = (void *)cinfo;
		event.events = EPOLLIN | EPOLLOUT;
		if (0 > epoll_ctl (efd, EPOLL_CTL_MOD, cinfo->fd, &event)) {
			ASSERT(0);
			free(sinfo);
			return;
		}
		event.data.ptr = (void *)sinfo;
		event.events = EPOLLIN | EPOLLOUT;
		if (0 > epoll_ctl (efd, EPOLL_CTL_ADD, sinfo->fd, &event)) {
			ASSERT(0);
			free(sinfo);
			return;
		}
		sinfo->session->server = cinfo;
		cinfo->session->server = sinfo;
		lb_server->cpool = cinfo->cpool;
		cinfo->usage_flags &= ~CLIENT_CONN_PENDING;
		lb_server->cur_pending_conn--;
		lb_server->cur_conn++;
	}
}

void insert_backend_server(server_info_t *server)
{
	ASSERT(server->server_flags & BACKEND_SERVER);
	if (cur_lbserver->bserver)
		cur_lbserver->bserver->prev = &(server->next);
	server->next = cur_lbserver->bserver;
	cur_lbserver->bserver = server;
	server->prev = &(cur_lbserver->bserver);
}

server_info_t *create_backend_server()
{
	struct sockaddr_in s_addr;
	int sock_fd;
	server_info_t *server;

	if ((sock_fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0)) < 0)
		return NULL;

	bzero((char *) &s_addr, sizeof(s_addr));
	s_addr.sin_family = AF_INET;
	s_addr.sin_port = htons(cur_lbserver->port);
	s_addr.sin_addr.s_addr = inet_addr(cur_lbserver->ip_str);

	if (0 > connect(sock_fd, (struct sockaddr *) &s_addr, sizeof(s_addr))) {
		close(sock_fd);
		return NULL;
	}
	if (!(server = (server_info_t *)malloc(server_info_s))) {
		close(sock_fd);
		return NULL;
	}
	bzero((char *)server, server_info_s);
	if (!(server->session = (session_info_t *)malloc(session_info_s))) {
		close(sock_fd);
		free(server);
		return NULL;
	}
	bzero((char *)server->session, session_info_s);
	server->fd = sock_fd;
	server->id = server_id++;
	server->server_flags |= BACKEND_SERVER;
	return server;
}

bserver_info_t *create_bserver_info(char *ip, u16bits port)
{
	bserver_info_t *bserver;
	if (!(bserver = (bserver_info_t *)malloc(bserver_info_s)))
		return NULL;
	bzero((char *)bserver, bserver_info_s);
	int len = strlen(ip) + 1;
	if (!(bserver->ip_str = (char *)malloc(len))) {
		free(bserver);
		return NULL;
	}
	strncpy(bserver->ip_str, ip, len);
	bserver->port = port;
	bserver->bnext = bserver_head;
	bserver_head = bserver;
	return bserver;
}

void init_backend_server(int efd)
{
	int port;
	char ip_str[13];
	FILE *fp;
	if (!(fp = fopen("server.conf", "r")))
		ERROR_EXIT("server.conf does not exist");

	while (true) {
		fscanf(fp, "%s %d", ip_str, &port);
		if (feof(fp))
			break;
		printf("ip = %s port = %d\n", ip_str, port);
		bserver_info_t *server = create_bserver_info(ip_str, port);
		if (NULL == cur_lbserver) {
			cur_lbserver = server;
		}
	}
	fclose(fp);
	return ;
}
