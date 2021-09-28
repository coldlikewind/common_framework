//
// Created by count on 2020/7/28.
#include "agc.h"
#include "SysAlarm.h"
#include "CfgNetwork.h"


sysinfo_alarm::sysinfo_alarm(string id, int high_limit, int low_limit, int number)
{
    alarm_status = STATUS_NORMAL;
    limit_count = 0;
    upper_limit = high_limit;
    lower_limit = low_limit;
    check_number = number;
    alarm_id = id;
}

void sysinfo_alarm::check_alarm(int usage)
{
    if (alarm_status == STATUS_NORMAL)
    {
        if (usage >= upper_limit)
        {
            limit_count++;
            if (limit_count >= check_number)
            {
                alarm_status = STATUS_ALARM;
                report_alarm(to_string(usage).c_str());
                agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "alarm %s report: %d\n", alarm_id.c_str(), usage);
            }
        }
        else
        {
            limit_count = 0;
        }
    }
    else if (alarm_status == STATUS_ALARM)
    {
        if (usage <= lower_limit)
        {
            limit_count++;
            if (limit_count >= check_number)
            {
                alarm_status = STATUS_ALARM;
                report_alarm(to_string(usage).c_str());
                agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "alarm %s restore: %d\n", alarm_id.c_str(), usage);
            }
        }
        else
        {
            limit_count = 0;
        }
    }
}

int sysinfo_alarm::report_alarm(string alarm_param)
{
    agc_event_t *new_event = NULL;

    if ((agc_event_create(&new_event, EVENT_ID_ALARM, EVENT_NULL_SOURCEID) != AGC_STATUS_SUCCESS) || !new_event)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sysinfo create alarm [fail].\n");
        return AGC_STATUS_FALSE;
    }

    if (agc_event_add_header(new_event, HEADER_ALARM_MOD_ID, "%s", sys_mo_id.c_str()) != AGC_STATUS_SUCCESS)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sysinfo add mo id [fail].\n");
        agc_event_destroy(&new_event);
        return AGC_STATUS_FALSE;
    }

    if (agc_event_add_header(new_event, HEADER_ALARM_ID, "%s", alarm_id.c_str()) != AGC_STATUS_SUCCESS)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sysinfo add alarm id [fail].\n");
        agc_event_destroy(&new_event);
        return AGC_STATUS_FALSE;
    }

    if (agc_event_add_header(new_event, HEADER_ALARM_INST_ID, "0") != AGC_STATUS_SUCCESS)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sysinfo add instance id [fail].\n");
        agc_event_destroy(&new_event);
        return AGC_STATUS_FALSE;
    }

    if (alarm_status == STATUS_NORMAL)
    {
        if (agc_event_add_header(new_event, HEADER_ALARM_TYPE, ALARM_TYPE_CLEAR) != AGC_STATUS_SUCCESS)
        {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sysinfo add alarm type [fail].\n");
            agc_event_destroy(&new_event);
            return AGC_STATUS_FALSE;
        }
    }
    else
    {
        if (agc_event_add_header(new_event, HEADER_ALARM_TYPE, ALARM_TYPE_REPORT) != AGC_STATUS_SUCCESS)
        {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sysinfo add alarm type [fail].\n");
            agc_event_destroy(&new_event);
            return AGC_STATUS_FALSE;
        }
    }

    if (agc_event_add_header(new_event, HEADER_ALARM_PARAM, "%s", alarm_param.c_str()) != AGC_STATUS_SUCCESS)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sysinfo add alarm param [fail].\n");
        agc_event_destroy(&new_event);
        return AGC_STATUS_FALSE;
    }

    if (agc_event_fire(&new_event) != AGC_STATUS_SUCCESS) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sysinfo report alarm [fail].\n");
        agc_event_destroy(&new_event);
        return AGC_STATUS_FALSE;
    }
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "sysinfo report alarm [ok].\n");
    return AGC_STATUS_SUCCESS;
}

void ethnet_alarm::ethnet_status_init()
{
    vector<string> if_name_list;
    ethnet_status_t ethnet_status;

    if (AGC_STATUS_SUCCESS == get_if_name(if_name_list))
    {
        for (int i=0; i < if_name_list.size(); i++)
        {
            if (0 == check_nic(if_name_list[i].c_str()))
            {
                ethnet_status.if_name = if_name_list[i];
                ethnet_status.if_status = STATUS_NORMAL;
            }
            else
            {
                ethnet_status.if_name = if_name_list[i];
                ethnet_status.if_status = STATUS_ALARM;
            }
            agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ethnet status init: %s %d\n", ethnet_status.if_name.c_str(), ethnet_status.if_status);
            ethnet_status_list.emplace_back(ethnet_status);
        }
    }
}

void ethnet_alarm::check_alarm()
{
    for (int i=0; i < ethnet_status_list.size(); i++)
    {
        if (0 == check_nic(ethnet_status_list[i].if_name.c_str()))
        {
            if (STATUS_ALARM == ethnet_status_list[i].if_status)
            {
                ethnet_status_list[i].if_status = STATUS_NORMAL;
                agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ethnet alarm clear: %s", ethnet_status_list[i].if_name.c_str());
                report_alarm(STATUS_NORMAL, ethnet_status_list[i].if_name, "");
            }
        }
        else
        {
            if (STATUS_NORMAL == ethnet_status_list[i].if_status)
            {
                ethnet_status_list[i].if_status = STATUS_ALARM;
                agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ethnet alarm: %s", ethnet_status_list[i].if_name.c_str());
                report_alarm(STATUS_ALARM,ethnet_status_list[i].if_name,"");
            }
        }
    }
}

int ethnet_alarm::report_alarm(ENUM_ALARM_STATUS alarm_status, string inst_id, string alarm_param)
{
    agc_event_t *new_event = NULL;

    if ((agc_event_create(&new_event, EVENT_ID_ALARM, EVENT_NULL_SOURCEID) != AGC_STATUS_SUCCESS) || !new_event)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ethnet create alarm [fail].\n");
        return AGC_STATUS_FALSE;
    }

    if (agc_event_add_header(new_event, HEADER_ALARM_MOD_ID, "%s", sys_mo_id.c_str()) != AGC_STATUS_SUCCESS)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ethnet add mo id [fail].\n");
        agc_event_destroy(&new_event);
        return AGC_STATUS_FALSE;
    }

    if (agc_event_add_header(new_event, HEADER_ALARM_ID, "%s", ethnet_alarm_id.c_str()) != AGC_STATUS_SUCCESS)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ethnet add alarm id [fail].\n");
        agc_event_destroy(&new_event);
        return AGC_STATUS_FALSE;
    }

    if (agc_event_add_header(new_event, HEADER_ALARM_INST_ID, "%s", inst_id.c_str()) != AGC_STATUS_SUCCESS)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ethnet add instance id [fail].\n");
        agc_event_destroy(&new_event);
        return AGC_STATUS_FALSE;
    }

    if (alarm_status == STATUS_NORMAL)
    {
        if (agc_event_add_header(new_event, HEADER_ALARM_TYPE, ALARM_TYPE_CLEAR) != AGC_STATUS_SUCCESS)
        {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ethnet add alarm type [fail].\n");
            agc_event_destroy(&new_event);
            return AGC_STATUS_FALSE;
        }
    }
    else
    {
        if (agc_event_add_header(new_event, HEADER_ALARM_TYPE, ALARM_TYPE_REPORT) != AGC_STATUS_SUCCESS)
        {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ethnet add alarm type [fail].\n");
            agc_event_destroy(&new_event);
            return AGC_STATUS_FALSE;
        }
    }

    if (agc_event_add_header(new_event, HEADER_ALARM_PARAM, "%s", alarm_param.c_str()) != AGC_STATUS_SUCCESS)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ethnet add alarm param [fail].\n");
        agc_event_destroy(&new_event);
        return AGC_STATUS_FALSE;
    }

    if (agc_event_fire(&new_event) != AGC_STATUS_SUCCESS) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ethnet report alarm [fail].\n");
        agc_event_destroy(&new_event);
        return AGC_STATUS_FALSE;
    }
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "ethnet report alarm [ok].\n");
    return AGC_STATUS_SUCCESS;
}

//

