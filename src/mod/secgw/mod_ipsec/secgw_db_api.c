//
// Created by count on 2020/6/16.
//
#include "time.h"
#include "agc_db.h"
#include "agc_json.h"
#include "libvici.h"
#include "vici_api.h"
#include "secgw_db_api.h"

conn_query_t *g_conn_query[SECGW_MAX_QUERY_ROWS];
ike_query_t *g_ike_query[SECGW_MAX_QUERY_ROWS];
ipsec_secret_t *g_secret_query[SECGW_MAX_QUERY_ROWS];
ipsec_cert_t *g_cert_query[SECGW_MAX_QUERY_ROWS];

int g_conn_rows = 0;
int g_ike_info_rows = 0;
int g_secret_rows = 0;
int g_cert_rows = 0;

int g_conn_total_rows = 0;
int g_ike_info_total_rows = 0;
int g_secret_total_rows = 0;
int g_cert_total_rows = 0;

char *enum_auth[] = {"pubkey","psk","eap-aka"};
char *enum_action[] = {"none","trap","start"};
char *enum_send_cert[] = {"never","always","ifasked"};
char *enum_crypt_alg[] = {"aes128","aes192","aes256","aes128ctr","aes192ctr","aes256ctr","aes128gcm128","aes192gcm128","aes256gcm128"};
char *enum_child_auth_alg[] = {"md5","md5_128","sha1","sha1_160","aesxcbc","aescmac","sha256","sha384","sha512","sha256_96"};
char *enum_ike_auth_alg[] = {"md5","md5_128","sha1","sha1_160","aesxcbc","aescmac","sha256","sha384","sha512"};
char *enum_child_dh_alg[] = {"none","modp2048","modp3072","modp4096","modp6144","modp8192","ecp224","ecp256","ecp384","ecp521","ecp224bp",
                       "ecp256bp","ecp384bp","ecp512bp","x25519","x448"};
char *enum_ike_dh_alg[] = {"modp2048","modp3072","modp4096","modp6144","modp8192","ecp224","ecp256","ecp384","ecp521","ecp224bp",
                       "ecp256bp","ecp384bp","ecp512bp","x25519","x448"};

int secgw_db_api_init()
{
    int i;
    for (i = 0; i < SECGW_MAX_QUERY_ROWS; i++)
    {
        g_conn_query[i] = (conn_query_t *) malloc(sizeof(conn_query_t));
        malloc_conn_query(g_conn_query[i]);
        g_ike_query[i] = (ike_query_t *)malloc(sizeof(ike_query_t));
        malloc_ike_query(g_ike_query[i]);
        g_secret_query[i] = (ipsec_secret_t *)malloc(sizeof(ipsec_secret_t));
        malloc_secret(g_secret_query[i]);
        g_cert_query[i] = (ipsec_cert_t *)malloc(sizeof(ipsec_cert_t));
        malloc_cert(g_cert_query[i]);
    }
}

int secgw_db_api_deinit()
{
    int i;
    for (i = 0; i < SECGW_MAX_QUERY_ROWS; i++)
    {
        free_conn_query(g_conn_query[i]);
        free(g_conn_query[i]);
        free_ike_query(g_ike_query[i]);
        free(g_ike_query[i]);
        free_secret(g_secret_query[i]);
        free(g_secret_query[i]);
        free_cert(g_cert_query[i]);
        free(g_cert_query[i]);
    }
}


int secgw_db_api_deinit();

void ike_info_combine(ike_query_t *new, ike_query_t *old)
{
    if (0 == strlen(new->childSaName))
    {
        strncpy(new->childSaName, old->childSaName, MAX_CHARS);
    }
    if (0 == strlen(new->childSaId))
    {
        strncpy(new->childSaId, old->childSaId, MAX_CHARS);
    }
    if (0 == strlen(new->childSaState))
    {
        strncpy(new->childSaState, old->childSaState, MAX_CHARS);
    }
    if (0 == strlen(new->spiOut))
    {
        strncpy(new->spiOut, old->spiOut, MAX_CHARS);
    }
    if (0 == strlen(new->spiIn))
    {
        strncpy(new->spiIn, old->spiIn, MAX_CHARS);
    }
    if (0 == strlen(new->bytesOut))
    {
        strncpy(new->bytesOut, old->bytesOut, MAX_CHARS);
    }
    if (0 == strlen(new->bytesIn))
    {
        strncpy(new->bytesIn, old->bytesIn, MAX_CHARS);
    }
    if (0 == strlen(new->packetsOut))
    {
        strncpy(new->packetsOut, old->packetsOut, MAX_CHARS);
    }
    if (0 == strlen(new->packetsIn))
    {
        strncpy(new->packetsIn, old->packetsIn, MAX_CHARS);
    }
}

int verify_ike_info_to_db(agc_db_t *db, ike_query_t *ike_query)
{
    int result = AGC_STATUS_SUCCESS;
    ike_query_t local_record;

    malloc_ike_query(&local_record);
    strncpy(local_record.ikeName, ike_query->ikeName, MAX_CHARS);
    strncpy(local_record.localHost, ike_query->localHost, MAX_CHARS);
    strncpy(local_record.remoteHost, ike_query->remoteHost, MAX_CHARS);
    if (SECGW_DB_FIND == find_ike_info_from_db(db, &local_record))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "find ike query, copy and delete\n");
        ike_info_combine(ike_query, &local_record);
        if (AGC_STATUS_SUCCESS != delete_ike_info_from_db(db, ike_query))
        {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ike query delete failure\n");
            result = AGC_STATUS_FALSE;
        }
    }

    free_ike_query(&local_record);
    return result;
}

void get_local_time(char *localTime)
{
    time_t timep;
    struct tm *p;

    time(&timep);
    p=gmtime(&timep);
    sprintf(localTime, "%d-%d-%d %d:%d:%d",1900+p->tm_year,1+p->tm_mon,p->tm_mday,8+p->tm_hour,p->tm_min,p->tm_sec);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "current time: %s\n", localTime);
}

int save_ike_info_to_db(agc_db_t *db, ike_query_t *ike_query)
{
    char *err_msg = NULL;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(2048);
    get_local_time(ike_query->updateTime);
    sprintf(buf, "INSERT INTO ike_infos(IkeName,IkeId,IkeState,LocalHost,RemoteHost,ChildSaName,ChildSaId,ChildSaState,"
                 "SpiIn,SpiOut,BytesIn,PacketsIn,BytesOut,PacketsOut,UpdateTime)"
                 "VALUES('%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s')",
            ike_query->ikeName,ike_query->ikeId,ike_query->ikeState,ike_query->localHost,ike_query->remoteHost,ike_query->childSaName,
            ike_query->childSaId,ike_query->childSaState,ike_query->spiIn,ike_query->spiOut,ike_query->bytesIn,ike_query->packetsIn,
            ike_query->bytesOut,ike_query->packetsOut,ike_query->updateTime);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ike save sql %s\n", buf);
    if (agc_db_exec(db, buf, NULL, NULL, &err_msg) != AGC_DB_OK)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_GENERR;
    }
    free(buf);
    agc_db_free(err_msg);
    return result;
}

int delete_ike_info_from_db(agc_db_t *db, ike_query_t *ike_query)
{
    char *err_msg = NULL;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "DELETE FROM ike_infos where  IkeName='%s' and LocalHost='%s' and RemoteHost='%s'",
            ike_query->ikeName, ike_query->localHost, ike_query->remoteHost);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ike delete sql %s\n", buf);
    if (agc_db_exec(db, buf, NULL, NULL, &err_msg) != AGC_DB_OK)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_GENERR;
    }
    free(buf);
    agc_db_free(err_msg);
    return result;
}

int save_conn_to_db(agc_db_t *db, conn_query_t *ipsec_conn)
{
    char *err_msg = NULL;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(2048);
    sprintf(buf, "INSERT INTO ike_conns(ConnName,LocalHost,LocalId,LocalAuth,CertSend,LocalTs,RemoteHost,RemoteId,RemoteAuth,RemoteTs,"
                 "IkeReauthTime,IkeRekeyTime,IpsecRekeyTime,DpdDelay,IpsecAction,IkeCryptAlg,IkeAuthAlg,IkeDHAlg,IpsecCryptAlg,IpsecAuthAlg,IpsecDHAlg) "
                 "VALUES('%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s')",
            ipsec_conn->connName,ipsec_conn->localHost,ipsec_conn->localId,ipsec_conn->localAuth,ipsec_conn->certSend,ipsec_conn->localTs,
            ipsec_conn->remoteHost,ipsec_conn->remoteId,ipsec_conn->remoteAuth,ipsec_conn->remoteTs,ipsec_conn->ikeReauthTime,ipsec_conn->ikeRekeyTime,
            ipsec_conn->ipsecRekeyTime, ipsec_conn->dpdDelay, ipsec_conn->ipsecAction,ipsec_conn->ikeCryptAlg,ipsec_conn->ikeAuthAlg,ipsec_conn->ikeDHAlg,
            ipsec_conn->ipsecCryptAlg,ipsec_conn->ipsecAuthAlg,ipsec_conn->ipsecDHAlg);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "conn save sql %s \n", buf);
    if (agc_db_exec(db, buf, NULL, NULL, &err_msg) != AGC_DB_OK)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_GENERR;
    }
    free(buf);
    agc_db_free(err_msg);
    return result;
}

int delete_conn_from_db(agc_db_t *db, conn_query_t *ipsec_conn)
{
    char *err_msg = NULL;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "DELETE FROM ike_conns where ConnName='%s'",ipsec_conn->connName);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "conn delete sql %s", buf);
    if (agc_db_exec(db, buf, NULL, NULL, &err_msg) != AGC_DB_OK)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_GENERR;
    }
    free(buf);
    agc_db_free(err_msg);
    return result;
}

int save_secret_to_db(agc_db_t *db, ipsec_secret_t *ipsec_secret)
{
    char *err_msg = NULL;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "INSERT INTO ike_secrets(SecretName,SecretId,SecretData) VALUES('%s','%s','%s')",
            ipsec_secret->name,ipsec_secret->id,ipsec_secret->secret);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "secret save sql %s", buf);
    if (agc_db_exec(db, buf, NULL, NULL, &err_msg) != AGC_DB_OK)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_GENERR;
    }
    free(buf);
    agc_db_free(err_msg);
    return result;
}

int delete_secret_from_db(agc_db_t *db, ipsec_secret_t *ipsec_secret)
{
    char *err_msg = NULL;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "DELETE FROM ike_secrets WHERE SecretId='%s'",ipsec_secret->id);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "secret del sql %s", buf);
    if (agc_db_exec(db, buf, NULL, NULL, &err_msg) != AGC_DB_OK)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_GENERR;
    }
    free(buf);
    agc_db_free(err_msg);
    return result;
}

int save_cert_to_db(agc_db_t *db, ipsec_cert_t *ipsec_cert)
{
    char *err_msg = NULL;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "INSERT INTO ike_certs(CertId,CertType,CertName) VALUES('%s','%d','%s')",
            ipsec_cert->id, ipsec_cert->certType, ipsec_cert->certName);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "cert save sql %s", buf);
    if (agc_db_exec(db, buf, NULL, NULL, &err_msg) != AGC_DB_OK)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_GENERR;
    }
    free(buf);
    agc_db_free(err_msg);
    return result;
}

int delete_cert_from_db(agc_db_t *db, ipsec_cert_t *ipsec_cert)
{
    char *err_msg = NULL;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "DELETE FROM ike_certs WHERE CertId='%s'",ipsec_cert->id);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "cert del sql %s", buf);
    if (agc_db_exec(db, buf, NULL, NULL, &err_msg) != AGC_DB_OK)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_GENERR;
    }
    free(buf);
    agc_db_free(err_msg);
    return result;
}

int delete_cert_by_id(agc_db_t *db, int id)
{
    char *err_msg = NULL;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "DELETE FROM ike_certs WHERE ID='%d'", id);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "cert del sql %s", buf);
    if (agc_db_exec(db, buf, NULL, NULL, &err_msg) != AGC_DB_OK)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_GENERR;
    }
    free(buf);
    agc_db_free(err_msg);
    return result;
}

int delete_secret_by_id(agc_db_t *db, int id)
{
    char *err_msg = NULL;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "enter delete secret");

    buf = (char *)malloc(512);
    sprintf(buf, "DELETE FROM ike_secrets WHERE ID='%d'", id);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "secret del sql %s", buf);
    if (agc_db_exec(db, buf, NULL, NULL, &err_msg) != AGC_DB_OK)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_GENERR;
    }
    free(buf);
    agc_db_free(err_msg);
    return result;
}

int delete_conn_by_id(agc_db_t *db, int id)
{
    char *err_msg = NULL;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "DELETE FROM ike_conns WHERE ID='%d'", id);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "conn del sql %s", buf);
    if (agc_db_exec(db, buf, NULL, NULL, &err_msg) != AGC_DB_OK)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_GENERR;
    }
    free(buf);
    agc_db_free(err_msg);
    return result;
}


int find_secret_from_db(agc_db_t *db, ipsec_secret_t *secret)
{
    char *err_msg = NULL;
    char **query_result = NULL;
    int nrow;
    int ncolumn;
    char *sql;
    int result = SECGW_DB_NOT_FIND;

    sql = (char *)malloc(512);
    sprintf(sql, "select * from ike_secrets where ike_secrets.SecretId='%s'", secret->id);
    if (agc_db_get_table(db, sql, &query_result, &nrow, &ncolumn, &err_msg) == AGC_DB_OK)
    {
        if (nrow > 0)
        {
            result = SECGW_DB_FIND;
        }
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
    }
    agc_db_free(err_msg);
    agc_db_free_table(query_result);
    free(sql);
    return result;
}

int find_secret_by_name(agc_db_t *db, ipsec_secret_t *secret)
{
    char *err_msg = NULL;
    char **query_result = NULL;
    int nrow;
    int ncolumn;
    char *sql;
    int result = SECGW_DB_NOT_FIND;

    sql = (char *)malloc(512);
    sprintf(sql, "select * from ike_secrets where ike_secrets.SecretName='%s'", secret->name);
    if (agc_db_get_table(db, sql, &query_result, &nrow, &ncolumn, &err_msg) == AGC_DB_OK)
    {
        if (nrow > 0)
        {
            result = SECGW_DB_FIND;
        }
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
    }
    agc_db_free(err_msg);
    agc_db_free_table(query_result);
    free(sql);
    return result;
}


int find_cert_from_db(agc_db_t *db, ipsec_cert_t *cert)
{
    char *err_msg = NULL;
    char **query_result = NULL;
    int nrow;
    int ncolumn;
    char *sql;
    int result = SECGW_DB_NOT_FIND;

    sql = (char *)malloc(512);
    sprintf(sql, "select * from ike_certs where ike_certs.CertId='%s'", cert->id);
    if (agc_db_get_table(db, sql, &query_result, &nrow, &ncolumn, &err_msg) == AGC_DB_OK)
    {
        if (nrow > 0)
        {
            strncpy(cert->certName , query_result[ncolumn + 2], MAX_CHARS);
            result = SECGW_DB_FIND;
        }
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
    }
    agc_db_free(err_msg);
    agc_db_free_table(query_result);
    free(sql);
    return result;
}

int find_conn_from_db(agc_db_t *db, conn_query_t *conn)
{
    char *err_msg = NULL;
    char **query_result = NULL;
    int nrow;
    int ncolumn;
    char *sql;
    int result = SECGW_DB_NOT_FIND;

    sql = (char *)malloc(512);
    sprintf(sql, "select * from ike_conns where ike_conns.ConnName='%s'", conn->connName);
    if (agc_db_get_table(db, sql, &query_result, &nrow, &ncolumn, &err_msg) == AGC_DB_OK)
    {
        if (nrow > 0)
        {
            result = SECGW_DB_FIND;
        }
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
    }
    agc_db_free(err_msg);
    agc_db_free_table(query_result);
    free(sql);
    return result;
}

int find_conn_by_name(agc_db_t *db, char *conn_name)
{
    char *err_msg = NULL;
    char **query_result = NULL;
    int nrow;
    int ncolumn;
    char *sql;
    int result = SECGW_DB_NOT_FIND;

    sql = (char *)malloc(512);
    sprintf(sql, "select * from ike_conns where ike_conns.ConnName='%s'", conn_name);
    if (agc_db_get_table(db, sql, &query_result, &nrow, &ncolumn, &err_msg) == AGC_DB_OK)
    {
        if (nrow > 0)
        {
            result = SECGW_DB_FIND;
        }
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
    }
    agc_db_free(err_msg);
    agc_db_free_table(query_result);
    free(sql);
    return result;
}

int find_ike_info_from_db(agc_db_t *db, ike_query_t *ike_query)
{
    char *err_msg = NULL;
    char **query_result = NULL;
    int nrow;
    int ncolumn;
    char *sql;
    int result = SECGW_DB_NOT_FIND;

    sql = (char *)malloc(512);
    sprintf(sql, "select * from ike_infos where IkeName='%s' and LocalHost='%s' and RemoteHost='%s'",
            ike_query->ikeName, ike_query->localHost, ike_query->remoteHost);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ike info sql: %s\n", sql);
    if (agc_db_get_table(db, sql, &query_result, &nrow, &ncolumn, &err_msg) == AGC_DB_OK)
    {
        if (nrow > 0)
        {
            ike_info_db_callback(ike_query, ncolumn, query_result+ncolumn, query_result);
            result = SECGW_DB_FIND;
        }
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
    }
    agc_db_free(err_msg);
    agc_db_free_table(query_result);
    free(sql);
    return result;
}

int get_conns_from_db(agc_db_t *db, int page_index, int page_size)
{
    char *err_msg = NULL;
    char **query_result = NULL;
    int i, nrow, ncolumn;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "SELECT * FROM ike_conns LIMIT %d OFFSET %d", page_size, page_index);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "conn query sql %s", buf);
    if (agc_db_get_table(db, buf, &query_result, &nrow, &ncolumn, &err_msg) == AGC_DB_OK)
    {
        g_conn_rows = nrow;
        for(i = 1; i < nrow+1; i++)
        {
            conn_db_callback(g_conn_query[i-1], ncolumn, (query_result + i*ncolumn), query_result);
        }
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_GENERR;
    }
    free(buf);
    agc_db_free(err_msg);
    agc_db_free_table(query_result);
    return result;
}

int get_ike_info_from_db(agc_db_t *db, int page_index, int page_size)
{
    char *err_msg = NULL;
    char *buf;
    char **query_result = NULL;
    int i, nrow, ncolumn;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "SELECT * FROM ike_infos LIMIT %d OFFSET %d", page_size, page_index);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ike info query sql %s\n", buf);
    if (agc_db_get_table(db, buf, &query_result, &nrow, &ncolumn, &err_msg) == AGC_DB_OK)
    {
        g_ike_info_rows = nrow;
        for(i = 1; i < nrow+1; i++)
        {
            ike_info_db_callback(g_ike_query[i-1], ncolumn, (query_result + i*ncolumn), query_result);
        }
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_GENERR;
    }
    free(buf);
    agc_db_free(err_msg);
    agc_db_free_table(query_result);
    return result;
}

int get_secret_from_db(agc_db_t *db, int page_index, int page_size)
{
    char *err_msg = NULL;
    char **query_result = NULL;
    int i, nrow, ncolumn;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "SELECT * FROM ike_secrets LIMIT %d OFFSET %d", page_size, page_index);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "secret info query sql %s .\n", buf);
    if (agc_db_get_table(db, buf, &query_result, &nrow, &ncolumn, &err_msg) == AGC_DB_OK)
    {
        g_secret_rows = nrow;
        for(i = 1; i < nrow+1; i++)
        {
            secret_db_callback(g_secret_query[i-1], ncolumn, (query_result + i*ncolumn), query_result);
        }
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_GENERR;
    }
    free(buf);
    agc_db_free(err_msg);
    agc_db_free_table(query_result);
    return result;
}

int get_cert_from_db(agc_db_t *db, int page_index, int page_size)
{
    char *err_msg = NULL;
    char **query_result = NULL;
    int i, nrow, ncolumn;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "SELECT * FROM ike_certs LIMIT %d OFFSET %d", page_size, page_index);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "cert info query sql %s", buf);
    if (agc_db_get_table(db, buf, &query_result, &nrow, &ncolumn, &err_msg) == AGC_DB_OK)
    {
        g_cert_rows = nrow;
        for(i = 1; i < nrow+1; i++)
        {
            cert_db_callback(g_cert_query[i-1], ncolumn, (query_result + i*ncolumn), query_result);
        }
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_GENERR;
    }
    free(buf);
    agc_db_free(err_msg);
    agc_db_free_table(query_result);
    return result;
}

int get_cert_by_id(agc_db_t *db, int tableId, ipsec_cert_t *cert)
{
    char *err_msg = NULL;
    char **query_result = NULL;
    int i, nrow, ncolumn;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "SELECT * FROM ike_certs WHERE id=%d", tableId);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "cert info query sql %s\n", buf);
    if (agc_db_get_table(db, buf, &query_result, &nrow, &ncolumn, &err_msg) == AGC_DB_OK)
    {
        if ( nrow > 0 )
        {
            cert_db_callback(cert, ncolumn, (query_result + ncolumn), query_result);
        }
        else
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "agc_db_exec no found.\n");
            result = AGC_STATUS_FALSE;
        }
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_FALSE;
    }
    free(buf);
    agc_db_free(err_msg);
    agc_db_free_table(query_result);
    return result;
}

int get_secret_by_id(agc_db_t *db, int tableId, ipsec_secret_t *secret)
{
    char *err_msg = NULL;
    char **query_result = NULL;
    int i, nrow, ncolumn;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "SELECT * FROM ike_secrets WHERE id=%d", tableId);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "secret info query sql %s", buf);
    if (agc_db_get_table(db, buf, &query_result, &nrow, &ncolumn, &err_msg) == AGC_DB_OK)
    {
        if ( nrow > 0)
        {
            secret_db_callback(secret, ncolumn, (query_result + ncolumn), query_result);
        }
        else
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "agc_db_exec no found.\n");
            result = AGC_STATUS_GENERR;
        }
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_GENERR;
    }
    free(buf);
    agc_db_free(err_msg);
    agc_db_free_table(query_result);
    return result;
}

int get_conn_by_id(agc_db_t *db, int tableId, conn_query_t *conn)
{
    char *err_msg = NULL;
    char **query_result = NULL;
    int i, nrow, ncolumn;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "SELECT * FROM ike_conns WHERE id=%d", tableId);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "connect info query sql %s", buf);
    if (agc_db_get_table(db, buf, &query_result, &nrow, &ncolumn, &err_msg) == AGC_DB_OK)
    {
        if (nrow > 0)
        {
            conn_db_callback(conn, ncolumn, (query_result + ncolumn), query_result);
        }
        else
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "agc_db_exec no found.\n");
            result = AGC_STATUS_GENERR;
        }
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_GENERR;
    }
    free(buf);
    agc_db_free(err_msg);
    agc_db_free_table(query_result);
    return result;
}

int conn_db_callback(void *pArg, int argc, char **argv, char **columnNames)
{
    int i;
    conn_query_t *conn_query;

    if (columnNames != NULL)
    {
        for (i = 0; i < argc; i++) {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "conn column %s value %s .\n", columnNames[i], argv[i]);
        }
    }

    if (argc == IPSEC_CONN_TABLE_COLUMNS && argv != NULL)
    {
        conn_query = (conn_query_t *) pArg;
        conn_query->tableId = atoi(argv[0]);
        strncpy(conn_query->connName, argv[1], MAX_CHARS);
        strncpy(conn_query->localHost, argv[2], MAX_CHARS);
        strncpy(conn_query->localId, argv[3], MAX_CHARS);
        strncpy(conn_query->localAuth, argv[4], MAX_CHARS);
        strncpy(conn_query->certSend, argv[5], MAX_CHARS);
        if (0 != strncmp(argv[6], "null", MAX_CHARS))
        {
            strncpy(conn_query->localTs, argv[6], MAX_CHARS);
        }
        else
        {
            conn_query->localTs[0] = '\0';
        }
        strncpy(conn_query->ipsecAction, argv[7], MAX_CHARS);
        strncpy(conn_query->remoteHost, argv[8], MAX_CHARS);
        strncpy(conn_query->remoteId, argv[9], MAX_CHARS);
        strncpy(conn_query->remoteAuth, argv[10], MAX_CHARS);
        if (0 != strncmp(argv[11], "null", MAX_CHARS))
        {
            strncpy(conn_query->remoteTs, argv[11], MAX_CHARS);
        }
        else
        {
            conn_query->remoteTs[0] = '\0';
        }
        strncpy(conn_query->ikeReauthTime, argv[12], MAX_CHARS);
        strncpy(conn_query->ikeRekeyTime, argv[13], MAX_CHARS);
        strncpy(conn_query->ipsecRekeyTime, argv[14], MAX_CHARS);
        strncpy(conn_query->dpdDelay, argv[15], MAX_CHARS);
        strncpy(conn_query->ikeCryptAlg, argv[16], MAX_CHARS);
        strncpy(conn_query->ikeAuthAlg, argv[17], MAX_CHARS);
        strncpy(conn_query->ikeDHAlg, argv[18], MAX_CHARS);
        strncpy(conn_query->ipsecCryptAlg, argv[19], MAX_CHARS);
        strncpy(conn_query->ipsecAuthAlg, argv[20], MAX_CHARS);
        strncpy(conn_query->ipsecDHAlg, argv[21], MAX_CHARS);
    }

    return AGC_STATUS_SUCCESS;
}

int ike_info_db_callback(void *pArg, int argc, char **argv, char **columnNames)
{
    int i;
    ike_query_t *ike_query;

    if (columnNames != NULL)
    {
        for (i = 0; i < argc; i++)
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ike info column %s value %s .\n", columnNames[i], argv[i]);
        }
    }

    if (argc == IKE_INFO_TABLE_COLUMNS && argv != NULL)
    {
        ike_query = (ike_query_t *)pArg;
        ike_query->tableId = atoi(argv[0]);
        strncpy(ike_query->ikeName, argv[1], MAX_CHARS);
        strncpy(ike_query->ikeId, argv[2], MAX_CHARS);
        strncpy(ike_query->ikeState, argv[3], MAX_CHARS);
        strncpy(ike_query->localHost, argv[4], MAX_CHARS);
        strncpy(ike_query->remoteHost, argv[5], MAX_CHARS);
        strncpy(ike_query->childSaName, argv[6], MAX_CHARS);
        strncpy(ike_query->childSaId, argv[7], MAX_CHARS);
        strncpy(ike_query->childSaState, argv[8], MAX_CHARS);
        strncpy(ike_query->spiIn, argv[9], MAX_CHARS);
        strncpy(ike_query->spiOut, argv[10], MAX_CHARS);
        strncpy(ike_query->bytesIn, argv[11], MAX_CHARS);
        strncpy(ike_query->packetsIn, argv[12], MAX_CHARS);
        strncpy(ike_query->bytesOut, argv[13], MAX_CHARS);
        strncpy(ike_query->packetsOut, argv[14], MAX_CHARS);
        strncpy(ike_query->updateTime, argv[15], MAX_CHARS);
    }

    return AGC_DB_OK;
}

int secret_db_callback(void *pArg, int argc, char **argv, char **columnNames)
{
    int i;
    ipsec_secret_t *secret;

    if (columnNames != NULL)
    {
        for (i = 0; i < argc; i++)
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "secret column %s value %s .\n", columnNames[i], argv[i]);
        }
    }

    if (argc == IPSEC_SECRET_TABLE_COLUMNS  && argv != NULL)
    {
        secret = (ipsec_secret_t *) pArg;
        secret->tableId = atoi(argv[0]);
        strncpy(secret->name, argv[1], MAX_CHARS);
        strncpy(secret->id, argv[2], MAX_CHARS);
        strncpy(secret->secret, argv[3], SECRETS_LEN);
    }

    return AGC_DB_OK;
}

int cert_db_callback(void *pArg, int argc, char **argv, char **columnNames)
{
    int i;
    ipsec_cert_t *cert;

    if (columnNames != NULL)
    {
        for (i = 0; i < argc; i++)
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "cert column %s value %s .\n", columnNames[i], argv[i]);
        }
    }

    if (argc == IPSEC_CERT_TABLE_COLUMNS  && argv != NULL)
    {
        cert = (ipsec_cert_t *) pArg;
        cert->tableId = atoi(argv[0]);
        strncpy(cert->id, argv[1], MAX_CHARS);
        strncpy(cert->certName, argv[2], MAX_CHARS);
        cert->certType = atoi(argv[3]);
    }

    return AGC_DB_OK;
}

cJSON *encode_conns_to_json()
{
    int i;
    cJSON_Hooks json_hooks;
    cJSON *record_array, *record_item;
    conn_query_t *conn_query;

    json_hooks.malloc_fn = malloc;
    json_hooks.free_fn = free;
    cJSON_InitHooks(&json_hooks);

    record_array = cJSON_CreateArray();

    for (i = 0; i < g_conn_rows; i++)
    {
        conn_query = g_conn_query[i];
        record_item = cJSON_CreateObject();
        cJSON_AddNumberToObject(record_item, "ID",conn_query->tableId);
        cJSON_AddStringToObject(record_item, "ConnName",conn_query->connName);
        cJSON_AddStringToObject(record_item, "LocalHost",conn_query->localHost);
        cJSON_AddStringToObject(record_item, "LocalId",conn_query->localId);
        cJSON_AddNumberToObject(record_item, "LocalAuth",atoi(conn_query->localAuth));
        cJSON_AddNumberToObject(record_item, "CertSend",atoi(conn_query->certSend));
        cJSON_AddStringToObject(record_item, "LocalTs",conn_query->localTs);
        cJSON_AddNumberToObject(record_item, "IpsecAction",atoi(conn_query->ipsecAction));
        cJSON_AddStringToObject(record_item, "RemoteHost",conn_query->remoteHost);
        cJSON_AddStringToObject(record_item, "RemoteId",conn_query->remoteId);
        cJSON_AddNumberToObject(record_item, "RemoteAuth",atoi(conn_query->remoteAuth));
        cJSON_AddStringToObject(record_item, "RemoteTs",conn_query->remoteTs);
        cJSON_AddNumberToObject(record_item, "IkeReauthTime",atoi(conn_query->ikeReauthTime));
        cJSON_AddNumberToObject(record_item, "IkeRekeyTime",atoi(conn_query->ikeRekeyTime));
        cJSON_AddNumberToObject(record_item, "IpsecRekeyTime",atoi(conn_query->ipsecRekeyTime));
        cJSON_AddNumberToObject(record_item, "DpdDelay",atoi(conn_query->dpdDelay));
        cJSON_AddNumberToObject(record_item, "IkeCryptAlg",atoi(conn_query->ikeCryptAlg));
        cJSON_AddNumberToObject(record_item, "IkeAuthAlg",atoi(conn_query->ikeAuthAlg));
        cJSON_AddNumberToObject(record_item, "ikeDHAlg",atoi(conn_query->ikeDHAlg));
        cJSON_AddNumberToObject(record_item, "IpsecCryptAlg",atoi(conn_query->ipsecCryptAlg));
        cJSON_AddNumberToObject(record_item, "IpsecAuthAlg",atoi(conn_query->ipsecAuthAlg));
        cJSON_AddNumberToObject(record_item, "IpsecDHAlg",atoi(conn_query->ipsecDHAlg));
        cJSON_AddItemToArray(record_array, record_item);
    }
    return record_array;
}

cJSON *encode_ike_info_to_json()
{
    int i;
    cJSON_Hooks json_hooks;
    cJSON *record_array, *record_item;
    ike_query_t *ike_query;

    json_hooks.malloc_fn = malloc;
    json_hooks.free_fn = free;
    cJSON_InitHooks(&json_hooks);

    record_array = cJSON_CreateArray();

    for (i = 0; i < g_ike_info_rows; i++)
    {
        ike_query = g_ike_query[i];
        record_item = cJSON_CreateObject();
        cJSON_AddNumberToObject(record_item, "ID",ike_query->tableId);
        cJSON_AddStringToObject(record_item, "IkeName",ike_query->ikeName);
        cJSON_AddStringToObject(record_item, "IkeId",ike_query->ikeId);
        cJSON_AddStringToObject(record_item, "IkeState",ike_query->ikeState);
        cJSON_AddStringToObject(record_item, "LocalHost",ike_query->localHost);
        cJSON_AddStringToObject(record_item, "RemoteHost",ike_query->remoteHost);
        cJSON_AddStringToObject(record_item, "ChildSaName",ike_query->childSaName);
        cJSON_AddStringToObject(record_item, "ChildSaId",ike_query->childSaId);
        cJSON_AddStringToObject(record_item, "ChildSaState",ike_query->childSaState);
        cJSON_AddStringToObject(record_item, "SpiIn",ike_query->spiIn);
        cJSON_AddStringToObject(record_item, "SpiOut",ike_query->spiOut);
        cJSON_AddStringToObject(record_item, "BytesIn",ike_query->bytesIn);
        cJSON_AddStringToObject(record_item, "PacketsIn",ike_query->packetsIn);
        cJSON_AddStringToObject(record_item, "BytesOut",ike_query->bytesOut);
        cJSON_AddStringToObject(record_item, "PacketsOut",ike_query->packetsOut);
        cJSON_AddStringToObject(record_item, "UpdateTime",ike_query->updateTime);
        cJSON_AddItemToArray(record_array, record_item);
    }
    return record_array;
}

cJSON *encode_secret_to_json()
{
    int i;
    cJSON_Hooks json_hooks;
    cJSON *record_array, *record_item;
    ipsec_secret_t *secret;

    json_hooks.malloc_fn = malloc;
    json_hooks.free_fn = free;
    cJSON_InitHooks(&json_hooks);

    //global_object = cJSON_CreateObject();
    record_array = cJSON_CreateArray();

    for (i = 0; i < g_secret_rows; i++)
    {
        secret = g_secret_query[i];
        record_item = cJSON_CreateObject();
        cJSON_AddNumberToObject(record_item, "ID",secret->tableId);
        cJSON_AddStringToObject(record_item, "SecretName",secret->name);
        cJSON_AddStringToObject(record_item, "SecretId",secret->id);
        cJSON_AddStringToObject(record_item, "SecretData",secret->secret);
        cJSON_AddItemToArray(record_array, record_item);
    }

    return record_array;
}

cJSON *encode_cert_to_json()
{
    int i;
    cJSON_Hooks json_hooks;
    cJSON *record_array, *record_item;
    ipsec_cert_t *cert;

    json_hooks.malloc_fn = malloc;
    json_hooks.free_fn = free;
    cJSON_InitHooks(&json_hooks);

    record_array = cJSON_CreateArray();

    for (i = 0; i < g_cert_rows; i++)
    {
        cert = g_cert_query[i];
        record_item = cJSON_CreateObject();
        cJSON_AddNumberToObject(record_item, "ID",cert->tableId);
        cJSON_AddStringToObject(record_item, "CertId",cert->id);
        cJSON_AddNumberToObject(record_item, "CertType", cert->certType);
        cJSON_AddStringToObject(record_item, "CertName",cert->certName);
        cJSON_AddItemToArray(record_array, record_item);
    }

    return record_array;
}

int find_secret_by_id(agc_db_t *db, int tableId)
{
    char *err_msg = NULL;
    char **query_result = NULL;
    int nrow, ncolumn;
    char *buf;
    int result = SECGW_DB_NOT_FIND;

    buf = (char *)malloc(512);
    sprintf(buf, "SELECT * FROM ike_secrets WHERE id=%d", tableId);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "secret info query sql %s", buf);
    if (agc_db_get_table(db, buf, &query_result, &nrow, &ncolumn, &err_msg) == AGC_DB_OK)
    {
        if ( nrow > 0)
        {
            result = SECGW_DB_FIND;
        }
        else
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "agc_db_exec no found.\n");
        }
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
    }
    free(buf);
    agc_db_free(err_msg);
    agc_db_free_table(query_result);
    return result;
}

int find_conn_by_id(agc_db_t *db, int tableId)
{
    char *err_msg = NULL;
    char **query_result = NULL;
    int nrow, ncolumn;
    char *buf;
    int result = SECGW_DB_NOT_FIND;

    buf = (char *)malloc(512);
    sprintf(buf, "SELECT * FROM ike_conns WHERE id=%d\n", tableId);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "connect info query sql %s\n", buf);
    if (agc_db_get_table(db, buf, &query_result, &nrow, &ncolumn, &err_msg) == AGC_DB_OK)
    {
        if (nrow > 0)
        {
            result = SECGW_DB_FIND;
        }
        else
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "agc_db_exec no found.\n");
        }
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
    }
    free(buf);
    agc_db_free(err_msg);
    agc_db_free_table(query_result);
    return result;
}

int find_cert_by_id(agc_db_t *db, int tableId)
{
    char *err_msg = NULL;
    char **query_result = NULL;
    int nrow, ncolumn;
    char *buf;
    int result = SECGW_DB_NOT_FIND;

    buf = (char *)malloc(512);
    sprintf(buf, "SELECT * FROM ike_certs WHERE id=%d", tableId);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "cert info query sql %s", buf);
    if (agc_db_get_table(db, buf, &query_result, &nrow, &ncolumn, &err_msg) == AGC_DB_OK)
    {
        if ( nrow > 0 )
        {
            result = SECGW_DB_FIND;
        }
        else
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "agc_db_exec no found.\n");
        }
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
    }
    free(buf);
    agc_db_free(err_msg);
    agc_db_free_table(query_result);
    return result;
}

int update_conn_to_db(agc_db_t *db, conn_query_t *ipsec_conn)
{
    char *err_msg = NULL;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(2048);
    sprintf(buf, "UPDATE ike_conns SET ConnName='%s',LocalHost='%s',LocalId='%s',LocalAuth='%s',CertSend='%s',LocalTs='%s',RemoteHost='%s',"
                 "RemoteId='%s',RemoteAuth='%s',RemoteTs='%s',IkeReauthTime='%s',IkeRekeyTime='%s',IpsecRekeyTime='%s',DpdDelay='%s',IpsecAction='%s',"
                 "IkeCryptAlg='%s',IkeAuthAlg='%s',IkeDHAlg='%s',IpsecCryptAlg='%s',IpsecAuthAlg='%s',IpsecDHAlg='%s' WHERE ID='%d'",
            ipsec_conn->connName,ipsec_conn->localHost,ipsec_conn->localId,ipsec_conn->localAuth,ipsec_conn->certSend,ipsec_conn->localTs,
            ipsec_conn->remoteHost,ipsec_conn->remoteId,ipsec_conn->remoteAuth,ipsec_conn->remoteTs,ipsec_conn->ikeReauthTime,ipsec_conn->ikeRekeyTime,
            ipsec_conn->ipsecRekeyTime, ipsec_conn->dpdDelay, ipsec_conn->ipsecAction,ipsec_conn->ikeCryptAlg,ipsec_conn->ikeAuthAlg,ipsec_conn->ikeDHAlg,
            ipsec_conn->ipsecCryptAlg,ipsec_conn->ipsecAuthAlg,ipsec_conn->ipsecDHAlg,ipsec_conn->tableId);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "conn update sql %s \n", buf);
    if (agc_db_exec(db, buf, NULL, NULL, &err_msg) != AGC_DB_OK)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_FALSE;
    }
    free(buf);
    agc_db_free(err_msg);
    return result;
}

int update_secret_to_db(agc_db_t *db, ipsec_secret_t *ipsec_secret)
{
    char *err_msg = NULL;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "UPDATE ike_secrets SET SecretName='%s',SecretId='%s',SecretData='%s' WHERE ID='%d'",
            ipsec_secret->name,ipsec_secret->id,ipsec_secret->secret,ipsec_secret->tableId);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "secret update sql %s", buf);
    if (agc_db_exec(db, buf, NULL, NULL, &err_msg) != AGC_DB_OK)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_FALSE;
    }
    free(buf);
    agc_db_free(err_msg);
    return result;
}

int update_cert_to_db(agc_db_t *db, ipsec_cert_t *ipsec_cert)
{
    char *err_msg = NULL;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "UPDATE ike_certs SET CertId='%s',CertType='%d',CertName='%s' WHERE ID='%d'",
            ipsec_cert->id,ipsec_cert->certType,ipsec_cert->certName,ipsec_cert->tableId);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "cert update sql %s\n", buf);
    if (agc_db_exec(db, buf, NULL, NULL, &err_msg) != AGC_DB_OK)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_FALSE;
    }
    free(buf);
    agc_db_free(err_msg);
    return result;
}

int get_total_conns(agc_db_t *db, int *rows)
{
    char *err_msg = NULL;
    char **query_result = NULL;
    int nrow, ncolumn;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "SELECT COUNT(*) FROM ike_conns");
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "conn total query sql %s\n", buf);
    if (agc_db_get_table(db, buf, &query_result, &nrow, &ncolumn, &err_msg) == AGC_DB_OK)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "conn total number: %s\n", query_result[1]);
        *rows = atoi(query_result[1]);
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_FALSE;
    }
    free(buf);
    agc_db_free(err_msg);
    agc_db_free_table(query_result);
    return result;
}

int get_total_secrets(agc_db_t *db, int *rows)
{
    char *err_msg = NULL;
    char **query_result = NULL;
    int nrow, ncolumn;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "SELECT COUNT(*) FROM ike_secrets");
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "secret total query sql %s\n", buf);
    if (agc_db_get_table(db, buf, &query_result, &nrow, &ncolumn, &err_msg) == AGC_DB_OK)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "secret total number: %s\n", query_result[1]);
        *rows = atoi(query_result[1]);
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_FALSE;
    }
    free(buf);
    agc_db_free(err_msg);
    agc_db_free_table(query_result);
    return result;
}

int get_total_certs(agc_db_t *db, int *rows)
{
    char *err_msg = NULL;
    char **query_result = NULL;
    int nrow, ncolumn;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "SELECT COUNT(*) FROM ike_certs");
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "cert total query sql %s\n", buf);
    if (agc_db_get_table(db, buf, &query_result, &nrow, &ncolumn, &err_msg) == AGC_DB_OK)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "cert total number: %s\n", query_result[1]);
        *rows = atoi(query_result[1]);
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_FALSE;
    }
    free(buf);
    agc_db_free(err_msg);
    agc_db_free_table(query_result);
    return result;
}

int get_total_ike_infos(agc_db_t *db, int *rows)
{
    char *err_msg = NULL;
    char **query_result = NULL;
    int nrow, ncolumn;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "SELECT COUNT(*) FROM ike_infos");
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ike info total query sql %s\n", buf);
    if (agc_db_get_table(db, buf, &query_result, &nrow, &ncolumn, &err_msg) == AGC_DB_OK)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ike info total number: %s\n", query_result[1]);
        *rows = atoi(query_result[1]);
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_FALSE;
    }
    free(buf);
    agc_db_free(err_msg);
    agc_db_free_table(query_result);
    return result;
}
