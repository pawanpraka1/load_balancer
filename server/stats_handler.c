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
	u32bits count = 1;
	server_info_t *sinfo = server_head;
	while (sinfo) {
		len = snprintf(&buf[cur_len], BUF_LEN - cur_len,
				"%u. (%u)->(%u)\nread event = %u\nwrite events = %u\nbuf_len = %u buf_read = %u\n\n",
				count, sinfo->id, sinfo->session->server ? sinfo->session->server->id : -1, 
				sinfo->read_events, sinfo->write_events, sinfo->session->buf_len, sinfo->session->buf_read);
		cur_len += len;
		sinfo = sinfo->next;
		count++;
	}
	len = snprintf(&buf[cur_len], BUF_LEN - cur_len, "TOTAL COUNT = %u\n\n", count - 1);
	cur_len += len;
	return cur_len;
}

int stats_write_res(server_info_t *server)
{
	int len;

	if (server->write_events == 1) {
		if (!strncmp(server->session->buf, STATS, server->session->buf_len)) {
			len = snprintf(server->session->buf, BUF_LEN,
					"\nLB STATS :-\ncur client connection = %u\nunhandled connection = %u\n"
					"tot_conn = %u\n\nBACKEND STATS :-\n",
					lb_server->cur_conn, lb_server->tot_unhandled_conn, lb_server->read_events);
			server->session->buf_len = len;
			bserver_info_t *bserv = bserver_head;
			while (bserv) {
				server->session->buf_len += snprintf(&server->session->buf[server->session->buf_len], 
				BUF_LEN - server->session->buf_len, "\nBACKEND SERVER (%s:%hu) tot_conn %hd\n\n", 
				bserv->ip_str, bserv->port, bserv->cur_conn);
				server->session->buf_len += ad_stats(bserv->bserver, server->session->buf, server->session->buf_len);
				bserv= bserv->bnext;
			}
			server->session->buf_len += snprintf(server->session->buf + server->session->buf_len, 
								BUF_LEN - server->session->buf_len, "\nCLIENT STATS :-\n");
			server->session->buf_len += ad_stats(client_info_head, server->session->buf, server->session->buf_len);
		} else {
			server->session->buf_len = snprintf(server->session->buf, BUF_LEN, "command not implemented\n");
		}
	}

	if (0 > (len = write(server->fd, &server->session->buf[server->session->buf_read], 
				server->session->buf_len - server->session->buf_read)))
		ASSERT(0);
	server->session->buf_read += len;
	if (server->session->buf_read == server->session->buf_len) {
		close(server->fd);
		free(server->session);
		free(server);
	}
	return 0;
}
