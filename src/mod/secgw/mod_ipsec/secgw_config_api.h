//
// Created by count on 2020/6/18.
//

#ifndef INC_5GCORE_SECGW_CONFIG_API_H
#define INC_5GCORE_SECGW_CONFIG_API_H

#define ERROR_VERIFY_FAILURE -1
#define ERROR_SAVE_DB_FAILURE -2
#define ERROR_TAKE_EFFECT_FAILURE -3
#define ERROR_RECORD_NO_EXIST -4

typedef enum
{
    CONFIG_ADD = 0,
    CONFIG_MOD,
    CONFIG_RMV
}config_mode_t;

int config_add_conn(conn_query_t *conn_query);
int config_del_conn(int tableId);
int config_mod_conn(conn_query_t *conn_query);
int config_query_conns(int page_index, int page_size);
int config_add_secret(ipsec_secret_t *secret);
int config_del_secret(int tableId);
int config_mod_secret(ipsec_secret_t *secret);
int config_query_secrets(int page_index, int page_size);
int config_add_cert(ipsec_cert_t *cert);
int config_del_cert(int tableId);
int config_mod_cert(ipsec_cert_t *cert);
int config_query_certs(int page_index, int page_size);
int display_ipsec_conn(int page_index, int page_size);
int verify_conn_config(conn_query_t *conn_query, ipsec_conn_t *ipsec_conn, config_mode_t config_mod);

#endif //INC_5GCORE_SECGW_CONFIG_API_H
