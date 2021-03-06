#include <agc.h>

static agc_routine_actions_t *routine = NULL;

AGC_DECLARE(agc_status_t) agc_driver_init(agc_memory_pool_t *pool)
{
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Driver init success.\n");
    
    return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(agc_status_t) agc_driver_shutdown(void)
{
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Driver shutdown success.\n");
    
    return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(void) agc_driver_register_routine(agc_routine_actions_t *routines)
{
    assert(routines);
    routine = routines;
}

AGC_DECLARE(agc_status_t) agc_driver_add_connection(agc_connection_t *c)
{
    if (!routine) {
        return AGC_STATUS_GENERR;
    }
    
    return routine->add_conn(c);
}

AGC_DECLARE(agc_status_t) agc_driver_del_connection(agc_connection_t *c)
{
    if (!routine) {
        return AGC_STATUS_GENERR;
    }
    
    return routine->del_conn(c);
}

AGC_DECLARE(agc_status_t)  agc_driver_add_event(agc_connection_t *c, uint32_t event)
{
    if (!routine) {
        return AGC_STATUS_GENERR;
    }
    
    return routine->add(c, event);
}

AGC_DECLARE(agc_status_t)  agc_driver_del_event(agc_connection_t *c, uint32_t event)
{
    if (!routine) {
        return AGC_STATUS_GENERR;
    }
    
    return routine->del(c, event);
}

