#include <agc.h>

#include "mod_media.h"
#include "media_context.h"
#include "media_gtp_path.h"
#include "media_sig_path.h"
#include "media_tun_path.h"
#include "media_db.h"

AGC_MODULE_LOAD_FUNCTION(mod_media_load);
AGC_MODULE_SHUTDOWN_FUNCTION(mod_media_shutdown);
AGC_MODULE_DEFINITION(mod_media, mod_media_load, mod_media_shutdown, NULL);

datagw_api_command_t datagw_api_commands[] = {
        {"add_lbo",    	add_lbo_api, 		"", ""},
        {"rmv_lbo", 	rmv_lbo_api, 		"", ""},
        {"mod_lbo",		mod_lbo_api, 		"", ""},
        {"query_lbo", 	query_lbo_api, 		"", ""}, 
        {"query_dgver", query_datagw_version_api,	"", ""},
};

static agc_memory_pool_t 	*module_pool = NULL;
static agc_mutex_t			*module_mutex = NULL;
static char datagwElementName[]	= {"DataGateway"};
static char datagwVersion[] 	= {"Version 1.0.0"};

AGC_STANDARD_API(datagw_api_main)
{
    return datagw_main_real(cmd, stream);
}

AGC_MODULE_LOAD_FUNCTION(mod_media_load)
{
/*    agc_std_sockaddr_t servaddr;
    socklen_t addrlen = 0;
    struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&servaddr;
    local_addr1_v4->sin_family = AF_INET;
    local_addr1_v4->sin_port  = htons(MEDIA_GTPV1_U_UDP_PORT);
    inet_pton(AF_INET, "192.168.100.103", &(local_addr1_v4->sin_addr));
    addrlen = sizeof(struct sockaddr_in);
*/
    assert(pool);
    module_pool = pool;

	*module_interface = agc_loadable_module_create_interface(pool, modname);

    agc_mutex_init(&module_mutex, AGC_MUTEX_NESTED, module_pool);

	agc_api_register("datagw", "datagw API", "syntax", datagw_api_main);

    if (media_context_init() !=  AGC_STATUS_SUCCESS) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "media context init failed.\n");
        return AGC_STATUS_GENERR;
    }
/*
    if (media_gtp_open() !=  AGC_STATUS_SUCCESS) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "mod_media_load media_gtp_open failed.\n");
        return AGC_STATUS_GENERR;
    }
*/
    //media_add_lbo_node(&servaddr, addrlen);

    media_sig_path_bind();

    media_load_ipsubnet("datatun");

    if (media_tun_path_open() !=  AGC_STATUS_SUCCESS) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "mod_media_load media_tun_path_open failed.\n");
        return AGC_STATUS_GENERR;
    }

    media_db_load_lbo();
    
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "media init success.\n");
    return AGC_STATUS_SUCCESS;
}

AGC_MODULE_SHUTDOWN_FUNCTION(mod_media_shutdown)
{
//    media_gtp_close();
    media_context_shutdown();

    return AGC_STATUS_SUCCESS;
}

agc_memory_pool_t *media_get_memory_pool()
{
	return module_pool;
}

void media_module_lock()
{
	agc_mutex_lock(module_mutex);
}

void media_module_unlock()
{
	agc_mutex_unlock(module_mutex);
}

agc_status_t datagw_main_real(const char *cmd, agc_stream_handle_t *stream)
{
    char *cmdbuf = NULL;
    agc_status_t status = AGC_STATUS_SUCCESS;
    int argc, i;
    int found = 0;
    char *argv[25] = { 0 };

    //agc_log_printf(AGC_LOG, AGC_LOG_INFO, "datagw main enter.\n");

    if (!(cmdbuf = strdup(cmd))) {
        return status;
    }

    argc = agc_separate_string(cmdbuf , ' ', argv, (sizeof(argv) / sizeof(argv[0])));

    if (argc && argv[0]) {
        for (i = 0; i < DATAGW_API_SIZE; i++) {
            if (strcasecmp(argv[0], datagw_api_commands[i].pname) == 0) {
                datagw_api_func pfunc = datagw_api_commands[i].func;
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
		    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "datagw %s not found.\n", argv[0]);
            stream->write_function(stream, "123\n");
        }
    }

    agc_safe_free(cmdbuf);

    return status;

}


void datagw_make_general_body(int ret, char **body)
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

    agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "datagw general json body: %s.\n", *body);
	return;
}


int datagw_fire_event(char *uuid, char *body)
{
    agc_event_t *new_event = NULL;
    char *event_json;

    if ((agc_event_create(&new_event, EVENT_ID_CMDRESULT, EVENT_NULL_SOURCEID) != AGC_STATUS_SUCCESS) || !new_event)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "datagw create event [fail].\n");
        return AGC_STATUS_FALSE;
    }


	if (uuid != NULL)
    {
        if (agc_event_add_header(new_event, EVENT_HEADER_UUID, "%s", uuid) != AGC_STATUS_SUCCESS) {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "datagw add header [fail].\n");
            agc_event_destroy(&new_event);
            return AGC_STATUS_FALSE;
        }
    }
    else
    {
        if (agc_event_add_header(new_event, EVENT_HEADER_UUID, "_null") != AGC_STATUS_SUCCESS) {
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "datagw add header [fail].\n");
            agc_event_destroy(&new_event);
            return AGC_STATUS_FALSE;
        }
    }


    if (agc_event_set_body(new_event, body) != AGC_STATUS_SUCCESS)
    {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "datagw set body [fail].\n");
        agc_event_destroy(&new_event);
        return AGC_STATUS_FALSE;
    }


    agc_event_serialize_json(new_event, &event_json);
//    agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "datagw event body: %s\n", event_json);
    free(event_json);


    if (agc_event_fire(&new_event) != AGC_STATUS_SUCCESS) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "datagw event fire [fail].\n");
        agc_event_destroy(&new_event);
        return AGC_STATUS_FALSE;
    }

	
    agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "datagw event fire [ok].\n");

	return AGC_STATUS_SUCCESS;
}

agc_status_t lbo_insert(agc_stream_handle_t *stream, int argc, char **argv)
{
	agc_db_t *db = NULL;
	char *err_msg = NULL;
	agc_status_t status = AGC_STATUS_SUCCESS;	

	enum Data_IPType  iptype;
	char	PeerIP2[DATAGW_IPADDR_MAX_LEN+1];
	char *sql = NULL;
	
	agc_std_sockaddr_t servaddr;
	socklen_t addrlen = 0;
	
	//step 1 prepare parameter from argv.
	if(argv == NULL || argc < 2)
	{
		stream->write_function(stream, "lbo_insert input arg invalid.\n");
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "lbo_insert input arg invalid.\n");
		status = AGC_STATUS_SIGAPI_INPUT_ERROR;
		return status;
	}
	else
	{

		if(strlen(argv[1]) > DATAGW_IPADDR_MAX_LEN)
		{
			status = AGC_STATUS_SIGAPI_INPUT_ERROR;

			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "lbo_insert input IP error.\n");
			stream->write_function(stream, "lbo_insert input IP error.\n");
			
			return status;
		}

		memset(PeerIP2, 0, DATAGW_IPADDR_MAX_LEN+1);	
		
		iptype = (enum Data_IPType)atoi(argv[0]);
		strncpy(PeerIP2, argv[1], strlen(argv[1]));
	}

	//step 2 insert to db
	if (!(db = agc_db_open_file(DATA_DB_FILENAME))) {
		stream->write_function(stream, "lbo_insert open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	sql = agc_mprintf("insert into media_lbo_table(iptype, PeerIP2) values(%d, \"%s\");" , iptype, PeerIP2);

	if (agc_db_exec(db, sql, NULL, NULL, &err_msg) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
		stream->write_function(stream, "lbo_insert db [fail].\n");
		agc_db_free(err_msg);
		agc_db_close(db);
		agc_safe_free(sql);
		return AGC_STATUS_GENERR;
	}

	if (iptype == 0)
	{
		struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&servaddr;
		local_addr1_v4->sin_family = AF_INET;
		local_addr1_v4->sin_port  = htons(MEDIA_GTPV1_U_UDP_PORT);
		inet_pton(AF_INET, PeerIP2, &(local_addr1_v4->sin_addr));
		addrlen = sizeof(struct sockaddr_in);
	}
	else
	{
		struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&servaddr;
		local_addr1_v6->sin6_family = AF_INET6;
		local_addr1_v6->sin6_port  = htons(MEDIA_GTPV1_U_UDP_PORT);
		inet_pton(AF_INET6, PeerIP2, &(local_addr1_v6->sin6_addr));
		addrlen = sizeof(struct sockaddr_in6);
	}
	
	media_add_lbo_node(&servaddr, addrlen);

	stream->write_function(stream, "lbo_insert db [ok].\n");
	agc_db_close(db);
	agc_safe_free(sql);
	return status;

}


void add_lbo_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result = 0;
	char *body = NULL;

	//step 1. write into db.
	result = lbo_insert(stream, argc, argv);

	//setp 2. Prepare rsp body and Rsp to OM side.
    datagw_make_general_body(result, &body);
    datagw_fire_event(stream->uuid, body);
    free(body);
  
	return;
}


agc_status_t lbo_delete(agc_stream_handle_t *stream, int argc, char **argv)
{
	agc_db_t *db = NULL;
	char *err_msg = NULL;

    uint32_t ID;
	char *sql = NULL;
	
	//step 1.1 prepare parameter from argv.
	if(argv == NULL)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "lbo_delete : input arg invalid.\n");	
		return AGC_STATUS_SIGAPI_INPUT_ERROR;
	}
	else
	{
		ID = atoi(argv[0]);
	}

	if (!(db = agc_db_open_file(DATA_DB_FILENAME))) {
		stream->write_function(stream, "lbo_delete open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	sql = agc_mprintf("DELETE FROM media_lbo_table WHERE ID = %d;", ID);

	if (agc_db_exec(db, sql, NULL, NULL, &err_msg) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
		stream->write_function(stream, "lbo_delete db [fail].\n");
		agc_db_free(err_msg);
		agc_db_close(db);
		agc_safe_free(sql);
		return AGC_STATUS_GENERR;
	}

	stream->write_function(stream, "lbo_delete record [ok].\n");
	agc_db_close(db);
	agc_safe_free(sql);
	return AGC_STATUS_SUCCESS;

}

void rmv_lbo_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result = 0;
	char *body = NULL;

	//step 1. write into db.
	result = lbo_delete(stream, argc, argv);

	//setp 2. Prepare rsp body and Rsp to OM side.
    datagw_make_general_body(result, &body);
    datagw_fire_event(stream->uuid, body);
    free(body);    

	return;
}

agc_status_t lbo_update(agc_stream_handle_t *stream, int argc, char **argv)
{

	agc_db_t *db = NULL;
	char *err_msg = NULL;
	agc_status_t status = AGC_STATUS_SUCCESS;
		
	uint32_t ID;
	enum Data_IPType  iptype;
	char	PeerIP2[DATAGW_IPADDR_MAX_LEN+1];
	char *sql = NULL;
	
	//step 1 prepare parameter from argv.
	if(argv == NULL || argc < 3)
	{
		stream->write_function(stream, "lbo_update input arg invalid.\n");
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "lbo_update input arg invalid.\n");
		status = AGC_STATUS_SIGAPI_INPUT_ERROR;
		return status;
	}
	else
	{
		if(strlen(argv[2]) > DATAGW_IPADDR_MAX_LEN)
		{
			status = AGC_STATUS_SIGAPI_INPUT_ERROR;

			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "lbo_update input ip error.\n");
			stream->write_function(stream, "lbo_update input ip error.\n");
			
			return status;
		}

		memset(PeerIP2, 0, DATAGW_IPADDR_MAX_LEN+1);
	
		ID 		= atoi(argv[0]);
		iptype  = (enum Data_IPType)atoi(argv[1]);
		strncpy(PeerIP2, argv[2], strlen(argv[2]));
	}

	//step 2 insert into db.
	if (!(db = agc_db_open_file(DATA_DB_FILENAME))) {
		stream->write_function(stream, "lbo_update open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	sql = agc_mprintf("UPDATE media_lbo_table SET iptype = %d ,PeerIP2 = \"%s\" WHERE ID = %d;",iptype, PeerIP2, ID);

	if (agc_db_exec(db, sql, NULL, NULL, &err_msg) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
		stream->write_function(stream, "lbo_update db [fail].\n");
		agc_db_free(err_msg);
		agc_db_close(db);
		agc_safe_free(sql);
		return AGC_STATUS_GENERR;
	}

	stream->write_function(stream, "lbo_update record [ok].\n");
	agc_db_close(db);
	agc_safe_free(sql);
	return status;
}


void mod_lbo_api(agc_stream_handle_t *stream, int argc, char **argv)
{
    int result = 0;
	char *body = NULL;

	//step 1. write into db.
	result = lbo_update(stream, argc, argv);

	//setp 2. Prepare rsp body and Rsp to OM side.
    datagw_make_general_body(result, &body);
    datagw_fire_event(stream->uuid, body);
    free(body);    
    
	return;
}


agc_status_t lbo_queryN(agc_stream_handle_t *stream, char **body, int page_index, int page_size)
{
	agc_db_t *db = NULL;
    char *err_msg = NULL;
    char **query_result = NULL;
    char **query_rec = NULL;
    int i, nrow, ncolumn;
	agc_status_t status = AGC_STATUS_SUCCESS;
	int totalRec;
    char *terr_msg = NULL;
    char **tquery_result = NULL;
	
	uint32_t ID;
	enum Data_IPType  iptype;
	char	PeerIP2[DATAGW_IPADDR_MAX_LEN+1];
	char *sql = NULL;
	char *tsql = NULL;

    cJSON_Hooks json_hooks, json_hooks2;
    cJSON *record_item, *record_array, *json_object;

	if (!(db = agc_db_open_file(DATA_DB_FILENAME)))  {
		stream->write_function(stream, "lbo_queryN open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	tsql = agc_mprintf("SELECT * FROM media_lbo_table;");
	if (agc_db_get_table(db, tsql, &tquery_result, &nrow, &ncolumn, &terr_msg) == AGC_DB_OK)
	{
		totalRec = nrow;
	}
	else
	{
		totalRec = 0;
	}

	free(tsql);
	agc_db_free(terr_msg);
	agc_db_free_table(tquery_result);

	nrow = 0;
	ncolumn	= 0;
	sql = agc_mprintf("SELECT * FROM media_lbo_table LIMIT %d OFFSET %d;", page_size, page_index);

	if (agc_db_get_table(db, sql, &query_result, &nrow, &ncolumn, &err_msg) == AGC_DB_OK)
	{
		//Query N record success
		
		json_hooks.malloc_fn = malloc;
		json_hooks.free_fn = free;
		cJSON_InitHooks(&json_hooks);
		record_array = cJSON_CreateArray();
		json_object = cJSON_CreateObject();

		//prepare record_array.
		for(i = 1; i < nrow+1; i++)
		{
			query_rec = query_result + i*ncolumn;

			memset(PeerIP2, 0, DATAGW_IPADDR_MAX_LEN+1);

			if(strlen(query_rec[2]) > DATAGW_IPADDR_MAX_LEN)
			{
				continue;
			}

			ID		= atoi(query_rec[0]);
			iptype	= (enum Data_IPType)atoi(query_rec[1]);
			strncpy(PeerIP2, query_rec[2], strlen(query_rec[2]));

			//Put result data to query Msg for OM server.
			record_item = cJSON_CreateObject();

			cJSON_AddNumberToObject(record_item, "ID", ID);
			cJSON_AddNumberToObject(record_item, "IpType", iptype);
			cJSON_AddStringToObject(record_item, "PeerIP", PeerIP2);
			
			cJSON_AddItemToArray(record_array, record_item);
		}

		//record_array is ready.
	    cJSON_AddNumberToObject(json_object, "ret", status);
		cJSON_AddItemToObject(json_object, "records", record_array);
		cJSON_AddNumberToObject(json_object, "total", totalRec);

		*body = cJSON_Print(json_object);
		cJSON_Delete(json_object);

	    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "datagw lbo query json body: %s.\n", *body);
		stream->write_function(stream, "lbo_queryN [ok].\n");
	}
	else
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "lbo_queryN [fail] ---  err_msg : %s .\n", err_msg);
		stream->write_function(stream, "lbo_queryN [fail].\n");
		status = AGC_STATUS_GENERR;
	}

	free(sql);
	agc_db_free(err_msg);
	agc_db_free_table(query_result);

	return status;

}

void query_lbo_api(agc_stream_handle_t *stream, int argc, char **argv)
{
	char *body = NULL;
	int page_index, page_size;
    int result = 0;

	//Query N record each timer.
	if(argv == NULL || argc < 2)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "query_lbo_api : input arg invalid.\n");	
		result = AGC_STATUS_SIGAPI_INPUT_ERROR;

	    datagw_make_general_body(result, &body);
	    datagw_fire_event(stream->uuid, body);
	    free(body);

		return;
	}
	else
	{
		page_index = atoi(argv[0]);
		page_size = atoi(argv[1]);
	}

	//setp 1. query and prepare rsp body.
	lbo_queryN(stream, &body, page_index, page_size);

	//setp 2. Rsp to OM side.
    datagw_fire_event(stream->uuid, body);
    free(body);
  
	return;
}


void query_DataGwVersion(agc_stream_handle_t *stream, char **body)
{
    cJSON_Hooks json_hooks;
    cJSON *json_object;

    json_hooks.malloc_fn = malloc;
    json_hooks.free_fn = free;
    cJSON_InitHooks(&json_hooks);

    json_object = cJSON_CreateObject();
    cJSON_AddStringToObject(json_object, "ElementName", datagwElementName);
	cJSON_AddStringToObject(json_object, "ElementVersion", datagwVersion);

    *body = cJSON_Print(json_object);
    cJSON_Delete(json_object);

    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "datagw version json body: %s.\n", *body);

	return;

}


void query_datagw_version_api(agc_stream_handle_t *stream, int argc, char **argv)
{
	char *body = NULL;

	query_DataGwVersion(stream, &body);
    datagw_fire_event(stream->uuid, body);
    free(body);
  
	return;
}
