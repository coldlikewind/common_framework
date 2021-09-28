//
// Created by zhangjun on 2020/6/15.
//

#ifndef INC_5GCORE_MOD_IPSEC_H
#define INC_5GCORE_MOD_IPSEC_H

#define SECGW_DB_FILENAME "secgw.db"

#define SECGW_API_SIZE (sizeof(secgw_api_commands)/sizeof(secgw_api_commands[0]))

#define HEADER_ALARM_ID "alarm_id"
#define HEADER_ALARM_MOD_ID "mo_id"
#define HEADER_ALARM_INST_ID "instance_id"
#define HEADER_ALARM_PARAM "alarm_param"
#define HEADER_ALARM_TYPE "alarm_type"

#define ALARM_TYPE_REPORT "report"
#define ALARM_TYPE_CLEAR "clear"

#define SECGW_MO_ID "secgw"
#define IKE_CONN_ALARM_ID "ike connection broken"

int ViciInit();
int secgw_db_init();
vici_conn_t *get_vici_conn();
agc_db_t *get_agc_db();
void parseIkeEvent(void *null, char *name, vici_res_t *res);

typedef void (*secgw_api_func) (agc_stream_handle_t *stream, int argc, char **argv);

typedef struct secgw_api_command {
    char *pname;
    secgw_api_func func;
    char *pcommand;
    char *psyntax;
}secgw_api_command_t;

agc_status_t secgw_main_real(const char *cmd, agc_stream_handle_t *stream);
void secgw_add_ipsec_conn_api(agc_stream_handle_t *stream, int argc, char **argv);
void secgw_mod_ipsec_conn_api(agc_stream_handle_t *stream, int argc, char **argv);
void secgw_rmv_ipsec_conn_api(agc_stream_handle_t *stream, int argc, char **argv);
void secgw_list_ipsec_conn_api(agc_stream_handle_t *stream, int argc, char **argv);
void secgw_add_ipsec_secret_api(agc_stream_handle_t *stream, int argc, char **argv);
void secgw_mod_ipsec_secret_api(agc_stream_handle_t *stream, int argc, char **argv);
void secgw_rmv_ipsec_secret_api(agc_stream_handle_t *stream, int argc, char **argv);
void secgw_list_ipsec_secret_api(agc_stream_handle_t *stream, int argc, char **argv);
void secgw_add_ipsec_cert_api(agc_stream_handle_t *stream, int argc, char **argv);
void secgw_mod_ipsec_cert_api(agc_stream_handle_t *stream, int argc, char **argv);
void secgw_rmv_ipsec_cert_api(agc_stream_handle_t *stream, int argc, char **argv);
void secgw_list_ipsec_cert_api(agc_stream_handle_t *stream, int argc, char **argv);
void secgw_display_ipsec_conn_api(agc_stream_handle_t *stream, int argc, char **argv);

void secgw_make_list_secret_body(int ret, char **body);
void secgw_make_list_cert_body(int ret, char **body);
void secgw_make_list_conn_body(int ret, char **body);
void secgw_make_general_body(int ret, char **body);
void secgw_make_display_conn_body(int ret, char **body);
int secgw_fire_event(char *uuid, char *body);

void secgw_message_callback(void *data);

void secgw_timer_callback(void *data);
int CheckExecCmdResult(const char* cmd);

int secgw_report_alarm(char *alarm_id, char *inst_id, char *alarm_param, char *alarm_type);
void secgw_ike_conn_alarm(ike_query_t *ikeQueryInfo);

#define SECGW_SOURCE_NAME "SECGW"
//#define SECGW_EVENT_NAME "secgw_event"
#define SECGW_HEADER_NAME "secgw_header"

#endif //INC_5GCORE_MOD_IPSEC_H
