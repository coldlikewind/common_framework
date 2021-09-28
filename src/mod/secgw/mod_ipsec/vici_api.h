#ifndef VICILMT_H_
#define VICILMT_H_

#ifdef __cplusplus
extern "C" {
#endif
#define CONF_FILE_PATH "/usr/local/strongswan/etc/swanctl/conf.d/"
#define CERT_FILE_PATH "/usr/local/strongswan/etc/swanctl/x509/"
#define PRIVATE_KEY_FILE_PATH "/usr/local/strongswan/etc/swanctl/private/"
#define MAX_CHARS 100
#define SECRETS_LEN 500
#define SPI_LEN 8

typedef enum
{
    enum_cert,
    enum_private_key
}ENUM_CERT_TYPE;

typedef struct
{
	char *name;
	char *localAddrs;
	char *remoteAddrs;
	char *localAuth;
	char *certFile;
	char *certSend;
	char *ikeReauthTime;
	char *ikeRekeyTime;
	char *ipsecRekeyTime;
	char *dpdDelay;
	char *ipsecAction;
	char *localId;
	char *remoteAuth;
	char *remoteId;
	char *childrenEspProposals;
	char *localTs;
	char *remoteTs;
	char *mobike;
	char *version;
	char *proposals;
}ipsec_conn_t;

typedef struct
{
    int  tableId;
    char *connName;
    char *localId;
    char *localHost;
    char *localAuth;
    char *localTs;
    char *remoteId;
    char *remoteHost;
    char *remoteAuth;
    char *remoteTs;
    char *certSend;
    char *ikeReauthTime;
    char *ikeRekeyTime;
    char *ipsecRekeyTime;
    char *dpdDelay;
    char *ipsecAction;
    char *ikeCryptAlg;
    char *ikeAuthAlg;
    char *ikeDHAlg;
    char *ipsecCryptAlg;
    char *ipsecAuthAlg;
    char *ipsecDHAlg;
}conn_query_t;

typedef struct
{
    int  tableId;
    char *name;
    char *id;
    char *secret;
}ipsec_secret_t;

typedef struct
{
    int  tableId;
    char *id;
    int certType;
    char *certName;
}ipsec_cert_t;

typedef struct
{
    int  tableId;
	char *ikeName;
	char *ikeId;
	char *ikeState;
	char *localHost;
	char *remoteHost;
	char *childSaName;
	char *childSaId;
	char *childSaState;
	char *bytesIn;
	char *packetsIn;
	char *bytesOut;
	char *packetsOut;
	char *spiIn;
	char *spiOut;
	char *updateTime;
    int   isIkeSa;
    int   isChildSa;
    int   isOld;
}ike_query_t;

typedef void (*list_sa_cb_t)(ike_query_t *ike_query);

int vici_get_version(vici_conn_t *conn);
void malloc_conn(ipsec_conn_t *ipsec_conn);
void free_conn(ipsec_conn_t *ipsec_conn);
void malloc_conn_query(conn_query_t *conn_query);
void free_conn_query(conn_query_t *conn_query);
void malloc_ike_query(ike_query_t *ike_query);
void free_ike_query(ike_query_t *ike_query);
void malloc_secret(ipsec_secret_t *ipsec_secret);
void free_secret(ipsec_secret_t *ipsec_secret);
void malloc_cert(ipsec_cert_t *ipsec_cert);
void free_cert(ipsec_cert_t *ipsec_cert);
int add_conn(vici_conn_t *conn, ipsec_conn_t *ipsec_conn, char *success, char *errmsg);
int add_conn_file(ipsec_conn_t *ipsec_conn);
int add_cert_file(const char *name, const char *content, ENUM_CERT_TYPE cert_type);
int unload_conn(vici_conn_t *conn, char *conn_name, char *success, char *errmsg);
int delete_conn_file(char *conn_name);
int list_conn(vici_conn_t *conn, char *conn_name);
int list_sa(vici_conn_t *conn, ike_query_t *ike_query);
void print_sa(ike_query_t *ike_query);
int load_secret(vici_conn_t *conn, ipsec_secret_t *ipsec_secret, char *success, char *errmsg);
int unload_secret(vici_conn_t *conn, char *secret_id, char *success, char *errmsg);
int vici_load_cert(vici_conn_t *conn, FILE *pFile, char *success, char *errmsg);
int vici_load_key(vici_conn_t *conn, FILE *pFile, char *success, char *errmsg);
int create_secret_file(ipsec_secret_t *ipsec_secret);
int delete_secret_file(char *secret_name);
int delete_cert_file(ipsec_cert_t *cert);

void delete_special_char(char *dst, char *src);
void print_ike_query_info(ike_query_t *ikeQueryInfo);
int vici_parse_section_cb(void *null, vici_res_t *res, char *name);
int vici_parse_key_value_cb(void *null, vici_res_t *res, char *name, void *value, int len);
int vici_parse_list_item_cb(void *null, vici_res_t *res, char *name, void *value, int len);
//void parseIkeEvent(void *null, char *name, vici_res_t *res);
char* getCurrentTimeStr();

#ifdef __cplusplus
}
#endif

#endif

