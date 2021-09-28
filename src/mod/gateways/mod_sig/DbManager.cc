
#include "mod_sig.h"
#include "DbManager.h"


DbManager::DbManager()
{
	agc_memory_pool_t *module_pool = agcsig_get_sig_memory_pool();
	agc_mutex_init(&m_mutex, AGC_MUTEX_NESTED, module_pool);
}

DbManager::~DbManager()
{
}

bool DbManager::QueryMediaGW(std::vector<stCfg_MEDIA_DATA> &medias)
{
	agc_db_t *db = NULL;
	agc_db_stmt_t *stmt = NULL;
	int rc;
	int columns = 0;
	const char *text;
	
	int error = 0;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryMediaGW open db [fail].\n");
		return false;
	}

	if (agc_db_prepare(db, "SELECT * FROM sig_mediagw_table;", -1, &stmt, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryMediaGW failed .\n");
		agc_db_close(db);
		return false;
	}	

	do {
		rc = agc_db_step(stmt);
		switch(rc) {
			case AGC_DB_DONE:
				break;

			case AGC_DB_ROW:
			{
				stCfg_MEDIA_DATA media = {0};
				columns = agc_db_column_count(stmt);
				
				media.idSigGW = agc_db_column_int(stmt, 1);
				text = (const char *)agc_db_column_text(stmt, 2);
				strcpy(media.MediaGWName, text);

				medias.push_back(media);
				break;
			}
			default:
				error = 1;
				break;
		}
	} while (rc == AGC_DB_ROW);

	if (error) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryMediaGW [fail].\n");
	} else {
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "DbManager::QueryMediaGW size=%d[ok].\n", medias.size());
	}

	agc_db_finalize(stmt);
	agc_db_close(db);

	return true;

}

bool DbManager::QueryVgwParam(uint8_t idSigGW, stCfg_vGWParam_API &param)
{
	agc_db_t *db = NULL;
	agc_db_stmt_t *stmt = NULL;
	int rc;
	int columns = 0;
	const char *text;
	
	int error = 0;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryVgwParam open db [fail].\n");
		return false;
	}

	if (agc_db_prepare(db, "SELECT * FROM sig_vgw_table WHERE idSigGW = ?;", -1, &stmt, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryVgwParam failed .\n");
		agc_db_close(db);
		return false;
	}	

	if (agc_db_bind_int(stmt, 1, idSigGW) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryVgwParam bind failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		return false;
	}

	do {
		rc = agc_db_step(stmt);
		switch(rc) {
			case AGC_DB_DONE:
				break;

			case AGC_DB_ROW:
				columns = agc_db_column_count(stmt);
				
				param.idSigGW = agc_db_column_int(stmt, 1);
				text = (const char *)agc_db_column_text(stmt, 2);
				strcpy(param.AMFName, text);

				param.AMFCapacity = agc_db_column_int(stmt, 3);
				param.gNBid = agc_db_column_int(stmt, 4);
				text = (const char *)agc_db_column_text(stmt, 5);
				strcpy(param.gNBName, text);

				text = (const char *)agc_db_column_text(stmt, 6);
				param.Plmn.mcc1 = text[0] - '0';
				param.Plmn.mcc2 = text[1] - '0';
				param.Plmn.mcc3 = text[2] - '0';
				text = (const char *)agc_db_column_text(stmt, 7);
				if (strlen(text) == 2) {
					param.Plmn.mnc1 = 0xF;
					param.Plmn.mnc2 = text[0] - '0';
					param.Plmn.mnc3 = text[1] - '0';
				}
				else {
					param.Plmn.mnc1 = text[0] - '0';
					param.Plmn.mnc2 = text[1] - '0';
					param.Plmn.mnc3 = text[2] - '0';
				}
				//param.Plmn.mcc = atoi((const char*)agc_db_column_text(stmt, 6));
				//param.Plmn.mnc = atoi((const char*)agc_db_column_text(stmt, 7));
				param.DRX = (Cfg_PagingDRX)agc_db_column_int(stmt, 8);
				param.gNBType = (Cfg_GNBType)agc_db_column_int(stmt, 9);
				param.ngeNBType = (Cfg_NG_ENBType)agc_db_column_int(stmt, 10);

				break;
			default:
				error = 1;
				break;
		}
	} while (rc == AGC_DB_ROW);

	if (error) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryVgwParam [fail].\n");
	} else {
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "DbManager::QueryVgwParam [ok].\n" );
	}

	agc_db_finalize(stmt);
	agc_db_close(db);

	return true;
}

bool DbManager::QueryBplmn(uint8_t idSigGw, 
	std::vector<stCfg_BPlmn_API> &plmns)
{
	agc_db_t *db = NULL;
	agc_db_stmt_t *stmt = NULL;
	int rc;
	int columns = 0;
	const char *text;
	
	int error = 0;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryBplmn open db [fail].\n");
		return false;
	}

	if (agc_db_prepare(db, "SELECT * FROM sig_bplmn_view WHERE idSigGW = ?;", -1, &stmt, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryBplmn failed idSigGw=%d.\n",idSigGw);
		agc_db_close(db);
		return false;
	}	

	if (agc_db_bind_int(stmt, 1, idSigGw) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryBplmn bind failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		return false;
	}

	do {
		rc = agc_db_step(stmt);
		switch(rc) {
			case AGC_DB_DONE:
				break;

			case AGC_DB_ROW:
			{
				stCfg_BPlmn_API plmn = {0};
				columns = agc_db_column_count(stmt);
				
				plmn.idSigGW = agc_db_column_int(stmt, 0);
				plmn.idBplmn = agc_db_column_int(stmt, 1);
				text = (const char *)agc_db_column_text(stmt, 2);
				plmn.plmn.mcc1 = text[0] - '0';
				plmn.plmn.mcc2 = text[1] - '0';
				plmn.plmn.mcc3 = text[2] - '0';
				text = (const char *)agc_db_column_text(stmt, 3);
				if (strlen(text) == 2) {
					plmn.plmn.mnc1 = 0xF;
					plmn.plmn.mnc2 = text[0] - '0';
					plmn.plmn.mnc3 = text[1] - '0';
				}
				else {
					plmn.plmn.mnc1 = text[0] - '0';
					plmn.plmn.mnc2 = text[1] - '0';
					plmn.plmn.mnc3 = text[2] - '0';
				}
				//plmn.mcc = atoi((const char*)agc_db_column_text(stmt, 2));
				//plmn.mnc = atoi((const char*)agc_db_column_text(stmt, 3));
				//plmn.idNssai = agc_db_column_int(stmt, 4);
				plmns.push_back(plmn);
				error = 0;
				break;
			}
			default:
				error = 1;
				break;
		}
	} while (rc == AGC_DB_ROW);

	if (error) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryBplmn idSigGw=%d [fail].\n", idSigGw);
	} else {
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "DbManager::QueryBplmn size=%d.\n", plmns.size());
	}

	agc_db_finalize(stmt);
	agc_db_close(db);
	return true;
}

bool DbManager::QueryPlmnNssai(uint8_t idBplmn, std::vector<stCfg_PLMN_NSSAI_DATA> &nnsais)
{
	agc_db_t *db = NULL;
	agc_db_stmt_t *stmt = NULL;
	int rc;
	int columns = 0;
	const char *text;	
	int error = 0;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryPlmnNssai open db [fail].\n");
		return false;
	}

	if (agc_db_prepare(db, "SELECT * FROM sig_plmn_nnsai_view WHERE idBplmn = ?;", -1, &stmt, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryPlmnNssai failed .\n");
		agc_db_close(db);
		return false;
	}	

	if (agc_db_bind_int(stmt, 1, idBplmn) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryPlmnNssai bind failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		return false;
	}

	do {
		rc = agc_db_step(stmt);
		switch(rc) {
			case AGC_DB_DONE:
				break;

			case AGC_DB_ROW:
			{
				stCfg_PLMN_NSSAI_DATA nnsai = {0};
				columns = agc_db_column_count(stmt);
			
				nnsai.idBplmn = agc_db_column_int(stmt, 0);
				nnsai.idNssai = agc_db_column_int(stmt, 1);
				nnsai.SST = agc_db_column_int(stmt, 2);
				nnsai.SD = agc_db_column_int(stmt, 3);
				nnsais.push_back(nnsai);
				break;
			}
			default:
				error = 1;
				break;
		}
	} while (rc == AGC_DB_ROW);

	if (error) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryPlmnNssai [fail].\n");
	} else {
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "DbManager::QueryPlmnNssai idBplmn=%d size=%d[ok].\n", idBplmn, nnsais.size());
	}

	agc_db_finalize(stmt);
	agc_db_close(db);
	return true;
}

bool DbManager::QueryPlmnNssai(uint8_t idSigGw, uint8_t idBplmn, std::vector<stCfg_PLMN_NSSAI_DATA> &nnsais)
{
	agc_db_t *db = NULL;
	agc_db_stmt_t *stmt = NULL;
	int rc;
	int columns = 0;
	const char *text;	
	int error = 0;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryPlmnNssai open db [fail].\n");
		return false;
	}

	if (agc_db_prepare(db, "SELECT * FROM sig_plmn_nnsai_view WHERE idBplmn = ? and idSigGW = ?;", -1, &stmt, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryPlmnNssai failed .\n");
		agc_db_close(db);
		return false;
	}	

	if (agc_db_bind_int(stmt, 1, idBplmn) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryPlmnNssai bind failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		return false;
	}

	if (agc_db_bind_int(stmt, 2, idSigGw) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryPlmnNssai bind failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		return false;
	}

	do {
		rc = agc_db_step(stmt);
		switch(rc) {
			case AGC_DB_DONE:
				break;

			case AGC_DB_ROW:
			{
				stCfg_PLMN_NSSAI_DATA nnsai = {0};
				columns = agc_db_column_count(stmt);
			
				nnsai.idBplmn = agc_db_column_int(stmt, 0);
				nnsai.idNssai = agc_db_column_int(stmt, 1);
				nnsai.SST = agc_db_column_int(stmt, 2);
				nnsai.SD = agc_db_column_int(stmt, 3);
				nnsais.push_back(nnsai);
				break;
			}
			default:
				error = 1;
				break;
		}
	} while (rc == AGC_DB_ROW);

	if (error) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryPlmnNssai [fail].\n");
	} else {
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "DbManager::QueryPlmnNssai idSigGw=%d idBplmn=%d size=%d[ok].\n", idSigGw, idBplmn, nnsais.size());
	}

	agc_db_finalize(stmt);
	agc_db_close(db);
	return true;
}

bool DbManager::QueryNssai(uint8_t idNssai, std::vector<stCfg_NSSAI_API> &nnsais)
{
	agc_db_t *db = NULL;
	agc_db_stmt_t *stmt = NULL;
	int rc;
	int columns = 0;
	const char *text;
	
	int error = 0;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryNssai open db [fail].\n");
		return false;
	}

	if (agc_db_prepare(db, "SELECT * FROM sig_nssai_table WHERE idNssai = ?;", -1, &stmt, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryNssai failed .\n");
		agc_db_close(db);
		return false;
	}	

	if (agc_db_bind_int(stmt, 1, idNssai) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryNssai bind failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		return false;
	}

	do {
		rc = agc_db_step(stmt);
		switch(rc) {
			case AGC_DB_DONE:
				break;

			case AGC_DB_ROW:
			{
				stCfg_NSSAI_API nnsai = {0};
				columns = agc_db_column_count(stmt);
			
				nnsai.idNssai = agc_db_column_int(stmt, 1);
				nnsai.SST = agc_db_column_int(stmt, 2);
				nnsai.SD = agc_db_column_int(stmt, 3);
				nnsais.push_back(nnsai);
				break;
			}
			default:
				error = 1;
				break;
		}
	} while (rc == AGC_DB_ROW);

	if (error) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryNssai [fail].\n");
	} else {
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "DbManager::QueryNssai [ok].\n");
	}

	agc_db_finalize(stmt);
	agc_db_close(db);
	return true;
}

bool DbManager::QueryTacs(uint8_t idSigGw, std::vector<stCfg_TA_DATA> &tacs)
{
	agc_db_t *db = NULL;
	agc_db_stmt_t *stmt = NULL;
	int rc;
	int columns = 0;
	const char *text;
	
	int error = 0;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryTacs open db [fail].\n");
		return false;
	}

	if (agc_db_prepare(db, "SELECT * FROM sig_tac_view WHERE idSigGW = ?;", -1, &stmt, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryTacs failed .\n");
		agc_db_close(db);
		return false;
	}	

	if (agc_db_bind_int(stmt, 1, idSigGw) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryTacs bind failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		return false;
	}

	do {
		rc = agc_db_step(stmt);
		switch(rc) {
			case AGC_DB_DONE:
				break;

			case AGC_DB_ROW:
			{
				stCfg_TA_DATA tac = {0};
				columns = agc_db_column_count(stmt);
			
				tac.idSigGW 	= agc_db_column_int(stmt, 0);
				tac.idTA 		= agc_db_column_int(stmt, 1);
				tac.TAC 		= agc_db_column_int(stmt, 2);
				tacs.push_back(tac);
				break;
			}
			default:
				error = 1;
				break;
		}
	} while (rc == AGC_DB_ROW);

	if (error) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryTacs [fail].\n");
	} else {
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "DbManager::QueryTacs [ok].\n");
	}

	agc_db_finalize(stmt);
	agc_db_close(db);
	return true;

}

bool DbManager::QueryTacPlmn(uint8_t idTA, std::vector<stCfg_TA_PLMN_DATA> &plmns)
{
	agc_db_t *db = NULL;
	agc_db_stmt_t *stmt = NULL;
	int rc;
	int columns = 0;
	const char *text;
	
	int error = 0;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryTacPlmn open db [fail].\n");
		return false;
	}

	if (agc_db_prepare(db, "SELECT * FROM sig_tac_plmn_view WHERE idTA = ?;", -1, &stmt, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryTacPlmn failed .\n");
		agc_db_close(db);
		return false;
	}	

	if (agc_db_bind_int(stmt, 1, idTA) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryTacPlmn bind failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		return false;
	}

	do {
		rc = agc_db_step(stmt);
		switch(rc) {
			case AGC_DB_DONE:
				break;

			case AGC_DB_ROW:
			{
				stCfg_TA_PLMN_DATA plmn = {0};
				columns = agc_db_column_count(stmt);
				
				plmn.idTA = agc_db_column_int(stmt, 0);
				plmn.idBplmn = agc_db_column_int(stmt, 1);
				text = (const char *)agc_db_column_text(stmt, 2);
				plmn.plmn.mcc1 = text[0] - '0';
				plmn.plmn.mcc2 = text[1] - '0';
				plmn.plmn.mcc3 = text[2] - '0';
				text = (const char *)agc_db_column_text(stmt, 3);
				if (strlen(text) == 2) {
					plmn.plmn.mnc1 = 0xF;
					plmn.plmn.mnc2 = text[0] - '0';
					plmn.plmn.mnc3 = text[1] - '0';
				}
				else {
					plmn.plmn.mnc1 = text[0] - '0';
					plmn.plmn.mnc2 = text[1] - '0';
					plmn.plmn.mnc3 = text[2] - '0';
				}
				//plmn.mcc = atoi((const char*)agc_db_column_text(stmt, 2));
				//plmn.mnc = atoi((const char*)agc_db_column_text(stmt, 3));
				plmns.push_back(plmn);
				break;
			}
			default:
				error = 1;
				break;
		}
	} while (rc == AGC_DB_ROW);

	if (error) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryTacPlmn [fail].\n");
	} else {
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "DbManager::QueryTacPlmn [ok].\n");
	}

	agc_db_finalize(stmt);
	agc_db_close(db);
	return true;
}

bool DbManager::QueryTacPlmn(uint8_t idSigGw, uint8_t idTA, std::vector<stCfg_TA_PLMN_DATA> &plmns)
{
	agc_db_t *db = NULL;
	agc_db_stmt_t *stmt = NULL;
	int rc;
	int columns = 0;
	const char *text;
	
	int error = 0;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryTacPlmn open db [fail].\n");
		return false;
	}

	if (agc_db_prepare(db, "SELECT * FROM sig_tac_plmn_view WHERE idSigGW = ? and idTA = ?;", -1, &stmt, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryTacPlmn failed .\n");
		agc_db_close(db);
		return false;
	}	

	if (agc_db_bind_int(stmt, 1, idSigGw) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryTacPlmn bind failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		return false;
	}
	
	if (agc_db_bind_int(stmt, 2, idTA) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryTacPlmn bind failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		return false;
	}

	do {
		rc = agc_db_step(stmt);
		switch(rc) {
			case AGC_DB_DONE:
				break;

			case AGC_DB_ROW:
			{
				stCfg_TA_PLMN_DATA plmn = {0};
				columns = agc_db_column_count(stmt);
				
				plmn.idTA = agc_db_column_int(stmt, 0);
				plmn.idBplmn = agc_db_column_int(stmt, 1);
				text = (const char *)agc_db_column_text(stmt, 2);
				plmn.plmn.mcc1 = text[0] - '0';
				plmn.plmn.mcc2 = text[1] - '0';
				plmn.plmn.mcc3 = text[2] - '0';
				text = (const char *)agc_db_column_text(stmt, 3);
				if (strlen(text) == 2) {
					plmn.plmn.mnc1 = 0xF;
					plmn.plmn.mnc2 = text[0] - '0';
					plmn.plmn.mnc3 = text[1] - '0';
				}
				else {
					plmn.plmn.mnc1 = text[0] - '0';
					plmn.plmn.mnc2 = text[1] - '0';
					plmn.plmn.mnc3 = text[2] - '0';
				}
				//plmn.mcc = atoi((const char*)agc_db_column_text(stmt, 2));
				//plmn.mnc = atoi((const char*)agc_db_column_text(stmt, 3));
				plmns.push_back(plmn);
				break;
			}
			default:
				error = 1;
				break;
		}
	} while (rc == AGC_DB_ROW);

	if (error) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryTacPlmn [fail].\n");
	} else {
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "DbManager::QueryTacPlmn idSigGw=%d or idTA=%d [ok].\n", idSigGw, idTA);
	}

	agc_db_finalize(stmt);
	agc_db_close(db);
	return true;
}

bool DbManager::QueryGuami(uint8_t idSigGw, std::vector<stCfg_GUAMI_API> &guamis)
{
	agc_db_t *db = NULL;
	agc_db_stmt_t *stmt = NULL;
	int rc;
	int columns = 0;
	const char *text;
	
	int error = 0;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryGuami open db [fail].\n");
		return false;
	}

	if (agc_db_prepare(db, "SELECT * FROM sig_guami_table WHERE idSigGW = ?;", -1, &stmt, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryGuami failed .\n");
		agc_db_close(db);
		return false;
	}	

	if (agc_db_bind_int(stmt, 1, idSigGw) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryGuami bind failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		return false;
	}

	do {
		rc = agc_db_step(stmt);
		switch(rc) {
			case AGC_DB_DONE:
				break;

			case AGC_DB_ROW:
			{
				stCfg_GUAMI_API guami = {0};
				columns = agc_db_column_count(stmt);
				
				guami.idSigGW = agc_db_column_int(stmt, 1);
				guami.idGuami = agc_db_column_int(stmt, 2);
				text = (const char *)agc_db_column_text(stmt, 3);
				guami.plmn.mcc1 = text[0] - '0';
				guami.plmn.mcc2 = text[1] - '0';
				guami.plmn.mcc3 = text[2] - '0';
				text = (const char *)agc_db_column_text(stmt, 4);
				if (strlen(text) == 2) {
					guami.plmn.mnc1 = 0xF;
					guami.plmn.mnc2 = text[0] - '0';
					guami.plmn.mnc3 = text[1] - '0';
				}
				else {
					guami.plmn.mnc1 = text[0] - '0';
					guami.plmn.mnc2 = text[1] - '0';
					guami.plmn.mnc3 = text[2] - '0';
				}
				//guami.mcc = atoi((const char*)agc_db_column_text(stmt, 3));
				//guami.mnc = atoi((const char*)agc_db_column_text(stmt, 4));
				guami.AMFRegionID = agc_db_column_int(stmt, 5);
				guami.AMFSetId = agc_db_column_int(stmt, 6);
				guami.AMFPointer = agc_db_column_int(stmt, 7);
				guamis.push_back(guami);
				break;
			}
			default:
				error = 1;
				break;
		}
	} while (rc == AGC_DB_ROW);

	if (error) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QueryGuami [fail].\n");
	} else {
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "DbManager::QueryGuami [ok].\n");
	}

	agc_db_finalize(stmt);
	agc_db_close(db);
	return true;
}

bool DbManager::InsertSctpLinks(agc_db_stmt_t *stmt, std::vector<Cfg_LteSctp *> &links)
{
	int columns = 0;
	const char *local_addr1;
	const char *local_addr2;
	const char *remote_addr1;
	const char *remote_addr2;
	Cfg_IPType iptype;

	if (stmt == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::InsertSctpLinks stmt is NULL.\n");
		return false;
	}

	Cfg_LteSctp *link = new Cfg_LteSctp;
	columns = agc_db_column_count(stmt);
	
	link->idSctpIndex = agc_db_column_int(stmt, 1);
	link->idSigGW = agc_db_column_int(stmt, 2);
	link->Priority = agc_db_column_int(stmt, 3);
	link->WorkType = (Cfg_SctpWorkType)agc_db_column_int(stmt, 4);
	iptype = (Cfg_IPType)agc_db_column_int(stmt, 5);
	local_addr1 = (const char *)agc_db_column_text(stmt, 6);
	local_addr2 = (const char *)agc_db_column_text(stmt, 7);

	link->LocalPort = agc_db_column_int(stmt, 8);

	remote_addr1 = (const char *)agc_db_column_text(stmt, 9);
	remote_addr2 = (const char *)agc_db_column_text(stmt, 10);

	link->PeerPort = agc_db_column_int(stmt, 11);
	link->HeartBeatTime_s = agc_db_column_int(stmt, 12);
	link->SctpNum = agc_db_column_int(stmt, 13);
	link->iptype = iptype;
	link->LocalIP1Len = 0;
	link->LocalIP2Len = 0;
	link->PeerIP1Len = 0;
	link->PeerIP2Len = 0;

	// ipv4
	if (iptype == IPv4)
	{
		if (local_addr1 != NULL && strlen(local_addr1) > 0) {
			struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&link->LocalIP1;
			local_addr1_v4->sin_family = AF_INET;
			local_addr1_v4->sin_port  = htons(link->LocalPort);
			inet_pton(AF_INET, local_addr1, &(local_addr1_v4->sin_addr));
			link->LocalIP1Len = sizeof(struct sockaddr_in);
		}

		if (local_addr2 != NULL && strlen(local_addr2) > 0) {
			struct sockaddr_in *local_addr2_v4 = (struct sockaddr_in *)&link->LocalIP2;
			local_addr2_v4->sin_family = AF_INET;
			local_addr2_v4->sin_port  = htons(link->LocalPort);
			inet_pton(AF_INET, local_addr2, &(local_addr2_v4->sin_addr));
			link->LocalIP2Len = sizeof(struct sockaddr_in);
		}

		if (remote_addr1 != NULL && strlen(remote_addr1) > 0) {
			struct sockaddr_in *remote_addr1_v4 = (struct sockaddr_in *)&link->PeerIP1;
			remote_addr1_v4->sin_family = AF_INET;
			remote_addr1_v4->sin_port  = htons(link->PeerPort);
			inet_pton(AF_INET, remote_addr1, &(remote_addr1_v4->sin_addr));
			link->PeerIP1Len = sizeof(struct sockaddr_in);
		}

		if (remote_addr2 != NULL && strlen(remote_addr2) > 0) {
			struct sockaddr_in *remote_addr2_v4 = (struct sockaddr_in *)&link->PeerIP2;
			remote_addr2_v4->sin_family = AF_INET;
			remote_addr2_v4->sin_port = htons(link->PeerPort);
			inet_pton(AF_INET, remote_addr2, &(remote_addr2_v4->sin_addr));
			link->PeerIP2Len = sizeof(struct sockaddr_in);
		}
	}
	else
	{
		if (local_addr1 != NULL && strlen(local_addr1) > 0) {
			struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&link->LocalIP1;
			local_addr1_v6->sin6_family = AF_INET6;
			local_addr1_v6->sin6_port  = htons(link->LocalPort);
			inet_pton(AF_INET6, local_addr1, &(local_addr1_v6->sin6_addr));
			link->LocalIP1Len = sizeof(struct sockaddr_in6);
		}
		if (local_addr2 != NULL && strlen(local_addr2) > 0) {
			struct sockaddr_in6 *local_addr2_v6 = (struct sockaddr_in6 *)&link->LocalIP2;
			local_addr2_v6->sin6_family = AF_INET6;
			local_addr2_v6->sin6_port  = htons(link->LocalPort);
			inet_pton(AF_INET6, local_addr2, &(local_addr2_v6->sin6_addr));
			link->LocalIP2Len = sizeof(struct sockaddr_in6);
		}

		if (remote_addr1 != NULL && strlen(remote_addr1) > 0) {
			struct sockaddr_in6 *remote_addr1_v6 = (struct sockaddr_in6 *)&link->PeerIP1;
			remote_addr1_v6->sin6_family = AF_INET6;
			remote_addr1_v6->sin6_port  = htons(link->PeerPort);
			inet_pton(AF_INET6, local_addr1, &(remote_addr1_v6->sin6_addr));
			link->PeerIP1Len = sizeof(struct sockaddr_in6);
		}

		if (remote_addr2 != NULL && strlen(remote_addr2) > 0) {
			struct sockaddr_in6 *remote_addr2_v6 = (struct sockaddr_in6 *)&link->PeerIP2;
			remote_addr2_v6->sin6_family = AF_INET6;
			remote_addr2_v6->sin6_port  = htons(link->PeerPort);
			inet_pton(AF_INET6, remote_addr2, &(remote_addr2_v6->sin6_addr));
			link->PeerIP2Len = sizeof(struct sockaddr_in6);
		}
		
	}

	links.push_back(link);
	return true;
}

bool DbManager::QuerySctpLinks(std::vector<Cfg_LteSctp *> &links)
{
	agc_db_t *db = NULL;
	agc_db_stmt_t *stmt = NULL;
	int rc;
	int error = 0;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QuerySctpLinksByRemoteAddr open db [fail].\n");
		return false;
	}

	if (agc_db_prepare(db, "SELECT * FROM sig_sctp_table ;", -1, &stmt, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QuerySctpLinksByRemoteAddr failed .\n");
		agc_db_close(db);
		return false;
	}	

	do {
		rc = agc_db_step(stmt);
		switch(rc) {
			case AGC_DB_DONE:
				break;

			case AGC_DB_ROW:
				InsertSctpLinks(stmt, links);
				break;
			default:
				error = 1;
				break;
		}
	} while (rc == AGC_DB_ROW);

	if (error) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QuerySctpLinksByRemoteAddr [fail].\n");
	} else {
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "DbManager::QuerySctpLinksByRemoteAddr size=%d [ok].\n", links.size());
	}

	agc_db_finalize(stmt);
	agc_db_close(db);

	return true;
}


bool DbManager::QuerySctpServerLinks(std::vector<Cfg_LteSctp *> &links)
{
	agc_db_t *db = NULL;
	agc_db_stmt_t *stmt = NULL;
	int rc;
	int error = 0;
	const char *local_addr1;
	const char *local_addr2;
	const char *remote_addr1;
	const char *remote_addr2;
	Cfg_IPType iptype;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QuerySctpServerLinks open db [fail].\n");
		return false;
	}

	if (agc_db_prepare(db, "SELECT DISTINCT iptype, LocalIP1, LocalIP2, LocalPort, HeartBeatTime_s, SctpNum from sig_sctp_table WHERE WorkType=0;", -1, &stmt, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QuerySctpServerLinks failed .\n");
		agc_db_close(db);
		return false;
	}	

	do {
		Cfg_LteSctp *link = new Cfg_LteSctp;
		const char *value = NULL;
		rc = agc_db_step(stmt);
		switch(rc) {
			case AGC_DB_DONE:
				break;

			case AGC_DB_ROW:
				iptype = (Cfg_IPType)agc_db_column_int(stmt, 0);
				local_addr1 = (const char *)agc_db_column_text(stmt, 1);
				local_addr2 = (const char *)agc_db_column_text(stmt, 2);
				link->LocalPort = agc_db_column_int(stmt, 3);
				
				// ipv4
				if (iptype == IPv4)
				{
					if (local_addr1 != NULL && strlen(local_addr1) > 0) {
						struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&link->LocalIP1;
						local_addr1_v4->sin_family = AF_INET;
						local_addr1_v4->sin_port  = htons(link->LocalPort);
						inet_pton(AF_INET, local_addr1, &(local_addr1_v4->sin_addr));
						link->LocalIP1Len = sizeof(struct sockaddr_in);
					}
				
					if (local_addr2 != NULL && strlen(local_addr2) > 0) {
						struct sockaddr_in *local_addr2_v4 = (struct sockaddr_in *)&link->LocalIP2;
						local_addr2_v4->sin_family = AF_INET;
						local_addr2_v4->sin_port  = htons(link->LocalPort);
						inet_pton(AF_INET, local_addr2, &(local_addr2_v4->sin_addr));
						link->LocalIP2Len = sizeof(struct sockaddr_in);
					}
				
				}
				else
				{
					if (local_addr1 != NULL && strlen(local_addr1) > 0) {
						struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&link->LocalIP1;
						local_addr1_v6->sin6_family = AF_INET6;
						local_addr1_v6->sin6_port  = htons(link->LocalPort);
						inet_pton(AF_INET6, local_addr1, &(local_addr1_v6->sin6_addr));
						link->LocalIP1Len = sizeof(struct sockaddr_in6);
					}
					if (local_addr2 != NULL && strlen(local_addr2) > 0) {
						struct sockaddr_in6 *local_addr2_v6 = (struct sockaddr_in6 *)&link->LocalIP2;
						local_addr2_v6->sin6_family = AF_INET6;
						local_addr2_v6->sin6_port  = htons(link->LocalPort);
						inet_pton(AF_INET6, local_addr2, &(local_addr2_v6->sin6_addr));
						link->LocalIP2Len = sizeof(struct sockaddr_in6);
					}
					
				}

				link->HeartBeatTime_s = agc_db_column_int(stmt, 4);
				link->SctpNum = agc_db_column_int(stmt, 5);
				links.push_back(link);
				break;
			default:
				error = 1;
				break;
		}
	} while (rc == AGC_DB_ROW);

	if (error) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QuerySctpServerLinks [fail].\n");
	} else {
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "DbManager::QuerySctpServerLinks size=%d [ok].\n", links.size());
	}

	agc_db_finalize(stmt);
	agc_db_close(db);

	return true;
}


bool DbManager::QuerySctpLinksByRemoteAddr(agc_std_sockaddr_t &remote_addr, uint32_t &sctp_index, uint32_t &iSigGW, uint32_t &sctp_num)
{
	agc_db_t *db = NULL;
	agc_db_stmt_t *stmt = NULL;
	int rc;
	char str_addr[64];
	int error = 0;
	uint16_t port = 0;
	int columns = 0;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QuerySctpLinksByRemoteAddr open db [fail].\n");
		return false;
	}

	if (agc_db_prepare(db, "SELECT * FROM sig_sctp_table WHERE (PeerIP1 = ? or PeerIP2 = ?) and PeerPort = ?;", -1, &stmt, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QuerySctpLinksByRemoteAddr failed .\n");
		agc_db_close(db);
		return false;
	}	

	if (remote_addr.ss_family == AF_INET) {
		struct sockaddr_in *addr4 = (struct sockaddr_in *)&remote_addr;
		inet_ntop(AF_INET, &(addr4->sin_addr), str_addr, 64);

		port = ntohs(addr4->sin_port);
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "DbManager::QuerySctpLinksByRemoteAddr addr4 %s .\n", str_addr);
	} else {
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&remote_addr;
		char str_addr[64];
		inet_ntop(AF_INET6, &(addr6->sin6_addr), str_addr, 64);
		port = ntohs(addr6->sin6_port);
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "DbManager::QuerySctpLinksByRemoteAddr addr6 %s .\n", str_addr);
	}

	if (agc_db_bind_text(stmt, 1, str_addr, strlen(str_addr), AGC_CORE_DB_TRANSIENT) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QuerySctpLinksByRemoteAddr bind failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		return false;
	}

	if (agc_db_bind_text(stmt, 2, str_addr, strlen(str_addr), AGC_CORE_DB_TRANSIENT) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QuerySctpLinksByRemoteAddr bind failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		return false;
	}

	if (agc_db_bind_int(stmt, 3, port) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QuerySctpLinksByRemoteAddr bind failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		return false;
	}
	
	do {
		rc = agc_db_step(stmt);
		switch(rc) {
			case AGC_DB_DONE:
				break;

			case AGC_DB_ROW:
				columns = agc_db_column_count(stmt);
				sctp_index = agc_db_column_int(stmt, 1);
				iSigGW = agc_db_column_int(stmt, 2);
				sctp_num =  agc_db_column_int(stmt, 13);
				break;
			default:
				error = 1;
				break;
		}
	} while (rc == AGC_DB_ROW);

	if (error) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QuerySctpLinksByRemoteAddr [fail].\n");
	} else {
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "DbManager::QuerySctpLinksByRemoteAddr sctp_index=%d iSigGW=%d sctp_num=%d.\n", sctp_index, iSigGW, sctp_num);
	}

	agc_db_finalize(stmt);
	agc_db_close(db);

	return true;
}
/*
bool DbManager::QuerySctpLinksByRemoteAddr(agc_std_sockaddr_t &remote_addr, std::vector<Cfg_LteSctp *> &links)
{
	agc_db_t *db = NULL;
	agc_db_stmt_t *stmt = NULL;
	int rc;
	char str_addr[64];
	int error = 0;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QuerySctpLinksByRemoteAddr open db [fail].\n");
		return false;
	}

	if (agc_db_prepare(db, "SELECT * FROM sig_sctp_table WHERE PeerIP1 = ? or PeerIP2 = ?;", -1, &stmt, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QuerySctpLinksByRemoteAddr failed .\n");
		agc_db_close(db);
		return false;
	}	

	if (remote_addr.ss_family == AF_INET) {
		struct sockaddr_in *addr4 = (struct sockaddr_in *)&remote_addr;
		inet_ntop(AF_INET, &(addr4->sin_addr), str_addr, 64);
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "DbManager::QuerySctpLinksByRemoteAddr addr4 %s .\n", str_addr);
	} else {
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&remote_addr;
		char str_addr[64];
		inet_ntop(AF_INET6, &(addr6->sin6_addr), str_addr, 64);
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "DbManager::QuerySctpLinksByRemoteAddr addr6 %s .\n", str_addr);
	}

	if (agc_db_bind_text(stmt, 1, str_addr, strlen(str_addr), AGC_CORE_DB_TRANSIENT) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QuerySctpLinksByRemoteAddr bind failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		return false;
	}

	if (agc_db_bind_text(stmt, 2, str_addr, strlen(str_addr), AGC_CORE_DB_TRANSIENT) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QuerySctpLinksByRemoteAddr bind failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		return false;
	}

	do {
		rc = agc_db_step(stmt);
		switch(rc) {
			case AGC_DB_DONE:
				break;

			case AGC_DB_ROW:
				InsertSctpLinks(stmt, links);
				break;
			default:
				error = 1;
				break;
		}
	} while (rc == AGC_DB_ROW);

	if (error) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QuerySctpLinksByRemoteAddr [fail].\n");
	} else {
		agc_log_printf(AGC_LOG, AGC_LOG_INFO, "DbManager::QuerySctpLinksByRemoteAddr [ok].\n");
	}

	agc_db_finalize(stmt);
	agc_db_close(db);

	return true;
}
*/
bool DbManager::QuerySctpLinksByLocalAddr(agc_std_sockaddr_t &local_addr, std::vector<Cfg_LteSctp *> &links)
{
	return true;
}

bool DbManager::QuerySctpLinksByidSigGw(uint8_t idSigGw, std::vector<Cfg_LteSctp *> &links)
{
	agc_db_t *db = NULL;
	agc_db_stmt_t *stmt = NULL;
	int rc;
	int error = 0;

	if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QuerySctpLinksByidSigGw open db [fail].\n");
		return false;
	}

	if (agc_db_prepare(db, "SELECT * FROM sig_sctp_table WHERE idSigGW = ?;", -1, &stmt, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QuerySctpLinksByidSigGw failed .\n");
		agc_db_close(db);
		return false;
	}

	if (agc_db_bind_int(stmt, 1, idSigGw) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QuerySctpLinksByidSigGw bind failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		return false;
	}

	do {
		rc = agc_db_step(stmt);
		switch(rc) {
			case AGC_DB_DONE:
				break;

			case AGC_DB_ROW:
				InsertSctpLinks(stmt, links);
				break;
			default:
				error = 1;
				break;
		}
	} while (rc == AGC_DB_ROW);

	if (error) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QuerySctpLinksByidSigGw [fail].\n");
	} else {
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "DbManager::QuerySctpLinksByidSigGw [ok].\n");
	}

	agc_db_finalize(stmt);
	agc_db_close(db);

	return true;
}

bool DbManager::QuerySctpLinkById(uint32_t Id, vector<Cfg_LteSctp *> &links)
{
    agc_db_t *db = NULL;
    agc_db_stmt_t *stmt = NULL;
    int rc;
    int error = 0;

    if (!(db = agc_db_open_file(SIG_DB_FILENAME)))  {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QuerySctpLinkById open db [fail].\n");
        return false;
    }

    if (agc_db_prepare(db, "SELECT * FROM sig_sctp_table WHERE ID = ?;", -1, &stmt, NULL) != AGC_DB_OK) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QuerySctpLinkById failed .\n");
        agc_db_close(db);
        return false;
    }

    if (agc_db_bind_int(stmt, 1, Id) != AGC_DB_OK) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QuerySctpLinkById bind failed .\n");
        agc_db_finalize(stmt);
        agc_db_close(db);
        return false;
    }

    do {
        rc = agc_db_step(stmt);
        switch(rc) {
            case AGC_DB_DONE:
                break;
            case AGC_DB_ROW:
                InsertSctpLinks(stmt, links);
                break;
            default:
                error = 1;
                break;
        }
    } while (rc == AGC_DB_ROW);

    agc_db_finalize(stmt);
    agc_db_close(db);

    if (error) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "DbManager::QuerySctpLinkById [fail].\n");
        return false;
    } else {
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "DbManager::QuerySctpLinkById [ok].\n");
        return true;
    }
}

void DbManager::StringToIpAddr(const char* addr, Cfg_IPType iptype, uint16_t LocalPort, agc_std_sockaddr_t &IP, socklen_t &IPLen)
{
    if (iptype == IPv4)
    {
        if (addr != NULL && strlen(addr) > 0) {
            struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&IP;
            local_addr1_v4->sin_family = AF_INET;
            local_addr1_v4->sin_port  = htons(LocalPort);
            inet_pton(AF_INET, addr, &(local_addr1_v4->sin_addr));
            IPLen = sizeof(struct sockaddr_in);
        }
    }
    else
    {
        if (addr != NULL && strlen(addr) > 0) {
            struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&IP;
            local_addr1_v6->sin6_family = AF_INET6;
            local_addr1_v6->sin6_port  = htons(LocalPort);
            inet_pton(AF_INET6, addr, &(local_addr1_v6->sin6_addr));
            IPLen = sizeof(struct sockaddr_in6);
        }
    }
}