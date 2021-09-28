//
// Created by count on 2020/6/18.
//
#include "agc_db.h"
#include "libvici.h"
#include "vici_api.h"
#include "secgw_db_api.h"
#include "secgw_config_api.h"
#include "mod_ipsec.h"

int verify_conn_config(conn_query_t *conn_query, ipsec_conn_t *ipsec_conn, config_mode_t config_mode)
{
    agc_db_t *db;
    ipsec_cert_t cert;
    ipsec_secret_t secret;
    char tmp[MAX_CHARS];

    db = get_agc_db();
    if (config_mode == CONFIG_ADD && SECGW_DB_FIND == find_conn_from_db(db, conn_query))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ike conn exist");
        return AGC_STATUS_FALSE;
    }

    if (atoi(conn_query->localAuth) > auth_size || atoi(conn_query->remoteAuth) > auth_size ||
        atoi(conn_query->certSend) > send_cert_size || atoi(conn_query->ipsecAction) > action_size ||
        atoi(conn_query->ikeCryptAlg) > crypt_alg_size || atoi(conn_query->ikeAuthAlg) > ike_auth_alg_size ||
        atoi(conn_query->ikeDHAlg) > ike_dh_alg_size || atoi(conn_query->ipsecCryptAlg) > crypt_alg_size ||
        atoi(conn_query->ipsecAuthAlg) > child_auth_alg_size || atoi(conn_query->ipsecDHAlg) > child_dh_alg_size)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ike conn enum error");
        return AGC_STATUS_FALSE;
    }

    if (0 == strncmp(conn_query->localAuth, "0", MAX_CHARS))
    {
        malloc_cert(&cert);
        strncpy(cert.id, conn_query->localId, MAX_CHARS);
        if (SECGW_DB_FIND != find_cert_from_db(db, &cert))
        {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ike conn not found cert\n");
            free_cert(&cert);
            return AGC_STATUS_GENERR;
        }
        strncpy(tmp, CERT_FILE_PATH, MAX_CHARS);
        strncat(tmp, cert.certName, MAX_CHARS);
        free_cert(&cert);
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ike conn cert file: %s\n", tmp);
    }

    if (0 == strncmp(conn_query->localAuth, "1", MAX_CHARS))
    {
        malloc_secret(&secret);
        strncpy(secret.id, conn_query->localId, MAX_CHARS);
        if (SECGW_DB_FIND != find_secret_from_db(db, &secret))
        {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ike conn not found secret\n");
            free_secret(&secret);
            return AGC_STATUS_GENERR;
        }
        free_secret(&secret);
    }

    strncpy(ipsec_conn->certFile, tmp, MAX_CHARS);
    strncpy(ipsec_conn->name, conn_query->connName, MAX_CHARS);
    strncpy(ipsec_conn->localId, conn_query->localId, MAX_CHARS);
    strncpy(ipsec_conn->localAuth, enum_auth[atoi(conn_query->localAuth)], MAX_CHARS);
    strncpy(ipsec_conn->localAddrs, conn_query->localHost, MAX_CHARS);
    strncpy(ipsec_conn->certSend, enum_send_cert[atoi(conn_query->certSend)], MAX_CHARS);
    strncpy(ipsec_conn->remoteId, conn_query->remoteId, MAX_CHARS);
    strncpy(ipsec_conn->remoteAddrs, conn_query->remoteHost, MAX_CHARS);
    strncpy(ipsec_conn->remoteAuth, enum_auth[atoi(conn_query->remoteAuth)], MAX_CHARS);
    strncpy(ipsec_conn->localTs, conn_query->localTs, MAX_CHARS);
    strncpy(ipsec_conn->remoteTs, conn_query->remoteTs, MAX_CHARS);
    strncpy(ipsec_conn->ikeRekeyTime, conn_query->ikeRekeyTime, MAX_CHARS);
    strncpy(ipsec_conn->ikeReauthTime, conn_query->ikeReauthTime, MAX_CHARS);
    strncpy(ipsec_conn->ipsecRekeyTime, conn_query->ipsecRekeyTime, MAX_CHARS);
    strncpy(ipsec_conn->dpdDelay, conn_query->dpdDelay, MAX_CHARS);
    strncat(ipsec_conn->ikeRekeyTime, "s", MAX_CHARS);
    strncat(ipsec_conn->ikeReauthTime, "s", MAX_CHARS);
    strncat(ipsec_conn->ipsecRekeyTime, "s", MAX_CHARS);
    strncat(ipsec_conn->dpdDelay, "s", MAX_CHARS);
    strncpy(ipsec_conn->ipsecAction, enum_action[atoi(conn_query->ipsecAction)], MAX_CHARS);

    strncpy(tmp, enum_crypt_alg[atoi(conn_query->ikeCryptAlg)], MAX_CHARS);
    strncat(tmp, "-", MAX_CHARS);
    strncat(tmp, enum_ike_auth_alg[atoi(conn_query->ikeAuthAlg)], MAX_CHARS);
    strncat(tmp, "-", MAX_CHARS);
    strncat(tmp, enum_ike_dh_alg[atoi(conn_query->ikeDHAlg)], MAX_CHARS);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ike conn proposals: %s\n", tmp);
    strncpy(ipsec_conn->proposals, tmp, MAX_CHARS);

    strncpy(tmp, enum_crypt_alg[atoi(conn_query->ipsecCryptAlg)], MAX_CHARS);
    strncat(tmp, "-", MAX_CHARS);
    strncat(tmp, enum_child_auth_alg[atoi(conn_query->ipsecAuthAlg)], MAX_CHARS);
    if (strncmp(conn_query->ipsecDHAlg, "0", MAX_CHARS) != 0)
    {
        strncat(tmp, "-", MAX_CHARS);
        strncat(tmp, enum_child_dh_alg[atoi(conn_query->ipsecDHAlg)], MAX_CHARS);
    }
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ike conn child proposals: %s\n", tmp);
    strncpy(ipsec_conn->childrenEspProposals, tmp, MAX_CHARS);

    strncpy(ipsec_conn->version, "2", MAX_CHARS);
    strncpy(ipsec_conn->mobike, "no", MAX_CHARS);

    return AGC_STATUS_SUCCESS;
}

int config_add_conn(conn_query_t *conn_query)
{
    int result = AGC_STATUS_SUCCESS, ret;
    char *success, *errmsg;
    vici_conn_t *vici_conn;
    agc_db_t *db;
    ipsec_conn_t ipsec_conn;

    malloc_conn(&ipsec_conn);
    if (AGC_STATUS_SUCCESS != verify_conn_config(conn_query, &ipsec_conn, CONFIG_ADD))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ike conn verify failure\n");
        free_conn(&ipsec_conn);
        result = ERROR_VERIFY_FAILURE;
        return result;
    }

    vici_conn = get_vici_conn();
    success = (char *)malloc(MAX_CHARS);
    errmsg = (char *)malloc(MAX_CHARS);
    ret = add_conn(vici_conn, &ipsec_conn, success, errmsg);
    if (0 == ret && 0 == strcmp(success, "yes"))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "vici add connect success\n");
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vici add connect failure: %s\n", errmsg);
        result = ERROR_TAKE_EFFECT_FAILURE;
    }

    if (AGC_STATUS_SUCCESS == result)
    {
        add_conn_file(&ipsec_conn);

        db = get_agc_db();
        if (AGC_STATUS_SUCCESS != save_conn_to_db(db, conn_query))
        {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vici conn save db failure\n");
            result = ERROR_SAVE_DB_FAILURE;
        }
    }
    free(success);
    free(errmsg);
    free_conn(&ipsec_conn);
    return result;
}

int config_del_conn(int tableId)
{
    int result = AGC_STATUS_SUCCESS, ret;
    char *success, *errmsg;
    vici_conn_t *vici_conn;
    conn_query_t conn_query;
    agc_db_t *db;

    vici_conn = get_vici_conn();
    db = get_agc_db();
    malloc_conn_query(&conn_query);

    if (AGC_STATUS_SUCCESS != get_conn_by_id(db, tableId, &conn_query))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "conn id: %d not exist\n", tableId);
        free_conn_query(&conn_query);
        return ERROR_RECORD_NO_EXIST;
    }

    success = (char *)malloc(MAX_CHARS);
    errmsg = (char *)malloc(MAX_CHARS);
    ret = unload_conn(vici_conn, conn_query.connName, success, errmsg);
    if (0 == ret && 0 == strcmp(success, "yes"))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "vici unload connect success\n");
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vici unload connect failure: %s\n", errmsg);
        //result = ERROR_TAKE_EFFECT_FAILURE;
    }

    if (0 == delete_conn_file(conn_query.connName))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "remove conf file success\n");
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "remove conf file failure\n");
        //result = ERROR_TAKE_EFFECT_FAILURE;
    }

    if (AGC_STATUS_SUCCESS != delete_conn_by_id(db, tableId))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vici conn delete db failure\n");
        result = ERROR_SAVE_DB_FAILURE;
    }

    free(success);
    free(errmsg);
    free_conn_query(&conn_query);
    return result;
}

int config_mod_conn(conn_query_t *conn_query)
{
    int result = AGC_STATUS_SUCCESS, ret;
    char *success, *errmsg;
    vici_conn_t *vici_conn;
    agc_db_t *db;
    ipsec_conn_t ipsec_conn;

    db = get_agc_db();
    if (SECGW_DB_NOT_FIND == find_conn_by_id(db, conn_query->tableId))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "mod conn id: %d not exist\n", conn_query->tableId);
        return ERROR_RECORD_NO_EXIST;
    }

    malloc_conn(&ipsec_conn);
    if (AGC_STATUS_SUCCESS != verify_conn_config(conn_query, &ipsec_conn, CONFIG_MOD))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ike conn verify failure\n");
        free_conn(&ipsec_conn);
        result = ERROR_VERIFY_FAILURE;
        return result;
    }

    vici_conn = get_vici_conn();
    success = (char *)malloc(MAX_CHARS);
    errmsg = (char *)malloc(MAX_CHARS);
    ret = add_conn(vici_conn, &ipsec_conn, success, errmsg);
    if (0 == ret && 0 == strcmp(success, "yes"))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "vici mod connect success\n");
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vici mod connect failure: %s\n", errmsg);
        result = ERROR_TAKE_EFFECT_FAILURE;
    }

    if (AGC_STATUS_SUCCESS == result)
    {
        delete_conn_file(conn_query->connName);
        add_conn_file(&ipsec_conn);

        if (AGC_STATUS_SUCCESS != update_conn_to_db(db, conn_query))
        {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vici conn mod db failure\n");
            result = ERROR_SAVE_DB_FAILURE;
        }
    }
    free(success);
    free(errmsg);
    free_conn(&ipsec_conn);
    return result;
}

int config_query_conns(int page_index, int page_size)
{
    agc_db_t *db;

    if (page_size <= SECGW_MAX_QUERY_ROWS)
    {
        db = get_agc_db();
        if (AGC_STATUS_SUCCESS == get_conns_from_db(db, page_index, page_size))
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "conn query finish\n");
            return AGC_STATUS_SUCCESS;
        }
        else
        {
            return AGC_STATUS_FALSE;
        }
    }
    return AGC_STATUS_FALSE;
}

int display_ipsec_conn(int page_index, int page_size)
{
    agc_db_t *db;

    if (page_size <= SECGW_MAX_QUERY_ROWS)
    {
        db = get_agc_db();
        if (AGC_STATUS_SUCCESS == get_ike_info_from_db(db, page_index, page_size))
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ike info query finish\n");
            return AGC_STATUS_SUCCESS;
        }
        else
        {
            return AGC_STATUS_FALSE;
        }
    }
    return AGC_STATUS_FALSE;
}

int config_add_secret(ipsec_secret_t *secret)
{
    char *success, *errmsg;
    int ret, result = AGC_STATUS_SUCCESS;
    agc_db_t *db;
    vici_conn_t *vici_conn;

    db = get_agc_db();
    if (SECGW_DB_FIND == find_secret_by_name(db, secret))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "secret exists");
        return ERROR_VERIFY_FAILURE;
    }

    vici_conn = get_vici_conn();
    success = (char *)malloc(MAX_CHARS);
    errmsg = (char *)malloc(MAX_CHARS);
    ret = load_secret(vici_conn, secret, success, errmsg);
    if (0 == ret && 0 == strcmp(success, "yes"))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "vici load secret success");
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vici load secret failure: %s", errmsg);
        result = ERROR_TAKE_EFFECT_FAILURE;
    }
    free(success);
    free(errmsg);

    if (AGC_STATUS_SUCCESS == result)
    {
        create_secret_file(secret);

        if (AGC_STATUS_SUCCESS == save_secret_to_db(db, secret))
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "secret save success");
        }
        else
        {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "secret save failure");
            result = ERROR_SAVE_DB_FAILURE;
        }
    }
    return result;
}

int config_del_secret(int tableId)
{
    char *success, *errmsg;
    int ret, result = AGC_STATUS_SUCCESS;
    agc_db_t *db;
    ipsec_secret_t secret;
    vici_conn_t *vici_conn;

    db = get_agc_db();
    malloc_secret(&secret);
    if (AGC_STATUS_SUCCESS != get_secret_by_id(db, tableId, &secret))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "secret no exist\n");
        free_secret(&secret);
        return ERROR_RECORD_NO_EXIST;
    }

    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "delete secret id:%s secret:%s\n",secret.id,secret.secret);

    vici_conn = get_vici_conn();
    success = (char *)malloc(MAX_CHARS);
    errmsg = (char *)malloc(MAX_CHARS);
    ret = unload_secret(vici_conn, secret.id, success, errmsg);
    if (0 == ret && 0 == strcmp(success, "yes"))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "vici unload secret success\n");
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vici unload secret failure: %s\n", errmsg);
    }

    delete_secret_file(secret.name);
    free(success);
    free(errmsg);
    free_secret(&secret);

    if (AGC_STATUS_SUCCESS == delete_secret_by_id(db, tableId))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "secret delete success\n");
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "secret delete failure\n");
        result = ERROR_SAVE_DB_FAILURE;
    }

    return result;
}

int config_mod_secret(ipsec_secret_t *secret)
{
    char *success, *errmsg;
    int ret, result = AGC_STATUS_SUCCESS;
    agc_db_t *db;
    vici_conn_t *vici_conn;

    db = get_agc_db();
    if (SECGW_DB_NOT_FIND == find_secret_by_id(db, secret->tableId))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "mod secret id: %d not exist\n", secret->tableId);
        return ERROR_RECORD_NO_EXIST;
    }

    vici_conn = get_vici_conn();
    success = (char *)malloc(MAX_CHARS);
    errmsg = (char *)malloc(MAX_CHARS);
    ret = load_secret(vici_conn, secret, success, errmsg);
    if (0 == ret && 0 == strcmp(success, "yes"))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "vici load secret success\n");
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vici load secret failure: %s\n", errmsg);
        result = ERROR_TAKE_EFFECT_FAILURE;
    }
    free(success);
    free(errmsg);

    if (AGC_STATUS_SUCCESS == result)
    {
        delete_secret_file(secret->name);
        create_secret_file(secret);

        if (AGC_STATUS_SUCCESS == update_secret_to_db(db, secret))
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "secret update success\n");
        }
        else
        {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "secret update failure\n");
            result = ERROR_SAVE_DB_FAILURE;
        }
    }
    return result;
}

int config_query_secrets(int page_index, int page_size)
{
    agc_db_t *db;

    if (page_size <= SECGW_MAX_QUERY_ROWS)
    {
        db = get_agc_db();
        if (AGC_STATUS_SUCCESS == get_secret_from_db(db, page_index, page_size))
        {
            return AGC_STATUS_SUCCESS;
        }
        else
        {
            return AGC_STATUS_FALSE;
        }
    }
    return AGC_STATUS_FALSE;
}

int config_add_cert(ipsec_cert_t *cert)
{
    char filename[MAX_CHARS], *success, *errmsg;
    agc_db_t *db;
    FILE *pFile;
    int ret, result = AGC_STATUS_SUCCESS;

    db = get_agc_db();
    if (SECGW_DB_FIND == find_cert_from_db(db, cert))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "cert is exist\n");
        return ERROR_VERIFY_FAILURE;
    }

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
        return ERROR_VERIFY_FAILURE;
    }
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "add cert file: %s\n", filename);

    pFile = fopen(filename, "r");
    if (NULL == pFile)
    {
        printf("open file error\n");
        return ERROR_VERIFY_FAILURE;
    }

    success = (char *)malloc(MAX_CHARS);
    errmsg = (char *)malloc(MAX_CHARS);
    if (enum_cert == cert->certType)
    {
        ret = vici_load_cert(get_vici_conn(), pFile, success, errmsg);
    }
    else
    {
        ret = vici_load_key(get_vici_conn(), pFile, success, errmsg);
    }
    if (0 == ret && 0 == strcmp(success, "yes"))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "vici load secret success\n");
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vici load secret failure: %s\n", errmsg);
        result = ERROR_TAKE_EFFECT_FAILURE;
    }
    free(success);
    free(errmsg);
    fclose(pFile);

    if (AGC_STATUS_SUCCESS == result)
    {
        if (AGC_STATUS_SUCCESS == save_cert_to_db(db, cert))
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "cert save success\n");
        }
        else
        {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "cert save failure\n");
            result = ERROR_SAVE_DB_FAILURE;
        }
    }
    return result;
}

int config_del_cert(int tableId)
{
    agc_db_t *db;
    ipsec_cert_t cert;

    db = get_agc_db();
    malloc_cert(&cert);
    if (AGC_STATUS_SUCCESS != get_cert_by_id(db, tableId, &cert))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "cert not exist\n");
        free_cert(&cert);
        return ERROR_RECORD_NO_EXIST;
    }

    /*if (AGC_STATUS_SUCCESS != delete_cert_file(&cert))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "cert file delete failure\n");
        free_cert(&cert);
        return ERROR_TAKE_EFFECT_FAILURE;
    }*/
    free_cert(&cert);

    if (AGC_STATUS_SUCCESS == delete_cert_by_id(db, tableId))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "cert delete success\n");
        return AGC_STATUS_SUCCESS;
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "cert delete failure\n");
        return ERROR_SAVE_DB_FAILURE;
    }
}

int config_mod_cert(ipsec_cert_t *cert)
{
    char filename[MAX_CHARS], *success, *errmsg;
    agc_db_t *db;
    FILE *pFile;
    int ret, result = AGC_STATUS_SUCCESS;
    ipsec_cert_t old_cert;

    db = get_agc_db();
    malloc_cert(&old_cert);
    if (AGC_STATUS_SUCCESS != get_cert_by_id(db, cert->tableId, &old_cert))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "cert not exist\n");
        free_cert(&old_cert);
        return ERROR_RECORD_NO_EXIST;
    }

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
        return ERROR_VERIFY_FAILURE;
    }
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "mod cert file: %s\n", filename);

    pFile = fopen(filename, "r");
    if (NULL == pFile)
    {
        printf("open file error\n");
        return ERROR_VERIFY_FAILURE;
    }

    success = (char *)malloc(MAX_CHARS);
    errmsg = (char *)malloc(MAX_CHARS);
    if (enum_cert == cert->certType)
    {
        ret = vici_load_cert(get_vici_conn(), pFile, success, errmsg);
    }
    else
    {
        ret = vici_load_key(get_vici_conn(), pFile, success, errmsg);
    }
    if (0 == ret && 0 == strcmp(success, "yes"))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "vici load secret success\n");
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vici load secret failure: %s\n", errmsg);
        result = ERROR_TAKE_EFFECT_FAILURE;
    }
    free(success);
    free(errmsg);
    fclose(pFile);

    if (AGC_STATUS_SUCCESS == result)
    {
        if (AGC_STATUS_SUCCESS == update_cert_to_db(db, cert))
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "cert mod success\n");
        } else
        {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "cert mod failure\n");
            result = ERROR_SAVE_DB_FAILURE;
        }
    }
    return result;
}

int config_query_certs(int page_index, int page_size)
{
    agc_db_t *db;

    if (page_size <= SECGW_MAX_QUERY_ROWS)
    {
        db = get_agc_db();
        if (AGC_STATUS_SUCCESS == get_cert_from_db(db, page_index, page_size))
        {
            return AGC_STATUS_SUCCESS;
        }
        else
        {
            return AGC_STATUS_FALSE;
        }
    }
    return AGC_STATUS_FALSE;
}
