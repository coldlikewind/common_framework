#ifndef __AGC_GTP_PATH_H__
#define __AGC_GTP_PATH_H__

#include <agc.h>

AGC_BEGIN_EXTERN_C

#define MAX_GTP_BUFFER 1500

typedef struct agc_gtp_sock agc_gtp_sock_t;

/*
 * if the len is -1 or buf is NULL, then the sock is close.
 */
typedef void (*gtp_msg_handler_t)(
		agc_gtp_sock_t *sock, 
    	const char* buf, 
    	int32_t *len,
    	agc_std_sockaddr_t *remote_addr, 
    	socklen_t remote_addrlen);

typedef struct agc_gtp_buf
{
	char buf[MAX_GTP_BUFFER];
	int32_t len;
	struct agc_gtp_buf *next;
	agc_std_sockaddr_t 	remote_addr;
	socklen_t 			remote_addrlen;
}agc_gtp_buf_t;

struct agc_gtp_sock
{
	agc_std_socket_t 	sock_fd;
	agc_std_sockaddr_t 	local_addr;
	socklen_t 			local_addrlen;
	agc_std_sockaddr_t 	remote_addr;
	socklen_t 			remote_addrlen;
	gtp_msg_handler_t 	handler;

	agc_gtp_buf_t		*buf_header;
	agc_gtp_buf_t		*buf_tail;

	agc_connection_t 	*connection;
	agc_mutex_t			*mutex;
	agc_memory_pool_t 	*pool;
};

AGC_DECLARE(agc_status_t) agc_gtp_open(agc_gtp_sock_t **sock,
	agc_std_sockaddr_t *local_addr, socklen_t addrlen, 
	gtp_msg_handler_t handler);

AGC_DECLARE(agc_status_t) agc_gtp_open_v2(agc_gtp_sock_t **sock,
    agc_std_sockaddr_t *local_addr, socklen_t local_addrlen,
    agc_std_sockaddr_t *remote_addr, socklen_t remote_addrlen,
     gtp_msg_handler_t handler);

AGC_DECLARE(agc_status_t) agc_gtp_server(agc_gtp_sock_t **sock,
	agc_std_sockaddr_t *local_addr, socklen_t addrlen, 
	gtp_msg_handler_t handler);

AGC_DECLARE(agc_status_t) agc_gtp_client(agc_gtp_sock_t **sock, 
	agc_std_sockaddr_t *local_addr, socklen_t addrlen, 
	agc_std_sockaddr_t *remote_addr, socklen_t remote_addrlen,
	gtp_msg_handler_t handler);

AGC_DECLARE(agc_status_t) agc_gtp_close(agc_gtp_sock_t *sock);
AGC_DECLARE(agc_status_t) agc_gtp_add_message(agc_gtp_sock_t *sock, const char *buf, int32_t len,
	agc_std_sockaddr_t *remote_addr, socklen_t remote_addrlen);

AGC_END_EXTERN_C

#endif /* __GTP_PATH_H__ */
