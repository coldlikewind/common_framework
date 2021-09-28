//
// Created by count on 2020/7/28.
#include "agc.h"
#include "SigAlarm.h"


int sig_alarm::report_general_alarm(ENUM_ALARM_STATUS alarm_status, string instance_id, string alarm_param)
{
    agc_event_t *new_event = NULL;

    if ((agc_event_create(&new_event, EVENT_ID_ALARM, EVENT_NULL_SOURCEID) != AGC_STATUS_SUCCESS) || !new_event)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "siggw create alarm [fail].\n");
        return AGC_STATUS_FALSE;
    }

    if (agc_event_add_header(new_event, HEADER_ALARM_MOD_ID, "%s", mo_id.c_str()) != AGC_STATUS_SUCCESS)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "siggw add mo id [fail].\n");
        agc_event_destroy(&new_event);
        return AGC_STATUS_FALSE;
    }

    if (agc_event_add_header(new_event, HEADER_ALARM_ID, "%s", alarm_id.c_str()) != AGC_STATUS_SUCCESS)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "siggw add alarm id [fail].\n");
        agc_event_destroy(&new_event);
        return AGC_STATUS_FALSE;
    }

    if (agc_event_add_header(new_event, HEADER_ALARM_INSTANCE_ID, "%s", instance_id.c_str()) != AGC_STATUS_SUCCESS)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "siggw add alarm instance [fail].\n");
        agc_event_destroy(&new_event);
        return AGC_STATUS_FALSE;
    }

    if (alarm_status == STATUS_NORMAL)
    {
        if (agc_event_add_header(new_event, HEADER_ALARM_TYPE, ALARM_TYPE_CLEAR) != AGC_STATUS_SUCCESS)
        {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "siggw add alarm type [fail].\n");
            agc_event_destroy(&new_event);
            return AGC_STATUS_FALSE;
        }
    }
    else
    {
        if (agc_event_add_header(new_event, HEADER_ALARM_TYPE, ALARM_TYPE_REPORT) != AGC_STATUS_SUCCESS)
        {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "siggw add alarm type [fail].\n");
            agc_event_destroy(&new_event);
            return AGC_STATUS_FALSE;
        }
    }

    if (alarm_param.size()) {
        if (agc_event_add_header(new_event, HEADER_ALARM_PARAM, "%s", alarm_param.c_str()) != AGC_STATUS_SUCCESS) {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "siggw add alarm param [fail].\n");
            agc_event_destroy(&new_event);
            return AGC_STATUS_FALSE;
        }
    }

    if (agc_event_fire(&new_event) != AGC_STATUS_SUCCESS) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "siggw report alarm [fail].\n");
        agc_event_destroy(&new_event);
        return AGC_STATUS_FALSE;
    }
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "siggw report alarm id:%s, instance:%s, param:%s, type:%d.\n",
                                                    alarm_id.c_str(), instance_id.c_str(), alarm_param.c_str(), alarm_status);
    return AGC_STATUS_SUCCESS;
}
