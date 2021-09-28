#ifndef __MEDIA_CONTEXT_H__
#define __MEDIA_CONTEXT_H__

#include <agc.h>
#include <agc_list.h>
#include "agc_tun.h"
#include "agc_gtp_path.h"
#include "mod_media.h"

#define MEDIA_GTPV1_U_UDP_PORT            2152
#define MEDIA_MAX_CHECK_SESS	          1000
#define MEDIA_MAX_SESS_TIMEOUT	          (35ll * 60ll * 1000ll * 1000ll)   // five minus

#define MEDIA_FOUR_TIMES				4
#define MAX_MEDIA_SESSIONS				(100000)
#define MEDIA_VALID_TEID     			(400000)  // MAX_MEDIA_SESSIONS * 4
  
#define MEDIA_GTP_DIRECTION_FROM_UPF    (0x80000000)

#define MEDIA_IPADDRV4_KEY_LEN       4
#define MEDIA_IPADDRV6_KEY_LEN       16

#define MEDIA_DELAY_RELEASE_TIMER    2000

AGC_BEGIN_EXTERN_C

typedef enum {
	MEDIA_CFG_GTP_IPV4 = 0,
	MEDIA_CFG_GTP_IPV6 = 1,
}media_gtp_ip_t;

typedef enum media_interface{
	MEDIA_INTERFACE_N3 		= 0,
	MEDIA_INTERFACE_N3_UPF 	= 1,
}media_interface_t;

typedef struct media_gtp_node
{
	agc_node_t 			node;

	agc_std_sockaddr_t 	addr;
	socklen_t 			addrlen;

	agc_gtp_sock_t  	*sock;

	media_gtp_ip_t      ip_type;
	media_interface_t 	interface_type;
} media_gtp_node_t;

typedef struct media_gtp_node_v2
{
	agc_node_t 			node;

	int 				ref;
	
	agc_gtp_sock_t  	*sock;
} media_gtp_node_v2_t;

typedef struct media_gtp_path {
    uint32_t      		remote_teid;
    uint32_t			local_teid;

	agc_std_sockaddr_t 	local_addr;
	socklen_t 			local_addrlen;

	agc_std_sockaddr_t 	remote_addr;
	socklen_t 			remote_addrlen;

	agc_gtp_sock_t  	*sock;
} media_gtp_path_t;


typedef struct media_ipsubnet
{

	char 			dev_if_name[16];

    agc_connection_t  *connection;
	agc_std_socket_t   sock;
}media_ipsubnet_t;

typedef struct media_lbo_sess
{
	agc_node_t 		 	node;

	uint32_t			addr[4];

	media_ipsubnet_t   	*ipsubnet;
}media_lbo_sess_t;

typedef struct media_sig_sess
{
	agc_node_t 		 node;

    uint32_t	     sess_id; 				// sess_id is the same as local teid
    uint32_t		 sig_sess_id;			// sig_sess_id is alloc by sig gateway

    char			 sig_gateway_name[255];

	media_gtp_path_t upf;
	media_gtp_path_t gnb;	

	media_gtp_path_t upfv6;
	media_gtp_path_t gnbv6;	

	// LBO feature
	uint32_t			ue_addr[4];
	uint8_t				qos_flow_id;

	//media_lbo_sess_t 	*lbo_sess;

	agc_time_t			last_act_time;
} media_sig_sess_t;

typedef struct media_hash_key_table
{
	agc_node_t 		 node;

	uint8_t *key;
	uint32_t key_len;
}media_hash_key_table_t;

typedef struct media_context
{
	uint32_t 		next_sess_id;
	uint32_t		gtpu_port;

	media_ipsubnet_t subnet;

	agc_list_t		hash_key_list;
	agc_list_t		server_list;
	agc_list_t		server_list_v2;
	agc_list_t		lbo_sess_list;

	agc_hash_t	   		*sess_id_table;
	agc_hash_t	   		*ue_ip_table;
	agc_hash_t	   		*lbo_ip_table;

	media_sig_sess_t	*last_check_sess_node;
	media_sig_sess_t    *session_list[MAX_MEDIA_SESSIONS];
} media_context_t;

AGC_DECLARE(agc_status_t) 		media_context_init();
AGC_DECLARE(agc_status_t) 		media_context_shutdown();

AGC_DECLARE(media_context_t *) 	media_get_media_context();

AGC_DECLARE(agc_status_t)  		media_add_lbo_node(agc_std_sockaddr_t *addr, socklen_t addrlen);
AGC_DECLARE(media_hash_key_table_t *) media_find_lbo_node_by_key(uint8_t *key, uint32_t key_len);
AGC_DECLARE(agc_status_t)  		media_remove_all_hash_key();

AGC_DECLARE(agc_status_t)  		media_add_gtp_node(agc_std_sockaddr_t *addr, socklen_t addrlen, media_gtp_ip_t ip_type, media_interface_t it);
AGC_DECLARE(agc_status_t)  		media_remove_gtp_node(media_gtp_node_t *node);
AGC_DECLARE(agc_status_t)  		media_remove_all_gtp_nodes();
AGC_DECLARE(media_gtp_node_t *) media_gtp_node_first();

AGC_DECLARE(media_sig_sess_t *) media_add_media_sig_sess();
AGC_DECLARE(media_sig_sess_t *) media_find_media_sig_sess_by_id(uint32_t sess_id);
AGC_DECLARE(media_sig_sess_t *) media_find_media_sig_sess_by_teid(uint32_t teid);
AGC_DECLARE(agc_status_t) 		media_remove_media_sig_sess(media_sig_sess_t *sess);
AGC_DECLARE(agc_status_t)  		media_remove_all_media_sig_sess();
AGC_DECLARE(void)  				media_check_all_media_sig_sess();
AGC_DECLARE(void)  				media_context_start_check_timer();

AGC_DECLARE(agc_status_t) 		media_load_ipsubnet(const char *ifname);


AGC_DECLARE(agc_status_t) 		media_add_gtp_node_v2(agc_gtp_sock_t *sock);
AGC_DECLARE(agc_status_t) 		media_add_gtp_node_ref_v2(media_gtp_node_v2_t *node);
AGC_DECLARE(agc_status_t) 		media_del_gtp_node_ref_v2(agc_gtp_sock_t *sock);
AGC_DECLARE(agc_status_t) 		media_remove_gtp_node_v2(media_gtp_node_v2_t *node);
AGC_DECLARE(agc_status_t) 		media_remove_all_gtp_nodes_v2();


AGC_END_EXTERN_C

#endif
