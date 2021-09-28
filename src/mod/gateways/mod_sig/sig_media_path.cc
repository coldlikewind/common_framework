
#include "sig_media_path.h"
#include "PduSessManager.h"

#define SIG_MEDIA_EVENT_BIND_NAME "SIG_SEB_NAME"


void sig_media_handle_upf_setup_rsp(agc_event_t *event) {
	const char *value = NULL;
	socklen_t to_upf_addrlen = 0;
	socklen_t to_upf_addrlen_v6 = 0;
	socklen_t to_gnb_addrlen = 0;
	socklen_t to_gnb_addrlen_v6 = 0;
	agc_std_sockaddr_t to_upf_addr;
	agc_std_sockaddr_t to_upf_addr_v6;
	agc_std_sockaddr_t to_gnb_addr;
	agc_std_sockaddr_t to_gnb_addr_v6;
	uint32_t sig_sess_id = 0;
	uint32_t media_sess_id = 0;
	uint32_t errcode = 0;
	uint32_t to_upf_teid = 0;
	uint32_t to_gnb_teid = 0;
	uint32_t to_upf_teid_v6 = 0;
	uint32_t to_gnb_teid_v6 = 0;
	struct sockaddr_in *local_addr1_v4 = NULL;
	struct sockaddr_in6 *local_addr1_v6 = NULL;
	
	if (!event)
		return;	
	
	SIG_MEDIA_EVENT_GET_HEADER(event, SIG_MEDIA_EVENT_HEADER_ERRCODE, value);
	errcode = agc_atoui(value);

	if (errcode != MSEC_SUCCESS) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sig_media_handle_upf_setup_rsp failed errcode=%d.\n", errcode);
		return;
	}

	SIG_MEDIA_EVENT_GET_HEADER(event, SIG_MEDIA_EVENT_HEADER_SIG_SESSID, value);
	sig_sess_id = agc_atoui(value);

	SIG_MEDIA_EVENT_GET_HEADER(event, SIG_MEDIA_EVENT_HEADER_MEDIA_SESSID, value);
	media_sess_id = agc_atoui(value);

	SIG_MEDIA_EVENT_GET_HEADER(event, SIG_MEDIA_EVENT_HEADER_TO_UPF_TEID, value);
	to_upf_teid = agc_atoui(value);

	SIG_MEDIA_EVENT_GET_HEADER(event, SIG_MEDIA_EVENT_HEADER_TO_GNB_TEID, value);
	to_gnb_teid = agc_atoui(value);

	SIG_MEDIA_EVENT_GET_HEADER(event, SIG_MEDIA_EVENT_HEADER_TO_UPF_IPADDR, value);
	local_addr1_v4 = (struct sockaddr_in *)&to_upf_addr;
	local_addr1_v4->sin_family = AF_INET;
	inet_pton(AF_INET, value, &(local_addr1_v4->sin_addr));
	to_upf_addrlen = sizeof(struct sockaddr_in);

	SIG_MEDIA_EVENT_GET_HEADER(event, SIG_MEDIA_EVENT_HEADER_TO_GNB_IPADDR, value);
	local_addr1_v4 = (struct sockaddr_in *)&to_gnb_addr;
	local_addr1_v4->sin_family = AF_INET;
	inet_pton(AF_INET, value, &(local_addr1_v4->sin_addr));
	to_gnb_addrlen = sizeof(struct sockaddr_in);

	value = agc_event_get_header(event, SIG_MEDIA_EVENT_HEADER_TO_UPF_IPADDR_V6);	
	if (value != NULL)
	{
		local_addr1_v6 = (struct sockaddr_in6 *)&to_upf_addr_v6;
		local_addr1_v6->sin6_family = AF_INET6;
		inet_pton(AF_INET6, value, &(local_addr1_v6->sin6_addr));
		to_upf_addrlen_v6 = sizeof(struct sockaddr_in6);
	}
	value = agc_event_get_header(event, SIG_MEDIA_EVENT_HEADER_TO_GNB_IPADDR_V6);	
	if (value != NULL)
	{
		local_addr1_v6 = (struct sockaddr_in6 *)&to_gnb_addr_v6;
		local_addr1_v6->sin6_family = AF_INET6;
		inet_pton(AF_INET6, value, &(local_addr1_v6->sin6_addr));
		to_gnb_addrlen_v6 = sizeof(struct sockaddr_in6);
	}

	value = agc_event_get_header(event, SIG_MEDIA_EVENT_HEADER_TO_UPF_TEID_V6);	
	if (value != NULL)
	{
		to_upf_teid_v6 = agc_atoui(value);
	}
	value = agc_event_get_header(event, SIG_MEDIA_EVENT_HEADER_TO_GNB_TEID_V6);	
	if (value != NULL)
	{
		to_gnb_teid_v6 = agc_atoui(value);
	}
    agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "sig_media_handle_upf_setup_rsp sig_sess_id=%d media_sess_id=%d to_upf_teid=%d to_gnb_teid=%d debug_id=%d.\n", 
    	sig_sess_id, media_sess_id, to_upf_teid, to_gnb_teid, event->debug_id);

	GetPduSessManager().handleUpfSetupRsp(sig_sess_id, media_sess_id,
		&to_gnb_addr, to_gnb_addrlen, 
		&to_upf_addr, to_upf_addrlen, 
		to_upf_teid, to_gnb_teid,
		&to_gnb_addr_v6, to_gnb_addrlen_v6,
		&to_upf_addr_v6, to_upf_addrlen_v6,
		to_upf_teid_v6, to_gnb_teid_v6);
}

void sig_media_handle_upf_update_rsp(agc_event_t *event) {
	const char *value = NULL;
	uint32_t sig_sess_id = 0;
	uint32_t media_sess_id = 0;	
	uint32_t errcode = 0;	

	if (!event)
		return;	
	
	SIG_MEDIA_EVENT_GET_HEADER(event, SIG_MEDIA_EVENT_HEADER_ERRCODE, value);
	errcode = agc_atoui(value);
	if (errcode != MSEC_SUCCESS) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sig_media_handle_upf_update_rsp failed errcode=%d.\n", errcode);
		return;
	}
	
	SIG_MEDIA_EVENT_GET_HEADER(event, SIG_MEDIA_EVENT_HEADER_SIG_SESSID, value);
	sig_sess_id = agc_atoui(value);

	SIG_MEDIA_EVENT_GET_HEADER(event, SIG_MEDIA_EVENT_HEADER_MEDIA_SESSID, value);
	media_sess_id = agc_atoui(value);

    agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "sig_media_handle_upf_update_rsp sig_sess_id=%d media_sess_id=%d .\n", 
    	sig_sess_id, media_sess_id);

	GetPduSessManager().handleUpfUpdateRsp(sig_sess_id);
}

void sig_media_handle_gnb_setup_rsp(agc_event_t *event) {
	const char *value = NULL;
	uint32_t sig_sess_id = 0;
	uint32_t media_sess_id = 0;	
	uint32_t errcode = 0;	

	if (!event)
		return;	
	
	SIG_MEDIA_EVENT_GET_HEADER(event, SIG_MEDIA_EVENT_HEADER_ERRCODE, value);
	errcode = agc_atoui(value);
	if (errcode != MSEC_SUCCESS) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sig_media_handle_gnb_setup_rsp failed errcode=%d.\n", errcode);
		return;
	}
	
	SIG_MEDIA_EVENT_GET_HEADER(event, SIG_MEDIA_EVENT_HEADER_SIG_SESSID, value);
	sig_sess_id = agc_atoui(value);

	SIG_MEDIA_EVENT_GET_HEADER(event, SIG_MEDIA_EVENT_HEADER_MEDIA_SESSID, value);
	media_sess_id = agc_atoui(value);

    agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "sig_media_handle_gnb_setup_rsp sig_sess_id=%d media_sess_id=%d .\n", 
    	sig_sess_id, media_sess_id);

	GetPduSessManager().handleGnbSetupRsp(sig_sess_id);
}

void sig_media_handle_check_sess_rsp(agc_event_t *event) {
	const char *value = NULL;
	uint32_t media_sess_id = 0;	
	uint32_t sig_sess_id = 0;
	uint32_t errcode = 0;

	if (!event)
		return;	

	SIG_MEDIA_EVENT_GET_HEADER(event, SIG_MEDIA_EVENT_HEADER_SIG_SESSID, value);
	sig_sess_id = agc_atoui(value);

	SIG_MEDIA_EVENT_GET_HEADER(event, SIG_MEDIA_EVENT_HEADER_MEDIA_SESSID, value);
	media_sess_id = agc_atoui(value);

	SIG_MEDIA_EVENT_GET_HEADER(event, SIG_MEDIA_EVENT_HEADER_ERRCODE, value);
	errcode = agc_atoui(value);

	GetPduSessManager().handleGnbSetupRsp(sig_sess_id);
}

void sig_media_handle_sess_release_ind(agc_event_t *event) {
	const char *value = NULL;
	uint32_t media_sess_id = 0;	
	uint32_t sig_sess_id = 0;
	uint32_t errcode = 0;

	if (!event)
		return;	

	SIG_MEDIA_EVENT_GET_HEADER(event, SIG_MEDIA_EVENT_HEADER_SIG_SESSID, value);
	sig_sess_id = agc_atoui(value);

	SIG_MEDIA_EVENT_GET_HEADER(event, SIG_MEDIA_EVENT_HEADER_MEDIA_SESSID, value);
	media_sess_id = agc_atoui(value);

	SIG_MEDIA_EVENT_GET_HEADER(event, SIG_MEDIA_EVENT_HEADER_ERRCODE, value);
	errcode = agc_atoui(value);

    agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "sig_media_handle_sess_release_ind sig_sess_id=%d media_sess_id=%d .\n", 
    	sig_sess_id, media_sess_id);

	GetPduSessManager().handleSessReleaseInd(sig_sess_id);
}

void sig_media_path_event_callback(void *data){
	agc_event_t *event = (agc_event_t *)data;
	const char *value = NULL;
	int type;

	if (!event)
		return;	

	value = agc_event_get_header(event, SIG_MEDIA_EVENT_HEADER_TYPE);
	if (!value) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sig_media_path_event_callback get %s [fail].\n", SIG_MEDIA_EVENT_HEADER_TYPE);
		agc_event_destroy(&event);
		return;
	}

	type = agc_atoui(value);
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "sig_media_path_event_callback msg type=%d .\n", type);
	switch(type) {
		case MSET_UPF_SETUP_RSP:
			sig_media_handle_upf_setup_rsp(event);
			break;
		case MSET_GNB_SETUP_RSP:
			sig_media_handle_gnb_setup_rsp(event);
			break;
		case MSET_SESS_CHECK_RSP:
			sig_media_handle_check_sess_rsp(event);
			break;
		case MSET_SESS_RELEASE_IND:
			sig_media_handle_sess_release_ind(event);
			break;
		case MSET_UPF_UPDATE_RSP:
			sig_media_handle_upf_update_rsp(event);
			break;
		case MSET_SESS_RELEASE_RSP:
			break;
		default:
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sig_media_path_event_callback unsupported msg type=%d .\n", type);
			break;
	}
}

agc_status_t sig_media_path_bind() {	
	if (agc_event_bind(SIG_MEDIA_EVENT_BIND_NAME, EVENT_ID_MEDIA2SIG, sig_media_path_event_callback) != AGC_STATUS_SUCCESS) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sig_media_path_bind [fail].\n");
		return AGC_STATUS_FALSE;
	}

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "sig_media_path_bind [success].\n");
	return AGC_STATUS_SUCCESS;
}

agc_status_t sig_media_path_send_upf_setup_req(uint32_t pdu_sess_id, uint32_t upf_teid,
		agc_std_sockaddr_t *local_addr,	socklen_t local_addrlen, uint32_t upf_teid_v6,
		agc_std_sockaddr_t *local_addr_v6,	socklen_t local_addrlen_v6, char *MediaGWName, char *hostname) {
	agc_event_t *new_event = NULL;
	int errcode = 0;
	char str_addr[64];
	char str_addr_v6[64];
	char routing[255];
	struct sockaddr_in *listen_addr4 = NULL;
	struct sockaddr_in6 *listen_addr6 = NULL;
	
	if (local_addr == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sig_media_path_send_upf_setup_req local_addr is null .\n");
		return AGC_STATUS_FALSE;
	}

	listen_addr4 = (struct sockaddr_in *)local_addr;
	inet_ntop(AF_INET, &(listen_addr4->sin_addr), str_addr, 64);

	sprintf(routing, "%s.%s", MediaGWName, EVENT_NAME_SIG2MEDIA);	
	SIG_MEDIA_EVENT_CREATE(new_event, pdu_sess_id);
	SIG_MEDIA_EVENT_ADD_HEADER_INT(new_event, SIG_MEDIA_EVENT_HEADER_TYPE, (MSET_UPF_SETUP_REQ));
	SIG_MEDIA_EVENT_ADD_HEADER_INT(new_event, SIG_MEDIA_EVENT_HEADER_SIG_SESSID, (pdu_sess_id));
	SIG_MEDIA_EVENT_ADD_HEADER_INT(new_event, SIG_MEDIA_EVENT_HEADER_UPF_TEID, (upf_teid));

	SIG_MEDIA_EVENT_ADD_HEADER(new_event, SIG_MEDIA_EVENT_HEADER_UPF_IPADDR, str_addr);
	SIG_MEDIA_EVENT_ADD_HEADER(new_event, SIG_MEDIA_EVENT_HEADER_SIG_HOSTNAME, hostname);
	SIG_MEDIA_EVENT_ADD_HEADER(new_event, EVENT_HEADER_ROUTING, routing);
	if (local_addr_v6 != NULL && local_addrlen_v6 > 0) 
	{
		listen_addr6 = (struct sockaddr_in6 *)local_addr_v6;
		inet_ntop(AF_INET6, &(listen_addr6->sin6_addr), str_addr_v6, 64);
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "sig_media_path_send_upf_setup_req upf addr6 %s .\n", str_addr_v6);
		
		SIG_MEDIA_EVENT_ADD_HEADER_INT(new_event, SIG_MEDIA_EVENT_HEADER_UPF_TEID_V6, (upf_teid_v6));
		SIG_MEDIA_EVENT_ADD_HEADER(new_event, SIG_MEDIA_EVENT_HEADER_UPF_IPADDR_V6, str_addr_v6);
	}
	SIG_MEDIA_EVENT_FIRE(new_event);
	
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "sig_media_path_send_upf_setup_req pdu_sess_id=%d upf_teid=%d.\n", pdu_sess_id, upf_teid);

	return AGC_STATUS_SUCCESS;
}

agc_status_t sig_media_path_send_gnb_setup_req(uint32_t pdu_sess_id, uint32_t media_sess_id, uint32_t gnb_teid,
		agc_std_sockaddr_t *local_addr,	socklen_t local_addrlen, uint32_t gnb_teid_v6,
		agc_std_sockaddr_t *local_addr_v6,	socklen_t local_addrlen_v6, char *MediaGWName, char *hostname)  {
	agc_event_t *new_event = NULL;
	int errcode = 0;
	char str_addr[64];
	char str_addr_v6[64];
	char routing[255];
	struct sockaddr_in *listen_addr4 = NULL;
	struct sockaddr_in6 *listen_addr6 = NULL;

	if (local_addr == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sig_media_path_send_gnb_setup_req local_addr is null .\n");
		return AGC_STATUS_FALSE;
	}

	listen_addr4 = (struct sockaddr_in *)local_addr;
	inet_ntop(AF_INET, &(listen_addr4->sin_addr), str_addr, 64);
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "sig_media_path_send_gnb_setup_req pdu_sess_id=%d media_sess_id=%d, local addr4 %s .\n",
		pdu_sess_id, media_sess_id, str_addr);
	
	sprintf(routing, "%s.%s", MediaGWName, EVENT_NAME_SIG2MEDIA);
	SIG_MEDIA_EVENT_CREATE(new_event, pdu_sess_id);
	SIG_MEDIA_EVENT_ADD_HEADER_INT(new_event, SIG_MEDIA_EVENT_HEADER_TYPE, (MSET_GNB_SETUP_REQ));
	SIG_MEDIA_EVENT_ADD_HEADER_INT(new_event, SIG_MEDIA_EVENT_HEADER_MEDIA_SESSID, (media_sess_id));
	SIG_MEDIA_EVENT_ADD_HEADER_INT(new_event, SIG_MEDIA_EVENT_HEADER_SIG_SESSID, (pdu_sess_id));
	SIG_MEDIA_EVENT_ADD_HEADER_INT(new_event, SIG_MEDIA_EVENT_HEADER_GNB_TEID, (gnb_teid));

	SIG_MEDIA_EVENT_ADD_HEADER(new_event, SIG_MEDIA_EVENT_HEADER_GNB_IPADDR, str_addr);
	SIG_MEDIA_EVENT_ADD_HEADER(new_event, SIG_MEDIA_EVENT_HEADER_SIG_HOSTNAME, hostname);
	SIG_MEDIA_EVENT_ADD_HEADER(new_event, EVENT_HEADER_ROUTING, routing);

	if (local_addr_v6 != NULL && local_addrlen_v6 > 0)
	{
		listen_addr6 = (struct sockaddr_in6 *)local_addr_v6;
		inet_ntop(AF_INET6, &(listen_addr6->sin6_addr), str_addr_v6, 64);
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "sig_media_path_send_gnb_setup_req gnb addr6 %s .\n", str_addr_v6);
		
		SIG_MEDIA_EVENT_ADD_HEADER_INT(new_event, SIG_MEDIA_EVENT_HEADER_GNB_TEID_V6, (gnb_teid_v6));
		SIG_MEDIA_EVENT_ADD_HEADER(new_event, SIG_MEDIA_EVENT_HEADER_GNB_IPADDR_V6, str_addr_v6);
	}
	SIG_MEDIA_EVENT_FIRE(new_event);

	return AGC_STATUS_SUCCESS;
}

agc_status_t sig_media_path_send_req(uint32_t pdu_sess_id, uint32_t media_sess_id, int type, char *MediaGWName, char *hostname) {	
	agc_event_t *new_event = NULL;
	char routing[255];

	sprintf(routing, "%s.%s", MediaGWName, EVENT_NAME_SIG2MEDIA);
	SIG_MEDIA_EVENT_CREATE(new_event, pdu_sess_id);
	SIG_MEDIA_EVENT_ADD_HEADER_INT(new_event, SIG_MEDIA_EVENT_HEADER_TYPE, (type));
	SIG_MEDIA_EVENT_ADD_HEADER_INT(new_event, SIG_MEDIA_EVENT_HEADER_MEDIA_SESSID, (media_sess_id));
	SIG_MEDIA_EVENT_ADD_HEADER_INT(new_event, SIG_MEDIA_EVENT_HEADER_SIG_SESSID, (pdu_sess_id));
	SIG_MEDIA_EVENT_ADD_HEADER(new_event, SIG_MEDIA_EVENT_HEADER_SIG_HOSTNAME, hostname);
	SIG_MEDIA_EVENT_ADD_HEADER(new_event, EVENT_HEADER_ROUTING, routing);
	SIG_MEDIA_EVENT_FIRE(new_event);

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "sig_media_path_send_req pdu_sess_id=%d media_sess_id=%d.\n",
		pdu_sess_id, media_sess_id);

	return AGC_STATUS_SUCCESS;
}

agc_status_t sig_media_path_send_upf_update_req(uint32_t pdu_sess_id, uint32_t media_sess_id, uint32_t upf_teid,
		agc_std_sockaddr_t *local_addr,	socklen_t local_addrlen, uint32_t upf_teid_v6,
		agc_std_sockaddr_t *local_addr_v6,	socklen_t local_addrlen_v6, char *MediaGWName, char *hostname)  {
	agc_event_t *new_event = NULL;
	int errcode = 0;
	char str_addr[64];
	char str_addr_v6[64];
	char routing[255];
	struct sockaddr_in *listen_addr4 = NULL;
	struct sockaddr_in6 *listen_addr6 = NULL;

	if (local_addr == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sig_media_path_send_upf_update_req local_addr is null .\n");
		return AGC_STATUS_FALSE;
	}

	listen_addr4 = (struct sockaddr_in *)local_addr;
	inet_ntop(AF_INET, &(listen_addr4->sin_addr), str_addr, 64);
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "sig_media_path_send_upf_update_req local addr4 %s .\n", str_addr);

	sprintf(routing, "%s.%s", MediaGWName, EVENT_NAME_SIG2MEDIA);
	SIG_MEDIA_EVENT_CREATE(new_event, pdu_sess_id);
	SIG_MEDIA_EVENT_ADD_HEADER_INT(new_event, SIG_MEDIA_EVENT_HEADER_TYPE, (MSET_UPF_UPDATE_REQ));
	SIG_MEDIA_EVENT_ADD_HEADER_INT(new_event, SIG_MEDIA_EVENT_HEADER_MEDIA_SESSID, (media_sess_id));
	SIG_MEDIA_EVENT_ADD_HEADER_INT(new_event, SIG_MEDIA_EVENT_HEADER_SIG_SESSID, (pdu_sess_id));
	SIG_MEDIA_EVENT_ADD_HEADER_INT(new_event, SIG_MEDIA_EVENT_HEADER_UPF_TEID, (upf_teid));
	SIG_MEDIA_EVENT_ADD_HEADER(new_event, SIG_MEDIA_EVENT_HEADER_UPF_IPADDR, str_addr);
	SIG_MEDIA_EVENT_ADD_HEADER(new_event, SIG_MEDIA_EVENT_HEADER_SIG_HOSTNAME, hostname);
	SIG_MEDIA_EVENT_ADD_HEADER(new_event, EVENT_HEADER_ROUTING, routing);

	if (local_addrlen_v6 > 0 && local_addr_v6 != NULL)
	{
		listen_addr6 = (struct sockaddr_in6 *)local_addr_v6;
		inet_ntop(AF_INET6, &(listen_addr6->sin6_addr), str_addr_v6, 64);
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "sig_media_path_send_upf_update_req remote addr6 %s .\n", str_addr_v6);
		
		SIG_MEDIA_EVENT_ADD_HEADER_INT(new_event, SIG_MEDIA_EVENT_HEADER_UPF_TEID_V6, (upf_teid_v6));
		SIG_MEDIA_EVENT_ADD_HEADER(new_event, SIG_MEDIA_EVENT_HEADER_UPF_IPADDR_V6, str_addr_v6);
	}
	
	SIG_MEDIA_EVENT_FIRE(new_event);

	return AGC_STATUS_SUCCESS;
}

