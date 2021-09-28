#include <agc.h>

#include "AsmManager.h"
#include "ApmManager.h"
#include "EsmManager.h"
#include "EpmManager.h"
#include "NsmManager.h"
#include "SigApi.h"
#include "NgapSessManager.h"
#include "PduSessManager.h"


AGC_BEGIN_EXTERN_C

AGC_MODULE_LOAD_FUNCTION(mod_sig_load);
AGC_MODULE_SHUTDOWN_FUNCTION(mod_sig_shutdown);
AGC_MODULE_DEFINITION(mod_sig, mod_sig_load, mod_sig_shutdown, NULL);

siggw_api_command_t siggw_api_commands[] = {
        {"add_nssai",    	add_nssai_api, 		"", ""},
        {"rmv_nssai", 		rmv_nssai_api, 		"", ""},
        {"mod_nssai",		mod_nssai_api, 		"", ""},
        {"query_nssai", 	query_nssai_api, 	"", ""},
        {"add_vgw",	    	add_vgw_api, 		"", ""},
        {"rmv_vgw", 		rmv_vgw_api, 		"", ""},
        {"mod_vgw",			mod_vgw_api, 		"", ""},
        {"query_vgw", 		query_vgw_api,	 	"", ""},
        {"add_taplmn",	   	add_taplmn_api, 	"", ""},
        {"rmv_taplmn", 		rmv_taplmn_api, 	"", ""},
        {"mod_taplmn", 		mod_taplmn_api, 	"", ""},       
        {"query_taplmn",	query_taplmn_api, 	"", ""},
        {"add_guami",    	add_guami_api, 		"", ""},
        {"rmv_guami", 		rmv_guami_api, 		"", ""},
        {"mod_guami",		mod_guami_api, 		"", ""},
        {"query_guami", 	query_guami_api, 	"", ""},        
        {"add_bplmn",    	add_bplmn_api, 		"", ""},
        {"rmv_bplmn", 		rmv_bplmn_api, 		"", ""},
        {"mod_bplmn", 		mod_bplmn_api, 		"", ""},
        {"query_bplmn", 	query_bplmn_api, 	"", ""},       
        {"add_datagw",    	add_datagw_api,		"", ""},
        {"rmv_datagw", 		rmv_datagw_api,		"", ""},
        {"mod_datagw", 		mod_datagw_api,		"", ""},
        {"query_datagw", 	query_datagw_api, 	"", ""},        
        {"add_ransctp",    	add_ran_sctp_api, 	"", ""},
        {"rmv_ransctp", 	rmv_ran_sctp_api, 	"", ""},
        {"mod_ransctp",		mod_ran_sctp_api, 	"", ""},
        {"query_ransctp", 	query_ran_sctp_api,	"", ""},
        {"add_amfsctp",    	add_amf_sctp_api, 	"", ""},
        {"rmv_amfsctp", 	rmv_amf_sctp_api, 	"", ""},
        {"mod_amfsctp",		mod_amf_sctp_api, 	"", ""},
        {"query_amfsctp", 	query_amf_sctp_api,	"", ""},
        {"status_ransctp", 	status_ran_sctp_api,"", ""},
        {"status_amfsctp", 	status_amf_sctp_api,"", ""},
        {"create_nssai", 	add_nssaitable, 	"", ""},
        {"create_vgw", 		add_vgwtable, 		"", ""},
        {"create_taplmn", 	add_taplmntable, 	"", ""},
        {"create_guami", 	add_guamitable, 	"", ""},
        {"create_bplmn", 	add_bplmntable, 	"", ""},
        {"create_sctp", 	add_sctptable, 		"", ""},
        {"query_ver", 		query_version_api,	"", ""},

//For Handover test
		{"testho",	test_ho, 	"", ""},
		
};
		
static agc_memory_pool_t 	*module_pool = NULL;
static agc_mutex_t			*module_mutex = NULL;

static char siggwElementName[] 	= {"SigGateway"};
static char siggwVersion[] 		= {"Version 1.0.0"};



AGC_STANDARD_API(siggw_api_main)
{
    return siggw_main_real(cmd, stream);
}

AGC_MODULE_LOAD_FUNCTION(mod_sig_load)
{
    assert(pool);
    module_pool = pool;

    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "sig init success.\n");
	*module_interface = agc_loadable_module_create_interface(pool, modname);

    agc_mutex_init(&module_mutex, AGC_MUTEX_NESTED, module_pool);

	//GetSctpLayerDrv().Init();
	agc_api_register("siggw", "siggw API", "syntax", siggw_api_main);



	GetAsmManager().Init();
	GetEsmManager().Init();
	GetNsmManager().Init();

	GetNgapSessManager().Init();
	GetPduSessManager().Init();

	APM_Init();
	Epm_Init();

    return AGC_STATUS_SUCCESS;
}

AGC_MODULE_SHUTDOWN_FUNCTION(mod_sig_shutdown)
{
	GetPduSessManager().Exit();
	GetNsmManager().Exit();
    GetNsmManager().WaitForExit();
	GetEsmManager().Exit();
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "sig shutdown success.\n");
    return AGC_STATUS_SUCCESS;
}

agc_memory_pool_t *agcsig_get_sig_memory_pool()
{
	return module_pool;
}

void agcsig_module_lock()
{
	//agc_mutex_lock(module_mutex);
}

void agcsig_module_unlock()
{
	//agc_mutex_unlock(module_mutex);
}


agc_status_t siggw_main_real(const char *cmd, agc_stream_handle_t *stream)
{
    char *cmdbuf = NULL;
    agc_status_t status = AGC_STATUS_SUCCESS;
    int argc, i;
    int found = 0;
    char *argv[25] = { 0 };

    //agc_log_printf(AGC_LOG, AGC_LOG_INFO, "siggw main enter.\n");

    if (!(cmdbuf = strdup(cmd))) {
        return status;
    }

    argc = agc_separate_string(cmdbuf , ' ', argv, (sizeof(argv) / sizeof(argv[0])));

    if (argc && argv[0]) {
        for (i = 0; i < SIGGW_API_SIZE; i++) {
            if (strcasecmp(argv[0], siggw_api_commands[i].pname) == 0) {
                siggw_api_func pfunc = siggw_api_commands[i].func;
                int sub_argc = argc - 1;
                char **sub_argv = NULL;

                if (argc > 1)
                    sub_argv = &argv[1];

                pfunc(stream, sub_argc, sub_argv);

                found = 1;
                break;
            }
        }

        if (!found) {
            status = AGC_STATUS_GENERR;
		    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "siggw %s not found.\n", argv[0]);
            stream->write_function(stream, "123\n");
        }
    }

    agc_safe_free(cmdbuf);

    return status;

}


void siggw_make_general_body(int ret, char **body)
{
    cJSON_Hooks json_hooks;
    cJSON *json_object;
	char errmsg[255];

    json_hooks.malloc_fn = malloc;
    json_hooks.free_fn = free;
    cJSON_InitHooks(&json_hooks);

    json_object = cJSON_CreateObject();
    cJSON_AddNumberToObject(json_object, "ret", ret);

	memset(errmsg, 0, 255);
	switch (ret)
	{
		case AGC_STATUS_GENERR:
		strncpy(errmsg, "datebase operate error!", strlen("datebase operate error!"));
		break;

		case AGC_STATUS_SIGAPI_INPUT_ERROR:
		strncpy(errmsg, "input error!", strlen("input error!"));
		break;
	}

	cJSON_AddStringToObject(json_object, "msg", errmsg);

    *body = cJSON_Print(json_object);
    cJSON_Delete(json_object);

    agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "siggw general json body: %s.\n", *body);
	return;
}



int siggw_fire_event(char *uuid, char *body)
{
    agc_event_t *new_event = NULL;
    char *event_json;

    if ((agc_event_create(&new_event, EVENT_ID_CMDRESULT, EVENT_NULL_SOURCEID) != AGC_STATUS_SUCCESS) || !new_event)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "siggw create event [fail].\n");
        return AGC_STATUS_FALSE;
    }


	if (uuid != NULL)
    {
        if (agc_event_add_header(new_event, EVENT_HEADER_UUID, "%s", uuid) != AGC_STATUS_SUCCESS) {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "siggw add header [fail].\n");
            agc_event_destroy(&new_event);
            return AGC_STATUS_FALSE;
        }
    }
    else
    {
        if (agc_event_add_header(new_event, EVENT_HEADER_UUID, "_null") != AGC_STATUS_SUCCESS) {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "siggw add header [fail].\n");
            agc_event_destroy(&new_event);
            return AGC_STATUS_FALSE;
        }
    }


    if (agc_event_set_body(new_event, body) != AGC_STATUS_SUCCESS)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "siggw set body [fail].\n");
        agc_event_destroy(&new_event);
        return AGC_STATUS_FALSE;
    }


    agc_event_serialize_json(new_event, &event_json);
//    agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "siggw event body: %s\n", event_json);
    free(event_json);


    if (agc_event_fire(&new_event) != AGC_STATUS_SUCCESS) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "siggw event fire [fail].\n");
        agc_event_destroy(&new_event);
        return AGC_STATUS_FALSE;
    }

	
    agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "siggw event fire [ok].\n");

	return AGC_STATUS_SUCCESS;
}


//Sig API Start-----------------------------------------------------------------
//table 1 nssai
void add_nssai_api(agc_stream_handle_t *stream, int argc, char **argv)
{
	uint8_t idNssai = INVALIDE_NULL_int8;
	uint8_t SST = INVALIDE_NULL_int8; 
	uint32_t SD = INVALIDE_NULL_int32;
    int result = 0;
	char *body = NULL;

	//step 1. write into db.
	//step 1.1 prepare parameter from argv.
	if(argv == NULL || argc < 3)
	{
		stream->write_function(stream, "add_nssai input arg invalid.\n");
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "add_nssai_api : input arg invalid.\n");	
		result = AGC_STATUS_SIGAPI_INPUT_ERROR;

	    siggw_make_general_body(result, &body);
	    siggw_fire_event(stream->uuid, body);
	    free(body);
		
		return;
	}
	else
	{
		idNssai = atoi(argv[0]);  // Need Update.
		SST = atoi(argv[1]);

		if (0 != strcmp(argv[2], "null")) //SD input is valid
		{
			SD = atoi(argv[2]);
		}
		//agc_log_printf(AGC_LOG, AGC_LOG_INFO, "add_nssai_api : input arg %d ,%d, %d.\n", idNssai, SST, SD);	
	}

	//Check table value
	if (CheckTableKey(OP_Scene_ADD_Nssai ,idNssai, 0, 0))
	{
		stream->write_function(stream, "add_nssai idNssai input invalid.\n");
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "add_nssai idNssai input invalid.\n"); 
		return;
	}

	result = sig_api_add_nssai(stream, idNssai, SST, SD);

	//setp 2. Prepare rsp body and Rsp to OM side.
    siggw_make_general_body(result, &body);
    siggw_fire_event(stream->uuid, body);
    free(body);
  
	return;
}


void rmv_nssai_api(agc_stream_handle_t *stream, int argc, char **argv)
{
	uint32_t ID = INVALIDE_NULL_int32;	
    int result = 0;
	char *body = NULL;

	//step 1. write into db.
	//step 1.1 prepare parameter from argv.
	if(argv == NULL)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "rmv_nssai_api : input arg invalid.\n");	

		result = AGC_STATUS_SIGAPI_INPUT_ERROR;

	    siggw_make_general_body(result, &body);
	    siggw_fire_event(stream->uuid, body);
	    free(body);

		return;
	}
	else
	{
		ID = atoi(argv[0]);
		//agc_log_printf(AGC_LOG, AGC_LOG_INFO, "rmv_nssai_api : input idNssai: %d.\n", idNssai);
	}

	result = sig_api_del_nssai(stream, ID);

	//setp 2. Prepare rsp body and Rsp to OM side.
    siggw_make_general_body(result, &body);
    siggw_fire_event(stream->uuid, body);
    free(body);    

	return;
}

void mod_nssai_api(agc_stream_handle_t *stream, int argc, char **argv)
{
	uint32_t ID = INVALIDE_NULL_int32;
	uint8_t idNssai = INVALIDE_NULL_int8;
	uint8_t SST = INVALIDE_NULL_int8; 
	uint32_t SD = INVALIDE_NULL_int32;
    int result = 0;
	char *body = NULL;

	//step 1. write into db.
	//step 1.1 prepare parameter from argv.
	if(argv == NULL || argc < 4)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "mod_nssai_api : input arg invalid.\n");	
		result = AGC_STATUS_SIGAPI_INPUT_ERROR;

	    siggw_make_general_body(result, &body);
	    siggw_fire_event(stream->uuid, body);
	    free(body);

		return;
	}
	else
	{
		ID 		= atoi(argv[0]);
		idNssai = atoi(argv[1]);
		SST 	= atoi(argv[2]);
		if (0 != strcmp(argv[3], "null")) //SD input is valid
		{
			SD = atoi(argv[3]);
		}
		//agc_log_printf(AGC_LOG, AGC_LOG_INFO, "add_nssai_api : input arg %d ,%d, %d.\n", idNssai, SST, SD);	
	}

	result = sig_api_update_nssai(stream, ID, idNssai, SST, SD);

	//setp 2. Prepare rsp body and Rsp to OM side.
    siggw_make_general_body(result, &body);
    siggw_fire_event(stream->uuid, body);
    free(body);
    
	return;
}


void query_nssai_api(agc_stream_handle_t *stream, int argc, char **argv)
{
	char *body = NULL;
	int page_index, page_size;
    int result = 0;

	//Query all record
	//nssai_query(stream, &body);	 		


	//Query N record each timer.
	if(argv == NULL || argc < 2)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "query_nssai_api : input arg invalid.\n");	
		result = AGC_STATUS_SIGAPI_INPUT_ERROR;

	    siggw_make_general_body(result, &body);
	    siggw_fire_event(stream->uuid, body);
	    free(body);

		return;
	}
	else
	{
		page_index = atoi(argv[0]);
		page_size = atoi(argv[1]);
	}

	//setp 1. query and prepare rsp body.
	nssai_queryN(stream, &body, page_index, page_size);


	//setp 2. Rsp to OM side.
    siggw_fire_event(stream->uuid, body);
    free(body);
  
	return;
}


//table 2 vgw
void add_vgw_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result = 0;
	char *body = NULL;

	//step 1. write into db.
	result = vgw_insert(stream, argc, argv);

	//setp 2. Prepare rsp body and Rsp to OM side.
    siggw_make_general_body(result, &body);
    siggw_fire_event(stream->uuid, body);
    free(body);
  
	return;
}


void rmv_vgw_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result = 0;
	char *body = NULL;

	//step 1. write into db.

	result = vgw_delete(stream, argc, argv);

	//setp 2. Prepare rsp body and Rsp to OM side.
    siggw_make_general_body(result, &body);
    siggw_fire_event(stream->uuid, body);
    free(body);    

	return;
}

void mod_vgw_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result = 0;
	char *body = NULL;

	//step 1. write into db.
	result = vgw_update(stream, argc, argv);

	//setp 2. Prepare rsp body and Rsp to OM side.
    siggw_make_general_body(result, &body);
    siggw_fire_event(stream->uuid, body);
    free(body);
    
	return;
}


void query_vgw_api(agc_stream_handle_t *stream, int argc, char **argv)
{
	char *body = NULL;
	int page_index, page_size;
    int result = 0;

	//Query N record each timer.
	if(argv == NULL || argc < 2)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "query_vgw_api : input arg invalid.\n");	
		result = AGC_STATUS_SIGAPI_INPUT_ERROR;

	    siggw_make_general_body(result, &body);
	    siggw_fire_event(stream->uuid, body);
	    free(body);

		return;
	}
	else
	{
		page_index = atoi(argv[0]);
		page_size = atoi(argv[1]);
	}

	//setp 1. query and prepare rsp body.
	vgw_queryN(stream, &body, page_index, page_size);

	//setp 2. Rsp to OM side.
    siggw_fire_event(stream->uuid, body);
    free(body);
  
	return;
}





//table 3 taplmn
void add_taplmn_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result = 0;
	char *body = NULL;

	//step 1. write into db.
	result = taplmn_insert(stream, argc, argv);

	//setp 2. Prepare rsp body and Rsp to OM side.
    siggw_make_general_body(result, &body);
    siggw_fire_event(stream->uuid, body);
    free(body);
  
	return;
}


void rmv_taplmn_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result = 0;
	char *body = NULL;

	//step 1. write into db.

	result = taplmn_delete(stream, argc, argv);

	//setp 2. Prepare rsp body and Rsp to OM side.
    siggw_make_general_body(result, &body);
    siggw_fire_event(stream->uuid, body);
    free(body);

	return;
}


void mod_taplmn_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result = 0;
	char *body = NULL;

	//step 1. write into db.
	result = taplmn_update(stream, argc, argv);

	//setp 2. Prepare rsp body and Rsp to OM side.
    siggw_make_general_body(result, &body);
    siggw_fire_event(stream->uuid, body);
    free(body);
    
	return;
}


void query_taplmn_api(agc_stream_handle_t *stream, int argc, char **argv)
{
	char *body = NULL;
	int page_index, page_size;
    int result = 0;

	//Query N record each timer.
	if(argv == NULL || argc < 2)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "query_taplmn_api : input arg invalid.\n");	
		result = AGC_STATUS_SIGAPI_INPUT_ERROR;

	    siggw_make_general_body(result, &body);
	    siggw_fire_event(stream->uuid, body);
	    free(body);

		return;
	}
	else
	{
		page_index = atoi(argv[0]);
		page_size = atoi(argv[1]);
	}

	//setp 1. query and prepare rsp body.
	taplmn_queryN(stream, &body, page_index, page_size);

	//setp 2. Rsp to OM side.
    siggw_fire_event(stream->uuid, body);
    free(body);
  
	return;
}


//table 4 guami
void add_guami_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result = 0;
	char *body = NULL;

	//step 1. write into db.
	result = guami_insert(stream, argc, argv);

	//setp 2. Prepare rsp body and Rsp to OM side.
    siggw_make_general_body(result, &body);
    siggw_fire_event(stream->uuid, body);
    free(body);
  
	return;
}


void rmv_guami_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result = 0;
	char *body = NULL;

	//step 1. write into db.

	result = guami_delete(stream, argc, argv);

	//setp 2. Prepare rsp body and Rsp to OM side.
    siggw_make_general_body(result, &body);
    siggw_fire_event(stream->uuid, body);
    free(body);    

	return;
}

void mod_guami_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result = 0;
	char *body = NULL;

	//step 1. write into db.
	result = guami_update(stream, argc, argv);

	//setp 2. Prepare rsp body and Rsp to OM side.
    siggw_make_general_body(result, &body);
    siggw_fire_event(stream->uuid, body);
    free(body);
    
	return;
}


void query_guami_api(agc_stream_handle_t *stream, int argc, char **argv)
{
	char *body = NULL;
	int page_index, page_size;
    int result = 0;

	//Query N record each timer.
	if(argv == NULL || argc < 2)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "query_taplmn_api : input arg invalid.\n");	
		result = AGC_STATUS_SIGAPI_INPUT_ERROR;

	    siggw_make_general_body(result, &body);
	    siggw_fire_event(stream->uuid, body);
	    free(body);

		return;
	}
	else
	{
		page_index = atoi(argv[0]);
		page_size = atoi(argv[1]);
	}

	//setp 1. query and prepare rsp body.
	guami_queryN(stream, &body, page_index, page_size);

	//setp 2. Rsp to OM side.
    siggw_fire_event(stream->uuid, body);
    free(body);
  
	return;
}




//table 5 bplmn
void add_bplmn_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result = 0;
	char *body = NULL;

	//step 1. write into db.
	result = bplmn_insert(stream, argc, argv);

	//setp 2. Prepare rsp body and Rsp to OM side.
    siggw_make_general_body(result, &body);
    siggw_fire_event(stream->uuid, body);
    free(body);
  
	return;
}


void rmv_bplmn_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result = 0;
	char *body = NULL;

	//step 1. write into db.

	result = bplmn_delete(stream, argc, argv);

	//setp 2. Prepare rsp body and Rsp to OM side.
    siggw_make_general_body(result, &body);
    siggw_fire_event(stream->uuid, body);
    free(body);    

	return;
}


void mod_bplmn_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result = 0;
	char *body = NULL;

	//step 1. write into db.
	result = bplmn_update(stream, argc, argv);

	//setp 2. Prepare rsp body and Rsp to OM side.
    siggw_make_general_body(result, &body);
    siggw_fire_event(stream->uuid, body);
    free(body);
    
	return;
}


void query_bplmn_api(agc_stream_handle_t *stream, int argc, char **argv)
{
	char *body = NULL;
	int page_index, page_size;
    int result = 0;

	//Query N record each timer.
	if(argv == NULL || argc < 2)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "query_bplmn_api : input arg invalid.\n");	
		result = AGC_STATUS_SIGAPI_INPUT_ERROR;

	    siggw_make_general_body(result, &body);
	    siggw_fire_event(stream->uuid, body);
	    free(body);

		return;
	}
	else
	{
		page_index = atoi(argv[0]);
		page_size = atoi(argv[1]);
	}

	//setp 1. query and prepare rsp body.
	bplmn_queryN(stream, &body, page_index, page_size);

	//setp 2. Rsp to OM side.
    siggw_fire_event(stream->uuid, body);
    free(body);
  
	return;
}



//table datagw
void add_datagw_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result = 0;
	char *body = NULL;

	//step 1. write into db.
	result = datagw_insert(stream, argc, argv);

	//setp 2. Prepare rsp body and Rsp to OM side.
    siggw_make_general_body(result, &body);
    siggw_fire_event(stream->uuid, body);
    free(body);
  
	return;
}


void rmv_datagw_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result = 0;
	char *body = NULL;

	//step 1. write into db.

	result = datagw_delete(stream, argc, argv);

	//setp 2. Prepare rsp body and Rsp to OM side.
    siggw_make_general_body(result, &body);
    siggw_fire_event(stream->uuid, body);
    free(body);    

	return;
}


void mod_datagw_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result = 0;
	char *body = NULL;

	//step 1. write into db.
	result = datagw_update(stream, argc, argv);

	//setp 2. Prepare rsp body and Rsp to OM side.
    siggw_make_general_body(result, &body);
    siggw_fire_event(stream->uuid, body);
    free(body);
    
	return;
}


void query_datagw_api(agc_stream_handle_t *stream, int argc, char **argv)
{
	char *body = NULL;
	int page_index, page_size;
    int result = 0;

	//Query N record each timer.
	if(argv == NULL || argc < 2)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "query_datagw_api : input arg invalid.\n");	
		result = AGC_STATUS_SIGAPI_INPUT_ERROR;

	    siggw_make_general_body(result, &body);
	    siggw_fire_event(stream->uuid, body);
	    free(body);

		return;
	}
	else
	{
		page_index = atoi(argv[0]);
		page_size = atoi(argv[1]);
	}

	//setp 1. query and prepare rsp body.
	datagw_queryN(stream, &body, page_index, page_size);

	//setp 2. Rsp to OM side.
    siggw_fire_event(stream->uuid, body);
    free(body);
  
	return;
}




//table 6 ran-sctp
void add_ran_sctp_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result = 0;
	char *body = NULL;

	//step 1. write into db.
	result = sctp_insert(stream, argc, argv, SCTP_RAN);

	//setp 2. Prepare rsp body and Rsp to OM side.
    siggw_make_general_body(result, &body);
    siggw_fire_event(stream->uuid, body);
    free(body);
  
	return;
}


void rmv_ran_sctp_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result = 0;
	char *body = NULL;

	//step 1. write into db.

	result = sctp_delete(stream, argc, argv, SCTP_RAN);

	//setp 2. Prepare rsp body and Rsp to OM side.
    siggw_make_general_body(result, &body);
    siggw_fire_event(stream->uuid, body);
    free(body);    

	return;
}

void mod_ran_sctp_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result = 0;
	char *body = NULL;

	//step 1. write into db.
	result = sctp_update(stream, argc, argv, SCTP_RAN);

	//setp 2. Prepare rsp body and Rsp to OM side.
    siggw_make_general_body(result, &body);
    siggw_fire_event(stream->uuid, body);
    free(body);
    
	return;
}


void query_ran_sctp_api(agc_stream_handle_t *stream, int argc, char **argv)
{
	char *body = NULL;
	int page_index, page_size;
    int result = 0;

	//Query N record each timer.
	if(argv == NULL || argc < 2)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "query_ran_sctp_api : input arg invalid.\n");	
		result = AGC_STATUS_SIGAPI_INPUT_ERROR;

	    siggw_make_general_body(result, &body);
	    siggw_fire_event(stream->uuid, body);
	    free(body);

		return;
	}
	else
	{
		page_index = atoi(argv[0]);
		page_size = atoi(argv[1]);
	}

	//setp 1. query and prepare rsp body.
	sctp_queryN(stream, &body, page_index, page_size, SCTP_RAN);

	//setp 2. Rsp to OM side.
    siggw_fire_event(stream->uuid, body);
    free(body);
  
	return;
}

//table 6 amf-sctp
void add_amf_sctp_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result = 0;
	char *body = NULL;

	//step 1. write into db.
	result = sctp_insert(stream, argc, argv, SCTP_AMF);

	//setp 2. Prepare rsp body and Rsp to OM side.
    siggw_make_general_body(result, &body);
    siggw_fire_event(stream->uuid, body);
    free(body);
  
	return;
}


void rmv_amf_sctp_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result = 0;
	char *body = NULL;

	//step 1. write into db.

	result = sctp_delete(stream, argc, argv, SCTP_AMF);

	//setp 2. Prepare rsp body and Rsp to OM side.
    siggw_make_general_body(result, &body);
    siggw_fire_event(stream->uuid, body);
    free(body);    

	return;
}

void mod_amf_sctp_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result = 0;
	char *body = NULL;

	//step 1. write into db.
	result = sctp_update(stream, argc, argv, SCTP_AMF);

	//setp 2. Prepare rsp body and Rsp to OM side.
    siggw_make_general_body(result, &body);
    siggw_fire_event(stream->uuid, body);
    free(body);
    
	return;
}


void query_amf_sctp_api(agc_stream_handle_t *stream, int argc, char **argv)
{
	char *body = NULL;
	int page_index, page_size;
    int result = 0;

	//Query N record each timer.
	if(argv == NULL || argc < 2)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "query_amf_sctp_api : input arg invalid.\n");	
		result = AGC_STATUS_SIGAPI_INPUT_ERROR;

	    siggw_make_general_body(result, &body);
	    siggw_fire_event(stream->uuid, body);
	    free(body);

		return;
	}
	else
	{
		page_index = atoi(argv[0]);
		page_size = atoi(argv[1]);
	}

	//setp 1. query and prepare rsp body.
	sctp_queryN(stream, &body, page_index, page_size, SCTP_AMF);

	//setp 2. Rsp to OM side.
    siggw_fire_event(stream->uuid, body);
    free(body);
  
	return;
}




void status_ran_sctp_api(agc_stream_handle_t *stream, int argc, char **argv)
{
	char *body = NULL;
	int page_index, page_size;
    int result = 0;

	//Query N record status each timer.
	if(argv == NULL || argc < 2)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "status_ran_sctp_api : input arg invalid.\n");	
		result = AGC_STATUS_SIGAPI_INPUT_ERROR;

	    siggw_make_general_body(result, &body);
	    siggw_fire_event(stream->uuid, body);
	    free(body);

		return;
	}
	else
	{
		page_index = atoi(argv[0]);
		page_size = atoi(argv[1]);
	}

	//setp 1. query and prepare rsp body.
	sctp_statusN(stream, &body, page_index, page_size, SCTP_RAN);

	//setp 2. Rsp to OM side.
    siggw_fire_event(stream->uuid, body);
    free(body);
  
	return;
}


void status_amf_sctp_api(agc_stream_handle_t *stream, int argc, char **argv)
{
	char *body = NULL;
	int page_index, page_size;
    int result = 0;

	//Query N record each timer.
	if(argv == NULL || argc < 2)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "status_amf_sctp_api : input arg invalid.\n");	
		result = AGC_STATUS_SIGAPI_INPUT_ERROR;

	    siggw_make_general_body(result, &body);
	    siggw_fire_event(stream->uuid, body);
	    free(body);

		return;
	}
	else
	{
		page_index = atoi(argv[0]);
		page_size = atoi(argv[1]);
	}

	//setp 1. query and prepare rsp body.
	sctp_statusN(stream, &body, page_index, page_size, SCTP_AMF);

	//setp 2. Rsp to OM side.
    siggw_fire_event(stream->uuid, body);
    free(body);
  
	return;
}


void query_version(agc_stream_handle_t *stream, char **body)
{
    cJSON_Hooks json_hooks;
    cJSON *json_object;

    json_hooks.malloc_fn = malloc;
    json_hooks.free_fn = free;
    cJSON_InitHooks(&json_hooks);

    json_object = cJSON_CreateObject();
    cJSON_AddStringToObject(json_object, "ElementName", siggwElementName);
	cJSON_AddStringToObject(json_object, "ElementVersion", siggwVersion);

    *body = cJSON_Print(json_object);
    cJSON_Delete(json_object);

    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "siggw version json body: %s.\n", *body);

	return;

}



void query_version_api(agc_stream_handle_t *stream, int argc, char **argv)
{
	char *body = NULL;

/*
	int page_index, page_size;
    int result = 0;

	if(argv == NULL)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "query_version_api : input arg invalid.\n");	
		result = AGC_STATUS_SIGAPI_INPUT_ERROR;

	    siggw_make_general_body(result, &body);
	    siggw_fire_event(stream->uuid, body);
	    free(body);

		return;
	}
*/
	query_version(stream, &body);
    siggw_fire_event(stream->uuid, body);
    free(body);
  
	return;
}




agc_status_t CheckTableKey(OP_Scene scene, uint8_t idKey1, uint8_t idKey2, uint8_t idKey3)
{
	agc_status_t result = AGC_STATUS_SUCCESS;

	switch (scene)
	{
		case OP_Scene_ADD_Nssai:
			
		
			break;
	

		default:

			break;
		
	}


	return result;
}




//Only for test  --------------------------------------------------------------
void add_nssaitable(agc_stream_handle_t *stream, int argc, char **argv)
{
	agc_db_t *db = NULL;
	char *err_msg = NULL;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME))) {
		stream->write_function(stream, "addtable - open db [fail].\n");
		return;
	}
	stream->write_function(stream, "addtable - open [ok].\n");

	char *sql = agc_mprintf("create table sig_nssai_table(idNssai integer PRIMARY KEY AUTOINCREMENT, SST int, SD int);");

	if (agc_db_exec(db, sql, NULL, NULL, &err_msg) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "addtable failed %s .\n", err_msg);
		stream->write_function(stream, "addtable [fail].\n");
		agc_db_free(err_msg);
		agc_db_close(db);
		agc_safe_free(sql);
		return;
	}

	stream->write_function(stream, "addtable - create [ok].\n");
	agc_db_close(db);
	agc_safe_free(sql);
	return;

}


void add_vgwtable(agc_stream_handle_t *stream, int argc, char **argv)
{
	agc_db_t *db = NULL;
	char *err_msg = NULL;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME))) {
		stream->write_function(stream, "addtable - open db [fail].\n");
		return;
	}
	stream->write_function(stream, "addtable - open [ok].\n");

	char *sql = agc_mprintf("create table sig_vgw_table(idSigGW integer PRIMARY KEY AUTOINCREMENT, AMFName varchar(255), AMFCapacity int, gNBid int UNIQUE, gNBName varchar(255), MCC int, MNC int, DRX int, gNBType int, ngeNBType int);");

	if (agc_db_exec(db, sql, NULL, NULL, &err_msg) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "addtable failed %s .\n", err_msg);
		stream->write_function(stream, "addtable [fail].\n");
		agc_db_free(err_msg);
		agc_db_close(db);
		agc_safe_free(sql);
		return;
	}

	stream->write_function(stream, "addtable - create [ok].\n");
	agc_db_close(db);
	agc_safe_free(sql);
	return;

}


void add_taplmntable(agc_stream_handle_t *stream, int argc, char **argv)
{
	agc_db_t *db = NULL;
	char *err_msg = NULL;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME))) {
		stream->write_function(stream, "addtable - open db [fail].\n");
		return;
	}
	stream->write_function(stream, "addtable - open [ok].\n");

	char *sql = agc_mprintf("create table sig_taplmn_table(idSigGW integer, idTA int, TAC int, idBplmn int);");

	if (agc_db_exec(db, sql, NULL, NULL, &err_msg) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "addtable failed %s .\n", err_msg);
		stream->write_function(stream, "addtable [fail].\n");
		agc_db_free(err_msg);
		agc_db_close(db);
		agc_safe_free(sql);
		return;
	}

	stream->write_function(stream, "addtable - create [ok].\n");
	agc_db_close(db);
	agc_safe_free(sql);
	return;

}



void add_guamitable(agc_stream_handle_t *stream, int argc, char **argv)
{
	agc_db_t *db = NULL;
	char *err_msg = NULL;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME))) {
		stream->write_function(stream, "addtable - open db [fail].\n");
		return;
	}
	stream->write_function(stream, "addtable - open [ok].\n");

	char *sql = agc_mprintf("create table sig_guami_table(idSigGW int, idAMF int, MCC int, MNC int, AMFRegionID int, AMFSetId int, AMFPointer int, PRIMARY KEY (idSigGW, idAMF));");

	if (agc_db_exec(db, sql, NULL, NULL, &err_msg) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "addtable failed %s .\n", err_msg);
		stream->write_function(stream, "addtable [fail].\n");
		agc_db_free(err_msg);
		agc_db_close(db);
		agc_safe_free(sql);
		return;
	}

	stream->write_function(stream, "addtable - create [ok].\n");
	agc_db_close(db);
	agc_safe_free(sql);
	return;

}



void add_bplmntable(agc_stream_handle_t *stream, int argc, char **argv)
{
	agc_db_t *db = NULL;
	char *err_msg = NULL;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME))) {
		stream->write_function(stream, "addtable - open db [fail].\n");
		return;
	}
	stream->write_function(stream, "addtable - open [ok].\n");

	char *sql = agc_mprintf("create table sig_bplmn_table(idSigGW int, idBplmn int, MCC int, MNC int, idNssai int);");

	if (agc_db_exec(db, sql, NULL, NULL, &err_msg) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "addtable failed %s .\n", err_msg);
		stream->write_function(stream, "addtable [fail].\n");
		agc_db_free(err_msg);
		agc_db_close(db);
		agc_safe_free(sql);
		return;
	}

	stream->write_function(stream, "addtable - create [ok].\n");
	agc_db_close(db);
	agc_safe_free(sql);
	return;

}


void add_sctptable(agc_stream_handle_t *stream, int argc, char **argv)
{
	agc_db_t *db = NULL;
	char *err_msg = NULL;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME))) {
		stream->write_function(stream, "addtable - open db [fail].\n");
		return;
	}
	stream->write_function(stream, "addtable - open [ok].\n");

	char *sql = agc_mprintf("create table sig_sctp_table(idSctpIndex integer, idSigGW int, Priority int, WorkType int, iptype int, LocalIP1 varchar(64), LocalIP2 varchar(64),LocalPort int, PeerIP1 varchar(64), PeerIP2 varchar(64), PeerPort int, HeartBeatTime_s int, SctpNum int, gnbOrAmf int, PRIMARY KEY (idSctpIndex, idSigGW));");

	if (agc_db_exec(db, sql, NULL, NULL, &err_msg) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "addtable failed %s .\n", err_msg);
		stream->write_function(stream, "addtable [fail].\n");
		agc_db_free(err_msg);
		agc_db_close(db);
		agc_safe_free(sql);
		return;
	}

	stream->write_function(stream, "addtable - create [ok].\n");
	agc_db_close(db);
	agc_safe_free(sql);
	return;

}

//Create table END --- For TEST ---------------------------------------------------------------------


//for HO test
void test_ho(agc_stream_handle_t *stream, int argc, char **argv)
{
	//prepare info - HandoverRequired MSG.
	NgapMessage info;

	Ngap_InitiatingMessage_t	*initiatingMessage = NULL;
	Ngap_HandoverRequiredIEs_t   *ie = NULL;
	Ngap_HandoverRequired_t  *HandoverRequired = NULL;

    info.amfUeId = 1;
    info.sessId = 1;
    info.gnbUeId = 1;
	info.amfId = 1;
//	info.stream_no;

	info.PDUChoice = Ngap_NGAP_PDU_PR_initiatingMessage;
	info.ProcCode = Ngap_ProcedureCode_id_HandoverPreparation;

	info.sctpIndex = 1;
	info.idSigGW = 1;
//	info.ranNodeId = 1;
	
	memset(&info.ngapMessage, 0, sizeof (Ngap_NGAP_PDU_t));
	
    info.ngapMessage.present = Ngap_NGAP_PDU_PR_initiatingMessage;
    info.ngapMessage.choice.initiatingMessage = (Ngap_InitiatingMessage_t *)calloc(1, sizeof(Ngap_InitiatingMessage_t));
    initiatingMessage = info.ngapMessage.choice.initiatingMessage;
	
    initiatingMessage->procedureCode = Ngap_ProcedureCode_id_HandoverPreparation;
    initiatingMessage->criticality = Ngap_Criticality_reject;
    initiatingMessage->value.present = Ngap_InitiatingMessage__value_PR_HandoverRequired;

	HandoverRequired = &(initiatingMessage->value.choice.HandoverRequired);

//		union Ngap_HandoverRequiredIEs__Ngap_value_u {
//			Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
//			Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
//			Ngap_HandoverType_t	 HandoverType;
//			Ngap_Cause_t	 Cause;
//			Ngap_TargetID_t	 TargetID;
//			Ngap_DirectForwardingPathAvailability_t	 DirectForwardingPathAvailability;
//			Ngap_PDUSessionResourceListHORqd_t	 PDUSessionResourceListHORqd;
//			Ngap_SourceToTarget_TransparentContainer_t	 SourceToTarget_TransparentContainer;
//		} choice;

	//Make IE
	

	//AMF_UE_NGAP_ID
		ie = (Ngap_HandoverRequiredIEs_t *)calloc(1, sizeof(Ngap_HandoverRequiredIEs_t));
		ASN_SEQUENCE_ADD(&HandoverRequired->protocolIEs, ie);
		
		ie->id = Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
		ie->criticality = Ngap_Criticality_reject;
		ie->value.present = Ngap_HandoverRequiredIEs__value_PR_AMF_UE_NGAP_ID;
		asn_int642INTEGER(&ie->value.choice.AMF_UE_NGAP_ID, (uint64_t)info.amfUeId);

	//RAN_UE_NGAP_ID
		ie = (Ngap_HandoverRequiredIEs_t *)calloc(1, sizeof(Ngap_HandoverRequiredIEs_t));
		ASN_SEQUENCE_ADD(&HandoverRequired->protocolIEs, ie);
		
		ie->id = Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
		ie->criticality = Ngap_Criticality_reject;
		ie->value.present = Ngap_HandoverRequiredIEs__value_PR_RAN_UE_NGAP_ID;
		ie->value.choice.RAN_UE_NGAP_ID = info.gnbUeId;
		
	
	//HandoverType
		ie = (Ngap_HandoverRequiredIEs_t *)calloc(1, sizeof(Ngap_HandoverRequiredIEs_t));
		ASN_SEQUENCE_ADD(&HandoverRequired->protocolIEs, ie);
		
		ie->id = Ngap_ProtocolIE_ID_id_HandoverType;
		ie->criticality = Ngap_Criticality_reject;
		ie->value.present = Ngap_HandoverRequiredIEs__value_PR_HandoverType;
		ie->value.choice.HandoverType = Ngap_HandoverType_intra5gs;
		
	//Cause
		ie = (Ngap_HandoverRequiredIEs_t *)calloc(1, sizeof(Ngap_HandoverRequiredIEs_t));
		ASN_SEQUENCE_ADD(&HandoverRequired->protocolIEs, ie);
		
		ie->id = Ngap_ProtocolIE_ID_id_Cause;
		ie->criticality = Ngap_Criticality_ignore;
		ie->value.present = Ngap_HandoverRequiredIEs__value_PR_Cause;
		ie->value.choice.Cause.choice.radioNetwork = Ngap_CauseRadioNetwork_radio_resources_not_available;

	//TargetID
	

	//GNB_HandoverRequired(info);

	return;

}




AGC_END_EXTERN_C
