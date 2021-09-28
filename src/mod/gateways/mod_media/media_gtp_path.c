
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include "media_context.h"
#include "media_gtp_path.h"

#define MAX_GTP_MTU 1600

static char gtpbuf[MAX_GTP_BUFFER];

void media_gtp_send_n3_end_mark(media_sig_sess_t *sess)
{
    gtp_header_t *gtp_h = NULL;
    int32_t offset = 0;
    int32_t gtpbuf_len = 0;

    if (sess == NULL)
        return;

    agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "send N3 mark end to gnb begin");

    gtp_h = (gtp_header_t *)gtpbuf;
    /* Bits    8  7  6  5  4  3  2  1
     *        +--+--+--+--+--+--+--+--+
     *        |version |PT| 1| E| S|PN|
     *        +--+--+--+--+--+--+--+--+
     *         0  0  1   1  0  0  0  0
     */
    gtp_h->flags = GTPU_FLAGS_NULL_NEXT_EXTENSION;
    gtp_h->type = GTPU_MSGTYPE_END_MARKER;
    gtp_h->length = 0;

    gtpbuf_len = GTPV1U_HEADER_LEN;

    if (sess->gnb.remote_teid != 0)
    {
        gtp_h->teid = htonl(sess->gnb.remote_teid);
        agc_gtp_add_message(sess->gnb.sock, gtpbuf, gtpbuf_len, &sess->gnb.remote_addr, sess->gnb.remote_addrlen);
    }
    else if (sess->gnbv6.remote_teid != 0)
    {
        gtp_h->teid = htonl(sess->gnbv6.remote_teid);
        agc_gtp_add_message(sess->gnbv6.sock, gtpbuf, gtpbuf_len, &sess->gnbv6.remote_addr, sess->gnbv6.remote_addrlen);
    }

    agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "send N3 mark end to gnb end");
}

agc_status_t media_gtp_open() {
	media_gtp_node_t *gnode = NULL;
	media_context_t *mct = media_get_media_context();
	agc_status_t ret = AGC_STATUS_FALSE; 

	if (mct == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_gtp_open get context fail .\n");
		return AGC_STATUS_FALSE;
	}

    for (gnode = agc_list_first(&mct->server_list); gnode; gnode = agc_list_next(gnode))
    {
        ret = agc_gtp_open(&gnode->sock, &gnode->addr, gnode->addrlen, media_gtp_path_recv_cb); 

    	if (ret != AGC_STATUS_SUCCESS) {
			if (gnode->addr.ss_family == AF_INET) {
				struct sockaddr_in *listen_addr4 = (struct sockaddr_in *)&gnode->addr;
				char str_addr[64];
				inet_ntop(AF_INET, &(listen_addr4->sin_addr), str_addr, 64);
				agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "media_gtp_open failed addr_len=%d addr4 %s .\n", gnode->addrlen, str_addr);
			} else {
				struct sockaddr_in6 *listen_addr6 = (struct sockaddr_in6 *)&gnode->addr;
				char str_addr[64];
				inet_ntop(AF_INET6, &(listen_addr6->sin6_addr), str_addr, 64);
				agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "media_gtp_open failed addr_len=%d addr6 %s .\n", gnode->addrlen, str_addr);
			}
    	}
    }

	return AGC_STATUS_SUCCESS;
}

agc_status_t media_gtp_close() {
	media_gtp_node_t *gnode = NULL;
	media_context_t *mct = media_get_media_context();

	if (mct == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_gtp_open get context fail .\n");
		return AGC_STATUS_FALSE;
	}

    for (gnode = agc_list_first(&mct->server_list); gnode; gnode = agc_list_next(gnode))
    {
    	if (gnode->sock != NULL) {
    		agc_gtp_close(gnode->sock);
    	}

    	gnode->sock = NULL;
    }

	return AGC_STATUS_SUCCESS;
}

static media_gtp_node_v2_t *media_gtp_node_find(    
	agc_std_sockaddr_t *local_addr, socklen_t local_addrlen,
    agc_std_sockaddr_t *remote_addr, socklen_t remote_addrlen)
{
	media_gtp_node_v2_t *gnode = NULL;
	media_context_t *mct = media_get_media_context();
	char local_str_addr[64];
	char remote_str_addr[64];
	int local_port;
	int remote_port;

	if (mct == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_gtp_node_find get context fail .\n");
		return NULL;
	}
	
    for (gnode = agc_list_first(&mct->server_list_v2); gnode; gnode = agc_list_next(gnode))
    {
    	if (gnode->sock == NULL) {
			continue;
    	}

    	if (memcmp(&gnode->sock->local_addr, local_addr, local_addrlen) != 0)
			continue;
		
    	if (memcmp(&gnode->sock->remote_addr, remote_addr, remote_addrlen) != 0)
			continue;

		return gnode;
    }

	return NULL;
}

agc_status_t media_upf_gtp_open(media_sig_sess_t *sess) {
	agc_status_t 		ret = AGC_STATUS_FALSE; 	
	agc_gtp_sock_t  	*sock = NULL;
	agc_gtp_sock_t  	*sockv6 = NULL;
	media_gtp_node_v2_t *gnode = NULL;

	if (sess == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_upf_gtp_open sess is null .\n");
        return AGC_STATUS_FALSE;
	}

	sess->upf.sock = NULL;
	sess->upfv6.sock = NULL;

	gnode = media_gtp_node_find(&sess->upf.local_addr, sess->upf.local_addrlen,
    	&sess->upf.remote_addr, sess->upf.remote_addrlen);
	if (gnode != NULL) {
		media_add_gtp_node_ref_v2(gnode);
		sess->upf.sock = gnode->sock;
		return AGC_STATUS_SUCCESS;		
	}
	
    ret = agc_gtp_open_v2(&sock, &sess->upf.local_addr, sess->upf.local_addrlen,
    	&sess->upf.remote_addr, sess->upf.remote_addrlen, media_gtp_path_recv_cb); 

	if (ret != AGC_STATUS_SUCCESS) {
		if (sess->upf.local_addr.ss_family == AF_INET) {
			struct sockaddr_in *listen_addr4 = (struct sockaddr_in *)&sess->upf.local_addr;
			char str_addr[64];
			inet_ntop(AF_INET, &(listen_addr4->sin_addr), str_addr, 64);
			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "media_gtp_open failed addr_len=%d addr4 %s .\n", sess->upf.local_addrlen, str_addr);
		} else {
			struct sockaddr_in6 *listen_addr6 = (struct sockaddr_in6 *)&sess->upf.local_addr;
			char str_addr[64];
			inet_ntop(AF_INET6, &(listen_addr6->sin6_addr), str_addr, 64);
			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "media_gtp_open failed addr_len=%d addr6 %s .\n", sess->upf.local_addrlen, str_addr);
		}
		
		return AGC_STATUS_FALSE;
	}

	sess->upf.sock = sock;

	media_add_gtp_node_v2(sock);

	// process ipv6
	gnode = media_gtp_node_find(&sess->upfv6.local_addr, sess->upfv6.local_addrlen,
    	&sess->upfv6.remote_addr, sess->upfv6.remote_addrlen);
	if (gnode != NULL) {
		media_add_gtp_node_ref_v2(gnode);
		sess->upfv6.sock = gnode->sock;
		return AGC_STATUS_SUCCESS;
	}

    ret = agc_gtp_open_v2(&sockv6, &sess->upfv6.local_addr, sess->upfv6.local_addrlen,
    	&sess->upfv6.remote_addr, sess->upfv6.remote_addrlen, media_gtp_path_recv_cb); 

	if (ret == AGC_STATUS_SUCCESS) {
		media_add_gtp_node_v2(sockv6);
		sess->upfv6.sock = sockv6;
	}


	return AGC_STATUS_SUCCESS;
}

agc_status_t media_upf_gtp_close(media_sig_sess_t *sess) 
{
	if (sess == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_upf_gtp_close sess is null .\n");
        return AGC_STATUS_FALSE;
	}

	media_del_gtp_node_ref_v2(sess->upf.sock);

	if (sess->upfv6.sock)
		media_del_gtp_node_ref_v2(sess->upfv6.sock);
	
	return AGC_STATUS_SUCCESS;
}

agc_status_t media_gnb_gtp_open(media_sig_sess_t *sess) {
	agc_status_t 		ret = AGC_STATUS_FALSE; 
	
	agc_gtp_sock_t  	*sock = NULL;
	agc_gtp_sock_t  	*sockv6 = NULL;
	media_gtp_node_v2_t *gnode = NULL;

	if (sess == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_gnb_gtp_open sess is null .\n");
        return AGC_STATUS_FALSE;
	}
	
	sess->gnb.sock = NULL;
	sess->gnbv6.sock = NULL;

	gnode = media_gtp_node_find(&sess->gnb.local_addr, sess->gnb.local_addrlen,
    	&sess->gnb.remote_addr, sess->gnb.remote_addrlen);

	if (gnode != NULL) {
		media_add_gtp_node_ref_v2(gnode);
		sess->gnb.sock = gnode->sock;
		return AGC_STATUS_SUCCESS;		
	}

    ret = agc_gtp_open_v2(&sock, &sess->gnb.local_addr, sess->gnb.local_addrlen,
    	&sess->gnb.remote_addr, sess->gnb.remote_addrlen, media_gtp_path_recv_cb); 

	if (ret != AGC_STATUS_SUCCESS) {
		if (sess->upf.local_addr.ss_family == AF_INET) {
			struct sockaddr_in *listen_addr4 = (struct sockaddr_in *)&sess->upf.local_addr;
			char str_addr[64];
			inet_ntop(AF_INET, &(listen_addr4->sin_addr), str_addr, 64);
			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "media_gtp_open failed addr_len=%d addr4 %s .\n", sess->upf.local_addrlen, str_addr);
		} else {
			struct sockaddr_in6 *listen_addr6 = (struct sockaddr_in6 *)&sess->upf.local_addr;
			char str_addr[64];
			inet_ntop(AF_INET6, &(listen_addr6->sin6_addr), str_addr, 64);
			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "media_gtp_open failed addr_len=%d addr6 %s .\n", sess->upf.local_addrlen, str_addr);
		}
		return AGC_STATUS_FALSE;
	}

	sess->gnb.sock = sock;
	media_add_gtp_node_v2(sock);

	gnode = media_gtp_node_find(&sess->gnbv6.local_addr, sess->gnbv6.local_addrlen,
    	&sess->gnbv6.remote_addr, sess->gnbv6.remote_addrlen);

	if (gnode != NULL) {
		media_add_gtp_node_ref_v2(gnode);
		sess->gnbv6.sock = gnode->sock;
		return AGC_STATUS_SUCCESS;		
	}

    ret = agc_gtp_open_v2(&sockv6, &sess->gnbv6.local_addr, sess->gnbv6.local_addrlen,
    	&sess->gnbv6.remote_addr, sess->gnbv6.remote_addrlen, media_gtp_path_recv_cb); 

	if (ret == AGC_STATUS_SUCCESS) {
		media_add_gtp_node_v2(sockv6);
		sess->gnbv6.sock = sockv6;
	}

	return AGC_STATUS_SUCCESS;
}


agc_status_t media_gnb_gtp_close(media_sig_sess_t *sess) 
{
	if (sess == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_gnb_gtp_close sess is null .\n");
        return AGC_STATUS_FALSE;
	}

	media_del_gtp_node_ref_v2(sess->gnb.sock);

	if (sess->gnbv6.sock)
		media_del_gtp_node_ref_v2(sess->gnbv6.sock);
	
	return AGC_STATUS_SUCCESS;
}

agc_status_t media_gtp_packet_filter(media_sig_sess_t *sess, const char* buf, int32_t *len)
{
    gtp_header_t *gtp_header = NULL;
    media_hash_key_table_t    *lbo_key= NULL;
    struct ip *ip_h =  NULL;
    struct ip6_hdr *ip6_h =  NULL;
    const char* ip_buf = buf;
    int len_e = 0;
    int gtp_header_len = 0;
    agc_std_sockaddr_t src_addr;
    agc_std_sockaddr_t dst_addr;
    socklen_t src_addrlen;
    socklen_t dst_addrlen;
	uint8_t *key_src = NULL;
	uint8_t *key_dst = NULL;
	uint32_t key_src_len = 0;
	uint32_t key_dst_len = 0;
	int size = 0;
    char buf_send[1500];
    int32_t buf_send_len = 0;

    gtp_header = (gtp_header_t *)buf;

    if (sess == NULL || buf == NULL || len == NULL)
        return AGC_STATUS_FALSE;

    gtp_header_len += GTPV2C_HEADER_LEN;
    ip_buf += GTPV2C_HEADER_LEN;

    if ((gtp_header->flags & GTPU_FLAGS_E) != 0) 
    {
        len_e = (uint8_t)ip_buf[0] * GTP_EXT_HEADER_LEN_UNIT;
        gtp_header_len += len_e;
    }

    ip_h = (struct ip *)(buf + gtp_header_len);

    if (ip_h->ip_v == 4) {
        struct sockaddr_in *src_addr_v4 = (struct sockaddr_in *)&src_addr;
        struct sockaddr_in *dst_addr_v4 = (struct sockaddr_in *)&dst_addr;
        char str_src_addrv4[64];
        char str_dst_addrv4[64];

        ip_h = (struct ip *)ip_h;
        ip6_h = NULL;

        src_addr_v4->sin_family = AF_INET;
        memcpy(&src_addr_v4->sin_addr, &ip_h->ip_src, sizeof(struct in_addr));
        
        dst_addr_v4->sin_family = AF_INET;
        memcpy(&dst_addr_v4->sin_addr, &ip_h->ip_dst, sizeof(struct in_addr));

        key_src = (uint8_t *)&src_addr_v4->sin_addr;
        key_dst = (uint8_t *)&dst_addr_v4->sin_addr;
        key_src_len = MEDIA_IPADDRV4_KEY_LEN;
        key_dst_len = MEDIA_IPADDRV4_KEY_LEN;
        
        src_addrlen = sizeof(struct sockaddr_in);
        dst_addrlen = sizeof(struct sockaddr_in);

        inet_ntop(AF_INET, &(src_addr_v4->sin_addr), str_src_addrv4, 64);
        inet_ntop(AF_INET, &(dst_addr_v4->sin_addr), str_dst_addrv4, 64);

        //agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "media_gtp_packet_filter str_src_addrv4 %s str_dst_addrv4=%s.\n", 
        //  str_src_addrv4, str_dst_addrv4);
 
    } else if (ip_h->ip_v == 6) {
        struct sockaddr_in6 *src_addr_v6 = (struct sockaddr_in6 *)&src_addr;
        struct sockaddr_in6 *dst_addr_v6 = (struct sockaddr_in6 *)&dst_addr;
        char str_src_addrv6[64];
        char str_dst_addrv6[64];

        ip6_h = (struct ip6_hdr *)ip_h;

        src_addr_v6->sin6_family = AF_INET6;
        memcpy(&src_addr_v6->sin6_addr, &ip6_h->ip6_src, sizeof(struct in6_addr));

        dst_addr_v6->sin6_family = AF_INET6;
        memcpy(&dst_addr_v6->sin6_addr, &ip6_h->ip6_dst, sizeof(struct in6_addr));

        key_src = (uint8_t *)&src_addr_v6->sin6_addr;
        key_dst = (uint8_t *)&dst_addr_v6->sin6_addr;   
        key_src_len = MEDIA_IPADDRV6_KEY_LEN;
        key_dst_len = MEDIA_IPADDRV6_KEY_LEN;   

        src_addrlen = sizeof(struct sockaddr_in6);
        dst_addrlen = sizeof(struct sockaddr_in6);

        inet_ntop(AF_INET6, &(src_addr_v6->sin6_addr), str_src_addrv6, 64);
        inet_ntop(AF_INET6, &(dst_addr_v6->sin6_addr), str_dst_addrv6, 64);

        agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "media_gtp_packet_filter str_src_addrv6 %s str_dst_addrv6=%s.\n", 
            str_src_addrv6, str_dst_addrv6);
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "media_gtp_packet_filter Invalid IP version = %d\n", ip_h->ip_v);
        return AGC_STATUS_FALSE;
    }
		
    buf_send_len = *len - gtp_header_len;
    memcpy(buf_send, buf + gtp_header_len, buf_send_len);
    //agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "media_gtp_packet_filter 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x.\n",
    //    buf_send[0], buf_send[1], buf_send[2], buf_send[4], buf_send[5], buf_send[6], buf_send[7]);
    lbo_key = media_find_lbo_node_by_key(key_dst, key_dst_len);
    if (lbo_key == NULL)
    {
        return AGC_STATUS_FALSE;
    }
	
    ip_h = (struct ip *)buf_send;

    if (sess->qos_flow_id == 0)
    {
		sess->qos_flow_id = buf[14];

        if (ip_h->ip_v == 4) 
        {
            memcpy(&sess->ue_addr, &ip_h->ip_src, sizeof(struct in_addr));
            agc_hash_set(media_get_media_context()->ue_ip_table, &sess->ue_addr, sizeof(struct in_addr), sess);
        }
        else if (ip_h->ip_v == 6) {
            ip6_h = (struct ip6_hdr *)ip_h;

            memcpy(&sess->ue_addr, &ip6_h->ip6_src, sizeof(struct in6_addr));
            agc_hash_set(media_get_media_context()->ue_ip_table, sess->ue_addr, sizeof(struct in6_addr), sess);
        }
        else
            return AGC_STATUS_FALSE;
    }

    size = write(media_get_media_context()->subnet.sock, buf_send, buf_send_len);
    if (size < 0)
    {
    	int err = errno;
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_gtp_packet_filter write failed sock_fd %d:%d(%s).\n",
			media_get_media_context()->subnet.sock, err, strerror(err));
		return AGC_STATUS_FALSE;
    }

    agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "media_gtp_packet_filter send succed size=%d.\n", size);
    return AGC_STATUS_SUCCESS;
}

void media_gtp_path_recv_cb(agc_gtp_sock_t *sock, const char* buf, int32_t *len,
    agc_std_sockaddr_t *remote_addr, socklen_t remote_addrlen)
{
	gtp_header_t *gtp_header = NULL;
	media_sig_sess_t *sess = NULL;
	uint32_t teid = 0;
    char str_addr[64];
    char str_addr_gnb[64];
    char str_addr_upf[64];
    socklen_t addrlen = 0;
    socklen_t addrlengnb = 0; 
    socklen_t addrlenupf = 0; 
	agc_status_t ret = AGC_STATUS_FALSE;
	
	/* the sock is close */
	if (buf == NULL || *len < 0) {
		return;
	}

	gtp_header = (gtp_header_t *)buf;

    if (gtp_header->type == GTPU_MSGTYPE_ECHO_REQ)    {
    	media_gtp_path_handle_echo_req(sock, buf, len, remote_addr, remote_addrlen);
    }
    else if (gtp_header->type == GTPU_MSGTYPE_GPDU || 
        	gtp_header->type == GTPU_MSGTYPE_END_MARKER) {  	
	    teid = ntohl(gtp_header->teid);
	    sess = media_find_media_sig_sess_by_teid(teid);
	    if (sess == NULL) {
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_gtp_path_recv_cb gtp teid %d unknow .\n", teid);
	        return;    	
	    }

        if (teid == sess->gnb.local_teid)
        {
            //Local Break Out feature
            if (media_gtp_packet_filter(sess, buf, len) == AGC_STATUS_SUCCESS)
            {
            }
            else
            {
                gtp_header->teid = htonl(sess->upf.remote_teid);
                ret = agc_gtp_add_message(sess->upf.sock, buf, *len, &sess->upf.remote_addr, sess->upf.remote_addrlen);
				if (ret == AGC_STATUS_FALSE)
				{
					agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_gtp_path_recv_cb send to upf fail.\n");
				}
            }
        }
        else if (teid == sess->upf.local_teid)
        {
            gtp_header->teid = htonl(sess->gnb.remote_teid);
            ret = agc_gtp_add_message(sess->gnb.sock, buf, *len, &sess->gnb.remote_addr, sess->gnb.remote_addrlen);
			if (ret == AGC_STATUS_FALSE)
			{
				agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_gtp_path_recv_cb send to gnb fail.\n");
			}
        }
        else if (teid == sess->gnbv6.local_teid)
        {
            //Local Break Out feature
            if (media_gtp_packet_filter(sess, buf, len) == AGC_STATUS_SUCCESS)
            {
            }
            else
            {
                gtp_header->teid = htonl(sess->upfv6.remote_teid);
                ret = agc_gtp_add_message(sess->upfv6.sock, buf, *len, &sess->upfv6.remote_addr, sess->upfv6.remote_addrlen);
				if (ret == AGC_STATUS_FALSE)
				{
					agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_gtp_path_recv_cb send to upf fail.\n");
				}
            }
        }
        else if (teid == sess->upfv6.local_teid)
        {
            gtp_header->teid = htonl(sess->gnbv6.remote_teid);
            ret = agc_gtp_add_message(sess->gnbv6.sock, buf, *len, &sess->gnbv6.remote_addr, sess->gnbv6.remote_addrlen);
			if (ret == AGC_STATUS_FALSE)
			{
				agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_gtp_path_recv_cb send to gnb fail.\n");
			}
        }

    }
}

void media_gtp_path_handle_echo_req(agc_gtp_sock_t *sock, const char* buf, int32_t *len,
    agc_std_sockaddr_t *remote_addr, socklen_t remote_addrlen)
{
    gtp_header_t *gtph = NULL;
    gtp_header_t *gtph_resp = NULL;
    uint16_t length;
    int idx;
    char buf_echo_resp[100];

	if (buf == NULL) {
		return;
	}

    gtph = (gtp_header_t *)buf;
    /* Check GTP version. Now only support GTPv1(version = 1) */
    if ((gtph->flags >> 5) != 1)   {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_gtp_path_handle_echo_req gtp version %d unknow .\n", gtph->flags);
        return;
    }

    if (gtph->type != GTPU_MSGTYPE_ECHO_REQ)    {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_gtp_path_handle_echo_req gtp type %d unknow .\n", gtph->type);
        return;
    }

    gtph_resp = (gtp_header_t *)buf_echo_resp;

    /* reply back immediately */
    gtph_resp->flags = (1 << 5); /* set version */
    gtph_resp->flags |= (1 << 4); /* set PT */
    gtph_resp->type = GTPU_MSGTYPE_ECHO_RSP;
    length = 0;     /* length of Recovery IE */
    gtph_resp->teid = 0;
    idx = 8;

    if (gtph->flags & (GTPU_FLAGS_PN | GTPU_FLAGS_S))
    {
        length += 4;
        if (gtph->flags & GTPU_FLAGS_S)
        {
            /* sequence exists */
            gtph_resp->flags |= GTPU_FLAGS_S;
            *((uint8_t *)buf_echo_resp + idx) = *((uint8_t *)buf_echo_resp + idx);
            *((uint8_t *)buf_echo_resp + idx + 1) = *((uint8_t *)buf_echo_resp + idx + 1);
        }
        else
        {
            *((uint8_t *)buf_echo_resp + idx) = 0;
            *((uint8_t *)buf_echo_resp + idx + 1) = 0;
        }
        idx += 2;
        if (gtph->flags & GTPU_FLAGS_PN)
        {
            /* sequence exists */
            gtph_resp->flags |= GTPU_FLAGS_PN;
            *((uint8_t *)buf_echo_resp + idx) = *((uint8_t *)buf_echo_resp + idx);
        }
        else
        {
            *((uint8_t *)buf_echo_resp + idx) = 0;
        }
        idx++;
        *((uint8_t *)buf_echo_resp + idx) = 0; /* next-extension header */
        idx++;
    }
    
    /* fill Recovery IE */
    length += 2;
    *((uint8_t *)buf_echo_resp + idx) = 14; idx++; /* type */
    *((uint8_t *)buf_echo_resp + idx) = 0; idx++; /* restart counter */

    gtph_resp->length = htons(length);

    agc_gtp_add_message(sock, buf_echo_resp, (int32_t)length, remote_addr, remote_addrlen);
}


	
