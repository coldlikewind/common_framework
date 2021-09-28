
#include "mod_media.h"
#include "media_gtp_path.h"
#include "media_tun_path.h"
#include <netinet/ip.h>
#include <netinet/ip6.h>

#define MAX_GTP_MTU 1600

char g_gtpbuf[MAX_GTP_BUFFER];  

static void media_tun_send_gtp_to_ue(media_sig_sess_t *sess, const char* buf, int32_t len)
{
    gtp_header_t *gtp_h = NULL;
    int32_t offset = 0;
    int32_t gtpbuf_len = 0;

    if (sess == NULL)
    	return;

    // need to frament ip packet
    if (len > MAX_GTP_MTU)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_tun_send_gtp_to_ue buf (len:%ld) exceed max size %d ", len, MAX_GTP_MTU);
    	return ;
    }
    

    gtp_h = (gtp_header_t *)g_gtpbuf;
    /* Bits    8  7  6  5  4  3  2  1
     *        +--+--+--+--+--+--+--+--+
     *        |version |PT| 1| E| S|PN|
     *        +--+--+--+--+--+--+--+--+
     *         0  0  1   1  0  0  0  0
     */
    gtp_h->flags = GTPU_FLAGS_DEFAULT_PDU;
    gtp_h->type = GTPU_MSGTYPE_GPDU;
    gtp_h->length = htons(len + GTPV1U_HEADER_LEN);
    gtp_h->teid = htonl(sess->gnb.remote_teid);

    g_gtpbuf[11] = GTPU_NEXT_EXT_HEAD_TYPE;
    g_gtpbuf[12] = 1;
    g_gtpbuf[13] = 0;
    g_gtpbuf[14] = sess->qos_flow_id;
    g_gtpbuf[15] = 0;	//no more ext head

    offset = GTPV2C_HEADER_LEN + GTP_EXT_HEADER_LEN_UNIT;
    memcpy(g_gtpbuf + offset, buf, len);

    gtpbuf_len = len + offset;

    if (sess->gnb.remote_teid != 0)
    {
    	gtp_h->teid = htonl(sess->gnb.remote_teid);
    	agc_gtp_add_message(sess->gnb.sock, g_gtpbuf, gtpbuf_len, &sess->gnb.remote_addr, sess->gnb.remote_addrlen);
    }
    else if (sess->gnbv6.remote_teid != 0)
    {
    	gtp_h->teid = htonl(sess->gnbv6.remote_teid);
    	agc_gtp_add_message(sess->gnbv6.sock, g_gtpbuf, gtpbuf_len, &sess->gnbv6.remote_addr, sess->gnbv6.remote_addrlen);
    }

    agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "media_tun_send_gtp_to_ue send size=%d", gtpbuf_len);
}

void media_tun_path_recv_cb(const char* buf, int32_t len)
{
    agc_std_sockaddr_t src_addr;
    agc_std_sockaddr_t dst_addr;
    socklen_t src_addrlen;
    socklen_t dst_addrlen;
	uint8_t *key_src = NULL;
	uint8_t *key_dst = NULL;
	uint32_t key_src_len = 0;
	uint32_t key_dst_len = 0;
    struct ip *ip_h =  NULL;
    struct ip6_hdr *ip6_h =  NULL;
	media_sig_sess_t *sess = NULL;
	media_context_t *context = media_get_media_context();
	
    ip_h = (struct ip *)(buf);

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

    	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "media_tun_handle_read str_src_addrv4 %s str_dst_addrv4=%s.\n", 
    		str_src_addrv4, str_dst_addrv4);
 
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

    	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "media_tun_handle_read str_src_addrv6=%s str_dst_addrv6=%s.\n", 
    		str_src_addrv6, str_dst_addrv6);
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "media_tun_handle_read Invalid IP version = %d\n", ip_h->ip_v);
        return;
    }

	sess = (media_sig_sess_t *)agc_hash_get(context->ue_ip_table, key_dst, key_dst_len);    
    if (sess == NULL)
    	return;

    media_tun_send_gtp_to_ue(sess, buf, len);
}

static void media_tun_handle_read(void *data)
{	
    char buf[MAX_GTP_BUFFER];    
	media_context_t *context = media_get_media_context();

	// read all data in sock until it is empty.
	while (1)
	{
	    int32_t size = read(context->subnet.sock, buf, MAX_GTP_BUFFER);
	    if (size <= 0)
	    {
	        break;
	    }

	    media_tun_path_recv_cb(buf, size);
	}

}

agc_status_t media_tun_path_open()
{
	media_context_t *context = media_get_media_context();
	agc_memory_pool_t *pool = media_get_memory_pool();
    int flags = 0;

    context->subnet.connection = agc_conn_create_connection(context->subnet.sock, 
                      NULL, 
                      0, 
                      pool,
                      NULL, 
                      media_tun_handle_read,
                      NULL,
                      NULL);

    if (!context->subnet.connection) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_tun_path_open create connection failed .\n");
        return AGC_STATUS_FALSE;
    }

    if (agc_driver_add_connection(context->subnet.connection) != AGC_STATUS_SUCCESS) {
        agc_free_connection(context->subnet.connection);
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_tun_path_open add connection failed .\n");
        return AGC_STATUS_FALSE;
    }

    // set non block socket to receive all data in the buf
    flags = fcntl(context->subnet.sock, F_GETFL);
    fcntl(context->subnet.sock, F_SETFL, flags | O_NONBLOCK);

    return AGC_STATUS_SUCCESS;
}