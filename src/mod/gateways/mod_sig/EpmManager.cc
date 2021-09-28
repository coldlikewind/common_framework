#include "NsmManager.h"
#include "EsmManager.h"
#include "EpmManager.h"
#include "ApmManager.h"
#include "AsmManager.h"
#include "NgapSession.h"
#include "NgapSessManager.h"
#include "PduSessManager.h"
#include "PlmnUtil.h"
#include "DbManager.h"
#include <agc.h>
#include <CfgStruct.h>

using namespace std;
vector<stSIG_TAC_RAN> global_tac_ran;

#define EPM_REGISTER_APPCALLBACK(pduChoise, procedureCode, func)                     \
	{                                                                                \
		GetNsmManager().RegisterDownLayerRecv((pduChoise), (procedureCode), (func)); \
	}

static bool GNB_ConvertNgapId(NgapMessage &info, Ngap_RAN_UE_NGAP_ID_t *ranUeId, Ngap_AMF_UE_NGAP_ID_t *amfUeId)
{
	if(NULL == ranUeId)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_ConvertNgapId ranUeId addr = (%x)\n",ranUeId);
		return false;
	}

	info.gnbUeId = *ranUeId;
	//info.sessId = (uint32_t)U64FromBuffer(amfUeId->buf, amfUeId->size);
	uintmax_t ueid;
	asn_INTEGER2umax(amfUeId, &ueid);
	info.sessId = ueid;

	NgapSession *sess = NULL;
	if (GetNgapSessManager().GetNgapSession(info.sessId, &sess) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_ConvertNgapId getNgapSession fail to get ngap session, sessId=%d ranUeId=%d.\n", info.sessId, *ranUeId);
		return false;
	}

	*ranUeId = sess->sessId;
	asn_int642INTEGER(amfUeId, (uint64_t)sess->amfUeId);
	info.amfUeId = sess->amfUeId;
	info.stream_no = sess->stream_no;
	info.sockTarget = GetAsmManager().GetTargetSock(sess->amfId);

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "GNB_ConvertNgapId sessId=%d SessRanUeID=%d, SessAmfUeID=%lld sockTarget=%d amfUeId=%lld\n",
				   info.sessId, sess->gnbUeId, sess->amfUeId, info.sockTarget, info.amfUeId);
	return true;
}

int32_t HandlePlmn(Ngap_PLMNIdentity_t *PLMNidentity, EsmContext *newContext)
{
	plmn_id_t plmn_id;

	std::vector<stCfg_BPlmn_API> plmns;
	stCfg_vGWParam_API param;

	uint8_t idSigGW = newContext->idSigGW;

	if (GetDbManager().QueryVgwParam(idSigGW, param) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "HandlePlmn fail to QueryVgwParam gatewayid=%d\n",
					   idSigGW);
		return NGAP_CHECK_RNI_PLMN_ERROR;
	}
	if (GetDbManager().QueryBplmn(idSigGW, plmns) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "HandlePlmn fail to get plmn gatewayid=%d\n",
					   idSigGW);
		return NGAP_CHECK_RNI_PLMN_ERROR;
	}

	if (PLMNidentity == NULL)
		return NGAP_CHECK_RNI_PLMN_ERROR;

	if (PLMNidentity->size != PLMN_ID_LEN)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "HandlePlmn PLMN invalid len:%d\n", PLMNidentity->size);
		return NGAP_CHECK_RNI_PLMN_ERROR;
	}

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "PLMN check mcc:%d%d%d, mnc=%d%d%d, plmn-buf:0x%x%x%x\n",
				   param.Plmn.mcc1, param.Plmn.mcc2, param.Plmn.mcc3,
				   param.Plmn.mnc1, param.Plmn.mnc2, param.Plmn.mnc3,
				   PLMNidentity->buf[0], PLMNidentity->buf[1], PLMNidentity->buf[2]);
	memcpy(&plmn_id, PLMNidentity->buf, PLMN_ID_LEN);

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "PLMN plmn_id mcc:%d%d%d, mnc=%d%d%d\n",
				   plmn_id.mcc1, plmn_id.mcc2, plmn_id.mcc3,
				   plmn_id.mnc1, plmn_id.mnc2, plmn_id.mnc3);
	if (plmn_id.mcc1 != param.Plmn.mcc1 || plmn_id.mcc2 != param.Plmn.mcc2 || plmn_id.mcc3 != param.Plmn.mcc3 || plmn_id.mnc1 != param.Plmn.mnc1 || plmn_id.mnc2 != param.Plmn.mnc2 || plmn_id.mnc3 != param.Plmn.mnc3)
	{
		bool found = false;

		for (uint32_t i = 0; i < plmns.size(); i++)
		{
			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "PLMN check mcc:%d%d%d, mnc=%d%d%d\n",
						   plmns[i].plmn.mcc1, plmns[i].plmn.mcc2, plmns[i].plmn.mcc3,
						   plmns[i].plmn.mnc1, plmns[i].plmn.mnc2, plmns[i].plmn.mnc3);

			if (plmn_id.mcc1 == plmns[i].plmn.mcc1 && plmn_id.mcc2 == plmns[i].plmn.mcc2 && plmn_id.mcc3 == plmns[i].plmn.mcc3 && plmn_id.mnc1 == plmns[i].plmn.mnc1 && plmn_id.mnc2 == plmns[i].plmn.mnc2 && plmn_id.mnc3 == plmns[i].plmn.mnc3)
			{
				found = true;
				break;
			}
		}

		if (found == false)
		{
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "HandlePlmn PLMN invalid\n");
			return NGAP_CHECK_RNI_PLMN_ERROR;
		}
	}

	newContext->ranNodeId.setRanNodePlmn(plmn_id);

	return NGAP_CHECK_RNI_OK;
}

int32_t HandleGlobalRANNodeID(Ngap_GlobalRANNodeID_t *GlobalRANNodeID, EsmContext *newContext)
{
	uint32_t cause = NGAP_CHECK_RNI_UNKNOW_ERROR;
	uint32_t gid = 0;
	if (GlobalRANNodeID == NULL)
		return cause;

	switch (GlobalRANNodeID->present)
	{
	case Ngap_GlobalRANNodeID_PR_globalGNB_ID:
	{
		Ngap_GlobalGNB_ID_t *globalGNB_ID = GlobalRANNodeID->choice.globalGNB_ID;
		cause = HandlePlmn(&globalGNB_ID->pLMNIdentity, newContext);
		if (cause != NGAP_CHECK_RNI_OK)
		{
			return cause;
		}

		if (globalGNB_ID->gNB_ID.present != Ngap_GNB_ID_PR_gNB_ID)
		{
			agc_log_printf(AGC_LOG, AGC_LOG_WARNING, "HandleGlobalRANNodeID gnb-id isn't exist.\n");
			return NGAP_CHECK_RNI_UNKNOW_ERROR;
		}

		BIT_STRING_t *gNB_ID = &globalGNB_ID->gNB_ID.choice.gNB_ID;
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "HandleGlobalRANNodeID globalGNB_ID BitString size=%d, unused=%d.\n", gNB_ID->size, gNB_ID->bits_unused);
//		gid = (gNB_ID->buf[0] << gNB_ID->size) + (gNB_ID->buf[1] << (gNB_ID->size - 8)) + (gNB_ID->buf[2] << (gNB_ID->size - 16)) + ((gNB_ID->buf[3]) >> (gNB_ID->size - 24));
		gid += (gNB_ID->buf[0] << 16);
		gid += (gNB_ID->buf[1] << 8);
		gid += (gNB_ID->buf[2] << 0);
		newContext->ranNodeId.setRanNodeType(RanNodeId::RNT_GNB_ID);
		newContext->ranNodeId.setRanNodeID(gid);

		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "HandleGlobalRANNodeID gid=%d, gNB_ID:0x%x%x%x\n",
					   gid, gNB_ID->buf[0], gNB_ID->buf[1], gNB_ID->buf[2], gNB_ID->buf[3]);
		break;
	}
	case Ngap_GlobalRANNodeID_PR_globalNgENB_ID:
	{
		Ngap_GlobalNgENB_ID_t *globalNgENB_ID = GlobalRANNodeID->choice.globalNgENB_ID;
		BIT_STRING_t *bstring = NULL;
		cause = HandlePlmn(&globalNgENB_ID->pLMNIdentity, newContext);
		if (cause != NGAP_CHECK_RNI_OK)
		{
			return cause;
		}

		newContext->ranNodeId.setRanNodeType(RanNodeId::RNT_NG_ENB_ID);
		switch (globalNgENB_ID->ngENB_ID.present)
		{
		case Ngap_NgENB_ID_PR_macroNgENB_ID:
			bstring = &globalNgENB_ID->ngENB_ID.choice.macroNgENB_ID;
			gid = (bstring->buf[0] << 20) + (bstring->buf[1] << 12) + (bstring->buf[2] << 4) +
				  ((bstring->buf[3] & 0xF0) >> 4);
			newContext->ranNodeId.setRanNgEnbType(RanNodeId::RNET_MACRO_NG_ENB);
			newContext->ranNodeId.setRanNodeID(gid);
			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "HandleGlobalRANNodeID gid=%d, MacroNgENB_ID:0x%x%x%x\n",
						   gid, bstring->buf[0], bstring->buf[1], bstring->buf[2], bstring->buf[3]);
			break;
		case Ngap_NgENB_ID_PR_shortMacroNgENB_ID:
			bstring = &globalNgENB_ID->ngENB_ID.choice.shortMacroNgENB_ID;
			gid = (bstring->buf[0] << 18) + (bstring->buf[1] << 10) + (bstring->buf[2] << 2) +
				  ((bstring->buf[3] & 0xFC) >> 2);
			newContext->ranNodeId.setRanNgEnbType(RanNodeId::RNET_SHORT_NG_ENB);
			newContext->ranNodeId.setRanNodeID(gid);
			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "HandleGlobalRANNodeID gid=%d, shortMacroNgENB_ID:0x%x%x%x\n",
						   gid, bstring->buf[0], bstring->buf[1], bstring->buf[2], bstring->buf[3]);
			break;
		case Ngap_NgENB_ID_PR_longMacroNgENB_ID:
			bstring = &globalNgENB_ID->ngENB_ID.choice.longMacroNgENB_ID;
			gid = (bstring->buf[0] << 21) + (bstring->buf[1] << 13) + (bstring->buf[2] << 5) +
				  ((bstring->buf[3] & 0xE0) >> 5);
			newContext->ranNodeId.setRanNgEnbType(RanNodeId::RNET_LONG_NG_ENB);
			newContext->ranNodeId.setRanNodeID(gid);
			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "HandleGlobalRANNodeID gid=%d, longMacroNgENB_ID:0x%x%x%x\n",
						   gid, bstring->buf[0], bstring->buf[1], bstring->buf[2], bstring->buf[3]);
			break;
		default:
			agc_log_printf(AGC_LOG, AGC_LOG_WARNING, "HandleGlobalRANNodeID unsupported ngENB_ID type invalid\n");
			return cause;
		}
		break;
	}
	default:
		agc_log_printf(AGC_LOG, AGC_LOG_WARNING, "HandleGlobalRANNodeID unsupported ran node type invalid\n");
		return cause;
	}

	return NGAP_CHECK_RNI_OK;
}

bool GNB_PDUSessionSetupRsp(NgapMessage &info)
{
	Ngap_SuccessfulOutcome_t *successfulOutcome = NULL;
	Ngap_PDUSessionResourceSetupResponse_t *N2PDUSessionResourceSetupResponse = NULL;
	Ngap_PDUSessionResourceSetupResponseIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
	Ngap_PDUSessionResourceSetupListSURes_t *PDUSessionResourceSetupListSURes = NULL;
	Ngap_PDUSessionResourceSetupResponseTransfer_t *pResponseTransfer = NULL;
	Ngap_PDUSessionResourceSetupItemSURes_t *item = NULL;

	uint32_t teid = 0;
	uint32_t teidv6 = 0;
	socklen_t addrlen = 0;
	socklen_t addrlenv6 = 0;
	agc_std_sockaddr_t addr;
	agc_std_sockaddr_t addrv6;
	int i = 0;
	bool bMediaProcess = false;
	NgapProtocol ngapDecode;
	uint8_t pdu_resource_id = 0;

	successfulOutcome = info.ngapMessage.choice.successfulOutcome;
	N2PDUSessionResourceSetupResponse = &successfulOutcome->value.choice.PDUSessionResourceSetupResponse;

	for (i = 0; i < N2PDUSessionResourceSetupResponse->protocolIEs.list.count; i++)
	{
		ie = N2PDUSessionResourceSetupResponse->protocolIEs.list.array[i];
		switch (ie->id)
		{
		case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
			RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
			AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_PDUSessionResourceSetupListSURes:
			PDUSessionResourceSetupListSURes = &ie->value.choice.PDUSessionResourceSetupListSURes;
			break;

		default:
			break;
		}
	}

	if (GNB_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_PDUSessionSetupRsp fail to GNB_ConvertNgapId ngap session RanUeNgapId=%d\n", *RanUeNgapId);
		return false;
	}

	NgapSession *sess = NULL;
	if (GetNgapSessManager().GetNgapSession(info.sessId, &sess) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "GNB_InitialContextSetupSuccessRsp fail to get sess=%d \n", info.sessId);
		return false;
	}

	if (PDUSessionResourceSetupListSURes == NULL)
	{
		return true;
	}

	sess->pdu_resource_count = PDUSessionResourceSetupListSURes->list.count;
	sess->pdu_sess_rsp_count = 0;

	for (i = 0; i < PDUSessionResourceSetupListSURes->list.count; i++)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "GNB_PDUSessionSetupRsp sessId=%d ListCxtRes count=%d i=%d\n",
					   info.sessId, PDUSessionResourceSetupListSURes->list.count, i);
		item = (Ngap_PDUSessionResourceSetupItemSURes_t *)PDUSessionResourceSetupListSURes->list.array[i];
		OCTET_STRING_t *tranfer = &item->pDUSessionResourceSetupResponseTransfer;

		int32_t len = tranfer->size;
		pdu_resource_id = item->pDUSessionID;
		ngapDecode.DecodeResourceSetupResponseTransfer((char *)tranfer->buf, (int32_t *)&len, (void **)&pResponseTransfer);

		if (pResponseTransfer == NULL)
		{
			continue;
		}

		if (pResponseTransfer->dLQosFlowPerTNLInformation.uPTransportLayerInformation.present == Ngap_UPTransportLayerInformation_PR_gTPTunnel)
		{
			struct Ngap_GTPTunnel *gTPTunnel = pResponseTransfer->dLQosFlowPerTNLInformation.uPTransportLayerInformation.choice.gTPTunnel;

			teid = gTPTunnel->gTP_TEID.buf[0] << 24 | gTPTunnel->gTP_TEID.buf[1] << 16 | gTPTunnel->gTP_TEID.buf[2] << 8 | gTPTunnel->gTP_TEID.buf[3];

			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "GNB_PDUSessionSetupRsp teid=%d buf=0x%x%x%x%x\n",
						   teid, gTPTunnel->gTP_TEID.buf[0], gTPTunnel->gTP_TEID.buf[1], gTPTunnel->gTP_TEID.buf[2], gTPTunnel->gTP_TEID.buf[3]);

			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "GNB_PDUSessionSetupRsp transportLayerAddress size=%d buf=0x%x%x%x%x\n",
						   gTPTunnel->transportLayerAddress.size,
						   gTPTunnel->transportLayerAddress.buf[0],
						   gTPTunnel->transportLayerAddress.buf[1],
						   gTPTunnel->transportLayerAddress.buf[2],
						   gTPTunnel->transportLayerAddress.buf[3]);

			if (4 == gTPTunnel->transportLayerAddress.size) // ipv4
			{
				struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&addr;
				local_addr1_v4->sin_family = AF_INET;
				//local_addr1_v4->sin_addr = agc_atoui((char *)gTPTunnel->transportLayerAddress.buf);
				memcpy(&local_addr1_v4->sin_addr, gTPTunnel->transportLayerAddress.buf, 4);
				addrlen = sizeof(struct sockaddr_in);
			}
			else if (16 == gTPTunnel->transportLayerAddress.size) //ipv6
			{
				struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&addrv6;
				local_addr1_v6->sin6_family = AF_INET6;
				memcpy(&local_addr1_v6->sin6_addr, gTPTunnel->transportLayerAddress.buf, 16);
				addrlenv6 = sizeof(struct sockaddr_in6);
			}
			else if (20 == gTPTunnel->transportLayerAddress.size)//ipv4&ipv6
			{
				struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&addr;
				local_addr1_v4->sin_family = AF_INET;
				//local_addr1_v4->sin_addr = agc_atoui((char *)gTPTunnel->transportLayerAddress.buf);
				memcpy(&local_addr1_v4->sin_addr, gTPTunnel->transportLayerAddress.buf, 4);
				addrlen = sizeof(struct sockaddr_in);

				struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&addrv6;
				local_addr1_v6->sin6_family = AF_INET6;
				memcpy(&local_addr1_v6->sin6_addr, gTPTunnel->transportLayerAddress.buf + 4, 16);
				addrlenv6 = sizeof(struct sockaddr_in6);
			}
			else
			{
				agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_PDUSessionSetupRsp transportLayerAddress length is invalid\n");
			}

			bMediaProcess = GetPduSessManager().processSessSetupRsp(info, pdu_resource_id, teid, addr, addrlen, teidv6, addrv6, addrlenv6);
		}
		ngapDecode.FreeResourceSetupResponseTransfer(pResponseTransfer);
	}

	//Forward to Amf
	if (!bMediaProcess)
		GetNsmManager().SendUpLayerSctp(info);
	return true;
}

bool GNB_PDUSessionSetupRspByMedia(NgapMessage &info)
{
	Ngap_SuccessfulOutcome_t *successfulOutcome = NULL;
	Ngap_PDUSessionResourceSetupResponse_t *N2PDUSessionResourceSetupResponse = NULL;
	Ngap_PDUSessionResourceSetupResponseIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
	Ngap_PDUSessionResourceSetupListSURes_t *PDUSessionResourceSetupListSURes = NULL;
	Ngap_PDUSessionResourceSetupResponseTransfer_t *pResponseTransfer = NULL;
	OCTET_STRING_t *tranfer = NULL;

	int i = 0;
	bool bMediaProcess = false;
	NgapProtocol ngapDecode;

	successfulOutcome = info.ngapMessage.choice.successfulOutcome;
	N2PDUSessionResourceSetupResponse = &successfulOutcome->value.choice.PDUSessionResourceSetupResponse;

	for (i = 0; i < N2PDUSessionResourceSetupResponse->protocolIEs.list.count; i++)
	{
		ie = N2PDUSessionResourceSetupResponse->protocolIEs.list.array[i];
		switch (ie->id)
		{
		case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
			RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
			AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_PDUSessionResourceSetupListSURes:
			PDUSessionResourceSetupListSURes = &ie->value.choice.PDUSessionResourceSetupListSURes;
			break;

		default:
			break;
		}
	}

	if (PDUSessionResourceSetupListSURes == NULL)
	{
		return true;
	}

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "GNB_PDUSessionSetupRspByMedia ListCxtRes count=%d \n",
				   PDUSessionResourceSetupListSURes->list.count);

	for (i = 0; i < PDUSessionResourceSetupListSURes->list.count; i++)
	{
		Ngap_PDUSessionResourceSetupItemSURes_t *item =
			(Ngap_PDUSessionResourceSetupItemSURes_t *)PDUSessionResourceSetupListSURes->list.array[i];
		tranfer = &item->pDUSessionResourceSetupResponseTransfer;

		int32_t len = tranfer->size;
		uint8_t pdu_resource_id = item->pDUSessionID;
		ngapDecode.DecodeResourceSetupResponseTransfer((char *)tranfer->buf, (int32_t *)&len, (void **)&pResponseTransfer);

		if (pResponseTransfer == NULL)
		{
			continue;
		}

		if (pResponseTransfer->dLQosFlowPerTNLInformation.uPTransportLayerInformation.present == Ngap_UPTransportLayerInformation_PR_gTPTunnel)
		{
			uint32_t teid;
			uint32_t teid_v6;
			agc_std_sockaddr_t addr;
			agc_std_sockaddr_t addr_v6;
			GetPduSessManager().GetUpfGtpInfo(info.sessId, pdu_resource_id, addr, teid, addr_v6, teid_v6);

			struct Ngap_GTPTunnel *gTPTunnel = pResponseTransfer->dLQosFlowPerTNLInformation.uPTransportLayerInformation.choice.gTPTunnel;

			if ((char *)gTPTunnel->gTP_TEID.buf == NULL)
			{
				gTPTunnel->gTP_TEID.buf = (uint8_t *)calloc(4, sizeof(uint8_t));
				gTPTunnel->gTP_TEID.size = 4;
			}

			gTPTunnel->gTP_TEID.buf[0] = teid >> 24;
			gTPTunnel->gTP_TEID.buf[1] = teid >> 16;
			gTPTunnel->gTP_TEID.buf[2] = teid >> 8;
			gTPTunnel->gTP_TEID.buf[3] = teid;

			if (gTPTunnel->transportLayerAddress.buf != NULL)
				free(gTPTunnel->transportLayerAddress.buf);

			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"GNB_PDUSessionSetupRspByMedia gTP IP.size=%d \n", gTPTunnel->transportLayerAddress.size);

			if (4 == gTPTunnel->transportLayerAddress.size) // ipv4
			{
				struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&addr;
				gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(4, sizeof(uint8_t));
				memcpy(gTPTunnel->transportLayerAddress.buf, &local_addr1_v4->sin_addr, 4);

			}
			else if (16 == gTPTunnel->transportLayerAddress.size) //ipv6
			{
				struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&addr_v6;
				gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(16, sizeof(uint8_t));
				memcpy(gTPTunnel->transportLayerAddress.buf, &local_addr1_v6->sin6_addr, 16);

			}
			else if(20 == gTPTunnel->transportLayerAddress.size) //ipv4 & ipv6
			{
				struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&addr;
				struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&addr_v6;
				gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(20, sizeof(uint8_t));
				memcpy(gTPTunnel->transportLayerAddress.buf, &local_addr1_v4->sin_addr, 4);
				memcpy(gTPTunnel->transportLayerAddress.buf+4, &local_addr1_v6->sin6_addr, 16);		
			}
			else
			{
				agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"GNB_PDUSessionSetupRspByMedia gTP IP.size length is invalid\n");
			}
		}

		if (tranfer != NULL)
		{
			if (tranfer->buf != NULL)
				free(tranfer->buf);

			if (pResponseTransfer->securityResult != NULL)
			{
				free(pResponseTransfer->securityResult);
				pResponseTransfer->securityResult = NULL;
			}

			tranfer->buf = (uint8_t *)calloc(100, sizeof(uint8_t));
			NgapProtocol ngapEncode;
			int32_t len = 0;
			ngapDecode.EncodeResourceSetupResponseTransfer((void *)pResponseTransfer, (char *)tranfer->buf, (int32_t *)&len);
			tranfer->size = len;
		}
		ngapDecode.FreeResourceSetupResponseTransfer(pResponseTransfer);
	}

	//Forward to Amf
	GetNsmManager().SendUpLayerSctp(info);

	return true;
}

bool GNB_PDUSessionModifyRsp(NgapMessage &info)
{
	Ngap_SuccessfulOutcome_t *successfulOutcome = NULL;
	Ngap_PDUSessionResourceModifyResponse_t *N2PDUSessionResourceModifyResponse = NULL;
	Ngap_PDUSessionResourceModifyResponseIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
	int i = 0;

	successfulOutcome = info.ngapMessage.choice.successfulOutcome;
	//d_assert(successfulOutcome, return,);

	N2PDUSessionResourceModifyResponse = &successfulOutcome->value.choice.PDUSessionResourceModifyResponse;
	//d_assert(N2PDUSessionResourceModifyResponse, return,);

	//			union Ngap_PDUSessionResourceModifyResponseIEs__Ngap_value_u {
	//				Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
	//				Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
	//				Ngap_PDUSessionResourceModifyListModRes_t	 PDUSessionResourceModifyListModRes;
	//				Ngap_PDUSessionResourceFailedToModifyListModRes_t	 PDUSessionResourceFailedToModifyListModRes;
	//				Ngap_UserLocationInformation_t	 UserLocationInformation;
	//				Ngap_CriticalityDiagnostics_t	 CriticalityDiagnostics;
	//			} choice;

	for (i = 0; i < N2PDUSessionResourceModifyResponse->protocolIEs.list.count; i++)
	{
		ie = N2PDUSessionResourceModifyResponse->protocolIEs.list.array[i];
		switch (ie->id)
		{
		case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
			RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
			AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
			break;

		default:
			break;
		}
	}

	if (GNB_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_PDUSessionModifyRsp fail to GNB_ConvertNgapId ngap session\n");
		return false;
	}

	//Forward to Amf
	GetNsmManager().SendUpLayerSctp(info);
	return true;
}

//lyb 2020-5-11
bool GNB_PDUSessionModifyInd(NgapMessage &info)
{
	Ngap_InitiatingMessage_t *initiatingMessage = NULL;
	Ngap_PDUSessionResourceModifyIndication_t *PDUSessionResourceModifyIndication = NULL;
	Ngap_PDUSessionResourceModifyIndicationIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
	//d_assert(initiatingMessage, return,);

	PDUSessionResourceModifyIndication = &initiatingMessage->value.choice.PDUSessionResourceModifyIndication;
	//d_assert(PDUSessionResourceModifyIndication, return,);

	//	union Ngap_PDUSessionResourceModifyIndicationIEs__Ngap_value_u {
	//		Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
	//		Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
	//		Ngap_PDUSessionResourceModifyListModInd_t	 PDUSessionResourceModifyListModInd;

	for (i = 0; i < PDUSessionResourceModifyIndication->protocolIEs.list.count; i++)
	{
		ie = PDUSessionResourceModifyIndication->protocolIEs.list.array[i];
		switch (ie->id)
		{
		case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
			RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
			AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
			break;

		default:
			break;
		}
	}

	//d_assert(RanUeNgapId, return,);

	if (GNB_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_PDUSessionModifyInd fail to GNB_ConvertNgapId ngap session\n");
		return false;
	}

	//Forward to Amf
	GetNsmManager().SendUpLayerSctp(info);

	return true;
}

bool GNB_PDUSessionReleaseRsp(NgapMessage &info)
{
	Ngap_SuccessfulOutcome_t *successfulOutcome = NULL;
	Ngap_PDUSessionResourceReleaseResponse_t *N2PDUSessionResourceReleaseResponse = NULL;
	Ngap_PDUSessionResourceReleaseResponseIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
	int i = 0;

	successfulOutcome = info.ngapMessage.choice.successfulOutcome;
	//d_assert(successfulOutcome, return,);

	N2PDUSessionResourceReleaseResponse = &successfulOutcome->value.choice.PDUSessionResourceReleaseResponse;
	//d_assert(N2PDUSessionResourceReleaseResponse, return,);

	/*			union Ngap_PDUSessionResourceReleaseResponseIEs__Ngap_value_u {
				Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
				Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
				Ngap_PDUSessionResourceReleasedListRelRes_t  PDUSessionResourceReleasedListRelRes;
				Ngap_UserLocationInformation_t	 UserLocationInformation;
				Ngap_CriticalityDiagnostics_t	 CriticalityDiagnostics;
			} choice;
*/

	for (i = 0; i < N2PDUSessionResourceReleaseResponse->protocolIEs.list.count; i++)
	{
		ie = N2PDUSessionResourceReleaseResponse->protocolIEs.list.array[i];
		switch (ie->id)
		{
		case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
			RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
			AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
			break;

		default:
			break;
		}
	}

	if (GNB_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_PDUSessionReleaseRsp fail to GNB_ConvertNgapId ngap session\n");
		return false;
	}

	//Forward to Amf
	GetNsmManager().SendUpLayerSctp(info);
	return true;
}

//lyb 2020-5-13
bool GNB_PDUSessionRsrNotify(NgapMessage &info)
{
	Ngap_InitiatingMessage_t *initiatingMessage = NULL;
	Ngap_PDUSessionResourceNotify_t *PDUSessionResourceNotify = NULL;
	Ngap_PDUSessionResourceNotifyIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
	//d_assert(initiatingMessage, return,);

	PDUSessionResourceNotify = &initiatingMessage->value.choice.PDUSessionResourceNotify;
	//d_assert(PDUSessionResourceNotify, return,);

	//		union Ngap_PDUSessionResourceNotifyIEs__Ngap_value_u {
	//			Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
	//			Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
	//			Ngap_PDUSessionResourceNotifyList_t	 PDUSessionResourceNotifyList;
	//			Ngap_PDUSessionResourceReleasedListNot_t	 PDUSessionResourceReleasedListNot;
	//			Ngap_UserLocationInformation_t	 UserLocationInformation;
	//		} choice;

	for (i = 0; i < PDUSessionResourceNotify->protocolIEs.list.count; i++)
	{
		ie = PDUSessionResourceNotify->protocolIEs.list.array[i];
		switch (ie->id)
		{
		case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
			RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
			AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
			break;

		default:
			break;
		}
	}

	if (GNB_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_PDUSessionRsrNotify fail to GNB_ConvertNgapId ngap session\n");
		return false;
	}

	GetNsmManager().SendUpLayerSctp(info);
	return true;
}

bool GNB_InitialContextSetupSuccessRsp(NgapMessage &info)
{
	Ngap_SuccessfulOutcome_t *successfulOutcome = NULL;
	Ngap_InitialContextSetupResponse_t *N2InitialContextSetupResponse = NULL;
	Ngap_InitialContextSetupResponseIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
	Ngap_PDUSessionResourceSetupListCxtRes_t *PDUSessionResourceSetupListCxtRes = NULL;
	Ngap_PDUSessionResourceSetupResponseTransfer_t *pResponseTransfer = NULL;
	int i = 0;
	NgapProtocol ngapDecode;
	bool bMediaProcess = false;
	uint32_t teid = 0;
	uint32_t teidv6 = 0;
	socklen_t addrlen = 0;
	socklen_t addrlenv6 = 0;
	agc_std_sockaddr_t addr;
	agc_std_sockaddr_t addrv6;
	uint8_t pdu_resource_id = 0;

	successfulOutcome = info.ngapMessage.choice.successfulOutcome;
	//d_assert(successfulOutcome, return,);

	N2InitialContextSetupResponse = &successfulOutcome->value.choice.InitialContextSetupResponse;
	//d_assert(N2InitialContextSetupResponse, return,);

	//			union Ngap_InitialContextSetupResponseIEs__Ngap_value_u {
	//				Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
	//				Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
	//				Ngap_PDUSessionResourceSetupListCxtRes_t	 PDUSessionResourceSetupListCxtRes;
	//				Ngap_PDUSessionResourceFailedToSetupListCxtRes_t	 PDUSessionResourceFailedToSetupListCxtRes;
	//				Ngap_CriticalityDiagnostics_t	 CriticalityDiagnostics;
	//			} choice;

	for (i = 0; i < N2InitialContextSetupResponse->protocolIEs.list.count; i++)
	{
		ie = N2InitialContextSetupResponse->protocolIEs.list.array[i];
		switch (ie->id)
		{
		case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
			RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
			AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_PDUSessionResourceSetupListCxtRes:
			PDUSessionResourceSetupListCxtRes = &ie->value.choice.PDUSessionResourceSetupListCxtRes;
			break;

		default:
			break;
		}
	}

	if (GNB_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_InitialContextSetupSuccessRsp fail to GNB_ConvertNgapId ngap session\n");
		return false;
	}

	NgapSession *sess;
	if (GetNgapSessManager().GetNgapSession(info.sessId, &sess) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "GNB_InitialContextSetupSuccessRsp fail to get sessId=%d \n", info.sessId);
		return false;
	}

	if (PDUSessionResourceSetupListCxtRes != NULL)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "GNB_InitialContextSetupSuccessRsp ListCxtRes count=%d \n",
					   PDUSessionResourceSetupListCxtRes->list.count);

		sess->pdu_resource_count = PDUSessionResourceSetupListCxtRes->list.count;
		sess->pdu_sess_rsp_count = 0;

		for (i = 0; i < PDUSessionResourceSetupListCxtRes->list.count; i++)
		{
			Ngap_PDUSessionResourceSetupItemCxtRes_t *item =
				(Ngap_PDUSessionResourceSetupItemCxtRes_t *)PDUSessionResourceSetupListCxtRes->list.array[i];
			OCTET_STRING_t *tranfer = &item->pDUSessionResourceSetupResponseTransfer;

			int32_t len = tranfer->size;
			ngapDecode.DecodeResourceSetupResponseTransfer((char *)tranfer->buf, (int32_t *)&len, (void **)&pResponseTransfer);

			pdu_resource_id = item->pDUSessionID;
			if (pResponseTransfer != NULL)
			{
				if (pResponseTransfer->dLQosFlowPerTNLInformation.uPTransportLayerInformation.present == Ngap_UPTransportLayerInformation_PR_gTPTunnel)
				{
					struct Ngap_GTPTunnel *gTPTunnel = pResponseTransfer->dLQosFlowPerTNLInformation.uPTransportLayerInformation.choice.gTPTunnel;

					teid = gTPTunnel->gTP_TEID.buf[0] << 24 | gTPTunnel->gTP_TEID.buf[1] << 16 | gTPTunnel->gTP_TEID.buf[2] << 8 | gTPTunnel->gTP_TEID.buf[3];

					agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "GNB_InitialContextSetupSuccessRsp teid=%d buf=0x%x%x%x%x\n",
								   teid, gTPTunnel->gTP_TEID.buf[0], gTPTunnel->gTP_TEID.buf[1], gTPTunnel->gTP_TEID.buf[2], gTPTunnel->gTP_TEID.buf[3]);

					agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "GNB_InitialContextSetupSuccessRsp transportLayerAddress size=%d buf=0x%x%x%x%x\n",
								   gTPTunnel->transportLayerAddress.size,
								   gTPTunnel->transportLayerAddress.buf[0],
								   gTPTunnel->transportLayerAddress.buf[1],
								   gTPTunnel->transportLayerAddress.buf[2],
								   gTPTunnel->transportLayerAddress.buf[3]);

					if (gTPTunnel->transportLayerAddress.size == 4) // ipv4
					{
						struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&addr;
						local_addr1_v4->sin_family = AF_INET;
						//local_addr1_v4->sin_addr = agc_atoui((char *)gTPTunnel->transportLayerAddress.buf);
						memcpy(&local_addr1_v4->sin_addr, gTPTunnel->transportLayerAddress.buf, 4);
						addrlen = sizeof(struct sockaddr_in);
					}
					else if (gTPTunnel->transportLayerAddress.size == 16) //ipv6
					{
						struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&addrv6;
						local_addr1_v6->sin6_family = AF_INET6;
						memcpy(&local_addr1_v6->sin6_addr, gTPTunnel->transportLayerAddress.buf, 16);
						addrlenv6 = sizeof(struct sockaddr_in6);
					}
					else if (20 == gTPTunnel->transportLayerAddress.size)//ipv4&ipv6
					{
						struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&addr;
						local_addr1_v4->sin_family = AF_INET;
						memcpy(&local_addr1_v4->sin_addr, gTPTunnel->transportLayerAddress.buf, 4);
						addrlen = sizeof(struct sockaddr_in);

						struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&addrv6;
						local_addr1_v6->sin6_family = AF_INET6;
						memcpy(&local_addr1_v6->sin6_addr, gTPTunnel->transportLayerAddress.buf + 4, 16);
						addrlenv6 = sizeof(struct sockaddr_in6);
					}
					else
					{				
						agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"GNB_InitialContextSetupSuccessRsp gtp ip length is invalid\n");
					}

					bMediaProcess = GetPduSessManager().processSessSetupRsp(info, pdu_resource_id, teid, addr, addrlen, teidv6, addrv6, addrlenv6);
				}

				ngapDecode.FreeResourceSetupResponseTransfer(pResponseTransfer);
			}
		}
	}

	//Forward to Amf
	if (!bMediaProcess)
		GetNsmManager().SendUpLayerSctp(info);

	GetNgapSessManager().StopNgapSessCheck(info.sessId);

	return true;
}

bool GNB_InitialContextSetupSuccessRspByMedia(NgapMessage &info)
{
	Ngap_SuccessfulOutcome_t *successfulOutcome = NULL;
	Ngap_InitialContextSetupResponse_t *N2InitialContextSetupResponse = NULL;
	Ngap_InitialContextSetupResponseIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
	Ngap_PDUSessionResourceSetupListCxtRes_t *PDUSessionResourceSetupListCxtRes = NULL;
	Ngap_PDUSessionResourceSetupResponseTransfer_t *pResponseTransfer = NULL;
	int i = 0;
	NgapProtocol ngapDecode;
	OCTET_STRING_t *tranfer = NULL;

	successfulOutcome = info.ngapMessage.choice.successfulOutcome;
	N2InitialContextSetupResponse = &successfulOutcome->value.choice.InitialContextSetupResponse;

	for (i = 0; i < N2InitialContextSetupResponse->protocolIEs.list.count; i++)
	{
		ie = N2InitialContextSetupResponse->protocolIEs.list.array[i];
		switch (ie->id)
		{
		case Ngap_ProtocolIE_ID_id_PDUSessionResourceSetupListCxtRes:
			PDUSessionResourceSetupListCxtRes = &ie->value.choice.PDUSessionResourceSetupListCxtRes;
			break;

		default:
			break;
		}
	}

	if (PDUSessionResourceSetupListCxtRes == NULL)
	{
		return true;
	}

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "GNB_InitialContextSetupSuccessRspByMedia ListCxtRes count=%d \n",
				   PDUSessionResourceSetupListCxtRes->list.count);

	for (i = 0; i < PDUSessionResourceSetupListCxtRes->list.count; i++)
	{
		Ngap_PDUSessionResourceSetupItemCxtRes_t *item =
			(Ngap_PDUSessionResourceSetupItemCxtRes_t *)PDUSessionResourceSetupListCxtRes->list.array[i];
		tranfer = &item->pDUSessionResourceSetupResponseTransfer;

		int32_t len = tranfer->size;
		ngapDecode.DecodeResourceSetupResponseTransfer((char *)tranfer->buf, (int32_t *)&len, (void **)&pResponseTransfer);
		uint8_t pdu_resource_id = item->pDUSessionID;

		if (pResponseTransfer != NULL)
		{
			uint32_t teid;
			uint32_t teid_v6;
			agc_std_sockaddr_t addr;
			agc_std_sockaddr_t addr_v6;
			GetPduSessManager().GetUpfGtpInfo(info.sessId, pdu_resource_id, addr, teid, addr_v6, teid_v6);

			if (pResponseTransfer->dLQosFlowPerTNLInformation.uPTransportLayerInformation.present == Ngap_UPTransportLayerInformation_PR_gTPTunnel)
			{
				struct Ngap_GTPTunnel *gTPTunnel = pResponseTransfer->dLQosFlowPerTNLInformation.uPTransportLayerInformation.choice.gTPTunnel;

				if ((char *)gTPTunnel->gTP_TEID.buf == NULL)
				{
					gTPTunnel->gTP_TEID.buf = (uint8_t *)calloc(4, sizeof(uint8_t));
					gTPTunnel->gTP_TEID.size = 4;
				}

				gTPTunnel->gTP_TEID.buf[0] = teid >> 24;
				gTPTunnel->gTP_TEID.buf[1] = teid >> 16;
				gTPTunnel->gTP_TEID.buf[2] = teid >> 8;
				gTPTunnel->gTP_TEID.buf[3] = teid;

				if (gTPTunnel->transportLayerAddress.buf != NULL)
					free(gTPTunnel->transportLayerAddress.buf);

				agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"GNB_InitialContextSetupSuccessRspByMedia gTP IP.size=%d \n", gTPTunnel->transportLayerAddress.size);

				if (4 == gTPTunnel->transportLayerAddress.size)//ipv4 only
				{
					struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&addr;
					gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(4, sizeof(uint8_t));
					memcpy(gTPTunnel->transportLayerAddress.buf, &local_addr1_v4->sin_addr, 4);
				}
				else if (16 == gTPTunnel->transportLayerAddress.size)//ipv6 only
				{
					struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&addr_v6;
					gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(16, sizeof(uint8_t));
					memcpy(gTPTunnel->transportLayerAddress.buf, &local_addr1_v6->sin6_addr, 16);
				}
				else if(20 == gTPTunnel->transportLayerAddress.size) //ipv4 & ipv6
				{
					struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&addr;
					struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&addr_v6;
					gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(20, sizeof(uint8_t));
					memcpy(gTPTunnel->transportLayerAddress.buf, &local_addr1_v4->sin_addr, 4);
					memcpy(gTPTunnel->transportLayerAddress.buf+4, &local_addr1_v6->sin6_addr, 16);		
				}
				else
				{
					agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"GNB_InitialContextSetupSuccessRspByMedia gTP IP.size length is invalid\n");
				}
				
			}

			if (tranfer != NULL)
			{
				if (pResponseTransfer->securityResult != NULL)
				{
					free(pResponseTransfer->securityResult);
					pResponseTransfer->securityResult = NULL;
				}

				NgapProtocol ngapEncode;
				int32_t len = 0;
				ngapDecode.EncodeResourceSetupResponseTransfer((void *)pResponseTransfer, (char *)tranfer->buf, (int32_t *)&len);
				tranfer->size = len;
			}
			ngapDecode.FreeResourceSetupResponseTransfer(pResponseTransfer);
		}
	}

	GetNsmManager().SendUpLayerSctp(info);
	return true;
}

bool GNB_InitialContextSetupFailureRsp(NgapMessage &info)
{
	Ngap_UnsuccessfulOutcome_t *unsuccessfulOutcome = NULL;
	Ngap_InitialContextSetupFailure_t *N2InitialContextSetupFailure = NULL;
	Ngap_InitialContextSetupFailureIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
	int i = 0;

	unsuccessfulOutcome = info.ngapMessage.choice.unsuccessfulOutcome;
	//d_assert(unsuccessfulOutcome, return,);

	N2InitialContextSetupFailure = &unsuccessfulOutcome->value.choice.InitialContextSetupFailure;
	//d_assert(N2InitialContextSetupFailure, return,);

	/*	union Ngap_InitialContextSetupFailureIEs__Ngap_value_u {
		Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
		Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
		Ngap_PDUSessionResourceFailedToSetupListCxtFail_t	 PDUSessionResourceFailedToSetupListCxtFail;
		Ngap_Cause_t	 Cause;
		Ngap_CriticalityDiagnostics_t	 CriticalityDiagnostics;
	} choice;
*/

	for (i = 0; i < N2InitialContextSetupFailure->protocolIEs.list.count; i++)
	{
		ie = N2InitialContextSetupFailure->protocolIEs.list.array[i];
		switch (ie->id)
		{
		case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
			RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
			AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
			break;

		default:
			break;
		}
	}

	if (GNB_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_InitialContextSetupFailureRsp fail to GNB_ConvertNgapId ngap session\n");
		return false;
	}

	NgapSession *sess = NULL;
	if (GetNgapSessManager().GetNgapSession(info.sessId, &sess) == true)
	{
		for (i = 0; i < MAX_PDU_SESSION_SIZE; i++)
		{
			if (sess->vecPduSessionID[i] == INVALID_PDU_SESSION_ID)
				continue;

			GetPduSessManager().processSessSetupfail(info, sess->vecPduSessionID[i]);
		}
	}

	//Forward to Amf
	GetNsmManager().SendUpLayerSctp(info);
	return true;
}

bool GNB_UEContextReleaseReq(NgapMessage &info)
{
	Ngap_InitiatingMessage_t *initiatingMessage = NULL;
	Ngap_UEContextReleaseRequest_t *N2UEContextReleaseRequest = NULL;
	Ngap_UEContextReleaseRequest_IEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
	//d_assert(initiatingMessage, return,);

	N2UEContextReleaseRequest = &initiatingMessage->value.choice.UEContextReleaseRequest;
	//d_assert(N2UEContextReleaseRequest, return,);

	//			union Ngap_UEContextReleaseRequest_IEs__Ngap_value_u {
	//				Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
	//				Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
	//				Ngap_PDUSessionResourceListCxtRelReq_t	 PDUSessionResourceListCxtRelReq;
	//				Ngap_Cause_t	 Cause;
	//			} choice;

	for (i = 0; i < N2UEContextReleaseRequest->protocolIEs.list.count; i++)
	{
		ie = N2UEContextReleaseRequest->protocolIEs.list.array[i];
		switch (ie->id)
		{
		case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
			RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
			AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
			break;

		default:
			break;
		}
	}

	if (GNB_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_UEContextReleaseReq fail to GNB_ConvertNgapId ngap session\n");
		return false;
	}

	GetNsmManager().SendUpLayerSctp(info);

	NgapSession *sess = NULL;
	if (GetNgapSessManager().GetNgapSession(info.sessId, &sess) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_HandoverNotify getNgapSession fail to get ngap session, sessId=%d\n", info.sessId);
		return false;
	}

	// UE is release, so release pdu sess
	for (i = 0; i < MAX_PDU_SESSION_SIZE; i++)
	{
		if (sess->vecPduSessionID[i] == INVALID_PDU_SESSION_ID)
			continue;

		GetPduSessManager().ReleasePduSess(info.sessId, sess->vecPduSessionID[i]);
	}

	//release ngap session
	GetNgapSessManager().DeleteNgapSession(info.sessId);
	return true;
}

bool GNB_UEContextReleaseComplete(NgapMessage &info)
{
	Ngap_SuccessfulOutcome_t *successfulOutcome = NULL;
	Ngap_UEContextReleaseComplete_t *N2UEContextReleaseComplete = NULL;
	Ngap_UEContextReleaseComplete_IEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
	int i = 0;

	successfulOutcome = info.ngapMessage.choice.successfulOutcome;
	//d_assert(successfulOutcome, return,);

	N2UEContextReleaseComplete = &successfulOutcome->value.choice.UEContextReleaseComplete;
	//d_assert(N2UEContextReleaseComplete, return,);

	/*			union Ngap_UEContextReleaseComplete_IEs__Ngap_value_u {
				Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
				Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
				Ngap_UserLocationInformation_t	 UserLocationInformation;
				Ngap_InfoOnRecommendedCellsAndRANNodesForPaging_t	 InfoOnRecommendedCellsAndRANNodesForPaging;
				Ngap_PDUSessionResourceListCxtRelCpl_t	 PDUSessionResourceListCxtRelCpl;
				Ngap_CriticalityDiagnostics_t	 CriticalityDiagnostics;
			} choice;
*/

	for (i = 0; i < N2UEContextReleaseComplete->protocolIEs.list.count; i++)
	{
		ie = N2UEContextReleaseComplete->protocolIEs.list.array[i];
		switch (ie->id)
		{
		case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
			RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
			AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
			break;

		default:
			break;
		}
	}

	//Add for Handover
	//Check sigGW role of handover.
	uintmax_t ueid;
	asn_INTEGER2umax(AMF_UE_NGAP_ID, &ueid);
	info.sessId = (uint32_t)ueid;

	NgapSession *sess = NULL;
	if (GetNgapSessManager().GetNgapSession(info.sessId, &sess) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_HandoverNotify getNgapSession fail to get ngap session, sessId=%d\n", info.sessId);
		return false;
	}

	//If handover is under the same vGW, vGW handle the response, not forward.
	if (Ralation_RAN_Handover_IN_SAME_GW == sess->eHandoverflag && HO_State_SendRelCommandWaitRelComplete == sess->eHOState)
	{
		//release resource of src RAN side.

		sess->ResetHandoverInfo();

		return true;
	}

	if (GNB_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_UEContextReleaseComplete fail to GNB_ConvertNgapId ngap session\n");
		return false;
	}

	//Forward to Amf
	GetNsmManager().SendUpLayerSctp(info);
	/*
	// UE is release, so release pdu sess
	for (uint32_t i = 0; i < sess->vecPduSessionID.size(); i++)
	{
		GetPduSessManager().ReleasePduSess(info.sessId, sess->vecPduSessionID[i]);
	}

	//release ngap session
	GetNgapSessManager().DeleteNgapSession(info.sessId);
	*/
	return true;
}

bool GNB_UEContextModificationSuccessRsp(NgapMessage &info)
{
	Ngap_SuccessfulOutcome_t *successfulOutcome = NULL;
	Ngap_UEContextModificationResponse_t *N2UEContextModificationResponse = NULL;
	Ngap_UEContextModificationResponseIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
	int i = 0;

	successfulOutcome = info.ngapMessage.choice.successfulOutcome;
	//d_assert(successfulOutcome, return,);

	N2UEContextModificationResponse = &successfulOutcome->value.choice.UEContextModificationResponse;
	//d_assert(N2UEContextModificationResponse, return,);

	/*			union Ngap_UEContextModificationResponseIEs__Ngap_value_u {
				Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
				Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
				Ngap_RRCState_t  RRCState;
				Ngap_UserLocationInformation_t	 UserLocationInformation;
				Ngap_CriticalityDiagnostics_t	 CriticalityDiagnostics;
			} choice;
*/

	for (i = 0; i < N2UEContextModificationResponse->protocolIEs.list.count; i++)
	{
		ie = N2UEContextModificationResponse->protocolIEs.list.array[i];
		switch (ie->id)
		{
		case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
			RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
			AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
			break;

		default:
			break;
		}
	}

	if (GNB_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_UEContextModificationSuccessRsp fail to GNB_ConvertNgapId ngap session\n");
		return false;
	}

	//Forward to Amf
	GetNsmManager().SendUpLayerSctp(info);
	return true;
}

bool GNB_UEContextModificationFailureRsp(NgapMessage &info)
{
	Ngap_UnsuccessfulOutcome_t *unsuccessfulOutcome = NULL;
	Ngap_UEContextModificationFailure_t *N2UEContextModificationFailure = NULL;
	Ngap_UEContextModificationFailureIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
	int i = 0;

	unsuccessfulOutcome = info.ngapMessage.choice.unsuccessfulOutcome;
	//d_assert(unsuccessfulOutcome, return,);

	N2UEContextModificationFailure = &unsuccessfulOutcome->value.choice.UEContextModificationFailure;
	//d_assert(N2UEContextModificationFailure, return,);

	/*			union Ngap_UEContextModificationFailureIEs__Ngap_value_u {
				Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
				Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
				Ngap_Cause_t	 Cause;
				Ngap_CriticalityDiagnostics_t	 CriticalityDiagnostics;
			} choice;
*/

	for (i = 0; i < N2UEContextModificationFailure->protocolIEs.list.count; i++)
	{
		ie = N2UEContextModificationFailure->protocolIEs.list.array[i];
		switch (ie->id)
		{
		case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
			RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
			AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
			break;

		default:
			break;
		}
	}

	if (GNB_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_UEContextModificationFailureRsp fail to GNB_ConvertNgapId ngap session\n");
		return false;
	}

	//Forward to Amf
	GetNsmManager().SendUpLayerSctp(info);
	return true;
}

bool GNB_GetInfoFromS2TTransparentContainer(NgapMessage &info,
											Ngap_SourceToTarget_TransparentContainer_t *SourceToTarget_TransparentContainer, Ngap_NR_CGI_t **nr_cgi)
{
	int32_t len = SourceToTarget_TransparentContainer->size;
	NgapProtocol ngapDecode;
	Ngap_SourceNGRANNode_ToTargetNGRANNode_TransparentContainer *pContainer = NULL;

	ngapDecode.DecodeS2TTransparentContainer((char *)SourceToTarget_TransparentContainer->buf, (int32_t *)&len, (void **)&pContainer);

	if (pContainer == NULL || pContainer->pDUSessionResourceInformationList == NULL)
	{

		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_GetInfoFromS2TTransparentContainer pDUSessionResourceInformationList is null .\n");

		ngapDecode.FreeS2TTransparentContainer(pContainer);
		return false;
	}

	for (int i = 0; i < pContainer->pDUSessionResourceInformationList->list.count; i++)
	{
		Ngap_PDUSessionResourceInformationItem_t *itemResource = (Ngap_PDUSessionResourceInformationItem_t *)pContainer->pDUSessionResourceInformationList->list.array[i];

		for (int j = 0; j < itemResource->dRBsToQosFlowsMappingList->list.count; j++)
		{
			Ngap_DRBsToQosFlowsMappingItem_t *itemDrb = (Ngap_DRBsToQosFlowsMappingItem_t *)itemResource->dRBsToQosFlowsMappingList->list.array[i];

			GetPduSessManager().prepareHoDrb(info.sessId, itemResource->pDUSessionID, itemDrb->dRB_ID);
		}
	}

	if (pContainer->targetCell_ID.present == Ngap_NGRAN_CGI_PR_nR_CGI)
	{
		*nr_cgi = pContainer->targetCell_ID.choice.nR_CGI;
	}

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "before package info: nr_cgi->nRCellIdentity.size=%d, buf=%p, nr_cgi->pLMNIdentity.size=%d, buf=%p\n", 
																(*nr_cgi)->nRCellIdentity.size,
																(*nr_cgi)->nRCellIdentity.buf,
																(*nr_cgi)->pLMNIdentity.size,
																(*nr_cgi)->pLMNIdentity.buf);

	ngapDecode.FreeS2TTransparentContainer(pContainer);

	
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "before package info: nr_cgi->nRCellIdentity.size=%d, buf=%p, nr_cgi->pLMNIdentity.size=%d, buf=%p\n", 
																	(*nr_cgi)->nRCellIdentity.size,
																	(*nr_cgi)->nRCellIdentity.buf,
																	(*nr_cgi)->pLMNIdentity.size,
																	(*nr_cgi)->pLMNIdentity.buf);
	
	return true;
}
											
bool GNB_GetInfoFromS2TTransparentContainer2(NgapMessage &info,
											Ngap_SourceToTarget_TransparentContainer_t *SourceToTarget_TransparentContainer, Ngap_SourceNGRANNode_ToTargetNGRANNode_TransparentContainer **container)
{
	int32_t len = SourceToTarget_TransparentContainer->size;
	NgapProtocol ngapDecode;
	Ngap_SourceNGRANNode_ToTargetNGRANNode_TransparentContainer *pContainer = NULL;

	ngapDecode.DecodeS2TTransparentContainer((char *)SourceToTarget_TransparentContainer->buf, (int32_t *)&len, (void **)&pContainer);

	if (pContainer == NULL || pContainer->pDUSessionResourceInformationList == NULL)
	{

		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_GetInfoFromS2TTransparentContainer pDUSessionResourceInformationList is null .\n");

		ngapDecode.FreeS2TTransparentContainer(pContainer);
		return false;
	}

    agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "pDUSessionResourceInformationList->list.count=%d .\n",  pContainer->pDUSessionResourceInformationList->list.count);

	for (int i = 0; i < pContainer->pDUSessionResourceInformationList->list.count; i++)
	{
		Ngap_PDUSessionResourceInformationItem_t *itemResource = (Ngap_PDUSessionResourceInformationItem_t *)pContainer->pDUSessionResourceInformationList->list.array[i];
        if(itemResource == NULL){
            continue;
        }

        agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "dRBsToQosFlowsMappingList->list.count %d\n", itemResource->dRBsToQosFlowsMappingList->list.count);
		for (int j = 0; j < itemResource->dRBsToQosFlowsMappingList->list.count; j++)
		{

			Ngap_DRBsToQosFlowsMappingItem_t *itemDrb = (Ngap_DRBsToQosFlowsMappingItem_t *)itemResource->dRBsToQosFlowsMappingList->list.array[j];
            if (itemDrb == NULL){
                continue;
            }
			GetPduSessManager().prepareHoDrb(info.sessId, itemResource->pDUSessionID, itemDrb->dRB_ID);
		}
	}

	if (pContainer->targetCell_ID.present == Ngap_NGRAN_CGI_PR_nR_CGI)
	{
		//*nr_cgi = pContainer->targetCell_ID.choice.nR_CGI;
		*container = pContainer;
	}
	#if 0
	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "before package info: nr_cgi->nRCellIdentity.size=%d, buf=%p, nr_cgi->pLMNIdentity.size=%d, buf=%p\n", 
																(*nr_cgi)->nRCellIdentity.size,
																(*nr_cgi)->nRCellIdentity.buf,
																(*nr_cgi)->pLMNIdentity.size,
																(*nr_cgi)->pLMNIdentity.buf);

	ngapDecode.FreeS2TTransparentContainer(pContainer);

	
	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "before package info: nr_cgi->nRCellIdentity.size=%d, buf=%p, nr_cgi->pLMNIdentity.size=%d, buf=%p\n", 
																	(*nr_cgi)->nRCellIdentity.size,
																	(*nr_cgi)->nRCellIdentity.buf,
																	(*nr_cgi)->pLMNIdentity.size,
																	(*nr_cgi)->pLMNIdentity.buf);

	#endif
	
	return true;
}

bool GNB_HandoverRequired(NgapMessage &info)
{
	Ngap_InitiatingMessage_t *initiatingMessage = NULL;
	Ngap_HandoverRequired_t *HandoverRequired = NULL;
	Ngap_HandoverRequiredIEs_t *ie = NULL;

	//	RanNodeId                   srcGnbID = info.ranNodeId;
	Ngap_TargetID_t *TargetID = NULL;
	Ngap_GlobalRANNodeID_t *globalRANNodeID = NULL;
	Ngap_RAN_UE_NGAP_ID_t *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
	Ngap_NR_CGI_t *nr_cgi = NULL;

	Ngap_SourceToTarget_TransparentContainer_t *SourceToTarget_TransparentContainer = NULL;
	Ngap_SourceNGRANNode_ToTargetNGRANNode_TransparentContainer *container = NULL;
	NgapProtocol ngapDecode;

	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
	//d_assert(initiatingMessage, return,);

	HandoverRequired = &initiatingMessage->value.choice.HandoverRequired;
	//d_assert(HandoverRequired, return,);

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

	for (i = 0; i < HandoverRequired->protocolIEs.list.count; i++)
	{
		ie = HandoverRequired->protocolIEs.list.array[i];
		switch (ie->id)
		{
		case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
			RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
			AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_HandoverType:
			if (Ngap_HandoverType_intra5gs != ie->value.choice.HandoverType) //only support handover intra5Gs
			{
				agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_HandoverRequired Handover type not support by the system.\n");
				return false;
			}
			break;

		case Ngap_ProtocolIE_ID_id_TargetID:
			TargetID = &ie->value.choice.TargetID;
			break;

		case Ngap_ProtocolIE_ID_id_SourceToTarget_TransparentContainer:
			SourceToTarget_TransparentContainer = &ie->value.choice.SourceToTarget_TransparentContainer;

			break;

		default:
			break;
		}
	}

	if (TargetID == NULL)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_HandoverRequired TargetID is NULL\n");
		return false;
	}

	globalRANNodeID = &TargetID->choice.targetRANNodeID->globalRANNodeID;

	if (GNB_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_HandoverRequired fail to GNB_ConvertNgapId ngap session\n");
		return false;
	}

	NgapSession *CurSess = NULL;
	if (GetNgapSessManager().GetNgapSession(info.sessId, &CurSess) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_HandoverRequired getNgapSession fail to get ngap session, sessId=%d\n", info.sessId);
		return false;
	}

	//1.Set flag in session
	//	uintmax_t ueid;
	//	asn_INTEGER2umax(AMF_UE_NGAP_ID, &ueid);
	//	info.sessId = (uint32_t)ueid;

	uint32_t cause = NGAP_CHECK_RNI_OK;
	EsmContext newContext;
	newContext.sctp_index = info.sockSource;
	newContext.idSigGW = info.idSigGW;

	GNB_GetInfoFromS2TTransparentContainer2(info, SourceToTarget_TransparentContainer, &container);
	nr_cgi = container->targetCell_ID.choice.nR_CGI;

	
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "before package info: nr_cgi->nRCellIdentity.size=%d, buf=%p, nr_cgi->pLMNIdentity.size=%d, buf=%p\n", 
																		(nr_cgi)->nRCellIdentity.size,
																		(nr_cgi)->nRCellIdentity.buf,
																		(nr_cgi)->pLMNIdentity.size,
																		(nr_cgi)->pLMNIdentity.buf);
	
	cause = HandleGlobalRANNodeID(globalRANNodeID, &newContext);
	if ((cause == NGAP_CHECK_RNI_OK) && TargetID != NULL 
		&& TargetID->present == Ngap_TargetID_PR_targetRANNodeID 
		&& TargetID->choice.targetRANNodeID != NULL 
		&& nr_cgi != NULL 
		&& Ralation_RAN_Handover_IN_SAME_GW == GetEsmManager().CheckRANinGW(info, newContext.ranNodeId)) //Handover in the same GW ,need handle by the GW.
	{

		GetNgapSessManager().SaveSourceToTarget_TransparentContainer(info.sessId, SourceToTarget_TransparentContainer);

		CurSess->eHandoverflag = Ralation_RAN_Handover_IN_SAME_GW;
		CurSess->TgtGnbId = info.ranNodeId; //info.ranNodeId updated to target RAN by CheckRANinGW.
		//Send Handover Request msg to target RAN
		//GetEsmManager().SendHandoverRequest(info, SourceToTarget_TransparentContainer);
		GetAsmManager().SendPathSwithRequest(info, nr_cgi, &TargetID->choice.targetRANNodeID->selectedTAI);
		return true;
	}

	GetNsmManager().SendUpLayerSctp(info);
	ngapDecode.FreeS2TTransparentContainer(container);
	return true;
}

//lyb 2020
bool GNB_HandoverReqAck(NgapMessage &info)
{
	Ngap_SuccessfulOutcome_t *successfulOutcome = NULL;
	Ngap_HandoverRequestAcknowledge_t *HandoverRequestAcknowledge = NULL;
	Ngap_HandoverRequestAcknowledgeIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
	Ngap_TargetToSource_TransparentContainer_t *pTargetToSource_TransparentContainer = NULL;
	Ngap_PDUSessionResourceAdmittedList_t *PDUSessionResourceAdmittedList = NULL;
	int i = 0;

	successfulOutcome = info.ngapMessage.choice.successfulOutcome;
	//d_assert(successfulOutcome, return,);

	HandoverRequestAcknowledge = &successfulOutcome->value.choice.HandoverRequestAcknowledge;
	//d_assert(HandoverRequestAcknowledge, return,);


	for (i = 0; i < HandoverRequestAcknowledge->protocolIEs.list.count; i++)
	{
		ie = HandoverRequestAcknowledge->protocolIEs.list.array[i];
		switch (ie->id)
		{
		case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
			RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
			AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_PDUSessionResourceAdmittedList:
			PDUSessionResourceAdmittedList = &ie->value.choice.PDUSessionResourceAdmittedList;
			break;

		case Ngap_ProtocolIE_ID_id_TargetToSource_TransparentContainer:
			pTargetToSource_TransparentContainer = &ie->value.choice.TargetToSource_TransparentContainer;
			break;

		default:
			break;
		}
	}

	//Check sigGW role of handover.

	uintmax_t ueid;
	asn_INTEGER2umax(AMF_UE_NGAP_ID, &ueid);
	info.sessId = (uint32_t)ueid;
	NgapSession *sess = NULL;
	if (GetNgapSessManager().GetNgapSession(info.sessId, &sess) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_HandoverReqAck getNgapSession fail to get ngap session, sessId=%d\n", info.sessId);
		return false;
	}

	EsmContext *esmContext;
	if (GetEsmManager().GetEsmContext(sess->gnb_sctp_index, &esmContext) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_HandoverReqAck fail to get target esm context.\n");
		return false;
	}

	//If handover is under the same vGW, vGW handle the response, not forward.

	bool bMediaProcess = false;

	if (Ralation_RAN_Handover_IN_SAME_GW == sess->eHandoverflag) {
		bool bMediaProcess = false;
		//Update session when first receiving Rsp from target RAN.
		sess->TgtGnbUeId = *RanUeNgapId;

		//prepare info for sending msg to src RAN.
		info.amfUeId = sess->sessId;
		info.gnbUeId = sess->gnbUeId;
		info.ranNodeId = sess->gnbId;
		info.sockTarget = esmContext->sctp_index;

		if (PDUSessionResourceAdmittedList != NULL) {
			NgapProtocol ngapDecode;
			OCTET_STRING_t *tranfer;
			Ngap_HandoverRequestAcknowledgeTransfer_t *pContainer = NULL;

			for (i = 0; i < PDUSessionResourceAdmittedList->list.count; i++) {
				Ngap_PDUSessionResourceAdmittedItem_t *item =
						(Ngap_PDUSessionResourceAdmittedItem_t*) PDUSessionResourceAdmittedList->list.array[i];
				tranfer = &item->handoverRequestAcknowledgeTransfer;
				uint8_t pdu_resource_id = item->pDUSessionID;
				int32_t len = tranfer->size;
				ngapDecode.DecodeHandoverReqAckTransfer((char*) tranfer->buf,
						(int32_t*) &len, (void**) &pContainer);

				if (pContainer == NULL) {
					continue;
				}

				if (pContainer->dL_NGU_UP_TNLInformation.present
						== Ngap_UPTransportLayerInformation_PR_gTPTunnel) {
					struct Ngap_GTPTunnel *gTPTunnel =
							pContainer->dL_NGU_UP_TNLInformation.choice.gTPTunnel;
					uint32_t teid = 0;
					uint32_t teidv6 = 0;
					socklen_t addrlen = 0;
					socklen_t addrlenv6 = 0;
					agc_std_sockaddr_t addr;
					agc_std_sockaddr_t addrv6;

					teid = gTPTunnel->gTP_TEID.buf[0] << 24
							| gTPTunnel->gTP_TEID.buf[1] << 16
							| gTPTunnel->gTP_TEID.buf[2] << 8
							| gTPTunnel->gTP_TEID.buf[3];

					agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,
							"GNB_HandoverReqAck teid=%d buf=0x%x%x%x%x\n", teid,
							gTPTunnel->gTP_TEID.buf[0],
							gTPTunnel->gTP_TEID.buf[1],
							gTPTunnel->gTP_TEID.buf[2],
							gTPTunnel->gTP_TEID.buf[3]);

					agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,
							"GNB_HandoverReqAck transportLayerAddress size=%d buf=0x%x%x%x%x\n",
							gTPTunnel->transportLayerAddress.size,
							gTPTunnel->transportLayerAddress.buf[0],
							gTPTunnel->transportLayerAddress.buf[1],
							gTPTunnel->transportLayerAddress.buf[2],
							gTPTunnel->transportLayerAddress.buf[3]);

					if (gTPTunnel->transportLayerAddress.size == 4) // ipv4
					{
						struct sockaddr_in *local_addr1_v4 =
								(struct sockaddr_in*) &addr;
						local_addr1_v4->sin_family = AF_INET;

						memcpy(&local_addr1_v4->sin_addr,
								gTPTunnel->transportLayerAddress.buf, 4);
						addrlen = sizeof(struct sockaddr_in);
					} else if (gTPTunnel->transportLayerAddress.size == 16) //ipv6
					{
						struct sockaddr_in6 *local_addr1_v6 =
								(struct sockaddr_in6*) &addrv6;
						local_addr1_v6->sin6_family = AF_INET6;
						memcpy(&local_addr1_v6->sin6_addr,
								gTPTunnel->transportLayerAddress.buf, 16);
						addrlenv6 = sizeof(struct sockaddr_in6);
					} else if (20 == gTPTunnel->transportLayerAddress.size)//ipv4&ipv6
					{
						struct sockaddr_in *local_addr1_v4 =
								(struct sockaddr_in*) &addr;
						local_addr1_v4->sin_family = AF_INET;

						memcpy(&local_addr1_v4->sin_addr,
								gTPTunnel->transportLayerAddress.buf, 4);
						addrlen = sizeof(struct sockaddr_in);

						struct sockaddr_in6 *local_addr1_v6 =
								(struct sockaddr_in6*) &addrv6;
						local_addr1_v6->sin6_family = AF_INET6;
						memcpy(&local_addr1_v6->sin6_addr,
								gTPTunnel->transportLayerAddress.buf + 4, 16);
						addrlenv6 = sizeof(struct sockaddr_in6);

					}

					GetPduSessManager().prepareHoTargetGnbAddr(sess->sessId,
							pdu_resource_id, teid, addr, addrlen, teidv6,
							addrv6, addrlenv6);

					//	GetPduSessManager().processSessSetupRsp(info, pdu_resource_id, teid, addr, addrlen, teidv6, addrv6, addrlenv6);

					ngapDecode.FreeHandoverReqAckTransfer(pContainer);
				}
			}
		}

		GetEsmManager().SendHandoverCommand(info,
				pTargetToSource_TransparentContainer,
				PDUSessionResourceAdmittedList);
	} else //The handover is handled by AMF, sigGW will forward msg.
	{
		sess->gnbUeId = *RanUeNgapId;
		if (GNB_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false) {
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR,
					"GNB_HandoverReqAck fail to GNB_ConvertNgapId ngap session\n");
			return false;
		}

		if (PDUSessionResourceAdmittedList != NULL) {
			NgapProtocol ngapDecode;
			OCTET_STRING_t *tranfer;
			Ngap_HandoverRequestAcknowledgeTransfer_t *pContainer = NULL;

			sess->pdu_resource_count = PDUSessionResourceAdmittedList->list.count;
			sess->pdu_sess_rsp_count = 0;

			for (i = 0; i < PDUSessionResourceAdmittedList->list.count; i++) {
				Ngap_PDUSessionResourceAdmittedItem_t *item =
						(Ngap_PDUSessionResourceAdmittedItem_t*) PDUSessionResourceAdmittedList->list.array[i];
				tranfer = &item->handoverRequestAcknowledgeTransfer;
				uint8_t pdu_resource_id = item->pDUSessionID;
				int32_t len = tranfer->size;
				ngapDecode.DecodeHandoverReqAckTransfer((char*) tranfer->buf,
						(int32_t*) &len, (void**) &pContainer);

				if (pContainer == NULL) {
					continue;
				}

				if (pContainer->dL_NGU_UP_TNLInformation.present
						== Ngap_UPTransportLayerInformation_PR_gTPTunnel) {
					struct Ngap_GTPTunnel *gTPTunnel =
							pContainer->dL_NGU_UP_TNLInformation.choice.gTPTunnel;
					uint32_t teid = 0;
					uint32_t teidv6 = 0;
					socklen_t addrlen = 0;
					socklen_t addrlenv6 = 0;
					agc_std_sockaddr_t addr;
					agc_std_sockaddr_t addrv6;

					teid = gTPTunnel->gTP_TEID.buf[0] << 24
							| gTPTunnel->gTP_TEID.buf[1] << 16
							| gTPTunnel->gTP_TEID.buf[2] << 8
							| gTPTunnel->gTP_TEID.buf[3];

					agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,
							"GNB_HandoverReqAck teid=%d buf=0x%x%x%x%x\n", teid,
							gTPTunnel->gTP_TEID.buf[0],
							gTPTunnel->gTP_TEID.buf[1],
							gTPTunnel->gTP_TEID.buf[2],
							gTPTunnel->gTP_TEID.buf[3]);

					agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,
							"GNB_HandoverReqAck transportLayerAddress size=%d buf=0x%x%x%x%x\n",
							gTPTunnel->transportLayerAddress.size,
							gTPTunnel->transportLayerAddress.buf[0],
							gTPTunnel->transportLayerAddress.buf[1],
							gTPTunnel->transportLayerAddress.buf[2],
							gTPTunnel->transportLayerAddress.buf[3]);

					if (gTPTunnel->transportLayerAddress.size == 4) // ipv4
					{
						struct sockaddr_in *local_addr1_v4 =
								(struct sockaddr_in*) &addr;
						local_addr1_v4->sin_family = AF_INET;

						memcpy(&local_addr1_v4->sin_addr,
								gTPTunnel->transportLayerAddress.buf, 4);
						addrlen = sizeof(struct sockaddr_in);
					} else if (gTPTunnel->transportLayerAddress.size == 16) //ipv6
					{
						struct sockaddr_in6 *local_addr1_v6 =
								(struct sockaddr_in6*) &addrv6;
						local_addr1_v6->sin6_family = AF_INET6;
						memcpy(&local_addr1_v6->sin6_addr,
								gTPTunnel->transportLayerAddress.buf, 16);
						addrlenv6 = sizeof(struct sockaddr_in6);
					} else if (20 == gTPTunnel->transportLayerAddress.size)//ipv4&ipv6
					{
						struct sockaddr_in *local_addr1_v4 =
								(struct sockaddr_in*) &addr;
						local_addr1_v4->sin_family = AF_INET;

						memcpy(&local_addr1_v4->sin_addr,
								gTPTunnel->transportLayerAddress.buf, 4);
						addrlen = sizeof(struct sockaddr_in);

						struct sockaddr_in6 *local_addr1_v6 =
								(struct sockaddr_in6*) &addrv6;
						local_addr1_v6->sin6_family = AF_INET6;
						memcpy(&local_addr1_v6->sin6_addr,
								gTPTunnel->transportLayerAddress.buf + 4, 16);
						addrlenv6 = sizeof(struct sockaddr_in6);

					}
					else
					{				
						agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"GNB_HandoverReqAck transportLayerAddress size length is invalid\n");
					}

					bMediaProcess = GetPduSessManager().processSessSetupRsp(info, pdu_resource_id, teid, addr, addrlen, teidv6, addrv6, addrlenv6);
					ngapDecode.FreeHandoverReqAckTransfer(pContainer);
				}
			}
		}

		if (!bMediaProcess)
			GetNsmManager().SendUpLayerSctp(info);
	}

	return true;
}

bool GNB_HandoverReqAckMedia(NgapMessage &info)
{
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "GNB_HandoverReqAckMedia\n");
	Ngap_SuccessfulOutcome_t *successfulOutcome = NULL;
	Ngap_HandoverRequestAcknowledge_t *HandoverRequestAcknowledge = NULL;
	Ngap_HandoverRequestAcknowledgeIEs *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
	Ngap_PDUSessionResourceAdmittedList_t *PDUSessionResourceAdmittedList = NULL;
	Ngap_PDUSessionResourceFailedToSetupListSURes_t *PDUSessionResourceFailedToSetupListSURes =NULL;
	Ngap_CriticalityDiagnostics_t* CriticalityDiagnostics=NULL;
	Ngap_HandoverRequestAcknowledgeTransfer_t *pResponseTransfer = NULL;
	OCTET_STRING_t *tranfer = NULL;

	int i = 0;
	bool bMediaProcess = false;
	NgapProtocol ngapDecode;

	successfulOutcome = info.ngapMessage.choice.successfulOutcome;
	HandoverRequestAcknowledge = &successfulOutcome->value.choice.HandoverRequestAcknowledge;

	for (i = 0; i < HandoverRequestAcknowledge->protocolIEs.list.count; i++) {
		ie = HandoverRequestAcknowledge->protocolIEs.list.array[i];
		switch (ie->id) {
		case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
			RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
			AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_PDUSessionResourceAdmittedList:
			PDUSessionResourceAdmittedList =
					&ie->value.choice.PDUSessionResourceAdmittedList;
			break;
		default:
			break;
		}
	}

	if (PDUSessionResourceAdmittedList == NULL) {
		return true;
	}

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "GNB_HandoverReqAckMedia ListCxtRes count=%d \n", PDUSessionResourceAdmittedList->list.count);

	for (i = 0; i < PDUSessionResourceAdmittedList->list.count; i++) {
		Ngap_PDUSessionResourceAdmittedItem *item = (Ngap_PDUSessionResourceAdmittedItem*) PDUSessionResourceAdmittedList->list.array[i];
		tranfer = &item->handoverRequestAcknowledgeTransfer;

		int32_t len = tranfer->size;
		uint8_t pdu_resource_id = item->pDUSessionID;
		ngapDecode.DecodeHandoverReqAckTransfer((char*) tranfer->buf, (int32_t*) &len, (void**) &pResponseTransfer);

		if (pResponseTransfer == NULL) {
			continue;
		}

		if (pResponseTransfer->dL_NGU_UP_TNLInformation.present == Ngap_UPTransportLayerInformation_PR_gTPTunnel) {
			uint32_t teid;
			uint32_t teid_v6;
			agc_std_sockaddr_t addr;
			agc_std_sockaddr_t addr_v6;
			GetPduSessManager().GetUpfGtpInfo(info.sessId, pdu_resource_id,
					addr, teid, addr_v6, teid_v6);

			struct Ngap_GTPTunnel *gTPTunnel = pResponseTransfer->dL_NGU_UP_TNLInformation.choice.gTPTunnel;

			if ((char*) gTPTunnel->gTP_TEID.buf == NULL) {
				gTPTunnel->gTP_TEID.buf = (uint8_t*) calloc(4, sizeof(uint8_t));
				gTPTunnel->gTP_TEID.size = 4;
			}

			gTPTunnel->gTP_TEID.buf[0] = teid >> 24;
			gTPTunnel->gTP_TEID.buf[1] = teid >> 16;
			gTPTunnel->gTP_TEID.buf[2] = teid >> 8;
			gTPTunnel->gTP_TEID.buf[3] = teid;

			if (gTPTunnel->transportLayerAddress.buf != NULL)
				free(gTPTunnel->transportLayerAddress.buf);

			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"GNB_HandoverReqAckMedia gTP IP.size=%d \n", gTPTunnel->transportLayerAddress.size);

			if (4 == gTPTunnel->transportLayerAddress.size)//ipv4 only
			{
				struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in*) &addr;
				gTPTunnel->transportLayerAddress.buf = (uint8_t*) calloc(4,
						sizeof(uint8_t));
				memcpy(gTPTunnel->transportLayerAddress.buf,
						&local_addr1_v4->sin_addr, 4);

			} else if(16 == gTPTunnel->transportLayerAddress.size)//ipv6 only
			{
				struct sockaddr_in6 *local_addr1_v6 =
						(struct sockaddr_in6*) &addr_v6;
				gTPTunnel->transportLayerAddress.buf = (uint8_t*) calloc(16,
						sizeof(uint8_t));
				memcpy(gTPTunnel->transportLayerAddress.buf,
						&local_addr1_v6->sin6_addr, 16);

			} else if(20 == gTPTunnel->transportLayerAddress.size) //ipv4 & ipv6
			{
				struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&addr;
				struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&addr_v6;
				gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(20, sizeof(uint8_t));
				memcpy(gTPTunnel->transportLayerAddress.buf, &local_addr1_v4->sin_addr, 4);
				memcpy(gTPTunnel->transportLayerAddress.buf+4, &local_addr1_v6->sin6_addr, 16); 	
			}
			else
			{
				agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"GNB_HandoverReqAckMedia gTP IP.size length is invalid\n");
			}	
		}

		if (tranfer != NULL) {
			if (tranfer->buf != NULL)
				free(tranfer->buf);
			tranfer->buf = (uint8_t*) calloc(200, sizeof(uint8_t));
			NgapProtocol ngapEncode;
			int32_t len = 0;
			ngapDecode.EncodeHandoverReqAckTransfer(
					(void*) pResponseTransfer, (char*) tranfer->buf,
					(int32_t*) &len);
			tranfer->size = len;
		}
		ngapDecode.FreeHandoverReqAckTransfer(pResponseTransfer);
	}

	//Forward to Amf
	GetNsmManager().SendUpLayerSctp(info);
	return true;
}

//lyb 2020-5-18
bool GNB_HandoverFailure(NgapMessage &info)
{
	Ngap_UnsuccessfulOutcome_t *unsuccessfulOutcome = NULL;
	Ngap_HandoverFailure_t *HandoverFailure = NULL;
	Ngap_HandoverFailureIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t iRanUeNgapId = 0;
	Ngap_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
	Ngap_Cause_t *ptCause = NULL;
	int i = 0;

	unsuccessfulOutcome = info.ngapMessage.choice.unsuccessfulOutcome;
	//d_assert(unsuccessfulOutcome, return,);

	HandoverFailure = &unsuccessfulOutcome->value.choice.HandoverFailure;
	//d_assert(HandoverFailure, return,);

	//		union Ngap_HandoverFailureIEs__Ngap_value_u {
	//			Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
	//			Ngap_Cause_t	 Cause;
	//			Ngap_CriticalityDiagnostics_t	 CriticalityDiagnostics;
	//		} choice;

	for (i = 0; i < HandoverFailure->protocolIEs.list.count; i++)
	{
		ie = HandoverFailure->protocolIEs.list.array[i];
		switch (ie->id)
		{
		case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
			AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_Cause:
			ptCause = &ie->value.choice.Cause;
			break;

		default:
			break;
		}
	}

	//Check sigGW role of handover.

	uintmax_t ueid;
	asn_INTEGER2umax(AMF_UE_NGAP_ID, &ueid);
	info.sessId = (uint32_t)ueid;

	NgapSession *sess = NULL;
	if (GetNgapSessManager().GetNgapSession(info.sessId, &sess) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_HandoverFailure getNgapSession fail to get ngap session, sessId=%d\n", info.sessId);
		return false;
	}

	EsmContext *esmContext;
	if (GetEsmManager().GetEsmContext(sess->gnb_sctp_index, &esmContext) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_HandoverFailure fail to get target esm context.\n");
		return false;
	}

	//If handover is under the same vGW, vGW handle the response, not forward.
	if (Ralation_RAN_Handover_IN_SAME_GW == sess->eHandoverflag)
	{
		//prepare info for sending msg to src RAN.
		info.amfUeId = sess->sessId;
		info.gnbUeId = sess->gnbUeId;
		info.ranNodeId = sess->gnbId;
		info.sockTarget = esmContext->sctp_index;

		sess->ResetHandoverInfo();

		for (int iPdu = 0; iPdu < MAX_PDU_SESSION_SIZE; iPdu++)
		{
			if (sess->vecPduSessionID[iPdu] == INVALID_PDU_SESSION_ID)
				continue;

			GetPduSessManager().resetHoTargetGnbAddr(info.sessId, sess->vecPduSessionID[iPdu]);
		}

		//Send HANDOVER PREPARATION FAILURE to source RAN
		GetEsmManager().SendHOPrepareFailure(info, ptCause);
	}
	else //The handover is handled by AMF, sigGW will forward msg.
	{
		//No RanUeNgapID in MSG.
		if (GNB_ConvertNgapId(info, &iRanUeNgapId, AMF_UE_NGAP_ID) == false)
		{
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_HandoverFailure fail to GNB_ConvertNgapId ngap session\n");
			return false;
		}

		//Forward to Amf
		GetNsmManager().SendUpLayerSctp(info);
	}

	return true;
}

//lyb 2020
bool GNB_HandoverNotify(NgapMessage &info)
{
	Ngap_InitiatingMessage_t *initiatingMessage = NULL;
	Ngap_HandoverNotify_t *HandoverNotify = NULL;
	Ngap_HandoverNotifyIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
	//d_assert(initiatingMessage, return,);

	HandoverNotify = &initiatingMessage->value.choice.HandoverNotify;
	//d_assert(HandoverNotify, return,);

	//			union Ngap_HandoverNotifyIEs__Ngap_value_u {
	//				Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
	//				Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
	//				Ngap_UserLocationInformation_t	 UserLocationInformation;
	//			} choice;

	for (i = 0; i < HandoverNotify->protocolIEs.list.count; i++)
	{
		ie = HandoverNotify->protocolIEs.list.array[i];
		switch (ie->id)
		{
		case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
			RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
			AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
			break;

		default:
			break;
		}
	}

	//d_assert(RanUeNgapId, return,);

	//Check sigGW role of handover.
	uintmax_t ueid;
	asn_INTEGER2umax(AMF_UE_NGAP_ID, &ueid);
	info.sessId = (uint32_t)ueid;

	NgapSession *sess = NULL;
	if (GetNgapSessManager().GetNgapSession(info.sessId, &sess) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_HandoverNotify getNgapSession fail to get ngap session, sessId=%d\n", info.sessId);
		return false;
	}

	EsmContext *esmContext;
	if (GetEsmManager().GetEsmContext(sess->gnb_sctp_index, &esmContext) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_HandoverNotify fail to get target esm context.\n");
		return false;
	}

	//If handover is under the same vGW, vGW handle the response, not forward.
	if (Ralation_RAN_Handover_IN_SAME_GW == sess->eHandoverflag)
	{
		uint8_t pdu_resource_id = 0;
		//UE CONTEXT RELEASE COMMAND
		//prepare info for sending msg to Src RAN;
		info.sessId = sess->sessId;
		info.amfUeId = sess->amfUeId;
		info.gnbUeId = sess->gnbUeId;
		info.ranNodeId = sess->gnbId;


		sess->gnbId = sess->TgtGnbId;
		sess->gnbUeId = sess->TgtGnbUeId;
		sess->gnb_sctp_index = info.sockTarget;
		
		info.sockTarget = esmContext->sctp_index;


		for (int iPdu = 0; iPdu < MAX_PDU_SESSION_SIZE; iPdu++)
		{
			if (sess->vecPduSessionID[iPdu] == INVALID_PDU_SESSION_ID)
				continue;

			GetPduSessManager().finishHo(sess->sessId, sess->vecPduSessionID[iPdu]);
		}

		//Send UE CONTEXT RELEASE COMMAND Msg to Src RAN
		GetEsmManager().SendUCRelCommand(info);

		//Need Update handover Status.
		sess->eHOState = HO_State_SendRelCommandWaitRelComplete;
	}
	else //The handover is handled by AMF, sigGW will forward msg.
	{
		if (GNB_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
		{
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_HandoverNotify fail to GNB_ConvertNgapId ngap session\n");
			return false;
		}

		//Forward to Amf
		GetNsmManager().SendUpLayerSctp(info);
	}

	return true;
}

bool GNB_PathSwitchRequestByMedia(NgapMessage &info)
{
	Ngap_InitiatingMessage_t *initiatingMessage = NULL;
	Ngap_PathSwitchRequest_t *pathSwitchRequest = NULL;

	Ngap_PathSwitchRequestIEs_t *ie = NULL;
	Ngap_PDUSessionResourceToBeSwitchedDLList_t *PDUSessionResourceToBeSwitchedDLList = NULL;
	Ngap_PathSwitchRequestTransfer_t *pathSwitchRequestTransfer = NULL;

	Ngap_RAN_UE_NGAP_ID_t *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;

	int i = 0;
	NgapProtocol ngapDecode;
	OCTET_STRING_t *tranfer = NULL;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
	pathSwitchRequest = &initiatingMessage->value.choice.PathSwitchRequest;

	for (i = 0; i < pathSwitchRequest->protocolIEs.list.count; i++)
	{
		ie = pathSwitchRequest->protocolIEs.list.array[i];
		switch (ie->id)
		{
		case Ngap_ProtocolIE_ID_id_PDUSessionResourceToBeSwitchedDLList:
			PDUSessionResourceToBeSwitchedDLList = &ie->value.choice.PDUSessionResourceToBeSwitchedDLList;
			break;
		default:
			break;
		}
	}

	if (PDUSessionResourceToBeSwitchedDLList == NULL)
	{
		return true;
	}

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "GNB_PathSwitchRequestByMedia ListCxtRes count=%d \n",
				   PDUSessionResourceToBeSwitchedDLList->list.count);

	for (i = 0; i < PDUSessionResourceToBeSwitchedDLList->list.count; i++)
	{
		Ngap_PDUSessionResourceToBeSwitchedDLItem_t *item =
			(Ngap_PDUSessionResourceToBeSwitchedDLItem_t *)PDUSessionResourceToBeSwitchedDLList->list.array[i];
		tranfer = &item->pathSwitchRequestTransfer;

		int32_t len = tranfer->size;
		char lastByteOrigin = tranfer->buf[len - 1];
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "GNB_PathSwitchRequestByMedia encoded len=%d, lastByteOrigin=%d\n", len, lastByteOrigin);



		/* see IE details in specs-38.413 #9.3.4.8 */
		ngapDecode.DecodePathSwtichReqTransfer((char *)tranfer->buf, (int32_t *)&len, (void **)&pathSwitchRequestTransfer);
		uint8_t pdu_resource_id = item->pDUSessionID;

		if (pathSwitchRequestTransfer != NULL)
		{
			uint32_t teid;
			uint32_t teid_v6;
			agc_std_sockaddr_t addr;
			agc_std_sockaddr_t addr_v6;
			GetPduSessManager().GetUpfGtpInfo(info.sessId, pdu_resource_id, addr, teid, addr_v6, teid_v6);

			/* change ip */
			struct Ngap_GTPTunnel *gTPTunnel = pathSwitchRequestTransfer->dL_NGU_UP_TNLInformation.choice.gTPTunnel;

			if ((char *)gTPTunnel->gTP_TEID.buf == NULL)
			{
				gTPTunnel->gTP_TEID.buf = (uint8_t *)calloc(4, sizeof(uint8_t));
				gTPTunnel->gTP_TEID.size = 4;
			}

			gTPTunnel->gTP_TEID.buf[0] = teid >> 24;
			gTPTunnel->gTP_TEID.buf[1] = teid >> 16;
			gTPTunnel->gTP_TEID.buf[2] = teid >> 8;
			gTPTunnel->gTP_TEID.buf[3] = teid;

			if (gTPTunnel->transportLayerAddress.buf != NULL)
				free(gTPTunnel->transportLayerAddress.buf);

			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"GNB_PathSwitchRequestByMedia gTP IP.size=%d \n", gTPTunnel->transportLayerAddress.size);

			if (4 == gTPTunnel->transportLayerAddress.size)//ipv4 only
			{
				struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&addr;
				gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(4, sizeof(uint8_t));
				memcpy(gTPTunnel->transportLayerAddress.buf, &local_addr1_v4->sin_addr, 4);

			}
			else if(16 == gTPTunnel->transportLayerAddress.size)//ipv6 only
			{
				struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&addr_v6;
				gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(16, sizeof(uint8_t));
				memcpy(gTPTunnel->transportLayerAddress.buf, &local_addr1_v6->sin6_addr, 16);
			}
			else if(20 == gTPTunnel->transportLayerAddress.size) //ipv4 & ipv6
			{
				struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&addr;
				struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&addr_v6;
				gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(20, sizeof(uint8_t));
				memcpy(gTPTunnel->transportLayerAddress.buf, &local_addr1_v4->sin_addr, 4);
				memcpy(gTPTunnel->transportLayerAddress.buf+4, &local_addr1_v6->sin6_addr, 16);		
			}
			else
			{
				agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"GNB_PathSwitchRequestByMedia gTP IP.size length is invalid\n");
			}

			if (tranfer != NULL)
			{
				if (tranfer->buf != NULL)
					free(tranfer->buf);
				tranfer->buf = (uint8_t *)calloc(100, sizeof(uint8_t));
				NgapProtocol ngapEncode;
				int32_t len = 0;
				ngapEncode.EncodePathSwtichReqTransfer((void *)pathSwitchRequestTransfer, (char *)tranfer->buf, (int32_t *)&len);
				
				agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "Ngap_PDUSessionResourceToBeSwitchedDLItem_t new len=%d, lastByteOrigin=%d\n", len, tranfer->buf[len - 1]);
				/* 将源IE的最后一个编码字节回填至处理后的编码字节序列末尾 */
				tranfer->buf[len++] = lastByteOrigin;
				tranfer->size = len;
			}
			ngapDecode.FreePatchSwitchReqTransfer(pathSwitchRequestTransfer);
		}
	}
	GetNsmManager().SendUpLayerSctp(info);
	return true;
}

//lyb 2020-5-18
bool GNB_PathSwitchRequest(NgapMessage &info)
{
	Ngap_InitiatingMessage_t *initiatingMessage = NULL;
	Ngap_PathSwitchRequest_t *PathSwitchRequest = NULL;
	Ngap_PathSwitchRequestIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t *srcAMF_UE_NGAP_ID = NULL;
	Ngap_PDUSessionResourceToBeSwitchedDLList_t *PDUSessionResourceToBeSwitchedDLList = NULL;
	int i = 0;
	NgapProtocol ngapDecode;
	uint32_t teid = 0;
	uint32_t teidv6 = 0;
	socklen_t addrlen = 0;
	socklen_t addrlenv6 = 0;
	agc_std_sockaddr_t addr;
	agc_std_sockaddr_t addrv6;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
	//d_assert(initiatingMessage, return,);

	PathSwitchRequest = &initiatingMessage->value.choice.PathSwitchRequest;
	//d_assert(PathSwitchRequest, return,);

	//		union Ngap_PathSwitchRequestIEs__Ngap_value_u {
	//			Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
	//			Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;                  Source AMF UE NGAP ID
	//			Ngap_UserLocationInformation_t	 UserLocationInformation;
	//			Ngap_UESecurityCapabilities_t	 UESecurityCapabilities;
	//			Ngap_PDUSessionResourceToBeSwitchedDLList_t	 PDUSessionResourceToBeSwitchedDLList;
	//			Ngap_PDUSessionResourceFailedToSetupListPSReq_t	 PDUSessionResourceFailedToSetupListPSReq;
	//		} choice;

	for (i = 0; i < PathSwitchRequest->protocolIEs.list.count; i++)
	{
		ie = PathSwitchRequest->protocolIEs.list.array[i];
		switch (ie->id)
		{
		case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
			RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
			break;
		case Ngap_ProtocolIE_ID_id_SourceAMF_UE_NGAP_ID:
			srcAMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_PDUSessionResourceToBeSwitchedDLList:
			PDUSessionResourceToBeSwitchedDLList = &ie->value.choice.PDUSessionResourceToBeSwitchedDLList;
			break;

		default:
			break;
		}
	}

	if (GNB_ConvertNgapId(info, RanUeNgapId, srcAMF_UE_NGAP_ID) == false) /* 把网关给小站的AFM_UEID，作为发给AMF的GNB)UEID */
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_PathSwitchRequest fail to GNB_ConvertNgapId ngap session\n");
		return false;
	}

	if (PDUSessionResourceToBeSwitchedDLList == NULL)
	{
		return false;
	}

	// uintmax_t ueid;
	// asn_INTEGER2umax(srcAMF_UE_NGAP_ID, &ueid);
	// info.sessId = (uint32_t)ueid;
	// info.gnbUeId = *RanUeNgapId;

	NgapSession *sess = NULL;
	if (GetNgapSessManager().GetNgapSession(info.sessId, &sess) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_PathSwitchRequest getNgapSession fail to get ngap session, sessId=%d\n", info.sessId);
		return false;
	}

	sess->pdu_resource_count = PDUSessionResourceToBeSwitchedDLList->list.count;
	sess->pdu_sess_rsp_count = 0;

	sess->gnbUeId = info.gnbUeId;
	sess->gnbId = info.ranNodeId;
	sess->gnb_sctp_index = info.sockSource;
	info.amfUeId = sess->amfUeId;
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "GNB_PathSwitchRequest ListCxtRes count=%d \n",
				   PDUSessionResourceToBeSwitchedDLList->list.count);

	for (i = 0; i < PDUSessionResourceToBeSwitchedDLList->list.count; i++)
	{
		Ngap_PDUSessionResourceToBeSwitchedDLItem_t *item =
			(Ngap_PDUSessionResourceToBeSwitchedDLItem_t *)PDUSessionResourceToBeSwitchedDLList->list.array[i];
		OCTET_STRING_t *tranfer = &item->pathSwitchRequestTransfer;
		Ngap_PathSwitchRequestTransfer_t *pContainer = NULL;

		int32_t len = tranfer->size;
		uint8_t pdu_resource_id = item->pDUSessionID;

		ngapDecode.DecodePathSwtichReqTransfer((char *)tranfer->buf, (int32_t *)&len, (void **)&pContainer);

		if (pContainer == NULL)
		{
			continue;
		}

		if (pContainer->dL_NGU_UP_TNLInformation.present == Ngap_UPTransportLayerInformation_PR_gTPTunnel)
		{
			Ngap_GTPTunnel_t *gTPTunnel = pContainer->dL_NGU_UP_TNLInformation.choice.gTPTunnel;

			teid = gTPTunnel->gTP_TEID.buf[0] << 24 | gTPTunnel->gTP_TEID.buf[1] << 16 | gTPTunnel->gTP_TEID.buf[2] << 8 | gTPTunnel->gTP_TEID.buf[3];

			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "GNB_PathSwitchRequest teid=%d buf=0x%x%x%x%x\n",
						   teid, gTPTunnel->gTP_TEID.buf[0], gTPTunnel->gTP_TEID.buf[1], gTPTunnel->gTP_TEID.buf[2], gTPTunnel->gTP_TEID.buf[3]);

			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "GNB_PathSwitchRequest transportLayerAddress size=%d buf=0x%x%x%x%x\n",
						   gTPTunnel->transportLayerAddress.size,
						   gTPTunnel->transportLayerAddress.buf[0],
						   gTPTunnel->transportLayerAddress.buf[1],
						   gTPTunnel->transportLayerAddress.buf[2],
						   gTPTunnel->transportLayerAddress.buf[3]);

			if (gTPTunnel->transportLayerAddress.size == 4) // ipv4
			{
				struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&addr;
				local_addr1_v4->sin_family = AF_INET;

				memcpy(&local_addr1_v4->sin_addr, gTPTunnel->transportLayerAddress.buf, 4);
				addrlen = sizeof(struct sockaddr_in);
			}
			else if (gTPTunnel->transportLayerAddress.size == 16) //ipv6
			{
				struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&addrv6;
				local_addr1_v6->sin6_family = AF_INET6;
				memcpy(&local_addr1_v6->sin6_addr, gTPTunnel->transportLayerAddress.buf, 16);
				addrlenv6 = sizeof(struct sockaddr_in6);
			}
			else if (20 == gTPTunnel->transportLayerAddress.size)//ipv4&ipv6
			{
				struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&addr;
				local_addr1_v4->sin_family = AF_INET;

				memcpy(&local_addr1_v4->sin_addr, gTPTunnel->transportLayerAddress.buf, 4);
				addrlen = sizeof(struct sockaddr_in);

				struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&addrv6;
				local_addr1_v6->sin6_family = AF_INET6;
				memcpy(&local_addr1_v6->sin6_addr, gTPTunnel->transportLayerAddress.buf + 4, 16);
				addrlenv6 = sizeof(struct sockaddr_in6);
			}

			GetPduSessManager().processSessSetupRsp(info, pdu_resource_id, teid, addr, addrlen, teidv6, addrv6, addrlenv6);
		}
		ngapDecode.FreePatchSwitchReqTransfer(pContainer);
	}

	return true;
}

//lyb 2020-5-19
bool GNB_ULRanStatusTsfer(NgapMessage &info)
{
	Ngap_InitiatingMessage_t *initiatingMessage = NULL;
	Ngap_UplinkRANStatusTransfer_t *UplinkRANStatusTransfer = NULL;
	Ngap_UplinkRANStatusTransferIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
	Ngap_RANStatusTransfer_TransparentContainer_t *RANStatusTransfer_TransparentContainer = NULL;
	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
	//d_assert(initiatingMessage, return,);

	UplinkRANStatusTransfer = &initiatingMessage->value.choice.UplinkRANStatusTransfer;
	//d_assert(UplinkRANStatusTransfer, return,);

	//		union Ngap_UplinkRANStatusTransferIEs__Ngap_value_u {
	//			Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
	//			Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
	//			Ngap_RANStatusTransfer_TransparentContainer_t	 RANStatusTransfer_TransparentContainer;
	//		} choice;

	for (i = 0; i < UplinkRANStatusTransfer->protocolIEs.list.count; i++)
	{
		ie = UplinkRANStatusTransfer->protocolIEs.list.array[i];
		switch (ie->id)
		{
		case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
			RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
			AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_RANStatusTransfer_TransparentContainer:
			RANStatusTransfer_TransparentContainer = &ie->value.choice.RANStatusTransfer_TransparentContainer;
			break;

		default:
			break;
		}
	}

	//d_assert(RanUeNgapId, return,);

	//Check sigGW role of handover.

	uintmax_t ueid;
	asn_INTEGER2umax(AMF_UE_NGAP_ID, &ueid);
	info.sessId = (uint32_t)ueid;

	NgapSession *sess = NULL;
	if (GetNgapSessManager().GetNgapSession(info.sessId, &sess) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_ULRanStatusTsfer getNgapSession fail to get ngap session, sessId=%d\n", info.sessId);
		return false;
	}

	//If handover is under the same vGW, vGW handle the response, not forward.
	if (Ralation_RAN_Handover_IN_SAME_GW == sess->eHandoverflag)
	{
		//prepare info for sending msg to Tgt RAN.
		info.gnbUeId = sess->TgtGnbUeId;
		info.ranNodeId = sess->TgtGnbId; //Need to check.
		info.amfUeId = sess->sessId;

		EsmContext *esmContext;
		if (GetEsmManager().GetEsmContext(sess->TgtGnbId, &esmContext) == true)
		{
			info.ranNodeId = esmContext->ranNodeId;
			info.sockTarget = esmContext->sctp_index;
		}

		//Send DLRanStatusTsfer to target RAN.
		GetEsmManager().SendDLRanStatusTsfer(info, RANStatusTransfer_TransparentContainer);
	}
	else //The handover is handled by AMF, sigGW will forward msg.
	{
		if (GNB_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
		{
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_ULRanStatusTsfer fail to GNB_ConvertNgapId ngap session\n");
			return false;
		}

		//Forward to Amf
		GetNsmManager().SendUpLayerSctp(info);
	}

	return true;
}

//lyb 2020-5-18
bool GNB_HandoverCancel(NgapMessage &info)
{
	Ngap_InitiatingMessage_t *initiatingMessage = NULL;
	Ngap_HandoverCancel_t *HandoverCancel = NULL;
	Ngap_HandoverCancelIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
	//d_assert(initiatingMessage, return,);

	HandoverCancel = &initiatingMessage->value.choice.HandoverCancel;
	//d_assert(HandoverCancel, return,);

	//		union Ngap_HandoverCancelIEs__Ngap_value_u {
	//			Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
	//			Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
	//			Ngap_Cause_t	 Cause;
	//		} choice;

	for (i = 0; i < HandoverCancel->protocolIEs.list.count; i++)
	{
		ie = HandoverCancel->protocolIEs.list.array[i];
		switch (ie->id)
		{
		case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
			RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
			AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
			break;

		default:
			break;
		}
	}

	//d_assert(RanUeNgapId, return,);

	//Check sigGW role of handover.

	uintmax_t ueid;
	asn_INTEGER2umax(AMF_UE_NGAP_ID, &ueid);
	info.sessId = (uint32_t)ueid;
	info.gnbUeId = *((uint32_t *)RanUeNgapId);
	NgapSession *sess = NULL;
	if (GetNgapSessManager().GetNgapSession(info.sessId, &sess) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_HandoverCancel getNgapSession fail to get ngap session, sessId=%d\n", info.sessId);
		return false;
	}

	//If handover is under the same vGW, vGW handle the response, not forward.
	if (Ralation_RAN_Handover_IN_SAME_GW == sess->eHandoverflag)
	{
		//Send Handover Cancel Acknowledge to Source RAN. Under same ESM context and Ngap session.

		sess->ResetHandoverInfo();

		info.sockTarget = info.sockSource;

		for (int iPdu = 0; iPdu < MAX_PDU_SESSION_SIZE; iPdu++)
		{
			if (sess->vecPduSessionID[iPdu] == INVALID_PDU_SESSION_ID)
				continue;

			GetPduSessManager().resetHoTargetGnbAddr(info.sessId, sess->vecPduSessionID[iPdu]);
		}

		GetEsmManager().SendHandoverCancelAck(info);
	}
	else //The handover is handled by AMF, sigGW will forward msg.
	{
		if (GNB_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
		{
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_HandoverCancel fail to GNB_ConvertNgapId ngap session\n");
			return false;
		}

		//Forward to Amf
		GetNsmManager().SendUpLayerSctp(info);
	}

	return true;
}

bool GNB_AMFStatusIndication(NgapMessage &info)
{
	return true;
}

bool GNB_InitialUEMessage(NgapMessage &info)
{
	Ngap_InitiatingMessage_t *initiatingMessage = NULL;
	Ngap_InitialUEMessage_t *N2InitialUEMessage = NULL;
	Ngap_InitialUEMessage_IEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t *RanUeNgapId = NULL;
	Ngap_NAS_PDU_t *NAS_PDU = NULL;
	int i = 0;

	if (info.ranNodeId.getRanNodeID() == INVALID_RANNODEID)
	{
		return false;
	}

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
	//d_assert(initiatingMessage, return,);

	N2InitialUEMessage = &initiatingMessage->value.choice.InitialUEMessage;
	//d_assert(N2InitialUEMessage, return,);

	//	union Ngap_InitialUEMessage_IEs__Ngap_value_u {
	//		Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
	//		Ngap_NAS_PDU_t	 NAS_PDU;
	//		Ngap_UserLocationInformation_t	 UserLocationInformation;
	//		Ngap_FiveG_S_TMSI_t  FiveG_S_TMSI;
	//		Ngap_AMFSetID_t  AMFSetID;
	//		Ngap_UEContextRequest_t  UEContextRequest;
	//		Ngap_AllowedNSSAI_t  AllowedNSSAI;
	//	} choice;

	for (i = 0; i < N2InitialUEMessage->protocolIEs.list.count; i++)
	{
		ie = N2InitialUEMessage->protocolIEs.list.array[i];
		switch (ie->id)
		{
		case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
			RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_NAS_PDU:
			NAS_PDU = &ie->value.choice.NAS_PDU;

			GetNgapSessManager().DecodeNasPdu(info, NAS_PDU->buf, NAS_PDU->size);
		default:
			break;
		}
	}

	info.gnbUeId = *RanUeNgapId;

	NgapSession *sess = NULL;
	// try to find where session is exist
	//if (GetNgapSessManager().GetNgapSession(info.ranNodeId, info.gnbUeId, &sess) == false)
	{
		// no session, then new one
		info.amfId = GetAsmManager().SelectAmf(info.idSigGW);
		if (GetNgapSessManager().NewNgapSession(info, &sess) == false)
		{
			return false;
		}
	}

	info.sessId = sess->sessId;
	info.amfId = sess->amfId;
	info.sockTarget = GetAsmManager().GetTargetSock(info.amfId);
	*RanUeNgapId = sess->sessId;

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,
				   "GNB_InitialUEMessage SessRanUeID=%d, SessAmfUeID=%d sessId=%d sctp_index=%d \n",
				   sess->gnbUeId, sess->amfUeId, sess->sessId, sess->gnb_sctp_index);
	//Forward to Amf
	GetNsmManager().SendUpLayerSctp(info);

	return true;
}

bool GNB_UplinkNASTransport(NgapMessage &info)
{
	Ngap_InitiatingMessage_t *initiatingMessage = NULL;
	Ngap_UplinkNASTransport_t *N2UplinkNASTransport = NULL;
	Ngap_UplinkNASTransport_IEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
	Ngap_NAS_PDU_t *NAS_PDU = NULL;
	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
	//d_assert(initiatingMessage, return,);

	N2UplinkNASTransport = &initiatingMessage->value.choice.UplinkNASTransport;
	//d_assert(N2UplinkNASTransport, return,);

	//	union Ngap_UplinkNASTransport_IEs__Ngap_value_u {
	//		Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
	//		Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
	//		Ngap_NAS_PDU_t	 NAS_PDU;
	//		Ngap_UserLocationInformation_t	 UserLocationInformation;
	//	} choice;

	for (i = 0; i < N2UplinkNASTransport->protocolIEs.list.count; i++)
	{
		ie = N2UplinkNASTransport->protocolIEs.list.array[i];
		switch (ie->id)
		{
		case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
			RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
			AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_NAS_PDU:
			NAS_PDU = &ie->value.choice.NAS_PDU;

			GetNgapSessManager().DecodeNasPdu(info, NAS_PDU->buf, NAS_PDU->size);
		default:
			break;
		}
	}

	if (GNB_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_UplinkNASTransport fail to GNB_ConvertNgapId ngap session RanUeNgapId=%d\n", *RanUeNgapId);
		return false;
	}

	//Forward to Amf
	GetNsmManager().SendUpLayerSctp(info);
	return true;
}

bool GNB_UERadioCapInfoInd(NgapMessage &info)
{
	Ngap_InitiatingMessage_t *initiatingMessage = NULL;
	Ngap_UERadioCapabilityInfoIndication_t *UERadioCapabilityInfoIndication = NULL;
	Ngap_UERadioCapabilityInfoIndicationIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
	//d_assert(initiatingMessage, return,);

	UERadioCapabilityInfoIndication = &initiatingMessage->value.choice.UERadioCapabilityInfoIndication;
	//d_assert(N2UplinkNASTransport, return,);

	//		union Ngap_UERadioCapabilityInfoIndicationIEs__Ngap_value_u {
	//			Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
	//			Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
	//			Ngap_UERadioCapability_t	 UERadioCapability;
	//			Ngap_UERadioCapabilityForPaging_t	 UERadioCapabilityForPaging;
	//		} choice;

	for (i = 0; i < UERadioCapabilityInfoIndication->protocolIEs.list.count; i++)
	{
		ie = UERadioCapabilityInfoIndication->protocolIEs.list.array[i];
		switch (ie->id)
		{
		case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
			RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
			AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
			break;

		default:
			break;
		}
	}

	if (GNB_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_UERadioCapInfoInd fail to GNB_ConvertNgapId ngap session\n");
		return false;
	}

	//Forward to Amf
	GetNsmManager().SendUpLayerSctp(info);

	return true;
}

bool GNB_NASNonDeliveryIndication(NgapMessage &info)
{
	Ngap_InitiatingMessage_t *initiatingMessage = NULL;
	Ngap_NASNonDeliveryIndication_t *N2NASNonDeliveryIndication = NULL;
	Ngap_NASNonDeliveryIndication_IEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
	Ngap_NAS_PDU_t *NAS_PDU = NULL;
	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
	//d_assert(initiatingMessage, return,);

	N2NASNonDeliveryIndication = &initiatingMessage->value.choice.NASNonDeliveryIndication;
	//d_assert(N2NASNonDeliveryIndication, return,);

	//			union Ngap_NASNonDeliveryIndication_IEs__Ngap_value_u {
	//				Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
	//				Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
	//				Ngap_NAS_PDU_t	 NAS_PDU;
	//				Ngap_Cause_t	 Cause;
	//			} choice;

	for (i = 0; i < N2NASNonDeliveryIndication->protocolIEs.list.count; i++)
	{
		ie = N2NASNonDeliveryIndication->protocolIEs.list.array[i];
		switch (ie->id)
		{
		case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
			RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
			AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_NAS_PDU:
			NAS_PDU = &ie->value.choice.NAS_PDU;
			GetNgapSessManager().DecodeNasPdu(info, NAS_PDU->buf, NAS_PDU->size);
		default:
			break;
		}
	}

	if (GNB_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_NASNonDeliveryIndication fail to GNB_ConvertNgapId ngap session\n");
		return false;
	}

	//Forward to Amf
	GetNsmManager().SendUpLayerSctp(info);
	return true;
}

bool GNB_Reset(NgapMessage &info)
{
	Ngap_InitiatingMessage_t *initiatingMessage = NULL;
	Ngap_NGReset_t *NGReset = NULL;
	Ngap_NGResetIEs_t *ie = NULL;
	Ngap_ResetType_t *ResetType = NULL;
	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
	NGReset = &initiatingMessage->value.choice.NGReset;

	for (i = 0; i < NGReset->protocolIEs.list.count; i++)
	{
		ie = NGReset->protocolIEs.list.array[i];
		switch (ie->id)
		{
		case Ngap_ProtocolIE_ID_id_ResetType:
			ResetType = &ie->value.choice.ResetType;
			break;
		}
	}

	if (ResetType->present == Ngap_ResetType_PR_nG_Interface)
	{
		// clear all the ngap sess and pdu sess
		GetNgapSessManager().DeleteAllGnbSession(info.ranNodeId);
	}
	else if (ResetType->present == Ngap_ResetType_PR_partOfNG_Interface)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_Reset part of NG isn't process.\n");
	}

	GetEsmManager().SendResetAck(info);

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "GNB_Reset finished.\n");
	return true;
}

bool GNB_ResetAck(NgapMessage &info)
{
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "GNB_ResetAck finished.\n");
	return true;
}

//lyb 2020-5-13
bool GNB_RRCInactTrstRpt(NgapMessage &info)
{
	Ngap_InitiatingMessage_t *initiatingMessage = NULL;
	Ngap_RRCInactiveTransitionReport_t *RRCInactiveTransitionReport = NULL;
	Ngap_RRCInactiveTransitionReportIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
	//d_assert(initiatingMessage, return,);

	RRCInactiveTransitionReport = &initiatingMessage->value.choice.RRCInactiveTransitionReport;
	//d_assert(RRCInactiveTransitionReport, return,);

	//		union Ngap_RRCInactiveTransitionReportIEs__Ngap_value_u {
	//			Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
	//			Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
	//			Ngap_RRCState_t	 RRCState;
	//			Ngap_UserLocationInformation_t	 UserLocationInformation;
	//		} choice;

	for (i = 0; i < RRCInactiveTransitionReport->protocolIEs.list.count; i++)
	{
		ie = RRCInactiveTransitionReport->protocolIEs.list.array[i];
		switch (ie->id)
		{
		case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
			RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
			AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
			break;

		default:
			break;
		}
	}

	//d_assert(RanUeNgapId, return,);

	if (GNB_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_RRCInactTrstRpt fail to GNB_ConvertNgapId ngap session\n");
		return false;
	}

	//Forward to Amf
	GetNsmManager().SendUpLayerSctp(info);

	return true;
}

bool GNB_ErrorIndication(NgapMessage &info)
{
	Ngap_InitiatingMessage_t *initiatingMessage = NULL;
	Ngap_ErrorIndication_t *N2ErrorIndication = NULL;
	Ngap_ErrorIndicationIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
	//d_assert(initiatingMessage, return,);

	N2ErrorIndication = &initiatingMessage->value.choice.ErrorIndication;
	//d_assert(N2ErrorIndication, return,);

	//	union Ngap_ErrorIndicationIEs__Ngap_value_u {
	//		Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
	//		Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
	//		Ngap_Cause_t	 Cause;
	//		Ngap_CriticalityDiagnostics_t	 CriticalityDiagnostics;
	//	} choice;

	for (i = 0; i < N2ErrorIndication->protocolIEs.list.count; i++)
	{
		ie = N2ErrorIndication->protocolIEs.list.array[i];
		switch (ie->id)
		{
		case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
			RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
			AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
			break;

		default:
			break;
		}
	}

	//d_assert(RanUeNgapId, return,);
	if (NULL == RanUeNgapId || NULL == AMF_UE_NGAP_ID)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_ErrorIndication RanUeNgapId or AMF_UE_NGAP_ID is null\n");
		return false;
	}

	if (GNB_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_ErrorIndication fail to GNB_ConvertNgapId ngap session\n");
		return false;
	}

	//Forward to Amf
	GetNsmManager().SendUpLayerSctp(info);

	return true;
}

bool GNB_SetupReq(NgapMessage &info)
{
	uint32_t cause = NGAP_CHECK_RNI_OK;
	EsmContext *newContext = new EsmContext;
	Ngap_SupportedTAList_t *SupportedTAList = NULL;
	uint32_t uTac;
	Ngap_NGSetupRequest_t *NGSetupRequest = &info.ngapMessage.choice.initiatingMessage->value.choice.NGSetupRequest;
	stSIG_TAC_RAN stTacRan;

	newContext->sctp_index = info.sockSource;
	newContext->idSigGW = info.idSigGW;

	stTacRan.uSctpIndex = newContext->sctp_index;

	for (int i = 0; i < NGSetupRequest->protocolIEs.list.count; i++)
	{
		Ngap_GlobalRANNodeID_t *GlobalRANNodeID = NULL;
		Ngap_NGSetupRequestIEs_t *ie = NGSetupRequest->protocolIEs.list.array[i];
		switch (ie->id)
		{
		case Ngap_ProtocolIE_ID_id_PagingDRX:
			newContext->pagingDRX = ie->value.choice.PagingDRX;
			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "GNB_SetupReq PagingDRX:%d\n", newContext->pagingDRX);
			break;
		case Ngap_ProtocolIE_ID_id_RANNodeName:
			newContext->gnbName.assign((char *)ie->value.choice.RANNodeName.buf, ie->value.choice.RANNodeName.size);
			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "GNB_SetupReq RANNodeName:%s\n", newContext->gnbName.c_str());
			break;
		case Ngap_ProtocolIE_ID_id_SupportedTAList:
			SupportedTAList = &ie->value.choice.SupportedTAList;
			break;
		case Ngap_ProtocolIE_ID_id_GlobalRANNodeID:
		{
			GlobalRANNodeID = &ie->value.choice.GlobalRANNodeID;
			cause = HandleGlobalRANNodeID(GlobalRANNodeID, newContext);
			if (cause != NGAP_CHECK_RNI_OK)
			{
				GetEsmManager().SendNgSetupFailure(cause, info);
				delete newContext;
				return false;
			}
			break;
		}
		}
	}

	if (SupportedTAList == NULL)
	{
		delete newContext;
		GetEsmManager().SendNgSetupFailure(NGAP_CHECK_RNI_UNKNOW_ERROR, info);
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_SetupReq SupportedTAList is missed.\n");
		return false;
	}

	for (int i = 0; i < SupportedTAList->list.count; i++)
	{
		// todo: store TAitem
		Ngap_SupportedTAItem_t *item = NULL;
		item = (Ngap_SupportedTAItem_t *)SupportedTAList->list.array[i];

		uTac = ((item->tAC.buf[0]) << 16) + ((item->tAC.buf[1]) << 8) + item->tAC.buf[2];
		newContext->tac.push_back(uTac);
		stTacRan.uTAC = uTac;

		//global_tac_ran.push_back(stTacRan);
	}

	info.ranNodeId = newContext->ranNodeId;
	GetEsmManager().Setup(info, newContext);
	GetEsmManager().SendNgSetupResponse(info);

	return true;
}

bool GNB_LocationReport(NgapMessage &info)
{
	Ngap_InitiatingMessage_t *initiatingMessage = NULL;
	Ngap_LocationReport_t *LocationReport = NULL;
	Ngap_LocationReportIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
	LocationReport = &initiatingMessage->value.choice.LocationReport;
	for (i = 0; i < LocationReport->protocolIEs.list.count; i++)
	{
		ie = LocationReport->protocolIEs.list.array[i];
		switch (ie->id)
		{
		case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
			RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
			AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
			break;

		default:
			break;
		}
	}

	if (GNB_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GNB_LocationReport fail to GNB_ConvertNgapId ngap session\n");
		return false;
	}

	//Forward to Amf
	GetNsmManager().SendUpLayerSctp(info);
	return true;
}

bool GNB_RANConfigurationUpdate(NgapMessage &info)
{
	NgapMessage ack;

	Ngap_SuccessfulOutcome_t *successfulOutcome;

	ack.gnbUeId = INVALIDE_ID;
	ack.amfUeId = INVALIDE_ID_X64;
	ack.sessId = INVALIDE_ID;
	ack.sctpIndex = info.sctpIndex;
	ack.ranNodeId = info.ranNodeId;
	ack.sockSource = info.sockSource;
	ack.sockTarget = info.sockSource;
	ack.stream_no = 0;

	memset(&ack.ngapMessage, 0, sizeof(Ngap_NGAP_PDU_t));
	ack.PDUChoice = Ngap_NGAP_PDU_PR_successfulOutcome;
	ack.ProcCode = Ngap_ProcedureCode_id_RANConfigurationUpdate;

	ack.ngapMessage.present = Ngap_NGAP_PDU_PR_successfulOutcome;
	ack.ngapMessage.choice.successfulOutcome = (Ngap_SuccessfulOutcome_t *)calloc(1, sizeof(Ngap_SuccessfulOutcome_t));

	successfulOutcome = ack.ngapMessage.choice.successfulOutcome;
	successfulOutcome->procedureCode = Ngap_ProcedureCode_id_RANConfigurationUpdate;
	successfulOutcome->criticality = Ngap_Criticality_reject;
	successfulOutcome->value.present = Ngap_SuccessfulOutcome__value_PR_RANConfigurationUpdateAcknowledge;

	GetNsmManager().SendDownLayerSctp(ack);
	return true;
}

bool Epm_Init()
{
	EPM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_successfulOutcome, Ngap_ProcedureCode_id_PDUSessionResourceSetup, &GNB_PDUSessionSetupRsp);
	EPM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, Ngap_ProcedureCode_id_PDUSessionResourceModifyIndication, &GNB_PDUSessionModifyInd);
	EPM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_successfulOutcome, Ngap_ProcedureCode_id_PDUSessionResourceModify, &GNB_PDUSessionModifyRsp);
	EPM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_successfulOutcome, Ngap_ProcedureCode_id_PDUSessionResourceRelease, &GNB_PDUSessionReleaseRsp);
	EPM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, Ngap_ProcedureCode_id_PDUSessionResourceNotify, &GNB_PDUSessionRsrNotify);
	EPM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_successfulOutcome, Ngap_ProcedureCode_id_InitialContextSetup, &GNB_InitialContextSetupSuccessRsp);
	EPM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_unsuccessfulOutcome, Ngap_ProcedureCode_id_InitialContextSetup, &GNB_InitialContextSetupFailureRsp);
	EPM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_successfulOutcome, Ngap_ProcedureCode_id_UEContextRelease, &GNB_UEContextReleaseComplete);
	EPM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, Ngap_ProcedureCode_id_UEContextReleaseRequest, &GNB_UEContextReleaseReq);
	EPM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_successfulOutcome, Ngap_ProcedureCode_id_UEContextModification, &GNB_UEContextModificationSuccessRsp);
	EPM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_unsuccessfulOutcome, Ngap_ProcedureCode_id_UEContextModification, &GNB_UEContextModificationFailureRsp);
	EPM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, Ngap_ProcedureCode_id_HandoverPreparation, &GNB_HandoverRequired);
	EPM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_successfulOutcome, Ngap_ProcedureCode_id_HandoverResourceAllocation, &GNB_HandoverReqAck);
	EPM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_unsuccessfulOutcome, Ngap_ProcedureCode_id_HandoverResourceAllocation, &GNB_HandoverFailure);
	EPM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, Ngap_ProcedureCode_id_PathSwitchRequest, &GNB_PathSwitchRequest);
	EPM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, Ngap_ProcedureCode_id_HandoverNotification, &GNB_HandoverNotify);
	EPM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, Ngap_ProcedureCode_id_HandoverCancel, &GNB_HandoverCancel);
	EPM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, Ngap_ProcedureCode_id_UplinkRANStatusTransfer, &GNB_ULRanStatusTsfer);
	EPM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, Ngap_ProcedureCode_id_AMFStatusIndication, &GNB_AMFStatusIndication);
	EPM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, Ngap_ProcedureCode_id_InitialUEMessage, &GNB_InitialUEMessage);
	EPM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, Ngap_ProcedureCode_id_UplinkNASTransport, &GNB_UplinkNASTransport);
	EPM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, Ngap_ProcedureCode_id_UERadioCapabilityInfoIndication, &GNB_UERadioCapInfoInd);
	EPM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, Ngap_ProcedureCode_id_NGReset, &GNB_Reset);
	EPM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_successfulOutcome, Ngap_ProcedureCode_id_NGReset, &GNB_ResetAck);
	EPM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, Ngap_ProcedureCode_id_RRCInactiveTransitionReport, &GNB_RRCInactTrstRpt);
	EPM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, Ngap_ProcedureCode_id_ErrorIndication, &GNB_ErrorIndication);
	EPM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, Ngap_ProcedureCode_id_NASNonDeliveryIndication, &GNB_NASNonDeliveryIndication);
	EPM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, Ngap_ProcedureCode_id_NGSetup, &GNB_SetupReq);
	EPM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, Ngap_ProcedureCode_id_LocationReport, &GNB_LocationReport);
	EPM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, Ngap_ProcedureCode_id_RANConfigurationUpdate, &GNB_RANConfigurationUpdate);

	return true;
}
