#include <server.h>

server_info_t *backend_server_head;

int attach_backend_lbserver(int efd, server_info_t *client_info)
{
	struct epoll_event event;
	server_info_t *tserver = backend_server_head;
	while (tserver) {
		if (!(tserver->usage_flags & BACKEND_SERVER_INUSE))
			break;
		tserver = tserver->next;
	}
	if (tserver) {
		event.data.ptr = (void *)client_info;
		event.events = EPOLLIN | EPOLLOUT;
		if (0 > epoll_ctl (efd, EPOLL_CTL_MOD, client_info->fd, &event))
			return -1;
		event.data.ptr = (void *)tserver;
		event.events = EPOLLIN | EPOLLOUT;
		if (0 > epoll_ctl (efd, EPOLL_CTL_ADD, tserver->fd, &event))
			return -1;
		tserver->usage_flags |= BACKEND_SERVER_INUSE;
		client_info->session->server = tserver;
		tserver->session->server = client_info;
		return 0;
	}
	return -1; 
}

void attach_pending_connection(int efd, server_info_t *sinfo)
{
	struct epoll_event event;
	server_info_t *cinfo;
	ASSERT(sinfo->server_flags & BACKEND_SERVER);
	ASSERT(sinfo->usage_flags & BACKEND_SERVER_INUSE);

	sinfo->session->buf_len = 0;
	sinfo->session->buf_read = 0;
	if (lb_server->cur_pending_conn) {
		cinfo = lb_server->cpool;
		sinfo->session->server = cinfo;
		cinfo->session->server = sinfo;
		lb_server->cpool = cinfo->cpool;
		ASSERT(cinfo->usage_flags & CLIENT_CONN_PENDING);
		cinfo->usage_flags &= ~CLIENT_CONN_PENDING;
		lb_server->cur_pending_conn--;
		event.data.ptr = (void *)(sinfo->session->server);
		event.events = EPOLLIN | EPOLLOUT;
		if (0 > epoll_ctl (efd, EPOLL_CTL_MOD, cinfo->fd, &event)) {
			close(cinfo->fd);
			free(cinfo->session);
			free(cinfo);
			ASSERT(0);
		}
	} else {
		if (0 > epoll_ctl (efd, EPOLL_CTL_DEL, sinfo->fd, &event)) {
			close(cinfo->fd);
			free(cinfo->session);
			free(cinfo);
			ASSERT(0);
		} 
		lb_server->cur_conn--;
		sinfo->session->server = NULL;
		sinfo->usage_flags &= ~BACKEND_SERVER_INUSE;
	}
}

void insert_backend_server(server_info_t *server)
{
	server->next = backend_server_head;
	backend_server_head = server;
}

server_info_t *create_backend_server(int fd)     
{
	server_info_t *server;
	if (!(server = (server_info_t *)malloc(server_info_s)))
		return NULL;
	bzero((char *)server, server_info_s);
	if (!(server->session = (session_info_t *)malloc(session_info_s))) {
		free(server);
		return NULL;
	}
	bzero((char *)server->session, session_info_s);
	server->fd = fd;
	insert_backend_server(server);
	server->server_flags |= BACKEND_SERVER;
	return server;
}

void init_backend_server(int efd)
{
	struct sockaddr_in s_addr;
	int sock_fd, port;
	char ip_str[13];
	FILE *fp;
	if (!(fp = fopen("server.conf", "r")))
		ERROR_EXIT("server.conf does not exist");

	while (true) {
		fscanf(fp, "%s %d", ip_str, &port);
		if (feof(fp))
			break;
		printf("ip = %s port = %d\n", ip_str, port);
		if ((sock_fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0)) < 0)
			ERROR_EXIT("socket");

		bzero((char *) &s_addr, sizeof(s_addr));
		s_addr.sin_family = AF_INET;
		s_addr.sin_port = htons(port);
		s_addr.sin_addr.s_addr = inet_addr(ip_str);

		if (0 > connect(sock_fd, (struct sockaddr *) &s_addr, sizeof(s_addr)))
			ERROR_EXIT("connect");
		server_info_t *server = create_backend_server(sock_fd);
		if (NULL == server) {
			close(sock_fd);
			continue;
		}
	}
	fclose(fp);
	return ;
}
