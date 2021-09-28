#ifndef MOD_SIGAPI_H
#define MOD_SIGAPI_H

#include "mod_sig.h"
#include <agc.h>
#include <CfgStruct.h>
#include "NsmManager.h"
#include "SctpLayer.h"

#define SIGGW_NAME_MAX_LEN 255
#define SIGGW_IPADDR_MAX_LEN 63
#define SIGGW_MEDIAGW_MAX_LEN 63
#define SIGGW_MCCMNC_MAX_LEN 3

agc_status_t sig_api_add_nssai(agc_stream_handle_t *stream, uint8_t idNssai, uint8_t SST, uint32_t SD);
agc_status_t sig_api_del_nssai(agc_stream_handle_t *stream, uint32_t ID);
agc_status_t sig_api_update_nssai(agc_stream_handle_t *stream, uint32_t ID, uint8_t idNssai, uint8_t SST, uint32_t SD);

agc_status_t nssai_insert(agc_stream_handle_t *stream, uint8_t idNssai, uint8_t SST, uint32_t SD);
agc_status_t nssai_delete(agc_stream_handle_t *stream, uint32_t ID);
agc_status_t nssai_update(agc_stream_handle_t *stream, uint32_t ID, uint8_t idNssai, uint8_t SST, uint32_t SD);
agc_status_t nssai_queryN(agc_stream_handle_t *stream, char **body, int page_index, int page_size);

agc_status_t vgw_insert(agc_stream_handle_t *stream, int argc, char **argv);
agc_status_t vgw_delete(agc_stream_handle_t *stream, int argc, char **argv);
agc_status_t vgw_update(agc_stream_handle_t *stream, int argc, char **argv);
agc_status_t vgw_queryN(agc_stream_handle_t *stream, char **body, int page_index, int page_size);



agc_status_t taplmn_insert(agc_stream_handle_t *stream, int argc, char **argv);
agc_status_t taplmn_delete(agc_stream_handle_t *stream, int argc, char **argv);
agc_status_t taplmn_update(agc_stream_handle_t *stream, int argc, char **argv);
agc_status_t taplmn_queryN(agc_stream_handle_t *stream, char **body, int page_index, int page_size);



agc_status_t guami_insert(agc_stream_handle_t *stream, int argc, char **argv);
agc_status_t guami_delete(agc_stream_handle_t *stream, int argc, char **argv);
agc_status_t guami_update(agc_stream_handle_t *stream, int argc, char **argv);
agc_status_t guami_queryN(agc_stream_handle_t *stream, char **body, int page_index, int page_size);


agc_status_t bplmn_insert(agc_stream_handle_t *stream, int argc, char **argv);
agc_status_t bplmn_delete(agc_stream_handle_t *stream, int argc, char **argv);
agc_status_t bplmn_update(agc_stream_handle_t *stream, int argc, char **argv);
agc_status_t bplmn_queryN(agc_stream_handle_t *stream, char **body, int page_index, int page_size);


agc_status_t datagw_insert(agc_stream_handle_t *stream, int argc, char **argv);
agc_status_t datagw_delete(agc_stream_handle_t *stream, int argc, char **argv);
agc_status_t datagw_update(agc_stream_handle_t *stream, int argc, char **argv);
agc_status_t datagw_queryN(agc_stream_handle_t *stream, char **body, int page_index, int page_size);


agc_status_t sctp_insert(agc_stream_handle_t *stream, int argc, char **argv, enum SCTPpeerpoint eSctpSide);
agc_status_t sctp_delete(agc_stream_handle_t *stream, int argc, char **argv, enum SCTPpeerpoint eSctpSide);
agc_status_t sctp_update(agc_stream_handle_t *stream, int argc, char **argv, enum SCTPpeerpoint eSctpSide);
agc_status_t sctp_queryN(agc_stream_handle_t *stream, char **body, int page_index, int page_size, enum SCTPpeerpoint eSctpSide);
agc_status_t sctp_statusN(agc_stream_handle_t *stream, char **body, int page_index, int page_size, enum SCTPpeerpoint eSctpSide);


/*
agc_status_t sig_api_query_nssai(agc_stream_handle_t *stream);

agc_status_t sig_api_add_amfgw(agc_stream_handle_t *stream, uint8_t idSigGW, char *AMFName, uint8_t AMFCapacity);
agc_status_t amfgw_insert(agc_stream_handle_t *stream, uint8_t idSigGW, char *AMFName, uint8_t AMFCapacity);
agc_status_t sig_api_del_amfgw(agc_stream_handle_t *stream, uint8_t idSigGW);
agc_status_t amfgw_delete(agc_stream_handle_t *stream, uint8_t idSigGW);
agc_status_t sig_api_update_amfgw(agc_stream_handle_t *stream, uint8_t idSigGW, char *AMFName, uint8_t AMFCapacity);
agc_status_t amfgw_update(agc_stream_handle_t *stream, uint8_t idSigGW, char *AMFName, uint8_t AMFCapacity);
agc_status_t sig_api_query_amfgw(agc_stream_handle_t *stream);


agc_status_t sigApi_main_real(const char *cmd, agc_stream_handle_t *stream);
agc_status_t nssai_query(agc_stream_handle_t *stream, char **body);
agc_status_t sig_api_add_bplmn(agc_stream_handle_t *stream, uint8_t idBplmn, uint16_t MCC, uint16_t MNC, uint8_t idNssai);
agc_status_t sig_api_del_bplmn(agc_stream_handle_t *stream, uint8_t idBplmn);
agc_status_t sig_api_update_bplmn(agc_stream_handle_t *stream, uint8_t idBplmn, uint16_t MCC, uint16_t MNC, uint8_t idNssai);
agc_status_t sig_api_query_bplmn(agc_stream_handle_t *stream, uint8_t idBplmn, uint16_t MCC, uint16_t MNC, uint8_t idNssai);

agc_status_t sig_api_add_guami(agc_stream_handle_t *stream, uint8_t idAMF, uint16_t MCC, uint16_t MNC, uint8_t AMFRegionID, uint16_t AMFSetId, uint8_t AMFPointer);
agc_status_t sig_api_del_guami(agc_stream_handle_t *stream, uint8_t idAMF);
agc_status_t sig_api_update_guami(agc_stream_handle_t *stream, uint8_t idAMF, uint16_t MCC, uint16_t MNC, uint8_t AMFRegionID, uint16_t AMFSetId, uint8_t AMFPointer);
agc_status_t sig_api_query_guami(agc_stream_handle_t *stream, uint8_t idAMF);


agc_status_t sig_api_add_siggwamf(agc_stream_handle_t *stream, uint8_t idSigGW, uint8_t idAMF);
agc_status_t sig_api_del_siggwamf(agc_stream_handle_t *stream, uint8_t idSigGW, uint8_t idAMF);
agc_status_t sig_api_query_siggwamf(agc_stream_handle_t *stream);

agc_status_t sig_api_add_siggwplmn(agc_stream_handle_t *stream, uint8_t idSigGW, uint8_t idBplmn);
agc_status_t sig_api_del_siggwplmn(agc_stream_handle_t *stream, uint8_t idSigGW, uint8_t idBplmn);
agc_status_t sig_api_query_siggwplmn(agc_stream_handle_t *stream);

agc_status_t sig_api_del_vgw(agc_stream_handle_t *stream, uint8_t idSigGW);
agc_status_t sig_api_update_vgw(agc_stream_handle_t *stream, uint8_t idSigGW, char *AMFName, uint8_t AMFCapacity, uint32_t gNBid, char *gNBName, uint16_t MCC, uint16_t MNC, enum Cfg_PagingDRX DRX, enum Cfg_GNBType gNBType, enum Cfg_NG_ENBType ngeNBType);
agc_status_t sig_api_query_vgw(agc_stream_handle_t *stream);


agc_status_t sig_api_add_gwta(agc_stream_handle_t *stream, uint8_t idSigGW, uint8_t idTA);
agc_status_t sig_api_del_gwta(agc_stream_handle_t *stream, uint8_t idSigGW, uint8_t idTA);
agc_status_t sig_api_query_gwta(agc_stream_handle_t *stream);

agc_status_t sig_api_add_taplmn(agc_stream_handle_t *stream, uint8_t idTA, uint32_t TAC, uint8_t idBplmn);
agc_status_t sig_api_del_taplmn(agc_stream_handle_t *stream, uint8_t idTA, uint8_t idBplmn);
agc_status_t sig_api_query_taplmn(agc_stream_handle_t *stream);

*/


#endif