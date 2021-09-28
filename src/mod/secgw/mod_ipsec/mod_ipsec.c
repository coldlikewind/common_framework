#include "agc_db.h"
#include "agc_json.h"
#include "agc_api.h"
#include "libvici.h"
#include "vici_api.h"
#include "mod_ipsec.h"
#include "secgw_db_api.h"
#include "secgw_config_api.h"

AGC_MODULE_LOAD_FUNCTION(mod_ipsec_load);
AGC_MODULE_SHUTDOWN_FUNCTION(mod_ipsec_shutdown);
AGC_MODULE_DEFINITION(mod_ipsec, mod_ipsec_load, mod_ipsec_shutdown, NULL);

secgw_api_command_t secgw_api_commands[] = {
        {"add_ipsec_conn", secgw_add_ipsec_conn_api, "", ""},
        {"rmv_ipsec_conn", secgw_rmv_ipsec_conn_api, "", ""},
        {"mod_ipsec_conn", secgw_mod_ipsec_conn_api, "", ""},
        {"list_ipsec_conn", secgw_list_ipsec_conn_api, "", ""},
        {"add_ipsec_secret", secgw_add_ipsec_secret_api, "", ""},
        {"rmv_ipsec_secret", secgw_rmv_ipsec_secret_api, "", ""},
        {"mod_ipsec_secret", secgw_mod_ipsec_secret_api, "", ""},
        {"list_ipsec_secret", secgw_list_ipsec_secret_api, "", ""},
        {"add_ipsec_cert", secgw_add_ipsec_cert_api, "", ""},
        {"rmv_ipsec_cert", secgw_rmv_ipsec_cert_api, "", ""},
        {"mod_ipsec_cert", secgw_mod_ipsec_cert_api, "", ""},
        {"list_ipsec_cert", secgw_list_ipsec_cert_api, "", ""},
        {"display_ipsec_conn", secgw_display_ipsec_conn_api, "", ""},
};

vici_conn_t *m_viciConn = NULL;
agc_db_t *m_db = NULL;
static agc_event_t *timer_event = NULL;

static agc_event_node_t *event_node = NULL;

char *config_return_msg[] = {"ok", "data verify failure", "db operation failure", "take effect failure", "record not exist"};

vici_conn_t *get_vici_conn()
{
    return m_viciConn;
}

agc_db_t *get_agc_db()
{
    return m_db;
}

int CheckExecCmdResult(const char* cmd)
{
    FILE *ptr;
    int ret;

    if((ptr=popen(cmd, "r")) != NULL)
    {
        ret = pclose(ptr);
        if (ret == 0)
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

int ViciInit()
{
    int init_result = AGC_STATUS_SUCCESS;
    char *vici_ike_updown_event = "ike-updown";
    char *vici_child_updown_event = "child-updown";
    char *vici_ike_rekey_event = "ike-rekey";
    char *vici_child_rekey_event = "child-rekey";

    vici_init();
    m_viciConn = vici_connect(NULL);
    if (m_viciConn) {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "vici connect success\n");
        /*if (vici_register(m_viciConn, vici_ike_updown_event, parseIkeEvent, NULL) == 0)
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO,"ike-updown register success\n");
        }
        else
        {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"ike-updown register failure\n");
            init_result = AGC_STATUS_FALSE;
        }*/
        if (vici_register(m_viciConn, vici_child_updown_event, parseIkeEvent, NULL) == 0)
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO,"child-updown register success\n");
        }
        else
        {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"child-updown register failure\n");
            init_result = AGC_STATUS_FALSE;
        }
        /*if (vici_register(m_viciConn, vici_ike_rekey_event, parseIkeEvent, NULL) == 0)
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO,"ike-rekey register success");
        }
        else
        {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"ike-rekey register failure");
            init_result = AGC_STATUS_FALSE;
        }
        if (vici_register(m_viciConn, vici_child_rekey_event, parseIkeEvent, NULL) == 0)
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO,"child-rekey register success");
        }
        else
        {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"child-rekey register failure");
            init_result = AGC_STATUS_FALSE;
        }*/
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"connecting failed: %s\n", strerror(errno));
        init_result = AGC_STATUS_FALSE;
    }
    if (init_result != AGC_STATUS_SUCCESS)
    {
        if (m_viciConn)
        {
            vici_disconnect(m_viciConn);
        }
        vici_deinit();
    }
    else
    {
        if (AGC_STATUS_SUCCESS == CheckExecCmdResult("swanctl --load-all"))
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO,"ipsec load all ok\n");
            init_result = AGC_STATUS_SUCCESS;
        }
        else
        {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"ipsec load all failure\n");
            init_result = AGC_STATUS_FALSE;
        }
    }
    agc_log_printf(AGC_LOG, AGC_LOG_INFO,"vici init finish\n");
    return init_result;
}

int secgw_db_init()
{
    if (!(m_db = agc_db_open_file(SECGW_DB_FILENAME)))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "secgw db open failure\n");
        return AGC_STATUS_FALSE;
    }
    secgw_db_api_init();
    return AGC_STATUS_SUCCESS;
}

void parseIkeEvent(void *null, char *name, vici_res_t *res)
{
    ike_query_t ikeQueryInfo;

    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "receive ipsec event\n");
    malloc_ike_query(&ikeQueryInfo);
    ikeQueryInfo.isIkeSa = AGC_TRUE;
    ikeQueryInfo.isChildSa = AGC_FALSE;
    ikeQueryInfo.isOld = AGC_FALSE;

    if (vici_parse_cb(res, vici_parse_section_cb, vici_parse_key_value_cb, vici_parse_list_item_cb, (void *)&ikeQueryInfo) != 0)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "parsing failed: %s\n", strerror(errno));
    }
    print_ike_query_info(&ikeQueryInfo);
    if (AGC_STATUS_SUCCESS == verify_ike_info_to_db(m_db, &ikeQueryInfo))
    {
        if (NULL == strstr(ikeQueryInfo.ikeState, "DELET"))
        {
            save_ike_info_to_db(m_db, &ikeQueryInfo);
        }
        else
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ike state delete: %s, not save\n", ikeQueryInfo.ikeState);
        }
    }
    secgw_ike_conn_alarm(&ikeQueryInfo);
    free_ike_query(&ikeQueryInfo);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "event parse end\n");
}

AGC_STANDARD_API(secgw_api_main)
{
    return secgw_main_real(cmd, stream);
}

AGC_MODULE_LOAD_FUNCTION(mod_ipsec_load)
{
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ipsec init enter.\n");

	*module_interface = agc_loadable_module_create_interface(pool, modname);

    agc_api_register("secgw", "secgw API", "syntax", secgw_api_main);

    if (agc_event_bind_removable("", EVENT_ID_JSONCMD, secgw_message_callback, &event_node) != AGC_STATUS_SUCCESS)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "agc_event_bind_removable [fail].\n");
        return AGC_STATUS_FALSE;
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "agc event bind success\n");
    }

    if (AGC_STATUS_SUCCESS != ViciInit())
    {
        return AGC_STATUS_FALSE;
    }

	if (AGC_STATUS_SUCCESS != secgw_db_init())
    {
	    return AGC_STATUS_FALSE;
    }

    if (agc_event_create_callback(&timer_event, EVENT_NULL_SOURCEID, NULL, secgw_timer_callback) != AGC_STATUS_SUCCESS)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sys info timer agc_event_create [fail].\n");
        return AGC_STATUS_FALSE;
    }

    agc_timer_add_timer(timer_event, 10000);

    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ipsec init finish.\n");
    return AGC_STATUS_SUCCESS;
}

AGC_MODULE_SHUTDOWN_FUNCTION(mod_ipsec_shutdown)
{
    if (m_viciConn)
    {
        vici_disconnect(m_viciConn);
    }
    vici_deinit();
    if (m_db)
    {
        agc_db_close(m_db);
    }
    secgw_db_api_deinit();
    if (agc_event_unbind(&event_node) == AGC_STATUS_SUCCESS)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "agc_event_unbind [ok].\n");
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "agc_event_unbind [fail].\n");
    }
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ipsec shutdown success.\n");
    return AGC_STATUS_SUCCESS;
}

void secgw_timer_callback(void *data)
{
    agc_timer_add_timer(timer_event, 10000);
    if (0 != vici_get_version(get_vici_conn()))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vici connect failure\n");
        if (AGC_STATUS_SUCCESS != ViciInit())
        {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vici restore again\n");
        }
    }
    else
    {
        //agc_log_printf(AGC_LOG, AGC_LOG_INFO, "vici connect normal\n");
    }
}

void secgw_message_callback(void *data)
{
    agc_event_t *event = (agc_event_t *)data;
    const char *uuid = NULL;
    const char *command = NULL;
    const char *cert_file_name = NULL, *cert_file_type = NULL;
    const char *cert_content = NULL;
    char *body = NULL;
    int result = -1;

    if (!event)
    {
        return;
    }

    uuid = agc_event_get_header(event, EVENT_HEADER_UUID);
    if (uuid)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "bind_callback received uuid %s.\n", uuid);
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "bind_callback get uuid failure.\n");
        return;
    }

    command = agc_event_get_header(event, EVENT_HEADER_SUBNAME);
    if (command)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "bind_callback received command %s.\n", command);
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "bind_callback get command failure.\n");
        return;
    }

    if (0 == strncmp(command, "upload_cert_file", MAX_CHARS))
    {
        cert_file_name = agc_event_get_header(event, "file_name");
        cert_file_type = agc_event_get_header(event, "file_type");
        cert_content = agc_event_get_body(event);
        if (cert_file_name && cert_content && cert_file_type)
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "cert file name: %s type: %s\n", cert_file_name, cert_file_type);
            if (0 == strncmp(cert_file_type, "0", MAX_CHARS))
            {
                result = add_cert_file(cert_file_name, cert_content, enum_cert);
            }
            else if (0 == strncmp(cert_file_type, "1", MAX_CHARS))
            {
                result = add_cert_file(cert_file_name, cert_content, enum_private_key);
            }
        }
        else
        {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "cert file name and content parse failure.\n");
        }
        secgw_make_general_body(result, &body);
        secgw_fire_event((char *)uuid, body);
        free(body);
    }

}

agc_status_t secgw_main_real(const char *cmd, agc_stream_handle_t *stream)
{
    char *cmdbuf = NULL;
    agc_status_t status = AGC_STATUS_SUCCESS;
    int argc, i;
    int found = 0;
    char *argv[25] = { 0 };

    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ipsec main enter.\n");

    if (!(cmdbuf = strdup(cmd))) {
        return status;
    }

    argc = agc_separate_string(cmdbuf , ' ', argv, (sizeof(argv) / sizeof(argv[0])));

    if (argc && argv[0]) {
        for (i = 0; i < SECGW_API_SIZE; i++) {
            if (strcasecmp(argv[0], secgw_api_commands[i].pname) == 0) {
                secgw_api_func pfunc = secgw_api_commands[i].func;
                int sub_argc = argc - 1;
                char **sub_argv = NULL;

                if (argc > 1)
                    sub_argv = &argv[1];

                pfunc(stream, sub_argc, sub_argv);

                found = 1;
                break;
            }
        }

        if (!found) {
            status = AGC_STATUS_GENERR;
            stream->write_function(stream, "command '%s' not found.\n", argv[0]);
        }
    }

    agc_safe_free(cmdbuf);

    return status;

}

void secgw_make_general_body(int ret, char **body)
{
    cJSON_Hooks json_hooks;
    cJSON *json_object;

    json_hooks.malloc_fn = malloc;
    json_hooks.free_fn = free;
    cJSON_InitHooks(&json_hooks);

    json_object = cJSON_CreateObject();
    cJSON_AddNumberToObject(json_object, "ret", ret);
    cJSON_AddStringToObject(json_object, "msg", config_return_msg[abs(ret)]);

    *body = cJSON_Print(json_object);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "secgw json body: %s.\n", *body);

    cJSON_Delete(json_object);
}

int secgw_fire_event(char *uuid, char *body)
{
    agc_event_t *new_event = NULL;
    char *event_json;

    if ((agc_event_create(&new_event, EVENT_ID_CMDRESULT, EVENT_NULL_SOURCEID) != AGC_STATUS_SUCCESS) || !new_event)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "secgw create event [fail].\n");
        return AGC_STATUS_FALSE;
    }
    if (uuid != NULL)
    {
        if (agc_event_add_header(new_event, EVENT_HEADER_UUID, "%s", uuid) != AGC_STATUS_SUCCESS) {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "secgw add header [fail].\n");
            agc_event_destroy(&new_event);
            return AGC_STATUS_FALSE;
        }
    }
    else
    {
        if (agc_event_add_header(new_event, EVENT_HEADER_UUID, "_null") != AGC_STATUS_SUCCESS) {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "secgw add header [fail].\n");
            agc_event_destroy(&new_event);
            return AGC_STATUS_FALSE;
        }
    }

    if (agc_event_set_body(new_event, body) != AGC_STATUS_SUCCESS)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "secgw set body [fail].\n");
        agc_event_destroy(&new_event);
        return AGC_STATUS_FALSE;
    }

    agc_event_serialize_json(new_event, &event_json);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "secgw event body: %s\n", event_json);
    free(event_json);

    if (agc_event_fire(&new_event) != AGC_STATUS_SUCCESS) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "secgw event fire [fail].\n");
        agc_event_destroy(&new_event);
        return AGC_STATUS_FALSE;
    }
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "secgw event fire [ok].\n");
    return AGC_STATUS_SUCCESS;
}

void secgw_add_ipsec_conn_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result;
    conn_query_t conn_query;
    char *body = NULL;

    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "secgw add conn param number: %d.\n", argc);
    malloc_conn_query(&conn_query);
    conn_db_callback(&conn_query, argc, argv, NULL);
    result = config_add_conn(&conn_query);
    free_conn_query(&conn_query);

    secgw_make_general_body(result, &body);
    secgw_fire_event(stream->uuid, body);
    free(body);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "secgw add conn finish.\n");
}

void secgw_mod_ipsec_conn_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result;
    conn_query_t conn_query;
    char *body = NULL;

    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "secgw mod conn param number: %d.\n", argc);
    malloc_conn_query(&conn_query);
    conn_db_callback(&conn_query, argc, argv, NULL);
    result = config_mod_conn(&conn_query);
    free_conn_query(&conn_query);

    secgw_make_general_body(result, &body);
    secgw_fire_event(stream->uuid, body);
    free(body);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "secgw mod conn finish.\n");
}

void secgw_rmv_ipsec_conn_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int tableId, result;
    char *body = NULL;

    tableId = atoi(argv[0]);
    result = config_del_conn(tableId);
    secgw_make_general_body(result, &body);
    secgw_fire_event(stream->uuid, body);
    free(body);
}

void secgw_make_list_conn_body(int ret, char **body)
{
    cJSON_Hooks json_hooks;
    cJSON *json_object, *json_record;

    json_hooks.malloc_fn = malloc;
    json_hooks.free_fn = free;
    cJSON_InitHooks(&json_hooks);

    json_object = cJSON_CreateObject();
    cJSON_AddNumberToObject(json_object, "ret", ret);
    cJSON_AddNumberToObject(json_object, "total", g_conn_total_rows);
    if (AGC_STATUS_SUCCESS == ret && g_conn_total_rows > 0)
    {
        json_record = encode_conns_to_json();
        cJSON_AddItemToObject(json_object, "records", json_record);
    }

    *body = cJSON_Print(json_object);
    cJSON_Delete(json_object);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "secgw json body: %s.\n", *body);
}

void secgw_list_ipsec_conn_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int page_index, page_size, result;
    char *body = NULL;

    g_conn_total_rows = 0;
    result = get_total_conns(get_agc_db(), &g_conn_total_rows);
    if (AGC_STATUS_SUCCESS == result && g_conn_total_rows > 0)
    {
        page_index = atoi(argv[0]);
        page_size = atoi(argv[1]);
        result = config_query_conns(page_index, page_size);
    }

    secgw_make_list_conn_body(result, &body);
    secgw_fire_event(stream->uuid, body);
    free(body);
}

void secgw_make_display_conn_body(int ret, char **body)
{
    cJSON_Hooks json_hooks;
    cJSON *json_object, *json_record;

    json_hooks.malloc_fn = malloc;
    json_hooks.free_fn = free;
    cJSON_InitHooks(&json_hooks);

    json_object = cJSON_CreateObject();
    cJSON_AddNumberToObject(json_object, "ret", ret);
    cJSON_AddNumberToObject(json_object, "total", g_ike_info_total_rows);
    if (AGC_STATUS_SUCCESS == ret && g_ike_info_total_rows > 0)
    {
        json_record = encode_ike_info_to_json();
        cJSON_AddItemToObject(json_object, "records", json_record);
    }

    *body = cJSON_Print(json_object);
    cJSON_Delete(json_object);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "secgw json body: %s.\n", *body);
}

void secgw_display_ipsec_conn_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int page_index, page_size, result;
    char *body = NULL;

    g_ike_info_total_rows = 0;
    result = get_total_ike_infos(get_agc_db(), &g_ike_info_total_rows);
    if (AGC_STATUS_SUCCESS == result && g_ike_info_total_rows > 0)
    {
        page_index = atoi(argv[0]);
        page_size = atoi(argv[1]);
        result = display_ipsec_conn(page_index, page_size);
    }

    secgw_make_display_conn_body(result, &body);
    secgw_fire_event(stream->uuid, body);
    free(body);
}

void secgw_add_ipsec_secret_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result;
    ipsec_secret_t secret;
    char *body = NULL;

    malloc_secret(&secret);
    secret_db_callback(&secret, argc, argv, NULL);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "secret: %s, %s, %s.\n", secret.name, secret.id, secret.secret);

    result = config_add_secret(&secret);
    free_secret(&secret);

    secgw_make_general_body(result, &body);
    secgw_fire_event(stream->uuid, body);
    free(body);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "secgw add secret finish.\n");
}

void secgw_mod_ipsec_secret_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result;
    ipsec_secret_t secret;
    char *body = NULL;

    malloc_secret(&secret);
    secret_db_callback(&secret, argc, argv, NULL);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "secret: %s, %s, %s.\n", secret.name, secret.id, secret.secret);

    result = config_mod_secret(&secret);
    free_secret(&secret);

    secgw_make_general_body(result, &body);
    secgw_fire_event(stream->uuid, body);
    free(body);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "secgw mod secret finish.\n");
}

void secgw_rmv_ipsec_secret_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int tableId, result;
    char *body = NULL;

    tableId = atoi(argv[0]);
    result = config_del_secret(tableId);
    secgw_make_general_body(result, &body);
    secgw_fire_event(stream->uuid, body);
    free(body);
}

void secgw_make_list_secret_body(int ret, char **body)
{
    cJSON_Hooks json_hooks;
    cJSON *json_object, *json_record;

    json_hooks.malloc_fn = malloc;
    json_hooks.free_fn = free;
    cJSON_InitHooks(&json_hooks);

    json_object = cJSON_CreateObject();
    cJSON_AddNumberToObject(json_object, "ret", ret);
    cJSON_AddNumberToObject(json_object, "total", g_secret_total_rows);
    if (AGC_STATUS_SUCCESS == ret && g_secret_total_rows)
    {
        json_record = encode_secret_to_json();
        cJSON_AddItemToObject(json_object, "records", json_record);
    }

    *body = cJSON_Print(json_object);
    cJSON_Delete(json_object);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "secgw json body: %s.\n", *body);
}

void secgw_list_ipsec_secret_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int page_index, page_size, result;
    char *body = NULL;

    g_secret_total_rows = 0;
    result = get_total_secrets(get_agc_db(), &g_secret_total_rows);
    if (AGC_STATUS_SUCCESS == result && g_secret_total_rows > 0)
    {
        page_index = atoi(argv[0]);
        page_size = atoi(argv[1]);
        result = config_query_secrets(page_index, page_size);
    }

    secgw_make_list_secret_body(result, &body);
    secgw_fire_event(stream->uuid, body);
    free(body);
}

void secgw_add_ipsec_cert_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result;
    ipsec_cert_t cert;
    char *body = NULL;

    malloc_cert(&cert);
    cert_db_callback(&cert, argc, argv, NULL);

    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "add cert id:%s, type:%d, name:%s\n", cert.id, cert.certType, cert.certName);

    result = config_add_cert(&cert);
    free_cert(&cert);
    secgw_make_general_body(result, &body);
    secgw_fire_event(stream->uuid, body);
    free(body);
}

void secgw_mod_ipsec_cert_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result;
    ipsec_cert_t cert;
    char *body = NULL;

    malloc_cert(&cert);
    cert_db_callback(&cert, argc, argv, NULL);

    result = config_mod_cert(&cert);
    free_cert(&cert);
    secgw_make_general_body(result, &body);
    secgw_fire_event(stream->uuid, body);
    free(body);
}

void secgw_rmv_ipsec_cert_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int tableId, result;
    char *body = NULL;

    tableId = atoi(argv[0]);
    result = config_del_cert(tableId);
    secgw_make_general_body(result, &body);
    secgw_fire_event(stream->uuid, body);
    free(body);
}

void secgw_make_list_cert_body(int ret, char **body)
{
    cJSON_Hooks json_hooks;
    cJSON *json_object, *json_record;

    json_hooks.malloc_fn = malloc;
    json_hooks.free_fn = free;
    cJSON_InitHooks(&json_hooks);

    json_object = cJSON_CreateObject();
    cJSON_AddNumberToObject(json_object, "ret", ret);
    cJSON_AddNumberToObject(json_object, "total", g_cert_total_rows);
    if (AGC_STATUS_SUCCESS == ret && g_cert_total_rows)
    {
        json_record = encode_cert_to_json();
        cJSON_AddItemToObject(json_object, "records", json_record);
    }

    *body = cJSON_Print(json_object);
    cJSON_Delete(json_object);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "secgw json body: %s.\n", *body);
}

void secgw_list_ipsec_cert_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int page_index, page_size, result;
    char *body = NULL;

    g_cert_total_rows = 0;
    result = get_total_certs(get_agc_db(), &g_cert_total_rows);
    if (AGC_STATUS_SUCCESS == result && g_cert_total_rows > 0)
    {
        page_index = atoi(argv[0]);
        page_size = atoi(argv[1]);
        result = config_query_certs(page_index, page_size);
    }
    secgw_make_list_cert_body(result, &body);
    secgw_fire_event(stream->uuid, body);
    free(body);
}

int secgw_report_alarm(char *alarm_id, char *inst_id, char *alarm_param, char *alarm_type)
{
    agc_event_t *new_event = NULL;

    if ((agc_event_create(&new_event, EVENT_ID_ALARM, EVENT_NULL_SOURCEID) != AGC_STATUS_SUCCESS) || !new_event)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "secgw create alarm [fail].\n");
        return AGC_STATUS_FALSE;
    }

    if (agc_event_add_header(new_event, HEADER_ALARM_MOD_ID, "%s", SECGW_MO_ID) != AGC_STATUS_SUCCESS)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "secgw add mo id [fail].\n");
        agc_event_destroy(&new_event);
        return AGC_STATUS_FALSE;
    }

    if (agc_event_add_header(new_event, HEADER_ALARM_ID, "%s", alarm_id) != AGC_STATUS_SUCCESS)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "secgw add alarm id [fail].\n");
        agc_event_destroy(&new_event);
        return AGC_STATUS_FALSE;
    }

    if (agc_event_add_header(new_event, HEADER_ALARM_INST_ID, "%s", inst_id) != AGC_STATUS_SUCCESS)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "secgw add instance id [fail].\n");
        agc_event_destroy(&new_event);
        return AGC_STATUS_FALSE;
    }

    if (agc_event_add_header(new_event, HEADER_ALARM_TYPE, alarm_type) != AGC_STATUS_SUCCESS)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "secgw add alarm type [fail].\n");
        agc_event_destroy(&new_event);
        return AGC_STATUS_FALSE;
    }


    if (agc_event_add_header(new_event, HEADER_ALARM_PARAM, "%s", alarm_param) != AGC_STATUS_SUCCESS)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "secgw add alarm param [fail].\n");
        agc_event_destroy(&new_event);
        return AGC_STATUS_FALSE;
    }

    if (agc_event_fire(&new_event) != AGC_STATUS_SUCCESS) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "secgw report alarm [fail].\n");
        agc_event_destroy(&new_event);
        return AGC_STATUS_FALSE;
    }
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "secgw report alarm [ok].\n");
    return AGC_STATUS_SUCCESS;
}

void secgw_ike_conn_alarm(ike_query_t *ikeQueryInfo)
{
    char alarm_param[100];

    if (SECGW_DB_NOT_FIND == find_conn_by_name(m_db, ikeQueryInfo->ikeName))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "secgw don't find conn %s from db\n", ikeQueryInfo->ikeName);
        return;
    }
    if (strstr(ikeQueryInfo->ikeState, "ESTABLISH") && strstr(ikeQueryInfo->childSaState, "INSTALL"))
    {
        sprintf(alarm_param, "local addr=%s,remote addr=%s",ikeQueryInfo->localHost,ikeQueryInfo->remoteHost);
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ike alarm clear: %s\n", alarm_param);
        secgw_report_alarm(IKE_CONN_ALARM_ID, ikeQueryInfo->ikeName, alarm_param, ALARM_TYPE_CLEAR);
        return;
    }
    if (strstr(ikeQueryInfo->ikeState, "DELET"))
    {
        sprintf(alarm_param, "local addr=%s,remote addr=%s",ikeQueryInfo->localHost,ikeQueryInfo->remoteHost);
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ike alarm report: %s\n", alarm_param);
        secgw_report_alarm(IKE_CONN_ALARM_ID, ikeQueryInfo->ikeName, alarm_param, ALARM_TYPE_REPORT);
        return;
    }
}