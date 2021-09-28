//
// Created by count on 2020/7/29.
#include "agc.h"
#include "SysInfoDB.h"
#include "mod_sysinfo.h"

const int IP_CONFIG_TABLE_COLUMNS = 7;
const int IP_ROUTE_TABLE_COLUMNS = 6;
const int MAX_QUERY_ROWS = 50;

cfg_ethnet g_ip_config_query[MAX_QUERY_ROWS];
cfg_ip_route g_ip_route_query[MAX_QUERY_ROWS];
int g_ip_config_rows = 0;
int g_ip_route_rows = 0;


int save_ip_config(cfg_ethnet &eth_net)
{
    char *err_msg = NULL;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(2048);
    sprintf(buf, "INSERT INTO ip_config(PortName,IfName,IpAddr,NetMask,GateWay,DNS) VALUES('%s','%s','%s','%d','%s','%s')",
            eth_net.net_name.c_str(), eth_net.if_name.c_str(), eth_net.ip_addr.c_str(), eth_net.net_mask,eth_net.gateway.c_str(),eth_net.DNS.c_str());
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ip config save sql %s\n", buf);
    if (agc_db_exec(get_agc_db(), buf, NULL, NULL, &err_msg) != AGC_DB_OK)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_FALSE;
    }
    free(buf);
    agc_db_free(err_msg);
    return result;
}

int get_ip_config_by_id(cfg_ethnet *eth_net, int id)
{
    char *err_msg = NULL;
    char **query_result = NULL;
    int i, nrow, ncolumn;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "SELECT * FROM ip_config WHERE id=%d", id);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ip route query sql %s", buf);
    if (agc_db_get_table(get_agc_db(), buf, &query_result, &nrow, &ncolumn, &err_msg) == AGC_DB_OK)
    {
        if ( nrow > 0)
        {
            ip_config_db_callback((void *)eth_net, ncolumn, (query_result + ncolumn), query_result);
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

int delete_ip_config_by_id(int id)
{
    char *err_msg = NULL;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "DELETE FROM ip_config WHERE ID='%d'", id);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ip config del sql %s", buf);
    if (agc_db_exec(get_agc_db(), buf, NULL, NULL, &err_msg) != AGC_DB_OK)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_GENERR;
    }
    free(buf);
    agc_db_free(err_msg);
    return result;
}

int update_ip_config(cfg_ethnet &eth_net)
{
    char *err_msg = NULL;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(2048);
    sprintf(buf, "UPDATE ip_config SET PortName='%s',IfName='%s',IpAddr='%s',NetMask='%d',GateWay='%s',DNS='%s' WHERE ID='%d'",
            eth_net.net_name.c_str(),eth_net.if_name.c_str(),eth_net.ip_addr.c_str(),eth_net.net_mask,eth_net.gateway.c_str(),eth_net.DNS.c_str(),eth_net.id);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ip config update sql %s \n", buf);
    if (agc_db_exec(get_agc_db(), buf, NULL, NULL, &err_msg) != AGC_DB_OK)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_FALSE;
    }
    free(buf);
    agc_db_free(err_msg);
    return result;
}

int get_ip_config_rows(int &rows)
{
    char *err_msg = NULL;
    char **query_result = NULL;
    int nrow, ncolumn;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "SELECT COUNT(*) FROM ip_config");
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ip config total query sql %s\n", buf);
    if (agc_db_get_table(get_agc_db(), buf, &query_result, &nrow, &ncolumn, &err_msg) == AGC_DB_OK)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ip config total number: %s\n", query_result[1]);
        rows = atoi(query_result[1]);
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

int find_ip_config_by_name(string name)
{
    char *err_msg = NULL;
    char **query_result = NULL;
    int nrow;
    int ncolumn;
    char *sql;
    int result = AGC_STATUS_FALSE;

    sql = (char *)malloc(512);
    sprintf(sql, "select * from ip_config where ip_config.PortName='%s'", name.c_str());
    if (agc_db_get_table(get_agc_db(), sql, &query_result, &nrow, &ncolumn, &err_msg) == AGC_DB_OK)
    {
        if (nrow > 0)
        {
            result = AGC_STATUS_SUCCESS;
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
    agc_db_free(err_msg);
    agc_db_free_table(query_result);
    free(sql);
    return result;
}

int find_ip_config_by_ifname(string ifname)
{
    char *err_msg = NULL;
    char **query_result = NULL;
    int nrow;
    int ncolumn;
    char *sql;
    int result = AGC_STATUS_FALSE;

    sql = (char *)malloc(512);
    sprintf(sql, "select * from ip_config where ip_config.IfName='%s'", ifname.c_str());
    if (agc_db_get_table(get_agc_db(), sql, &query_result, &nrow, &ncolumn, &err_msg) == AGC_DB_OK)
    {
        if (nrow > 0)
        {
            result = AGC_STATUS_SUCCESS;
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
    agc_db_free(err_msg);
    agc_db_free_table(query_result);
    free(sql);
    return result;
}

int page_query_ip_config(int page_index, int page_size)
{
    char *err_msg = NULL;
    char **query_result = NULL;
    int i, nrow, ncolumn;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "SELECT * FROM ip_config LIMIT %d OFFSET %d", page_size, page_index);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ip config info query sql %s", buf);
    if (agc_db_get_table(get_agc_db(), buf, &query_result, &nrow, &ncolumn, &err_msg) == AGC_DB_OK)
    {
        g_ip_config_rows = nrow;
        for(i = 1; i < nrow+1; i++)
        {
            ip_config_db_callback(&g_ip_config_query[i-1], ncolumn, (query_result + i*ncolumn), query_result);
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

int save_ip_route(cfg_ip_route &ip_route)
{
    char *err_msg = NULL;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "INSERT INTO ip_route(RouteName,DestIpAddr,NextHop,NetMask,Priority) VALUES('%s','%s','%s','%d','%d')",
            ip_route.route_name.c_str(), ip_route.dest_ip.c_str(), ip_route.next_hop.c_str(), ip_route.net_mask,ip_route.priority);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ip route save sql %s\n", buf);
    if (agc_db_exec(get_agc_db(), buf, NULL, NULL, &err_msg) != AGC_DB_OK)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_FALSE;
    }
    free(buf);
    agc_db_free(err_msg);
    return result;
}

int get_ip_route_by_id(cfg_ip_route *ip_route, int id)
{
    char *err_msg = NULL;
    char **query_result = NULL;
    int i, nrow, ncolumn;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "SELECT * FROM ip_route WHERE id=%d", id);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ip route query sql %s", buf);
    if (agc_db_get_table(get_agc_db(), buf, &query_result, &nrow, &ncolumn, &err_msg) == AGC_DB_OK)
    {
        if ( nrow > 0)
        {
            ip_route_db_callback((void *)ip_route, ncolumn, (query_result + ncolumn), query_result);
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

int delete_ip_route_by_id(int id)
{
    char *err_msg = NULL;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "DELETE FROM ip_route WHERE ID='%d'", id);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ip route del sql %s", buf);
    if (agc_db_exec(get_agc_db(), buf, NULL, NULL, &err_msg) != AGC_DB_OK)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_GENERR;
    }
    free(buf);
    agc_db_free(err_msg);
    return result;
}

int update_ip_route(cfg_ip_route &ip_route)
{
    char *err_msg = NULL;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "UPDATE ip_route SET RouteName='%s',DestIpAddr='%s',NextHop='%s',NetMask='%d',Priority='%d' WHERE ID='%d'",
            ip_route.route_name.c_str(),ip_route.dest_ip.c_str(),ip_route.next_hop.c_str(),ip_route.net_mask,ip_route.priority,ip_route.id);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ip config update sql %s \n", buf);
    if (agc_db_exec(get_agc_db(), buf, NULL, NULL, &err_msg) != AGC_DB_OK)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
        result = AGC_STATUS_FALSE;
    }
    free(buf);
    agc_db_free(err_msg);
    return result;
}

int get_ip_route_rows(int &rows)
{
    char *err_msg = NULL;
    char **query_result = NULL;
    int nrow, ncolumn;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "SELECT COUNT(*) FROM ip_route");
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ip route total query sql %s\n", buf);
    if (agc_db_get_table(get_agc_db(), buf, &query_result, &nrow, &ncolumn, &err_msg) == AGC_DB_OK)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ip route total number: %s\n", query_result[1]);
        rows = atoi(query_result[1]);
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

int find_ip_route_by_name(string name)
{
    char *err_msg = NULL;
    char **query_result = NULL;
    int nrow;
    int ncolumn;
    char *sql;
    int result = AGC_STATUS_FALSE;

    sql = (char *)malloc(512);
    sprintf(sql, "select * from ip_route where ip_route.RouteName='%s'", name.c_str());
    if (agc_db_get_table(get_agc_db(), sql, &query_result, &nrow, &ncolumn, &err_msg) == AGC_DB_OK)
    {
        if (nrow > 0)
        {
            result = AGC_STATUS_SUCCESS;
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
    agc_db_free(err_msg);
    agc_db_free_table(query_result);
    free(sql);
    return result;
}

int ip_config_db_callback(void *pArg, int argc, char **argv, char **columnNames)
{
    int i;
    cfg_ethnet *eth_net;

    if (columnNames != NULL)
    {
        for (i = 0; i < argc; i++)
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ip config column %s value %s .\n", columnNames[i], argv[i]);
        }
    }

    if (argc == IP_CONFIG_TABLE_COLUMNS  && argv != NULL)
    {
        eth_net = (cfg_ethnet *) pArg;
        eth_net->id = atoi(argv[0]);
        eth_net->net_name = argv[1];
        eth_net->if_name = argv[2];
        eth_net->ip_addr = argv[3];
        eth_net->net_mask = atoi(argv[4]);
        eth_net->gateway = argv[5];
        if (strcmp(argv[6], "null") == 0)
        {
            eth_net->DNS = "";
        }
        else
        {
            eth_net->DNS = argv[6];
        }
    }

    return AGC_DB_OK;
}

int ip_route_db_callback(void *pArg, int argc, char **argv, char **columnNames)
{
    int i;
    cfg_ip_route *ip_route;

    if (columnNames != NULL)
    {
        for (i = 0; i < argc; i++)
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ip route column %s value %s .\n", columnNames[i], argv[i]);
        }
    }

    if (argc == IP_ROUTE_TABLE_COLUMNS  && argv != NULL)
    {
        ip_route = (cfg_ip_route *) pArg;
        ip_route->id = atoi(argv[0]);
        ip_route->route_name.assign(argv[1]);
        ip_route->dest_ip.assign(argv[2]);
        ip_route->next_hop.assign(argv[3]);
        ip_route->net_mask = atoi(argv[4]);
        ip_route->priority = atoi(argv[5]);
    }

    return AGC_DB_OK;
}

int page_query_ip_route(int page_index, int page_size)
{
    char *err_msg = NULL;
    char **query_result = NULL;
    int i, nrow, ncolumn;
    char *buf;
    int result = AGC_STATUS_SUCCESS;

    buf = (char *)malloc(512);
    sprintf(buf, "SELECT * FROM ip_route LIMIT %d OFFSET %d", page_size, page_index);
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ip route info query sql %s\n", buf);
    if (agc_db_get_table(get_agc_db(), buf, &query_result, &nrow, &ncolumn, &err_msg) == AGC_DB_OK)
    {
        g_ip_route_rows = nrow;
        for(i = 1; i < nrow+1; i++)
        {
            ip_route_db_callback(&g_ip_route_query[i-1], ncolumn, (query_result + i*ncolumn), query_result);
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

cJSON *encode_ip_config_to_json()
{
    int i;
    cJSON_Hooks json_hooks;
    cJSON *record_array, *record_item;
    cfg_ethnet *eth_net;

    json_hooks.malloc_fn = malloc;
    json_hooks.free_fn = free;
    cJSON_InitHooks(&json_hooks);

    record_array = cJSON_CreateArray();

    for (i = 0; i < g_ip_config_rows; i++)
    {
        eth_net = &g_ip_config_query[i];
        record_item = cJSON_CreateObject();
        cJSON_AddNumberToObject(record_item, "ID",eth_net->id);
        cJSON_AddStringToObject(record_item, "PortName",eth_net->net_name.c_str());
        cJSON_AddStringToObject(record_item, "IfName",eth_net->if_name.c_str());
        cJSON_AddStringToObject(record_item, "IpAddr",eth_net->ip_addr.c_str());
        cJSON_AddNumberToObject(record_item, "Prefix",eth_net->net_mask);
        cJSON_AddStringToObject(record_item, "GateWay",eth_net->gateway.c_str());
        cJSON_AddStringToObject(record_item, "DNS",eth_net->DNS.c_str());
        cJSON_AddItemToArray(record_array, record_item);
    }

    return record_array;
}

cJSON *encode_ip_route_to_json()
{
    int i;
    cJSON_Hooks json_hooks;
    cJSON *record_array, *record_item;
    cfg_ip_route *ip_route;

    json_hooks.malloc_fn = malloc;
    json_hooks.free_fn = free;
    cJSON_InitHooks(&json_hooks);

    record_array = cJSON_CreateArray();

    for (i = 0; i < g_ip_route_rows; i++)
    {
        ip_route = &g_ip_route_query[i];
        record_item = cJSON_CreateObject();
        cJSON_AddNumberToObject(record_item, "ID",ip_route->id);
        cJSON_AddStringToObject(record_item, "RouteName",ip_route->route_name.c_str());
        cJSON_AddStringToObject(record_item, "DestIpAddr",ip_route->dest_ip.c_str());
        cJSON_AddNumberToObject(record_item, "NetMask",ip_route->net_mask);
        cJSON_AddStringToObject(record_item, "NextHop",ip_route->next_hop.c_str());
        cJSON_AddNumberToObject(record_item, "Priority",ip_route->priority);
        cJSON_AddItemToArray(record_array, record_item);
    }

    return record_array;
}

//

