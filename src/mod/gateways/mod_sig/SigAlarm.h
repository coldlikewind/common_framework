//
// Created by count on 2020/7/28.
//

#ifndef INC_5GCORE_SIGALARM_H
#define INC_5GCORE_SIGALARM_H

#include <string>
#include <vector>

using namespace std;

#define HEADER_ALARM_ID "alarm_id"
#define HEADER_ALARM_MOD_ID "mo_id"
#define HEADER_ALARM_INSTANCE_ID "instance_id"
#define HEADER_ALARM_PARAM "alarm_param"
#define HEADER_ALARM_TYPE "alarm_type"

#define ALARM_TYPE_REPORT "report"
#define ALARM_TYPE_CLEAR "clear"

const string siggw_mo_id = "siggw";
const string sctp_alarm_id = "sctp link down";

typedef enum
{
    STATUS_NORMAL,
    STATUS_ALARM
}ENUM_ALARM_STATUS;


class sig_alarm
{
    ENUM_ALARM_STATUS alarm_status;

    unsigned int limit_count;
    unsigned int upper_limit;
    unsigned int lower_limit;
    unsigned int check_number;
    string alarm_id;
    string mo_id;

protected:

public:
    int report_general_alarm(ENUM_ALARM_STATUS alarm_status, string instance_id, string alarm_param);
    void set_mo_id(string mo_id) { this->mo_id = mo_id; }
    void set_alarm_id(string alarm_id) { this->alarm_id = alarm_id; }
};


#endif //INC_5GCORE_SIGALARM_H
