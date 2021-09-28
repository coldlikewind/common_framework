
#include "mod_media.h"
#include "media_sig_path.h"
#include "media_gtp_path.h"

#define MEDIA_SIG_EVENT_BIND_NAME "MEDIA_SEB_NAME"

void media_sig_handle_upf_setup_req(agc_event_t *event) {
	const char *value = NULL;
	uint32_t upf_teid = 0;	
	uint32_t upf_teid_v6 = 0;	
	media_sig_sess_t *sess = NULL;
	struct sockaddr_in *local_addr1_v4 = NULL;
	struct sockaddr_in6 *local_addr1_v6 = NULL;
	struct sockaddr_in *remote_addr1_v4 = NULL;
	struct sockaddr_in6 *remote_addr1_v6 = NULL;
	uint32_t sig_sess_id = 0;
	agc_std_sockaddr_t upf_addrv4;
	agc_std_sockaddr_t upf_addrv6;
	socklen_t addrlenv4 = sizeof(struct sockaddr_in);
	socklen_t addrlenv6 = 0;
	const char *str_ipv4 = NULL;
	const char *str_ipv6 = NULL;

	if (!event)
		return;	

	MEDIA_SIG_EVENT_GET_HEADER(event, MEDIA_SIG_EVENT_HEADER_SIG_SESSID, value);
	sig_sess_id = agc_atoui(value);

	MEDIA_SIG_EVENT_GET_HEADER(event, MEDIA_SIG_EVENT_HEADER_UPF_IPADDR, value);
	local_addr1_v4 = (struct sockaddr_in *)&upf_addrv4;
	local_addr1_v4->sin_family = AF_INET;
	local_addr1_v4->sin_port  = htons(MEDIA_GTPV1_U_UDP_PORT);
	inet_pton(AF_INET, value, &(local_addr1_v4->sin_addr));
	str_ipv4 = value;

	value = agc_event_get_header(event, MEDIA_SIG_EVENT_HEADER_UPF_TEID_V6);	
	if (value)
	{
		upf_teid_v6 = agc_atoui(value);
	}

	value = agc_event_get_header(event, MEDIA_SIG_EVENT_HEADER_UPF_IPADDR_V6);	
	if (value)
	{
		local_addr1_v6 = (struct sockaddr_in6 *)&upf_addrv6;
		local_addr1_v6->sin6_family = AF_INET6;
		local_addr1_v6->sin6_port  = htons(MEDIA_GTPV1_U_UDP_PORT);
		inet_pton(AF_INET6, value, &(local_addr1_v6->sin6_addr));
		addrlenv6 = sizeof(struct sockaddr_in6);
		str_ipv6 = value;
	}

	MEDIA_SIG_EVENT_GET_HEADER(event, MEDIA_SIG_EVENT_HEADER_UPF_TEID, value);
	upf_teid = agc_atoui(value);

	sess = media_add_media_sig_sess();
	if (sess == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_sig_handle_upf_setup_req fail to add sess.\n");
		return;
	}

	MEDIA_SIG_EVENT_GET_HEADER(event, MEDIA_SIG_EVENT_HEADER_SIG_HOSTNAME, value);

	sess->sig_sess_id 			= sig_sess_id;	
	strcpy(sess->sig_gateway_name, value);

	sess->upf.remote_teid 		= upf_teid;
	sess->upf.remote_addrlen 	= addrlenv4;
	memcpy(&sess->upf.remote_addr, &upf_addrv4, addrlenv4);	

	if (addrlenv6 > 0)
	{
		sess->upfv6.remote_teid 	= upf_teid;
		sess->upfv6.remote_addrlen 	= addrlenv6;
		memcpy(&sess->upfv6.remote_addr, &upf_addrv6, addrlenv6);
	}

	if (media_sig_path_send_upf_setup_rsp(sess) != AGC_STATUS_SUCCESS) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_sig_handle_upf_setup_req send rsp fail.\n");
		media_remove_media_sig_sess(sess);
		return;		
	}
	
	media_upf_gtp_open(sess);
    agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "media_sig_handle_upf_setup_req sig_sess_id=%d media_sess_id=%d to_upf_teid=%d local_teid=%d debug_id=%d ipv4=%s, ipv6=%s.\n", 
    	sig_sess_id, sess->sess_id, upf_teid, sess->upf.local_teid, event->debug_id, str_ipv4, str_ipv6);
}

void media_sig_handle_upf_update_req(agc_event_t *event) {
	const char *value = NULL;
	uint32_t sess_id = 0;
	uint32_t sig_sess_id = 0;	
	uint32_t upf_teid = 0;	
	uint32_t upf_teid_v6 = 0;	
	media_sig_sess_t *sess = NULL;
	struct sockaddr_in *local_addr1_v4 = NULL;
	struct sockaddr_in6 *local_addr1_v6 = NULL;
	agc_std_sockaddr_t upf_addrv4;
	agc_std_sockaddr_t upf_addrv6;
	socklen_t addrlenv4 = sizeof(struct sockaddr_in);
	socklen_t addrlenv6 = 0;

	if (!event)
		return;	

	MEDIA_SIG_EVENT_GET_HEADER(event, MEDIA_SIG_EVENT_HEADER_SIG_SESSID, value);
	sig_sess_id = agc_atoui(value);

	MEDIA_SIG_EVENT_GET_HEADER(event, MEDIA_SIG_EVENT_HEADER_MEDIA_SESSID, value);
	sess_id = agc_atoui(value);	
	sess = media_find_media_sig_sess_by_id( sess_id);
	if (sess == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_sig_handle_upf_update_req fail sess_id=%d sess->sig_sess_id=%d\n", sess_id, sess->sig_sess_id);
		return;		
	}

	MEDIA_SIG_EVENT_GET_HEADER(event, MEDIA_SIG_EVENT_HEADER_UPF_IPADDR, value);
	local_addr1_v4 = (struct sockaddr_in *)&upf_addrv4;
	local_addr1_v4->sin_family = AF_INET;
	local_addr1_v4->sin_port  = htons(MEDIA_GTPV1_U_UDP_PORT);
	inet_pton(AF_INET, value, &(local_addr1_v4->sin_addr));

	value = agc_event_get_header(event, MEDIA_SIG_EVENT_HEADER_UPF_TEID_V6);	
	if (value)
	{
		upf_teid_v6 = agc_atoui(value);
	}

	value = agc_event_get_header(event, MEDIA_SIG_EVENT_HEADER_UPF_IPADDR_V6);	
	if (value)
	{
		local_addr1_v6 = (struct sockaddr_in6 *)&upf_addrv6;
		local_addr1_v6->sin6_family = AF_INET6;
		local_addr1_v6->sin6_port  = htons(MEDIA_GTPV1_U_UDP_PORT);
		inet_pton(AF_INET6, value, &(local_addr1_v6->sin6_addr));
		addrlenv6 = sizeof(struct sockaddr_in6);
	}
	
	MEDIA_SIG_EVENT_GET_HEADER(event, MEDIA_SIG_EVENT_HEADER_UPF_TEID, value);
	upf_teid = agc_atoui(value);

	sess->upf.remote_teid = upf_teid;
	sess->upf.remote_addrlen = addrlenv4;
	memcpy(&sess->upf.remote_addr, &upf_addrv4, addrlenv4);
	
	if (addrlenv6 > 0)
	{
		sess->upfv6.remote_teid 	= upf_teid;
		sess->upfv6.remote_addrlen 	= addrlenv6;
		memcpy(&sess->upfv6.remote_addr, &upf_addrv6, addrlenv6);
	}

	if (media_sig_path_send_error_rsp(sess, MSET_UPF_UPDATE_RSP, 0) != AGC_STATUS_SUCCESS) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_sig_handle_upf_update_req send rsp fail.\n");
		media_remove_media_sig_sess(sess);
		return;		
	}
}

void media_sig_handle_gnb_setup_req(agc_event_t *event) {
	const char *value = NULL;
	uint32_t sess_id = 0;	
	uint32_t sig_sess_id = 0;
	uint32_t gnb_teid = 0;	
	uint32_t gnb_teid_v6 = 0;	
	media_sig_sess_t *sess = NULL;
	agc_std_sockaddr_t gnb_addr;
	agc_std_sockaddr_t gnb_addr_v6;
	socklen_t addrlen = 0;
	socklen_t addrlenv6 = 0;
	struct sockaddr_in *local_addr1_v4 = NULL;
	struct sockaddr_in6 *local_addr1_v6 = NULL;

	if (!event)
		return;	

	MEDIA_SIG_EVENT_GET_HEADER(event, MEDIA_SIG_EVENT_HEADER_SIG_SESSID, value);
	sig_sess_id = agc_atoui(value);

	MEDIA_SIG_EVENT_GET_HEADER(event, MEDIA_SIG_EVENT_HEADER_MEDIA_SESSID, value);
	sess_id = agc_atoui(value);	
	sess = media_find_media_sig_sess_by_id( sess_id);
	if (sess == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_sig_handle_upf_update_req fail sess_id=%d sig_sess_id=%d\n",
			sess_id, sig_sess_id);
		return;		
	}

	if (sig_sess_id != sess->sig_sess_id) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_sig_handle_gnb_setup_req fail sess_id=%d sess->sig_sess_id=%d\n", sess_id, sess->sig_sess_id);
		return;		
	}

	MEDIA_SIG_EVENT_GET_HEADER(event, MEDIA_SIG_EVENT_HEADER_GNB_IPADDR, value);
	local_addr1_v4 = (struct sockaddr_in *)&gnb_addr;
	local_addr1_v4->sin_family = AF_INET;
	local_addr1_v4->sin_port  = htons(MEDIA_GTPV1_U_UDP_PORT);
	inet_pton(AF_INET, value, &(local_addr1_v4->sin_addr));
	addrlen = sizeof(struct sockaddr_in);

	value = agc_event_get_header(event, MEDIA_SIG_EVENT_HEADER_GNB_TEID_V6);	
	if (value)
	{
		gnb_teid_v6 = agc_atoui(value);
	}

	value = agc_event_get_header(event, MEDIA_SIG_EVENT_HEADER_GNB_IPADDR_V6);	
	if (value)
	{
		local_addr1_v6 = (struct sockaddr_in6 *)&gnb_addr_v6;
		local_addr1_v6->sin6_family = AF_INET6;
		local_addr1_v6->sin6_port  = htons(MEDIA_GTPV1_U_UDP_PORT);
		inet_pton(AF_INET6, value, &(local_addr1_v6->sin6_addr));
		addrlenv6 = sizeof(struct sockaddr_in6);
	}
	
	MEDIA_SIG_EVENT_GET_HEADER(event, MEDIA_SIG_EVENT_HEADER_GNB_TEID, value);
	gnb_teid = agc_atoui(value);

	if (sess->gnb.remote_teid != 0)
    {
	    media_gtp_send_n3_end_mark(sess);
    }

	sess->gnb.remote_teid = gnb_teid;
	sess->gnb.remote_addrlen = addrlen;
	memcpy(&sess->gnb.remote_addr, &gnb_addr, addrlen);

	if (addrlenv6)
	{
		sess->gnbv6.remote_teid = gnb_teid_v6;
		sess->gnbv6.remote_addrlen = addrlenv6;
		memcpy(&sess->gnbv6.remote_addr, &gnb_addr_v6, addrlenv6);
	}
	
	media_gnb_gtp_open(sess);
	
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "media_sig_handle_gnb_setup_req  gnb_teid=%d gnb_teid_v6=%d\n", gnb_teid, gnb_teid_v6);

	if (media_sig_path_send_gnb_setup_rsp(sess) != AGC_STATUS_SUCCESS) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_sig_handle_gnb_setup_req send rsp fail.\n");
		media_remove_media_sig_sess(sess);
		return;		
	}
}

void media_sig_handle_check_sess_req(agc_event_t *event) {
	const char *value = NULL;
	uint32_t sess_id = 0;	
	media_sig_sess_t *sess = NULL;
	uint32_t sig_sess_id = 0;

	if (!event)
		return;	

	MEDIA_SIG_EVENT_GET_HEADER(event, MEDIA_SIG_EVENT_HEADER_SIG_SESSID, value);
	sig_sess_id = agc_atoui(value);

	MEDIA_SIG_EVENT_GET_HEADER(event, MEDIA_SIG_EVENT_HEADER_MEDIA_SESSID, value);
	sess_id = agc_atoui(value);	
	sess = media_find_media_sig_sess_by_id( sess_id);
	if (sess == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_sig_handle_upf_update_req fail sess_id=%d sess->sig_sess_id=%d\n", sess_id, sess->sig_sess_id);
		return;		
	}

	media_sig_path_send_error_rsp(sess, MSET_SESS_CHECK_RSP, 0);
}

void media_sig_handle_sess_release_req(agc_event_t *event) {
	const char *value = NULL;
	uint32_t sess_id = 0;	
	media_sig_sess_t *sess = NULL;
	uint32_t sig_sess_id = 0;

	if (!event)
		return;	

	MEDIA_SIG_EVENT_GET_HEADER(event, MEDIA_SIG_EVENT_HEADER_SIG_SESSID, value);
	sig_sess_id = agc_atoui(value);

	MEDIA_SIG_EVENT_GET_HEADER(event, MEDIA_SIG_EVENT_HEADER_MEDIA_SESSID, value);
	sess_id = agc_atoui(value);	
	sess = media_find_media_sig_sess_by_id(sess_id);
	if (sess == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_sig_handle_upf_update_req fail sess_id=%d sig_sess_id=%d\n", sess_id, sig_sess_id);
		return;		
	}

	if (sig_sess_id != sess->sig_sess_id) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_sig_handle_sess_release_req fail sess_id=%d sess->sig_sess_id=%d\n", sess_id, sess->sig_sess_id);
		return;		
	}

	media_sig_path_send_error_rsp(sess, MSET_SESS_RELEASE_RSP, 0);

	media_remove_media_sig_sess(sess);
}

void media_sig_path_event_callback(void *data){
	agc_event_t *event = (agc_event_t *)data;
	const char *value = NULL;
	int type;

	if (!event)
		return;	

	value = agc_event_get_header(event, MEDIA_SIG_EVENT_HEADER_TYPE);
	if (!value) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_sig_path_event_callback get %s [fail].\n", MEDIA_SIG_EVENT_HEADER_TYPE);
		agc_event_destroy(&event);
		return;
	}

	type = agc_atoui(value);
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "media_sig_path_event_callback msg type=%d .\n", type);
	switch(type) {
		case MSET_UPF_SETUP_REQ:
			media_sig_handle_upf_setup_req(event);
			break;
		case MSET_GNB_SETUP_REQ:
			media_sig_handle_gnb_setup_req(event);
			break;
		case MSET_SESS_CHECK_REQ:
			media_sig_handle_check_sess_req(event);
			break;
		case MSET_SESS_RELEASE_REQ:
			media_sig_handle_sess_release_req(event);
			break;
		case MSET_UPF_UPDATE_REQ:
			media_sig_handle_upf_update_req(event);
			break;
		default:
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_sig_path_event_callback unsupported msg type=%d .\n", type);
			break;
	}
}

agc_status_t media_sig_path_bind() {	
	if (agc_event_bind(MEDIA_SIG_EVENT_BIND_NAME, EVENT_ID_SIG2MEDIA, media_sig_path_event_callback) != AGC_STATUS_SUCCESS) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_sig_path_bind [fail].\n");
		return AGC_STATUS_FALSE;
	}

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "media_sig_path_bind [success].\n");
	return AGC_STATUS_SUCCESS;
}

agc_status_t media_sig_path_send_upf_setup_rsp(media_sig_sess_t *sess) {
	agc_event_t *new_event = NULL;
	int errcode = 0;
	char str_addr_upf[64];
	char str_addr_gnb[64];
	char routing[255];
	struct sockaddr_in *listen_addr4 = NULL;
	struct sockaddr_in6 *listen_addr6 = NULL;

	if (sess == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_sig_path_send_upf_setup_rsp sess is null .\n");
		return AGC_STATUS_FALSE;
	}

	listen_addr4 = (struct sockaddr_in *)&sess->upf.local_addr;
	inet_ntop(AF_INET, &(listen_addr4->sin_addr), str_addr_upf, 64);

	listen_addr4 = (struct sockaddr_in *)&sess->gnb.local_addr;
	inet_ntop(AF_INET, &(listen_addr4->sin_addr), str_addr_gnb, 64);

	sprintf(routing, "%s.%s", sess->sig_gateway_name, EVENT_NAME_MEDIA2SIG);

	MEDIA_SIG_EVENT_CREATE(new_event, sess);
	MEDIA_SIG_EVENT_ADD_HEADER_INT(new_event, MEDIA_SIG_EVENT_HEADER_TYPE, (MSET_UPF_SETUP_RSP));
	MEDIA_SIG_EVENT_ADD_HEADER_INT(new_event, MEDIA_SIG_EVENT_HEADER_MEDIA_SESSID, (sess->sess_id));
	MEDIA_SIG_EVENT_ADD_HEADER_INT(new_event, MEDIA_SIG_EVENT_HEADER_SIG_SESSID, (sess->sig_sess_id));
	MEDIA_SIG_EVENT_ADD_HEADER_INT(new_event, MEDIA_SIG_EVENT_HEADER_TO_UPF_TEID, (sess->upf.local_teid));
	MEDIA_SIG_EVENT_ADD_HEADER_INT(new_event, MEDIA_SIG_EVENT_HEADER_TO_GNB_TEID, (sess->gnb.local_teid));
	MEDIA_SIG_EVENT_ADD_HEADER_INT(new_event, MEDIA_SIG_EVENT_HEADER_ERRCODE, (errcode));
	MEDIA_SIG_EVENT_ADD_HEADER(new_event, MEDIA_SIG_EVENT_HEADER_TO_UPF_IPADDR, str_addr_upf);
	MEDIA_SIG_EVENT_ADD_HEADER(new_event, MEDIA_SIG_EVENT_HEADER_TO_GNB_IPADDR, str_addr_gnb);
	MEDIA_SIG_EVENT_ADD_HEADER(new_event, EVENT_HEADER_ROUTING, routing);

	if (sess->upfv6.local_addrlen && sess->gnbv6.local_addrlen)
	{
		listen_addr6 = (struct sockaddr_in6 *)&sess->upfv6.local_addr;
		inet_ntop(AF_INET6, &(listen_addr6->sin6_addr), str_addr_upf, 64);

		listen_addr6 = (struct sockaddr_in6 *)&sess->gnbv6.local_addr;
		inet_ntop(AF_INET6, &(listen_addr6->sin6_addr), str_addr_gnb, 64);

		MEDIA_SIG_EVENT_ADD_HEADER_INT(new_event, MEDIA_SIG_EVENT_HEADER_TO_UPF_TEID_V6, (sess->upfv6.local_teid));
		MEDIA_SIG_EVENT_ADD_HEADER_INT(new_event, MEDIA_SIG_EVENT_HEADER_TO_GNB_TEID_V6, (sess->gnbv6.local_teid));
		MEDIA_SIG_EVENT_ADD_HEADER(new_event, MEDIA_SIG_EVENT_HEADER_TO_UPF_IPADDR_V6, str_addr_upf);
		MEDIA_SIG_EVENT_ADD_HEADER(new_event, MEDIA_SIG_EVENT_HEADER_TO_GNB_IPADDR_V6, str_addr_gnb);
	}
	MEDIA_SIG_EVENT_FIRE(new_event);

	return AGC_STATUS_SUCCESS;
}

agc_status_t media_sig_path_send_gnb_setup_rsp(media_sig_sess_t *sess) {
	agc_event_t *new_event = NULL;
	int errcode = 0;
	char routing[255];

	if (sess == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_sig_path_send_gnb_setup_rsp sess is null .\n");
		return AGC_STATUS_FALSE;
	}

	sprintf(routing, "%s.%s", sess->sig_gateway_name, EVENT_NAME_MEDIA2SIG);

	MEDIA_SIG_EVENT_CREATE(new_event, sess);
	MEDIA_SIG_EVENT_ADD_HEADER_INT(new_event, MEDIA_SIG_EVENT_HEADER_TYPE, (MSET_GNB_SETUP_RSP));
	MEDIA_SIG_EVENT_ADD_HEADER_INT(new_event, MEDIA_SIG_EVENT_HEADER_MEDIA_SESSID, (sess->sess_id));
	MEDIA_SIG_EVENT_ADD_HEADER_INT(new_event, MEDIA_SIG_EVENT_HEADER_SIG_SESSID, (sess->sig_sess_id));
	MEDIA_SIG_EVENT_ADD_HEADER_INT(new_event, MEDIA_SIG_EVENT_HEADER_ERRCODE, (errcode));
	MEDIA_SIG_EVENT_ADD_HEADER(new_event, EVENT_HEADER_ROUTING, routing);
	MEDIA_SIG_EVENT_FIRE(new_event);

	return AGC_STATUS_SUCCESS;
}

agc_status_t media_sig_path_send_error_rsp(media_sig_sess_t *sess, int type, int errcode) {	
	agc_event_t *new_event = NULL;
	char routing[255];
	
	if (sess == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media_sig_path_send_error_rsp sess is null .\n");
		return AGC_STATUS_FALSE;
	}

	sprintf(routing, "%s.%s", sess->sig_gateway_name, EVENT_NAME_MEDIA2SIG);

	MEDIA_SIG_EVENT_CREATE(new_event, sess);
	MEDIA_SIG_EVENT_ADD_HEADER_INT(new_event, MEDIA_SIG_EVENT_HEADER_TYPE, (type));
	MEDIA_SIG_EVENT_ADD_HEADER_INT(new_event, MEDIA_SIG_EVENT_HEADER_MEDIA_SESSID, (sess->sess_id));
	MEDIA_SIG_EVENT_ADD_HEADER_INT(new_event, MEDIA_SIG_EVENT_HEADER_SIG_SESSID, (sess->sig_sess_id));
	MEDIA_SIG_EVENT_ADD_HEADER_INT(new_event, MEDIA_SIG_EVENT_HEADER_ERRCODE, (errcode));
	MEDIA_SIG_EVENT_ADD_HEADER(new_event, EVENT_HEADER_ROUTING, routing);
	MEDIA_SIG_EVENT_FIRE(new_event);

	return AGC_STATUS_SUCCESS;
}
