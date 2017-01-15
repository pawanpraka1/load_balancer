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
	lb_server->cur_conn--;
}

void close_conn(int efd, server_info_t *server)
{
	if (server->server_flags & STATS_CONN) {
		mark_pending_event_invalid(server);
		close(server->fd);
		free(server->session);
		free(server);
		stats_server->cur_conn--;
	} else if (server->server_flags & BACKEND_SERVER) {
		close_server_conn(efd, server);
	}else {
		close_client_conn(server);
	} 
}

void close_server_conn(int efd, server_info_t *server)
{
	struct epoll_event event;
	if (0 == server->session->buf_size) {
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
}

void close_client_pconn(server_info_t *client)
{
	ASSERT(NULL == client->session->server);
	remove_server_info(client);
	mark_pending_event_invalid(client);
	close(client->fd);
	free(client->session);
	free(client);
}


