#include <server.h>

void close_session(server_info_t *server)
{
	server_info_t *lserver = server->session->server;
	remove_server_info(server);
	remove_server_info(lserver);
	mark_pending_event_invalid(server);
	if (0 > close(server->fd))
		ASSERT(0);
	mark_pending_event_invalid(lserver);
	if (0 > close(lserver->fd))
		ASSERT(0);
	free(server->session);
	free(lserver->session);
	free(server);
	free(lserver);
}

void close_server_conn(int efd, server_info_t *server)
{
	struct epoll_event event;
	if (server->session->buf_len == server->session->buf_read) {
		close_session(server);
	} else {
		server->server_flags |= CONN_CLOSED;
		if (0 > epoll_ctl (efd, EPOLL_CTL_DEL, server->fd, &event)) 
			ASSERT(0);
	}
}

void close_client_conn(server_info_t *client)
{
	close_session(client);
	lb_server->cur_conn--;
}

void close_client_pconn(server_info_t *client)
{
	ASSERT(NULL == client->session->server);
	remove_server_cpool(client);
	remove_server_info(client);
	lb_server->cur_pending_conn--;
	mark_pending_event_invalid(client);
	close(client->fd);
	free(client->session);
	free(client);
}


