#ifndef MOD_SIG_H
#define MOD_SIG_H

#include <agc.h>
#include <agc_sctp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SIG_SESSIONS	100000


#define INVALID_MCCMNC 0xFFFF
#define INVALID_ENODEBID 0xFFFFFFFF
#define INVALID_RANNODEID 0xFFFFFFFF
#define INVALID_ECGI_CID 0xFFFFFFFF
#define INVALID_MMEC  0xFF
#define INVALID_MMEGI 0xFFFF
#define INVALID_MTMSI 0xFFFFFFFF
#define INVALID_TAC 0xFFFF
#define INVALIDE_ID                         0xFFFFFFFF
#define INVALIDE_ID_X64                     0x7FFFFFFFFFFFFFFFULL
#define INVALIDE_NULL_int8  0xFF
#define INVALIDE_NULL_int16 0xFFFF
#define INVALIDE_NULL_int32 0xFFFFFFFF

#define SIG_DB_FILENAME "siggw.db"

#define SIG_NEXT_ID(__id, __min, __max) \
    ((__id) = ((__id) == (__max) ? (__min) : ((__id) + 1)))
    
typedef enum {
	OP_Scene_ADD_Nssai,
	OP_Scene_RMV_Nssai,
	OP_Scene_MOD_Nssai,
	OP_Scene_QUERY_Nssai,
	
} OP_Scene;

enum SCTPpeerpoint
{
	SCTP_RAN = 0,
	SCTP_AMF = 1
};


#define ALL_PDU_CHOISE      0xff
#define ALL_PROCEDURE_CODE  0xff

#define MOD_SIG_SCTP_NGAP_PPID 60
#define SIGGW_API_SIZE (sizeof(siggw_api_commands)/sizeof(siggw_api_commands[0]))

template<class T>
bool EqualVector(const T& vec1,const T& vec2) 
{
	if(vec1.size() != vec2.size()) 
	{
		return false;
	}

	for (uint32_t i = 0; i < vec1.size(); i++)
	{
        uint32_t j;
		for (j =0; j < vec2.size(); j++)
		{
			if (vec1[i] == vec2[j])
            {
                break;
            }
		}
        if (j >= vec2.size())
        {
            return false;
        }
	}
	return true;
}


AGC_BEGIN_EXTERN_C

agc_memory_pool_t *agcsig_get_sig_memory_pool();
typedef void (*siggw_api_func) (agc_stream_handle_t *stream, int argc, char **argv);
agc_status_t siggw_main_real(const char *cmd, agc_stream_handle_t *stream);

void agcsig_module_lock();
void agcsig_module_unlock();

void siggw_make_general_body(int ret, char **body);
int siggw_fire_event(char *uuid, char *body);


void add_nssai_api(agc_stream_handle_t *stream, int argc, char **argv);
void rmv_nssai_api(agc_stream_handle_t *stream, int argc, char **argv);
void mod_nssai_api(agc_stream_handle_t *stream, int argc, char **argv);
void query_nssai_api(agc_stream_handle_t *stream, int argc, char **argv);


void add_vgw_api(agc_stream_handle_t *stream, int argc, char **argv);
void rmv_vgw_api(agc_stream_handle_t *stream, int argc, char **argv);
void mod_vgw_api(agc_stream_handle_t *stream, int argc, char **argv);
void query_vgw_api(agc_stream_handle_t *stream, int argc, char **argv);


void add_taplmn_api(agc_stream_handle_t *stream, int argc, char **argv);
void rmv_taplmn_api(agc_stream_handle_t *stream, int argc, char **argv);
void mod_taplmn_api(agc_stream_handle_t *stream, int argc, char **argv);
void query_taplmn_api(agc_stream_handle_t *stream, int argc, char **argv);


void add_guami_api(agc_stream_handle_t *stream, int argc, char **argv);
void rmv_guami_api(agc_stream_handle_t *stream, int argc, char **argv);
void mod_guami_api(agc_stream_handle_t *stream, int argc, char **argv);
void query_guami_api(agc_stream_handle_t *stream, int argc, char **argv);


void add_bplmn_api(agc_stream_handle_t *stream, int argc, char **argv);
void rmv_bplmn_api(agc_stream_handle_t *stream, int argc, char **argv);
void mod_bplmn_api(agc_stream_handle_t *stream, int argc, char **argv);
void query_bplmn_api(agc_stream_handle_t *stream, int argc, char **argv);


void add_datagw_api(agc_stream_handle_t *stream, int argc, char **argv);
void rmv_datagw_api(agc_stream_handle_t *stream, int argc, char **argv);
void mod_datagw_api(agc_stream_handle_t *stream, int argc, char **argv);
void query_datagw_api(agc_stream_handle_t *stream, int argc, char **argv);


void add_ran_sctp_api(agc_stream_handle_t *stream, int argc, char **argv);
void rmv_ran_sctp_api(agc_stream_handle_t *stream, int argc, char **argv);
void mod_ran_sctp_api(agc_stream_handle_t *stream, int argc, char **argv);
void query_ran_sctp_api(agc_stream_handle_t *stream, int argc, char **argv);

void add_amf_sctp_api(agc_stream_handle_t *stream, int argc, char **argv);
void rmv_amf_sctp_api(agc_stream_handle_t *stream, int argc, char **argv);
void mod_amf_sctp_api(agc_stream_handle_t *stream, int argc, char **argv);
void query_amf_sctp_api(agc_stream_handle_t *stream, int argc, char **argv);

void status_ran_sctp_api(agc_stream_handle_t *stream, int argc, char **argv);
void status_amf_sctp_api(agc_stream_handle_t *stream, int argc, char **argv);
void query_version(agc_stream_handle_t *stream, char **body);
void query_version_api(agc_stream_handle_t *stream, int argc, char **argv);


agc_status_t CheckTableKey(OP_Scene scene, uint8_t idKey1, uint8_t idKey2, uint8_t idKey3);
void add_nssaitable(agc_stream_handle_t *stream, int argc, char **argv);
void add_vgwtable(agc_stream_handle_t *stream, int argc, char **argv);
void add_taplmntable(agc_stream_handle_t *stream, int argc, char **argv);
void add_guamitable(agc_stream_handle_t *stream, int argc, char **argv);
void add_bplmntable(agc_stream_handle_t *stream, int argc, char **argv);
void add_sctptable(agc_stream_handle_t *stream, int argc, char **argv);


//for HO test
void test_ho(agc_stream_handle_t *stream, int argc, char **argv);

AGC_END_EXTERN_C


typedef struct siggw_api_command {
    const char *pname;
    siggw_api_func func;
    const char *pcommand;
    const char *psyntax;
}siggw_api_command_t;



#endif