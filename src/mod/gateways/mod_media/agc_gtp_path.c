
#include "agc_gtp_path.h"

agc_status_t agc_gtp_add_message(agc_gtp_sock_t *sock, const char *buf, int32_t len,
        agc_std_sockaddr_t *remote_addr, socklen_t remote_addrlen)
{
	if (sock == NULL) {
		char remote_str_addr[64];
		int remote_port;
		if (remote_addr->ss_family == AF_INET) {
			struct sockaddr_in *listen_addr4 = (struct sockaddr_in *)remote_addr;
			remote_port = listen_addr4->sin_port;
			inet_ntop(AF_INET, &(listen_addr4->sin_addr), remote_str_addr, 64);
		} else {
			struct sockaddr_in6 *listen_addr6 = (struct sockaddr_in6 *)remote_addr;
			remote_port = listen_addr6->sin6_port;
			inet_ntop(AF_INET6, &(listen_addr6->sin6_addr), remote_str_addr, 64);
		}
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_gtp_add_message sock is invalid, remote_str_addr=%s, port=%d.\n", remote_str_addr, remote_port);
		return AGC_STATUS_FALSE;
	}
	
	if (write(sock->sock_fd, buf, len) < 0) {
        int err = errno;
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_gtp_add_message write failed sock_fd %d:%d(%s).\n",
            sock->sock_fd, err, strerror(err));
        return AGC_STATUS_FALSE;
    } 
    
    return AGC_STATUS_SUCCESS;
}

static agc_status_t agc_gtp_get_message(agc_gtp_sock_t *sock, agc_gtp_buf_t **msg)
{
    agc_status_t ret = AGC_STATUS_FALSE;
    if (sock == NULL) {
        return AGC_STATUS_FALSE;
    }

    agc_mutex_lock(sock->mutex);
    if (sock->buf_header != NULL) {
        *msg = sock->buf_header; 
        sock->buf_header = sock->buf_header->next;

        if (sock->buf_header == NULL) {
            sock->buf_header = sock->buf_tail = NULL;
        }

        ret = AGC_STATUS_SUCCESS;
    }
    agc_mutex_unlock(sock->mutex);
    return ret;
}

static void agc_gtp_handle_read(void *data)
{
    agc_connection_t *connection = (agc_connection_t *)data;
    agc_gtp_sock_t *sock = (agc_gtp_sock_t *)connection->context;
    int32_t reads = 0;
    char buf[MAX_GTP_BUFFER];    
    agc_std_sockaddr_t  remote_addr;
    socklen_t           remote_addrlen;

    if (!connection) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_gtp_handle_read invalid connection read failed .\n");
        return;
    }

    // read all data in sock until it is empty.
    while (1)
    {
        remote_addrlen = sizeof(remote_addr);
        reads = recvfrom(connection->fd, (char *)buf, MAX_GTP_BUFFER,  
                    MSG_WAITALL, (struct sockaddr *) &remote_addr, &remote_addrlen); 
        if (reads <= 0 )
        {
            break;
        }

       /* if (reads > 0) {
            if (remote_addr.ss_family == AF_INET) {
                struct sockaddr_in *clientaddr4 = (struct sockaddr_in *)&remote_addr;
                char str_addr[64];
                inet_ntop(AF_INET, &(clientaddr4->sin_addr), str_addr, 64);
                
                agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "agc_gtp_handle_read connection addr4 %s .\n", str_addr);
            } else {
                struct sockaddr_in6 *clientaddr6 = (struct sockaddr_in6 *)&remote_addr;
                char str_addr[64];
                inet_ntop(AF_INET6, &(clientaddr6 ->sin6_addr), str_addr, 64);
                
                agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "agc_gtp_handle_read connection addr6 %s .\n", str_addr);
            }       
        }
    */
        if (sock->handler)
        {
            sock->handler(sock, buf, &reads, &remote_addr, remote_addrlen);
        }
    }
}

static void agc_gtp_handle_read_v2(void *data)
{
    agc_connection_t *connection = (agc_connection_t *)data;
    agc_gtp_sock_t *sock = (agc_gtp_sock_t *)connection->context;
    int32_t reads = 0;
    char buf[MAX_GTP_BUFFER];    

    if (!connection) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_gtp_handle_read invalid connection read failed .\n");
        return;
    }

    // read all data in sock until it is empty.
    while (1)
    {
		reads = read(connection->fd, (char *)buf, MAX_GTP_BUFFER);
        if (reads <= 0 )
        {
            break;
        }
    
        if (sock->handler)
        {
            sock->handler(sock, buf, &reads, &sock->remote_addr, sock->remote_addrlen);
        }
    }
}

static void agc_gtp_handle_write(void *data)
{
    /*
    agc_connection_t *connection = (agc_connection_t *)data;
    agc_gtp_sock_t *sock = (agc_gtp_sock_t *)connection->context;
    agc_status_t ret = AGC_STATUS_FALSE;
    agc_gtp_buf_t *msg = NULL;

    if (!connection) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_gtp_handle_write invalid connection write failed .\n");
        return;
    }
    agc_driver_del_event(connection, AGC_WRITE_EVENT);

    ret = agc_gtp_get_message(sock, &msg);
    if (ret == AGC_STATUS_FALSE) {
        return;
    }

    if (msg->remote_addr.ss_family == AF_INET) {
        struct sockaddr_in *clientaddr4 = (struct sockaddr_in *)&msg->remote_addr;
        char str_addr[64];
        inet_ntop(AF_INET, &(clientaddr4->sin_addr), str_addr, 64);
        
        agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "agc_gtp_handle_write connection addr4 %s .\n", str_addr);
    } else {
        struct sockaddr_in6 *clientaddr6 = (struct sockaddr_in6 *)&msg->remote_addr;
        char str_addr[64];
        inet_ntop(AF_INET6, &(clientaddr6 ->sin6_addr), str_addr, 64);
        
        agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "agc_gtp_handle_write connection addr6 %s .\n", str_addr);
    }       

    if (sendto(sock->sock_fd, msg->buf, msg->len, 0, (struct sockaddr *)&msg->remote_addr, msg->remote_addrlen) < 0) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_gtp_handle_write write failed %d.\n",  connection->fd);
        return;
    } 

    agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "agc_gtp_handle_write finished sock=0x%x len=%d.\n", sock, msg->len);
    agc_safe_free(msg);
    */
}

static void agc_gtp_handle_error(void *data)
{
    agc_connection_t *connection = (agc_connection_t *)data;
    agc_gtp_sock_t *sock = (agc_gtp_sock_t *)connection->context;

    if (sock->handler)
    {
        int32_t len = -1;
        agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "agc_gtp_handle_error sock=%d, len=%d.\n", sock->sock_fd, len);
        sock->handler(sock, NULL, &len, NULL, 0);
    }
}

static void agc_gtp_handle_newconnection(agc_connection_t *c)
{
    agc_std_socket_t new_socket;
    agc_std_sockaddr_t *conn_addr;
    agc_connection_t *connection;
    agc_memory_pool_t *pool = NULL;
    agc_gtp_sock_t *sock;
    agc_gtp_sock_t *server_sock = (agc_gtp_sock_t *)c->context;

    if (agc_memory_create_pool(&pool) != AGC_STATUS_SUCCESS) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_gtp_handle_newconnection alloc memory failed .\n");
        return;        
    }

    sock = (agc_gtp_sock_t *)agc_memory_alloc(pool, sizeof(agc_gtp_sock_t));
    sock->handler       = server_sock->handler;
    sock->pool          = pool;
    sock->connection    = NULL;
    sock->mutex         = NULL;
    sock->buf_header    = NULL;
    sock->buf_tail      = NULL;

    new_socket = c->fd; /*accept(c->fd, (struct sockaddr*)&sock->remote_addr, 
        &sock->remote_addrlen);
    if (new_socket == -1) {
        agc_memory_destroy_pool(&pool);
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_gtp_handle_newconnection accept socket failed .\n");
        return;
    }
    */

    getpeername(c->fd, (struct sockaddr*)&sock->remote_addr, 
        &sock->remote_addrlen);
    
    conn_addr = &sock->remote_addr;
    if (conn_addr) {
        if (conn_addr->ss_family == AF_INET) {
            struct sockaddr_in *clientaddr4 = (struct sockaddr_in *)conn_addr;
            char str_addr[64];
            inet_ntop(AF_INET, &(clientaddr4->sin_addr), str_addr, 64);
            
            agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "agc_gtp_handle_newconnection connection addr4 %s .\n", str_addr);
        } else {
            struct sockaddr_in6 *clientaddr6 = (struct sockaddr_in6 *)conn_addr;
            char str_addr[64];
            inet_ntop(AF_INET6, &(clientaddr6 ->sin6_addr), str_addr, 64);
            
            agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "agc_gtp_handle_newconnection connection addr6 %s .\n", str_addr);
        }       
    }

    connection = agc_conn_create_connection(new_socket, 
                      (agc_std_sockaddr_t *)&sock->remote_addr, 
                      sock->remote_addrlen, 
                      pool,
                      sock, 
                      agc_gtp_handle_read,
                      agc_gtp_handle_write,
                      agc_gtp_handle_error);

    if (!connection) {
        agc_memory_destroy_pool(&pool);
        close(new_socket);
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_gtp_handle_newconnection alloc memory failed .\n");
        return;
    }

    if (agc_driver_add_connection(connection) != AGC_STATUS_SUCCESS ) {
        agc_free_connection(connection);
        agc_memory_destroy_pool(&pool);
        close(new_socket);
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_gtp_handle_newconnection add connection failed .\n");
        return;
    }

    agc_mutex_init(&sock->mutex, AGC_MUTEX_NESTED, pool);
    sock->connection = connection;
    sock->sock_fd = new_socket;
    agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "agc_gtp_handle_newconnection add connection success .\n");
}

agc_status_t agc_gtp_open(agc_gtp_sock_t **sock,
    agc_std_sockaddr_t *local_addr, socklen_t addrlen,
     gtp_msg_handler_t handler)
{
    agc_connection_t *connection;
    agc_std_socket_t fd;
    int one = 1;
    agc_memory_pool_t *pool = NULL;
    int flags = 0;
    if (agc_memory_create_pool(&pool) != AGC_STATUS_SUCCESS) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_gtp_open alloc memory failed .\n");
        return AGC_STATUS_FALSE;        
    }

    *sock = (agc_gtp_sock_t *)agc_memory_alloc(pool, sizeof(agc_gtp_sock_t));
    (*sock)->handler        = handler;
    (*sock)->pool           = pool;
    (*sock)->connection     = NULL;
    (*sock)->mutex          = NULL;
    (*sock)->buf_header     = NULL;
    (*sock)->buf_tail       = NULL;

    fd = socket(local_addr->ss_family, SOCK_DGRAM, 0);
    if (fd <= 0)
    {
        agc_memory_destroy_pool(&pool);
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_gtp_open Create gtp socket failed.\n");
        return AGC_STATUS_FALSE;
    }

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&one, sizeof(int));
    if (bind(fd, (struct sockaddr *)local_addr, addrlen) != 0)
    {
        agc_memory_destroy_pool(&pool);
        close(fd);
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_gtp_open bind(%d) failed(%d:%s)\n",
                local_addr->ss_family, errno, strerror(errno));
        return AGC_STATUS_FALSE;
    }
    
    flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    connection = agc_conn_create_connection(fd, 
                      (agc_std_sockaddr_t *)&(*sock)->remote_addr, 
                      (*sock)->remote_addrlen, 
                      pool,
                      (*sock), 
                      agc_gtp_handle_read,
                      agc_gtp_handle_write,
                      agc_gtp_handle_error);

    if (!connection) {
        agc_memory_destroy_pool(&pool);
        close(fd);
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_gtp_open create connection failed .\n");
        return AGC_STATUS_FALSE;
    }

    if (agc_driver_add_connection(connection) != AGC_STATUS_SUCCESS) {
        agc_free_connection(connection);
        agc_memory_destroy_pool(&pool);
        close(fd);
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_gtp_open add connection failed .\n");
        return AGC_STATUS_FALSE;
    }

    agc_mutex_init(&(*sock)->mutex, AGC_MUTEX_NESTED, pool);
    (*sock)->sock_fd      = fd;
    (*sock)->handler      = handler;
    (*sock)->connection   = connection;

    connection->context = *sock;

    agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "agc_gtp_open add server success sock=0x%x sock_fd=%d.\n ", *sock, fd); 

    return AGC_STATUS_SUCCESS;
}

agc_status_t agc_gtp_open_v2(agc_gtp_sock_t **sock,
    agc_std_sockaddr_t *local_addr, socklen_t local_addrlen,
    agc_std_sockaddr_t *remote_addr, socklen_t remote_addrlen,
     gtp_msg_handler_t handler)
{
	agc_connection_t *connection;
	agc_std_socket_t fd;
	int one = 1;
	agc_memory_pool_t *pool = NULL;
	int flags = 0;
	agc_gtp_sock_t *new_sock = NULL;
	
	if (agc_memory_create_pool(&pool) != AGC_STATUS_SUCCESS) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_gtp_open_v2 alloc memory failed .\n");
		return AGC_STATUS_FALSE;        
	}

	new_sock = (agc_gtp_sock_t *)agc_memory_alloc(pool, sizeof(agc_gtp_sock_t));
	agc_assert(new_sock);

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "agc_gtp_open_v2 alloc new_sock %p .\n", (void *)new_sock);
	new_sock->handler        = handler;
	new_sock->pool           = pool;
	new_sock->connection     = NULL;
	new_sock->mutex          = NULL;
	new_sock->buf_header     = NULL;
	new_sock->buf_tail       = NULL;

	fd = socket(local_addr->ss_family, SOCK_DGRAM, 0);
	if (fd <= 0)
	{
		agc_memory_destroy_pool(&pool);
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_gtp_open_v2 Create gtp socket failed.\n");
		return AGC_STATUS_FALSE;
	}

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&one, sizeof(int));
	if (bind(fd, (struct sockaddr *)local_addr, local_addrlen) != 0)
	{
		agc_memory_destroy_pool(&pool);
		close(fd);
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_gtp_open_v2 bind(%d) failed(%d:%s)\n",
                local_addr->ss_family, errno, strerror(errno));
		return AGC_STATUS_FALSE;
	}
    
	flags = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	
	if (connect(fd, (struct sockaddr *)remote_addr, remote_addrlen) < 0) {
		agc_memory_destroy_pool(&pool);
		close(fd);
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_gtp_open_v2 connect(%d) failed(%d:%s)\n",
                local_addr->ss_family, errno, strerror(errno));
		return AGC_STATUS_FALSE;
	}
	
	connection = agc_conn_create_connection(fd, 
                      (agc_std_sockaddr_t *)&remote_addr, 
                      remote_addrlen, 
                      pool,
                      new_sock, 
                      agc_gtp_handle_read_v2,
                      agc_gtp_handle_write,
                      agc_gtp_handle_error);

	if (!connection) {
		agc_memory_destroy_pool(&pool);
		close(fd);
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_gtp_open_v2 create connection failed .\n");
		return AGC_STATUS_FALSE;
	}

	 if (agc_driver_add_connection(connection) != AGC_STATUS_SUCCESS) {
		agc_free_connection(connection);
		agc_memory_destroy_pool(&pool);
		close(fd);
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_gtp_open_v2 add connection failed .\n");
		return AGC_STATUS_FALSE;
	}

	agc_mutex_init(&new_sock->mutex, AGC_MUTEX_NESTED, pool);
	new_sock->sock_fd      = fd;
	new_sock->handler      = handler;
	new_sock->connection   = connection;
	new_sock->remote_addrlen = remote_addrlen;
	memcpy(&new_sock->remote_addr, remote_addr, remote_addrlen);
	new_sock->local_addrlen = local_addrlen;
	memcpy(&new_sock->local_addr, local_addr, local_addrlen);

	connection->context = new_sock;
	*sock = new_sock;

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "agc_gtp_open_v2 add server success sock=0x%x sock_fd=%d.\n ", new_sock,  fd); 

	return AGC_STATUS_SUCCESS;
}

agc_status_t agc_gtp_server(agc_gtp_sock_t **sock,
    agc_std_sockaddr_t *local_addr, socklen_t addrlen,
     gtp_msg_handler_t handler)
{
    agc_listening_t *listening;
    agc_connection_t *connection;
    agc_std_socket_t fd;
    int one = 1;
    agc_memory_pool_t *pool = NULL;

    if (agc_memory_create_pool(&pool) != AGC_STATUS_SUCCESS) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_gtp_client alloc memory failed .\n");
        return AGC_STATUS_FALSE;        
    }

    *sock = (agc_gtp_sock_t *)agc_memory_alloc(pool, sizeof(agc_gtp_sock_t));
    (*sock)->handler        = handler;
    (*sock)->pool           = pool;
    (*sock)->connection     = NULL;
    (*sock)->mutex          = NULL;
    (*sock)->buf_header     = NULL;
    (*sock)->buf_tail       = NULL;

    fd = socket(local_addr->ss_family, SOCK_DGRAM, 0);
    if (fd <= 0)
    {
        agc_memory_destroy_pool(&pool);
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_gtp_server Create gtp socket failed.\n");
        return AGC_STATUS_FALSE;
    }

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&one, sizeof(int));
    if (bind(fd, (struct sockaddr *)local_addr, addrlen) != 0)
    {
        agc_memory_destroy_pool(&pool);
        close(fd);
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_gtp_server bind(%d) failed(%d:%s)\n",
                local_addr->ss_family, errno, strerror(errno));
        return AGC_STATUS_FALSE;
    }

    (*sock)->sock_fd      = fd;
    (*sock)->handler      = handler;
    (*sock)->connection   = connection;

    connection->context = *sock;

    agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "agc_gtp_server add server success sock=0x%x sock_fd=%d.\n ", *sock, fd); 

    return AGC_STATUS_SUCCESS;
}

agc_status_t agc_gtp_client(agc_gtp_sock_t **sock, 
    agc_std_sockaddr_t *local_addr, socklen_t addrlen, 
    agc_std_sockaddr_t *remote_addr, socklen_t remote_addrlen,
    gtp_msg_handler_t handler)
{
    struct timeval tv; 
    int valopt; 
    long arg;
    agc_std_socket_t fd;
    agc_memory_pool_t *pool = NULL;
    int one = 1;

    if (agc_memory_create_pool(&pool) != AGC_STATUS_SUCCESS) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_gtp_client alloc memory failed .\n");
        return AGC_STATUS_FALSE;        
    }

    *sock = (agc_gtp_sock_t *)agc_memory_alloc(pool, sizeof(agc_gtp_sock_t));
    (*sock)->handler        = handler;
    (*sock)->pool           = pool;
    (*sock)->connection     = NULL;
    (*sock)->buf_header     = NULL;
    (*sock)->buf_tail       = NULL;
    (*sock)->local_addrlen  = addrlen;
    (*sock)->remote_addrlen = remote_addrlen;

    memcpy(&(*sock)->local_addr, local_addr, addrlen);
    memcpy(&(*sock)->remote_addr, remote_addr, remote_addrlen);

    fd = socket(local_addr->ss_family, SOCK_DGRAM, 0);
    if (fd <= 0)
    {
        agc_memory_destroy_pool(&pool);
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_gtp_client Create gtp socket failed.\n");
        return AGC_STATUS_FALSE;
    }

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&one, sizeof(int));
    if (bind(fd, (struct sockaddr *)local_addr, addrlen) != 0)
    {
        agc_memory_destroy_pool(&pool);
        close(fd);
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_gtp_server bind(%d) failed(%d:%s)\n",
                local_addr->ss_family, errno, strerror(errno));
        return AGC_STATUS_FALSE;
    }

    (*sock)->connection = agc_conn_create_connection(fd, 
                                                      (agc_std_sockaddr_t *)&(*sock)->remote_addr, 
                                                      (*sock)->remote_addrlen, 
                                                      pool,
                                                      (*sock), 
                                                      agc_gtp_handle_read,
                                                      agc_gtp_handle_write,
                                                      agc_gtp_handle_error);

    if (!(*sock)->connection) {
        agc_memory_destroy_pool(&pool);
        close(fd);
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_gtp_client create connn failed .\n");
        return AGC_STATUS_FALSE;
    }

    if (agc_driver_add_connection((*sock)->connection) != AGC_STATUS_SUCCESS ) {
        agc_free_connection((*sock)->connection);
        agc_memory_destroy_pool(&pool);
        close(fd);
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_gtp_client add connection failed .\n");
        return AGC_STATUS_FALSE;
    }

    agc_mutex_init(&(*sock)->mutex, AGC_MUTEX_NESTED, pool);
    (*sock)->sock_fd = fd;

    agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "agc_gtp_client add client success sock=0x%x sock_fd=%d.\n ", *sock, fd); 

    return AGC_STATUS_SUCCESS;
}

agc_status_t agc_gtp_close(agc_gtp_sock_t *sock)
{
    agc_memory_pool_t *pool = NULL;
    if (sock == NULL) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_gtp_close Error sock is invalid"); 
        return AGC_STATUS_FALSE;
    }

    agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "agc_gtp_close sock is cloesd sock=0x%x sock_fd=%d.\n ", sock, sock->sock_fd); 

    close(sock->sock_fd);

    if (sock->connection) {
        agc_driver_del_connection(sock->connection);
    }

    if (sock->pool) {
        agc_memory_destroy_pool(&sock->pool);
    }

    return AGC_STATUS_SUCCESS;
}
