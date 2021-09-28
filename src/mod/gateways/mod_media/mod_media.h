#ifndef __MOD_MEDIA_H__
#define __MOD_MEDIA_H__

#include <agc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

AGC_BEGIN_EXTERN_C

agc_memory_pool_t *media_get_memory_pool();

enum Data_IPType
{
	IPv4 = 0,
	IPv6 = 1
};


#define DATA_DB_FILENAME "datagw.db"
#define DATAGW_API_SIZE (sizeof(datagw_api_commands)/sizeof(datagw_api_commands[0]))
#define DATAGW_IPADDR_MAX_LEN 63


typedef void (*datagw_api_func) (agc_stream_handle_t *stream, int argc, char **argv);

agc_status_t datagw_main_real(const char *cmd, agc_stream_handle_t *stream);

void datagw_make_general_body(int ret, char **body);

int datagw_fire_event(char *uuid, char *body);

agc_status_t lbo_insert(agc_stream_handle_t *stream, int argc, char **argv);
agc_status_t lbo_delete(agc_stream_handle_t *stream, int argc, char **argv);
agc_status_t lbo_update(agc_stream_handle_t *stream, int argc, char **argv);
agc_status_t lbo_queryN(agc_stream_handle_t *stream, char **body, int page_index, int page_size);


void add_lbo_api(agc_stream_handle_t *stream, int argc, char **argv);
void rmv_lbo_api(agc_stream_handle_t *stream, int argc, char **argv);
void mod_lbo_api(agc_stream_handle_t *stream, int argc, char **argv);
void query_lbo_api(agc_stream_handle_t *stream, int argc, char **argv);

void query_DataGwVersion(agc_stream_handle_t *stream, char **body);
void query_datagw_version_api(agc_stream_handle_t *stream, int argc, char **argv);


void media_module_lock();

void media_module_unlock();

typedef struct datagw_api_command {
    const char *pname;
    datagw_api_func func;
    const char *pcommand;
    const char *psyntax;
}datagw_api_command_t;

AGC_END_EXTERN_C

#endif /* __MEDIA_H__ */