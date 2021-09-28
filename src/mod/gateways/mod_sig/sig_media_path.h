#ifndef __SIG_MEDIA_PATH_H__
#define __SIG_MEDIA_PATH_H__

#include "mod_sig.h"

#define SIG_MEDIA_EVENT_HEADER_TYPE 	"_type"
#define SIG_MEDIA_EVENT_HEADER_IPTYPE 	"_IPv4orIPv6"
#define SIG_MEDIA_EVENT_HEADER_IPADDR 	"_ipaddr"
#define SIG_MEDIA_EVENT_HEADER_ERRCODE 	"_errcode"
#define SIG_MEDIA_EVENT_HEADER_MEDIA_SESSID "_media_sessid"
#define SIG_MEDIA_EVENT_HEADER_SIG_SESSID 	"_sig_sessid"
#define SIG_MEDIA_EVENT_HEADER_SIG_HOSTNAME	"_sig_hostname"
#define SIG_MEDIA_EVENT_HEADER_MEDIA_HOSTNAME	"_media_hostname"

#define SIG_MEDIA_EVENT_HEADER_UPF_TEID 		"_upf_teid"
#define SIG_MEDIA_EVENT_HEADER_GNB_TEID 		"_gnb_teid"
#define SIG_MEDIA_EVENT_HEADER_GNB_IPADDR 		"_gnb_ipaddr"
#define SIG_MEDIA_EVENT_HEADER_UPF_IPADDR 		"_upf_ipaddr"
#define SIG_MEDIA_EVENT_HEADER_TO_UPF_TEID 		"_to_upf_teid"
#define SIG_MEDIA_EVENT_HEADER_TO_GNB_TEID 		"_to_gnb_teid"
#define SIG_MEDIA_EVENT_HEADER_TO_GNB_IPADDR 	"_to_gnb_ipaddr"
#define SIG_MEDIA_EVENT_HEADER_TO_UPF_IPADDR 	"_to_upf_ipaddr"

#define SIG_MEDIA_EVENT_HEADER_UPF_TEID_V6 		"_upf_teid_v6"
#define SIG_MEDIA_EVENT_HEADER_GNB_TEID_V6 		"_gnb_teid_v6"
#define SIG_MEDIA_EVENT_HEADER_GNB_IPADDR_V6 	"_gnb_ipaddrv6"
#define SIG_MEDIA_EVENT_HEADER_UPF_IPADDR_V6 	"_upf_ipaddrv6"
#define SIG_MEDIA_EVENT_HEADER_TO_UPF_TEID_V6 	"_to_upf_teid_v6"
#define SIG_MEDIA_EVENT_HEADER_TO_GNB_TEID_V6 	"_to_gnb_teid_v6"
#define SIG_MEDIA_EVENT_HEADER_TO_GNB_IPADDR_V6 "_to_gnb_ipaddrv6"
#define SIG_MEDIA_EVENT_HEADER_TO_UPF_IPADDR_V6 "_to_upf_ipaddrv6"

#define SIG_MEDIA_EVENT_CREATE(_event, _pdu_sess_id) \
	do { \
		if ((agc_event_create(&(_event), EVENT_ID_SIG2MEDIA, EVENT_NULL_SOURCEID) != AGC_STATUS_SUCCESS) || !(_event)) { \
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG_MEDIA_EVENT_CREATE create event [fail].\n"); 	\
			return AGC_STATUS_FALSE;	\
		}	\
		_event->debug_id = rand(); \
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "SIG_MEDIA_EVENT_CREATE create event debug_id=%d pdu_sess_id=%d.\n", _event->debug_id, pdu_sess_id); 	\
	} while (0);

#define SIG_MEDIA_EVENT_ADD_HEADER(_event, _header_name, _header_value) 	\
	do { \
		if (agc_event_add_header((_event), (_header_name), (_header_value)) != AGC_STATUS_SUCCESS) { \
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG_MEDIA_EVENT_ADD_HEADER add %s header [fail].\n", (_header_name));	\
			agc_event_destroy(&(_event));	\
			return AGC_STATUS_FALSE;	\
		}	\
	} while(0);

#define SIG_MEDIA_EVENT_ADD_HEADER_INT(_event, _header_name, _header_value) 	\
	do { \
		if (agc_event_add_header((_event), (_header_name), "%u", (_header_value)) != AGC_STATUS_SUCCESS) { \
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG_MEDIA_EVENT_ADD_HEADER_INT add %s header [fail].\n", (_header_name));	\
			agc_event_destroy(&(_event));	\
			return AGC_STATUS_FALSE;	\
		}	\
	} while(0);

#define SIG_MEDIA_EVENT_FIRE(_event) \
	do { \
		if (agc_event_fire(&(_event)) != AGC_STATUS_SUCCESS) {	\
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG_MEDIA_EVENT_FIRE fire event [fail].\n");	\
			agc_event_destroy(&(_event));	\
			return AGC_STATUS_FALSE;	\
		}	\
	} while(0);

#define SIG_MEDIA_EVENT_GET_HEADER(_event, _header_name, _header_value)  		\
	do	{																		\
		value = agc_event_get_header((_event), (_header_name));					\
		if (!value) {															\
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG_MEDIA_EVENT_GET_HEADER get %s [fail].\n", (_header_name));	\
			agc_event_destroy(&event);	\
			return;						\
		}								\
	} while (0);

AGC_BEGIN_EXTERN_C


typedef enum _SIG_MEDIA_EVENT_TYPE
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
    MSET_GNB_UPDATE_REQ,
    MSET_GNB_UPDATE_RSP,
}SIG_MEDIA_EVENT_TYPE;

typedef enum _SIG_MEDIA_ERRCODE {
	MSEC_SUCCESS,
	MSEC_SESS_INACTIVE
}SIG_MEDIA_ERRCODE;

typedef enum _SIG_MEDIA_IPTYPE {
	SIP_MEDIA_IPV4,
	SIP_MEDIA_IPV6
}SIG_MEDIA_IPTYPE;

typedef struct media_gtp_path {
    uint32_t      media_teid;
    uint32_t      remote_teid;

	agc_std_sockaddr_t 	local_addr;
	socklen_t 			local_addrlen;

	agc_std_sockaddr_t 	remote_addr;
	socklen_t 			remote_addrlen;
} media_gtp_path_t;

typedef struct pdu_session_resource 
{
    uint32_t         pdu_session_id;
	struct s_nnsai_t {
	    uint8_t	 		sst;
	    uint32_t		sd;
	} s_nnsai;
    uint64_t		 maxbitrate_dl;
    uint64_t		 maxbitrate_ul;
    uint32_t		 qos_flow_id;
	uint8_t			 qos_character_type;
	enum {QOS_NON_DYNAMIC_5QI, QOS_DYNAMIC_5QI};
    union QosCharacteristics
    {
    	struct nonDynamic5QI{
		    uint32_t		 fiveQI;
		    uint32_t		 priorityLevelQos;
    	} nonvalue;

    	struct Dynamic5QIDescriptor{
		    uint32_t		 priorityLevelQos;
		    uint32_t		 packetDelayBudget;
			uint32_t	 pERScalar;
			uint32_t	 pERExponent;
    	} value ;
    }choice;
	uint32_t	 	 priorityLevelARP;
	uint32_t	 	 pre_emptionCapability;
	uint32_t	 	 pre_emptionVulnerability;
}pdu_session_resource_t;

typedef struct sig_media_sess
{
    uint32_t		 sig_sess_id;			// sig_sess_id is alloc by sig gateway
    uint32_t		 media_sess_id;
	uint32_t		 ngap_sess_id;
	uint8_t			 pdu_resource_id;

    char			 MediaGWName[255];

	media_gtp_path_t upf;
	media_gtp_path_t gnb;	

	media_gtp_path_t upfv6;
	media_gtp_path_t gnbv6;

	agc_event_t      *evt_timer;

	// infomation in pdu session 
	pdu_session_resource_t pdu_resource;

	// target gnb addr
    uint32_t      		target_teid;
	agc_std_sockaddr_t 	target_addr;
	socklen_t 			target_addrlen;
    uint32_t      		target_teid_v6;
	agc_std_sockaddr_t 	target_addr_v6;
	socklen_t 			target_addrlen_v6;
} sig_media_sess_t;

AGC_END_EXTERN_C

AGC_DECLARE(agc_status_t) sig_media_path_send_upf_setup_req(uint32_t pdu_sess_id, uint32_t upf_teid,
		agc_std_sockaddr_t *local_addr,	socklen_t local_addrlen, uint32_t upf_teid_v6,
		agc_std_sockaddr_t *local_addr_v6, socklen_t local_addrlen_v6, char *MediaGWName, char *hostname);
AGC_DECLARE(agc_status_t) sig_media_path_send_gnb_setup_req(uint32_t pdu_sess_id, uint32_t media_sess_id, uint32_t gnb_teid,
		agc_std_sockaddr_t *local_addr,	socklen_t local_addrlen, uint32_t gnb_teid_v6,
		agc_std_sockaddr_t *local_addr_v6,	socklen_t local_addrlen_v6, char *MediaGWName, char *hostname);
AGC_DECLARE(agc_status_t) sig_media_path_send_req(uint32_t pdu_sess_id, uint32_t media_sess_id, int type, char *MediaGWName, char *hostname);
AGC_DECLARE(agc_status_t) sig_media_path_bind() ;

AGC_DECLARE(agc_status_t) sig_media_path_send_upf_update_req(uint32_t pdu_sess_id, uint32_t media_sess_id, uint32_t upf_teid,
		agc_std_sockaddr_t *local_addr,	socklen_t local_addrlen, uint32_t upf_teid_v6,
		agc_std_sockaddr_t *local_addr_v6,	socklen_t local_addrlen_v6, char *MediaGWName, char *hostname);
#endif