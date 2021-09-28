#include "SigApi.h"
#include "DbManager.h"

using namespace std;

//table 1
vector<stCfg_NSSAI_API>	global_nssai;

//Add one NSSAI record
/*-----------------------------------------------------------
input  :idNSSAI, SST, SD;
process:modify global parameter global_nssai and table nssai in db.
output :result of add record.
-------------------------------------------------------------*/
agc_status_t sig_api_add_nssai(agc_stream_handle_t *stream, uint8_t idNssai, uint8_t SST, uint32_t SD)
{
	stCfg_NSSAI_API tNSSAI;
	agc_status_t status = AGC_STATUS_SUCCESS;

	//NEED check idNssai is exist in table or not.  TBD
	//if exist, return back with cause code.
	
	//if not exist, then push back and add.
	tNSSAI.idNssai = idNssai;
	tNSSAI.SST = SST;
	tNSSAI.SD  = SD;
	global_nssai.push_back(tNSSAI);

	//add record to DB.
	status = nssai_insert(stream, idNssai, SST, SD);
	if(AGC_STATUS_SUCCESS != status)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG API add nssai failed .\n");
		stream->write_function(stream, "SIG API add nssai [fail].\n");
	}

	return status;
}


//Del one NSSAI record
agc_status_t sig_api_del_nssai(agc_stream_handle_t *stream, uint32_t ID)
{

	agc_status_t status = AGC_STATUS_SUCCESS;

/*
	vector<stCfg_NSSAI_API>::iterator it = global_nssai.begin();

	//search record in NSSAI list.
	for (it = global_nssai.begin(); it != global_nssai.end();)
	{
		if((*it).idNssai == idNssai)
		{
			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "SIG API nssai del record is %d,%d,%d .\n", it->idNssai, it->SST, it->SD);
	
			//delete record from nssai list.
			global_nssai.erase(it);  //pointer “it” will be move to next after erase.
			
		}

	}
	//the last one can be search or not?  NEED CHECK ------------------------------------------------------------------------------
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "SIG API del nssai, last record is %d,%d,%d .\n", (*it).idNssai, (*it).SST, (*it).SD);
*/

	//NEED check be used by bplmn table or not   . TBD ---------------------------------------------
	//IF USED, return back- “can't delete”.

    //IF NOT be used.
	//del record from DB.
	status = nssai_delete(stream, ID);
	if(AGC_STATUS_SUCCESS != status)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG API del nssai failed .\n");
		stream->write_function(stream, "SIG API del nssai [fail].\n");
	}

	return status;
}



//Update one NSSAI record
agc_status_t sig_api_update_nssai(agc_stream_handle_t *stream, uint32_t ID, uint8_t idNssai, uint8_t SST, uint32_t SD)
{

	agc_status_t status = AGC_STATUS_SUCCESS;

	vector<stCfg_NSSAI_API>::iterator it = global_nssai.begin();

	//update record in NSSAI list.
	for (it = global_nssai.begin(); it != global_nssai.end(); it++)
	{
		if((*it).idNssai == idNssai)
		{
			//update record in nssai list.
			it->SST = SST;
			it->SD  = SD;
			break;
		}

	}

	//update record from DB.
	status = nssai_update(stream, ID, idNssai, SST, SD);
	if(AGC_STATUS_SUCCESS != status)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG API update nssai failed .\n");
		stream->write_function(stream, "SIG API update nssai [fail].\n");
	}

	return status;
}





agc_status_t nssai_insert(agc_stream_handle_t *stream, uint8_t idNssai, uint8_t SST, uint32_t SD)
{

	agc_db_t *db = NULL;
	agc_db_stmt_t *stmt = NULL;
	char *err_msg = NULL;
	int32_t ret = 0;
	agc_status_t status = AGC_STATUS_GENERR;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
		stream->write_function(stream, "nssai_insert open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_prepare(db, "insert into sig_nssai_table(idNssai, SST, SD) values(?, ?, ?);", -1, &stmt, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "nssai_insert prepare failed .\n");
		stream->write_function(stream, "nssai_insert prepare [fail].\n");
		agc_db_close(db);
		return AGC_STATUS_GENERR;
	}

/*
	if (agc_db_bind_int(stmt, 1, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "nssai_insert bind ID failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "nssai_insert bind ID [fail].\n");
		return AGC_STATUS_GENERR;
	}
*/

	//NSSAI ID
	if (agc_db_bind_int(stmt, 1, idNssai) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "nssai_insert bind idNssai failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "nssai_insert bind idNssai [fail].\n");
		return AGC_STATUS_GENERR;
	}

	//NSSAI SST
	if (agc_db_bind_int(stmt, 2, SST) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "nssai_insert bind SST failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "nssai_insert bind SST [fail].\n");
		return AGC_STATUS_GENERR;
	}

	//NSSAI SD
	if (agc_db_bind_int(stmt, 3, SD) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "nssai_insert bind SD failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "nssai_insert bind SD [fail].\n");
		return AGC_STATUS_GENERR;
	}

	ret = agc_db_step(stmt);

	if (ret != AGC_DB_DONE) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "nssai insert data failed, err-code: %d.\n", ret);
		stream->write_function(stream, "nssai insert data [fail] err-code: %d.\n", ret);
	} else {
		status = AGC_STATUS_SUCCESS;
		stream->write_function(stream, "nssai insert data  [ok].\n");
	}

	agc_db_finalize(stmt);
	agc_db_close(db);

	return status;
}


//lyb 2020-6-16
agc_status_t nssai_delete(agc_stream_handle_t *stream, uint32_t ID)
{

	agc_db_t *db = NULL;
	char *err_msg = NULL;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME))) {
		stream->write_function(stream, "nssai_delete open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	char *sql = agc_mprintf("DELETE FROM sig_nssai_table WHERE ID = %d;", ID);

	if (agc_db_exec(db, sql, NULL, NULL, &err_msg) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
		stream->write_function(stream, "nssai_delete db [fail].\n");
		agc_db_free(err_msg);
		agc_db_close(db);
		agc_safe_free(sql);
		return AGC_STATUS_GENERR;
	}

	stream->write_function(stream, "nssai_delete record [ok].\n");
	agc_db_close(db);
	agc_safe_free(sql);
	return AGC_STATUS_SUCCESS;

}



//lyb 2020-6-16
agc_status_t nssai_update(agc_stream_handle_t *stream, uint32_t ID, uint8_t idNssai, uint8_t SST, uint32_t SD)
{

	agc_db_t *db = NULL;
	char *err_msg = NULL;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME))) {
		stream->write_function(stream, "nssai_update open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	char *sql = agc_mprintf("UPDATE sig_nssai_table SET idNssai = %d, SST = %d, SD = %d WHERE ID = %d;", idNssai, SST, SD, ID);

	if (agc_db_exec(db, sql, NULL, NULL, &err_msg) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
		stream->write_function(stream, "nssai_update db [fail].\n");
		agc_db_free(err_msg);
		agc_db_close(db);
		agc_safe_free(sql);
		return AGC_STATUS_GENERR;
	}

	stream->write_function(stream, "nssai_update record [ok].\n");
	agc_db_close(db);
	agc_safe_free(sql);
	return AGC_STATUS_SUCCESS;

}


//Query N record  page_index should start from 0;
agc_status_t nssai_queryN(agc_stream_handle_t *stream, char **body, int page_index, int page_size)
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
	uint8_t	  idNssai;
	uint8_t   SST;	
	uint32_t  SD;	

    cJSON_Hooks json_hooks, json_hooks2;
    cJSON *record_item, *record_array, *json_object;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
		stream->write_function(stream, "nssai_query open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	char *tsql = agc_mprintf("SELECT * FROM sig_nssai_table;");
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
	char *sql = agc_mprintf("SELECT * FROM sig_nssai_table LIMIT %d OFFSET %d;", page_size, page_index);

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

			ID = atoi(query_rec[0]);
			idNssai = atoi(query_rec[1]);
			SST = atoi(query_rec[2]);
			SD = atoi(query_rec[3]);

			//Put result data to query Msg for OM server.
			record_item = cJSON_CreateObject();
			cJSON_AddNumberToObject(record_item, "ID", ID);
			cJSON_AddNumberToObject(record_item, "idNssai", idNssai);
			cJSON_AddNumberToObject(record_item, "SST", SST);
			if(INVALIDE_NULL_int32 == SD)
			{
				cJSON_AddStringToObject(record_item, "SD", "");
			}
			else
			{
				cJSON_AddNumberToObject(record_item, "SD", SD);
			}
			
			cJSON_AddItemToArray(record_array, record_item);
		}

		//record_array is ready.
	    cJSON_AddNumberToObject(json_object, "ret", status);
		cJSON_AddItemToObject(json_object, "records", record_array);
		cJSON_AddNumberToObject(json_object, "total", totalRec);

		*body = cJSON_Print(json_object);
		cJSON_Delete(json_object);

	    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "siggw nssai query json body: %s.\n", *body);
		stream->write_function(stream, "nssai_query [ok].\n");
	}
	else
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "nssai_queryN [fail] ---  err_msg : %s .\n", err_msg);
		stream->write_function(stream, "nssai_query [fail].\n");
		status = AGC_STATUS_GENERR;
	}

	free(sql);
	agc_db_free(err_msg);
	agc_db_free_table(query_result);

	return status;

}




//table 2
agc_status_t vgw_insert(agc_stream_handle_t *stream, int argc, char **argv)
{

	agc_db_t *db = NULL;
	agc_db_stmt_t *stmt = NULL;
	char *err_msg = NULL;
	agc_status_t status = AGC_STATUS_GENERR;

	uint8_t idSigGW;
	char AMFName[SIGGW_NAME_MAX_LEN+1];
	uint8_t AMFCapacity;
	uint32_t gNBid;
	char gNBName[SIGGW_NAME_MAX_LEN+1];
	char MCC[SIGGW_MCCMNC_MAX_LEN+1];
	char MNC[SIGGW_MCCMNC_MAX_LEN+1]; 
	enum Cfg_PagingDRX DRX;
	enum Cfg_GNBType gNBType;
	enum Cfg_NG_ENBType ngeNBType;


	//step 1.1 prepare parameter from argv.
	if(argv == NULL || argc < 10)
	{
		stream->write_function(stream, "vgw_insert input arg invalid.\n");
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vgw_insert input arg invalid.\n");
		status = AGC_STATUS_SIGAPI_INPUT_ERROR;
		return status;
	}
	else
	{
		if(strlen(argv[1]) > SIGGW_NAME_MAX_LEN || strlen(argv[4]) > SIGGW_NAME_MAX_LEN || strlen(argv[5]) > SIGGW_MCCMNC_MAX_LEN || strlen(argv[6]) > SIGGW_MCCMNC_MAX_LEN)
		{
			status = AGC_STATUS_SIGAPI_INPUT_ERROR;

			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vgw_insert input name error.\n");
			stream->write_function(stream, "vgw_insert input name error.\n");
			
			return status;
		}

		memset(AMFName, 0, SIGGW_NAME_MAX_LEN+1);
		memset(gNBName, 0, SIGGW_NAME_MAX_LEN+1);
		memset(MCC, 	0, SIGGW_MCCMNC_MAX_LEN+1);
		memset(MNC, 	0, SIGGW_MCCMNC_MAX_LEN+1);
		
		idSigGW = atoi(argv[0]);
		strncpy(AMFName, argv[1], strlen(argv[1]));
		AMFCapacity = atoi(argv[2]);
		gNBid = atoi(argv[3]);
		strncpy(gNBName, argv[4], strlen(argv[4]));
		strncpy(MCC, argv[5], strlen(argv[5]));
		strncpy(MNC, argv[6], strlen(argv[6]));
		DRX = (enum Cfg_PagingDRX)atoi(argv[7]);
		gNBType = (enum Cfg_GNBType)atoi(argv[8]);
		ngeNBType = (enum Cfg_NG_ENBType)atoi(argv[9]);
		//agc_log_printf(AGC_LOG, AGC_LOG_INFO, "vgw_insert : input arg %d ,%d, %d.\n", , , );	
	}

	if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
		stream->write_function(stream, "vgw_insert open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_prepare(db, "insert into sig_vgw_table(idSigGW, AMFName, AMFCapacity, gNBid, gNBName, MCC, MNC, DRX, gNBType, ngeNBType) values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?);", -1, &stmt, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vgw_insert prepare failed .\n");
		stream->write_function(stream, "vgw_insert prepare [fail].\n");
		agc_db_close(db);
		return AGC_STATUS_GENERR;
	}

/*
	if (agc_db_bind_int(stmt, 1, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vgw_insert bind ID failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "vgw_insert bind ID [fail].\n");
		return AGC_STATUS_GENERR;
	}
*/

	if (agc_db_bind_int(stmt, 1, idSigGW) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vgw_insert bind int1 failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "vgw_insert bind int1 [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_bind_text(stmt, 2, AMFName, strlen(AMFName), AGC_CORE_DB_TRANSIENT) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vgw_insert bind txt2 failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "vgw_insert bind txt2 [fail].\n");
		return AGC_STATUS_GENERR;
	}


	if (agc_db_bind_int(stmt, 3, AMFCapacity) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vgw_insert bind int3 failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "vgw_insert bind int3 [fail].\n");
		return AGC_STATUS_GENERR;
	}


	if (agc_db_bind_int(stmt, 4, gNBid) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vgw_insert bind int4 failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "vgw_insert bind int4 [fail].\n");
		return AGC_STATUS_GENERR;
	}


	if (agc_db_bind_text(stmt, 5, gNBName, strlen(gNBName), AGC_CORE_DB_TRANSIENT) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vgw_insert bind txt5 failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "vgw_insert bind txt5 [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_bind_text(stmt, 6, MCC, strlen(MCC), AGC_CORE_DB_TRANSIENT) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vgw_insert bind MCC failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "vgw_insert bind MCC [fail].\n");
		return AGC_STATUS_GENERR;
	}


	if (agc_db_bind_text(stmt, 7, MNC, strlen(MNC), AGC_CORE_DB_TRANSIENT) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vgw_insert bind MNC failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "vgw_insert bind MNC [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_bind_int(stmt, 8, DRX) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vgw_insert bind int8 failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "vgw_insert bind int8 [fail].\n");
		return AGC_STATUS_GENERR;
	}


	if (agc_db_bind_int(stmt, 9, gNBType) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vgw_insert bind int9 failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "vgw_insert bind int9 [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_bind_int(stmt, 10, ngeNBType) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vgw_insert bind int10 failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "vgw_insert bind int10 [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_step(stmt) != AGC_DB_DONE) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vgw_insert data failed .\n");
		stream->write_function(stream, "vgw_insert data [fail].\n");
	} else {

		status = AGC_STATUS_SUCCESS;
		stream->write_function(stream, "vgw_insert record [ok].\n");
	}

	agc_db_finalize(stmt);
	agc_db_close(db);

	return status;
}



agc_status_t vgw_update(agc_stream_handle_t *stream, int argc, char **argv)
{

	agc_db_t *db = NULL;
	char *err_msg = NULL;
	agc_status_t status = AGC_STATUS_SUCCESS;
		
	uint32_t ID;
	uint8_t idSigGW;
	char AMFName[SIGGW_NAME_MAX_LEN+1];
	uint8_t AMFCapacity;
	uint32_t gNBid;
	char gNBName[SIGGW_NAME_MAX_LEN+1];
	char MCC[SIGGW_MCCMNC_MAX_LEN+1];
	char MNC[SIGGW_MCCMNC_MAX_LEN+1]; 
	enum Cfg_PagingDRX DRX;
	enum Cfg_GNBType gNBType;
	enum Cfg_NG_ENBType ngeNBType;

	//step 1 prepare parameter from argv.
	if(argv == NULL || argc < 11)
	{
		stream->write_function(stream, "vgw_update input arg invalid.\n");
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vgw_update input arg invalid.\n");
		status = AGC_STATUS_SIGAPI_INPUT_ERROR;
		return status;
	}
	else
	{
		if(strlen(argv[2]) > SIGGW_NAME_MAX_LEN || strlen(argv[5]) > SIGGW_NAME_MAX_LEN || strlen(argv[6]) > SIGGW_MCCMNC_MAX_LEN || strlen(argv[7]) > SIGGW_MCCMNC_MAX_LEN)
		{
			status = AGC_STATUS_SIGAPI_INPUT_ERROR;

			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vgw_update input name error.\n");
			stream->write_function(stream, "vgw_update input name error.\n");
			
			return status;
		}

		memset(AMFName, 0, SIGGW_NAME_MAX_LEN+1);
		memset(gNBName, 0, SIGGW_NAME_MAX_LEN+1);
		memset(MCC, 	0, SIGGW_MCCMNC_MAX_LEN+1);
		memset(MNC, 	0, SIGGW_MCCMNC_MAX_LEN+1);

		ID = atoi(argv[0]);	
		idSigGW = atoi(argv[1]);
		strncpy(AMFName, argv[2], strlen(argv[2]));
		AMFCapacity = atoi(argv[3]);
		gNBid = atoi(argv[4]);
		strncpy(gNBName, argv[5], strlen(argv[5]));
		strncpy(MCC, argv[6], strlen(argv[6]));
		strncpy(MNC, argv[7], strlen(argv[7]));
		DRX = (enum Cfg_PagingDRX)atoi(argv[8]);
		gNBType = (enum Cfg_GNBType)atoi(argv[9]);
		ngeNBType = (enum Cfg_NG_ENBType)atoi(argv[10]);
		//agc_log_printf(AGC_LOG, AGC_LOG_INFO, "vgw_update : input arg %d ,%d, %d.\n", , , );	
	}


	//step 2 insert into db.
	if (!(db = agc_db_open_file(SIG_DB_FILENAME))) {
		stream->write_function(stream, "vgw_update open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	char *sql = agc_mprintf("UPDATE sig_vgw_table SET idSigGW = %d, AMFName = \"%s\", AMFCapacity = %d, gNBid = %d, gNBName = \"%s\", MCC = \"%s\", MNC = \"%s\", DRX = %d,  gNBType = %d, ngeNBType = %d WHERE ID = %d;", idSigGW, AMFName, AMFCapacity, gNBid, gNBName, MCC, MNC, DRX, gNBType, ngeNBType, ID);

	if (agc_db_exec(db, sql, NULL, NULL, &err_msg) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
		stream->write_function(stream, "vgw_update record [fail].\n");
		agc_db_free(err_msg);
		agc_db_close(db);
		agc_safe_free(sql);
		return AGC_STATUS_GENERR;
	}

	stream->write_function(stream, "vgw_update record [ok].\n");
	agc_db_close(db);
	agc_safe_free(sql);
	return AGC_STATUS_SUCCESS;

}



agc_status_t vgw_delete(agc_stream_handle_t *stream, int argc, char **argv)
{

	agc_db_t *db = NULL;
	char *err_msg = NULL;

    uint32_t ID;

	//step 1.1 prepare parameter from argv.
	if(argv == NULL)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vgw_delete : input arg invalid.\n");	
		return AGC_STATUS_SIGAPI_INPUT_ERROR;
	}
	else
	{
		ID = atoi(argv[0]);
		//agc_log_printf(AGC_LOG, AGC_LOG_INFO, "vgw_delete : input idSigGW: %d.\n", idSigGW);
	}

	if (!(db = agc_db_open_file(SIG_DB_FILENAME))) {
		stream->write_function(stream, "vgw_delete open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	char *sql = agc_mprintf("DELETE FROM sig_vgw_table WHERE ID = %d;", ID);

	if (agc_db_exec(db, sql, NULL, NULL, &err_msg) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
		stream->write_function(stream, "vgw_delete db [fail].\n");
		agc_db_free(err_msg);
		agc_db_close(db);
		agc_safe_free(sql);
		return AGC_STATUS_GENERR;
	}

	stream->write_function(stream, "vgw_delete record [ok].\n");
	agc_db_close(db);
	agc_safe_free(sql);
	return AGC_STATUS_SUCCESS;

}



//Query N record 
agc_status_t vgw_queryN(agc_stream_handle_t *stream, char **body, int page_index, int page_size)
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
	uint8_t idSigGW;
	char AMFName[SIGGW_NAME_MAX_LEN+1];
	uint8_t AMFCapacity;
	uint32_t gNBid;
	char gNBName[SIGGW_NAME_MAX_LEN+1];
	char MCC[SIGGW_MCCMNC_MAX_LEN+1];
	char MNC[SIGGW_MCCMNC_MAX_LEN+1]; 
	enum Cfg_PagingDRX DRX;
	enum Cfg_GNBType gNBType;
	enum Cfg_NG_ENBType ngeNBType;

    cJSON_Hooks json_hooks, json_hooks2;
    cJSON *record_item, *record_array, *json_object;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
		stream->write_function(stream, "vgw_queryN open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	char *tsql = agc_mprintf("SELECT * FROM sig_vgw_table;");
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
	char *sql = agc_mprintf("SELECT * FROM sig_vgw_table LIMIT %d OFFSET %d;", page_size, page_index);

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

			memset(AMFName, 0, SIGGW_NAME_MAX_LEN+1);
			memset(gNBName, 0, SIGGW_NAME_MAX_LEN+1);
			memset(MCC, 	0, SIGGW_MCCMNC_MAX_LEN+1);
			memset(MNC, 	0, SIGGW_MCCMNC_MAX_LEN+1);

			if(strlen(query_rec[2]) > SIGGW_NAME_MAX_LEN || strlen(query_rec[5]) > SIGGW_NAME_MAX_LEN || strlen(query_rec[6]) > SIGGW_MCCMNC_MAX_LEN || strlen(query_rec[7]) > SIGGW_MCCMNC_MAX_LEN)
			{
				continue;
			}
			

			ID = atoi(query_rec[0]);
			idSigGW = atoi(query_rec[1]);
			strncpy(AMFName, query_rec[2], strlen(query_rec[2]));
			AMFCapacity = atoi(query_rec[3]);
			gNBid = atoi(query_rec[4]);
			strncpy(gNBName, query_rec[5], strlen(query_rec[5]));
			strncpy(MCC , query_rec[6], strlen(query_rec[6]));
			strncpy(MNC, query_rec[7], strlen(query_rec[7]));
			DRX = (enum Cfg_PagingDRX)atoi(query_rec[8]);
			gNBType = (enum Cfg_GNBType)atoi(query_rec[9]);
			ngeNBType = (enum Cfg_NG_ENBType)atoi(query_rec[10]);

			//Put result data to query Msg for OM server.
			record_item = cJSON_CreateObject();

			cJSON_AddNumberToObject(record_item, "ID", ID);
			cJSON_AddNumberToObject(record_item, "idSigGW", idSigGW);
			cJSON_AddStringToObject(record_item, "AMFName", AMFName);
			cJSON_AddNumberToObject(record_item, "AMFCapacity", AMFCapacity);
			cJSON_AddNumberToObject(record_item, "gNBid", gNBid);
			cJSON_AddStringToObject(record_item, "gNBName", gNBName);

			cJSON_AddStringToObject(record_item, "MCC", MCC);
			cJSON_AddStringToObject(record_item, "MNC", MNC);
			cJSON_AddNumberToObject(record_item, "DRX", DRX);
			cJSON_AddNumberToObject(record_item, "gNBType", gNBType);
			cJSON_AddNumberToObject(record_item, "ngeNBType", ngeNBType);
			
			cJSON_AddItemToArray(record_array, record_item);
		}

		//record_array is ready.
	    cJSON_AddNumberToObject(json_object, "ret", status);
		cJSON_AddItemToObject(json_object, "records", record_array);
		cJSON_AddNumberToObject(json_object, "total", totalRec);

		*body = cJSON_Print(json_object);
		cJSON_Delete(json_object);

	    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "siggw vgw query json body: %s.\n", *body);
		stream->write_function(stream, "vgw_queryN [ok].\n");
	}
	else
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "vgw_queryN [fail] ---  err_msg : %s .\n", err_msg);
		stream->write_function(stream, "vgw_queryN [fail].\n");
		status = AGC_STATUS_GENERR;
	}

	free(sql);
	agc_db_free(err_msg);
	agc_db_free_table(query_result);

	return status;

}



//table 3
agc_status_t taplmn_insert(agc_stream_handle_t *stream, int argc, char **argv)
{

	agc_db_t *db = NULL;
	agc_db_stmt_t *stmt = NULL;
	agc_status_t status = AGC_STATUS_GENERR;

	uint8_t idSigGW;
	uint8_t idTA;
	uint32_t TAC;
	uint8_t idBplmn;

	//step 1 prepare parameter from argv.
	if(argv == NULL || argc < 4)
	{
		stream->write_function(stream, "taplmn_insert input arg invalid.\n");
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "taplmn_insert input arg invalid.\n");
		status = AGC_STATUS_SIGAPI_INPUT_ERROR;
		return status;
	}
	else
	{
		idSigGW = atoi(argv[0]);
		idTA 	= atoi(argv[1]);
		TAC 	= atoi(argv[2]);
		idBplmn = atoi(argv[3]);
		//agc_log_printf(AGC_LOG, AGC_LOG_INFO, "taplmn_insert : input arg %d ,%d, %d.\n", , , );	
	}

	if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
		stream->write_function(stream, "taplmn_insert open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_prepare(db, "insert into sig_taplmn_table(idSigGW, idTA, TAC, idBplmn) values(?, ?, ?, ?);", -1, &stmt, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "taplmn_insert prepare failed .\n");
		stream->write_function(stream, "taplmn_insert prepare [fail].\n");
		agc_db_close(db);
		return AGC_STATUS_GENERR;
	}

/*
	if (agc_db_bind_int(stmt, 1, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "taplmn_insert bind ID failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "taplmn_insert bind ID [fail].\n");
		return AGC_STATUS_GENERR;
	}
*/

	if (agc_db_bind_int(stmt, 1, idSigGW) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "taplmn_insert bind idSigGW failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "taplmn_insert bind idSigGW [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_bind_int(stmt, 2, idTA) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "taplmn_insert bind idTA failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "taplmn_insert bind idTA [fail].\n");
		return AGC_STATUS_GENERR;
	}


	if (agc_db_bind_int(stmt, 3, TAC) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "taplmn_insert bind TAC failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "taplmn_insert bind TAC [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_bind_int(stmt, 4, idBplmn) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "taplmn_insert bind idBplmn failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "taplmn_insert bind idBplmn [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_step(stmt) != AGC_DB_DONE) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "taplmn_insert data failed .\n");
		stream->write_function(stream, "taplmn_insert data [fail].\n");
	} else {

		status = AGC_STATUS_SUCCESS;
		stream->write_function(stream, "taplmn_insert record [ok].\n");
	}

	agc_db_finalize(stmt);
	agc_db_close(db);

	return status;
}



agc_status_t taplmn_update(agc_stream_handle_t *stream, int argc, char **argv)
{

	agc_db_t *db = NULL;
	char *err_msg = NULL;
	agc_status_t status = AGC_STATUS_SUCCESS;
		
	uint32_t ID;
	uint8_t idSigGW;
	uint8_t idTA;
	uint32_t TAC;
	uint8_t idBplmn;

	//step 1 prepare parameter from argv.
	if(argv == NULL || argc < 5)
	{
		stream->write_function(stream, "taplmn_update input arg invalid.\n");
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "taplmn_update input arg invalid.\n");
		status = AGC_STATUS_SIGAPI_INPUT_ERROR;
		return status;
	}
	else
	{
		ID 		= atoi(argv[0]);
		idSigGW = atoi(argv[1]);
		idTA 	= atoi(argv[2]);
		TAC 	= atoi(argv[3]);
		idBplmn = atoi(argv[4]);
		//agc_log_printf(AGC_LOG, AGC_LOG_INFO, "taplmn_update : input arg %d ,%d, %d.\n", , , );	
	}

	//step 2 insert into db.
	if (!(db = agc_db_open_file(SIG_DB_FILENAME))) {
		stream->write_function(stream, "taplmn_update open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	char *sql = agc_mprintf("UPDATE sig_taplmn_table SET idSigGW = %d ,idTA = %d, TAC = %d, idBplmn = %d WHERE ID = %d;",idSigGW, idTA, TAC, idBplmn, ID);

	if (agc_db_exec(db, sql, NULL, NULL, &err_msg) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
		stream->write_function(stream, "taplmn_update db [fail].\n");
		agc_db_free(err_msg);
		agc_db_close(db);
		agc_safe_free(sql);
		return AGC_STATUS_GENERR;
	}

	stream->write_function(stream, "taplmn_update record [ok].\n");
	agc_db_close(db);
	agc_safe_free(sql);
	return status;

}




agc_status_t taplmn_delete(agc_stream_handle_t *stream, int argc, char **argv)
{

	agc_db_t *db = NULL;
	char *err_msg = NULL;

	uint32_t ID;
	
	 //step 1 prepare parameter from argv.
	 if(argv == NULL || argc < 1)
	 {
		 agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "taplmn_delete : input arg invalid.\n");	 
		 return AGC_STATUS_SIGAPI_INPUT_ERROR;
	 }
	 else
	 {
		 ID = atoi(argv[0]);
		 //agc_log_printf(AGC_LOG, AGC_LOG_INFO, "taplmn_delete : input idSigGW: %d.\n", idSigGW);
	 }

	if (!(db = agc_db_open_file(SIG_DB_FILENAME))) {
		stream->write_function(stream, "taplmn_delete open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	char *sql = agc_mprintf("DELETE FROM sig_taplmn_table WHERE ID = %d;", ID);

	if (agc_db_exec(db, sql, NULL, NULL, &err_msg) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
		stream->write_function(stream, "taplmn_delete db [fail].\n");
		agc_db_free(err_msg);
		agc_db_close(db);
		agc_safe_free(sql);
		return AGC_STATUS_GENERR;
	}

	stream->write_function(stream, "taplmn_delete record [ok].\n");
	agc_db_close(db);
	agc_safe_free(sql);
	return AGC_STATUS_SUCCESS;

}



//Query N record 
agc_status_t taplmn_queryN(agc_stream_handle_t *stream, char **body, int page_index, int page_size)
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
	uint8_t idSigGW;
	uint8_t idTA;
	uint32_t TAC;
	uint8_t idBplmn;

    cJSON_Hooks json_hooks, json_hooks2;
    cJSON *record_item, *record_array, *json_object;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
		stream->write_function(stream, "taplmn_queryN open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	char *tsql = agc_mprintf("SELECT * FROM sig_taplmn_table;");
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
	char *sql = agc_mprintf("SELECT * FROM sig_taplmn_table LIMIT %d OFFSET %d;", page_size, page_index);

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

			ID		= atoi(query_rec[0]);
			idSigGW = atoi(query_rec[1]);
			idTA 	= atoi(query_rec[2]);
			TAC 	= atoi(query_rec[3]);
			idBplmn = atoi(query_rec[4]);

			//Put result data to query Msg for OM server.
			record_item = cJSON_CreateObject();

			cJSON_AddNumberToObject(record_item, "ID", ID);
			cJSON_AddNumberToObject(record_item, "idSigGW", idSigGW);
			cJSON_AddNumberToObject(record_item, "idTA", idTA);
			cJSON_AddNumberToObject(record_item, "TAC", TAC);
			cJSON_AddNumberToObject(record_item, "idBplmn", idBplmn);
			
			cJSON_AddItemToArray(record_array, record_item);
		}

		//record_array is ready.
	    cJSON_AddNumberToObject(json_object, "ret", status);
		cJSON_AddItemToObject(json_object, "records", record_array);
		cJSON_AddNumberToObject(json_object, "total", totalRec);

		*body = cJSON_Print(json_object);
		cJSON_Delete(json_object);

	    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "siggw taplmn query json body: %s.\n", *body);
		stream->write_function(stream, "taplmn_queryN [ok].\n");
	}
	else
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "taplmn_queryN [fail] ---  err_msg : %s .\n", err_msg);
		stream->write_function(stream, "taplmn_queryN [fail].\n");
		status = AGC_STATUS_GENERR;
	}

	free(sql);
	agc_db_free(err_msg);
	agc_db_free_table(query_result);

	return status;

}




//table 4
agc_status_t guami_insert(agc_stream_handle_t *stream, int argc, char **argv)
{

	agc_db_t *db = NULL;
	agc_db_stmt_t *stmt = NULL;
	char *err_msg = NULL;
	agc_status_t status = AGC_STATUS_GENERR;

	uint8_t idSigGW;
	uint8_t idAMF;
	char MCC[SIGGW_MCCMNC_MAX_LEN+1];
	char MNC[SIGGW_MCCMNC_MAX_LEN+1]; 
	uint8_t AMFRegionID;
	uint16_t AMFSetId;
	uint8_t AMFPointer;

	//step 1 prepare parameter from argv.
	if(argv == NULL || argc < 7)
	{
		stream->write_function(stream, "guami_insert input arg invalid.\n");
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "guami_insert input arg invalid.\n");
		status = AGC_STATUS_SIGAPI_INPUT_ERROR;
		return status;
	}
	else
	{
		if(strlen(argv[2]) > SIGGW_MCCMNC_MAX_LEN || strlen(argv[3]) > SIGGW_MCCMNC_MAX_LEN)
		{
			status = AGC_STATUS_SIGAPI_INPUT_ERROR;

			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "guami_insert input plmn error.\n");
			stream->write_function(stream, "guami_insert input plmn error.\n");
			
			return status;
		}

		memset(MCC, 0, SIGGW_MCCMNC_MAX_LEN+1);
		memset(MNC, 0, SIGGW_MCCMNC_MAX_LEN+1);
	
		idSigGW = atoi(argv[0]);
		idAMF 	= atoi(argv[1]);
		strncpy(MCC, argv[2], strlen(argv[2]));
		strncpy(MNC, argv[3], strlen(argv[3]));
		AMFRegionID = atoi(argv[4]);
		AMFSetId 	= atoi(argv[5]);
		AMFPointer	= atoi(argv[6]);
		//agc_log_printf(AGC_LOG, AGC_LOG_INFO, "guami_insert : input arg %d ,%d, %d.\n", , , );	
	}

	//step 2 insert to db
	if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
		stream->write_function(stream, "guami_insert open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_prepare(db, "insert into sig_guami_table(idSigGW, idAMF, MCC, MNC, AMFRegionID, AMFSetId, AMFPointer) values(?, ?, ?, ?, ?, ?, ?);", -1, &stmt, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "guami_insert prepare failed .\n");
		stream->write_function(stream, "guami_insert prepare [fail].\n");
		agc_db_close(db);
		return AGC_STATUS_GENERR;
	}
/*	
	if (agc_db_bind_int(stmt, 1, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "guami_insert bind ID failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "guami_insert bind ID [fail].\n");
		return AGC_STATUS_GENERR;
	}
*/

	if (agc_db_bind_int(stmt, 1, idSigGW) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "guami_insert bind idSigGW failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "guami_insert bind idSigGW [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_bind_int(stmt, 2, idAMF) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "guami_insert bind idAMF failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "guami_insert bind idAMF [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_bind_text(stmt, 3, MCC, strlen(MCC), AGC_CORE_DB_TRANSIENT) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "guami_insert bind MCC failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "guami_insert bind MCC [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_bind_text(stmt, 4, MNC, strlen(MNC), AGC_CORE_DB_TRANSIENT) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "guami_insert bind MNC failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "guami_insert bind MNC [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_bind_int(stmt, 5, AMFRegionID) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "guami_insert bind AMFRegionID failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "guami_insert bind AMFRegionID [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_bind_int(stmt, 6, AMFSetId) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "guami_insert bind AMFSetId failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "guami_insert bind AMFSetId [fail].\n");
		return AGC_STATUS_GENERR;
	}


	if (agc_db_bind_int(stmt, 7, AMFPointer) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "guami_insert bind AMFPointer failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "guami_insert bind AMFPointer [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_step(stmt) != AGC_DB_DONE) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "guami_insert record failed .\n");
		stream->write_function(stream, "guami_insert record [fail].\n");
	} else {
		status = AGC_STATUS_SUCCESS;
		stream->write_function(stream, "guami_insert record [ok].\n");
	}

	agc_db_finalize(stmt);
	agc_db_close(db);

	return status;
}


agc_status_t guami_update(agc_stream_handle_t *stream, int argc, char **argv)
{

	agc_db_t *db = NULL;
	char *err_msg = NULL;
	agc_status_t status = AGC_STATUS_SUCCESS;	

	uint32_t ID;
	uint8_t idSigGW;
	uint8_t idAMF;
	char MCC[SIGGW_MCCMNC_MAX_LEN+1];
	char MNC[SIGGW_MCCMNC_MAX_LEN+1]; 
	uint8_t AMFRegionID;
	uint16_t AMFSetId;
	uint8_t AMFPointer;

	//step 1 prepare parameter from argv.
	if(argv == NULL || argc < 8)
	{
		stream->write_function(stream, "guami_update input arg invalid.\n");
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "guami_update input arg invalid.\n");
		status = AGC_STATUS_SIGAPI_INPUT_ERROR;
		return status;
	}
	else
	{
		if(strlen(argv[3]) > SIGGW_MCCMNC_MAX_LEN || strlen(argv[4]) > SIGGW_MCCMNC_MAX_LEN)
		{
			status = AGC_STATUS_SIGAPI_INPUT_ERROR;

			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "guami_update input plmn error.\n");
			stream->write_function(stream, "guami_update input plmn error.\n");
			
			return status;
		}

		memset(MCC, 0, SIGGW_MCCMNC_MAX_LEN+1);
		memset(MNC, 0, SIGGW_MCCMNC_MAX_LEN+1);	
		
		ID 		= atoi(argv[0]);
		idSigGW = atoi(argv[1]);
		idAMF 	= atoi(argv[2]);
		strncpy(MCC, argv[3], strlen(argv[3]));
		strncpy(MNC, argv[4], strlen(argv[4]));
		AMFRegionID = atoi(argv[5]);
		AMFSetId 	= atoi(argv[6]);
		AMFPointer	= atoi(argv[7]);
		//agc_log_printf(AGC_LOG, AGC_LOG_INFO, "guami_update : input arg %d ,%d, %d.\n", , , );	
	}

	//step 2 insert to db
	if (!(db = agc_db_open_file(SIG_DB_FILENAME))) {
		stream->write_function(stream, "guami_update open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	char *sql = agc_mprintf("UPDATE sig_guami_table SET idSigGW = %d, idAMF = %d, MCC = \"%s\", MNC = \"%s\", AMFRegionID = %d, AMFSetId = %d, AMFPointer = %d WHERE ID = %d;", idSigGW, idAMF, MCC, MNC,AMFRegionID, AMFSetId, AMFPointer, ID);

	if (agc_db_exec(db, sql, NULL, NULL, &err_msg) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
		stream->write_function(stream, "guami_update db [fail].\n");
		agc_db_free(err_msg);
		agc_db_close(db);
		agc_safe_free(sql);
		return AGC_STATUS_GENERR;
	}

	stream->write_function(stream, "guami_update record [ok].\n");
	agc_db_close(db);
	agc_safe_free(sql);
	return AGC_STATUS_SUCCESS;

}


agc_status_t guami_delete(agc_stream_handle_t *stream, int argc, char **argv)
{

	agc_db_t *db = NULL;
	char *err_msg = NULL;

	uint32_t ID;
	
	 //step 1 prepare parameter from argv.
	 if(argv == NULL || argc < 1)
	 {
		 agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "guami_delete : input arg invalid.\n");	 
		 return AGC_STATUS_SIGAPI_INPUT_ERROR;
	 }
	 else
	 {
		 ID = atoi(argv[0]);
		 //agc_log_printf(AGC_LOG, AGC_LOG_INFO, "taplmn_delete : input idSigGW: %d.\n", idSigGW);
	 }

	//step 2 write to db
	if (!(db = agc_db_open_file(SIG_DB_FILENAME))) {
		stream->write_function(stream, "guami_delete open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	char *sql = agc_mprintf("DELETE FROM sig_guami_table WHERE ID  = %d;", ID);

	if (agc_db_exec(db, sql, NULL, NULL, &err_msg) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
		stream->write_function(stream, "guami_delete db [fail].\n");
		agc_db_free(err_msg);
		agc_db_close(db);
		agc_safe_free(sql);
		return AGC_STATUS_GENERR;
	}

	stream->write_function(stream, "guami_delete record [ok].\n");
	agc_db_close(db);
	agc_safe_free(sql);
	return AGC_STATUS_SUCCESS;

}




//Query N record 
agc_status_t guami_queryN(agc_stream_handle_t *stream, char **body, int page_index, int page_size)
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
	uint8_t idSigGW;
	uint8_t idAMF;
	char MCC[SIGGW_MCCMNC_MAX_LEN+1];
	char MNC[SIGGW_MCCMNC_MAX_LEN+1];
	uint8_t AMFRegionID;
	uint16_t AMFSetId;
	uint8_t AMFPointer;

    cJSON_Hooks json_hooks, json_hooks2;
    cJSON *record_item, *record_array, *json_object;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
		stream->write_function(stream, "guami_queryN open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	char *tsql = agc_mprintf("SELECT * FROM sig_guami_table;");
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
	char *sql = agc_mprintf("SELECT * FROM sig_guami_table LIMIT %d OFFSET %d;", page_size, page_index);

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

			memset(MCC, 0, SIGGW_MCCMNC_MAX_LEN+1);
			memset(MNC, 0, SIGGW_MCCMNC_MAX_LEN+1);

			if(strlen(query_rec[3]) > SIGGW_MCCMNC_MAX_LEN || strlen(query_rec[4]) > SIGGW_MCCMNC_MAX_LEN)
			{
				continue;
			}

			ID 		= atoi(query_rec[0]);
			idSigGW = atoi(query_rec[1]);
			idAMF 	= atoi(query_rec[2]);
			strncpy(MCC, query_rec[3], strlen(query_rec[3]));
			strncpy(MNC, query_rec[4], strlen(query_rec[4]));
			AMFRegionID	= atoi(query_rec[5]);
			AMFSetId 	= atoi(query_rec[6]);
			AMFPointer  = atoi(query_rec[7]);

			//Put result data to query Msg for OM server.
			record_item = cJSON_CreateObject();

			cJSON_AddNumberToObject(record_item, "ID", ID);
			cJSON_AddNumberToObject(record_item, "idSigGW", idSigGW);
			cJSON_AddNumberToObject(record_item, "idAMF", idAMF);
			cJSON_AddStringToObject(record_item, "MCC", MCC);
			cJSON_AddStringToObject(record_item, "MNC", MNC);
			cJSON_AddNumberToObject(record_item, "AMFRegionID", AMFRegionID);
			cJSON_AddNumberToObject(record_item, "AMFSetId", AMFSetId);
			cJSON_AddNumberToObject(record_item, "AMFPointer", AMFPointer);
			
			cJSON_AddItemToArray(record_array, record_item);
		}

		//record_array is ready.
	    cJSON_AddNumberToObject(json_object, "ret", status);
		cJSON_AddItemToObject(json_object, "records", record_array);
		cJSON_AddNumberToObject(json_object, "total", totalRec);

		*body = cJSON_Print(json_object);
		cJSON_Delete(json_object);

	    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "siggw guami query json body: %s.\n", *body);
		stream->write_function(stream, "guami_queryN [ok].\n");
	}
	else
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "guami_queryN [fail] ---  err_msg : %s .\n", err_msg);
		stream->write_function(stream, "guami_queryN [fail].\n");
		status = AGC_STATUS_GENERR;
	}

	free(sql);
	agc_db_free(err_msg);
	agc_db_free_table(query_result);

	return status;

}




//table 5 bplmn
agc_status_t bplmn_insert(agc_stream_handle_t *stream, int argc, char **argv)
{

	agc_db_t *db = NULL;
	agc_db_stmt_t *stmt = NULL;
	char *err_msg = NULL;
	agc_status_t status = AGC_STATUS_GENERR;

	uint8_t idSigGW;
	uint8_t idBplmn;
	char MCC[SIGGW_MCCMNC_MAX_LEN+1];
	char MNC[SIGGW_MCCMNC_MAX_LEN+1]; 
	uint8_t idNssai;

	//step 1 prepare parameter from argv.
	if(argv == NULL || argc < 5)
	{
		stream->write_function(stream, "bplmn_insert input arg invalid.\n");
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "bplmn_insert input arg invalid.\n");
		status = AGC_STATUS_SIGAPI_INPUT_ERROR;
		return status;
	}
	else
	{
		if(strlen(argv[2]) > SIGGW_MCCMNC_MAX_LEN || strlen(argv[3]) > SIGGW_MCCMNC_MAX_LEN)
		{
			status = AGC_STATUS_SIGAPI_INPUT_ERROR;

			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "bplmn_insert input plmn error.\n");
			stream->write_function(stream, "bplmn_insert input plmn error.\n");
			
			return status;
		}

		memset(MCC, 0, SIGGW_MCCMNC_MAX_LEN+1);
		memset(MNC, 0, SIGGW_MCCMNC_MAX_LEN+1);
	
		idSigGW = atoi(argv[0]);
		idBplmn	= atoi(argv[1]);
		strncpy(MCC, argv[2], strlen(argv[2]));
		strncpy(MNC, argv[3], strlen(argv[3]));
		idNssai = atoi(argv[4]);
		//agc_log_printf(AGC_LOG, AGC_LOG_INFO, "bplmn_insert : input arg %d ,%d, %d.\n", , , );	
	}

	//step 2 insert to db
	if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
		stream->write_function(stream, "bplmn_insert open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_prepare(db, "insert into sig_bplmn_table(idSigGW, idBplmn, MCC, MNC, idNssai) values(?, ?, ?, ?, ?);", -1, &stmt, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "bplmn_insert prepare failed .\n");
		stream->write_function(stream, "bplmn_insert prepare [fail].\n");
		agc_db_close(db);
		return AGC_STATUS_GENERR;
	}
/*
	if (agc_db_bind_int(stmt, 1, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "guami_insert bind ID failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "guami_insert bind ID [fail].\n");
		return AGC_STATUS_GENERR;
	}
*/
	if (agc_db_bind_int(stmt, 1, idSigGW) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "bplmn_insert bind idSigGW failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "bplmn_insert bind idSigGW [fail].\n");
		return AGC_STATUS_GENERR;
	}


	if (agc_db_bind_int(stmt, 2, idBplmn) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "bplmn_insert bind idBplmn failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "bplmn_insert bind idBplmn [fail].\n");
		return AGC_STATUS_GENERR;
	}


	if (agc_db_bind_text(stmt, 3, MCC, strlen(MCC), AGC_CORE_DB_TRANSIENT) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "bplmn_insert bind MCC failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "bplmn_insert bind MCC [fail].\n");
		return AGC_STATUS_GENERR;
	}


	if (agc_db_bind_text(stmt, 4, MNC, strlen(MNC), AGC_CORE_DB_TRANSIENT) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "bplmn_insert bind MNC failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "bplmn_insert bind MNC [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_bind_int(stmt, 5, idNssai) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "bplmn_insert bind idNssai failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "bplmn_insert bind idNssai [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_step(stmt) != AGC_DB_DONE) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "bplmn_insert record failed .\n");
		stream->write_function(stream, "bplmn_insert record [fail].\n");
	} else {
		status = AGC_STATUS_SUCCESS;
		stream->write_function(stream, "bplmn_insert record [ok].\n");
	}

	agc_db_finalize(stmt);
	agc_db_close(db);

	return status;
}



//lyb 2020-6-16
agc_status_t bplmn_update(agc_stream_handle_t *stream, int argc, char **argv)
{

	agc_db_t *db = NULL;
	char *err_msg = NULL;
	agc_status_t status = AGC_STATUS_SUCCESS;	

	uint32_t ID;
	uint8_t idSigGW;
	uint8_t idBplmn;
	char MCC[SIGGW_MCCMNC_MAX_LEN+1];
	char MNC[SIGGW_MCCMNC_MAX_LEN+1]; 
	uint8_t idNssai;

	//step 1 prepare parameter from argv.
	if(argv == NULL || argc < 6)
	{
		stream->write_function(stream, "bplmn_update input arg invalid.\n");
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "bplmn_update input arg invalid.\n");
		status = AGC_STATUS_SIGAPI_INPUT_ERROR;
		return status;
	}
	else
	{

		if(strlen(argv[3]) > SIGGW_MCCMNC_MAX_LEN || strlen(argv[4]) > SIGGW_MCCMNC_MAX_LEN)
		{
			status = AGC_STATUS_SIGAPI_INPUT_ERROR;

			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "bplmn_update input plmn error.\n");
			stream->write_function(stream, "bplmn_update input plmn error.\n");
			
			return status;
		}

		memset(MCC, 0, SIGGW_MCCMNC_MAX_LEN+1);
		memset(MNC, 0, SIGGW_MCCMNC_MAX_LEN+1);	
		
		ID 		= atoi(argv[0]);
		idSigGW = atoi(argv[1]);
		idBplmn = atoi(argv[2]);	
		strncpy(MCC, argv[3], strlen(argv[3]));
		strncpy(MNC, argv[4], strlen(argv[4]));
		idNssai = atoi(argv[5]);
		//agc_log_printf(AGC_LOG, AGC_LOG_INFO, "bplmn_update : input arg %d ,%d, %d.\n", , , );	
	}

	//step 2 insert to db
	if (!(db = agc_db_open_file(SIG_DB_FILENAME))) {
		stream->write_function(stream, "bplmn_update open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	char *sql = agc_mprintf("UPDATE sig_bplmn_table SET idSigGW = %d, idBplmn = %d, MCC = \"%s\", MNC = \"%s\", idNssai = %d WHERE ID = %d;",idSigGW, idBplmn, MCC, MNC,idNssai, ID);

	if (agc_db_exec(db, sql, NULL, NULL, &err_msg) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
		stream->write_function(stream, "bplmn_update db [fail].\n");
		agc_db_free(err_msg);
		agc_db_close(db);
		agc_safe_free(sql);
		return AGC_STATUS_GENERR;
	}

	stream->write_function(stream, "bplmn_update db [ok].\n");
	agc_db_close(db);
	agc_safe_free(sql);
	return status;

}



//lyb 2020-6-16
agc_status_t bplmn_delete(agc_stream_handle_t *stream, int argc, char **argv)
{

	agc_db_t *db = NULL;
	char *err_msg = NULL;

	uint32_t ID;
	
	 //step 1 prepare parameter from argv.
	 if(argv == NULL || argc < 1)
	 {
		 agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "taplmn_delete : input arg invalid.\n");	 
		 return AGC_STATUS_SIGAPI_INPUT_ERROR;
	 }
	 else
	 {
		 ID = atoi(argv[0]);
		 //agc_log_printf(AGC_LOG, AGC_LOG_INFO, "taplmn_delete : input idSigGW: %d.\n", idSigGW);
	 }

	if (!(db = agc_db_open_file(SIG_DB_FILENAME))) {
		stream->write_function(stream, "bplmn_delete open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	char *sql = agc_mprintf("DELETE FROM sig_bplmn_table WHERE ID = %d;", ID);

	if (agc_db_exec(db, sql, NULL, NULL, &err_msg) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
		stream->write_function(stream, "bplmn_delete record [fail].\n");
		agc_db_free(err_msg);
		agc_db_close(db);
		agc_safe_free(sql);
		return AGC_STATUS_GENERR;
	}

	stream->write_function(stream, "bplmn_delete record [ok].\n");
	agc_db_close(db);
	agc_safe_free(sql);
	return AGC_STATUS_SUCCESS;

}





//Query N record 
agc_status_t bplmn_queryN(agc_stream_handle_t *stream, char **body, int page_index, int page_size)
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
	uint8_t idSigGW;
	uint8_t idBplmn;
	char MCC[SIGGW_MCCMNC_MAX_LEN+1];
	char MNC[SIGGW_MCCMNC_MAX_LEN+1]; 
	uint8_t idNssai;


    cJSON_Hooks json_hooks, json_hooks2;
    cJSON *record_item, *record_array, *json_object;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
		stream->write_function(stream, "bplmn_queryN open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	char *tsql = agc_mprintf("SELECT * FROM sig_bplmn_table;");
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
	char *sql = agc_mprintf("SELECT * FROM sig_bplmn_table LIMIT %d OFFSET %d;", page_size, page_index);

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

			memset(MCC, 0, SIGGW_MCCMNC_MAX_LEN+1);
			memset(MNC, 0, SIGGW_MCCMNC_MAX_LEN+1);

			if(strlen(query_rec[3]) > SIGGW_MCCMNC_MAX_LEN || strlen(query_rec[4]) > SIGGW_MCCMNC_MAX_LEN)
			{
				continue;
			}

			ID		= atoi(query_rec[0]);
			idSigGW = atoi(query_rec[1]);
			idBplmn = atoi(query_rec[2]);
			strncpy(MCC, query_rec[3], strlen(query_rec[3]));
			strncpy(MNC, query_rec[4], strlen(query_rec[4]));
			idNssai	= atoi(query_rec[5]);		

			//Put result data to query Msg for OM server.
			record_item = cJSON_CreateObject();

			cJSON_AddNumberToObject(record_item, "ID", ID);
			cJSON_AddNumberToObject(record_item, "idSigGW", idSigGW);
			cJSON_AddNumberToObject(record_item, "idBplmn", idBplmn);
			cJSON_AddStringToObject(record_item, "MCC", MCC);
			cJSON_AddStringToObject(record_item, "MNC", MNC);
			cJSON_AddNumberToObject(record_item, "idNssai", idNssai);
			
			cJSON_AddItemToArray(record_array, record_item);
		}

		//record_array is ready.
	    cJSON_AddNumberToObject(json_object, "ret", status);
		cJSON_AddItemToObject(json_object, "records", record_array);
		cJSON_AddNumberToObject(json_object, "total", totalRec);

		*body = cJSON_Print(json_object);
		cJSON_Delete(json_object);

	    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "siggw bplmn query json body: %s.\n", *body);
		stream->write_function(stream, "bplmn_queryN [ok].\n");
	}
	else
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "bplmn_queryN [fail] ---  err_msg : %s .\n", err_msg);
		stream->write_function(stream, "bplmn_queryN [fail].\n");
		status = AGC_STATUS_GENERR;
	}

	free(sql);
	agc_db_free(err_msg);
	agc_db_free_table(query_result);

	return status;

}



//table datagw
agc_status_t datagw_insert(agc_stream_handle_t *stream, int argc, char **argv)
{

	agc_db_t *db = NULL;
	agc_db_stmt_t *stmt = NULL;
	char *err_msg = NULL;
	agc_status_t status = AGC_STATUS_GENERR;

	uint8_t idSigGW;
	char MediaGWName[SIGGW_MEDIAGW_MAX_LEN+1];

	//step 1 prepare parameter from argv.
	if(argv == NULL || argc < 2)
	{
		stream->write_function(stream, "datagw_insert input arg invalid.\n");
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "datagw_insert input arg invalid.\n");
		status = AGC_STATUS_SIGAPI_INPUT_ERROR;
		return status;
	}
	else
	{
		if(strlen(argv[1]) > SIGGW_MEDIAGW_MAX_LEN)
		{
			status = AGC_STATUS_SIGAPI_INPUT_ERROR;

			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "datagw_insert input name error.\n");
			stream->write_function(stream, "datagw_insert input name error.\n");
			
			return status;
		}

		memset(MediaGWName, 0, SIGGW_MEDIAGW_MAX_LEN+1);
	
		idSigGW = atoi(argv[0]);
		strncpy(MediaGWName, argv[1], strlen(argv[1]));
		//agc_log_printf(AGC_LOG, AGC_LOG_INFO, "datagw_insert : input arg %d ,%d, %d.\n", , , );	
	}

	//step 2 insert to db
	if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
		stream->write_function(stream, "datagw_insert open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_prepare(db, "insert into sig_mediagw_table(idSigGW, MediaGWName) values(?, ?);", -1, &stmt, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "datagw_insert prepare failed .\n");
		stream->write_function(stream, "datagw_insert prepare [fail].\n");
		agc_db_close(db);
		return AGC_STATUS_GENERR;
	}

	if (agc_db_bind_int(stmt, 1, idSigGW) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "datagw_insert bind idSigGW failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "datagw_insert bind idSigGW [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_bind_text(stmt, 2, MediaGWName, strlen(MediaGWName), AGC_CORE_DB_TRANSIENT) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "datagw_insert bind MediaGWName failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "datagw_insert bind MediaGWName [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_step(stmt) != AGC_DB_DONE) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "datagw_insert record failed .\n");
		stream->write_function(stream, "datagw_insert record [fail].\n");
	} else {
		status = AGC_STATUS_SUCCESS;
		stream->write_function(stream, "datagw_insert record [ok].\n");
	}

	agc_db_finalize(stmt);
	agc_db_close(db);

	return status;
}



//lyb 2020-6-16
agc_status_t datagw_update(agc_stream_handle_t *stream, int argc, char **argv)
{

	agc_db_t *db = NULL;
	char *err_msg = NULL;
	agc_status_t status = AGC_STATUS_SUCCESS;	

	uint32_t ID;
	uint8_t idSigGW;
	char MediaGWName[SIGGW_MEDIAGW_MAX_LEN+1];

	//step 1 prepare parameter from argv.
	if(argv == NULL || argc < 3)
	{
		stream->write_function(stream, "datagw_update input arg invalid.\n");
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "datagw_update input arg invalid.\n");
		status = AGC_STATUS_SIGAPI_INPUT_ERROR;
		return status;
	}
	else
	{

		if(strlen(argv[2]) > SIGGW_MEDIAGW_MAX_LEN)
		{
			status = AGC_STATUS_SIGAPI_INPUT_ERROR;

			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "datagw_update input name error.\n");
			stream->write_function(stream, "datagw_update input name error.\n");
			
			return status;
		}

		memset(MediaGWName, 0, SIGGW_MEDIAGW_MAX_LEN+1);	
		
		ID 		= atoi(argv[0]);
		idSigGW = atoi(argv[1]);
		strncpy(MediaGWName, argv[2], strlen(argv[2]));
		//agc_log_printf(AGC_LOG, AGC_LOG_INFO, "datagw_update : input arg %d ,%d, %d.\n", , , );	
	}

	//step 2 insert to db
	if (!(db = agc_db_open_file(SIG_DB_FILENAME))) {
		stream->write_function(stream, "datagw_update open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	char *sql = agc_mprintf("UPDATE sig_mediagw_table SET idSigGW = %d, MediaGWName = \"%s\" WHERE ID = %d;",idSigGW, MediaGWName, ID);

	if (agc_db_exec(db, sql, NULL, NULL, &err_msg) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
		stream->write_function(stream, "datagw_update db [fail].\n");
		agc_db_free(err_msg);
		agc_db_close(db);
		agc_safe_free(sql);
		return AGC_STATUS_GENERR;
	}

	stream->write_function(stream, "datagw_update db [ok].\n");
	agc_db_close(db);
	agc_safe_free(sql);
	return status;

}



agc_status_t datagw_delete(agc_stream_handle_t *stream, int argc, char **argv)
{

	agc_db_t *db = NULL;
	char *err_msg = NULL;

	uint32_t ID;
	
	 //step 1 prepare parameter from argv.
	 if(argv == NULL || argc < 1)
	 {
		 agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "datagw_delete : input arg invalid.\n");	 
		 return AGC_STATUS_SIGAPI_INPUT_ERROR;
	 }
	 else
	 {
		 ID = atoi(argv[0]);
		 //agc_log_printf(AGC_LOG, AGC_LOG_INFO, "taplmn_delete : input idSigGW: %d.\n", idSigGW);
	 }

	if (!(db = agc_db_open_file(SIG_DB_FILENAME))) {
		stream->write_function(stream, "datagw_delete open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	char *sql = agc_mprintf("DELETE FROM sig_mediagw_table WHERE ID = %d;", ID);

	if (agc_db_exec(db, sql, NULL, NULL, &err_msg) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
		stream->write_function(stream, "datagw_delete record [fail].\n");
		agc_db_free(err_msg);
		agc_db_close(db);
		agc_safe_free(sql);
		return AGC_STATUS_GENERR;
	}

	stream->write_function(stream, "datagw_delete record [ok].\n");
	agc_db_close(db);
	agc_safe_free(sql);
	return AGC_STATUS_SUCCESS;

}



//Query N record 
agc_status_t datagw_queryN(agc_stream_handle_t *stream, char **body, int page_index, int page_size)
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
	uint8_t idSigGW;
	char MediaGWName[SIGGW_MEDIAGW_MAX_LEN+1];


    cJSON_Hooks json_hooks, json_hooks2;
    cJSON *record_item, *record_array, *json_object;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
		stream->write_function(stream, "datagw_queryN open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	char *tsql = agc_mprintf("SELECT * FROM sig_mediagw_table;");
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
	char *sql = agc_mprintf("SELECT * FROM sig_mediagw_table LIMIT %d OFFSET %d;", page_size, page_index);

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

			memset(MediaGWName, 0, SIGGW_MEDIAGW_MAX_LEN+1);

			if(strlen(query_rec[2]) > SIGGW_MEDIAGW_MAX_LEN)
			{
				continue;
			}

			ID		= atoi(query_rec[0]);
			idSigGW = atoi(query_rec[1]);
			strncpy(MediaGWName, query_rec[2], strlen(query_rec[2]));

			//Put result data to query Msg for OM server.
			record_item = cJSON_CreateObject();

			cJSON_AddNumberToObject(record_item, "ID", ID);
			cJSON_AddNumberToObject(record_item, "idSigGW", idSigGW);
			cJSON_AddStringToObject(record_item, "MediaGWName", MediaGWName);
			
			cJSON_AddItemToArray(record_array, record_item);
		}

		//record_array is ready.
	    cJSON_AddNumberToObject(json_object, "ret", status);
		cJSON_AddItemToObject(json_object, "records", record_array);
		cJSON_AddNumberToObject(json_object, "total", totalRec);

		*body = cJSON_Print(json_object);
		cJSON_Delete(json_object);

	    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "siggw datagw query json body: %s.\n", *body);
		stream->write_function(stream, "datagw_queryN [ok].\n");
	}
	else
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "datagw_queryN [fail] ---  err_msg : %s .\n", err_msg);
		stream->write_function(stream, "datagw_queryN [fail].\n");
		status = AGC_STATUS_GENERR;
	}

	free(sql);
	agc_db_free(err_msg);
	agc_db_free_table(query_result);

	return status;

}


agc_status_t sctp_insert(agc_stream_handle_t *stream, int argc, char **argv, enum SCTPpeerpoint eSctpSide)
{
	agc_db_t *db = NULL;
	char *err_msg = NULL;
	agc_status_t status = AGC_STATUS_SUCCESS;
		
	uint32_t     		idSctpIndex;	
	uint8_t	   			idSigGW;            
	uint8_t	   			Priority;			
	Cfg_SctpWorkType 	WorkType;			
	enum Cfg_IPType     iptype;
	char	  LocalIP1[SIGGW_IPADDR_MAX_LEN+1];
	char	  LocalIP2[SIGGW_IPADDR_MAX_LEN+1];			
	uint16_t  LocalPort;			
	char	  PeerIP1[SIGGW_IPADDR_MAX_LEN+1];		
	char	  PeerIP2[SIGGW_IPADDR_MAX_LEN+1];		
	uint16_t  PeerPort;		
	uint8_t   gnbOrAmf;
	uint16_t  HeartBeatTime_s;
	uint16_t  SctpNum;
    Cfg_LteSctp CfgSctp;

	//step 1 prepare parameter from argv.
	if(argv == NULL || argc < 13)
	{
		stream->write_function(stream, "sctp_insert input arg invalid.\n");
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sctp_insert input arg invalid.\n");
		status = AGC_STATUS_SIGAPI_INPUT_ERROR;
		return status;
	}
	else
	{
		if(strlen(argv[5]) > SIGGW_IPADDR_MAX_LEN || strlen(argv[6]) > SIGGW_IPADDR_MAX_LEN || strlen(argv[8]) > SIGGW_IPADDR_MAX_LEN || strlen(argv[9]) > SIGGW_IPADDR_MAX_LEN)
		{
			status = AGC_STATUS_SIGAPI_INPUT_ERROR;

			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sctp_insert input ip error.\n");
			stream->write_function(stream, "sctp_insert input ip error.\n");
			
			return status;
		}

		memset(LocalIP1, 0, SIGGW_IPADDR_MAX_LEN+1);
		memset(LocalIP2, 0, SIGGW_IPADDR_MAX_LEN+1);
		memset(PeerIP1,  0, SIGGW_IPADDR_MAX_LEN+1);
		memset(PeerIP2,  0, SIGGW_IPADDR_MAX_LEN+1);
		
		//idSctpIndex = atoi(argv[0]);
		idSigGW 	= atoi(argv[1]);
		Priority	= atoi(argv[2]);
		WorkType	= (enum Cfg_SctpWorkType)atoi(argv[3]);
		iptype		= (enum Cfg_IPType)atoi(argv[4]);
        LocalPort	= atoi(argv[7]);
        PeerPort	= atoi(argv[10]);
		strncpy(LocalIP1, argv[5], strlen(argv[5]));
        GetDbManager().StringToIpAddr(LocalIP1, iptype, LocalPort, CfgSctp.LocalIP1, CfgSctp.LocalIP1Len);

		if (0 != strcmp(argv[6], "null")) //local IP2 input is valid
		{
			strncpy(LocalIP2, argv[6], strlen(argv[6]));
            GetDbManager().StringToIpAddr(LocalIP2, iptype, LocalPort, CfgSctp.LocalIP2, CfgSctp.LocalIP2Len);
		} else {
		    CfgSctp.LocalIP2Len = 0;
		}

		strncpy(PeerIP1, argv[8], strlen(argv[8]));
        GetDbManager().StringToIpAddr(PeerIP1, iptype, PeerPort, CfgSctp.PeerIP1, CfgSctp.PeerIP1Len);
		
		if (0 != strcmp(argv[9], "null")) //remote IP2 input is valid
		{
			strncpy(PeerIP2, argv[9], strlen(argv[9]));
            GetDbManager().StringToIpAddr(PeerIP2, iptype, PeerPort, CfgSctp.PeerIP2, CfgSctp.PeerIP2Len);
		} else {
		    CfgSctp.PeerIP2Len = 0;
		}

		gnbOrAmf = eSctpSide;
		HeartBeatTime_s = atoi(argv[11]);
		SctpNum = atoi(argv[12]);
		//agc_log_printf(AGC_LOG, AGC_LOG_INFO, "vgw_update : input arg %d ,%d, %d.\n", , , );

        //CfgSctp.idSctpIndex = idSctpIndex;
        CfgSctp.idSigGW = idSigGW;
        CfgSctp.Priority = Priority;
        CfgSctp.WorkType = WorkType;
        CfgSctp.iptype = iptype;
        CfgSctp.gnbOrAmf = gnbOrAmf;
        CfgSctp.HeartBeatTime_s = HeartBeatTime_s;
        CfgSctp.SctpNum = SctpNum;
	}

    if (AGC_STATUS_SUCCESS != GetSctpLayerDrv().CfgGetFreeIndex(idSctpIndex)) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sctp_insert : sctp layer get index failure.\n");
        return AGC_STATUS_GENERR;
    }
    CfgSctp.idSctpIndex = idSctpIndex;

    if (AGC_STATUS_SUCCESS != GetSctpLayerDrv().CfgAddSctp(CfgSctp)) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sctp_insert : sctp layer add failure.\n");
        return AGC_STATUS_GENERR;
    }

	//step 2 insert into db.
	if (!(db = agc_db_open_file(SIG_DB_FILENAME))) {
		stream->write_function(stream, "sctp_insert open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	//need check
	char *sql = agc_mprintf("insert into sig_sctp_table(idSctpIndex, idSigGW, Priority, WorkType, iptype, LocalIP1, LocalIP2,LocalPort, PeerIP1, PeerIP2, PeerPort, HeartBeatTime_s, SctpNum, gnbOrAmf) values(%d,%d,%d,%d,%d,\"%s\",\"%s\",%d,\"%s\",\"%s\",%d, %d, %d, %d);",idSctpIndex, idSigGW, Priority, WorkType, iptype, LocalIP1, LocalIP2, LocalPort, PeerIP1, PeerIP2, PeerPort, HeartBeatTime_s, SctpNum, gnbOrAmf);

	if (agc_db_exec(db, sql, NULL, NULL, &err_msg) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
		stream->write_function(stream, "sctp_insert record [fail].\n");
		agc_db_free(err_msg);
		agc_db_close(db);
		agc_safe_free(sql);
		return AGC_STATUS_GENERR;
	}

	stream->write_function(stream, "sctp_insert record [ok].\n");
	agc_db_close(db);
	agc_safe_free(sql);
	return AGC_STATUS_SUCCESS;

}


agc_status_t sctp_delete(agc_stream_handle_t *stream, int argc, char **argv, enum SCTPpeerpoint eSctpSide)
{
	agc_db_t *db = NULL;
	char *err_msg = NULL;

	uint32_t ID;
    Cfg_LteSctp CfgSctp;
    vector<Cfg_LteSctp *> Links;
	
	 //step 1 prepare parameter from argv.
	 if(argv == NULL || argc < 1)
	 {
		 agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sctp_delete : input arg invalid.\n");	 
		 return AGC_STATUS_SIGAPI_INPUT_ERROR;
	 }
	 else
	 {
		 ID = atoi(argv[0]);
		 //agc_log_printf(AGC_LOG, AGC_LOG_INFO, "taplmn_delete : input idSigGW: %d.\n", idSigGW);
	 }

	 if (GetDbManager().QuerySctpLinkById(ID, Links)) {
	     if (Links.size() == 1) {
             memcpy(&CfgSctp, Links[0], sizeof(Cfg_LteSctp));
             if (AGC_STATUS_SUCCESS != GetSctpLayerDrv().CfgRmvSctp(CfgSctp)) {
                 agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sctp_delete : sctp layer delete failure.\n");
             }
         }
	     for (int i = 0; i < Links.size(); i++) {
	         free(Links[i]);
	     }
	 } else {
         agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sctp_delete : not find record.\n");
	     return AGC_STATUS_SUCCESS;
	 }

	if (!(db = agc_db_open_file(SIG_DB_FILENAME))) {
		stream->write_function(stream, "sctp_delete open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	char *sql = agc_mprintf("DELETE FROM sig_sctp_table WHERE ID = %d AND gnbOrAmf = %d;", ID, eSctpSide);

	if (agc_db_exec(db, sql, NULL, NULL, &err_msg) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
		stream->write_function(stream, "sctp_delete record [fail].\n");
		agc_db_free(err_msg);
		agc_db_close(db);
		agc_safe_free(sql);
		return AGC_STATUS_GENERR;
	}

	stream->write_function(stream, "sctp_delete record [ok].\n");
	agc_db_close(db);
	agc_safe_free(sql);
	return AGC_STATUS_SUCCESS;

}


agc_status_t sctp_update(agc_stream_handle_t *stream, int argc, char **argv, enum SCTPpeerpoint eSctpSide)
{
	agc_db_t *db = NULL;
	char *err_msg = NULL;
	agc_status_t status = AGC_STATUS_SUCCESS;
		
	uint32_t     		ID;	
	uint32_t     		idSctpIndex;	
	uint8_t	   			idSigGW;            
	uint8_t	   			Priority;			
	Cfg_SctpWorkType 	WorkType;			
	enum Cfg_IPType     iptype;
	char	  LocalIP1[SIGGW_IPADDR_MAX_LEN+1];
	char	  LocalIP2[SIGGW_IPADDR_MAX_LEN+1];			
	uint16_t  LocalPort;			
	char	  PeerIP1[SIGGW_IPADDR_MAX_LEN+1];		
	char	  PeerIP2[SIGGW_IPADDR_MAX_LEN+1];		
	uint16_t  PeerPort;		
	uint8_t   gnbOrAmf;
	uint16_t  HeartBeatTime_s;
	uint16_t  SctpNum;
    Cfg_LteSctp CfgSctp;
    vector<Cfg_LteSctp *> Links;

	//step 1 prepare parameter from argv.
	if(argv == NULL || argc < 14)
	{
		stream->write_function(stream, "sctp_update input arg invalid.\n");
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sctp_update input arg invalid.\n");
		status = AGC_STATUS_SIGAPI_INPUT_ERROR;
		return status;
	}
	else
	{
		if(strlen(argv[6]) > SIGGW_IPADDR_MAX_LEN || strlen(argv[7]) > SIGGW_IPADDR_MAX_LEN || strlen(argv[9]) > SIGGW_IPADDR_MAX_LEN || strlen(argv[10]) > SIGGW_IPADDR_MAX_LEN)
		{
			status = AGC_STATUS_SIGAPI_INPUT_ERROR;

			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sctp_update input ip error.\n");
			stream->write_function(stream, "sctp_update input ip error.\n");
			
			return status;
		}

		memset(LocalIP1, 0, SIGGW_IPADDR_MAX_LEN+1);
		memset(LocalIP2, 0, SIGGW_IPADDR_MAX_LEN+1);
		memset(PeerIP1,  0, SIGGW_IPADDR_MAX_LEN+1);
		memset(PeerIP2,  0, SIGGW_IPADDR_MAX_LEN+1);
		
		ID		 	= atoi(argv[0]);
        if (GetDbManager().QuerySctpLinkById(ID, Links)) {
            if (Links.size() == 1) {
                memcpy(&CfgSctp, Links[0], sizeof(Cfg_LteSctp));
                if (AGC_STATUS_SUCCESS != GetSctpLayerDrv().CfgRmvSctp(CfgSctp)) {
                    agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sctp_update : sctp layer delete failure.\n");
                }
            }
            for (int i = 0; i < Links.size(); i++) {
                free(Links[i]);
            }
        }

		idSctpIndex = atoi(argv[1]);
		idSigGW 	= atoi(argv[2]);
		Priority	= atoi(argv[3]);
		WorkType	= (enum Cfg_SctpWorkType)atoi(argv[4]);
		iptype		= (enum Cfg_IPType)atoi(argv[5]);
        LocalPort	= atoi(argv[8]);
        PeerPort	= atoi(argv[11]);
		strncpy(LocalIP1, argv[6], strlen(argv[6]));
        GetDbManager().StringToIpAddr(LocalIP1, iptype, LocalPort, CfgSctp.LocalIP1, CfgSctp.LocalIP1Len);

		if (0 != strcmp(argv[7], "null")) //IP2 input is valid
		{
			strncpy(LocalIP2, argv[7], strlen(argv[7]));
			GetDbManager().StringToIpAddr(LocalIP2, iptype, LocalPort, CfgSctp.LocalIP2, CfgSctp.LocalIP2Len);
        } else {
            CfgSctp.LocalIP2Len = 0;
        }

		strncpy(PeerIP1, argv[9], strlen(argv[9]));
        GetDbManager().StringToIpAddr(PeerIP1, iptype, PeerPort, CfgSctp.PeerIP1, CfgSctp.PeerIP1Len);

		if (0 != strcmp(argv[10], "null")) //IP2 input is valid
		{
			strncpy(PeerIP2, argv[10], strlen(argv[10]));
            GetDbManager().StringToIpAddr(PeerIP2, iptype, LocalPort, CfgSctp.PeerIP2, CfgSctp.PeerIP2Len);
        } else {
            CfgSctp.PeerIP2Len = 0;
        }

		gnbOrAmf = eSctpSide;
		HeartBeatTime_s = atoi(argv[12]);
		SctpNum = atoi(argv[13]);
		//agc_log_printf(AGC_LOG, AGC_LOG_INFO, "vgw_update : input arg %d ,%d, %d.\n", , , );
        CfgSctp.idSctpIndex = idSctpIndex;
        CfgSctp.idSigGW = idSigGW;
        CfgSctp.Priority = Priority;
        CfgSctp.WorkType = WorkType;
        CfgSctp.iptype = iptype;
        CfgSctp.gnbOrAmf = gnbOrAmf;
        CfgSctp.HeartBeatTime_s = HeartBeatTime_s;
        CfgSctp.SctpNum = SctpNum;
	}

    if (AGC_STATUS_SUCCESS != GetSctpLayerDrv().CfgAddSctp(CfgSctp)) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sctp_update : sctp layer add failure.\n");
        return AGC_STATUS_GENERR;
    }

	//step 2 write to db.
	if (!(db = agc_db_open_file(SIG_DB_FILENAME))) {
		stream->write_function(stream, "sctp_update open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	char *sql = agc_mprintf("UPDATE sig_sctp_table SET idSctpIndex = %d, idSigGW = %d, Priority = %d, WorkType = %d, iptype = %d, LocalIP1 = \"%s\", LocalIP2 = \"%s\", LocalPort = %d, PeerIP1 = \"%s\", PeerIP2 = \"%s\", PeerPort = %d, gnbOrAmf = %d, HeartBeatTime_s = %d,  SctpNum = %d WHERE ID = %d;", idSctpIndex, idSigGW, Priority, WorkType, iptype, LocalIP1, LocalIP2, LocalPort, PeerIP1, PeerIP2, PeerPort, gnbOrAmf, HeartBeatTime_s, SctpNum, ID);

	if (agc_db_exec(db, sql, NULL, NULL, &err_msg) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
		stream->write_function(stream, "sctp_update record [fail].\n");
		agc_db_free(err_msg);
		agc_db_close(db);
		agc_safe_free(sql);
		return AGC_STATUS_GENERR;
	}

	stream->write_function(stream, "sctp_update record [ok].\n");
	agc_db_close(db);
	agc_safe_free(sql);
	return AGC_STATUS_SUCCESS;

}


agc_status_t sctp_queryN(agc_stream_handle_t *stream, char **body, int page_index, int page_size, enum SCTPpeerpoint eSctpSide)
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

	uint32_t     		ID;		
	uint32_t     		idSctpIndex;	
	uint8_t	   			idSigGW;            
	uint8_t	   			Priority;			
	Cfg_SctpWorkType 	WorkType;			
	enum Cfg_IPType     iptype;
	char	  LocalIP1[SIGGW_IPADDR_MAX_LEN+1];
	char	  LocalIP2[SIGGW_IPADDR_MAX_LEN+1];			
	uint16_t  LocalPort;			
	char	  PeerIP1[SIGGW_IPADDR_MAX_LEN+1];		
	char	  PeerIP2[SIGGW_IPADDR_MAX_LEN+1];		
	uint16_t  PeerPort;		
	uint16_t  HeartBeatTime_s;
	uint16_t  SctpNum;


    cJSON_Hooks json_hooks, json_hooks2;
    cJSON *record_item, *record_array, *json_object;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
		stream->write_function(stream, "sctp_queryN open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	char *tsql = agc_mprintf("SELECT * FROM sig_sctp_table WHERE gnbOrAmf = %d;", eSctpSide);
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
	char *sql = agc_mprintf("SELECT * FROM sig_sctp_table WHERE gnbOrAmf = %d LIMIT %d OFFSET %d;", eSctpSide, page_size, page_index);

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

			memset(LocalIP1, 0, SIGGW_IPADDR_MAX_LEN+1);
			memset(LocalIP2, 0, SIGGW_IPADDR_MAX_LEN+1);
			memset(PeerIP1,  0, SIGGW_IPADDR_MAX_LEN+1);
			memset(PeerIP2,  0, SIGGW_IPADDR_MAX_LEN+1);

			if(strlen(query_rec[6]) > SIGGW_IPADDR_MAX_LEN || strlen(query_rec[7]) > SIGGW_IPADDR_MAX_LEN || strlen(query_rec[9]) > SIGGW_IPADDR_MAX_LEN || strlen(query_rec[10]) > SIGGW_IPADDR_MAX_LEN)
			{
				continue;
			}

			ID			= atoi(query_rec[0]);
			idSctpIndex = atoi(query_rec[1]);
			idSigGW     = atoi(query_rec[2]);
			Priority	= atoi(query_rec[3]);
			WorkType 	= (enum Cfg_SctpWorkType)atoi(query_rec[4]);
			iptype 		= (enum Cfg_IPType)atoi(query_rec[5]);
			strncpy(LocalIP1, query_rec[6], strlen(query_rec[6]));
			strncpy(LocalIP2, query_rec[7], strlen(query_rec[7]));
			LocalPort	= atoi(query_rec[8]);
			strncpy(PeerIP1, query_rec[9], strlen(query_rec[9]));
			strncpy(PeerIP2, query_rec[10], strlen(query_rec[10]));
			PeerPort	= atoi(query_rec[11]);
			HeartBeatTime_s	= atoi(query_rec[12]);
			SctpNum	= atoi(query_rec[13]);

			//Put result data to query Msg for OM server.
			record_item = cJSON_CreateObject();
			
			cJSON_AddNumberToObject(record_item, "ID", ID);
			cJSON_AddNumberToObject(record_item, "idSctpIndex", idSctpIndex);
			cJSON_AddNumberToObject(record_item, "idSigGW", idSigGW);
			cJSON_AddNumberToObject(record_item, "Priority", Priority);
			cJSON_AddNumberToObject(record_item, "WorkType", WorkType);
			cJSON_AddNumberToObject(record_item, "iptype", iptype);

			cJSON_AddStringToObject(record_item, "LocalIP1", LocalIP1);
			cJSON_AddStringToObject(record_item, "LocalIP2", LocalIP2);
			cJSON_AddNumberToObject(record_item, "LocalPort", LocalPort);
			cJSON_AddStringToObject(record_item, "PeerIP1", PeerIP1);
			cJSON_AddStringToObject(record_item, "PeerIP2", PeerIP2);
			cJSON_AddNumberToObject(record_item, "PeerPort", PeerPort);

			cJSON_AddNumberToObject(record_item, "HeartBeatTime_s", HeartBeatTime_s);
			cJSON_AddNumberToObject(record_item, "SctpNum", SctpNum);
					
			cJSON_AddItemToArray(record_array, record_item);
		}

		//record_array is ready.
	    cJSON_AddNumberToObject(json_object, "ret", status);
		cJSON_AddItemToObject(json_object, "records", record_array);
		cJSON_AddNumberToObject(json_object, "total", totalRec);

		*body = cJSON_Print(json_object);
		cJSON_Delete(json_object);

	    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "siggw sctp query json body: %s.\n", *body);
		stream->write_function(stream, "sctp_queryN [ok].\n");
	}
	else
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sctp_queryN [fail] ---  err_msg : %s .\n", err_msg);
		stream->write_function(stream, "sctp_queryN [fail].\n");
		status = AGC_STATUS_GENERR;
	}

	free(sql);
	agc_db_free(err_msg);
	agc_db_free_table(query_result);

	return status;

}



agc_status_t sctp_statusN(agc_stream_handle_t *stream, char **body, int page_index, int page_size, enum SCTPpeerpoint eSctpSide)
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
	bool status_ret = false;

	uint32_t     		ID;		
	uint32_t     		idSctpIndex;	
	uint8_t	   			idSigGW;            
	uint8_t	   			Priority;			
	Cfg_SctpWorkType 	WorkType;			
	enum Cfg_IPType     iptype;
	char	  LocalIP1[SIGGW_IPADDR_MAX_LEN+1];
	char	  LocalIP2[SIGGW_IPADDR_MAX_LEN+1];			
	uint16_t  LocalPort;			
	char	  PeerIP1[SIGGW_IPADDR_MAX_LEN+1];		
	char	  PeerIP2[SIGGW_IPADDR_MAX_LEN+1];		
	uint16_t  PeerPort;		
	uint16_t  HeartBeatTime_s;
	uint16_t  SctpNum;


    cJSON_Hooks json_hooks, json_hooks2;
    cJSON *record_item, *record_array, *json_object;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
		stream->write_function(stream, "sctp_statusN open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	char *tsql = agc_mprintf("SELECT * FROM sig_sctp_table WHERE gnbOrAmf = %d;", eSctpSide);
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
	char *sql = agc_mprintf("SELECT * FROM sig_sctp_table WHERE gnbOrAmf = %d LIMIT %d OFFSET %d;", eSctpSide, page_size, page_index);

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

			memset(LocalIP1, 0, SIGGW_IPADDR_MAX_LEN+1);
			memset(LocalIP2, 0, SIGGW_IPADDR_MAX_LEN+1);
			memset(PeerIP1,  0, SIGGW_IPADDR_MAX_LEN+1);
			memset(PeerIP2,  0, SIGGW_IPADDR_MAX_LEN+1);

			if(strlen(query_rec[6]) > SIGGW_IPADDR_MAX_LEN || strlen(query_rec[7]) > SIGGW_IPADDR_MAX_LEN || strlen(query_rec[9]) > SIGGW_IPADDR_MAX_LEN || strlen(query_rec[10]) > SIGGW_IPADDR_MAX_LEN)
			{
				continue;
			}

			ID			= atoi(query_rec[0]);
			idSctpIndex = atoi(query_rec[1]);
			idSigGW     = atoi(query_rec[2]);
			Priority	= atoi(query_rec[3]);
			WorkType 	= (enum Cfg_SctpWorkType)atoi(query_rec[4]);
			iptype 		= (enum Cfg_IPType)atoi(query_rec[5]);
			strncpy(LocalIP1, query_rec[6], strlen(query_rec[6]));
			strncpy(LocalIP2, query_rec[7], strlen(query_rec[7]));
			LocalPort	= atoi(query_rec[8]);
			strncpy(PeerIP1, query_rec[9], strlen(query_rec[9]));
			strncpy(PeerIP2, query_rec[10], strlen(query_rec[10]));
			PeerPort	= atoi(query_rec[11]);
			HeartBeatTime_s	= atoi(query_rec[12]);
			SctpNum	= atoi(query_rec[13]);			

			//Put result data to query Msg for OM server.
			record_item = cJSON_CreateObject();
			
			status_ret = GetNsmManager().status_sctp(idSctpIndex);
			//Add SCTP status.		
			if(true == status_ret)
			{
				cJSON_AddStringToObject(record_item, "Status", "UP");
			}
			else
			{
				cJSON_AddStringToObject(record_item, "Status", "DOWN");
			}

			cJSON_AddNumberToObject(record_item, "idSctpIndex", idSctpIndex);						
			cJSON_AddNumberToObject(record_item, "idSigGW", idSigGW);
			cJSON_AddNumberToObject(record_item, "Priority", Priority);
			cJSON_AddNumberToObject(record_item, "WorkType", WorkType);
			cJSON_AddNumberToObject(record_item, "iptype", iptype);

			cJSON_AddStringToObject(record_item, "LocalIP1", LocalIP1);
			cJSON_AddStringToObject(record_item, "LocalIP2", LocalIP2);
			cJSON_AddNumberToObject(record_item, "LocalPort", LocalPort);
			cJSON_AddStringToObject(record_item, "PeerIP1", PeerIP1);
			cJSON_AddStringToObject(record_item, "PeerIP2", PeerIP2);
			cJSON_AddNumberToObject(record_item, "PeerPort", PeerPort);

			cJSON_AddNumberToObject(record_item, "HeartBeatTime_s", HeartBeatTime_s);
			cJSON_AddNumberToObject(record_item, "SctpNum", SctpNum);
					
			cJSON_AddItemToArray(record_array, record_item);
		}

		//record_array is ready.
	    cJSON_AddNumberToObject(json_object, "ret", status);
		cJSON_AddItemToObject(json_object, "records", record_array);
		cJSON_AddNumberToObject(json_object, "total", totalRec);

		*body = cJSON_Print(json_object);
		cJSON_Delete(json_object);

	    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "siggw sctp query json body: %s.\n", *body);
		stream->write_function(stream, "sctp_statusN [ok].\n");
	}
	else
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "sctp_queryN [fail] ---  err_msg : %s .\n", err_msg);
		stream->write_function(stream, "sctp_statusN [fail].\n");
		status = AGC_STATUS_GENERR;
	}

	free(sql);
	agc_db_free(err_msg);
	agc_db_free_table(query_result);

	return status;

}



/*

//need add member to support ipv6

//test_driver_listen6
vector<Cfg_LteSctp>  global_SctpToAMF;


vector<stCfg_TA_PLMN_API>	    global_TaPlmnList;


agc_status_t sig_api_add_taplmn(agc_stream_handle_t *stream, uint8_t idTA, uint32_t TAC, uint8_t idBplmn)
{
	stCfg_TA_PLMN_API tTaPlmn;
	agc_status_t status = AGC_STATUS_SUCCESS;

	//NEED check idAMF is exist in table or not.  TBD
	//if exist, return back with cause code.

	//if not exist, then push back and add.
	tTaPlmn.idTA = idTA;
	tTaPlmn.TAC = TAC;
	tTaPlmn.idBplmn = idBplmn;
	
	global_TaPlmnList.push_back(tTaPlmn);

	//add record to DB.
	status = taplmn_insert(stream, idTA, TAC, idBplmn);
	if(AGC_STATUS_SUCCESS != status)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG API add taplmn failed .\n");
		stream->write_function(stream, "SIG API add taplmn [fail].\n");
	}

	return status;
}


//Del one taplmn record
agc_status_t sig_api_del_taplmn(agc_stream_handle_t *stream, uint8_t idTA, uint8_t idBplmn)
{

	agc_status_t status = AGC_STATUS_SUCCESS;

	vector<stCfg_TA_PLMN_API>::iterator it = global_TaPlmnList.begin();

	//NEED check be used by other table or not   . TBD ---------------------------------------------
	//IF USED, return back- “can't delete”.

    //IF NOT be used.
	//search record in taplmn list.
	for (it = global_TaPlmnList.begin(); it != global_TaPlmnList.end();)
	{
		if(((*it).idTA == idTA)
			&& ((*it).idBplmn == idBplmn))

		{
			//delete record from taplmn list.
			global_TaPlmnList.erase(it);
		}

	}

	//del record from DB.
	status = taplmn_delete(stream, idTA, idBplmn);
	if(AGC_STATUS_SUCCESS != status)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG API taplmn_delete failed .\n");
		stream->write_function(stream, "SIG API taplmn_delete [fail].\n");
	}

	return status;
}


agc_status_t sig_api_query_taplmn(agc_stream_handle_t *stream)
{

}


//Del one vgw record
agc_status_t sig_api_del_vgw(agc_stream_handle_t *stream, uint8_t idSigGW)
{

	agc_status_t status = AGC_STATUS_SUCCESS;

	vector<stCfg_vGWParam_API>::iterator it = global_vGwList.begin();

	//NEED check be used by other table or not   . TBD ---------------------------------------------
	//IF USED, return back- “can't delete”.

    //IF NOT be used.
	//search record in vgw list.
	for (it = global_vGwList.begin(); it != global_vGwList.end();)
	{
		if((*it).idSigGW == idSigGW)
		{
			//delete record from vgw list.
			global_vGwList.erase(it);
		}

	}

	//del record from DB.
	status = vgw_delete(stream, idSigGW);
	if(AGC_STATUS_SUCCESS != status)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG API vgw_delete failed .\n");
		stream->write_function(stream, "SIG API vgw_delete [fail].\n");
	}

	return status;
}


//Update one vgw record
agc_status_t sig_api_update_vgw(agc_stream_handle_t *stream, uint8_t idSigGW, char *AMFName, uint8_t AMFCapacity, uint32_t gNBid, char *gNBName, uint16_t MCC, uint16_t MNC, enum Cfg_PagingDRX DRX, enum Cfg_GNBType gNBType, enum Cfg_NG_ENBType ngeNBType)
{

	agc_status_t status = AGC_STATUS_SUCCESS;

	vector<stCfg_vGWParam_API>::iterator it = global_vGwList.begin();

	if(strlen(AMFName) > 255 || strlen(gNBName) > 255 )
	{
		status = AGC_STATUS_FALSE;

		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG API update vgw input name error.\n");
		stream->write_function(stream, "SIG API update vgw input name error.\n");
		
		return status;
	}

	//update record in vgw list.
	for (it = global_vGwList.begin(); it != global_vGwList.end(); it++)
	{
		if((*it).idSigGW == idSigGW)
		{
			//update record from vgw list.
			strcpy(it->AMFName, AMFName);
			it->AMFCapacity = AMFCapacity;	
			it->gNBid = gNBid;
			strcpy(it->gNBName, gNBName);
			it->Plmn.MCC = MCC;
			it->Plmn.MNC = MNC;
			it->DRX = DRX;
			it->gNBType = gNBType;
			it->ngeNBType = ngeNBType;
			break;

		}
	}

	//update record from DB.
	status = vgw_update(stream, idSigGW, AMFName, AMFCapacity, gNBid, gNBName, MCC, MNC, DRX, gNBType, ngeNBType);
	if(AGC_STATUS_SUCCESS != status)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG API update_vgw failed .\n");
		stream->write_function(stream, "SIG API update_vgw [fail].\n");
	}

	return status;
}


vector<stCfg_BPlmn_API>	global_bplmn;


//Add one bPLMN record

//input  : idBplmn,  MCC,  MNC,  idNssai
//process:modify global parameter global_bplmn and table bplmn in db.
//output :result of add record.

agc_status_t sig_api_add_bplmn(agc_stream_handle_t *stream, uint8_t idBplmn, uint16_t MCC, uint16_t MNC, uint8_t idNssai)
{
	stCfg_BPlmn_API tBplmn;
	agc_status_t status = AGC_STATUS_SUCCESS;

	//NEED check idBplmn is exist in table or not.  TBD
	//if exist, return back with cause code.

	//if not exist, then push back and add.
	tBplmn.idBplmn = idBplmn;
	tBplmn.MCC = MCC;
	tBplmn.MNC = MNC;
	tBplmn.idNssai = idNssai;

	global_bplmn.push_back(tBplmn);

	//add record to DB.
	status = bplmn_insert(stream, idBplmn, MCC, MNC, idNssai);
	if(AGC_STATUS_SUCCESS != status)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG API add bplmn failed .\n");
		stream->write_function(stream, "SIG API add bplmn [fail].\n");
	}

	return status;
}


//Del one bplmn record
agc_status_t sig_api_del_bplmn(agc_stream_handle_t *stream, uint8_t idBplmn)
{

	agc_status_t status = AGC_STATUS_SUCCESS;

	vector<stCfg_BPlmn_API>::iterator it = global_bplmn.begin();

	//NEED check be used by other table or not   . TBD ---------------------------------------------
	//IF USED, return back- “can't delete”.

    //IF NOT be used.
	//search record in bplmn list.
	for (it = global_bplmn.begin(); it != global_bplmn.end();)
	{
		if((*it).idBplmn == idBplmn)
		{
			//delete record from bplmn list.
			global_bplmn.erase(it);
		}

	}

	//del record from DB.
	status = bplmn_delete(stream, idBplmn);
	if(AGC_STATUS_SUCCESS != status)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG API bplmn_delete failed .\n");
		stream->write_function(stream, "SIG API bplmn_delete [fail].\n");
	}

	return status;
}






//TBD 2020 -------------------------------------------------
//query bplmn record
//result need package by api,  unpackage by caller.
agc_status_t sig_api_query_bplmn(agc_stream_handle_t *stream, uint8_t idBplmn, uint16_t MCC, uint16_t MNC, uint8_t idNssai)
{

	agc_status_t status = AGC_STATUS_SUCCESS;

	return status;
}


//Update one bplmn record
agc_status_t sig_api_update_bplmn(agc_stream_handle_t *stream, uint8_t idBplmn, uint16_t MCC, uint16_t MNC, uint8_t idNssai)
{

	agc_status_t status = AGC_STATUS_SUCCESS;

	vector<stCfg_BPlmn_API>::iterator it = global_bplmn.begin();

	//update record in bplmn list.
	for (it = global_bplmn.begin(); it != global_bplmn.end(); it++)
	{
		if((*it).idBplmn == idBplmn)
		{
			//update record from bplmn list.
			it->MCC = MCC;
			it->MNC = MNC;
			it->idNssai = idNssai;
			break;

		}
	}

	//update record from DB.
	status = bplmn_update(stream, idBplmn, MCC, MNC, idNssai);
	if(AGC_STATUS_SUCCESS != status)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG API update_bplmn failed .\n");
		stream->write_function(stream, "SIG API update_bplmn [fail].\n");
	}

	return status;
}


vector<stCfg_GUAMI_API>	global_guAmiList;

agc_status_t sig_api_add_guami(agc_stream_handle_t *stream, uint8_t idAMF, uint16_t MCC, uint16_t MNC, uint8_t AMFRegionID, uint16_t AMFSetId, uint8_t AMFPointer)
{
	stCfg_GUAMI_API tGuami;
	agc_status_t status = AGC_STATUS_SUCCESS;

	//NEED check idAMF is exist in table or not.  TBD
	//if exist, return back with cause code.

	//if not exist, then push back and add.
	tGuami.idAMF = idAMF;
	tGuami.MCC = MCC;
	tGuami.MNC = MNC;
	tGuami.AMFRegionID = AMFRegionID;
	tGuami.AMFSetId = AMFSetId;
	tGuami.AMFPointer =	AMFPointer;
	
	global_guAmiList.push_back(tGuami);

	//add record to DB.
	status = guami_insert(stream, idAMF, MCC, MNC, AMFRegionID, AMFSetId, AMFPointer);
	if(AGC_STATUS_SUCCESS != status)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG API add guami failed .\n");
		stream->write_function(stream, "SIG API add guami [fail].\n");
	}

	return status;
}



//Del one guami record
agc_status_t sig_api_del_guami(agc_stream_handle_t *stream, uint8_t idAMF)
{

	agc_status_t status = AGC_STATUS_SUCCESS;

	vector<stCfg_GUAMI_API>::iterator it = global_guAmiList.begin();

	//NEED check be used by other table or not   . TBD ---------------------------------------------
	//IF USED, return back- “can't delete”.

    //IF NOT be used.
	//search record in guami list.
	for (it = global_guAmiList.begin(); it != global_guAmiList.end();)
	{
		if((*it).idAMF == idAMF)
		{
			//delete record from guami list.
			global_guAmiList.erase(it);
		}

	}

	//del record from DB.
	status = guami_delete(stream, idAMF);
	if(AGC_STATUS_SUCCESS != status)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG API guami_delete failed .\n");
		stream->write_function(stream, "SIG API guami_delete [fail].\n");
	}

	return status;
}



//Update one guami record
agc_status_t sig_api_update_guami(agc_stream_handle_t *stream, uint8_t idAMF, uint16_t MCC, uint16_t MNC, uint8_t AMFRegionID, uint16_t AMFSetId, uint8_t AMFPointer)
{

	agc_status_t status = AGC_STATUS_SUCCESS;

	vector<stCfg_GUAMI_API>::iterator it = global_guAmiList.begin();

	//update record in guami list.
	for (it = global_guAmiList.begin(); it != global_guAmiList.end(); it++)
	{
		if((*it).idAMF == idAMF)
		{
			//update record from guami list.
			it->MCC = MCC;
			it->MNC = MNC;
			it->AMFRegionID = AMFRegionID;
			it->AMFSetId = AMFSetId;
			it->AMFPointer = AMFPointer;
			break;

		}
	}

	//update record from DB.
	status = guami_update(stream, idAMF, MCC, MNC, AMFRegionID, AMFSetId, AMFPointer);
	if(AGC_STATUS_SUCCESS != status)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG API update_guami failed .\n");
		stream->write_function(stream, "SIG API update_guami [fail].\n");
	}

	return status;
}

agc_status_t sig_api_query_guami(agc_stream_handle_t *stream, uint8_t idAMF)
{

}






//table 4



agc_status_t sig_api_add_amfgw(agc_stream_handle_t *stream, uint8_t idSigGW, char *AMFName, uint8_t AMFCapacity)
{
	stCfg_gNBInterParam_API tAmfGw;
	agc_status_t status = AGC_STATUS_SUCCESS;

	//if exist, return back with cause code.

	//if not exist, then push back and add.

	if(strlen(AMFName) > 255)
	{
		status = AGC_STATUS_FALSE;

		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG API add amfgw input AMF name error.\n");
		stream->write_function(stream, "SIG API add amfgw input AMF name error.\n");
		
		return status;
	}

	tAmfGw.idSigGW = idSigGW;
	strcpy(tAmfGw.AMFName, AMFName);
	tAmfGw.AMFCapacity = AMFCapacity;
	
	global_AmfGwList.push_back(tAmfGw);

	//add record to DB.
	status = amfgw_insert(stream, idSigGW, AMFName, AMFCapacity);
	if(AGC_STATUS_SUCCESS != status)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG API add amfgw failed .\n");
		stream->write_function(stream, "SIG API add amfgw [fail].\n");
	}

	return status;
}




//Del one amfgw record
agc_status_t sig_api_del_amfgw(agc_stream_handle_t *stream, uint8_t idSigGW)
{

	agc_status_t status = AGC_STATUS_SUCCESS;

	vector<stCfg_gNBInterParam_API>::iterator it = global_AmfGwList.begin();

	//NEED check be used by other table or not   . TBD ---------------------------------------------
	//IF USED, return back- “can't delete”.

    //IF NOT be used.
	//search record in amfgw list.
	for (it = global_AmfGwList.begin(); it != global_AmfGwList.end();)
	{
		if((*it).idSigGW == idSigGW)
		{
			//delete record from amfgw list.
			global_AmfGwList.erase(it);
		}

	}

	//del record from DB.
	status = amfgw_delete(stream, idSigGW);
	if(AGC_STATUS_SUCCESS != status)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG API amfgw_delete failed .\n");
		stream->write_function(stream, "SIG API amfgw_delete [fail].\n");
	}

	return status;
}




//Update one amfgw record
agc_status_t sig_api_update_amfgw(agc_stream_handle_t *stream, uint8_t idSigGW, char *AMFName, uint8_t AMFCapacity)
{

	agc_status_t status = AGC_STATUS_SUCCESS;

	vector<stCfg_gNBInterParam_API>::iterator it = global_AmfGwList.begin();

	if(strlen(AMFName) > 255)
	{
		status = AGC_STATUS_FALSE;

		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG API update amfgw input AMF name error.\n");
		stream->write_function(stream, "SIG API update amfgw input AMF name error.\n");
		
		return status;
	}

	//update record in amfgw list.
	for (it = global_AmfGwList.begin(); it != global_AmfGwList.end(); it++)
	{
		if((*it).idSigGW == idSigGW)
		{
			//update record from amfgw list.
			strcpy(it->AMFName, AMFName);
			it->AMFCapacity = AMFCapacity;
			break;

		}
	}

	//update record from DB.
	status = amfgw_update(stream, idSigGW, AMFName, AMFCapacity);
	if(AGC_STATUS_SUCCESS != status)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG API update_amfgw failed .\n");
		stream->write_function(stream, "SIG API update_amfgw [fail].\n");
	}

	return status;
}




agc_status_t sig_api_query_amfgw(agc_stream_handle_t *stream)
{

}



vector<stCfg_SigGW_AMF_API>	    global_sigGwAmfList;

agc_status_t sig_api_add_siggwamf(agc_stream_handle_t *stream, uint8_t idSigGW, uint8_t idAMF)
{
	stCfg_SigGW_AMF_API tGwAmf;
	agc_status_t status = AGC_STATUS_SUCCESS;

	//NEED check idAMF is exist in table or not.  TBD
	//if exist, return back with cause code.

	//if not exist, then push back and add.
	tGwAmf.idSigGW = idSigGW;
	tGwAmf.idAMF = idAMF;
	
	global_sigGwAmfList.push_back(tGwAmf);

	//add record to DB.
	status = siggwamf_insert(stream, idSigGW, idAMF);
	if(AGC_STATUS_SUCCESS != status)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG API add siggwamf failed .\n");
		stream->write_function(stream, "SIG API add siggwamf [fail].\n");
	}

	return status;
}


//Del one siggwamf record
agc_status_t sig_api_del_siggwamf(agc_stream_handle_t *stream, uint8_t idSigGW, uint8_t idAMF)
{

	agc_status_t status = AGC_STATUS_SUCCESS;

	vector<stCfg_SigGW_AMF_API>::iterator it = global_sigGwAmfList.begin();

	//NEED check be used by other table or not   . TBD ---------------------------------------------
	//IF USED, return back- “can't delete”.

    //IF NOT be used.
	//search record in siggwamf list.
	for (it = global_sigGwAmfList.begin(); it != global_sigGwAmfList.end();)
	{
		if(((*it).idAMF == idAMF)
			&& ((*it).idSigGW == idSigGW))
		{
			//delete record from siggwamf list.
			global_sigGwAmfList.erase(it);
		}

	}

	//del record from DB.
	status = siggwamf_delete(stream, idSigGW, idAMF);
	if(AGC_STATUS_SUCCESS != status)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG API siggwamf_delete failed .\n");
		stream->write_function(stream, "SIG API siggwamf_delete [fail].\n");
	}

	return status;
}


agc_status_t sig_api_query_siggwamf(agc_stream_handle_t *stream)
{

}


vector<stCfg_SigGW_PLMN_API>	global_sigGwPlmnList;

agc_status_t sig_api_add_siggwplmn(agc_stream_handle_t *stream, uint8_t idSigGW, uint8_t idBplmn)
{
	stCfg_SigGW_PLMN_API tGwPlmn;
	agc_status_t status = AGC_STATUS_SUCCESS;

	//NEED check idAMF is exist in table or not.  TBD
	//if exist, return back with cause code.

	//if not exist, then push back and add.
	tGwPlmn.idSigGW = idSigGW;
	tGwPlmn.idBplmn = idBplmn;
	
	global_sigGwPlmnList.push_back(tGwPlmn);

	//add record to DB.
	status = siggwplmn_insert(stream, idSigGW, idBplmn);
	if(AGC_STATUS_SUCCESS != status)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG API add siggwplmn failed .\n");
		stream->write_function(stream, "SIG API add siggwplmn [fail].\n");
	}

	return status;
}


//Del one siggwplmn record
agc_status_t sig_api_del_siggwplmn(agc_stream_handle_t *stream, uint8_t idSigGW, uint8_t idBplmn)
{

	agc_status_t status = AGC_STATUS_SUCCESS;

	vector<stCfg_SigGW_PLMN_API>::iterator it = global_sigGwPlmnList.begin();

	//NEED check be used by other table or not   . TBD ---------------------------------------------
	//IF USED, return back- “can't delete”.

    //IF NOT be used.
	//search record in siggwplmn list.
	for (it = global_sigGwPlmnList.begin(); it != global_sigGwPlmnList.end();)
	{
		if(((*it).idBplmn == idBplmn)
			&& ((*it).idSigGW == idSigGW))

		{
			//delete record from siggwplmn list.
			global_sigGwPlmnList.erase(it);
		}

	}

	//del record from DB.
	status = siggwplmn_delete(stream, idSigGW, idBplmn);
	if(AGC_STATUS_SUCCESS != status)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG API siggwplmn_delete failed .\n");
		stream->write_function(stream, "SIG API siggwplmn_delete [fail].\n");
	}

	return status;
}



agc_status_t sig_api_query_vgw(agc_stream_handle_t *stream)
{

}


vector<stCfg_SigGW_TA_API>  	global_sigGwTAList;

agc_status_t sig_api_add_gwta(agc_stream_handle_t *stream, uint8_t idSigGW, uint8_t idTA)
{
	stCfg_SigGW_TA_API tgwTa;
	agc_status_t status = AGC_STATUS_SUCCESS;

	//NEED check idAMF is exist in table or not.  TBD
	//if exist, return back with cause code.

	//if not exist, then push back and add.
	tgwTa.idSigGW = idSigGW;
	tgwTa.idTA = idTA;
	
	global_sigGwTAList.push_back(tgwTa);

	//add record to DB.
	status = gwta_insert(stream, idSigGW, idTA);
	if(AGC_STATUS_SUCCESS != status)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG API add gwta failed .\n");
		stream->write_function(stream, "SIG API add gwta [fail].\n");
	}

	return status;
}


//Del one gwta record
agc_status_t sig_api_del_gwta(agc_stream_handle_t *stream, uint8_t idSigGW, uint8_t idTA)
{

	agc_status_t status = AGC_STATUS_SUCCESS;

	vector<stCfg_SigGW_TA_API>::iterator it = global_sigGwTAList.begin();

	//NEED check be used by other table or not   . TBD ---------------------------------------------
	//IF USED, return back- “can't delete”.

    //IF NOT be used.
	//search record in gwta list.
	for (it = global_sigGwTAList.begin(); it != global_sigGwTAList.end();)
	{
		if(((*it).idSigGW == idSigGW)
			&& ((*it).idTA == idTA))
		{
			//delete record from gwta list.
			global_sigGwTAList.erase(it);
		}

	}

	//del record from DB.
	status = gwta_delete(stream, idSigGW, idTA);
	if(AGC_STATUS_SUCCESS != status)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG API gwta_delete failed .\n");
		stream->write_function(stream, "SIG API gwta_delete [fail].\n");
	}

	return status;
}


agc_status_t sig_api_query_gwta(agc_stream_handle_t *stream)
{

}



agc_status_t sig_api_query_siggwplmn(agc_stream_handle_t *stream)
{

}


//table 7
vector<stCfg_vGWParam_API>	global_vGwList;


agc_status_t sig_api_add_vgw(agc_stream_handle_t *stream, int argc, char **argv)
{
	stCfg_vGWParam_API tvGW;
	agc_status_t status = AGC_STATUS_SUCCESS;


	//NEED check idAMF is exist in table or not.  TBD
	//if exist, return back with cause code.

	//if not exist, then push back and add.
	tvGW.idSigGW = idSigGW;
	strcpy(tvGW.AMFName, AMFName);
	tvGW.AMFCapacity = AMFCapacity;
	tvGW.gNBid = gNBid;
	strcpy(tvGW.gNBName, gNBName);
	tvGW.Plmn.MCC = MCC;
	tvGW.Plmn.MNC = MNC;
	tvGW.DRX = DRX;
	tvGW.gNBType = gNBType;
	tvGW.ngeNBType = ngeNBType;
	
	global_vGwList.push_back(tvGW);

	//add record to DB.
	status = vgw_insert(stream, idSigGW, AMFName, AMFCapacity, gNBid, gNBName, MCC, MNC, DRX, gNBType, ngeNBType);
	if(AGC_STATUS_SUCCESS != status)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG API add vgw failed .\n");
		stream->write_function(stream, "SIG API add vgw [fail].\n");
	}

	return status;
}



agc_status_t sig_api_query_nssai(agc_stream_handle_t *stream)
{
	agc_status_t status = AGC_STATUS_SUCCESS;

	//Step1. Prepare query_NssaiIdList from query request msg.
	//TBD.

	//Step2: Query
	switch (query_NssaiIdList.size())
	{
		case 0:		//query all record
			
			status = nssai_query(stream, NULL);
			if(AGC_STATUS_SUCCESS != status)
			{
				agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SIG API query nssai failed .\n");
				stream->write_function(stream, "SIG API query nssai [fail].\n");
			}

			break;
			
		case 1:		//query one record
			//TBD

			break;
			
		default:
			break;
			
	}


	//Step3: Reset query_NssaiIdList
	query_NssaiIdList.clear();	
	
	return status;
}


//Query all record
agc_status_t nssai_query(agc_stream_handle_t *stream, char **body)
{
	agc_db_t *db = NULL;
	agc_db_stmt_t *stmt = NULL;
	int rc;
	int columns = 0;
	
	uint8_t	  idNssai;
	uint8_t   SST;	
	uint32_t  SD;	
	
	int ret = 0;
	agc_status_t status = AGC_STATUS_GENERR;

    cJSON_Hooks json_hooks, json_hooks2;
    cJSON *record_item, *record_array, *json_object;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
		stream->write_function(stream, "nssai_query open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_prepare(db, "SELECT * FROM sig_nssai_table;", -1, &stmt, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "nssai_query failed .\n");
		stream->write_function(stream, "nssai_query prepare [fail].\n");
		agc_db_close(db);
		return AGC_STATUS_GENERR;
	}	

    json_hooks.malloc_fn = malloc;
    json_hooks.free_fn = free;
    cJSON_InitHooks(&json_hooks);
    record_array = cJSON_CreateArray();
    json_object = cJSON_CreateObject();

	do {
		rc = agc_db_step(stmt);
		switch(rc) {
			case AGC_DB_DONE:
				break;

			case AGC_DB_ROW:
				columns = agc_db_column_count(stmt);
				
				idNssai = agc_db_column_int(stmt, 0);
				SST = agc_db_column_int(stmt, 1);
				SD = agc_db_column_int(stmt, 2);

				//Put result data to query Msg for OM server.
				record_item = cJSON_CreateObject();
				cJSON_AddNumberToObject(record_item, "idNssai", idNssai);
				cJSON_AddNumberToObject(record_item, "SST", SST);
				cJSON_AddNumberToObject(record_item, "SD", SD);
				cJSON_AddItemToArray(record_array, record_item);
								
				//agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Query result row -- idNssai %d, SST %d, SD %d .\n", idNssai, SST, SD);
				break;
			default:
				ret = 1;
				break;
		}
	} while (rc == AGC_DB_ROW);

	//record_array is ready.
    cJSON_AddNumberToObject(json_object, "ret", ret);
	cJSON_AddItemToObject(json_object, "records", record_array);

	*body = cJSON_Print(json_object);
	cJSON_Delete(json_object);

    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "siggw nssai query json body: %s.\n", *body);

	if (ret) {
		stream->write_function(stream, "nssai_query [fail].\n");
	} else {
		status = AGC_STATUS_SUCCESS;
		stream->write_function(stream, "nssai_query [ok].\n");
	}

	agc_db_finalize(stmt);
	agc_db_close(db);

	return status;

}

*/


