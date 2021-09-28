#ifndef __MEDIA_GTP_PATH_H__
#define __MEDIA_GTP_PATH_H__

#include <agc.h>
#include "agc_gtp_path.h"
#include "media_context.h"

#define GTPV1U_HEADER_LEN   8
#define GTPV2C_HEADER_LEN   12
#define GTP_TEID_LEN        4
#define GTP_SEQUENCE_NUMBER_LEN        2
#define GTP_EXT_HEADER_LEN_UNIT        4

/* GTP-U message type, defined in 3GPP TS 29.281 Release 11 */
#define GTPU_MSGTYPE_ECHO_REQ               1
#define GTPU_MSGTYPE_ECHO_RSP               2
#define GTPU_MSGTYPE_ERR_IND                26
#define GTPU_MSGTYPE_SUPP_EXTHDR_NOTI       31
#define GTPU_MSGTYPE_END_MARKER             254
#define GTPU_MSGTYPE_GPDU                   255

#define GTPU_FLAGS_DEFAULT_PDU  			 0x34
#define GTPU_FLAGS_NULL_NEXT_EXTENSION  	 0x30
#define GTPU_NEXT_EXT_HEAD_TYPE     	     0x85

AGC_BEGIN_EXTERN_C

typedef struct _gtp_header_t {
/* GTU-U flags */
#define GTPU_FLAGS_PN                       0x1
#define GTPU_FLAGS_S                        0x2
#define GTPU_FLAGS_E                        0x4
    uint8_t flags;
    uint8_t type;
    uint16_t length;
    uint32_t teid;
} __attribute__ ((packed)) gtp_header_t;


AGC_DECLARE(void) media_gtp_send_n3_end_mark(media_sig_sess_t *sess);
AGC_DECLARE(agc_status_t) media_gtp_open();
AGC_DECLARE(agc_status_t) media_gtp_close();
AGC_DECLARE(agc_status_t) media_upf_gtp_open(media_sig_sess_t *sess);
AGC_DECLARE(agc_status_t) media_gnb_gtp_open(media_sig_sess_t *sess);
AGC_DECLARE(agc_status_t) media_upf_gtp_close(media_sig_sess_t *sess);
AGC_DECLARE(agc_status_t) media_gnb_gtp_close(media_sig_sess_t *sess);

AGC_DECLARE(void) media_gtp_path_recv_cb(agc_gtp_sock_t *sock, const char* buf, int32_t *len,
    agc_std_sockaddr_t *remote_addr, socklen_t remote_addrlen);
AGC_DECLARE(void) media_gtp_path_handle_echo_req(agc_gtp_sock_t *sock, const char* buf, int32_t *len,
    agc_std_sockaddr_t *remote_addr, socklen_t remote_addrlen);

AGC_END_EXTERN_C

#endif