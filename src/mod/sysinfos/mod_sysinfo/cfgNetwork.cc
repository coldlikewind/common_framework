//
// Created by count on 2020/7/29.
#include "CfgNetwork.h"
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <error.h>
#include <net/route.h>
#include <fstream>
#include <sstream>
#include "agc.h"
#include "SysInfoDB.h"
#include "../../libs/apr/include/apr_thread_proc.h"

static const int CMD_MAX_ARGS = 20;
static char *cmd_args[CMD_MAX_ARGS];
static int cmd_argc = 0;
const char ETH_CONFIG_FILE[] = "/etc/sysconfig/network-scripts/ifcfg-";
const char ROUTE_CONFIG_FILE[] = "/etc/sysconfig/static-routes";

int cfg_add_ip_config(cfg_ethnet &eth_net)
{
    vector<string> cmd_line;

    if (AGC_STATUS_SUCCESS == find_ip_config_by_name(eth_net.net_name)
    ||  AGC_STATUS_SUCCESS == find_ip_config_by_ifname(eth_net.if_name))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ip config port name duplicate\n");
        return ERROR_VERIFY_FAILURE;
    }

    int cmd_count = GetAddIpConfigCmd(eth_net, cmd_line);
    for (int i=0; i < cmd_count; i++)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "add ip cmd[%d]: %s\n", i, cmd_line.at(i).c_str());
        if (CheckExecCmdResult(cmd_line.at(i).c_str()))
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ip config set success\n");
        } else
        {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ip config set failure\n");
            return ERROR_TAKE_EFFECT_FAILURE;
        }
    }

    if (AGC_STATUS_SUCCESS == save_ip_config(eth_net))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ip config save success\n");
        return AGC_STATUS_SUCCESS;
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ip config save failure\n");
        return ERROR_SAVE_DB_FAILURE;
    }
}

int cfg_mod_ip_config(cfg_ethnet &eth_net)
{
    vector<string> cmd_line;
    cfg_ethnet old_eth_net;

    if (AGC_STATUS_SUCCESS != get_ip_config_by_id(&old_eth_net, eth_net.id))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ip config record not found\n");
        return ERROR_RECORD_NO_EXIST;
    }

    int cmd_count = GetAddIpConfigCmd(eth_net, cmd_line);
    for (int i=0; i < cmd_count; i++) {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "mod ip cmd[%d]: %s\n", i, cmd_line.at(i).c_str());
        if (CheckExecCmdResult(cmd_line.at(i).c_str()))
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ip config set success\n");
        } else
        {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ip config set failure\n");
            return ERROR_TAKE_EFFECT_FAILURE;
        }
    }

    if (AGC_STATUS_SUCCESS == update_ip_config(eth_net))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ip config save success\n");
        return AGC_STATUS_SUCCESS;
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ip config save failure\n");
        return ERROR_SAVE_DB_FAILURE;
    }
}

int cfg_rmv_ip_config(int id)
{
    vector<string> cmd_line;
    cfg_ethnet old_eth_net;
    if (AGC_STATUS_SUCCESS != get_ip_config_by_id(&old_eth_net, id))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ip config record not found\n");
        return ERROR_RECORD_NO_EXIST;
    }

    int cmd_count = GetDelIpConfigCmd(old_eth_net, cmd_line);
    for (int i=0; i < cmd_count; i++)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "delete ip cmd[%d]: %s\n", i, cmd_line.at(i).c_str());
        if (CheckExecCmdResult(cmd_line.at(i).c_str()))
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ip config delete success\n");
        }
        else
        {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ip config delete failure\n");
            return ERROR_TAKE_EFFECT_FAILURE;
        }
    }

    if (AGC_STATUS_SUCCESS == delete_ip_config_by_id(id))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ip config db delete success\n");
        return AGC_STATUS_SUCCESS;
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ip config db delete failure\n");
        return ERROR_SAVE_DB_FAILURE;
    }
}

int cfg_list_ip_config(int page_index, int page_size)
{
    if (page_size <= MAX_QUERY_ROWS)
    {
        if (AGC_STATUS_SUCCESS == page_query_ip_config(page_index, page_size))
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ip config query finish\n");
            return AGC_STATUS_SUCCESS;
        }
        else
        {
            return AGC_STATUS_FALSE;
        }
    }
    return AGC_STATUS_FALSE;
}

int cfg_add_ip_route(cfg_ip_route &ip_route)
{
    vector<string> cmd_line;

    if (AGC_STATUS_SUCCESS == find_ip_route_by_name(ip_route.route_name))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ip route name duplicate\n");
        return ERROR_VERIFY_FAILURE;
    }

    int cmd_number = GetRouteCmd(ip_route, true, cmd_line);
    for (int i=0; i<cmd_number; i++)
    {
        if (CheckExecCmdResult(cmd_line[i].c_str()))
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "route add success: %s\n", cmd_line[i].c_str());
        }
        else
        {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "route add failure: %s\n", cmd_line[i].c_str());
            return ERROR_TAKE_EFFECT_FAILURE;
        }
    }

    if (AGC_STATUS_SUCCESS == save_ip_route(ip_route))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ip route save success\n");
        return AGC_STATUS_SUCCESS;
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ip route save failure\n");
        return ERROR_SAVE_DB_FAILURE;
    }
}

int cfg_mod_ip_route(cfg_ip_route &ip_route)
{
    cfg_ip_route old_ip_route;
    vector<string> cmd_line;

    if (AGC_STATUS_SUCCESS != get_ip_route_by_id(&old_ip_route, ip_route.id))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ip route record not exist\n");
        return ERROR_RECORD_NO_EXIST;
    }

    int cmd_number = GetRouteCmd(old_ip_route, false, cmd_line);
    for (int i=0; i<cmd_number; i++)
    {
        if (CheckExecCmdResult(cmd_line[i].c_str()))
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "route delete success: %s\n", cmd_line[i].c_str());
        }
        else
        {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "route delete failure: %s\n", cmd_line[i].c_str());
        }
    }

    cmd_line.clear();
    cmd_number = GetRouteCmd(ip_route, true, cmd_line);
    for (int i=0; i<cmd_number; i++)
    {
        if (CheckExecCmdResult(cmd_line[i].c_str()))
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "route add success: %s\n", cmd_line[i].c_str());
        }
        else
        {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "route add failure: %s\n", cmd_line[i].c_str());
            return ERROR_TAKE_EFFECT_FAILURE;
        }
    }

    if (AGC_STATUS_SUCCESS == update_ip_route(ip_route))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ip route update success\n");
        return AGC_STATUS_SUCCESS;
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ip route update failure\n");
        return ERROR_SAVE_DB_FAILURE;
    }
}

int cfg_rmv_ip_route(int id)
{
    cfg_ip_route old_ip_route;
    vector<string> cmd_line;

    if (AGC_STATUS_SUCCESS != get_ip_route_by_id(&old_ip_route, id))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ip route record not exist\n");
        return ERROR_RECORD_NO_EXIST;
    }

    int cmd_number = GetRouteCmd(old_ip_route, false, cmd_line);
    for (int i=0; i<cmd_number; i++)
    {
        if (CheckExecCmdResult(cmd_line[i].c_str()))
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "route delete success: %s\n", cmd_line[i].c_str());
        }
        else
        {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "route delete failure: %s\n", cmd_line[i].c_str());
            return ERROR_TAKE_EFFECT_FAILURE;
        }
    }


    if (AGC_STATUS_SUCCESS == delete_ip_route_by_id(id))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ip route delete db success\n");
        return AGC_STATUS_SUCCESS;
    }
    else
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ip route delete db failure\n");
        return ERROR_SAVE_DB_FAILURE;
    }
}

int cfg_list_ip_route(int page_index, int page_size)
{
    if (page_size <= MAX_QUERY_ROWS)
    {
        if (AGC_STATUS_SUCCESS == page_query_ip_route(page_index, page_size))
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ip route query finish\n");
            return AGC_STATUS_SUCCESS;
        }
        else
        {
            return AGC_STATUS_FALSE;
        }
    }
    return AGC_STATUS_FALSE;
}

int GetRouteCmd(const cfg_ip_route& cfg, bool isAdd, vector<string> &cmd_list)
{
    char buf[1024];

    if(string::npos != cfg.dest_ip.find(".0", cfg.dest_ip.size()-2))
    {
        sprintf(buf,"route %s -net %s/%d gw %s",
                (isAdd ? "add" : "del"),
                cfg.dest_ip.c_str(),
                cfg.net_mask,
                cfg.next_hop.c_str());
        cmd_list.emplace_back(buf);
        if (isAdd)
        {
            sprintf(buf, "echo 'any net %s/%d gw %s' >> %s",
                    cfg.dest_ip.c_str(),
                    cfg.net_mask,
                    cfg.next_hop.c_str(),
                    ROUTE_CONFIG_FILE);
            cmd_list.emplace_back(buf);
        }
        else
        {
            sprintf(buf, "sed -i '/^any net %s\\/%d gw %s/d' %s",
                    cfg.dest_ip.c_str(),
                    cfg.net_mask,
                    cfg.next_hop.c_str(),
                    ROUTE_CONFIG_FILE);
            cmd_list.emplace_back(buf);
        }
    }
    else
    {
        sprintf(buf,"route %s -host %s gw %s",
                (isAdd ? "add" : "del"),
                cfg.dest_ip.c_str(),
                cfg.next_hop.c_str());
        cmd_list.emplace_back(buf);
        if (isAdd)
        {
            sprintf(buf, "echo 'any host %s gw %s' >> %s",
                    cfg.dest_ip.c_str(),
                    cfg.next_hop.c_str(),
                    ROUTE_CONFIG_FILE);
            cmd_list.emplace_back(buf);
        }
        else
        {
            sprintf(buf, "sed -i '/^any host %s gw %s' %s",
                    cfg.dest_ip.c_str(),
                    cfg.next_hop.c_str(),
                    ROUTE_CONFIG_FILE);
            cmd_list.emplace_back(buf);
        }
    }

    return cmd_list.size();
}

int GetAddIpConfigCmd(const cfg_ethnet& cfg, vector<string> &cmd_list)
{
    char buff[256];

    if (!cfg.DNS.empty())
    {
        sprintf(buff, "nmcli con mod %s ipv4.address %s/%d ipv4.gateway %s ipv4.method manual ipv4.dns %s",
                cfg.if_name.c_str(), cfg.ip_addr.c_str(), cfg.net_mask, cfg.gateway.c_str(), cfg.DNS.c_str());
    }
    else
    {
        sprintf(buff, "nmcli con mod %s ipv4.address %s/%d ipv4.gateway %s ipv4.method manual",
                cfg.if_name.c_str(), cfg.ip_addr.c_str(), cfg.net_mask, cfg.gateway.c_str());
    }
    cmd_list.emplace_back(buff);
    sprintf(buff, "nmcli con up %s",cfg.if_name.c_str());
    cmd_list.emplace_back(buff);

    return cmd_list.size();
}

int GetDelIpConfigCmd(const cfg_ethnet& cfg, vector<string> &cmd_line)
{
    char buff[256];

    sprintf(buff,"ip addr del %s dev %s", cfg.ip_addr.c_str(), cfg.if_name.c_str());
    cmd_line.emplace_back(buff);

    string file = ETH_CONFIG_FILE;
    file = file + cfg.if_name;

    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "eth config file name: %s\n", file.c_str());
    ifstream fin(file);
    if(fin)
    {
        sprintf(buff, "sed -i '/^IPADDR/d' %s", file.c_str());
        cmd_line.emplace_back(buff);
        sprintf(buff, "sed -i '/^PREFIX/d' %s", file.c_str());
        cmd_line.emplace_back(buff);
        sprintf(buff, "sed -i '/^GATEWAY/d' %s", file.c_str());
        cmd_line.emplace_back(buff);
        sprintf(buff, "sed -i '/^DNS/d' %s", file.c_str());
        cmd_line.emplace_back(buff);
        fin.close();
    }

    return cmd_line.size();
}

bool CheckExecCmdResult(const char* cmd)
{
    FILE *ptr;
    if((ptr=popen(cmd, "r")) != NULL)
    {
        int ret = pclose(ptr);
        return ret == 0;
    }
    return false;
}

bool GetExecCmdResult(const char* cmd, string &result)
{
    FILE *ptr;
    char output[100];

    if((ptr=popen(cmd, "r")) != NULL)
    {
        fgets(output, 100, ptr);
        result = output;
        int ret = pclose(ptr);
        return ret == 0;
    }
    return false;
}


int get_if_name(vector<string> &if_name_list)
{
    struct sockaddr_in *sin = NULL;
    struct ifaddrs *ifa = NULL, *ifList;

    if (getifaddrs(&ifList) < 0)
    {
        return AGC_STATUS_FALSE;
    }

    for (ifa = ifList; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
        {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET)
        {
            if (0 == strcmp(ifa->ifa_name, "lo"))
            {
                continue;
            }
            string if_name = ifa->ifa_name;
            if_name_list.emplace_back(if_name);
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "n>>> interfaceName: %s\n", ifa->ifa_name);
        }
    }

    return AGC_STATUS_SUCCESS;
}

int get_if_status(vector<string> &if_name_list, vector<query_ethnet> &query_eth_list)
{
    char cmd_buff[100];
    string result;
    query_ethnet eth_item;
    for (int i=0; i<if_name_list.size(); i++)
    {
        sprintf(cmd_buff, "nmcli device show %s | grep IP4.ADDRESS | awk '{print $2}'", if_name_list[i].c_str());
        if (GetExecCmdResult(cmd_buff, result))
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "eth[%d] ip: %s\n", i, result.c_str());
            eth_item.ip_addr = result;
        }
        else
        {
            return AGC_STATUS_FALSE;
        }
        sprintf(cmd_buff, "nmcli device show %s | grep IP4.GATEWAY | awk '{print $2}'", if_name_list[i].c_str());
        if (GetExecCmdResult(cmd_buff, result))
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "eth[%d] gateway: %s\n", i, result.c_str());
            eth_item.gateway = result;
        }
        else
        {
            return AGC_STATUS_FALSE;
        }
        sprintf(cmd_buff, "nmcli device show %s | grep IP4.DNS[1] | awk '{print $2}'", if_name_list[i].c_str());
        if (GetExecCmdResult(cmd_buff, result))
        {
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "eth[%d] DNS: %s\n", i, result.c_str());
            eth_item.DNS = result;
        }
        else
        {
            return AGC_STATUS_FALSE;
        }
        eth_item.if_name = if_name_list[i];
        query_eth_list.emplace_back(eth_item);
    }
    return AGC_STATUS_SUCCESS;
}

int get_route_list(vector<query_route> &query_route_list)
{
    char cmd_line[50] = "route > /tmp/route_print.txt";

    if (!CheckExecCmdResult(cmd_line))
    {
        return AGC_STATUS_FALSE;
    }

    ifstream fs("/tmp/route_print.txt");
    if (fs)
    {
        string line;
        int i=0, j=0;
        while (getline (fs, line))
        {
            i++;
            if (i < 3) continue;
            query_route route;
            int k = 0;
            char *strc = new char[strlen(line.c_str())+1];
            strcpy(strc, line.c_str());
            char* temp = strtok(strc, " ");
            while(temp != NULL)
            {
                if (k==0) route.dst_addr = temp;
                if (k==1) route.gate_way = temp;
                if (k==2) route.net_mask = temp;
                if (k==7)
                {
                    route.if_name = temp;
                    query_route_list.emplace_back(route);
                    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "dst addr:%s gateway:%s net mask:%s if name:%s\n",
                                   route.dst_addr.c_str(), route.gate_way.c_str(), route.net_mask.c_str(), route.if_name.c_str());
                }
                k++;
                temp = strtok(NULL, " ");
            }
            delete[] strc;
        }
        fs.close();
    }
    else
    {
        return AGC_STATUS_FALSE;
    }

    return AGC_STATUS_SUCCESS;
}

int getSubnetMask(cfg_ethnet_list &eth_list)
{
    struct sockaddr_in *sin = NULL;
    struct ifaddrs *ifa = NULL, *ifList;

    if (getifaddrs(&ifList) < 0)
    {
        return -1;
    }

    for (ifa = ifList; ifa != NULL; ifa = ifa->ifa_next)
    {
        if(ifa->ifa_addr->sa_family == AF_INET)
        {
            cfg_ethnet eth_net;
            agc_log_printf(AGC_LOG, AGC_LOG_INFO,"n>>> interfaceName: %s\n", ifa->ifa_name);
            eth_net.if_name = ifa->ifa_name;

            sin = (struct sockaddr_in *)ifa->ifa_addr;
            agc_log_printf(AGC_LOG, AGC_LOG_INFO,">>> ipAddress: %s\n", inet_ntoa(sin->sin_addr));
            eth_net.if_name = inet_ntoa(sin->sin_addr);

            sin = (struct sockaddr_in *)ifa->ifa_dstaddr;
            agc_log_printf(AGC_LOG, AGC_LOG_INFO,">>> broadcast: %s\n", inet_ntoa(sin->sin_addr));

            sin = (struct sockaddr_in *)ifa->ifa_netmask;
            agc_log_printf(AGC_LOG, AGC_LOG_INFO,">>> subnetMask: %s\n", inet_ntoa(sin->sin_addr));
            //eth_net.net_mask = inet_ntoa(sin->sin_addr);

            eth_list.ethnet_list.push_back(eth_net);
        }
    }

    freeifaddrs(ifList);

    return 0;
}

int SetIfAddr(char *ifname, char *Ipaddr, char *mask,char *gateway)
{
    int fd;
    int rc;
    struct ifreq ifr;
    struct sockaddr_in *sin;
    struct rtentry rt;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd < 0)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"socket   error");
        return -1;
    }
    memset(&ifr,0,sizeof(ifr));
    strcpy(ifr.ifr_name,ifname);
    sin = (struct sockaddr_in*)&ifr.ifr_addr;
    sin->sin_family = AF_INET;
    //IP地址
    if( inet_aton(Ipaddr,&(sin->sin_addr)) < 0
        || inet_aton(mask,&(sin->sin_addr)) < 0
        || inet_aton(gateway, &sin->sin_addr) <0 )
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"inet_aton   error");
        return -2;
    }

    if(ioctl(fd,SIOCSIFADDR, &ifr) < 0)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"ioctl ip address error");
        return -3;
    }
    if(ioctl(fd, SIOCSIFNETMASK, &ifr) < 0)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"ioctl netmask error");
        return -5;
    }
    //网关
    memset(&rt, 0, sizeof(struct rtentry));
    memset(sin, 0, sizeof(struct sockaddr_in));
    sin->sin_family = AF_INET;
    sin->sin_port = 0;
    memcpy(&rt.rt_gateway, sin, sizeof(struct sockaddr_in));
    ((struct sockaddr_in *)&rt.rt_dst)->sin_family=AF_INET;
    ((struct sockaddr_in *)&rt.rt_genmask)->sin_family=AF_INET;
    rt.rt_flags = RTF_GATEWAY;
    if (ioctl(fd, SIOCADDRT, &rt)<0)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"ioctl(SIOCADDRT) error in set_default_route\n");
        close(fd);
        return -1;
    }
    close(fd);
    return rc;
}

int check_nic(const char *ifname)
{
    struct ifreq ifr;
    int skfd = socket(AF_INET, SOCK_DGRAM, 0);
    int result = -1;

    strcpy(ifr.ifr_name, ifname);
    if (ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0)
    {
        close(skfd);
        return result;
    }
    if (ifr.ifr_flags & IFF_RUNNING)
    {
        result = 0;  // 网卡已插上网线
    }
    close(skfd);
    return result;
}

int shell_run(char *exe, int argc, char **args)
{
    apr_pool_t *p = NULL;
    apr_procattr_t *attr;
    apr_proc_t newproc;
    apr_exit_why_e exit_why;
    int exit_code;

    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "shell cmd enter\n");

    for (int i=0; i<argc; i++)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "cmd args[%d]:%s\n",i,args[i]);
    }

    if (AGC_STATUS_SUCCESS != apr_pool_create(&p, NULL))
    {
        goto error;
    }

    if (AGC_STATUS_SUCCESS != apr_procattr_create(&attr, p))
    {
        goto error;
    }

    if (AGC_STATUS_SUCCESS != apr_procattr_io_set(attr, APR_NO_PIPE, APR_NO_PIPE, APR_NO_PIPE))
    {
        goto error;
    }

    if (AGC_STATUS_SUCCESS !=apr_procattr_cmdtype_set(attr, APR_PROGRAM_PATH))
    {
        goto error;
    }

    if (AGC_STATUS_SUCCESS != apr_proc_create(&newproc, exe, args, NULL, attr, p))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "cmd proc create failure\n");
        goto error;
    }

    if (APR_CHILD_DONE != apr_proc_wait(&newproc, &exit_code, &exit_why, APR_WAIT))
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "cmd proc wait failure\n");
        goto error;
    }

    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "cmd result: %d, %d\n", exit_why, exit_code);
    apr_pool_destroy(p);
    for (int i = 0; i < argc; i++)
    {
        if (args[i]) free(args[i]);
    }
    if (exit_why == APR_PROC_EXIT && exit_code == 0)
    {
        return AGC_STATUS_SUCCESS;
    }
    else
    {
        return AGC_STATUS_FALSE;
    }
    error:
        if (p != NULL)
        {
            apr_pool_destroy(p);
        }
        for (int i = 0; i < argc; i++)
        {
            if (args[i]) free(args[i]);
        }
        return AGC_STATUS_FALSE;
}

//

