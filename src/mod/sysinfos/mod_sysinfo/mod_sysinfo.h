//
// Created by count on 2020/7/16.
//

#ifndef INC_5GCORE_MOD_SYSINFO_H
#define INC_5GCORE_MOD_SYSINFO_H

typedef void (*sysinfo_api_func) (agc_stream_handle_t *stream, int argc, char **argv);

typedef struct sysinfo_api_command {
    const char *pname;
    sysinfo_api_func func;
    const char *pcommand;
    const char *psyntax;
}sysinfo_api_command_t;

agc_status_t sysinfo_main_real(const char *cmd, agc_stream_handle_t *stream);
void sysinfo_timer_callback(void *data);
int sysinfo_db_init();
agc_db_t *get_agc_db();
void sysinfo_make_general_body(int ret, char **body);
int sysinfo_fire_event(char *uuid, char *body);
void sysinfo_cpu_usage_api(agc_stream_handle_t *stream, int argc, char **argv);
void sysinfo_mem_usage_api(agc_stream_handle_t *stream, int argc, char **argv);
void sysinfo_disk_usage_api(agc_stream_handle_t *stream, int argc, char **argv);
void sysinfo_get_network_api(agc_stream_handle_t *stream, int argc, char **argv);
void sysinfo_add_ip_config_api(agc_stream_handle_t *stream, int argc, char **argv);
void sysinfo_rmv_ip_config_api(agc_stream_handle_t *stream, int argc, char **argv);
void sysinfo_mod_ip_config_api(agc_stream_handle_t *stream, int argc, char **argv);
void sysinfo_list_ip_config_api(agc_stream_handle_t *stream, int argc, char **argv);
void sysinfo_add_ip_route_api(agc_stream_handle_t *stream, int argc, char **argv);
void sysinfo_rmv_ip_route_api(agc_stream_handle_t *stream, int argc, char **argv);
void sysinfo_mod_ip_route_api(agc_stream_handle_t *stream, int argc, char **argv);
void sysinfo_list_ip_route_api(agc_stream_handle_t *stream, int argc, char **argv);
void sysinfo_dsp_ip_config_api(agc_stream_handle_t *stream, int argc, char **argv);
void sysinfo_dsp_ip_route_api(agc_stream_handle_t *stream, int argc, char **argv);
void sysinfo_dsp_version_api(agc_stream_handle_t *stream, int argc, char **argv);

#endif //INC_5GCORE_MOD_SYSINFO_H
