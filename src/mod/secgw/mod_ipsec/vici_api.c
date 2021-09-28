//
// Created by count on 2020/6/15.
//
#include <agc.h>
#include <string.h>
#include <malloc.h>
#include "libvici.h"
#include "vici_api.h"

int vici_get_version(vici_conn_t *conn)
{
    vici_req_t *req;
    vici_res_t *res;
    int ret = 0;

    req = vici_begin("version");
    res = vici_submit(req, conn);
    if (res)
    {
        /*agc_log_printf(AGC_LOG, AGC_LOG_INFO, "strongswan version: %s\n",
               vici_find_str(res, "", "version"));*/
        vici_free_res(res);
    }
    else
    {
        ret = errno;
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "version request failed: %s\n", strerror(errno));
    }
    return ret;
}

int add_conn(vici_conn_t *conn, ipsec_conn_t *ipsec_conn, char *success, char *errmsg)
{
    vici_req_t *req;
    vici_res_t *res;
    char *s1, *s2;
    int ret = 0;

    req = vici_begin("load-conn");

    vici_begin_section(req, ipsec_conn->name);

    vici_begin_list(req, "local_addrs");
    vici_add_list_item(req, ipsec_conn->localAddrs, strlen(ipsec_conn->localAddrs));
    vici_end_list(req);

    vici_begin_list(req, "remote_addrs");
    vici_add_list_item(req, ipsec_conn->remoteAddrs, strlen(ipsec_conn->remoteAddrs));
    vici_end_list(req);

    vici_add_key_value(req, "version", ipsec_conn->version, strlen(ipsec_conn->version));
    vici_add_key_value(req, "mobike", ipsec_conn->mobike, strlen(ipsec_conn->mobike));

    vici_begin_list(req, "proposals");
    vici_add_list_item(req, ipsec_conn->proposals, strlen(ipsec_conn->proposals));
    vici_end_list(req);

    if (0 == strncmp(ipsec_conn->localAuth, "pubkey", MAX_CHARS))
    {
        vici_add_key_value(req, "send_cert", ipsec_conn->certSend, strlen(ipsec_conn->certSend));
    }
    vici_add_key_value(req, "reauth_time", ipsec_conn->ikeReauthTime, strlen(ipsec_conn->ikeReauthTime));
    vici_add_key_value(req, "rekey_time", ipsec_conn->ikeRekeyTime, strlen(ipsec_conn->ikeRekeyTime));
    vici_add_key_value(req, "dpd_delay", ipsec_conn->dpdDelay, strlen(ipsec_conn->dpdDelay));

    vici_begin_section(req, "local");
    vici_add_key_value(req, "auth", ipsec_conn->localAuth, strlen(ipsec_conn->localAuth));
    vici_add_key_value(req, "id", ipsec_conn->localId, strlen(ipsec_conn->localId));
    if (0 == strncmp(ipsec_conn->localAuth, "pubkey", MAX_CHARS))
    {

        vici_begin_section(req, "cert");
        vici_add_key_value(req, "file", ipsec_conn->certFile, strlen(ipsec_conn->certFile));
        vici_end_section(req);
    }
    vici_end_section(req);

    vici_begin_section(req, "remote");
    vici_add_key_value(req, "auth", ipsec_conn->remoteAuth, strlen(ipsec_conn->remoteAuth));
    vici_add_key_value(req, "id", ipsec_conn->remoteId, strlen(ipsec_conn->remoteId));
    vici_end_section(req);

    vici_begin_section(req, "children");
    vici_begin_section(req, ipsec_conn->name);
    vici_begin_list(req, "esp_proposals");
    vici_add_list_item(req, ipsec_conn->childrenEspProposals, strlen(ipsec_conn->childrenEspProposals));
    vici_end_list(req);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ts len: %s %s\n", ipsec_conn->localTs, ipsec_conn->remoteTs);
    if (strnlen(ipsec_conn->localTs, MAX_CHARS) > 0)
    {
        vici_begin_list(req, "local_ts");
        vici_add_list_item(req, ipsec_conn->localTs, strlen(ipsec_conn->localTs));
        vici_end_list(req);
    }
    if (strnlen(ipsec_conn->remoteTs, MAX_CHARS) > 0)
    {
        vici_begin_list(req, "remote_ts");
        vici_add_list_item(req, ipsec_conn->remoteTs, strlen(ipsec_conn->remoteTs));
        vici_end_list(req);
    }
    vici_add_key_value(req, "rekey_time", ipsec_conn->ipsecRekeyTime, strlen(ipsec_conn->ipsecRekeyTime));
    vici_add_key_value(req, "start_action", ipsec_conn->ipsecAction, strlen(ipsec_conn->ipsecAction));
    vici_end_section(req);
    vici_end_section(req);

    vici_end_section(req);

    res = vici_submit(req, conn);
    if (res)
    {
        s1 = vici_find_str(res, "", "success");
        s2 = vici_find_str(res, "", "errmsg");
        printf("add conn success = %s errmsg=%s\n", s1, s2);
        strncpy(success, s1, MAX_CHARS);
        strncpy(errmsg, s2, MAX_CHARS);
        vici_free_res(res);
    }
    else
    {
        ret = errno;
        s2 = strerror(errno);
        fprintf(stderr, "add conn load-conn request failed: %s\n", s2);
        strncpy(errmsg, s2, MAX_CHARS);
    }
    return ret;
}

int unload_conn(vici_conn_t *conn, char *conn_name, char *success, char *errmsg)
{
    vici_req_t *req;
    vici_res_t *res;
    char *s1, *s2;
    int ret = 0;

    req = vici_begin("unload-conn");
    vici_add_key_value(req, "name", conn_name, strlen(conn_name));

    res = vici_submit(req, conn);
    if (res)
    {
        s1 = vici_find_str(res, "", "success");
        s2 = vici_find_str(res, "", "errmsg");
        printf("unload conn success = %s errmsg=%s\n", s1, s2);
        strncpy(success, s1, MAX_CHARS);
        strncpy(errmsg, s2, MAX_CHARS);
        vici_free_res(res);
    }
    else
    {
        ret = errno;
        s2 = strerror(errno);
        fprintf(stderr, "unload conn load-conn request failed: %s\n", s2);
        strncpy(errmsg, s2, MAX_CHARS);
    }
    return ret;
}

void malloc_conn(ipsec_conn_t *ipsec_conn)
{
    ipsec_conn->name = (char *)malloc(MAX_CHARS);
    ipsec_conn->localAddrs = (char *)malloc(MAX_CHARS);
    ipsec_conn->remoteAddrs = (char *)malloc(MAX_CHARS);
    ipsec_conn->localAuth = (char *)malloc(MAX_CHARS);
    ipsec_conn->certFile = (char *)malloc(MAX_CHARS);
    ipsec_conn->certSend = (char *)malloc(MAX_CHARS);
    ipsec_conn->ikeReauthTime = (char *)malloc(MAX_CHARS);
    ipsec_conn->ikeRekeyTime = (char *)malloc(MAX_CHARS);
    ipsec_conn->ipsecRekeyTime = (char *)malloc(MAX_CHARS);
    ipsec_conn->dpdDelay = (char *)malloc(MAX_CHARS);
    ipsec_conn->ipsecAction = (char *)malloc(MAX_CHARS);
    ipsec_conn->localId = (char *)malloc(MAX_CHARS);
    ipsec_conn->remoteAuth = (char *)malloc(MAX_CHARS);
    ipsec_conn->remoteId = (char *)malloc(MAX_CHARS);
    ipsec_conn->childrenEspProposals = (char *)malloc(MAX_CHARS);
    ipsec_conn->mobike = (char *)malloc(MAX_CHARS);
    ipsec_conn->version = (char *)malloc(MAX_CHARS);
    ipsec_conn->proposals = (char *)malloc(MAX_CHARS);
    ipsec_conn->localTs = (char *)malloc(MAX_CHARS);
    ipsec_conn->remoteTs = (char *)malloc(MAX_CHARS);
    memset(ipsec_conn->name, 0, MAX_CHARS);
    memset(ipsec_conn->localAddrs, 0, MAX_CHARS);
    memset(ipsec_conn->remoteAddrs, 0, MAX_CHARS);
    memset(ipsec_conn->localAuth, 0, MAX_CHARS);
    memset(ipsec_conn->certFile, 0, MAX_CHARS);
    memset(ipsec_conn->certSend, 0, MAX_CHARS);
    memset(ipsec_conn->ikeReauthTime, 0, MAX_CHARS);
    memset(ipsec_conn->ikeRekeyTime, 0, MAX_CHARS);
    memset(ipsec_conn->ipsecRekeyTime, 0, MAX_CHARS);
    memset(ipsec_conn->dpdDelay, 0, MAX_CHARS);
    memset(ipsec_conn->ipsecAction, 0, MAX_CHARS);
    memset(ipsec_conn->localId, 0, MAX_CHARS);
    memset(ipsec_conn->remoteAuth, 0, MAX_CHARS);
    memset(ipsec_conn->remoteId, 0, MAX_CHARS);
    memset(ipsec_conn->childrenEspProposals, 0, MAX_CHARS);
    memset(ipsec_conn->mobike, 0, MAX_CHARS);
    memset(ipsec_conn->version, 0, MAX_CHARS);
    memset(ipsec_conn->proposals, 0, MAX_CHARS);
    memset(ipsec_conn->localTs, 0, MAX_CHARS);
    memset(ipsec_conn->remoteTs, 0, MAX_CHARS);
}
void free_conn(ipsec_conn_t *ipsec_conn)
{
    if(ipsec_conn->name) free (ipsec_conn->name);
    if(ipsec_conn->localAddrs) free(ipsec_conn->localAddrs);
    if(ipsec_conn->remoteAddrs) free(ipsec_conn->remoteAddrs);
    if(ipsec_conn->localAuth) free(ipsec_conn->localAuth);
    if(ipsec_conn->certFile) free(ipsec_conn->certFile);
    if(ipsec_conn->certSend) free(ipsec_conn->certSend);
    if(ipsec_conn->ikeReauthTime) free(ipsec_conn->ikeReauthTime);
    if(ipsec_conn->ikeRekeyTime) free(ipsec_conn->ikeRekeyTime);
    if(ipsec_conn->ipsecRekeyTime) free(ipsec_conn->ipsecRekeyTime);
    if(ipsec_conn->dpdDelay) free(ipsec_conn->dpdDelay);
    if(ipsec_conn->ipsecAction) free(ipsec_conn->ipsecAction);
    if(ipsec_conn->localId) free(ipsec_conn->localId);
    if(ipsec_conn->remoteAuth) free(ipsec_conn->remoteAuth);
    if(ipsec_conn->remoteId) free(ipsec_conn->remoteId);
    if(ipsec_conn->childrenEspProposals) free(ipsec_conn->childrenEspProposals);
    if(ipsec_conn->mobike) free(ipsec_conn->mobike);
    if(ipsec_conn->version) free(ipsec_conn->version);
    if(ipsec_conn->proposals) free(ipsec_conn->proposals);
    if(ipsec_conn->localTs) free(ipsec_conn->localTs);
    if(ipsec_conn->remoteTs) free(ipsec_conn->remoteTs);
}

void malloc_conn_query(conn_query_t *conn_query)
{
    conn_query->connName = (char *)malloc(MAX_CHARS);
    conn_query->localHost = (char *)malloc(MAX_CHARS);
    conn_query->remoteHost = (char *)malloc(MAX_CHARS);
    conn_query->localAuth = (char *)malloc(MAX_CHARS);
    conn_query->certSend = (char *)malloc(MAX_CHARS);
    conn_query->ikeReauthTime = (char *)malloc(MAX_CHARS);
    conn_query->ikeRekeyTime = (char *)malloc(MAX_CHARS);
    conn_query->ipsecRekeyTime = (char *)malloc(MAX_CHARS);
    conn_query->dpdDelay = (char *)malloc(MAX_CHARS);
    conn_query->ipsecAction = (char *)malloc(MAX_CHARS);
    conn_query->localId = (char *)malloc(MAX_CHARS);
    conn_query->remoteAuth = (char *)malloc(MAX_CHARS);
    conn_query->remoteId = (char *)malloc(MAX_CHARS);
    conn_query->ikeAuthAlg = (char *)malloc(MAX_CHARS);
    conn_query->ikeCryptAlg = (char *)malloc(MAX_CHARS);
    conn_query->ikeDHAlg = (char *)malloc(MAX_CHARS);
    conn_query->ipsecCryptAlg = (char *)malloc(MAX_CHARS);
    conn_query->ipsecAuthAlg = (char *)malloc(MAX_CHARS);
    conn_query->ipsecDHAlg = (char *)malloc(MAX_CHARS);
    conn_query->localTs = (char *)malloc(MAX_CHARS);
    conn_query->remoteTs = (char *)malloc(MAX_CHARS);
    memset(conn_query->connName, 0, MAX_CHARS);
    memset(conn_query->localHost, 0, MAX_CHARS);
    memset(conn_query->remoteHost, 0, MAX_CHARS);
    memset(conn_query->localAuth, 0, MAX_CHARS);
    memset(conn_query->certSend, 0, MAX_CHARS);
    memset(conn_query->ikeReauthTime, 0, MAX_CHARS);
    memset(conn_query->ikeRekeyTime, 0, MAX_CHARS);
    memset(conn_query->ipsecRekeyTime, 0, MAX_CHARS);
    memset(conn_query->dpdDelay, 0, MAX_CHARS);
    memset(conn_query->ipsecAction, 0, MAX_CHARS);
    memset(conn_query->localId, 0, MAX_CHARS);
    memset(conn_query->remoteAuth, 0, MAX_CHARS);
    memset(conn_query->remoteId, 0, MAX_CHARS);
    memset(conn_query->ikeAuthAlg, 0, MAX_CHARS);
    memset(conn_query->ikeCryptAlg, 0, MAX_CHARS);
    memset(conn_query->ikeDHAlg, 0, MAX_CHARS);
    memset(conn_query->ipsecCryptAlg, 0, MAX_CHARS);
    memset(conn_query->ipsecAuthAlg, 0, MAX_CHARS);
    memset(conn_query->ipsecDHAlg, 0, MAX_CHARS);
    memset(conn_query->localTs, 0, MAX_CHARS);
    memset(conn_query->remoteTs, 0, MAX_CHARS);
}
void free_conn_query(conn_query_t *conn_query)
{
    if (conn_query->connName) free(conn_query->connName);
    if (conn_query->localHost) free(conn_query->localHost);
    if (conn_query->remoteHost) free(conn_query->remoteHost);
    if (conn_query->localAuth) free(conn_query->localAuth);
    if (conn_query->certSend) free(conn_query->certSend);
    if (conn_query->ikeReauthTime) free(conn_query->ikeReauthTime);
    if (conn_query->ikeRekeyTime) free(conn_query->ikeRekeyTime);
    if (conn_query->ipsecRekeyTime) free(conn_query->ipsecRekeyTime);
    if (conn_query->dpdDelay) free(conn_query->dpdDelay);
    if (conn_query->ipsecAction) free(conn_query->ipsecAction);
    if (conn_query->localId) free(conn_query->localId);
    if (conn_query->remoteAuth) free(conn_query->remoteAuth);
    if (conn_query->remoteId) free(conn_query->remoteId);
    if (conn_query->ikeAuthAlg) free(conn_query->ikeAuthAlg);
    if (conn_query->ikeCryptAlg) free(conn_query->ikeCryptAlg);
    if (conn_query->ikeDHAlg) free(conn_query->ikeDHAlg);
    if (conn_query->ipsecCryptAlg) free(conn_query->ipsecCryptAlg);
    if (conn_query->ipsecAuthAlg) free(conn_query->ipsecAuthAlg);
    if (conn_query->ipsecDHAlg) free(conn_query->ipsecDHAlg);
    if (conn_query->localTs) free(conn_query->localTs);
    if (conn_query->remoteTs) free(conn_query->remoteTs);
}

void malloc_ike_query(ike_query_t *ike_query)
{
    ike_query->ikeName = (char *)malloc(MAX_CHARS);
    ike_query->ikeId = (char *)malloc(MAX_CHARS);
    ike_query->ikeState = (char *)malloc(MAX_CHARS);
    ike_query->localHost = (char *)malloc(MAX_CHARS);
    ike_query->remoteHost = (char *)malloc(MAX_CHARS);
    ike_query->childSaName = (char *)malloc(MAX_CHARS);
    ike_query->childSaId = (char *)malloc(MAX_CHARS);
    ike_query->childSaState = (char *)malloc(MAX_CHARS);
    ike_query->bytesIn = (char *)malloc(MAX_CHARS);
    ike_query->packetsIn = (char *)malloc(MAX_CHARS);
    ike_query->bytesOut = (char *)malloc(MAX_CHARS);
    ike_query->packetsOut = (char *)malloc(MAX_CHARS);
    ike_query->spiIn = (char *)malloc(MAX_CHARS);
    ike_query->spiOut = (char *)malloc(MAX_CHARS);
    ike_query->updateTime = (char *)malloc(MAX_CHARS);
    memset(ike_query->ikeName, 0, MAX_CHARS);
    memset(ike_query->ikeId, 0, MAX_CHARS);
    memset(ike_query->ikeState, 0, MAX_CHARS);
    memset(ike_query->localHost, 0, MAX_CHARS);
    memset(ike_query->remoteHost, 0, MAX_CHARS);
    memset(ike_query->childSaName, 0, MAX_CHARS);
    memset(ike_query->childSaId, 0, MAX_CHARS);
    memset(ike_query->childSaState, 0, MAX_CHARS);
    memset(ike_query->bytesIn, 0, MAX_CHARS);
    memset(ike_query->packetsIn, 0, MAX_CHARS);
    memset(ike_query->bytesOut, 0, MAX_CHARS);
    memset(ike_query->packetsOut, 0, MAX_CHARS);
    memset(ike_query->spiIn, 0, MAX_CHARS);
    memset(ike_query->spiOut, 0, MAX_CHARS);
    memset(ike_query->updateTime, 0, MAX_CHARS);
}

void free_ike_query(ike_query_t *ike_query)
{
    if (ike_query->ikeName) free(ike_query->ikeName);
    if (ike_query->ikeId) free(ike_query->ikeId);
    if (ike_query->ikeState) free(ike_query->ikeState);
    if (ike_query->localHost) free(ike_query->localHost);
    if (ike_query->remoteHost) free(ike_query->remoteHost);
    if (ike_query->childSaName) free(ike_query->childSaName);
    if (ike_query->childSaId) free(ike_query->childSaId);
    if (ike_query->childSaState) free(ike_query->childSaState);
    if (ike_query->bytesIn) free(ike_query->bytesIn);
    if (ike_query->packetsIn) free(ike_query->packetsIn);
    if (ike_query->bytesOut) free(ike_query->bytesOut);
    if (ike_query->packetsOut) free(ike_query->packetsOut);
    if (ike_query->spiIn) free(ike_query->spiIn);
    if (ike_query->spiOut) free(ike_query->spiOut);
    if (ike_query->updateTime) free(ike_query->updateTime);
}

int add_cert_file(const char *name, const char *content, ENUM_CERT_TYPE cert_type)
{
    char *file_name;
    FILE *pFile;

    file_name = (char *)malloc(MAX_CHARS);
    if (enum_cert == cert_type)
    {
        strncpy(file_name, CERT_FILE_PATH, MAX_CHARS);
    }
    else if (enum_private_key == cert_type)
    {
        strncpy(file_name, PRIVATE_KEY_FILE_PATH, MAX_CHARS);
    }
    else
    {
        return AGC_STATUS_FALSE;
    }
    strncat(file_name, name, MAX_CHARS);
    printf("cert or private file name: %s\n", file_name);

    pFile = fopen(file_name, "w");
    if (NULL == pFile)
    {
        printf("open cert file error\n");
        free(file_name);
        return AGC_STATUS_FALSE;
    }

    fprintf(pFile, "%s", content);
    fclose(pFile);
    free(file_name);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "cert file write success.\n");

    return AGC_STATUS_SUCCESS;
}

int add_conn_file(ipsec_conn_t *ipsec_conn)
{
    char *file_name;
    FILE *pFile;

    file_name = (char *)malloc(MAX_CHARS);
    strncpy(file_name, CONF_FILE_PATH, MAX_CHARS);
    strncat(file_name, ipsec_conn->name, MAX_CHARS);
    strncat(file_name, ".conf", MAX_CHARS);
    printf("conf file name: %s\n", file_name);

    pFile = fopen(file_name, "w");
    if (NULL == pFile)
    {
        printf("open file error\n");
        free(file_name);
        return -1;
    }

    fprintf(pFile, "connections {\n");
    fprintf(pFile, "    %s {\n", ipsec_conn->name);
    fprintf(pFile, "        version = %s\n", ipsec_conn->version);
    fprintf(pFile, "        local_addrs = %s\n", ipsec_conn->localAddrs);
    fprintf(pFile, "        remote_addrs = %s\n", ipsec_conn->remoteAddrs);
    fprintf(pFile, "        proposals = %s\n", ipsec_conn->proposals);
    fprintf(pFile, "        mobike = %s\n", ipsec_conn->mobike);
    fprintf(pFile, "        reauth_time = %s\n", ipsec_conn->ikeReauthTime);
    fprintf(pFile, "        rekey_time = %s\n", ipsec_conn->ikeRekeyTime);
    fprintf(pFile, "        dpd_delay = %s\n", ipsec_conn->dpdDelay);
    fprintf(pFile, "        send_cert = %s\n", ipsec_conn->certSend);

    fprintf(pFile, "        local {\n");
    fprintf(pFile, "            auth = %s\n", ipsec_conn->localAuth);
    fprintf(pFile, "            id = %s\n", ipsec_conn->localId);
    if (strncmp(ipsec_conn->localAuth, "pubkey", MAX_CHARS) == 0)
    {
        fprintf(pFile, "            cert {\n");
        fprintf(pFile, "                file = %s\n", ipsec_conn->certFile);
        fprintf(pFile, "            }\n");
    }
    fprintf(pFile, "        }\n");
    fprintf(pFile, "        remote {\n");
    fprintf(pFile, "            auth = %s\n", ipsec_conn->remoteAuth);
    fprintf(pFile, "            id = %s\n", ipsec_conn->remoteId);
    fprintf(pFile, "        }\n");
    fprintf(pFile, "        children {\n");
    fprintf(pFile, "            %s {\n", ipsec_conn->name);
    fprintf(pFile, "                rekey_time = %s\n", ipsec_conn->ipsecRekeyTime);
    fprintf(pFile, "                esp_proposals = %s\n", ipsec_conn->childrenEspProposals);
    fprintf(pFile, "                start_action = %s\n", ipsec_conn->ipsecAction);
    if (strnlen(ipsec_conn->localTs, MAX_CHARS) > 0)
    {
        fprintf(pFile, "                local_ts = %s\n", ipsec_conn->localTs);
    }
    if (strnlen(ipsec_conn->remoteTs, MAX_CHARS) > 0)
    {
        fprintf(pFile, "                remote_ts = %s\n", ipsec_conn->remoteTs);
    }
    fprintf(pFile, "            }\n");
    fprintf(pFile, "        }\n");
    fprintf(pFile, "    }\n");
    fprintf(pFile, "}\n");

    fclose(pFile);
    free(file_name);
    return 0;
}

int delete_conn_file(char *conn_name)
{
    char *file_name;
    int ret = 0;

    file_name = (char *)malloc(MAX_CHARS);
    strncpy(file_name, CONF_FILE_PATH, MAX_CHARS);
    strncat(file_name, conn_name, MAX_CHARS);
    strncat(file_name, ".conf", MAX_CHARS);
    printf("conf file name: %s\n", file_name);

    if( remove(file_name) == 0 )
    {
        printf("Removed %s.", file_name);
    }
    else
    {
        perror("remove");
        ret = -1;
    }
    free(file_name);
    return ret;
}

void print_sa(ike_query_t *ike_query)
{
    printf("ike name: %s\n", ike_query->ikeName);
    printf("ike id: %s\n", ike_query->ikeId);
    printf("local host: %s\n", ike_query->localHost);
    printf("remote host: %s\n", ike_query->remoteHost);
    printf("ike state: %s\n", ike_query->ikeState);
    printf("child sa name: %s\n", ike_query->childSaName);
    printf("child sa id: %s\n", ike_query->childSaId);
    printf("sa state: %s\n", ike_query->childSaState);
    printf("spi-in: %s\n", ike_query->spiIn);
    printf("spi-out: %s\n", ike_query->spiOut);
    printf("bytes-in: %s\n", ike_query->bytesIn);
    printf("packets-in: %s\n", ike_query->packetsIn);
    printf("bytes-out: %s\n", ike_query->bytesOut);
    printf("packets-out: %s\n", ike_query->packetsOut);
}

void delete_special_char(char *dst, char *src)
{
    int i;
    for (i = 0; src[i] != '\0'; i++)
    {
        if ((src[i] >= '0' && src[i] <= '9')
            || (src[i] >= 'a' && src[i] <= 'z')
            || (src[i] >= 'A' && src[i] <= 'Z')
            || (src[i] == '.') || (src[i] == '-') || (src[i] == '_'))
        {
            dst[i] = src[i];
        }
        else
        {
            dst[i] = '\0';
            return;
        }
        if (i == 99)
        {
            dst[i] = '\0';
            return;
        }
    }
}

void print_ike_query_info(ike_query_t *ikeQueryInfo)
{
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ike name: %s\n", ikeQueryInfo->ikeName);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ike id: %s\n", ikeQueryInfo->ikeId);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ike state: %s\n", ikeQueryInfo->ikeState);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ike local host: %s\n", ikeQueryInfo->localHost);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ike remote host: %s\n", ikeQueryInfo->remoteHost);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "child name: %s\n", ikeQueryInfo->childSaName);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "child id: %s\n", ikeQueryInfo->childSaId);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "child state: %s\n", ikeQueryInfo->childSaState);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "bytes in: %s\n", ikeQueryInfo->bytesIn);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "bytes out: %s\n", ikeQueryInfo->bytesOut);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "packets in: %s\n", ikeQueryInfo->packetsIn);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "packets out: %s\n", ikeQueryInfo->packetsOut);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "spi in: %s\n", ikeQueryInfo->spiIn);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "spi out: %s\n", ikeQueryInfo->spiOut);
}

int vici_parse_section_cb(void *save, vici_res_t *res, char *name)
{
    ike_query_t *ikeQueryInfo;

    agc_log_printf(AGC_LOG, AGC_LOG_INFO,"section: %s\n", name);
    ikeQueryInfo= (ike_query_t *)save;
    if (ikeQueryInfo->isIkeSa)
    {
        strncpy(ikeQueryInfo->ikeName, name, MAX_CHARS);
        ikeQueryInfo->isIkeSa = AGC_FALSE;
    }
    if (strcmp(name, "old") == 0) {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "enter old section\n");
        ikeQueryInfo->isOld = AGC_TRUE;
    }
    if (strcmp(name, "new") == 0) {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "enter new section\n");
        ikeQueryInfo->isOld = AGC_FALSE;
    }
    if (strcmp(name, "child-sas") == 0) {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "enter child section\n");
        ikeQueryInfo->isChildSa = AGC_TRUE;
    }
    if (vici_parse_cb(res, vici_parse_section_cb, vici_parse_key_value_cb, vici_parse_list_item_cb, save) != 0)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "parsing failed: %s\n", strerror(errno));
    }
    return 0;
}

int vici_parse_key_value_cb(void *save, vici_res_t *res, char *name, void *value, int len)
{
    char tripValue[MAX_CHARS];
    ike_query_t *ikeQueryInfo;

    delete_special_char(tripValue, (char *)value);
    //agc_log_printf(AGC_LOG, AGC_LOG_INFO, "key=%s, value=%s\n", name, tripValue);
    ikeQueryInfo = (ike_query_t *)save;
    if (ikeQueryInfo->isOld)
    {
        return 0;
    }
    if (strcmp(name, "name") == 0)
    {
        if (ikeQueryInfo->isChildSa == AGC_TRUE)
        {
            //agc_log_printf(AGC_LOG, AGC_LOG_INFO, "child name\n");
            strncpy(ikeQueryInfo->childSaName,tripValue,MAX_CHARS);
        }
    }
    if (strcmp(name, "uniqueid") == 0)
    {
        if (ikeQueryInfo->isChildSa == AGC_TRUE)
        {
            //agc_log_printf(AGC_LOG, AGC_LOG_INFO, "child id\n");
            strncpy(ikeQueryInfo->childSaId,tripValue,MAX_CHARS);
        }
        else
        {
            //agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ike id\n");
            strncpy(ikeQueryInfo->ikeId,tripValue,MAX_CHARS);
        }
    }
    if (strcmp(name, "state") == 0)
    {
        if (ikeQueryInfo->isChildSa == AGC_TRUE)
        {
            //agc_log_printf(AGC_LOG, AGC_LOG_INFO, "child state\n");
            strncpy(ikeQueryInfo->childSaState,tripValue,MAX_CHARS);
        }
        else
        {
            //agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ike state\n");
            strncpy(ikeQueryInfo->ikeState,tripValue,MAX_CHARS);
        }
    }
    if (strcmp(name, "local-host") == 0)
    {
        //agc_log_printf(AGC_LOG, AGC_LOG_INFO, "local host\n");
        strncpy(ikeQueryInfo->localHost,tripValue,MAX_CHARS);
    }
    if (strcmp(name, "remote-host") == 0)
    {
        //agc_log_printf(AGC_LOG, AGC_LOG_INFO, "remote host\n");
        strncpy(ikeQueryInfo->remoteHost,tripValue,MAX_CHARS);
    }
    if (strcmp(name, "spi-in") == 0)
    {
        strncpy(ikeQueryInfo->spiIn, (char *)value, SPI_LEN);
    }
    if (strcmp(name, "spi-out") == 0)
    {
        strncpy(ikeQueryInfo->spiOut, (char *)value, SPI_LEN);
    }
    if (strcmp(name, "bytes-in") == 0)
    {
        strncpy(ikeQueryInfo->bytesIn,tripValue,MAX_CHARS);
    }
    if (strcmp(name, "bytes-out") == 0)
    {
        strncpy(ikeQueryInfo->bytesOut,tripValue,MAX_CHARS);
    }
    if (strcmp(name, "packets-in") == 0)
    {
        strncpy(ikeQueryInfo->packetsIn,tripValue,MAX_CHARS);
    }
    if (strcmp(name, "packets-out") == 0)
    {
        strncpy(ikeQueryInfo->packetsOut,tripValue,MAX_CHARS);
    }

    return 0;
}

int vici_parse_list_item_cb(void *save, vici_res_t *res, char *name, void *value, int len)
{
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "list: %s\n", name);
    return 0;
}

void malloc_secret(ipsec_secret_t *ipsec_secret)
{
    ipsec_secret->name = (char *)malloc(MAX_CHARS);
    ipsec_secret->id = (char *)malloc(MAX_CHARS);
    ipsec_secret->secret = (char *)malloc(SECRETS_LEN);
    memset(ipsec_secret->name, 0, MAX_CHARS);
    memset(ipsec_secret->id, 0, MAX_CHARS);
    memset(ipsec_secret->secret, 0, SECRETS_LEN);
}

void free_secret(ipsec_secret_t *ipsec_secret)
{
    if (ipsec_secret->name) free(ipsec_secret->name);
    if (ipsec_secret->id) free(ipsec_secret->id);
    if (ipsec_secret->secret) free(ipsec_secret->secret);
}

void malloc_cert(ipsec_cert_t *ipsec_cert)
{
    ipsec_cert->id = (char *)malloc(MAX_CHARS);
    ipsec_cert->certName = (char *)malloc(MAX_CHARS);
    memset(ipsec_cert->id, 0, MAX_CHARS);
    memset(ipsec_cert->certName, 0, MAX_CHARS);
}

void free_cert(ipsec_cert_t *ipsec_cert)
{
    if (ipsec_cert->id) free(ipsec_cert->id);
    if (ipsec_cert->certName) free(ipsec_cert->certName);
}

int load_secret(vici_conn_t *conn, ipsec_secret_t *ipsec_secret, char *success, char *errmsg)
{
    vici_req_t *req;
    vici_res_t *res;
    char *s1, *s2;
    int ret = 0;

    req = vici_begin("load-shared");

    vici_add_key_value(req, "id", ipsec_secret->name, strlen(ipsec_secret->name));
    vici_add_key_value(req, "type", "IKE", strlen("IKE"));
    vici_add_key_value(req, "data", ipsec_secret->secret, strlen(ipsec_secret->secret));

    vici_begin_list(req, "owners");
    vici_add_list_item(req, ipsec_secret->id, strlen(ipsec_secret->id));
    vici_end_list(req);

    res = vici_submit(req, conn);
    if (res)
    {
        s1 = vici_find_str(res, "", "success");
        s2 = vici_find_str(res, "", "errmsg");
        agc_log_printf(AGC_LOG, AGC_LOG_INFO,"load secret success = %s errmsg=%s\n", s1, s2);
        strncpy(success, s1, MAX_CHARS);
        strncpy(errmsg, s2, MAX_CHARS);
        vici_free_res(res);
    }
    else
    {
        ret = errno;
        s2 = strerror(errno);
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"load-shared request failed: %s\n", s2);
        strncpy(errmsg, s2, MAX_CHARS);
    }
    return ret;
}

int unload_secret(vici_conn_t *conn, char *secret_id, char *success, char *errmsg)
{
    vici_req_t *req;
    vici_res_t *res;
    char *s1, *s2;
    int ret = 0;

    req = vici_begin("unload-shared");
    vici_add_key_value(req, "id", secret_id, strlen(secret_id));
    res = vici_submit(req, conn);
    if (res)
    {
        s1 = vici_find_str(res, "", "success");
        s2 = vici_find_str(res, "", "errmsg");
        agc_log_printf(AGC_LOG, AGC_LOG_INFO,"unload secret success = %s errmsg=%s\n", s1, s2);
        strncpy(success, s1, MAX_CHARS);
        strncpy(errmsg, s2, MAX_CHARS);
        vici_free_res(res);
    }
    else
    {
        ret = errno;
        s2 = strerror(errno);
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"unload-shared request failed: %s\n", s2);
        strncpy(errmsg, s2, MAX_CHARS);
    }
    return ret;
}

int vici_load_cert(vici_conn_t *conn, FILE *pFile, char *success, char *errmsg)
{
    vici_req_t *req;
    vici_res_t *res;
    char *s1, *s2, *data = NULL;
    int ret = 0, length = 0;

    fseek(pFile, 0, SEEK_END);
    length = ftell(pFile);
    data = (char *)malloc((length + 1) * sizeof(char));
    rewind(pFile);
    length = fread(data, 1, length, pFile);

    if (length == 0)
    {
        printf("read file error\n");
        free(data);
        return AGC_STATUS_FALSE;
    }
    data[length] = '\0';

    req = vici_begin("load-cert");
    vici_add_key_value(req, "type", "X509", strlen("X509"));
    vici_add_key_value(req, "flag", "NONE", strlen("NONE"));
    vici_add_key_value(req, "data", data, strlen(data));

    res = vici_submit(req, conn);
    if (res)
    {
        s1 = vici_find_str(res, "", "success");
        s2 = vici_find_str(res, "", "errmsg");
        agc_log_printf(AGC_LOG, AGC_LOG_INFO,"load cert success = %s errmsg=%s\n", s1, s2);
        strncpy(success, s1, MAX_CHARS);
        strncpy(errmsg, s2, MAX_CHARS);
        vici_free_res(res);
    }
    else
    {
        ret = errno;
        s2 = strerror(errno);
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"load cert request failed: %s\n", s2);
        strncpy(errmsg, s2, MAX_CHARS);
    }
    free(data);
    return ret;
}

int vici_load_key(vici_conn_t *conn, FILE *pFile, char *success, char *errmsg)
{
    vici_req_t *req;
    vici_res_t *res;
    char *s1, *s2, *filename, *data = NULL;
    int ret = 0, length = 0;

    fseek(pFile, 0, SEEK_END);
    length = ftell(pFile);
    data = (char *)malloc((length + 1) * sizeof(char));
    rewind(pFile);
    length = fread(data, 1, length, pFile);
    if (length == 0)
    {
        printf("read file error\n");
        free(data);
        return AGC_STATUS_FALSE;
    }
    data[length] = '\0';

    req = vici_begin("load-key");
    vici_add_key_value(req, "type", "rsa", strlen("rsa"));
    vici_add_key_value(req, "data", data, strlen(data));

    res = vici_submit(req, conn);
    if (res)
    {
        s1 = vici_find_str(res, "", "success");
        s2 = vici_find_str(res, "", "errmsg");
        agc_log_printf(AGC_LOG, AGC_LOG_INFO,"load key success = %s errmsg=%s\n", s1, s2);
        strncpy(success, s1, MAX_CHARS);
        strncpy(errmsg, s2, MAX_CHARS);
        vici_free_res(res);
    }
    else
    {
        ret = errno;
        s2 = strerror(errno);
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"load key request failed: %s\n", s2);
        strncpy(errmsg, s2, MAX_CHARS);
    }
    free(data);
    return ret;
}

int create_secret_file(ipsec_secret_t *ipsec_secret)
{
    char *file_name;
    FILE *pFile;

    file_name = (char *)malloc(MAX_CHARS);
    strncpy(file_name, CONF_FILE_PATH, MAX_CHARS);
    strncat(file_name, "secret_", MAX_CHARS);
    strncat(file_name, ipsec_secret->name, MAX_CHARS);
    strncat(file_name, ".conf", MAX_CHARS);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO,"secret file name: %s\n", file_name);

    pFile = fopen(file_name, "w");
    if (NULL == pFile)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"open secret file error\n");
        free(file_name);
        return -1;
    }

    fprintf(pFile, "secrets {\n");
    fprintf(pFile, "    ike-%s {\n", ipsec_secret->name);
    fprintf(pFile, "        id = %s\n", ipsec_secret->id);
    fprintf(pFile, "        secret = \"%s\"\n", ipsec_secret->secret);
    fprintf(pFile, "    }\n");
    fprintf(pFile, "}\n");

    fclose(pFile);
    free(file_name);
    return 0;
}

int delete_secret_file(char *secret_name)
{
    char *file_name;
    int ret = 0;

    file_name = (char *)malloc(MAX_CHARS);
    strncpy(file_name, CONF_FILE_PATH, MAX_CHARS);
    strncat(file_name, "secret_", MAX_CHARS);
    strncat(file_name, secret_name, MAX_CHARS);
    strncat(file_name, ".conf", MAX_CHARS);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO,"secret file name: %s\n", file_name);

    if( remove(file_name) == 0 )
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO,"Removed %s.\n", file_name);
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"remove %s failure\n", file_name);
        ret = -1;
    }
    free(file_name);
    return ret;
}

int delete_cert_file(ipsec_cert_t *cert)
{
    char filename[MAX_CHARS];

    if (enum_cert == cert->certType)
    {
        strncpy(filename, CERT_FILE_PATH, MAX_CHARS);
        strncat(filename, cert->certName, MAX_CHARS);
    }
    else if (enum_private_key == cert->certType)
    {
        strncpy(filename, PRIVATE_KEY_FILE_PATH, MAX_CHARS);
        strncat(filename, cert->certName, MAX_CHARS);
    }
    else
    {
        return AGC_STATUS_FALSE;
    }
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "remove cert file: %s\n", filename);

    if( remove(filename) == 0 )
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO,"Removed %s.\n", filename);
        return AGC_STATUS_SUCCESS;
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"remove %s failure\n", filename);
        return AGC_STATUS_FALSE;
    }
}
