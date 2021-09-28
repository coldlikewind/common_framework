//
// Created by count on 2020/7/28.
//

#ifndef INC_5GCORE_SYSALARM_H
#define INC_5GCORE_SYSALARM_H

#include <string>
#include <vector>

using namespace std;

#define HEADER_ALARM_ID "alarm_id"
#define HEADER_ALARM_MOD_ID "mo_id"
#define HEADER_ALARM_INST_ID "instance_id"
#define HEADER_ALARM_PARAM "alarm_param"
#define HEADER_ALARM_TYPE "alarm_type"

#define ALARM_TYPE_REPORT "report"
#define ALARM_TYPE_CLEAR "clear"

static const string sys_mo_id = "system";
static const string cpu_usage_alarm_id = "cpu load high";
static const string mem_usage_alarm_id = "memory occupied high";
static const string disk_usage_alarm_id = "disk free space low";
static const string ethnet_alarm_id = "ethnet port down";

typedef enum
{
    STATUS_NORMAL,
    STATUS_ALARM
}ENUM_ALARM_STATUS;


typedef struct
{
    string if_name;
    ENUM_ALARM_STATUS if_status;
}ethnet_status_t;

class sysinfo_alarm
{
    ENUM_ALARM_STATUS alarm_status;

    unsigned int limit_count;
    unsigned int upper_limit;
    unsigned int lower_limit;
    unsigned int check_number;
    string alarm_id;

protected:

public:
    sysinfo_alarm(string id, int high_limit = 80, int low_limit = 70, int number = 5);
    void check_alarm(int usage);
    int report_alarm(string alarm_param);
};

class ethnet_alarm
{
    vector<ethnet_status_t> ethnet_status_list;

public:
    void ethnet_status_init();
    void check_alarm();
    int report_alarm(ENUM_ALARM_STATUS alarm_status, string inst_id, string alarm_param);
};

#endif //INC_5GCORE_SYSALARM_H
