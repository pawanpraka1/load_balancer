#include <server.h>
#include <stats.h>

void mark_pending_event_invalid(void *sptr)
{
	int i;
	for (i = 0; i < event_count; i++) {
		if (cur_events[i].data.ptr == sptr)
			cur_events[i].events |= EPOLLERR;
	}
}

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
			ASSERT(server->session->buf_len <= BUF_LEN);
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
				if (server->server_flags & STATS_CONN) {
					mark_pending_event_invalid(server);
					close(server->fd);
					free(server->session);
					free(server);
					stats_server->cur_conn--;
				} else if (server->usage_flags & CLIENT_CONN_PENDING) {
					close_client_pconn(server);
				} else if (server->server_flags & BACKEND_SERVER) {
					close_server_conn(efd, server);
				}else {
					close_client_conn(server);
				}
				return 0;
			}
			if (server->server_flags & STATS_CONN)
				stats_read_req(server, efd);
		}
	}
	return 0;
}

int write_event_handler(server_info_t *server)
{
	server->write_events++;
	if (server->server_flags & STATS_CONN) {
		stats_write_res(server);
		stats_server->write_events++;
	} else {
		if (server->session->server->session->buf_len > server->session->server->session->buf_read)
			server->session->server->session->buf_read += write(server->fd, 
						server->session->server->session->buf + server->session->server->session->buf_read, 
						server->session->server->session->buf_len - server->session->server->session->buf_read);
		if (!(server->server_flags & BACKEND_SERVER) &&
			(server->session->server->session->buf_len == server->session->server->session->buf_read) &&
			(server->session->server->server_flags & CONN_CLOSED)) {
			close_client_conn(server);
		}
	}
}
