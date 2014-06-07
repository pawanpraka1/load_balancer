#include <server.h>
#include <stats.h>

int read_event_handler(server_info_t *server, int efd)
{
	struct sockaddr_in c_addr;
	int c_len, read_len;
	int accepted_fd;
	server_info_t *client_info;
	server->read_events++;
	switch (server->server_flags) {
		case LB_SERVER: 
		{
			c_len = sizeof(c_addr);
			if (0 > (accepted_fd = accept(server->fd, (struct sockaddr *)&c_addr, &c_len)))
				ASSERT(0);
			if (client_info = create_client_info(efd, accepted_fd)) {
				insert_client_info(client_info);
				if (0 > attach_backend_lbserver(efd, client_info)) {
					insert_into_cpool(client_info);
					lb_server->cur_pending_conn++;
				} else {
					lb_server->cur_conn++;
				}
			}
			break;
		}
		case STATS_SERVER:
		{
			c_len = sizeof(c_addr);
			if (0 > (accepted_fd = accept(server->fd, (struct sockaddr *)&c_addr, &c_len)))
				ASSERT(0);
			if (client_info = create_client_info(efd, accepted_fd))
				client_info->server_flags |= STATS_CONN;
			break;
		}
		default:
		{
			if (BUF_LEN == server->session->buf_len)
				server->session->buf_len = 0;
			if (0 > (read_len = read(server->fd, 
				server->session->buf + server->session->buf_len, BUF_LEN - server->session->buf_len))) 
				ASSERT(0);
			if (read_len > 0) {
				server->session->buf_len += read_len;
			} else {
				ASSERT(!(server->server_flags & LB_SERVER));
				printf("closing connection data read = %d\n", server->session->buf_len);
				if (server->server_flags & STATS_CONN)
					stats_server->cur_conn--;
				else if (server->usage_flags & CLIENT_CONN_PENDING) {
					remove_server_cpool(server, &(lb_server->cpool));
					remove_server_info(server, &client_info_head);
					lb_server->cur_pending_conn--;
				} else {
					remove_server_info(server, &client_info_head);
					attach_pending_connection(efd, server->session->server);
				}
				close(server->fd);
				free(server->session);
				free(server);
				return 0;
			}
			if (server->server_flags & STATS_CONN)
				stats_read_req(server, efd);
			else if (server->server_flags & BACKEND_SERVER)
				bserver_session_rhandler(server);
			else
				client_session_rhandler(server);
		}
	}
	return 0;
}

int write_event_handler(server_info_t *server)
{
	server->write_events++;
	if (server->server_flags & STATS_CONN)
		stats_write_res(server);
	else if (server->server_flags & BACKEND_SERVER)
		bserver_session_whandler(server);
	else
		client_session_whandler(server);
}
