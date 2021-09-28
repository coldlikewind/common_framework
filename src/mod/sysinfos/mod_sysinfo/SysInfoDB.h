//
// Created by count on 2020/7/29.
//
#include "CfgNetwork.h"

#ifndef INC_5GCORE_SYSINFODB_H
#define INC_5GCORE_SYSINFODB_H

extern const int MAX_QUERY_ROWS;
extern int g_ip_config_rows;
extern int g_ip_route_rows;

typedef enum
{
    DB_FOUND,
    DB_NOT_FOUND
}db_result;

int save_ip_config(cfg_ethnet &eth_net);
int get_ip_config_by_id(cfg_ethnet *eth_net, int id);
int delete_ip_config_by_id(int id);
int update_ip_config(cfg_ethnet &eth_net);
int get_ip_config_rows(int &rows);
int find_ip_config_by_name(string name);
int find_ip_config_by_ifname(string ifname);
int page_query_ip_config(int page_index, int page_size);
cJSON *encode_ip_config_to_json();
int save_ip_route(cfg_ip_route &ip_route);
int get_ip_route_by_id(cfg_ip_route *ip_route, int id);
int delete_ip_route_by_id(int id);
int update_ip_route(cfg_ip_route &ip_route);
int get_ip_route_rows(int &rows);
int find_ip_route_by_name(string name);
int page_query_ip_route(int page_index, int page_size);
cJSON *encode_ip_route_to_json();
int ip_route_db_callback(void *pArg, int argc, char **argv, char **columnNames);
int ip_config_db_callback(void *pArg, int argc, char **argv, char **columnNames);
#endif //INC_5GCORE_SYSINFODB_H
