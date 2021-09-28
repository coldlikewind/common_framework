
#include <yaml.h>
#include "media_sig_path.h"
#include "media_context.h"
#include "media_gtp_path.h"
#include <time.h>

#define MEDIA_SESS_NEXT_ID(__id, __min, __max) \
    ((__id) = ((__id) == (__max) ? (__min) : ((__id) + 1)))

#define MEDIAGW_CONFIG_FILE "mediagw.yml"

media_context_t g_media_context;

static agc_status_t load_configuration()
{
    char *filename;
    char pos[255] = "\0";
    FILE *file;
    yaml_parser_t parser;
    yaml_token_t token;
    int done = 0;
    int error = 0;
    int iskey = 0;
    int type = 0;

    enum {
        BLOCK_N3,
        BLOCK_N3_UPF,
        BLOCK_UNKOWN
    } blocktype = BLOCK_UNKOWN;

    enum {
        KEYTYPE_ADDRTYPE,
        KEYTYPE_ADDR,
        KEYTYPE_UNKOWN
    } keytype = KEYTYPE_UNKOWN;
    
    const char *err;

    filename = agc_mprintf("%s%s%s", AGC_GLOBAL_dirs.conf_dir, 
        AGC_PATH_SEPARATOR, MEDIAGW_CONFIG_FILE);
    
    file = fopen(filename, "rb");
    assert(file);
    assert(yaml_parser_initialize(&parser));
    
    yaml_parser_set_input_file(&parser, file);
    
    while (!done)
    {
        if (!yaml_parser_scan(&parser, &token)) {
            error = 1;
            break;
        }
        
        switch(token.type)
        {
            case YAML_KEY_TOKEN:
                iskey = 1;
                break;
            case YAML_VALUE_TOKEN:
                iskey = 0;
                break;
            case YAML_SCALAR_TOKEN:
                {
                    if (iskey)
                    {
                        if (strcmp(token.data.scalar.value, "addrtype") == 0)
                        {
                            keytype = KEYTYPE_ADDRTYPE;
                        }
                        else if (strcmp(token.data.scalar.value, "addr") == 0)
                        {
                            keytype = KEYTYPE_ADDR;
                        }
                        else if (strcmp(token.data.scalar.value, "ProxyAddress") == 0)
                        {
                            blocktype = BLOCK_N3;
                        }
                        else if (strcmp(token.data.scalar.value, "B2BAddress") == 0)
                        {
                            blocktype = BLOCK_N3_UPF;
                        }
                        else
                        {
                            keytype = KEYTYPE_UNKOWN;
                        }
                    } else {
                        if (keytype == KEYTYPE_ADDR) {
                            strcpy(pos, token.data.scalar.value);
                        }
                        else if (keytype == KEYTYPE_ADDRTYPE)
                        {
                            type = agc_atoui(token.data.scalar.value);
                        } else {
                        }  

                        if (strlen(pos) > 0)
                        {
                            agc_std_sockaddr_t servaddr;
                            socklen_t addrlen = 0;

                            if (type == MEDIA_CFG_GTP_IPV4) {
                                struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&servaddr;
                                local_addr1_v4->sin_family = AF_INET;
                                local_addr1_v4->sin_port  = htons(MEDIA_GTPV1_U_UDP_PORT);
                                inet_pton(AF_INET, pos, &(local_addr1_v4->sin_addr));
                                addrlen = sizeof(struct sockaddr_in);
                            }
                            else if (type == MEDIA_CFG_GTP_IPV6) {
                                struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&servaddr;
                                local_addr1_v6->sin6_family = AF_INET6;
                                local_addr1_v6->sin6_port  = htons(MEDIA_GTPV1_U_UDP_PORT);
                                inet_pton(AF_INET6, pos, &(local_addr1_v6->sin6_addr));
                                addrlen = sizeof(struct sockaddr_in6);
                            }
                            else {
                                agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_handle_loading_gtp_addr failed type=%d.\n", type);

                            }

                            if (blocktype == BLOCK_N3)
                            {      
                                media_add_gtp_node(&servaddr, addrlen, type, MEDIA_INTERFACE_N3); 
                                agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "media load n3 address type=%d addr=%s.\n", type, pos);
                            }
                            else if (blocktype == BLOCK_N3_UPF)
                            {
                                media_add_gtp_node(&servaddr, addrlen, type, MEDIA_INTERFACE_N3_UPF); 
                                agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "media load n3 upf address type=%d addr=%s.\n", type, pos);

                            }

                            pos[0] = '\0';                            
                        }
                    }
                }
                break;
            default:
                break;
        }
        
        done = (token.type == YAML_STREAM_END_TOKEN);
        yaml_token_delete(&token);
    }

    yaml_parser_delete(&parser);
    assert(!fclose(file));  

    agc_safe_free(filename);

    if (*(pos -1) == ',') {
        *(pos -1) = '\0';
    }

    return AGC_STATUS_SUCCESS;
}

agc_status_t media_context_init() {
    time_t t;
	agc_memory_pool_t *pool = (agc_memory_pool_t *)media_get_memory_pool();

	agc_list_init(&g_media_context.hash_key_list);
	agc_list_init(&g_media_context.server_list);
    agc_list_init(&g_media_context.lbo_sess_list);
	agc_list_init(&g_media_context.server_list_v2);

    srand((unsigned) time(&t));
	g_media_context.next_sess_id = rand() % MAX_MEDIA_SESSIONS;
	g_media_context.gtpu_port = MEDIA_GTPV1_U_UDP_PORT;
	g_media_context.sess_id_table = agc_hash_make(pool);
    g_media_context.ue_ip_table = agc_hash_make(pool);
    g_media_context.lbo_ip_table = agc_hash_make(pool);
    g_media_context.last_check_sess_node = NULL;

    if (load_configuration() !=  AGC_STATUS_SUCCESS) {
        media_context_shutdown();
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media load configuration failed.\n");
        return AGC_STATUS_GENERR;
    }

	memset(g_media_context.session_list, 0 , MAX_MEDIA_SESSIONS * sizeof(media_sig_sess_t *));
    media_context_start_check_timer();
    
	return AGC_STATUS_SUCCESS;
}

agc_status_t media_context_shutdown() {

    agc_hash_clear(g_media_context.lbo_ip_table);
    agc_hash_clear(g_media_context.ue_ip_table);
    agc_hash_clear(g_media_context.sess_id_table);

	media_remove_all_media_sig_sess();
	
	media_remove_all_gtp_nodes();
	media_remove_all_gtp_nodes_v2();
	return AGC_STATUS_SUCCESS;
}

media_context_t* media_get_media_context() {
	return &g_media_context;
}

agc_status_t media_add_gtp_node(agc_std_sockaddr_t *addr, socklen_t addrlen, media_gtp_ip_t ip_type, media_interface_t it) {

	media_gtp_node_t *node = NULL;
	agc_malloc(node, sizeof(media_gtp_node_t));

	node->sock = NULL;
	node->addrlen = addrlen;
    node->interface_type = it;
    node->ip_type = ip_type;
	memcpy(&node->addr, addr, addrlen);

	agc_list_append(&g_media_context.server_list, node);

	return AGC_STATUS_SUCCESS;
}

agc_status_t media_remove_gtp_node(media_gtp_node_t *node)
{
	if (node == NULL) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_remove_node node=%x.\n", node);
        return AGC_STATUS_FALSE;
	}

    agc_list_remove(&g_media_context.server_list, node);

    if (node->sock) {
    	agc_gtp_close(node->sock);
    }

    agc_safe_free(node);

    return AGC_STATUS_SUCCESS;
}

agc_status_t media_remove_all_gtp_nodes()
{
    media_gtp_node_t *node = NULL, *next_node = NULL;

    node = agc_list_first(&g_media_context.server_list);
    while(node)
    {
        next_node = agc_list_next(node);
        media_remove_gtp_node(node);
        node = next_node;
    }

    return AGC_STATUS_SUCCESS;
}

agc_status_t media_add_gtp_node_v2(agc_gtp_sock_t *sock) {
	media_gtp_node_v2_t *node = NULL;

	if (sock == NULL) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_del_gtp_node_ref_v2 node=%x.\n", node);
        return AGC_STATUS_FALSE;
	}
	
	agc_malloc(node, sizeof(media_gtp_node_v2_t));

	node->sock = sock;
	node->ref = 1;

	agc_list_append(&g_media_context.server_list_v2, node);

	return AGC_STATUS_SUCCESS;
}


agc_status_t media_add_gtp_node_ref_v2(media_gtp_node_v2_t *node) {
	if (node == NULL) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_add_gtp_node_ref_v2 node=%x.\n", node);
        return AGC_STATUS_FALSE;
	}
	
	node->ref++;
	return AGC_STATUS_SUCCESS;
}

agc_status_t media_del_gtp_node_ref_v2(agc_gtp_sock_t *sock) {

    media_gtp_node_v2_t *node = NULL;
	if (sock == NULL) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_del_gtp_node_ref_v2 node=%x.\n", node);
        return AGC_STATUS_FALSE;
	}

    node = agc_list_first(&g_media_context.server_list_v2);
    while(node)
    {
    	if (node->sock == sock)
		{
			node->ref--;

			if (node->ref <= 0) {
				media_remove_gtp_node_v2(node);
			}

			break;
		}
		
        node  = agc_list_next(node);
    }

	return AGC_STATUS_SUCCESS;
}

agc_status_t media_remove_gtp_node_v2(media_gtp_node_v2_t *node)
{
	if (node == NULL) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_remove_gtp_node_v2 node=%x.\n", node);
        return AGC_STATUS_FALSE;
	}

    agc_list_remove(&g_media_context.server_list_v2, node);

    if (node->sock) {
    	agc_gtp_close(node->sock);
    }

    agc_safe_free(node);

    return AGC_STATUS_SUCCESS;
}

agc_status_t media_remove_all_gtp_nodes_v2()
{
    media_gtp_node_v2_t *node = NULL, *next_node = NULL;

    node = agc_list_first(&g_media_context.server_list_v2);
    while(node)
    {
        next_node = agc_list_next(node);
        media_remove_gtp_node_v2(node);
        node = next_node;
    }

    return AGC_STATUS_SUCCESS;
}
media_gtp_node_t *media_gtp_node_first()
{
    return agc_list_first(&g_media_context.server_list);
}


void media_on_release_timer(void *data)
{
	media_sig_sess_t *sess = (media_sig_sess_t *)data;
	
	agc_safe_free(data);
}

agc_status_t media_remove_media_sig_sess(media_sig_sess_t *sess) {
	uint32_t sess_id = 0;
	agc_event_t *new_event = NULL;
	if (sess == NULL) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_free_media_sig_sess sess is invalid.\n");
        return AGC_STATUS_FALSE;
	}

	sess_id = sess->sess_id;
	if (sess_id >= MAX_MEDIA_SESSIONS) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_free_media_sig_sess sess_id=%d.\n", sess->sess_id);
        return AGC_STATUS_FALSE;
	}	

	// close sock
	media_upf_gtp_close(sess);
	media_gnb_gtp_close(sess);

    // remove the hash key
	media_module_lock();
	g_media_context.session_list[sess_id] = 0;
	//agc_list_remove(&g_media_context.sess_list, sess);
	media_module_unlock();

	// start timer to delay free sess	
	if (agc_event_create_callback(&new_event, EVENT_NULL_SOURCEID, sess, &media_on_release_timer) != AGC_STATUS_SUCCESS) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_free_media_sig_sess create timer agc_event_create [fail].\n");
		return AGC_STATUS_FALSE;
	} 
	agc_timer_add_timer(new_event, MEDIA_DELAY_RELEASE_TIMER);

    return AGC_STATUS_SUCCESS;
}

agc_status_t media_remove_all_media_sig_sess() {
	int index = 0;
	media_sig_sess_t *sess = NULL;
	for (index = 0; index < MAX_MEDIA_SESSIONS; index++)
	{		
		sess = g_media_context.session_list[index];
		if (sess != NULL)
		{
			agc_safe_free(sess);
		}
	}

    return AGC_STATUS_SUCCESS;
}

agc_status_t media_add_lbo_node(agc_std_sockaddr_t *addr, socklen_t addrlen)
{
	uint8_t *key = NULL;
	uint32_t key_len = 0;
	media_hash_key_table_t *key_node = NULL;

    key_node = malloc(sizeof(media_hash_key_table_t));
    if (key_node == NULL) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_add_lbo_node fail to alloc key_node\n");
        return AGC_STATUS_FALSE;
    }
    
    if (addr->ss_family == AF_INET) {
        struct sockaddr_in *listen_addr4 = (struct sockaddr_in *)addr;
        key = (uint8_t *)&(listen_addr4->sin_addr);
        key_len = MEDIA_IPADDRV4_KEY_LEN;
    } else {
        struct sockaddr_in6 *listen_addr6 = (struct sockaddr_in6 *)addr;
        key = (uint8_t *)&(listen_addr6->sin6_addr);
        key_len = MEDIA_IPADDRV6_KEY_LEN;
    }

    key_node->key = malloc(key_len);
    if (key_node->key == NULL) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_add_lbo_node fail to alloc key key_len=%d\n", key_len);
        return AGC_STATUS_FALSE;
    }

    memcpy(key_node->key, key, key_len);
    key_node->key_len = key_len;
    agc_list_append(&g_media_context.hash_key_list, key_node);

    agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "media_add_lbo_node key_len=%d  %x %x %x %x.\n",
        key_node->key_len, 
        key_node->key[0], key_node->key[1], key_node->key[2], key_node->key[3]);
        
    agc_hash_set(g_media_context.lbo_ip_table, key_node->key, key_node->key_len, key_node);

    return AGC_STATUS_FALSE;
}


media_hash_key_table_t * media_find_lbo_node_by_key(uint8_t *key, uint32_t key_len)
{
    return (media_hash_key_table_t *)agc_hash_get(g_media_context.lbo_ip_table, key, key_len);
}

agc_status_t media_remove_all_hash_key()
{
    media_hash_key_table_t *node = NULL, *next_node = NULL;

    node = agc_list_first(&g_media_context.hash_key_list);
    while(node)
    {
        next_node = agc_list_next(node);
		if (node->key)
			agc_safe_free(node->key);
    	agc_safe_free(node);		
        node = next_node;
    }

    return AGC_STATUS_SUCCESS;
}

media_gtp_node_t *media_gtp_node_n3_v4_first()
{
    media_gtp_node_t *gnode = NULL;
    for (gnode = agc_list_first(&g_media_context.server_list); gnode; gnode = agc_list_next(gnode))
    {
        if (gnode->interface_type == MEDIA_INTERFACE_N3)
        {
            if (gnode->ip_type == MEDIA_CFG_GTP_IPV4)
            {
                return gnode;
            }
        }
    }

    return NULL;
}

media_gtp_node_t *media_gtp_node_n3_v6_first()
{
    media_gtp_node_t *gnode = NULL;
    for (gnode = agc_list_first(&g_media_context.server_list); gnode; gnode = agc_list_next(gnode))
    {
        if (gnode->interface_type == MEDIA_INTERFACE_N3)
        {
            if (gnode->ip_type == MEDIA_CFG_GTP_IPV6)
            {
                return gnode;
            }
        }
    }

    return NULL;
}

media_gtp_node_t *media_gtp_node_n3_upf_v4_first()
{
    media_gtp_node_t *gnode = NULL;
    for (gnode = agc_list_first(&g_media_context.server_list); gnode; gnode = agc_list_next(gnode))
    {
        if (gnode->interface_type == MEDIA_INTERFACE_N3_UPF)
        {
            if (gnode->ip_type == MEDIA_CFG_GTP_IPV4)
            {
                return gnode;
            }
        }
    }

    return NULL;
}

media_gtp_node_t *media_gtp_node_n3_upf_v6_first()
{
    media_gtp_node_t *gnode = NULL;
    for (gnode = agc_list_first(&g_media_context.server_list); gnode; gnode = agc_list_next(gnode))
    {
        if (gnode->interface_type == MEDIA_INTERFACE_N3_UPF)
        {
            if (gnode->ip_type == MEDIA_CFG_GTP_IPV6)
            {
                return gnode;
            }
        }
    }

    return NULL;
}

media_sig_sess_t * media_add_media_sig_sess() {
	uint32_t count = 0;
    uint32_t	      sess_id = 0; 	
    media_sig_sess_t *sess = NULL;
    media_gtp_node_t *gnode_v4 = media_gtp_node_n3_v4_first();
    media_gtp_node_t *gnode_v6 = media_gtp_node_n3_v6_first();
    media_gtp_node_t *gnode_to_upf_v4 = media_gtp_node_n3_upf_v4_first();
    media_gtp_node_t *gnode_to_upf_v6 = media_gtp_node_n3_upf_v6_first();

    if (gnode_v4 == NULL && gnode_v6 == NULL) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_add_media_sig_sess gnode is invalid.\n");
        return NULL;
    }

    if (gnode_to_upf_v4 == NULL)
        gnode_to_upf_v4 = gnode_v4;

    if (gnode_to_upf_v6 == NULL)
        gnode_to_upf_v6 = gnode_v6;

	do
	{
		media_module_lock();
		sess_id = MEDIA_SESS_NEXT_ID(g_media_context.next_sess_id, 1, MAX_MEDIA_SESSIONS - 1);
		
		if (g_media_context.session_list[sess_id] == 0)
		{			
			media_module_unlock();
			break;
		}
		
		media_module_unlock();
		
		count++;
		if (count >= MAX_MEDIA_SESSIONS)
		{
	        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_add_media_sig_sess session is full！！！！\n");
			return NULL;
		}
	} while (1);
	
	agc_malloc(sess, sizeof(media_sig_sess_t));

	media_module_lock();
	g_media_context.session_list[sess_id] = sess;
	media_module_unlock();
	
	sess->sess_id = sess_id;
    sess->last_act_time = agc_time_now();
    sess->gnbv6.local_addrlen   = 0;
    sess->gnbv6.remote_addrlen  = 0;
    sess->upfv6.local_addrlen   = 0;
    sess->upfv6.remote_addrlen  = 0;
	sess->qos_flow_id			= 0;

    memset(sess->ue_addr, 0, sizeof(uint32_t) * 4);

    sess->gnb.local_teid        = sess->sess_id * MEDIA_FOUR_TIMES;
    sess->gnb.remote_teid       = 0;
    sess->upf.remote_teid       = 0;
    sess->gnbv6.remote_teid     = 0;
    sess->upfv6.remote_teid     = 0;

    sess->upf.local_teid        = sess->sess_id * MEDIA_FOUR_TIMES + 1;
    sess->gnbv6.local_teid      = sess->sess_id * MEDIA_FOUR_TIMES + 2;
    sess->upfv6.local_teid      = sess->sess_id * MEDIA_FOUR_TIMES + 3;

	sess->gnb.local_addrlen		= gnode_v4->addrlen;
	sess->gnb.remote_addrlen	= 0;
	sess->gnb.sock				= 0;//gnode_v4->sock;
    memcpy(&sess->gnb.local_addr, &gnode_v4->addr, gnode_v4->addrlen);

	sess->upf.local_addrlen		= gnode_to_upf_v4->addrlen;
	sess->upf.remote_addrlen	= 0;
	sess->upf.sock				= 0;//gnode_to_upf_v4->sock;
    memcpy(&sess->upf.local_addr, &gnode_to_upf_v4->addr, gnode_to_upf_v4->addrlen);

    if (gnode_v6 != NULL)
    {
        sess->gnbv6.local_addrlen     = gnode_v6->addrlen;
        sess->gnbv6.remote_addrlen    = 0;
        sess->gnbv6.sock              = 0;//gnode_v6->sock;
        memcpy(&sess->gnbv6.local_addr, &gnode_v6->addr, gnode_v6->addrlen);
    }

    if (gnode_to_upf_v6 != NULL)
    {
        sess->upfv6.local_addrlen     = gnode_to_upf_v6->addrlen;
        sess->upfv6.remote_addrlen    = 0;
        sess->upfv6.sock              = 0;//gnode_to_upf_v6->sock;
        memcpy(&sess->upfv6.local_addr, &gnode_to_upf_v6->addr, gnode_to_upf_v6->addrlen);
    }

    agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "media_add_media_sig_sess sessid=%d gnb_local_teid=%d upf_local_teid=%d gnb_local_teid_v6=%d upf_local_teid_v6=%d.\n", 
        sess->sess_id, sess->gnb.local_teid, sess->upf.local_teid,
        sess->gnbv6.local_teid, sess->upfv6.local_teid);
	return sess;
}

media_sig_sess_t * media_find_media_sig_sess_by_teid(uint32_t teid) {
	media_sig_sess_t *sess = NULL;
    uint32_t sess_id = teid / MEDIA_FOUR_TIMES;
	
	if (sess_id >= MAX_MEDIA_SESSIONS)
		return NULL;
	
	media_module_unlock();
	sess = g_media_context.session_list[sess_id] ;
	media_module_unlock();
	return sess;
}

media_sig_sess_t * media_find_media_sig_sess_by_id(uint32_t sess_id) {
	media_sig_sess_t *sess = NULL;
	if (sess_id >= MAX_MEDIA_SESSIONS)
		return NULL;
	
	media_module_unlock();
	sess = g_media_context.session_list[sess_id] ;
	media_module_unlock();
	return sess;
}

void media_check_all_media_sig_sess() {
#if 0
    media_sig_sess_t *node = NULL, *next_node = NULL;
    int count = 0;
    agc_time_t act_time = agc_time_now();
    if (g_media_context.last_check_sess_node != NULL) {
        node = g_media_context.last_check_sess_node;
    }
    else {
        node = agc_list_first(&g_media_context.sess_list);     
    }

    while(node)
    {
        next_node = agc_list_next(node);

        agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "media_check_all_media_sig_sess act_time %lld %lld.\n", act_time, node->last_act_time);
        // inactive sess, release this sess
        if ((act_time - node->last_act_time) > MEDIA_MAX_SESS_TIMEOUT) {
            media_sig_path_send_error_rsp(node, MSET_SESS_RELEASE_IND, MSEC_SESS_INACTIVE);
            media_remove_media_sig_sess(node);
        }
        node = next_node;

        count++;
        if (count >= MEDIA_MAX_CHECK_SESS) {
            g_media_context.last_check_sess_node = next_node;
            break;
        }
    }   
#endif
}

void media_context_on_check_timer() {
    //agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "media_context_on_check_timer start.\n");

    // don't need to check session
    //media_check_all_media_sig_sess();
    media_context_start_check_timer();
}

void media_context_start_check_timer() {
    agc_event_t *new_event = NULL;    
    if (agc_event_create_callback(&new_event, EVENT_NULL_SOURCEID, NULL, media_context_on_check_timer) != AGC_STATUS_SUCCESS) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_context_start_check_timer [fail].\n");
        return;
    } 

    agc_timer_add_timer(new_event, 20000);
}


agc_status_t media_load_ipsubnet(const char *ifname)
{
    agc_status_t rv;
    if (ifname == NULL)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_load_ipsubnet invalid param .\n");
        return AGC_STATUS_FALSE;
    }

    rv = agc_tun_open(&g_media_context.subnet.sock, ifname, 0);
    if (rv != AGC_STATUS_SUCCESS)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_load_ipsubnet agc_tun_open fail.\n");
        return AGC_STATUS_FALSE;
    }

    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "media_load_ipsubnet success sock=%d.\n", g_media_context.subnet.sock);
    strcpy(g_media_context.subnet.dev_if_name, ifname);

    return AGC_STATUS_SUCCESS;
}
