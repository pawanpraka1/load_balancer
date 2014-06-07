#include <server.h>
#include <stats.h>

int stats_read_req(server_info_t *server, int efd)
{
	struct epoll_event event;
	stats_server->read_events++;
	if (server->session->buf_len == sizeof(STATS) - 1) {
		printf("req = %s buf_len = %d\n", server->session->buf, server->session->buf_len);
		event.data.ptr = (void *)server;
		event.events = EPOLLOUT;
		if (0 > epoll_ctl (efd, EPOLL_CTL_MOD, server->fd, &event))
			return -1; 
	}
	return 0;
}
int ad_stats(server_info_t *server_head, char *buf, u32bits cur_len)
{
	int len;
	server_info_t *sinfo = server_head;
	while (sinfo) {
		len = snprintf(&buf[cur_len], BUF_LEN - cur_len,
				"read event = %u\nwrite events = %u\n",
				sinfo->read_events, sinfo->write_events);
		cur_len += len;
		sinfo = sinfo->next;
	}
	return cur_len;
}
int stats_write_res(server_info_t *server)
{
	int len;

	if (server->write_events == 1) {
		if (!strncmp(server->session->buf, STATS, server->session->buf_len)) {
			len = snprintf(server->session->buf, BUF_LEN,
					"\nLB STATS :-\nclient connection = %u\npending connection = %u\n"
					"read events = %u\nwrite events = %u\n\nBACKEND STATS :-\n",
					lb_server->cur_conn, lb_server->cur_pending_conn, 
					lb_server->read_events, lb_server->write_events);
			server->session->buf_len = len;
			server->session->buf_len += ad_stats(backend_server_head, server->session->buf, server->session->buf_len);
			server->session->buf_len += snprintf(server->session->buf + server->session->buf_len, 
								BUF_LEN - server->session->buf_len, "\nCLIENT STATS :-\n");
			server->session->buf_len += ad_stats(client_info_head, server->session->buf, server->session->buf_len);
		} else {
			server->session->buf_len = snprintf(server->session->buf, BUF_LEN, "command not implemented\n");
		}
	}

	if (0 > (len = write(server->fd, server->session->buf, server->session->buf_len)))
		ASSERT(0);
	server->session->buf_read += len;
	if (server->session->buf_read == server->session->buf_len) {
		close(server->fd);
		free(server->session);
		free(server);
	}
	return 0;
}
