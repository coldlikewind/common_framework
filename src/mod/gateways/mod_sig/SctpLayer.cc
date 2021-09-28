#include <sys/param.h>
#include "SctpLayer.h"
#include "DbManager.h"

typedef struct agc_sctp_event_context
{
	sctp_context_t *context;
	char *buf;
	int32_t len;
	agc_sctp_sock_t sock;
	sctp_msg_read_handler_t handler;
	uint32_t sctp_index;
	uint32_t idSigGW;
	uint32_t max_stream_no;
	uint32_t stream_no;
}agc_sctp_event_context;

static sig_alarm sctp_alarm;

SctpLayerDrv::SctpLayerDrv() 
{
	//listening = NULL;
	//connection = NULL;
	m_mutex = NULL;
	m_curContextId = 0;
}

SctpLayerDrv::~SctpLayerDrv()
{

}

bool SctpLayerDrv::Init(sctp_msg_read_handler_t server_recv_handler, sctp_error_handler_t server_err_handler,
		sctp_msg_read_handler_t client_recv_handler, sctp_error_handler_t client_err_handler)
{
	agc_memory_pool_t *module_pool = agcsig_get_sig_memory_pool();
	agc_mutex_init(&m_mutex, AGC_MUTEX_NESTED, module_pool);
	
	GetDbManager().QuerySctpLinks(links);
	
	//std::vector<Cfg_LteSctp *> server_links;
	//GetDbManager().QuerySctpServerLinks(server_links);
	for (int i = 0; i < links.size(); i++)
	{
        Cfg_LteSctp *sctp = links[i];
        if (sctp->WorkType == SCTP_WT_CLIENT) {
            continue;
        }
		agc_sctp_config_t cfg;
		cfg.outbound_stream_num = sctp->SctpNum;
		cfg.hbinvertal = sctp->HeartBeatTime_s;
		cfg.path_max_retrans = 3;
		cfg.init_max_retrans = 3;
		cfg.rto_initial = 300;
		cfg.rto_min = 200;
		cfg.rto_max = 500;
		
		if (sctp->LocalIP1Len) {
		    int j = FindServerAddr(sctp->LocalIP1, sctp->LocalIP1Len);
		    if ( j < 0) {
                bool bRet = GetSctpLayerDrv().AddSctpServer(&cfg, (agc_std_sockaddr_t *) &sctp->LocalIP1,
                                                            sctp->LocalIP1Len, server_recv_handler, server_err_handler);
                if (bRet) {
                    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "init sctp server conn1 listen success: %d.\n", sctp->idSctpIndex);
                } else {
                    agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "init sctp server conn1 listen failure: %d.\n", sctp->idSctpIndex);
                }
            } else {
		        serverContextList[j]->addr_reference++;
                agc_log_printf(AGC_LOG, AGC_LOG_INFO, "init sctp server conn1 %d add ref %d.\n",
                        sctp->idSctpIndex, serverContextList[j]->addr_reference);
		    }
		}

		if (sctp->LocalIP2Len) {
		    int j =  FindServerAddr(sctp->LocalIP2, sctp->LocalIP2Len);
		    if (j < 0) {
                bool bRet = GetSctpLayerDrv().AddSctpServer(&cfg, (agc_std_sockaddr_t *) &sctp->LocalIP2,
                                                            sctp->LocalIP2Len, server_recv_handler, server_err_handler);
                if (bRet) {
                    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "init sctp server conn2 listen success: %d.\n", sctp->idSctpIndex);
                } else {
                    agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "init sctp server conn2 listen failure: %d.\n", sctp->idSctpIndex);
                }
            } else {
		        serverContextList[j]->addr_reference++;
                agc_log_printf(AGC_LOG, AGC_LOG_INFO, "init sctp server conn1 %d add ref %d.\n",
                               sctp->idSctpIndex, serverContextList[j]->addr_reference);
		    }
		}
	}

	m_sctpContexts = (sctp_context_t *)calloc(1, MAX_SCTP_CONTEXTS * sizeof(sctp_context_t));
	for (int i = 0; i < MAX_SCTP_CONTEXTS; i++)
    {
	    SctpContextInit(&m_sctpContexts[i]);
    }

	client_read_handler = client_recv_handler;
	client_err_handler = client_err_handler;
    server_read_handler = server_recv_handler;
    server_err_handler = server_err_handler;

    vector<Cfg_LteSctp *>::iterator it = links.begin();
    for (; it != links.end(); it++) {
        int index = (*it)->idSctpIndex;
        m_sctpContexts[index].sctpStatus = SCTP_CONN_INACTIVE;
        if ((*it)->WorkType == SCTP_WT_CLIENT) {
            clients_links[index] = (*it);
        }
    }
    agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sctp init client number: %d .\n", clients_links.size());

	StartSctpTimer(SCTPLAYER_SCTP_TIMER_VALUE);

	sctp_alarm.set_mo_id(siggw_mo_id);
	sctp_alarm.set_alarm_id(sctp_alarm_id);
	return true;
}


void SctpLayerDrv::Exit()
{
	/*if (connection) {
		agc_memory_destroy_pool(&connection->pool);
		agc_driver_del_connection(connection);
		connection = NULL;
	}

	if (listenfd)
		agc_sctp_close(listenfd);*/

	
	for (int i = 0; i < links.size(); i++)
	{
		delete links[i];
	}
	links.clear();

	for (int i = 0; i < MAX_SCTP_CONTEXTS; i++) {
	    ReleaseClientContext(i);
	}
	free(m_sctpContexts);

	vector<sctp_context_t *>::iterator it = serverContextList.begin();
	for (; it != serverContextList.end(); it++)
    {
        ReleaseServerContext((*it)->local_addr, (*it)->local_addrlen);
        delete((*it));
    }
	serverContextList.clear();
	clients_links.clear();
}

bool SctpLayerDrv::AddSctpServer(agc_sctp_config_t *sctpCfg, 
	agc_std_sockaddr_t *local_addr, socklen_t addrlen, 
	sctp_msg_read_handler_t handler,
	sctp_error_handler_t server_err_handler)
{
    agc_sctp_sock_t listenfd = 0;
    agc_listening_t *listening = NULL;
    agc_connection_t *connection = NULL;

	agc_memory_pool_t *pool = NULL;
    if (agc_memory_create_pool(&pool) != AGC_STATUS_SUCCESS) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::AddSctpServer alloc memory failed .\n");
        return false;        
    }

	agc_status_t status = agc_sctp_server(&listenfd, local_addr, addrlen, sctpCfg);
	if (status == AGC_STATUS_FALSE || listenfd <= 0)
	{
        agc_memory_destroy_pool(&pool);
    	agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::AddSctpServer fail to create server listenfd1\n");
		return false;
	}

	//debug conn addr
	if (local_addr) {
		if (local_addr->ss_family == AF_INET) {
			struct sockaddr_in *clientaddr4 = (struct sockaddr_in *)local_addr;
			char str_addr[64];
			inet_ntop(AF_INET, &(clientaddr4->sin_addr), str_addr, 64);
			
			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "SctpLayerDrv::AddSctpServer connection addr4 %s port=%d.\n", str_addr, ntohs(clientaddr4->sin_port));
		} else {
			struct sockaddr_in6 *clientaddr6 = (struct sockaddr_in6 *)local_addr;
			char str_addr[64];
			inet_ntop(AF_INET6, &(clientaddr6 ->sin6_addr), str_addr, 64);
			
			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "SctpLayerDrv::AddSctpServer connection addr6 %s port=%d.\n", str_addr, ntohs(clientaddr6->sin6_port));
		}		
	}

	long arg;
	if( (arg = fcntl(listenfd, F_GETFL, NULL)) < 0) {
        agc_memory_destroy_pool(&pool);
		agc_sctp_close(listenfd);
    	agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::AddSctpServer fcntl get fail.\n");
		return false;
	} 

	arg |= O_NONBLOCK; 
	if( fcntl(listenfd, F_SETFL, arg) < 0) {
        agc_memory_destroy_pool(&pool);
		agc_sctp_close(listenfd);
    	agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::AddSctpServer fail to fcntl set\n");
		return false;
	} 

	listening = agc_conn_create_listening(listenfd, (agc_std_sockaddr_t *)local_addr,
							addrlen, 
							pool,
							SctpLayerDrv::handle_newconnection);

	if (!listening) {
        agc_memory_destroy_pool(&pool);
		agc_sctp_close(listenfd);
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::AddSctpServer create listening failed .\n");
		return false;
	}

	//todo listening释放
	connection = agc_conn_get_connection(listening);

	if (!connection) {
        agc_memory_destroy_pool(&pool);
		agc_sctp_close(listenfd);
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::AddSctpServer create connection failed .\n");
		return false;
	}

	if (agc_driver_add_connection(connection) != AGC_STATUS_SUCCESS) {
        agc_memory_destroy_pool(&pool);
		agc_free_connection(connection);
		agc_sctp_close(listenfd);
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::AddSctpServer add connection failed .\n");
		return false;
	}

	sctp_context_t *new_context = new sctp_context_t;
	if (!new_context) {
        agc_memory_destroy_pool(&pool);
		agc_free_connection(connection);
		agc_sctp_close(listenfd);
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::AddSctpServer invalid new connection .\n");
		return false;
	}

	m_curContextId++;
	new_context->context_id = m_curContextId;
	new_context->local_addrlen = addrlen;
	new_context->handler = handler;
	new_context->err_handler = err_handler;
	new_context->context = NULL;
	new_context->connection = connection;
	new_context->connection->pool = pool;
	new_context->sock = listenfd;
	new_context->connection->context = new_context;
	new_context->client = false;
	new_context->sctp_index = 65535;
	new_context->idSigGW = 0;
	new_context->addr_reference = 1;
	agc_mutex_init(&new_context->mutex, AGC_MUTEX_NESTED, pool);
	
	memcpy(&new_context->local_addr, local_addr, addrlen);
	
	read_handler = handler;
	err_handler = server_err_handler;
	server_context = NULL;

    serverContextList.push_back(new_context);

	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "SctpLayerDrv::AddSctpServer succeed listenfd=%d.\n", listenfd);
	return true;
}

	
agc_sctp_sock_t SctpLayerDrv::clientConnect(agc_sctp_config_t *sctpCfg,  		agc_std_sockaddr_t *local_addr, socklen_t addrlen,
		agc_std_sockaddr_t *remote_addr, socklen_t remote_addrlen)
{
	struct timeval tv; 
	int valopt; 
	agc_sctp_sock_t fd = 0;
	agc_sctp_client(&fd, local_addr, addrlen, sctpCfg);
	if (fd <= 0)
	{
    	agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::clientConntect fail to create client fd\n");
		return 0;
	}

	long arg;
	if( (arg = fcntl(fd, F_GETFL, NULL)) < 0) { 
		agc_sctp_close(fd);
    	agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::clientConntect fcntl get fail.\n");
		return 0;
	} 

	arg |= O_NONBLOCK; 
	if( fcntl(fd, F_SETFL, arg) < 0) { 
		agc_sctp_close(fd);
    	agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::clientConntect fail to fcntl set\n");
		return 0;
	} 

	connect(fd, (struct sockaddr *)remote_addr, remote_addrlen);
	if (errno == EINPROGRESS) { 
  		fd_set myset; 
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "SctpLayerDrv::clientConntect EINPROGRESS in connect() - selecting\n"); 
		do { 
			tv.tv_sec = 1; 
			tv.tv_usec = 0; 

			FD_ZERO(&myset); 
			FD_SET(fd, &myset); 
			int res = select(fd+1, NULL, &myset, NULL, &tv); 
			if (res < 0 && errno != EINTR) { 
				agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::clientConntect Error connecting %d - %s\n", errno, strerror(errno)); 
				agc_sctp_close(fd);
				return 0;
			} 
			else if (res > 0) { 
				// Socket selected for write 
				socklen_t lon = sizeof(int); 
				if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0) { 
					agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::clientConntect Error in getsockopt() %d - %s\n", errno, strerror(errno)); 
					agc_sctp_close(fd);
					return 0;
				} 
				if (valopt) { 
					agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::clientConntect Error in delayed connection() %d - %s\n", valopt, strerror(valopt)); 
					agc_sctp_close(fd);
					return 0;
				} 
				break; 
			} 
			else { 
				agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::clientConntect Timeout in select() - Cancelling!\n"); 
				agc_sctp_close(fd);
				return 0;
			} 
		} while (1); 
	} 
	else { 
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::clientConntect Error connecting %d - %s\n", errno, strerror(errno)); 
		agc_sctp_close(fd);
		return 0;
	} 

	return fd;
}
	
bool SctpLayerDrv::SctpClientConnect(sctp_context_t* context)
{
	struct timeval tv; 
	int valopt; 
	agc_sctp_sock_t fd = clientConnect(&context->sctpCfg, &context->local_addr, context->local_addrlen,
		&context->remote_addr, context->remote_addrlen);
	if (fd <= 0)
	{
    	agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::SctpClientConnect fail to create client fd\n");
		return false;
	}
	
	context->sock = fd;
	if (context->local_addrlen2 > 0 ) {
		context->sock2 = clientConnect(&context->sctpCfg, &context->local_addr2, context->local_addrlen2,
			&context->remote_addr2, context->remote_addrlen2);
	}
	return true;
}

bool SctpLayerDrv::AddSctpClient(agc_sctp_config_t *sctpCfg, 
	uint32_t sctp_index,
	Cfg_LteSctp *sctp,
	sctp_msg_read_handler_t handler,
	void *context,
	sctp_error_handler_t err_handler)
{
	agc_memory_pool_t *pool = NULL;
	sctp_context_t *new_context = NULL;

    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "AddSctpClient sctp_index=%d begin.\n", sctp_index);

	if (sctp_index >= MAX_SCTP_CONTEXTS || sctp == NULL){
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::AddSctpClient invalid new connection sctp_index=%d.\n", sctp_index);
		return false;
	}
	
	new_context = &m_sctpContexts[sctp_index];
	if (new_context->connection)
	{
		agc_driver_del_connection(new_context->connection);
		if (new_context->connection->pool) {
            agc_memory_destroy_pool(&new_context->connection->pool);
        }
		new_context->connection = NULL;
	}
	m_curContextId++;
	new_context->context_id = m_curContextId;	
	new_context->local_addrlen = sctp->LocalIP1Len;
	new_context->remote_addrlen = sctp->PeerIP1Len;
	new_context->local_addrlen2 = sctp->LocalIP2Len;
	new_context->remote_addrlen2 = sctp->PeerIP2Len;
	new_context->client = true;
	new_context->sctp_index = sctp_index;
	new_context->idSigGW = sctp->idSigGW;
	new_context->max_stream_no = sctp->SctpNum;
	memcpy(&new_context->local_addr, &sctp->LocalIP1, new_context->local_addrlen);
	memcpy(&new_context->remote_addr, &sctp->PeerIP1, new_context->remote_addrlen);
	memcpy(&new_context->sctpCfg, sctpCfg, sizeof(agc_sctp_config_t));

	if (new_context->local_addrlen2 > 0)
	{
		memcpy(&new_context->local_addr2, &sctp->LocalIP2, new_context->local_addrlen2);
	}
	
	if (new_context->remote_addrlen2 > 0)
	{
		memcpy(&new_context->remote_addr2, &sctp->PeerIP2, new_context->remote_addrlen2);
	}

	if (SctpClientConnect(new_context) == false) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::AddSctpClient invalid new connection .\n");
		return false;
	}

    if (agc_memory_create_pool(&pool) != AGC_STATUS_SUCCESS) {
		agc_sctp_close(new_context->sock);
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::AddSctpClient alloc memory failed .\n");
        return false;        
    }	

	new_context->connection = agc_conn_create_connection(new_context->sock, 
                                                      (agc_std_sockaddr_t *)&new_context->remote_addr, 
                                                      new_context->remote_addrlen, 
                                                      pool,
                                                      new_context, 
                                                      SctpLayerDrv::handle_read,
                                                      SctpLayerDrv::handle_write,
                                                      SctpLayerDrv::handle_error);

	if (!new_context->connection) {
        agc_memory_destroy_pool(&pool);
		agc_sctp_close(new_context->sock);
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::AddSctpClient alloc memory failed .\n");
		return false;
	}

	if (agc_driver_add_connection(new_context->connection) != AGC_STATUS_SUCCESS ) {
        agc_memory_destroy_pool(&pool);
		agc_sctp_close(new_context->sock);
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::AddSctpClient add connection failed .\n");
		return false;
	}

	new_context->connection->pool = pool;

	if (new_context->sock2 > 0) {
		new_context->connection2 = agc_conn_create_connection(new_context->sock2, 
	                                                      (agc_std_sockaddr_t *)&new_context->remote_addr2, 
	                                                      new_context->remote_addrlen2, 
	                                                      pool,
	                                                      new_context, 
	                                                      SctpLayerDrv::handle_read,
	                                                      SctpLayerDrv::handle_write,
	                                                      SctpLayerDrv::handle_error);

		if (!new_context->connection2) {
			agc_sctp_close(new_context->sock2);
			agc_log_printf(AGC_LOG, AGC_LOG_WARNING, "SctpLayerDrv::AddSctpClient alloc memory failed .\n");
		}
		else {
			new_context->connection2->context = new_context; 
		}
		
		if (agc_driver_add_connection(new_context->connection2) != AGC_STATUS_SUCCESS ) {
			agc_sctp_close(new_context->sock2);
			agc_log_printf(AGC_LOG, AGC_LOG_WARNING, "SctpLayerDrv::AddSctpClient add connection failed .\n");
		}

        new_context->connection2->pool = pool;
	}

	new_context->context = context;
	new_context->handler = handler;
	new_context->err_handler = err_handler;
	new_context->connection->context = new_context;
	new_context->sctpStatus = SCTP_CONN_ACTIVE;
	agc_mutex_init(&new_context->mutex, AGC_MUTEX_NESTED, pool);
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "SctpLayerDrv::AddSctpClient succeed fd=%d fd2=%d sctp_index=%d.\n", 
		new_context->sock, new_context->sock2, sctp_index);
	
	return true;
}
/*
bool SctpLayerDrv::AddSctpClient(agc_sctp_config_t *sctpCfg, 
		agc_std_sockaddr_t *local_addr, socklen_t addrlen,
		agc_std_sockaddr_t *remote_addr, socklen_t remote_addrlen,
		sctp_msg_read_handler_t handler, void *context, uint32_t sctp_index, uint32_t idSigGW, uint32_t max_stream_no,
		sctp_error_handler_t err_handler)
{
	agc_memory_pool_t *pool = NULL;
	sctp_context_t *new_context = NULL;
	if (sctp_index >= MAX_SCTP_CONTEXTS){
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::AddSctpClient invalid new connection sctp_index=%d.\n", sctp_index);
		return false;
	}
	
	new_context = &m_sctpContexts[sctp_index];	

	if (new_context->connection)
	{
		agc_driver_del_connection(new_context->connection);
		agc_memory_destroy_pool(&new_context->connection->pool);

		new_context->connection = NULL;
	}
	m_curContextId++;
	new_context->context_id = m_curContextId;	
	new_context->local_addrlen = addrlen;
	new_context->remote_addrlen = remote_addrlen;
	new_context->client = true;
	new_context->sctp_index = sctp_index;
	new_context->idSigGW = idSigGW;
	new_context->max_stream_no = max_stream_no;
	memcpy(&new_context->local_addr, local_addr, addrlen);
	memcpy(&new_context->remote_addr, remote_addr, remote_addrlen);
	memcpy(&new_context->sctpCfg, sctpCfg, sizeof(agc_sctp_config_t));

	if (!new_context) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::AddSctpClient invalid new connection .\n");
		return false;
	}

	if (SctpClientConnect(new_context) == false) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::AddSctpClient invalid new connection .\n");
		return false;
	}
	
    if (agc_memory_create_pool(&pool) != AGC_STATUS_SUCCESS) {
		agc_sctp_close(new_context->sock);
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::AddSctpClient alloc memory failed .\n");
        return false;        
    }	

	new_context->connection = agc_conn_create_connection(new_context->sock, 
                                                      (agc_std_sockaddr_t *)&new_context->remote_addr, 
                                                      new_context->remote_addrlen, 
                                                      pool,
                                                      new_context, 
                                                      SctpLayerDrv::handle_read,
                                                      SctpLayerDrv::handle_write,
                                                      SctpLayerDrv::handle_error);

	if (!new_context->connection) {
        agc_memory_destroy_pool(&pool);
		agc_sctp_close(new_context->sock);
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::AddSctpClient alloc memory failed .\n");
		return false;
	}

	if (agc_driver_add_connection(new_context->connection) != AGC_STATUS_SUCCESS ) {
        agc_memory_destroy_pool(&pool);
		agc_sctp_close(new_context->sock);
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::AddSctpClient add connection failed .\n");
		return false;
	}

	new_context->context = context;
	new_context->handler = handler;
	new_context->err_handler = err_handler;
	new_context->connection->context = new_context; 
	agc_mutex_init(&new_context->mutex, AGC_MUTEX_NESTED, pool);
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "SctpLayerDrv::AddSctpClient succeed fd=%d sctp_index=%d.\n", 
		new_context->sock, sctp_index);

	return true;
}
*/
void SctpLayerDrv::handle_newconnection(agc_connection_t *c)
{	
	//while (1)
	{
		agc_std_socket_t new_socket;
        agc_connection_t *new_connection;
		agc_std_sockaddr_t *listen_addr = NULL;
		agc_std_sockaddr_t *conn_addr = NULL;
		agc_memory_pool_t *pool = NULL;
		agc_std_sockaddr_t remote_addr;
		socklen_t remote_addrlen;
		char str_remote_addr[64];
		uint16_t remote_port = 0;
		
		//debug listen addr
		listen_addr = c->sockaddr;
		if (listen_addr) {
			if (listen_addr->ss_family == AF_INET) {
				struct sockaddr_in *listen_addr4 = (struct sockaddr_in *)listen_addr;
				char str_addr[64];
				inet_ntop(AF_INET, &(listen_addr4->sin_addr), str_addr, 64);
				agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "debug listen addr4 %s .\n", str_addr);

				remote_addrlen = sizeof(struct sockaddr_in);
			} else {
				struct sockaddr_in6 *listen_addr6 = (struct sockaddr_in6 *)listen_addr;
				char str_addr[64];
				inet_ntop(AF_INET6, &(listen_addr6->sin6_addr), str_addr, 64);
				agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "debug listen addr6 %s .\n", str_addr);

				remote_addrlen = sizeof(struct sockaddr_in6);
			}
		}
		//(struct sockaddr*)
		new_socket = accept(c->fd, (struct sockaddr*)&remote_addr, 
			&remote_addrlen);

		if (new_socket == -1) {
			return;
		}

		long arg;
		if( (arg = fcntl(new_socket, F_GETFL, NULL)) < 0) {
		    close(new_socket);
	    	agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::handle_newconnection fcntl get fail.\n");
			return;
		} 

		arg |= O_NONBLOCK; 
		if( fcntl(new_socket, F_SETFL, arg) < 0) {
            close(new_socket);
	    	agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::handle_newconnection fail to fcntl set\n");
			return;
		} 
		
	    int one = 1;
	    if (setsockopt(new_socket, IPPROTO_SCTP, SCTP_NODELAY,  &one, sizeof(one)) != 0) 
		{
            close(new_socket);
	        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_sctp_server setsockopt for SCTP_NODELAY failed");
	        return;
	    }
										
		uint32_t sctp_index = 0;
		uint32_t idSigGW = 0;
		uint32_t max_sctp_num = 0;
		if (GetDbManager().QuerySctpLinksByRemoteAddr(remote_addr, sctp_index, idSigGW, max_sctp_num) == false 
			|| idSigGW == 0 || sctp_index >= MAX_SCTP_CONTEXTS)
		{
			close(new_socket);
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::handle_newconnection new connection isn't config addr %s port=%d.\n",
				str_remote_addr, remote_port);
			return;
		}
		
	    if (agc_memory_create_pool(&pool) != AGC_STATUS_SUCCESS) {
			close(new_socket);
	        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::handle_newconnection alloc memory failed new_socket=%d sctp_index=%d.\n", 
	        	new_socket, sctp_index);
	        return;        
	    }
		
		sctp_context_t *new_context = NULL;
		new_context = &GetSctpLayerDrv().m_sctpContexts[sctp_index];
		if (!new_context) {
			close(new_socket);
	        agc_memory_destroy_pool(&pool);
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::handle_newconnection invalid new connection new_socket=%d sctp_index=%d.\n", 
	        	new_socket, sctp_index);
			return;
		}
		
		if (new_context->connection)
		{
			agc_driver_del_connection(new_context->connection);
			agc_memory_destroy_pool(&new_context->connection->pool);
		
			new_context->connection = NULL;
		}

        new_connection = agc_conn_create_connection(new_socket,
                                                    &remote_addr,
                                                    remote_addrlen,
                                                    pool,
                                                    new_context,
                                                    SctpLayerDrv::handle_read,
                                                    SctpLayerDrv::handle_write,
                                                    SctpLayerDrv::handle_error);

        if (new_connection == NULL) {
            agc_memory_destroy_pool(&pool);
            close(new_socket);
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "SctpLayerDrv::handle_newconnection new connection failed %d sctp_index=%d remote=%s port=%d .\n",
                           new_socket, sctp_index, str_remote_addr, remote_port);
            return;
        }

        if (agc_driver_add_connection(new_connection) != AGC_STATUS_SUCCESS ) {
            agc_memory_destroy_pool(&pool);
            close(new_socket);
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "SctpLayerDrv::handle_newconnection add connection failed %d sctp_index=%d remote=%s port=%d .\n",
                           new_socket, sctp_index, str_remote_addr, remote_port);
            return;
        }

        new_context->connection = new_connection;
        new_context->connection->pool = pool;

        memcpy(&new_context->local_addr, c->sockaddr, c->addrlen);
		new_context->local_addrlen = c->addrlen;
		new_context->remote_addrlen = remote_addrlen;
		memcpy(&new_context->remote_addr, &remote_addr, remote_addrlen);
		agc_mutex_init(&new_context->mutex, AGC_MUTEX_NESTED, pool);
		
		//debug conn addr
		conn_addr = &new_context->remote_addr;
		if (conn_addr) {
			if (conn_addr->ss_family == AF_INET) {
				struct sockaddr_in *clientaddr4 = (struct sockaddr_in *)conn_addr;
				inet_ntop(AF_INET, &(clientaddr4->sin_addr), str_remote_addr, 64);
				remote_port = ntohs(clientaddr4->sin_port);
			} else {
				struct sockaddr_in6 *clientaddr6 = (struct sockaddr_in6 *)conn_addr;
				inet_ntop(AF_INET6, &(clientaddr6 ->sin6_addr), str_remote_addr, 64);		
				remote_port = ntohs(clientaddr6->sin6_port);
			}		
		}
		
		new_context->client = false;
		new_context->context_id = GetSctpLayerDrv().m_curContextId;
		new_context->context = GetSctpLayerDrv().server_context;
		new_context->handler = GetSctpLayerDrv().read_handler;
		new_context->err_handler = GetSctpLayerDrv().err_handler;
		new_context->sock = new_socket;
		new_context->sctp_index = sctp_index;
		new_context->idSigGW = idSigGW;
		new_context->max_stream_no = max_sctp_num;
		
		new_context->connection = agc_conn_create_connection(new_socket, 
	                                                      (agc_std_sockaddr_t *)&new_context->remote_addr, 
	                                                      new_context->remote_addrlen, 
	                                                      pool,
	                                                      new_context, 
	                                                      SctpLayerDrv::handle_read,
	                                                      SctpLayerDrv::handle_write,
	                                                      SctpLayerDrv::handle_error);

		if (!new_context->connection) {
	        agc_memory_destroy_pool(&pool);
			close(new_socket);
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::handle_newconnection alloc memory failed %d sctp_index=%d remote=%s port=%d .\n",
				new_socket, sctp_index, str_remote_addr, remote_port);
			return;
		}

		new_context->connection->context = new_context;
		if (agc_driver_add_connection(new_context->connection) != AGC_STATUS_SUCCESS ) {
	        agc_memory_destroy_pool(&pool);
			close(new_socket);
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::handle_newconnection add connection failed %d sctp_index=%d remote=%s port=%d .\n",
				new_socket, sctp_index, str_remote_addr, remote_port);
			return;
		}

		new_context->sctpStatus = SCTP_CONN_ACTIVE;
		if (new_context->alarmStatus == STATUS_ALARM && new_context->alarm_event == NULL) {
		    GetSctpLayerDrv().StartAlarmTimer((void *)new_context);
		}
		agc_log_printf(AGC_LOG, AGC_LOG_INFO, "SctpLayerDrv::handle_newconnection new connection %d sctp_index=%d remote=%s port=%d .\n",
			new_socket, sctp_index, str_remote_addr, remote_port);
	}
}

void SctpLayerDrv::event_callback(void *data)
{
	agc_sctp_event_context *evt = (agc_sctp_event_context *)data;
	
	sctp_context_t *context = (sctp_context_t *)evt->context;
	if (!context) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "event_callback invalid connection no context .\n");
		return;
	}
	
	char *buf = evt->buf;
	int32_t len = evt->len;	

	agc_sctp_sock_t sock = (agc_sctp_sock_t)evt->sock;
	sctp_msg_read_handler_t handler = evt->handler;
	uint32_t sctp_index = evt->sctp_index;
	uint32_t idSigGW = evt->idSigGW;
	uint32_t max_stream_no = evt->max_stream_no;	
	uint32_t stream_no = evt->stream_no;	
	
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "event_callback sctp_index=%d len=%d sock=%d 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x.\n", 
		sctp_index, len, context->sock, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9], buf[10], buf[11], buf[12]);
	
	if (handler) 
	{
		handler(sctp_index,
			stream_no,
			60, 
			buf, 
			&len,
			idSigGW,
			max_stream_no);
	}
	

	delete [] buf;
	delete  evt;
}

agc_status_t SctpLayerDrv::event_create_callback(void *evt)
{
	agc_event_t *new_event = NULL;
	
	if ((agc_event_create_callback(&new_event, 0, (void *)evt, SctpLayerDrv::event_callback) != AGC_STATUS_SUCCESS)) {
		return AGC_STATUS_FALSE;
	}
	agc_event_fire(&new_event);

	return AGC_STATUS_SUCCESS;
}

void SctpLayerDrv::handle_read(void *data)
{
	agc_connection_t *connection = (agc_connection_t *)data;
	sctp_context_t *context = (sctp_context_t *)connection->context;
	agc_sctp_sock_t sock = 0;
	agc_sctp_stream_t stream;
	int32_t len = 0;
	uint32_t sctp_index = 0;
	agc_sctp_event_context *event = NULL;
	int readed = 0;
	
	if (!context) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::handle_read invalid connection no context .\n");
		return;
	}
		
	sctp_index = context->sctp_index;
	sock = (agc_sctp_sock_t)context->sock;
	memset(&stream, 0, sizeof(agc_sctp_stream_t));
	
	while (1)
	{			
		char *buf = new char[AGC_SCTP_MAX_BUFFER];	
		len = 0;
		len = read(sock, buf, AGC_SCTP_MAX_BUFFER - 1);
		if (len <= 0)
		{
		      
			if (errno != EAGAIN || (readed == 0))
			{
				handle_error(data);
			}
			delete buf;
			break;
		}

		readed = 1;

		if (len > 12)
			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "SctpLayerDrv::handle_read read sctp_index=%d len=%d sock=%d, 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x.\n", 
				sctp_index, len, sock, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9], buf[10], buf[11], buf[12]);
		else
			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "SctpLayerDrv::handle_read read sctp_index=%d end. (%d:%s)\n",
                sctp_index, errno, strerror(errno));

		event = new agc_sctp_event_context;
		event->context = context;
		event->buf = buf;
		event->len = len;
		event->sock = (agc_sctp_sock_t)context->sock;
		event->handler = context->handler;
		event->sctp_index = context->sctp_index;
		event->idSigGW = context->idSigGW;
		event->max_stream_no = context->max_stream_no;
		event->stream_no = 1;
		//agc_mutex_unlock(GetSctpLayerDrv().m_mutex);
		
		event_create_callback((void *)event);
	}
}

void SctpLayerDrv::handle_write(void *data)
{
#if 0
	agc_connection_t *connection = (agc_connection_t *)data;
	sctp_context_t *context = (sctp_context_t *)connection->context;
	sctp_message_t *msg = NULL;
	agc_sctp_stream_t stream;
	if (!context) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "invalid connection no context .\n");
		return;
	}

	agc_sctp_sock_t sock = context->sock;
	uint32_t sctp_index = context->sctp_index;
	
	stream.addrlen = context->remote_addrlen;
	memcpy(&stream.remote_addr, &context->remote_addr, context->remote_addrlen);

	do
	{
		agc_mutex_lock(context->mutex);
		if (context->buf.size() > 0)
		{
			msg = context->buf.front();
			context->buf.pop_front();
		}
		
		if (msg == NULL) 
		{
			agc_driver_del_event(connection, AGC_WRITE_EVENT);
			agc_mutex_unlock(context->mutex);
			break;		
		}
		sock = context->sock;
		agc_mutex_unlock(context->mutex);		
		
		stream.ppid = msg->ppid;
		stream.stream_no = msg->stream_no;	
		agc_sctp_send(sock, &stream, msg->buf, &msg->len);
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "SctpLayerDrv::handle_write sctp_index=%d add len %d sock %d stream_no %d 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x..\n", 
			sctp_index, msg->len, sock, stream.stream_no, 
			msg->buf[0], msg->buf[1], msg->buf[2], msg->buf[3], msg->buf[4], msg->buf[5], msg->buf[6], msg->buf[7], msg->buf[8], msg->buf[9]);
		delete msg;
		msg = NULL;
	}	while (sock > 0);
#endif
}

void SctpLayerDrv::handle_error(void *data)
{
	agc_connection_t *connection = (agc_connection_t *)data;
	sctp_context_t *context = (sctp_context_t *)connection->context;
    if (!context) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "invalid connection no context .\n");
        return;
    }
	uint32_t sctp_index = context->sctp_index;

	// first, stop socket write event
	agc_driver_del_event(connection, AGC_WRITE_EVENT);
    if (connection->pool != NULL) {
        agc_memory_destroy_pool(&connection->pool);
    }
    agc_driver_del_connection(connection);
    context->connection = NULL;

	agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::handle_error sctp_index=%d sock=%d.\n", sctp_index, context->sock);

	if (context->err_handler)
		context->err_handler(context->sctp_index);

	agc_sctp_close(context->sock);

    agc_mutex_lock(context->mutex);
    context->sock = 0;
    context->sctpStatus = SCTP_CONN_INACTIVE;
    agc_mutex_unlock(context->mutex);
    if (context->client == true)
    {
        GetSctpLayerDrv().StartSctpClientTimer(SCTP_CLIENT_TIMER_VALUE, (void *)context);
    }
    if (context->alarm_event) {
        agc_timer_del_timer(context->alarm_event);
        context->alarm_event = NULL;
    }
    if (context->alarmStatus == STATUS_NORMAL) {
        context->alarmStatus = STATUS_ALARM;
        sctp_alarm.report_general_alarm(STATUS_ALARM, to_string(sctp_index), "");
    }
    /*
    do
    {
        sctp_message_t *msg = NULL;
        agc_mutex_lock(context->mutex);
        if (!context->buf.empty())
        {
            msg = context->buf.front();
            context->buf.pop_front();
        }

        if (msg == NULL)
        {
            agc_mutex_unlock(context->mutex);
            break;
        }
        agc_mutex_unlock(context->mutex);

        delete msg;
        msg = NULL;
    }	while (1);
    */
}	

bool SctpLayerDrv::AddSctpMessage(uint32_t sctp_index, 
	uint32_t stream_no,
	uint32_t ppid, 
	const char* buf, 
	int32_t *len)
{
	agc_connection_t *conn = NULL;
	if (*len > MAX_SCTP_MSG_LEN || sctp_index >= MAX_SCTP_CONTEXTS)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::AddSctpMessage sctp message len=%d sctp_index=%d exceed max.\n", *len, sctp_index);
		return false;
	}

	sctp_context_t *context = (sctp_context_t *)&m_sctpContexts[sctp_index];

	if (context->sock == 0)
	{
		//agc_mutex_unlock(m_mutex);
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::AddSctpMessage invalid sock sctp_index(%d).\n", sctp_index);
		return false;
	}
	
	agc_status_t status = AGC_STATUS_SUCCESS;
	agc_sctp_stream_t stream;
	
	stream.ppid = ppid;
	stream.stream_no = stream_no;
	stream.addrlen = context->remote_addrlen;
	memcpy(&stream.remote_addr, &context->remote_addr, context->remote_addrlen);
	status = agc_sctp_send(context->sock, &stream, buf, len);	

	if (status == AGC_STATUS_SUCCESS)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "SctpLayerDrv::AddSctpMessage sctp_index=%d len=%d stream_no=%d 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x.\n", 
			sctp_index, *len, stream_no, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9], buf[10], buf[11], buf[12]);
	}
	/*else
	{	
		sctp_message_t *msg = new sctp_message_t;
		memcpy(msg->buf, buf, *len);
		msg->len = *len;
		msg->ppid = ppid;
		msg->stream_no = stream_no;

		agc_mutex_lock(context->mutex);
		conn = context->connection;
		if (conn)
		{
			context->buf.push_back(msg);
			agc_driver_add_event(conn, AGC_WRITE_EVENT);
		}
		else
		{
			delete msg;
		}
		agc_mutex_unlock(context->mutex);
	}
*/
	return true;
}

void SctpLayerDrv::OnSctpClientTimer(void *data)
{
	sctp_context_t *context = (sctp_context_t *)data;

	if (GetSctpLayerDrv().ReconnectSctpClient(context) == false)
	{
		GetSctpLayerDrv().StartSctpClientTimer(SCTP_CLIENT_TIMER_VALUE, data);
	}
}

bool SctpLayerDrv::ReconnectSctpClient(sctp_context_t* context)
{
	agc_memory_pool_t *pool = NULL;
    if (agc_memory_create_pool(&pool) != AGC_STATUS_SUCCESS) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::ReconnectSctpClient alloc memory failed .\n");
        return false;        
    }
	
	if (SctpClientConnect(context) == false) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::ReconnectSctpClient fail to connect sctp server .\n");
		return false;
	}

	context->connection = agc_conn_create_connection(context->sock, 
                              (agc_std_sockaddr_t *)&context->remote_addr, 
                              context->remote_addrlen, 
                              pool,
                              context, 
                              SctpLayerDrv::handle_read,
                              SctpLayerDrv::handle_write,
                              SctpLayerDrv::handle_error);

	if (!context->connection) {
	    agc_memory_destroy_pool(&pool);
		agc_sctp_close(context->sock);
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::ReconnectSctpClient alloc memory failed .\n");
		return false;
	}

	if (agc_driver_add_connection(context->connection) != AGC_STATUS_SUCCESS ) {
        agc_memory_destroy_pool(&pool);
		agc_sctp_close(context->sock);
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::ReconnectSctpClient add connection failed .\n");
		return false;
	}

	context->connection->context = context;
	context->connection->pool = pool;
	context->sctpStatus = SCTP_CONN_ACTIVE;
	if (context->alarmStatus == STATUS_ALARM && context->alarm_event == NULL) {
	    StartAlarmTimer((void *)context);
	}
	return true;
}

void SctpLayerDrv::StartSctpClientTimer(uint32_t TimeOut, void *data)
{
	agc_event_t *new_event = NULL;
	if (agc_event_create_callback(&new_event, EVENT_NULL_SOURCEID, data, &SctpLayerDrv::OnSctpClientTimer) != AGC_STATUS_SUCCESS) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::StartSctpClientTimer create timer agc_event_create [fail].\n");
		return;
	} 
	agc_timer_add_timer(new_event, TimeOut);
	//agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "SctpLayerDrv::StartSctpClientTimer add new timer \n");
}


void SctpLayerDrv::handle_sctp_timeout()
{
    map<uint32_t, Cfg_LteSctp*>::iterator it = clients_links.begin();
  	for(; it != clients_links.end(); it++)
	{
		agc_sctp_config_t cfg;
		uint32_t sctp_index = it->first;

		if (sctp_index >= MAX_SCTP_CONTEXTS) {
		    return;
		}
	    cfg.outbound_stream_num = it->second->SctpNum;
	    cfg.hbinvertal = it->second->HeartBeatTime_s;
	    cfg.path_max_retrans = 10;
	    cfg.init_max_retrans = 10;
	    cfg.rto_initial = 3000;
	    cfg.rto_min = 1000;
	    cfg.rto_max = 5000;

        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sctp timeout add client begin: %d.\n", sctp_index);
		bool bRet = GetSctpLayerDrv().AddSctpClient(&cfg, sctp_index, 
			it->second,
			client_read_handler,
			NULL, 
			client_err_handler);
		if (bRet) {
            agc_mutex_lock(GetSctpLayerDrv().m_mutex);
            clients_links.erase(it);
            agc_mutex_unlock(GetSctpLayerDrv().m_mutex);
		}
/*
	    if (links[i]->LocalIP1Len){		
			bool bRet = GetSctpLayerDrv().AddSctpClient(&cfg, (agc_std_sockaddr_t *)&links[i]->LocalIP1, links[i]->LocalIP1Len, 
				(agc_std_sockaddr_t *)&links[i]->PeerIP1, links[i]->PeerIP1Len, 
				client_read_handler, NULL, sctp_index, links[i]->idSigGW, links[i]->SctpNum,
				client_err_handler);
			
			if (bRet) {
				act_links[sctp_index] = sctp_index;
			}
	    }
		
	    if (links[i]->LocalIP2Len){		
			bool bRet = GetSctpLayerDrv().AddSctpClient(&cfg, (agc_std_sockaddr_t *)&links[i]->LocalIP2, links[i]->LocalIP2Len, 
				(agc_std_sockaddr_t *)&links[i]->PeerIP2, links[i]->PeerIP2Len, 
				client_read_handler, NULL, sctp_index, links[i]->idSigGW, links[i]->SctpNum,
				client_err_handler);
			if (bRet) {
				act_links[sctp_index] = sctp_index;
			}
	    }*/
	}

}

void SctpLayerDrv::StartSctpTimer(uint32_t TimeOut)
{
    agc_event_t *new_event = NULL;
    if (agc_event_create_callback(&new_event, EVENT_NULL_SOURCEID, NULL, &SctpLayerDrv::OnSctpTimer) != AGC_STATUS_SUCCESS) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::StartSctpTime create timer agc_event_create [fail].\n");
        return;
    }
    agc_timer_add_timer(new_event, TimeOut);
    agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "SctpLayerDrv::StartSctpTime add new timer \n");
}

void SctpLayerDrv::OnSctpTimer(void *data)
{
	GetSctpLayerDrv().handle_sctp_timeout();

	// restart sctp timer
	GetSctpLayerDrv().StartSctpTimer(SCTPLAYER_SCTP_TIMER_VALUE);
}

void SctpLayerDrv::OnSctpAlarmTimer(void *data)
{
    sctp_context_t* context = (sctp_context_t*)data;
    sctp_alarm.report_general_alarm(STATUS_NORMAL, to_string(context->sctp_index), "");
    context->alarmStatus = STATUS_NORMAL;
    context->alarm_event = NULL;
}

void SctpLayerDrv::StartAlarmTimer(void *data)
{
    agc_event_t *new_event = NULL;
    if (agc_event_create_callback(&new_event, EVENT_NULL_SOURCEID, data, &OnSctpAlarmTimer) != AGC_STATUS_SUCCESS) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SctpLayerDrv::StartAlarmTime create timer agc_event_create [fail].\n");
        return;
    }
    agc_timer_add_timer(new_event, SCTPLAYER_ALARM_TIMER_VALUE);
    sctp_context_t* context = (sctp_context_t*)data;
    context->alarm_event = new_event;
    agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "start sctp alarm timer \n");
}

agc_status_t SctpLayerDrv::CfgAddSctp(Cfg_LteSctp &CfgSctp)
{
    int sctpIndex = CfgSctp.idSctpIndex;
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "config add sctp: %d.\n", sctpIndex);

    if (sctpIndex >= MAX_SCTP_CONTEXTS)
    {
        return AGC_STATUS_FALSE;
    }

    Cfg_LteSctp *new_link = new(Cfg_LteSctp);
    memcpy(new_link, &CfgSctp, sizeof(Cfg_LteSctp));

    sctp_context_t *context = &m_sctpContexts[sctpIndex];
    if (context->sctpStatus) {
        CfgRmvSctp(CfgSctp);
    }

    if (CfgSctp.WorkType == SCTP_WT_SERVER) {
        agc_sctp_config_t cfg;
        cfg.outbound_stream_num = CfgSctp.SctpNum;
        cfg.hbinvertal = CfgSctp.HeartBeatTime_s;
        cfg.path_max_retrans = 3;
        cfg.init_max_retrans = 3;
        cfg.rto_initial = 300;
        cfg.rto_min = 200;
        cfg.rto_max = 500;

        if (CfgSctp.LocalIP1Len) {
            int i = FindServerAddr(CfgSctp.LocalIP1, CfgSctp.LocalIP1Len);
            if (i < 0) {
                bool bRet = AddSctpServer(&cfg, (agc_std_sockaddr_t *) &CfgSctp.LocalIP1,
                                                            CfgSctp.LocalIP1Len, server_read_handler, server_err_handler);
                if (bRet) {
                    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "sctp server conn1 listen success: %d.\n", sctpIndex);
                } else {
                    return AGC_STATUS_FALSE;
                }
            } else {
                serverContextList[i]->addr_reference++;
                agc_log_printf(AGC_LOG, AGC_LOG_INFO, "sctp server %d add ref: %d.\n", sctpIndex, serverContextList[i]->addr_reference);
            }
        }

        if (CfgSctp.LocalIP2Len) {
            int i = FindServerAddr(CfgSctp.LocalIP2, CfgSctp.LocalIP2Len);
            if (i < 0) {
                bool bRet = AddSctpServer(&cfg, (agc_std_sockaddr_t *) &CfgSctp.LocalIP2,
                                                            CfgSctp.LocalIP2Len, server_read_handler, server_err_handler);
                if (bRet) {
                    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "sctp server conn2 listen success: %d.\n", sctpIndex);
                } else {
                    return AGC_STATUS_FALSE;
                }
            } else {
                serverContextList[i]->addr_reference++;
                agc_log_printf(AGC_LOG, AGC_LOG_INFO, "sctp server %d add ref2: %d.\n", sctpIndex, serverContextList[i]->addr_reference);
            }
        }
    } else {
        agc_sctp_config_t cfg;
        cfg.outbound_stream_num = CfgSctp.SctpNum;
        cfg.hbinvertal = CfgSctp.HeartBeatTime_s;
        cfg.path_max_retrans = 10;
        cfg.init_max_retrans = 10;
        cfg.rto_initial = 3000;
        cfg.rto_min = 1000;
        cfg.rto_max = 5000;

        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "config add client begin: %d.\n", sctpIndex);
        bool bRet = AddSctpClient(&cfg, sctpIndex, new_link, client_read_handler, NULL, client_err_handler);
        if (bRet) {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "config add client success: %d.\n", sctpIndex);
        } else {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "config add client to list: %d.\n", sctpIndex);
            agc_mutex_lock(GetSctpLayerDrv().m_mutex);
            clients_links[sctpIndex] = new_link;
            agc_mutex_unlock(GetSctpLayerDrv().m_mutex);
        }
    }

    links.emplace_back(new_link);
    return AGC_STATUS_SUCCESS;
}

void SctpLayerDrv::ReleaseServerContext(agc_std_sockaddr_t addr, socklen_t addr_len)
{
    int index = FindServerAddr(addr, addr_len);
    if (index >=0) {
        sctp_context_t *context = serverContextList[index];
        context->addr_reference -= 1;
        int reference = context->addr_reference;
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "sctp release server context: %d, ref: %d.\n", index, reference);

        if (reference <= 0) {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "sctp release and clear server context : %d.\n", index);

            if (context->connection != NULL) {
                agc_driver_del_event(context->connection, AGC_WRITE_EVENT);
                agc_driver_del_connection(context->connection);
                if (context->connection->pool != NULL) {
                    agc_memory_destroy_pool(&context->connection->pool);
                }
            }
            if (context->sock) {
                agc_sctp_close(context->sock);
            }
            if (context->mutex != NULL) {
                agc_mutex_destroy(context->mutex);
            }
            delete(context);
            vector<sctp_context_t *>::iterator it = serverContextList.begin()+index;
            serverContextList.erase(it);
        }
    }
}

void SctpLayerDrv::ReleaseClientContext(int sctpIndex)
{
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "sctp release client context: %d.\n", sctpIndex);

    if (sctpIndex >= MAX_SCTP_CONTEXTS) {
        return;
    }

    sctp_context_t *context = &m_sctpContexts[sctpIndex];
    if (context->sctpStatus == SCTP_NOT_CONFIG) {
        return;
    }

    if (context->connection != NULL) {
        agc_driver_del_event(context->connection, AGC_WRITE_EVENT);
        agc_driver_del_connection(context->connection);
        if (context->connection->pool != NULL) {
            agc_memory_destroy_pool(&context->connection->pool);
        }
    }
    if (context->connection2 != NULL) {
        agc_driver_del_event(context->connection2, AGC_WRITE_EVENT);
        agc_driver_del_connection(context->connection2);
        if (context->connection2->pool != NULL) {
            agc_memory_destroy_pool(&context->connection->pool);
        }
    }


    if (context->sock) {
        agc_sctp_close(context->sock);
    }
    if (context->sock2) {
        agc_sctp_close(context->sock2);
    }
    if (context->mutex != NULL) {
        agc_mutex_destroy(context->mutex);
    }

    SctpContextInit(context);
}

agc_status_t SctpLayerDrv::CfgRmvSctp(Cfg_LteSctp &CfgSctp) {
    int sctpIndex = CfgSctp.idSctpIndex;
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "config remove sctp: %d.\n", sctpIndex);

    if (sctpIndex >= MAX_SCTP_CONTEXTS)
    {
        return AGC_STATUS_FALSE;
    }

    sctp_context_t *context = &m_sctpContexts[sctpIndex];
    if (CfgSctp.WorkType == SCTP_WT_CLIENT) {
        if (context->sctpStatus && context->client) {
            ReleaseClientContext(sctpIndex);
            agc_mutex_lock(GetSctpLayerDrv().m_mutex);
            GetSctpLayerDrv().clients_links.erase(sctpIndex);
            agc_mutex_unlock(GetSctpLayerDrv().m_mutex);
        }
    } else {
        ReleaseServerContext(CfgSctp.LocalIP1, CfgSctp.LocalIP1Len);
        if (CfgSctp.LocalIP2Len) {
            ReleaseServerContext(CfgSctp.LocalIP2, CfgSctp.LocalIP2Len);
        }
        if (context->sctpStatus && !context->client) {
            ReleaseClientContext(sctpIndex);
        }
    }

    vector<Cfg_LteSctp *>::iterator it = links.begin();
    for (; it != links.end(); it++) {
        if ((*it)->idSctpIndex == sctpIndex) {
            delete (*it);
            links.erase(it);
            break;
        }
    }

    return AGC_STATUS_SUCCESS;
}

agc_status_t SctpLayerDrv::CfgDisplaySctp(int sctpIndex, sctp_status_e& sctpStatus)
{
    if (sctpIndex >= MAX_SCTP_CONTEXTS)
    {
        return AGC_STATUS_FALSE;
    }

    sctpStatus = m_sctpContexts[sctpIndex].sctpStatus;

    return AGC_STATUS_SUCCESS;
}

void SctpLayerDrv::SctpContextInit(sctp_context_t *context)
{
    memset(context, 0, sizeof(sctp_context_t));
}

// returns < 0 if (left < right)
// returns > 0 if (left > right)
// returns 0 if (left == right)
// Note that in general, "less" and "greater" are not particularly
// meaningful, but this does provide a strict weak ordering since
// you probably need one.
int SctpLayerDrv::SocketCmp(agc_std_sockaddr_t left, agc_std_sockaddr_t right)
{
    if (left.ss_family == AF_INET && right.ss_family == AF_INET) {
        struct sockaddr_in *addr1 = (struct sockaddr_in *)&left;
        struct sockaddr_in *addr2 = (struct sockaddr_in *)&right;
        if(addr1->sin_addr.s_addr == addr2->sin_addr.s_addr && addr1->sin_port == addr2->sin_port) {
            return 0;
        } else {
            return -1;
        }
    } else if (left.ss_family == AF_INET6 && right.ss_family == AF_INET6) {
        struct sockaddr_in6 *addr1 = (struct sockaddr_in6 *) &left;
        struct sockaddr_in6 *addr2 = (struct sockaddr_in6 *) &right;
        if (0 == memcmp(&addr1->sin6_addr, &addr2->sin6_addr, sizeof(addr1->sin6_addr))
            && addr1->sin6_port == addr2->sin6_port) {
            return 0;
        } else {
            return -1;
        }
    } else {
        return -1;
    }
}

int SctpLayerDrv::FindServerAddr(agc_std_sockaddr_t addr, socklen_t addr_len)
{
    agc_std_sockaddr_t local_addr;
    socklen_t local_addr_len;

    for (int i = 0; i < serverContextList.size(); i++)
    {
        local_addr  = serverContextList[i]->local_addr;
        local_addr_len = serverContextList[i]->local_addrlen;
        if (0 == SocketCmp(local_addr, addr))
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "find address.\n");
            return i;
        }
    }

    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "can't find address.\n");
    return -1;
}

agc_status_t SctpLayerDrv::CfgGetFreeIndex(uint32_t &index)
{
    for (int i = 0; i < MAX_SCTP_CONTEXTS; i++) {
        if (!m_sctpContexts[i].sctpStatus) {
            index = i;
            return AGC_STATUS_SUCCESS;
        }
    }
    return AGC_STATUS_FALSE;
}