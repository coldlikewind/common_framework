//
// Created by count on 2020/7/29.
//

#ifndef INC_5GCORE_CFGNETWORK_H
#define INC_5GCORE_CFGNETWORK_H

#include <string>
#include <vector>

using namespace std;

typedef struct
{
    int     id;
    string  net_name;
    string  if_name;
    string  ip_addr;
    string  gateway;
    string  DNS;
    unsigned int  net_mask;
}cfg_ethnet;//子端口配置

typedef struct
{
    std::vector<cfg_ethnet> ethnet_list;
}cfg_ethnet_list;

typedef struct
{
    int     id;
    string  route_name;
    string  dest_ip;
    string  next_hop;
    unsigned int net_mask;
    unsigned int priority;
}cfg_ip_route;

typedef struct
{
    string if_name;
    string ip_addr;
    string gateway;
    string DNS;
}query_ethnet;

typedef struct
{
    string if_name;
    string dst_addr;
    string gate_way;
    string net_mask;
}query_route;

typedef struct
{
    vector<cfg_ip_route> ip_route_list;
}cfg_ip_route_list;

typedef enum
{
    ERROR_VERIFY_FAILURE = -1,
    ERROR_SAVE_DB_FAILURE = -2,
    ERROR_TAKE_EFFECT_FAILURE = -3,
    ERROR_RECORD_NO_EXIST = -4
}cfg_result;

int cfg_add_ip_config(cfg_ethnet &eth_net);
int cfg_mod_ip_config(cfg_ethnet &eth_net);
int cfg_rmv_ip_config(int id);
int cfg_list_ip_config(int page_index, int page_size);
int cfg_add_ip_route(cfg_ip_route &ip_route);
int cfg_mod_ip_route(cfg_ip_route &ip_route);
int cfg_rmv_ip_route(int id);
int cfg_list_ip_route(int page_index, int page_size);

int GetRouteCmd(const cfg_ip_route& cfg, bool isAdd, vector<string> &cmd_list);
int GetAddIpConfigCmd(const cfg_ethnet& cfg, vector<string> &cmd_list);
int GetDelIpConfigCmd(const cfg_ethnet& cfg, vector<string> &cmd_line);
bool CheckExecCmdResult(const char* cmd);
bool GetExecCmdResult(const char* cmd, string &result);
int get_if_name(vector<string> &if_name_list);
int SetIfAddr(char *ifname, char *Ipaddr, char *mask,char *gateway);
int check_nic(const char *ifname);
int get_if_status(vector<string> &if_name, vector<query_ethnet> &query_eth_list);
int getSubnetMask(cfg_ethnet_list &eth_list);
int shell_run(char *exe, int argc, char **args);
int get_route_list(vector<query_route> &query_route_list);
#endif //INC_5GCORE_CFGNETWORK_H
