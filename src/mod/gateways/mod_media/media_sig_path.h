#ifndef __MEDIA_SIG_PATH_H__
#define __MEDIA_SIG_PATH_H__

#include <agc.h>
#include "media_context.h"

#define MEDIA_SIG_EVENT_HEADER_TYPE 	"_type"
#define MEDIA_SIG_EVENT_HEADER_IPTYPE 	"_IPv4orIPv6"
#define MEDIA_SIG_EVENT_HEADER_IPADDR 	"_ipaddr"
#define MEDIA_SIG_EVENT_HEADER_ERRCODE 	"_errcode"
#define MEDIA_SIG_EVENT_HEADER_MEDIA_SESSID "_media_sessid"
#define MEDIA_SIG_EVENT_HEADER_SIG_SESSID 	"_sig_sessid"
#define MEDIA_SIG_EVENT_HEADER_SIG_HOSTNAME	"_sig_hostname"
#define MEDIA_SIG_EVENT_HEADER_MEDIA_HOSTNAME	"_media_hostname"

#define MEDIA_SIG_EVENT_HEADER_UPF_TEID 		"_upf_teid"
#define MEDIA_SIG_EVENT_HEADER_GNB_TEID 		"_gnb_teid"
#define MEDIA_SIG_EVENT_HEADER_GNB_IPADDR 		"_gnb_ipaddr"
#define MEDIA_SIG_EVENT_HEADER_UPF_IPADDR 		"_upf_ipaddr"
#define MEDIA_SIG_EVENT_HEADER_TO_UPF_TEID 		"_to_upf_teid"
#define MEDIA_SIG_EVENT_HEADER_TO_GNB_TEID 		"_to_gnb_teid"
#define MEDIA_SIG_EVENT_HEADER_TO_GNB_IPADDR 	"_to_gnb_ipaddr"
#define MEDIA_SIG_EVENT_HEADER_TO_UPF_IPADDR 	"_to_upf_ipaddr"

#define MEDIA_SIG_EVENT_HEADER_UPF_TEID_V6 		"_upf_teid_v6"
#define MEDIA_SIG_EVENT_HEADER_GNB_TEID_V6 		"_gnb_teid_v6"
#define MEDIA_SIG_EVENT_HEADER_GNB_IPADDR_V6 	"_gnb_ipaddrv6"
#define MEDIA_SIG_EVENT_HEADER_UPF_IPADDR_V6 	"_upf_ipaddrv6"
#define MEDIA_SIG_EVENT_HEADER_TO_UPF_TEID_V6 	"_to_upf_teid_v6"
#define MEDIA_SIG_EVENT_HEADER_TO_GNB_TEID_V6 	"_to_gnb_teid_v6"
#define MEDIA_SIG_EVENT_HEADER_TO_GNB_IPADDR_V6 "_to_gnb_ipaddrv6"
#define MEDIA_SIG_EVENT_HEADER_TO_UPF_IPADDR_V6 "_to_upf_ipaddrv6"

#define MEDIA_SIG_EVENT_CREATE(_event, _sess) \
	do { \
		if ((agc_event_create(&(_event), EVENT_ID_MEDIA2SIG, EVENT_NULL_SOURCEID) != AGC_STATUS_SUCCESS) || !(_event)) { \
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "MEDIA_SIG_EVENT_CREATE create event [fail].\n"); 	\
			return AGC_STATUS_FALSE;	\
		}	\
		_event->debug_id = rand(); \
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "MEDIA_SIG_EVENT_CREATE create event debug_id=%d media_sess_id=%d, sig_sess_id=%d.\n", _event->debug_id, sess->sess_id, sess->sig_sess_id); 	\
	} while (0);

#define MEDIA_SIG_EVENT_ADD_HEADER(_event, _header_name, _header_value) 	\
	do { \
		if (agc_event_add_header((_event), (_header_name), (_header_value)) != AGC_STATUS_SUCCESS) { \
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "MEDIA_EVENT_ADD_HEADER add %s header [fail].\n", (_header_name));	\
			agc_event_destroy(&(_event));	\
			return AGC_STATUS_FALSE;	\
		}	\
	} while(0);

#define MEDIA_SIG_EVENT_ADD_HEADER_INT(_event, _header_name, _header_value) 	\
	do { \
		if (agc_event_add_header((_event), (_header_name), "%u", (_header_value)) != AGC_STATUS_SUCCESS) { \
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "MEDIA_EVENT_ADD_HEADER add %s header [fail].\n", (_header_name));	\
			agc_event_destroy(&(_event));	\
			return AGC_STATUS_FALSE;	\
		}	\
	} while(0);

#define MEDIA_SIG_EVENT_FIRE(_event) \
	do { \
		if (agc_event_fire(&(_event)) != AGC_STATUS_SUCCESS) {	\
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "MEDIA_SIG_EVENT_FIRE fire event [fail].\n");	\
			agc_event_destroy(&(_event));	\
			return AGC_STATUS_FALSE;	\
		}	\
	} while(0);

#define MEDIA_SIG_EVENT_GET_HEADER(_event, _header_name, _header_value)  		\
	do	{																		\
		value = agc_event_get_header((_event), (_header_name));					\
		if (!value) {															\
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "MEDIA_SIG_EVENT_GET_HEADER get %s [fail].\n", (_header_name));	\
			agc_event_destroy(&event);	\
			return;						\
		}								\
	} while (0);

AGC_BEGIN_EXTERN_C

typedef enum _MEDIA_SIG_EVENT_TYPE
{
    MSET_UPF_SETUP_REQ,
    MSET_UPF_SETUP_RSP,
    MSET_GNB_SETUP_REQ,
    MSET_GNB_SETUP_RSP,
    MSET_SESS_CHECK_REQ,
    MSET_SESS_CHECK_RSP,
    MSET_SESS_RELEASE_REQ,
    MSET_SESS_RELEASE_RSP,
    MSET_SESS_RELEASE_IND,
    MSET_UPF_UPDATE_REQ,
    MSET_UPF_UPDATE_RSP,
}MEDIA_SIG_EVENT_TYPE;

typedef enum _MEDIA_SIG_ERRCODE {
	MSEC_SUCCESS,
	MSEC_SESS_INACTIVE
}MEDIA_SIG_ERRCODE;

AGC_DECLARE(agc_status_t) media_sig_path_send_upf_setup_rsp(media_sig_sess_t *sess);
AGC_DECLARE(agc_status_t) media_sig_path_send_upf_update_rsp(media_sig_sess_t *sess);
AGC_DECLARE(agc_status_t) media_sig_path_send_gnb_setup_rsp(media_sig_sess_t *sess);
AGC_DECLARE(agc_status_t) media_sig_path_send_error_rsp(media_sig_sess_t *sess, int type, int errcode);
AGC_DECLARE(agc_status_t) media_sig_path_bind();

AGC_END_EXTERN_C

#endif