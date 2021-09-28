//
// Created by count on 2020/7/16.
//

#include "agc.h"
#include "mod_sysinfo.h"
#include "SysInfo.h"
#include "SysAlarm.h"
#include "CfgNetwork.h"
#include "SysInfoDB.h"

AGC_MODULE_LOAD_FUNCTION(mod_sysinfo_load);
AGC_MODULE_SHUTDOWN_FUNCTION(mod_sysinfo_shutdown);
AGC_MODULE_DEFINITION(mod_sysinfo, mod_sysinfo_load, mod_sysinfo_shutdown, NULL);

sysinfo_api_command_t sysinfo_api_commands[] = {
        {"query_cpu_usage", sysinfo_cpu_usage_api,"", ""},
        {"query_mem_usage", sysinfo_mem_usage_api,"", ""},
        {"query_disk_usage", sysinfo_disk_usage_api,"", ""},
        {"query_if_name", sysinfo_get_network_api,"", ""},
        {"add_ip_config", sysinfo_add_ip_config_api,"", ""},
        {"rmv_ip_config", sysinfo_rmv_ip_config_api,"", ""},
        {"mod_ip_config", sysinfo_mod_ip_config_api,"", ""},
        {"list_ip_config", sysinfo_list_ip_config_api,"", ""},
        {"add_ip_route", sysinfo_add_ip_route_api,"", ""},
        {"rmv_ip_route", sysinfo_rmv_ip_route_api,"", ""},
        {"mod_ip_route", sysinfo_mod_ip_route_api,"", ""},
        {"list_ip_route", sysinfo_list_ip_route_api,"", ""},
        {"dsp_ip_config", sysinfo_dsp_ip_config_api,"", ""},
        {"dsp_ip_route", sysinfo_dsp_ip_route_api,"", ""},
        {"dsp_version", sysinfo_dsp_version_api, "", ""}
};

static const int SYSINFO_API_SIZE = sizeof(sysinfo_api_commands)/sizeof(sysinfo_api_commands[0]);
static const char SYSINF_DB_NAME[] = "sysinfo.db";
static const string CORE_VER = "V100R001C00";
static const string SECGW_VER = "V100R001C00";
static const string SIGGW_VER = "V100R001C00";
static const string DATAGW_VER = "V100R001C00";

static sysinfo_alarm cpu_alarm(cpu_usage_alarm_id);
static sysinfo_alarm mem_alarm(mem_usage_alarm_id, 80, 70, 5);
static sysinfo_alarm disk_alarm(disk_usage_alarm_id,80,70,5);
static ethnet_alarm ethnet_alarm;

static agc_event_t *timer_event = NULL;
static agc_db_t *m_db = NULL;

static string config_return_msg[] = {"ok",
                                     "data verify failure",
                                     "db operation failure",
                                     "take effect failure",
                                     "record not exist"};
static cfg_ethnet_list eth_net_list;

AGC_STANDARD_API(sysinfo_api_main)
{
    return sysinfo_main_real(cmd, stream);
}

AGC_MODULE_LOAD_FUNCTION(mod_sysinfo_load)
{
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "sys info load\n");

    *module_interface = agc_loadable_module_create_interface(pool, modname);

    agc_api_register("sysinfo", "sysinfo API", "syntax", sysinfo_api_main);

    if (agc_event_create_callback(&timer_event, EVENT_NULL_SOURCEID, NULL, sysinfo_timer_callback) != AGC_STATUS_SUCCESS)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sys info timer agc_event_create [fail].\n");
        return AGC_STATUS_FALSE;
    }

    agc_timer_add_timer(timer_event, 10000);

    ethnet_alarm.ethnet_status_init();

    if (AGC_STATUS_SUCCESS != sysinfo_db_init())
    {
        return AGC_STATUS_FALSE;
    }

    return AGC_STATUS_SUCCESS;
}

AGC_MODULE_SHUTDOWN_FUNCTION(mod_sysinfo_shutdown)
{
    return AGC_STATUS_SUCCESS;
}

agc_status_t sysinfo_main_real(const char *cmd, agc_stream_handle_t *stream)
{
    char *cmdbuf = NULL;
    agc_status_t status = AGC_STATUS_SUCCESS;
    int argc, i;
    int found = 0;
    char *argv[25] = { 0 };

    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "sys info main enter.\n");

    if (!(cmdbuf = strdup(cmd))) {
        return status;
    }

    argc = agc_separate_string(cmdbuf , ' ', argv, (sizeof(argv) / sizeof(argv[0])));

    if (argc && argv[0]) {
        for (i = 0; i < SYSINFO_API_SIZE; i++) {
            if (strcasecmp(argv[0], sysinfo_api_commands[i].pname) == 0) {
                sysinfo_api_func pfunc = sysinfo_api_commands[i].func;
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

int sysinfo_db_init()
{
    if (!(m_db = agc_db_open_file(SYSINF_DB_NAME)))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "secgw db open failure\n");
        return AGC_STATUS_FALSE;
    }
    return AGC_STATUS_SUCCESS;
}

void sysinfo_cpu_usage_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int cpuUsage;

    GetCpuUsage(cpuUsage);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "cpu usage: %d\n", cpuUsage);
}

void sysinfo_mem_usage_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int memUsage;

    GetMemUsage(memUsage);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "mem usage: %d\n", memUsage);
}

void sysinfo_disk_usage_api(agc_stream_handle_t *stream, int argc, char **argv)
{

    int diskUsage;

    GetDiskUsage(diskUsage);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "disk usage: %d\n", diskUsage);
}

cJSON *encode_query_eth_to_json(vector<query_ethnet> &query_eth_list)
{
    int i;
    cJSON_Hooks json_hooks;
    cJSON *record_array, *record_item;
    query_ethnet eth_net;

    json_hooks.malloc_fn = malloc;
    json_hooks.free_fn = free;
    cJSON_InitHooks(&json_hooks);

    record_array = cJSON_CreateArray();

    for (i = 0; i < query_eth_list.size(); i++)
    {
        eth_net = query_eth_list[i];
        record_item = cJSON_CreateObject();
        cJSON_AddStringToObject(record_item, "IfName",eth_net.if_name.c_str());
        cJSON_AddStringToObject(record_item, "IpAddr",eth_net.ip_addr.c_str());
        cJSON_AddStringToObject(record_item, "GateWay",eth_net.gateway.c_str());
        cJSON_AddStringToObject(record_item, "DNS",eth_net.DNS.c_str());
        cJSON_AddItemToArray(record_array, record_item);
    }

    return record_array;
}

void sysinfo_make_query_eth_body(int ret, char **body, vector<query_ethnet> query_eth_list)
{
    cJSON_Hooks json_hooks;
    cJSON *json_object, *json_record;

    json_hooks.malloc_fn = malloc;
    json_hooks.free_fn = free;
    cJSON_InitHooks(&json_hooks);

    json_object = cJSON_CreateObject();
    cJSON_AddNumberToObject(json_object, "ret", ret);
    cJSON_AddNumberToObject(json_object, "total", query_eth_list.size());
    if (AGC_STATUS_SUCCESS == ret && !query_eth_list.empty())
    {
        json_record = encode_query_eth_to_json(query_eth_list);
        cJSON_AddItemToObject(json_object, "records", json_record);
    }

    *body = cJSON_Print(json_object);
    cJSON_Delete(json_object);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "sysinfo eth json body: %s.\n", *body);
}

void sysinfo_make_query_ifname_body(int ret, char **body, vector<string> if_name_list)
{
    cJSON_Hooks json_hooks;
    cJSON *json_object;

    json_hooks.malloc_fn = malloc;
    json_hooks.free_fn = free;
    cJSON_InitHooks(&json_hooks);

    json_object = cJSON_CreateObject();
    cJSON_AddNumberToObject(json_object, "ret", ret);
    cJSON_AddNumberToObject(json_object, "total", if_name_list.size());
    if (!if_name_list.empty())
    {
        cJSON *record_array = cJSON_CreateArray();
        for (int i = 0; i < if_name_list.size(); i++)
        {
            cJSON *record_item = cJSON_CreateObject();
            cJSON_AddStringToObject(record_item, "IfName",if_name_list[i].c_str());
            cJSON_AddItemToArray(record_array, record_item);
        }
        cJSON_AddItemToObject(json_object, "records", record_array);
    }

    *body = cJSON_Print(json_object);
    cJSON_Delete(json_object);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "sysinfo if name json body: %s.\n", *body);
}

void sysinfo_get_network_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "enter get if name\n");

    vector<string> if_name_list;
    char *body = NULL;

    int result = get_if_name(if_name_list);
    sysinfo_make_query_ifname_body(result, &body, if_name_list);
    sysinfo_fire_event(stream->uuid, body);
    free(body);
}

void sysinfo_timer_callback(void *data)
{
    int cpuUsage, memUsage, diskUsage;

    agc_timer_add_timer(timer_event, 10000);

    GetCpuUsage(cpuUsage);
    GetMemUsage(memUsage);
    GetDiskUsage(diskUsage);

    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "cpu: %d,mem: %d,disk: %d\n", cpuUsage,memUsage,diskUsage);

    cpu_alarm.check_alarm(cpuUsage);
    mem_alarm.check_alarm(memUsage);
    disk_alarm.check_alarm(diskUsage);
    ethnet_alarm.check_alarm();
}

agc_db_t *get_agc_db()
{
    return m_db;
}

void sysinfo_make_general_body(int ret, char **body)
{
    cJSON_Hooks json_hooks;
    cJSON *json_object;

    json_hooks.malloc_fn = malloc;
    json_hooks.free_fn = free;
    cJSON_InitHooks(&json_hooks);

    json_object = cJSON_CreateObject();
    cJSON_AddNumberToObject(json_object, "ret", ret);
    cJSON_AddStringToObject(json_object, "msg", config_return_msg[abs(ret)].c_str());

    *body = cJSON_Print(json_object);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "sysinfo json body: %s.\n", *body);

    cJSON_Delete(json_object);
}

int sysinfo_fire_event(char *uuid, char *body)
{
    agc_event_t *new_event = NULL;
    char *event_json;

    if ((agc_event_create(&new_event, EVENT_ID_CMDRESULT, EVENT_NULL_SOURCEID) != AGC_STATUS_SUCCESS) || !new_event)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sysinfo create event [fail].\n");
        return AGC_STATUS_FALSE;
    }
    if (uuid != NULL)
    {
        if (agc_event_add_header(new_event, EVENT_HEADER_UUID, "%s", uuid) != AGC_STATUS_SUCCESS) {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sysinfo add header [fail].\n");
            agc_event_destroy(&new_event);
            return AGC_STATUS_FALSE;
        }
    }
    else
    {
        if (agc_event_add_header(new_event, EVENT_HEADER_UUID, "_null") != AGC_STATUS_SUCCESS) {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sysinfo add header [fail].\n");
            agc_event_destroy(&new_event);
            return AGC_STATUS_FALSE;
        }
    }

    if (agc_event_set_body(new_event, body) != AGC_STATUS_SUCCESS)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sysinfo set body [fail].\n");
        agc_event_destroy(&new_event);
        return AGC_STATUS_FALSE;
    }

    agc_event_serialize_json(new_event, &event_json);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "sysinfo event body: %s\n", event_json);
    free(event_json);

    if (agc_event_fire(&new_event) != AGC_STATUS_SUCCESS) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sysinfo event fire [fail].\n");
        agc_event_destroy(&new_event);
        return AGC_STATUS_FALSE;
    }
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "sysinfo event fire [ok].\n");
    return AGC_STATUS_SUCCESS;
}

void sysinfo_add_ip_config_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result;
    cfg_ethnet eth_net;
    char *body = NULL;

    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "sysinfo add ip param number: %d.\n", argc);
    ip_config_db_callback(&eth_net, argc, argv, NULL);
    result = cfg_add_ip_config(eth_net);

    sysinfo_make_general_body(result, &body);
    sysinfo_fire_event(stream->uuid, body);
    free(body);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "sysinfo add ip finish.\n");
}

void sysinfo_rmv_ip_config_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int tableId, result;
    char *body = NULL;

    tableId = atoi(argv[0]);
    result = cfg_rmv_ip_config(tableId);
    sysinfo_make_general_body(result, &body);
    sysinfo_fire_event(stream->uuid, body);
    free(body);
}

void sysinfo_mod_ip_config_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result;
    cfg_ethnet eth_net;
    char *body = NULL;

    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "sysinfo mod ip param number: %d.\n", argc);
    ip_config_db_callback(&eth_net, argc, argv, NULL);
    result = cfg_mod_ip_config(eth_net);

    sysinfo_make_general_body(result, &body);
    sysinfo_fire_event(stream->uuid, body);
    free(body);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "sysinfo mod ip finish.\n");
}

void sysinfo_make_list_ip_body(int ret, char **body)
{
    cJSON_Hooks json_hooks;
    cJSON *json_object, *json_record;

    json_hooks.malloc_fn = malloc;
    json_hooks.free_fn = free;
    cJSON_InitHooks(&json_hooks);

    json_object = cJSON_CreateObject();
    cJSON_AddNumberToObject(json_object, "ret", ret);
    cJSON_AddNumberToObject(json_object, "total", g_ip_config_rows);
    if (AGC_STATUS_SUCCESS == ret && g_ip_config_rows > 0)
    {
        json_record = encode_ip_config_to_json();
        cJSON_AddItemToObject(json_object, "records", json_record);
    }

    *body = cJSON_Print(json_object);
    cJSON_Delete(json_object);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "sysinfo ip json body: %s.\n", *body);
}

void sysinfo_list_ip_config_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int page_index, page_size, result;
    char *body = NULL;

    g_ip_config_rows = 0;
    result = get_ip_config_rows(g_ip_config_rows);
    if (AGC_STATUS_SUCCESS == result && g_ip_config_rows > 0)
    {
        page_index = atoi(argv[0]);
        page_size = atoi(argv[1]);
        result = cfg_list_ip_config(page_index, page_size);
    }

    sysinfo_make_list_ip_body(result, &body);
    sysinfo_fire_event(stream->uuid, body);
    free(body);
}

void sysinfo_add_ip_route_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result;
    cfg_ip_route ip_route;
    char *body = NULL;

    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "sysinfo add route param number: %d.\n", argc);
    ip_route_db_callback(&ip_route, argc, argv, NULL);
    result = cfg_add_ip_route(ip_route);

    sysinfo_make_general_body(result, &body);
    sysinfo_fire_event(stream->uuid, body);
    free(body);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "sysinfo add route finish.\n");
}

void sysinfo_rmv_ip_route_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int tableId, result;
    char *body = NULL;

    tableId = atoi(argv[0]);
    result = cfg_rmv_ip_route(tableId);
    sysinfo_make_general_body(result, &body);
    sysinfo_fire_event(stream->uuid, body);
    free(body);
}

void sysinfo_mod_ip_route_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result;
    cfg_ip_route ip_route;
    char *body = NULL;

    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "sysinfo mod route param number: %d.\n", argc);
    ip_route_db_callback(&ip_route, argc, argv, NULL);
    result = cfg_mod_ip_route(ip_route);

    sysinfo_make_general_body(result, &body);
    sysinfo_fire_event(stream->uuid, body);
    free(body);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "sysinfo mod route finish.\n");
}

void sysinfo_make_list_route_body(int ret, char **body)
{
    cJSON_Hooks json_hooks;
    cJSON *json_object, *json_record;

    json_hooks.malloc_fn = malloc;
    json_hooks.free_fn = free;
    cJSON_InitHooks(&json_hooks);

    json_object = cJSON_CreateObject();
    cJSON_AddNumberToObject(json_object, "ret", ret);
    cJSON_AddNumberToObject(json_object, "total", g_ip_route_rows);
    if (AGC_STATUS_SUCCESS == ret && g_ip_route_rows > 0)
    {
        json_record = encode_ip_route_to_json();
        cJSON_AddItemToObject(json_object, "records", json_record);
    }

    *body = cJSON_Print(json_object);
    cJSON_Delete(json_object);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "sysinfo route json body: %s.\n", *body);
}

void sysinfo_list_ip_route_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int page_index, page_size, result;
    char *body = NULL;

    g_ip_route_rows = 0;
    result = get_ip_route_rows(g_ip_route_rows);
    if (AGC_STATUS_SUCCESS == result && g_ip_route_rows > 0)
    {
        page_index = atoi(argv[0]);
        page_size = atoi(argv[1]);
        result = cfg_list_ip_route(page_index, page_size);
    }

    sysinfo_make_list_route_body(result, &body);
    sysinfo_fire_event(stream->uuid, body);
    free(body);
}

void sysinfo_dsp_ip_config_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "enter dsp ip config");

    vector<string> if_name_list;
    vector<query_ethnet> query_ethnet_list;
    char *body = NULL;

    int result = get_if_name(if_name_list);
    if (!if_name_list.empty())
    {
        result = get_if_status(if_name_list, query_ethnet_list);
    }
    sysinfo_make_query_eth_body(result, &body, query_ethnet_list);
    sysinfo_fire_event(stream->uuid, body);
    free(body);
}

cJSON *encode_query_route_to_json(vector<query_route> &query_route_list)
{
    int i;
    cJSON_Hooks json_hooks;
    cJSON *record_array, *record_item;
    query_route route;

    json_hooks.malloc_fn = malloc;
    json_hooks.free_fn = free;
    cJSON_InitHooks(&json_hooks);

    record_array = cJSON_CreateArray();

    for (i = 0; i < query_route_list.size(); i++)
    {
        route = query_route_list[i];
        record_item = cJSON_CreateObject();
        cJSON_AddStringToObject(record_item, "DstAddr",route.dst_addr.c_str());
        cJSON_AddStringToObject(record_item, "GateWay",route.gate_way.c_str());
        cJSON_AddStringToObject(record_item, "NetMask",route.net_mask.c_str());
        cJSON_AddStringToObject(record_item, "IfName",route.if_name.c_str());
        cJSON_AddItemToArray(record_array, record_item);
    }

    return record_array;
}

void sysinfo_make_query_route_body(int ret, char **body, vector<query_route> &query_route_list)
{
    cJSON_Hooks json_hooks;
    cJSON *json_object, *json_record;

    json_hooks.malloc_fn = malloc;
    json_hooks.free_fn = free;
    cJSON_InitHooks(&json_hooks);

    json_object = cJSON_CreateObject();
    cJSON_AddNumberToObject(json_object, "ret", ret);
    cJSON_AddNumberToObject(json_object, "total", query_route_list.size());
    if (AGC_STATUS_SUCCESS == ret && !query_route_list.empty())
    {
        json_record = encode_query_route_to_json(query_route_list);
        cJSON_AddItemToObject(json_object, "records", json_record);
    }

    *body = cJSON_Print(json_object);
    cJSON_Delete(json_object);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "sysinfo route json body: %s.\n", *body);
}

void sysinfo_dsp_ip_route_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "enter dsp ip route\n");

    vector<query_route> query_route_list;
    char *body = NULL;

    int result = get_route_list(query_route_list);

    sysinfo_make_query_route_body(result, &body, query_route_list);
    sysinfo_fire_event(stream->uuid, body);
    free(body);
}

void sysinfo_make_query_version_body(int device_type, char **body)
{
    cJSON_Hooks json_hooks;
    cJSON *json_object, *json_record;

    json_hooks.malloc_fn = malloc;
    json_hooks.free_fn = free;
    cJSON_InitHooks(&json_hooks);

    json_object = cJSON_CreateObject();
    cJSON_AddStringToObject(json_object, "core_ver", CORE_VER.c_str());
    if (device_type & 0x04)
    {
        cJSON_AddStringToObject(json_object, "secgw_ver", SECGW_VER.c_str());
    }
    if (device_type & 0x01)
    {
        cJSON_AddStringToObject(json_object, "siggw_ver", SIGGW_VER.c_str());
    }
    if (device_type & 0x02)
    {
        cJSON_AddStringToObject(json_object, "datagw_ver", DATAGW_VER.c_str());
    }


    *body = cJSON_Print(json_object);
    cJSON_Delete(json_object);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "sysinfo version json body: %s.\n", *body);
}

void sysinfo_dsp_version_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "enter dsp version\n");

    int device_type;
    char *body = NULL;

    device_type = atoi(argv[0]);

    sysinfo_make_query_version_body(device_type, &body);
    sysinfo_fire_event(stream->uuid, body);
    free(body);
}