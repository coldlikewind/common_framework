//
// Created by count on 2020/6/16.
//

#ifndef INC_5GCORE_SECGW_DB_API_H
#define INC_5GCORE_SECGW_DB_API_H

#define SECGW_DB_FIND 1
#define SECGW_DB_NOT_FIND 2

#define SECGW_MAX_QUERY_ROWS  50

#define IPSEC_CONN_TABLE_COLUMNS   22
#define IPSEC_SECRET_TABLE_COLUMNS 4
#define IPSEC_CERT_TABLE_COLUMNS   4
#define IKE_INFO_TABLE_COLUMNS     16

#define auth_size 2
#define action_size 2
#define send_cert_size 2
#define crypt_alg_size 8
#define ike_auth_alg_size 8
#define child_auth_alg_size 9
#define ike_dh_alg_size 14
#define child_dh_alg_size 15


extern int g_conn_rows;
extern int g_ike_info_rows;
extern int g_secret_rows;
extern int g_cert_rows;

extern int g_conn_total_rows;
extern int g_ike_info_total_rows;
extern int g_secret_total_rows;
extern int g_cert_total_rows;

extern char *enum_auth[];
extern char *enum_action[];
extern char *enum_send_cert[];
extern char *enum_crypt_alg[];
extern char *enum_ike_auth_alg[];
extern char *enum_child_auth_alg[];
extern char *enum_ike_dh_alg[];
extern char *enum_child_dh_alg[];

int secgw_db_api_init();
int secgw_db_api_deinit();
int save_conn_to_db(agc_db_t *db, conn_query_t *ipsec_conn);
int save_secret_to_db(agc_db_t *db, ipsec_secret_t *ipsec_secret);
void get_local_time(char *localTime);
int save_ike_info_to_db(agc_db_t *db, ike_query_t *ike_query);
int save_cert_to_db(agc_db_t *db, ipsec_cert_t *ipsec_cert);
int find_secret_from_db(agc_db_t *db, ipsec_secret_t *secret);
int find_secret_by_name(agc_db_t *db, ipsec_secret_t *secret);
int find_cert_from_db(agc_db_t *db, ipsec_cert_t *certId);
int find_conn_from_db(agc_db_t *db, conn_query_t *conn);
int find_conn_by_name(agc_db_t *db, char *conn_name);
int find_ike_info_from_db(agc_db_t *db, ike_query_t *ike_query);
int get_secret_by_id(agc_db_t *db, int tableId, ipsec_secret_t *secret);
int get_cert_by_id(agc_db_t *db, int tableId, ipsec_cert_t *certId);
int get_conn_by_id(agc_db_t *db, int tableId, conn_query_t *conn);
int delete_ike_info_from_db(agc_db_t *db, ike_query_t *ike_query);
int delete_conn_from_db(agc_db_t *db, conn_query_t *ipsec_conn);
int delete_secret_from_db(agc_db_t *db, ipsec_secret_t *ipsec_secret);
int delete_cert_from_db(agc_db_t *db, ipsec_cert_t *ipsec_cert);
int delete_conn_by_id(agc_db_t *db, int id);
int delete_secret_by_id(agc_db_t *db, int id);
int delete_cert_by_id(agc_db_t *db, int id);
void ike_info_combine(ike_query_t *new, ike_query_t *old);
int verify_ike_info_to_db(agc_db_t *db, ike_query_t *ike_query);
int get_conns_from_db(agc_db_t *db, int page_index, int page_size);
int get_ike_info_from_db(agc_db_t *db, int page_index, int page_size);
int get_secret_from_db(agc_db_t *db, int page_index, int page_size);
int get_cert_from_db(agc_db_t *db, int page_index, int page_size);
int conn_db_callback(void *pArg, int argc, char **argv, char **columnNames);
int ike_info_db_callback(void *pArg, int argc, char **argv, char **columnNames);
int secret_db_callback(void *pArg, int argc, char **argv, char **columnNames);
int cert_db_callback(void *pArg, int argc, char **argv, char **columnNames);
int find_cert_by_id(agc_db_t *db, int tableId);
int find_conn_by_id(agc_db_t *db, int tableId);
int find_secret_by_id(agc_db_t *db, int tableId);
int update_cert_to_db(agc_db_t *db, ipsec_cert_t *ipsec_cert);
int update_secret_to_db(agc_db_t *db, ipsec_secret_t *ipsec_secret);
int update_conn_to_db(agc_db_t *db, conn_query_t *ipsec_conn);
int get_total_conns(agc_db_t *db, int *rows);
int get_total_secrets(agc_db_t *db, int *rows);
int get_total_certs(agc_db_t *db, int *rows);
int get_total_ike_infos(agc_db_t *db, int *rows);

cJSON *encode_conns_to_json();
cJSON *encode_secret_to_json();
cJSON *encode_cert_to_json();
cJSON *encode_ike_info_to_json();

#endif //INC_5GCORE_SECGW_DB_API_H
