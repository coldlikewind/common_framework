
#include "NgapMessage.h"
#include "AsmManager.h"
#include "ApmManager.h"
#include "NgapSession.h"
#include "NsmManager.h"
#include "EsmManager.h"
#include "NgapSessManager.h"
#include "PduSessManager.h"
#include <agc.h>
#include <CfgStruct.h>


extern vector<stSIG_TAC_RAN> global_tac_ran;


#define APM_REGISTER_APPCALLBACK(pduChoise, procedureCode, func) \
{	\
	GetNsmManager().RegisterUpLayerRecv((pduChoise), (procedureCode), (func));\
}


static bool AMF_ConvertNgapId(NgapMessage& info, Ngap_RAN_UE_NGAP_ID_t *ranUeId, Ngap_AMF_UE_NGAP_ID_t *amfUeId)
{
	if(NULL == ranUeId || NULL == amfUeId)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AMF_ConvertNgapId ranUeId addr = (%x) , amfUeId addr = (%x).\n",ranUeId,amfUeId);
		return false;
	}

	info.sessId = *ranUeId;
	uintmax_t ueid;
	//info.amfUeId = U64FromBuffer(amfUeId->buf, amfUeId->size);
	asn_INTEGER2umax(amfUeId, &ueid);
	info.amfUeId = ueid;
	
	NgapSession *sess =  NULL;
	if (GetNgapSessManager().GetNgapSession(info.sessId, &sess) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AMF_ConvertNgapId fail to get ngap session, sessId=%d amfUeId=%d\n", info.sessId, info.amfUeId);
		return false;
	}
	
	EsmContext *esmContext = NULL;
	if (GetEsmManager().GetEsmContext(sess->gnb_sctp_index, &esmContext) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AMF_ConvertNgapId fail to get esm context.\n");
		return false;
	}	

	sess->amfUeId = info.amfUeId;
	asn_int642INTEGER(amfUeId, (uint64_t)info.sessId);
	*ranUeId = info.gnbUeId = sess->gnbUeId;
	info.ranNodeId = sess->gnbId;
	info.stream_no = sess->stream_no;
	info.sockTarget = esmContext->sctp_index;

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "Apm--AMF_ConvertNgapId SessRanUeID=%d, SessAmfUeID=%d sess.sessId %d info.sessId %d\n", 
		sess->gnbUeId, sess->amfUeId, sess->sessId, info.sessId);
	
	return true;
}


bool AMF_PDUResourceSetupReq_UpdatePduSess(NgapMessage& info, Ngap_PDUSessionResourceSetupListSUReq_t* PDUSessionResourceSetupListSUReq)
{
	int i = 0;
	int j = 0;
	int ivec = 0;
	bool bMediaProcess = false;
	NgapProtocol ngapDecode;
	Ngap_PDUSessionResourceSetupRequestTransfer_t* pRequetTransfer = NULL;
	Ngap_PDUSessionResourceSetupRequestTransferIEs_t *pduie = NULL;
	Ngap_UPTransportLayerInformation_t	 *UPTransportLayerInformation = NULL;
	Ngap_PDUSessionAggregateMaximumBitRate_t	 *PDUSessionAggregateMaximumBitRate = NULL;
	Ngap_QosFlowSetupRequestList_t	 *QosFlowSetupRequestList = NULL;
   	Ngap_PDUSessionResourceSetupItemSUReq_t *item = NULL;
	pdu_session_resource_t pdu_resource;
	uint32_t teid = 0;
	uint32_t teidv6 = 0;
	socklen_t addrlen = 0;
	socklen_t addrlenv6 = 0;
	agc_std_sockaddr_t addr;
	agc_std_sockaddr_t addrv6;
	uint8_t	pdu_resource_id = 0;
	
	NgapSession *sess =   NULL;
	if (GetNgapSessManager().GetNgapSession(info.sessId, &sess) == false || sess == NULL)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"AMF_PDUResourceSetupReq_UpdatePduSess fail to get sess=%d \n", info.sessId);
		return false;
	}
	
	memset(&pdu_resource, 0, sizeof(pdu_session_resource_t));
	
	if (PDUSessionResourceSetupListSUReq == NULL)
		return false;

	sess->pdu_resource_count = PDUSessionResourceSetupListSUReq->list.count;
	sess->pdu_sess_rsp_count = 0;

    for (i = 0; i < PDUSessionResourceSetupListSUReq->list.count; i++)
    {
    	item = (Ngap_PDUSessionResourceSetupItemSUReq_t *)PDUSessionResourceSetupListSUReq->list.array[i];
    	OCTET_STRING_t *tranfer = &item->pDUSessionResourceSetupRequestTransfer;

    	int32_t len = tranfer->size;
		pdu_resource_id = item->pDUSessionID;
    	ngapDecode.DecodeResourceSetupRequestTransfer((char*)tranfer->buf, (int32_t *)&len, (void **)&pRequetTransfer);

		if (pRequetTransfer == NULL || item == NULL) 
		{	
			continue;
		}
		
		for (j = 0; j < pRequetTransfer->protocolIEs.list.count; j++)
		{
			pduie = pRequetTransfer->protocolIEs.list.array[j];
			switch (pduie->id)
			{
				case Ngap_ProtocolIE_ID_id_PDUSessionAggregateMaximumBitRate:
					PDUSessionAggregateMaximumBitRate = &pduie->value.choice.PDUSessionAggregateMaximumBitRate;
					break;
				case Ngap_ProtocolIE_ID_id_QosFlowSetupRequestList:
					QosFlowSetupRequestList = &pduie->value.choice.QosFlowSetupRequestList;
					break;
				case Ngap_ProtocolIE_ID_id_UL_NGU_UP_TNLInformation:
					UPTransportLayerInformation = &pduie->value.choice.UPTransportLayerInformation;
					break;
			}
		}

		if (UPTransportLayerInformation != NULL 
			&& UPTransportLayerInformation->present == Ngap_UPTransportLayerInformation_PR_gTPTunnel)
		{		
			struct Ngap_GTPTunnel	*gTPTunnel = UPTransportLayerInformation->choice.gTPTunnel;

			teidv6 = teid = ((uint32_t)gTPTunnel->gTP_TEID.buf[0] << 24)
				| ((uint32_t)gTPTunnel->gTP_TEID.buf[1] << 16)
				| ((uint32_t)gTPTunnel->gTP_TEID.buf[2] << 8)
				| ((uint32_t)gTPTunnel->gTP_TEID.buf[3]);

			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"AMF_PDUResourceSetupReq_UpdatePduSess gTP IP.size=%d \n", gTPTunnel->transportLayerAddress.size);

			if (4 == gTPTunnel->transportLayerAddress.size)//only ipv4
			{
				struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&addr;
				local_addr1_v4->sin_family = AF_INET;
				//local_addr1_v4->sin_addr = agc_atoui((char *)gTPTunnel->transportLayerAddress.buf);	
				memcpy(&local_addr1_v4->sin_addr, gTPTunnel->transportLayerAddress.buf, 4);
				addrlen = sizeof(struct sockaddr_in);
			}
			else if (16 == gTPTunnel->transportLayerAddress.size)//only ipv6
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
				agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AMF_PDUResourceSetupReq_UpdatePduSess transportLayerAddress size=%d.\n", gTPTunnel->transportLayerAddress.size);
			}
			
			if (GetPduSessManager().hasPduSession(info.sessId, pdu_resource_id) == true)
			{
				bMediaProcess = GetPduSessManager().processSessUpdateUpf(info, pdu_resource_id, teid, addr, addrlen, teidv6, addrv6, addrlenv6);
				agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"AMF_PDUResourceSetupReq_UpdatePduSess Update Upf\n");
			}
			if (bMediaProcess == false) //Case of this pdu_resource_id not been found in gateway session, and come to gateway first time.
			{
				for (ivec = 0; ivec < MAX_PDU_SESSION_SIZE; ivec++)
				{
					if (sess->vecPduSessionID[ivec] == INVALID_PDU_SESSION_ID)
					{						
						sess->vecPduSessionID[ivec] = pdu_resource_id;
						break;
					}
				}	
				bMediaProcess = GetPduSessManager().processSessSetupReq(info, pdu_resource_id, teid, addr, addrlen, teidv6, addrv6, addrlenv6);
				agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"AMF_PDUResourceSetupReq_UpdatePduSess First setup from Upf \n");
			}
		}

		
		pdu_resource.pdu_session_id = item->pDUSessionID;
		
		if (item->s_NSSAI.sD && item->s_NSSAI.sD->size > 0 && item->s_NSSAI.sD->buf != NULL)
		{
			pdu_resource.s_nnsai.sd |= item->s_NSSAI.sD->buf[0] << 16;
			pdu_resource.s_nnsai.sd  |= item->s_NSSAI.sD->buf[1] << 8;
			pdu_resource.s_nnsai.sd  |= item->s_NSSAI.sD->buf[2];
		}

		if (item->s_NSSAI.sST.size > 0 && item->s_NSSAI.sST.buf != NULL)
			pdu_resource.s_nnsai.sst = *((uint8_t*)item->s_NSSAI.sST.buf);

		if (PDUSessionAggregateMaximumBitRate != NULL)
		{
			uintmax_t maxBitrateUl;
			uintmax_t maxBitrateDl;
			asn_INTEGER2umax(&PDUSessionAggregateMaximumBitRate->pDUSessionAggregateMaximumBitRateUL, &maxBitrateUl);
			asn_INTEGER2umax(&PDUSessionAggregateMaximumBitRate->pDUSessionAggregateMaximumBitRateDL, &maxBitrateDl);
			
			pdu_resource.maxbitrate_dl = maxBitrateDl;
			pdu_resource.maxbitrate_ul = maxBitrateUl;
		}
		
		if (QosFlowSetupRequestList != NULL)
		{
			Ngap_QosFlowSetupRequestItem_t *item = NULL;
			for (int k = 0; k < QosFlowSetupRequestList->list.count; k++)
			{
				item = QosFlowSetupRequestList->list.array[k];
				pdu_resource.qos_flow_id = item->qosFlowIdentifier;
				if (item->qosFlowLevelQosParameters.qosCharacteristics.present == Ngap_QosCharacteristics_PR_nonDynamic5QI)
				{
					struct Ngap_NonDynamic5QIDescriptor	*nonDynamic5QI = item->qosFlowLevelQosParameters.qosCharacteristics.choice.nonDynamic5QI;
					if (nonDynamic5QI)
					{
						pdu_resource.choice.nonvalue.fiveQI = nonDynamic5QI->fiveQI;
						if (nonDynamic5QI->priorityLevelQos)
							pdu_resource.choice.nonvalue.priorityLevelQos = *((long *)nonDynamic5QI->priorityLevelQos);
					}
					pdu_resource.qos_character_type = pdu_session_resource_t::QOS_NON_DYNAMIC_5QI;
				}
				else if (item->qosFlowLevelQosParameters.qosCharacteristics.present == Ngap_QosCharacteristics_PR_nonDynamic5QI)
				{
					struct Ngap_Dynamic5QIDescriptor *dynamic5QI = item->qosFlowLevelQosParameters.qosCharacteristics.choice.dynamic5QI;
					if (dynamic5QI)
					{
						pdu_resource.choice.value.priorityLevelQos = dynamic5QI->priorityLevelQos;
						pdu_resource.choice.value.packetDelayBudget = dynamic5QI->packetDelayBudget;
						pdu_resource.choice.value.pERScalar = dynamic5QI->packetErrorRate.pERScalar;
						pdu_resource.choice.value.pERExponent = dynamic5QI->packetErrorRate.pERExponent;
					}
					pdu_resource.qos_character_type = pdu_session_resource_t::QOS_DYNAMIC_5QI;
				}
				else 
				{
					agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"AMF_PDUResourceSetupReq_UpdatePduSess present=%d \n",
							item->qosFlowLevelQosParameters.qosCharacteristics.present);				
				}

				pdu_resource.priorityLevelARP = item->qosFlowLevelQosParameters.allocationAndRetentionPriority.priorityLevelARP;
				pdu_resource.pre_emptionCapability = item->qosFlowLevelQosParameters.allocationAndRetentionPriority.pre_emptionCapability;
				pdu_resource.pre_emptionVulnerability = item->qosFlowLevelQosParameters.allocationAndRetentionPriority.pre_emptionVulnerability;
				break;
			}
		}
		

		GetPduSessManager().UpdatePduSessResource(info.sessId, pdu_resource_id, pdu_resource);
		ngapDecode.FreeResourceSetupRequestTransfer(pRequetTransfer);
	}
	
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "AMF_PDUResourceSetupReq_UpdatePduSess SessRanUeID=%d, SessAmfUeID=%d  info.sessId %d\n", 
		info.gnbUeId, info.amfUeId, info.sessId);
	
	return bMediaProcess;
}

bool AMF_PDUSessionResourceSetupReq(NgapMessage& info)
{
	Ngap_InitiatingMessage_t	*initiatingMessage = NULL;
	Ngap_PDUSessionResourceSetupRequest_t	 *N2PDUSessionResourceSetupRequest = NULL;
	Ngap_PDUSessionResourceSetupRequestIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t     *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t     *AMF_UE_NGAP_ID = NULL;
	Ngap_PDUSessionResourceSetupListSUReq_t	 *PDUSessionResourceSetupListSUReq = NULL;
	Ngap_PDUSessionResourceSetupRequestTransfer_t* pRequetTransfer = NULL;
	OCTET_STRING_t *tranfer = NULL;
	int i = 0;
	NgapProtocol ngapDecode;
	bool bMediaProcess = false;
	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
	N2PDUSessionResourceSetupRequest = &initiatingMessage->value.choice.PDUSessionResourceSetupRequest;
	Ngap_UEAggregateMaximumBitRate_t	 *UEAggregateMaximumBitRate = NULL;
	
	agc_time_t time_start = agc_time_now();
	for (i = 0; i < N2PDUSessionResourceSetupRequest->protocolIEs.list.count; i++)
	{
		ie = N2PDUSessionResourceSetupRequest->protocolIEs.list.array[i];
		switch (ie->id)
		{
			case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
				RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
				break;
			case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
				AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
				break;
			case Ngap_ProtocolIE_ID_id_PDUSessionResourceSetupListSUReq:
				PDUSessionResourceSetupListSUReq = &ie->value.choice.PDUSessionResourceSetupListSUReq;
				break;
			case Ngap_ProtocolIE_ID_id_UEAggregateMaximumBitRate:
				UEAggregateMaximumBitRate = &ie->value.choice.UEAggregateMaximumBitRate;
				break;
			
			default:
				break;
		}
	}
	
	if (AMF_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
    	agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"AMF_PDUSessionResourceSetupReq fail to AMF_ConvertNgapId ngap session\n");
		return false;
	}
		
	if (UEAggregateMaximumBitRate != NULL)
	{
		uintmax_t maxBitrateUl;
		uintmax_t maxBitrateDl;
		asn_INTEGER2umax(&UEAggregateMaximumBitRate->uEAggregateMaximumBitRateUL, &maxBitrateUl);
		asn_INTEGER2umax(&UEAggregateMaximumBitRate->uEAggregateMaximumBitRateDL, &maxBitrateDl);
		
		NgapSession *sess = NULL;
		if (GetNgapSessManager().GetNgapSession(info.sessId, &sess) == false)
		{
			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"AMF_PDUSessionResourceSetupReq fail to get sess=%d \n", info.sessId);
			return false;
		}
	
		sess->maxBitrateUl = maxBitrateUl;
		sess->maxBitrateDl = maxBitrateDl;
	}
	bMediaProcess = AMF_PDUResourceSetupReq_UpdatePduSess(info, PDUSessionResourceSetupListSUReq);

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "AMF_PDUSessionResourceSetupReq SessRanUeID=%d, SessAmfUeID=%d  info.sessId %d\n", 
		info.gnbUeId, info.amfUeId, info.sessId);

	//forward msg
	if (!bMediaProcess)
		GetNsmManager().SendDownLayerSctp(info);

	return true;
}

bool AMF_PDUSessionResourceSetupReqByMedia(NgapMessage& info)
{
	Ngap_InitiatingMessage_t	*initiatingMessage = NULL;
	Ngap_PDUSessionResourceSetupRequest_t	 *N2PDUSessionResourceSetupRequest = NULL;
	Ngap_PDUSessionResourceSetupRequestIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t     *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t     *AMF_UE_NGAP_ID = NULL;
	Ngap_PDUSessionResourceSetupListSUReq_t	 *PDUSessionResourceSetupListSUReq = NULL;
	Ngap_PDUSessionResourceSetupRequestTransfer_t* pRequetTransfer = NULL;
	OCTET_STRING_t *tranfer = NULL;
	int i = 0;
	NgapProtocol ngapDecode;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
	N2PDUSessionResourceSetupRequest = &initiatingMessage->value.choice.PDUSessionResourceSetupRequest;
	
	for (i = 0; i < N2PDUSessionResourceSetupRequest->protocolIEs.list.count; i++)
	{
		ie = N2PDUSessionResourceSetupRequest->protocolIEs.list.array[i];
		switch (ie->id)
		{
			case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
				RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
				break;
			case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
				AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
				break;
			case Ngap_ProtocolIE_ID_id_PDUSessionResourceSetupListSUReq:
				PDUSessionResourceSetupListSUReq = &ie->value.choice.PDUSessionResourceSetupListSUReq;
				break;
			default:
				break;
		}
	}
		
	if (PDUSessionResourceSetupListSUReq == NULL)
	{
		return false;
	}

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"AMF_PDUSessionResourceSetupReqByMedia ListCxtReq count=%d \n",
			PDUSessionResourceSetupListSUReq->list.count);

    for (i = 0; i < PDUSessionResourceSetupListSUReq->list.count; i++)
    {
    	Ngap_PDUSessionResourceSetupItemSUReq_t *item= 
    		(Ngap_PDUSessionResourceSetupItemSUReq_t *)PDUSessionResourceSetupListSUReq->list.array[i];
    	tranfer = &item->pDUSessionResourceSetupRequestTransfer;

    	int32_t len = tranfer->size;
		uint8_t pdu_resource_id =  item->pDUSessionID;
    	ngapDecode.DecodeResourceSetupRequestTransfer((char*)tranfer->buf, (int32_t *)&len, (void **)&pRequetTransfer);
				
		if (pRequetTransfer == NULL) 
		{
			continue;
		}
		
		uint32_t teid = 0;
		uint32_t teid_v6 = 0;
		agc_std_sockaddr_t addr;
		agc_std_sockaddr_t addr_v6;
		GetPduSessManager().GetGnbGtpInfo(info.sessId, pdu_resource_id, addr, teid, addr_v6, teid_v6);
		
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "AMF_PDUSessionResourceSetupReqByMedia handle pRequetTransfer\n");
				
		Ngap_PDUSessionResourceSetupRequestTransferIEs_t *pduie = NULL;
		Ngap_UPTransportLayerInformation_t	 *UPTransportLayerInformation = NULL;
		for (int j = 0; j < pRequetTransfer->protocolIEs.list.count; j++)
		{
			pduie = pRequetTransfer->protocolIEs.list.array[j];
			switch (pduie->id)
			{
				case Ngap_ProtocolIE_ID_id_UL_NGU_UP_TNLInformation:
					UPTransportLayerInformation = &pduie->value.choice.UPTransportLayerInformation;
					break;
			}
		}

		if (UPTransportLayerInformation != NULL && UPTransportLayerInformation->present == Ngap_UPTransportLayerInformation_PR_gTPTunnel)
		{			
			struct Ngap_GTPTunnel	*gTPTunnel = UPTransportLayerInformation->choice.gTPTunnel;

			if ((char *)gTPTunnel->gTP_TEID.buf == NULL)
			{
			    gTPTunnel->gTP_TEID.buf = (uint8_t *)calloc(4, sizeof(uint8_t));
			    gTPTunnel->gTP_TEID.size = 4;
			}
			gTPTunnel->gTP_TEID.buf[0] = teid >> 24;
			gTPTunnel->gTP_TEID.buf[1] = teid >> 16;
			gTPTunnel->gTP_TEID.buf[2] = teid >> 8;
			gTPTunnel->gTP_TEID.buf[3] = teid ;

			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "AMF_PDUSessionResourceSetupReqByMedia gTP IP.size=%d \n", gTPTunnel->transportLayerAddress.size);

			if (gTPTunnel->transportLayerAddress.buf != NULL)
				free(gTPTunnel->transportLayerAddress.buf);
			
			if (4 == gTPTunnel->transportLayerAddress.size)//ipv4 only
			{
				if (addr.ss_family == AF_INET)
				{
					struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&addr;
					gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(4, sizeof(uint8_t));
					memcpy(gTPTunnel->transportLayerAddress.buf, &local_addr1_v4->sin_addr, 4);
				}
			}
			else if (16 == gTPTunnel->transportLayerAddress.size)//ipv6 only
			{
				if (addr_v6.ss_family == AF_INET6)
				{
					struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&addr_v6;
					gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(16, sizeof(uint8_t));
					memcpy(gTPTunnel->transportLayerAddress.buf, &local_addr1_v6->sin6_addr, 16);
				}
			}
			else if (20 == gTPTunnel->transportLayerAddress.size) //ipv4 & ipv6
			{
				struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&addr;
				struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&addr_v6;
				gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(20, sizeof(uint8_t));
				memcpy(gTPTunnel->transportLayerAddress.buf, &local_addr1_v4->sin_addr, 4);
				memcpy(gTPTunnel->transportLayerAddress.buf + 4, &local_addr1_v6->sin6_addr, 16);
			}
			else
			{
				agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AMF_PDUSessionResourceSetupReqByMedia gTP IP.size length is invalid\n");
			}
		}
		
		if (tranfer != NULL) 
		{
	    	NgapProtocol ngapEncode;
	    	int32_t len = 0;
	    	ngapEncode.EncodeResourceSetupRequestTransfer((void *)pRequetTransfer, (char*)tranfer->buf, (int32_t *)&len);
			tranfer->size = len;
		}
		ngapDecode.FreeResourceSetupRequestTransfer(pRequetTransfer);
    }		
	
	//forward msg
	GetNsmManager().SendDownLayerSctp(info);

	return true;
}


bool AMF_PDUSessionResourceModifyReq(NgapMessage& info)
{
	Ngap_InitiatingMessage_t	*initiatingMessage = NULL;
	Ngap_PDUSessionResourceModifyRequest_t	 *N2PDUSessionResourceModifyRequest = NULL;
	Ngap_PDUSessionResourceModifyListModReq_t *PDUSessionResourceModifyListModReq = NULL;
	Ngap_PDUSessionResourceModifyRequestIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t     *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t     *AMF_UE_NGAP_ID = NULL;
	OCTET_STRING_t *tranfer = NULL;
	Ngap_PDUSessionResourceModifyRequestTransfer_t* pRequetTransfer = NULL;
	
	int i = 0;
	NgapProtocol ngapDecode;
	
	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
	N2PDUSessionResourceModifyRequest = &initiatingMessage->value.choice.PDUSessionResourceModifyRequest;

//	union Ngap_PDUSessionResourceModifyRequestIEs__Ngap_value_u {
//		Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
//		Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
//		Ngap_RANPagingPriority_t	 RANPagingPriority;
//		Ngap_PDUSessionResourceModifyListModReq_t	 PDUSessionResourceModifyListModReq;
//	} choice;

	for (i = 0; i < N2PDUSessionResourceModifyRequest->protocolIEs.list.count; i++)
	{
		ie = N2PDUSessionResourceModifyRequest->protocolIEs.list.array[i];
		switch (ie->id)
		{
			case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
				RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
				break;
			
			case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
				AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
				break;
			
			case Ngap_ProtocolIE_ID_id_PDUSessionResourceModifyListModReq:
				PDUSessionResourceModifyListModReq = &ie->value.choice.PDUSessionResourceModifyListModReq;
				break;
			
			default:
				break;
		
		}
	
	}
	
	if (AMF_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
    	agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"AMF_PDUSessionResourceModifyReq fail to AMF_ConvertNgapId ngap session\n");
		return false;
	}

	if (PDUSessionResourceModifyListModReq != NULL)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"AMF_PDUSessionResourceModifyReq ListCxtReq count=%d \n",
				PDUSessionResourceModifyListModReq->list.count);

	    for (i = 0; i < PDUSessionResourceModifyListModReq->list.count; i++)
	    {
	    	Ngap_PDUSessionResourceModifyItemModReq_t *item= 
	    		(Ngap_PDUSessionResourceModifyItemModReq_t *)PDUSessionResourceModifyListModReq->list.array[i];
	    	tranfer = &item->pDUSessionResourceModifyRequestTransfer;

	    	int32_t len = tranfer->size;
	    	ngapDecode.DecodeResourceModifyRequestTransfer((char*)tranfer->buf, (int32_t *)&len, (void **)&pRequetTransfer);
			
	    	break;
	    }		
	}
	
	//forward msg
	GetNsmManager().SendDownLayerSctp(info);

	return true;
}


//lyb,2020-4-16  Release Command
bool AMF_PDUSessionResourceReleaseCmd(NgapMessage& info)
{
	Ngap_InitiatingMessage_t	*initiatingMessage = NULL;
	Ngap_PDUSessionResourceReleaseCommand_t	 *N2PDUSessionResourceReleaseCommand = NULL;
	
	Ngap_PDUSessionResourceReleaseCommandIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t     *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t     *AMF_UE_NGAP_ID = NULL;
	Ngap_PDUSessionResourceToReleaseListRelCmd_t	*PDUSessionResourceToReleaseListRelCmd = NULL;
	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
//	d_assert(initiatingMessage, return,);
	
	N2PDUSessionResourceReleaseCommand = &initiatingMessage->value.choice.PDUSessionResourceReleaseCommand;
//	d_assert(N2PDUSessionResourceReleaseCommand, return,);


//	union Ngap_PDUSessionResourceReleaseCommandIEs__Ngap_value_u {
//		Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
//		Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
//		Ngap_RANPagingPriority_t	 RANPagingPriority;
//		Ngap_NAS_PDU_t	 NAS_PDU;
//		Ngap_PDUSessionResourceToReleaseListRelCmd_t	 PDUSessionResourceToReleaseListRelCmd;
//	} choice;

	for (i = 0; i < N2PDUSessionResourceReleaseCommand->protocolIEs.list.count; i++)
	{
		ie = N2PDUSessionResourceReleaseCommand->protocolIEs.list.array[i];
		switch (ie->id)
		{
			case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
				RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
				break;
			
			case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
				AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
				break;
			
			case Ngap_ProtocolIE_ID_id_PDUSessionResourceToReleaseListRelCmd:
				PDUSessionResourceToReleaseListRelCmd = &ie->value.choice.PDUSessionResourceToReleaseListRelCmd;
				break;
			
			default:
				break;
		
		}
	
	}

	if (AMF_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AMF_PDUSessionResourceReleaseCmd fail to AMF_ConvertNgapId ngap session\n");
		return false;
	}

	if (PDUSessionResourceToReleaseListRelCmd != NULL)
	{
    	Ngap_PDUSessionResourceToReleaseItemRelCmd_t *item = NULL;
	    for (i = 0; i < PDUSessionResourceToReleaseListRelCmd->list.count; i++)
	    {
	    	item = (Ngap_PDUSessionResourceToReleaseItemRelCmd_t *)PDUSessionResourceToReleaseListRelCmd->list.array[i];
			GetPduSessManager().ReleasePduSess(info.sessId, item->pDUSessionID);
    	}
	}

	GetNsmManager().SendDownLayerSctp(info);

	return true;
}

bool AMF_InitialContextSetupReq_UpdateNgapSess(NgapMessage& info, Ngap_UEAggregateMaximumBitRate_t	*UEAggregateMaximumBitRate, 
	Ngap_UESecurityCapabilities_t *UESecurityCapabilities, Ngap_SecurityKey_t *SecurityKey,
	Ngap_AllowedNSSAI_t *AllowedNSSAI, Ngap_GUAMI_t *GUAMI)
{
	int i = 0;
	NgapSession *sess = NULL;
	if (GetNgapSessManager().GetNgapSession(info.sessId, &sess) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"AMF_InitialContextSetupReq_UpdateNgapSess fail to get sess=%d \n", info.sessId);
		return false;
	}
	
	if (UEAggregateMaximumBitRate != NULL)
	{
		uintmax_t maxBitrateUl;
		uintmax_t maxBitrateDl;
		asn_INTEGER2umax(&UEAggregateMaximumBitRate->uEAggregateMaximumBitRateUL, &maxBitrateUl);
		asn_INTEGER2umax(&UEAggregateMaximumBitRate->uEAggregateMaximumBitRateDL, &maxBitrateDl);
		
		sess->maxBitrateUl = maxBitrateUl;
		sess->maxBitrateDl = maxBitrateDl;
	}
	
	if (AllowedNSSAI != NULL)
	{
	    for (i = 0; i < AllowedNSSAI->list.count; i++)
	    {
	    	Ngap_AllowedNSSAI_Item_t *item= (Ngap_AllowedNSSAI_Item_t *)AllowedNSSAI->list.array[i];
			if (item->s_NSSAI.sD)
			{
				sess->allowedSNnsaiSD |= item->s_NSSAI.sD->buf[0] << 16;
				sess->allowedSNnsaiSD |= item->s_NSSAI.sD->buf[1] << 8;
				sess->allowedSNnsaiSD |= item->s_NSSAI.sD->buf[2];
			}

			sess->allowedSNnsaiSST = *((uint8_t*)item->s_NSSAI.sST.buf);
			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "AMF_InitialContextSetupReq_UpdateNgapSess allowedSNnsaiSD=%d allowedSNnsaiSST=%d \n",
					sess->allowedSNnsaiSD, sess->allowedSNnsaiSST);
	    	break;
	    }		
	}
	

	if (GUAMI != NULL)
	{
	    plmn_id_t plmn_id;
		
		sess->guami.AMFRegionID = *((uint8_t *)GUAMI->aMFRegionID.buf);
		sess->guami.AMFSetId =  GUAMI->aMFSetID.buf[0] << 8;
		sess->guami.AMFSetId |=  GUAMI->aMFSetID.buf[1];
		sess->guami.AMFPointer =  *((uint8_t *)GUAMI->aMFPointer.buf) & 0x3F;
		
		memcpy(&plmn_id, GUAMI->pLMNIdentity.buf, PLMN_ID_LEN);
		sess->guami.plmn.mcc1 = plmn_id.mcc1;
		sess->guami.plmn.mcc2 = plmn_id.mcc2;
		sess->guami.plmn.mcc3 = plmn_id.mcc3;
		sess->guami.plmn.mnc1 = plmn_id.mnc1;
		sess->guami.plmn.mnc2 = plmn_id.mnc2;
		sess->guami.plmn.mnc3 = plmn_id.mnc3;
		
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"AMF_InitialContextSetupReq_UpdateNgapSess aMFSetID count=%d \n",
				GUAMI->aMFSetID.buf[0], GUAMI->aMFSetID.buf[1]);
	}

	if (UESecurityCapabilities != NULL)
	{
		sess->nRencryptionAlgorithms = ntohs(*((uint16_t *)UESecurityCapabilities->nRencryptionAlgorithms.buf));
		sess->nRintegrityProtectionAlgorithms =  ntohs(*((uint16_t *)UESecurityCapabilities->nRintegrityProtectionAlgorithms.buf));
		sess->eUTRAencryptionAlgorithms =  ntohs(*((uint16_t *)UESecurityCapabilities->eUTRAencryptionAlgorithms.buf));
		sess->eUTRAintegrityProtectionAlgorithms =  ntohs(*((uint16_t *)UESecurityCapabilities->eUTRAintegrityProtectionAlgorithms.buf));
	}

	if (SecurityKey != NULL)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"AMF_InitialContextSetupReq_UpdateNgapSess SecurityKey count=%d \n",
				SecurityKey->size);
		memcpy(sess->securityContext, SecurityKey->buf, SecurityKey->size);
	}
	
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"AMF_InitialContextSetupReq_UpdateNgapSess maxBitrateUl=%lld maxBitrateDl=%lld allowedSNnsaiSD=%d allowedSNnsaiSST=%d \n",
			sess->maxBitrateUl, sess->maxBitrateDl, sess->allowedSNnsaiSD, sess->allowedSNnsaiSST);
	return true;
}

bool AMF_InitialContextSetupReq_UpdatePduSess(NgapMessage& info, Ngap_PDUSessionResourceSetupListCxtReq_t	*PDUSessionResourceSetupListCxtReq)
{
	int i = 0;
	int ivec = 0; 
	bool bMediaProcess = false;
	NgapProtocol ngapDecode;
	Ngap_PDUSessionResourceSetupRequestTransfer_t* pRequetTransfer = NULL;
	Ngap_PDUSessionResourceSetupRequestTransferIEs_t *pduie = NULL;
	Ngap_UPTransportLayerInformation_t	 *UPTransportLayerInformation = NULL;
	Ngap_PDUSessionAggregateMaximumBitRate_t	 *PDUSessionAggregateMaximumBitRate = NULL;
	Ngap_PDUSessionType_t	 *PDUSessionType = NULL;
	Ngap_QosFlowSetupRequestList_t	 *QosFlowSetupRequestList = NULL;
   	Ngap_PDUSessionResourceSetupItemCxtReq_t *item = NULL;
	pdu_session_resource_t pdu_resource;
	uint32_t teid = 0;
	uint32_t teidv6 = 0;
	socklen_t addrlen = 0;
	socklen_t addrlenv6 = 0;
	agc_std_sockaddr_t addr;
	agc_std_sockaddr_t addrv6;
	uint8_t pdu_resource_id = 0;
	
	NgapSession *sess =   NULL;
	if (GetNgapSessManager().GetNgapSession(info.sessId, &sess) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"AMF_InitialContextSetupReq_UpdatePduSess fail to get sess=%d \n", info.sessId);
		return false;
	}
	
	memset(&pdu_resource, 0, sizeof(pdu_session_resource_t));
	
	if (PDUSessionResourceSetupListCxtReq == NULL)
		return false;

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"AMF_InitialContextSetupReq_UpdatePduSess ListCxtReq count=%d \n",
			PDUSessionResourceSetupListCxtReq->list.count);

	sess->pdu_resource_count = PDUSessionResourceSetupListCxtReq->list.count;
	sess->pdu_sess_rsp_count = 0;

    for (i = 0; i < PDUSessionResourceSetupListCxtReq->list.count; i++)
    {
    	item = (Ngap_PDUSessionResourceSetupItemCxtReq_t *)PDUSessionResourceSetupListCxtReq->list.array[i];
		
    	OCTET_STRING_t *tranfer = &item->pDUSessionResourceSetupRequestTransfer;		
		pdu_resource_id = item->pDUSessionID;		
    	int32_t len = tranfer->size;
		
    	ngapDecode.DecodeResourceSetupRequestTransfer((char*)tranfer->buf, (int32_t *)&len, (void **)&pRequetTransfer);
			
		if (pRequetTransfer == NULL) 
		{	
			continue;
		}

		for (int j = 0; j < pRequetTransfer->protocolIEs.list.count; j++)
		{
			pduie = pRequetTransfer->protocolIEs.list.array[j];
			switch (pduie->id)
			{
				case Ngap_ProtocolIE_ID_id_PDUSessionAggregateMaximumBitRate:
					PDUSessionAggregateMaximumBitRate = &pduie->value.choice.PDUSessionAggregateMaximumBitRate;
					break;
				case Ngap_ProtocolIE_ID_id_PDUSessionType:
					PDUSessionType = &pduie->value.choice.PDUSessionType;
					break;
				case Ngap_ProtocolIE_ID_id_QosFlowSetupRequestList:
					QosFlowSetupRequestList = &pduie->value.choice.QosFlowSetupRequestList;
					break;
				case Ngap_ProtocolIE_ID_id_UL_NGU_UP_TNLInformation:
					UPTransportLayerInformation = &pduie->value.choice.UPTransportLayerInformation;
					break;
			}
		}

		if (PDUSessionType != NULL && UPTransportLayerInformation != NULL && UPTransportLayerInformation->present == Ngap_UPTransportLayerInformation_PR_gTPTunnel)
		{		
			struct Ngap_GTPTunnel	*gTPTunnel = UPTransportLayerInformation->choice.gTPTunnel;

			teid = ((uint32_t)gTPTunnel->gTP_TEID.buf[0] << 24)
				| ((uint32_t)gTPTunnel->gTP_TEID.buf[1] << 16)
				| ((uint32_t)gTPTunnel->gTP_TEID.buf[2] << 8)
				| ((uint32_t)gTPTunnel->gTP_TEID.buf[3]);


			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "AMF_InitialContextSetupReq_UpdatePduSess teid=0x%x size=%d, buf=0x%x%x%x%x\n",
					teid, gTPTunnel->gTP_TEID.size, gTPTunnel->gTP_TEID.buf[0], gTPTunnel->gTP_TEID.buf[1], gTPTunnel->gTP_TEID.buf[2], gTPTunnel->gTP_TEID.buf[3]);

			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"AMF_InitialContextSetupReq_UpdatePduSess transportLayerAddress size=%d buf=0x%x%x%x%x\n",
					gTPTunnel->transportLayerAddress.size, 
					gTPTunnel->transportLayerAddress.buf[0], 
					gTPTunnel->transportLayerAddress.buf[1], 
					gTPTunnel->transportLayerAddress.buf[2],
					gTPTunnel->transportLayerAddress.buf[3]);

			if (4 == gTPTunnel->transportLayerAddress.size)//only ipv4
			{
				struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&addr;
				local_addr1_v4->sin_family = AF_INET;
				//local_addr1_v4->sin_addr = agc_atoui((char *)gTPTunnel->transportLayerAddress.buf);	
				memcpy(&local_addr1_v4->sin_addr, gTPTunnel->transportLayerAddress.buf, 4);
				addrlen = sizeof(struct sockaddr_in);
			}
			else if (16 == gTPTunnel->transportLayerAddress.size)//only ipv6
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
			else			//other type
			{
				// todo: not support yet
				agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AMF_InitialContextSetupReq_UpdatePduSess gTP IP.size length is invalid\n");
			}
			
			bMediaProcess = GetPduSessManager().processSessSetupReq(info, pdu_resource_id, teid, addr, addrlen, teidv6, addrv6, addrlenv6);
		}

		//first save pdu_resource_id into gateway
		for (ivec = 0; ivec < MAX_PDU_SESSION_SIZE; ivec++)
		{
			if (sess->vecPduSessionID[ivec] == INVALID_PDU_SESSION_ID)
				{
					sess->vecPduSessionID[ivec] = pdu_resource_id;
					break;
				}
		}

		if (ivec >= MAX_PDU_SESSION_SIZE)
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"AMF_InitialContextSetupReq_UpdatePduSess ivec=%d \n",ivec);
			

		pdu_resource.pdu_session_id = pdu_resource_id;

		if (item->s_NSSAI.sD)
		{
			pdu_resource.s_nnsai.sd |= item->s_NSSAI.sD->buf[0] << 16;
			pdu_resource.s_nnsai.sd  |= item->s_NSSAI.sD->buf[1] << 8;
			pdu_resource.s_nnsai.sd  |= item->s_NSSAI.sD->buf[2];
		}

		pdu_resource.s_nnsai.sst = *((uint8_t*)item->s_NSSAI.sST.buf);
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"AMF_InitialContextSetupReq_UpdatePduSess allowedSNnsaiSD=%d allowedSNnsaiSST=%d \n",
				pdu_resource.s_nnsai.sd, pdu_resource.s_nnsai.sst);

		if (PDUSessionAggregateMaximumBitRate != NULL)
		{
			uintmax_t maxBitrateUl;
			uintmax_t maxBitrateDl;
			asn_INTEGER2umax(&PDUSessionAggregateMaximumBitRate->pDUSessionAggregateMaximumBitRateUL, &maxBitrateUl);
			asn_INTEGER2umax(&PDUSessionAggregateMaximumBitRate->pDUSessionAggregateMaximumBitRateDL, &maxBitrateDl);
			
			pdu_resource.maxbitrate_dl = maxBitrateDl;
			pdu_resource.maxbitrate_ul = maxBitrateUl;
		}

		if (QosFlowSetupRequestList != NULL)
		{
			Ngap_QosFlowSetupRequestItem_t *item = NULL;
			for (int k = 0; k < QosFlowSetupRequestList->list.count; k++)
			{
				item = QosFlowSetupRequestList->list.array[k];
				pdu_resource.qos_flow_id = item->qosFlowIdentifier;
				if (item->qosFlowLevelQosParameters.qosCharacteristics.present == Ngap_QosCharacteristics_PR_nonDynamic5QI)
				{
					struct Ngap_NonDynamic5QIDescriptor	*nonDynamic5QI = item->qosFlowLevelQosParameters.qosCharacteristics.choice.nonDynamic5QI;
					if (nonDynamic5QI)
					{
						pdu_resource.choice.nonvalue.fiveQI = nonDynamic5QI->fiveQI;
						if (nonDynamic5QI->priorityLevelQos)
							pdu_resource.choice.nonvalue.priorityLevelQos = *((long *)nonDynamic5QI->priorityLevelQos);
					}
					pdu_resource.qos_character_type = pdu_session_resource_t::QOS_NON_DYNAMIC_5QI;
				}
				else if (item->qosFlowLevelQosParameters.qosCharacteristics.present == Ngap_QosCharacteristics_PR_nonDynamic5QI)
				{
					struct Ngap_Dynamic5QIDescriptor *dynamic5QI = item->qosFlowLevelQosParameters.qosCharacteristics.choice.dynamic5QI;
					if (dynamic5QI)
					{
						pdu_resource.choice.value.priorityLevelQos = dynamic5QI->priorityLevelQos;
						pdu_resource.choice.value.packetDelayBudget = dynamic5QI->packetDelayBudget;
						pdu_resource.choice.value.pERScalar = dynamic5QI->packetErrorRate.pERScalar;
						pdu_resource.choice.value.pERExponent = dynamic5QI->packetErrorRate.pERExponent;
					}
					pdu_resource.qos_character_type = pdu_session_resource_t::QOS_DYNAMIC_5QI;
				}
				else 
				{
					agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"AMF_InitialContextSetupReq_UpdatePduSess present=%d \n",
							item->qosFlowLevelQosParameters.qosCharacteristics.present);				
				}

				pdu_resource.priorityLevelARP = item->qosFlowLevelQosParameters.allocationAndRetentionPriority.priorityLevelARP;
				pdu_resource.pre_emptionCapability = item->qosFlowLevelQosParameters.allocationAndRetentionPriority.pre_emptionCapability;
				pdu_resource.pre_emptionVulnerability = item->qosFlowLevelQosParameters.allocationAndRetentionPriority.pre_emptionVulnerability;
	
			}
		}

		GetPduSessManager().UpdatePduSessResource(info.sessId, pdu_resource_id, pdu_resource);
		ngapDecode.FreeResourceSetupRequestTransfer(pRequetTransfer);
    }		

	return bMediaProcess;
}

//lyb,2020-4-16
bool AMF_InitialContextSetupReq(NgapMessage& info)
{
	Ngap_InitiatingMessage_t	*initiatingMessage = NULL;
	Ngap_InitialContextSetupRequest_t	 *N2InitialContextSetupRequest = NULL;
	Ngap_InitialContextSetupRequestIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t     *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t     *AMF_UE_NGAP_ID = NULL;
	Ngap_PDUSessionResourceSetupListCxtReq_t	 *PDUSessionResourceSetupListCxtReq = NULL;
	Ngap_UEAggregateMaximumBitRate_t	 *UEAggregateMaximumBitRate = NULL;
	Ngap_UESecurityCapabilities_t	 *UESecurityCapabilities = NULL;
	Ngap_SecurityKey_t	 *SecurityKey = NULL;
	Ngap_AllowedNSSAI_t	 *AllowedNSSAI = NULL;
	Ngap_GUAMI_t	 *GUAMI = NULL;
	int i = 0;
	bool bMediaProcess = false;
	
	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
	N2InitialContextSetupRequest = &initiatingMessage->value.choice.InitialContextSetupRequest;

	for (i = 0; i < N2InitialContextSetupRequest->protocolIEs.list.count; i++)
	{
		ie = N2InitialContextSetupRequest->protocolIEs.list.array[i];
		switch (ie->id)
		{
			case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
				RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
				break;
			
			case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
				AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
				break;
			
			case Ngap_ProtocolIE_ID_id_UEAggregateMaximumBitRate:
				UEAggregateMaximumBitRate = &ie->value.choice.UEAggregateMaximumBitRate;
				break;
			
			case Ngap_ProtocolIE_ID_id_AllowedNSSAI:
				AllowedNSSAI = &ie->value.choice.AllowedNSSAI;
				break;
							
			case Ngap_ProtocolIE_ID_id_GUAMI:
				GUAMI = &ie->value.choice.GUAMI;
				break;
			
			case Ngap_ProtocolIE_ID_id_UESecurityCapabilities:
				UESecurityCapabilities = &ie->value.choice.UESecurityCapabilities;
				break;
			
			case Ngap_ProtocolIE_ID_id_SecurityKey:
				SecurityKey = &ie->value.choice.SecurityKey;
				break;
				
			case Ngap_ProtocolIE_ID_id_PDUSessionResourceSetupListCxtReq:
				PDUSessionResourceSetupListCxtReq = &ie->value.choice.PDUSessionResourceSetupListCxtReq;
				break;
				
			default:
				break;
		}
	}

	if (AMF_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"AMF_InitialContextSetupReq fail to AMF_ConvertNgapId ngap session\n");
		return false;
	}
	
	AMF_InitialContextSetupReq_UpdateNgapSess(info, UEAggregateMaximumBitRate, 
		UESecurityCapabilities, SecurityKey, AllowedNSSAI, GUAMI);
	
	bMediaProcess = AMF_InitialContextSetupReq_UpdatePduSess(info, PDUSessionResourceSetupListCxtReq);

	if (!bMediaProcess)
		GetNsmManager().SendDownLayerSctp(info);

	return true;
}


bool AMF_InitialContextSetupReqByMedia(NgapMessage& info)
{
	Ngap_InitiatingMessage_t	*initiatingMessage = NULL;
	Ngap_InitialContextSetupRequest_t	 *N2InitialContextSetupRequest = NULL;
	Ngap_InitialContextSetupRequestIEs_t *ie = NULL;
	Ngap_PDUSessionResourceSetupListCxtReq_t	 *PDUSessionResourceSetupListCxtReq = NULL;
	Ngap_PDUSessionResourceSetupRequestTransfer_t* pRequetTransfer = NULL;
	int i = 0;
	NgapProtocol ngapDecode;
	OCTET_STRING_t *tranfer = NULL;
	uint8_t pdu_resource_id = 0;
	
	initiatingMessage = info.ngapMessage.choice.initiatingMessage;	
	N2InitialContextSetupRequest = &initiatingMessage->value.choice.InitialContextSetupRequest;


	for (i = 0; i < N2InitialContextSetupRequest->protocolIEs.list.count; i++)
	{
		ie = N2InitialContextSetupRequest->protocolIEs.list.array[i];
		switch (ie->id)
		{			
			case Ngap_ProtocolIE_ID_id_PDUSessionResourceSetupListCxtReq:
				PDUSessionResourceSetupListCxtReq = &ie->value.choice.PDUSessionResourceSetupListCxtReq;
				break;
				
			default:
				break;
		}
	}
	
	if (PDUSessionResourceSetupListCxtReq == NULL)
	{	
		GetNsmManager().SendDownLayerSctp(info);
		return true;
	}
	
	for (i = 0; i < PDUSessionResourceSetupListCxtReq->list.count; i++)
	{
		Ngap_PDUSessionResourceSetupItemCxtReq_t *item= 
			(Ngap_PDUSessionResourceSetupItemCxtReq_t *)PDUSessionResourceSetupListCxtReq->list.array[i];
		tranfer = &item->pDUSessionResourceSetupRequestTransfer;
	
		int32_t len = tranfer->size;
		ngapDecode.DecodeResourceSetupRequestTransfer((char*)tranfer->buf, (int32_t *)&len, (void **)&pRequetTransfer);
		pdu_resource_id = item->pDUSessionID;
		
		if (pRequetTransfer != NULL) 
		{	
			uint32_t teid = 0;
			uint32_t teid_v6 = 0;
			agc_std_sockaddr_t addr;
			agc_std_sockaddr_t addr_v6;

			//use gnb info
			GetPduSessManager().GetGnbGtpInfo(info.sessId, pdu_resource_id, addr, teid, addr_v6, teid_v6);
			
			Ngap_PDUSessionResourceSetupRequestTransferIEs_t *pduie = NULL;
			Ngap_UPTransportLayerInformation_t	 *UPTransportLayerInformation = NULL;
			for (int j = 0; j < pRequetTransfer->protocolIEs.list.count; j++)
			{
				pduie = pRequetTransfer->protocolIEs.list.array[j];
				switch (pduie->id)
				{
					case Ngap_ProtocolIE_ID_id_UL_NGU_UP_TNLInformation:
						UPTransportLayerInformation = &pduie->value.choice.UPTransportLayerInformation;
						break;
				}
			}
		
			if (UPTransportLayerInformation != NULL && UPTransportLayerInformation->present == Ngap_UPTransportLayerInformation_PR_gTPTunnel)
			{			
				struct Ngap_GTPTunnel	*gTPTunnel = UPTransportLayerInformation->choice.gTPTunnel;
		
				if ((char *)gTPTunnel->gTP_TEID.buf == NULL)
				{
					gTPTunnel->gTP_TEID.buf = (uint8_t *)calloc(4, sizeof(uint8_t));
					gTPTunnel->gTP_TEID.size = 4;
				}
				gTPTunnel->gTP_TEID.buf[0] = teid >> 24;
				gTPTunnel->gTP_TEID.buf[1] = teid >> 16;
				gTPTunnel->gTP_TEID.buf[2] = teid >> 8;
				gTPTunnel->gTP_TEID.buf[3] = teid ;
		
				if (gTPTunnel->transportLayerAddress.buf != NULL)
					free(gTPTunnel->transportLayerAddress.buf);
				
				agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "AMF_InitialContextSetupReqByMedia gTP IP.size=%d \n", gTPTunnel->transportLayerAddress.size);

				if (4 == gTPTunnel->transportLayerAddress.size)//ipv4 only
				{
					if (addr.ss_family == AF_INET)
					{
						struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&addr;
						gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(4, sizeof(uint8_t));
						memcpy(gTPTunnel->transportLayerAddress.buf, &local_addr1_v4->sin_addr, 4);
					}
				}
				else if (16 == gTPTunnel->transportLayerAddress.size)//ipv6 only
				{
					if (addr_v6.ss_family == AF_INET6)
					{
						struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&addr_v6;
						gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(16, sizeof(uint8_t));
						memcpy(gTPTunnel->transportLayerAddress.buf, &local_addr1_v6->sin6_addr, 16);
					}
				}
				else if (20 == gTPTunnel->transportLayerAddress.size) //ipv4 & ipv6
				{
					struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&addr;
					struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&addr_v6;
					gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(20, sizeof(uint8_t));
					memcpy(gTPTunnel->transportLayerAddress.buf, &local_addr1_v4->sin_addr, 4);
					memcpy(gTPTunnel->transportLayerAddress.buf + 4, &local_addr1_v6->sin6_addr, 16);
				}
				else
				{
					agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AMF_InitialContextSetupReqByMedia gTP IP.size length is invalid\n");
				}
			}
			
			if (tranfer != NULL) 
			{
				NgapProtocol ngapEncode;
				int32_t len = 0;
				ngapEncode.EncodeResourceSetupRequestTransfer((void *)pRequetTransfer, (char*)tranfer->buf, (int32_t *)&len);
				tranfer->size = len;
			}
			ngapDecode.FreeResourceSetupRequestTransfer(pRequetTransfer);
		}
	}	

	GetNsmManager().SendDownLayerSctp(info);
	
	return true;
}


//lyb,2020-5-11
bool AMF_UEContextReleaseCommand(NgapMessage& info)
{
	Ngap_InitiatingMessage_t	*initiatingMessage = NULL;
	Ngap_UEContextReleaseCommand_t	 *N2UEContextReleaseCommand = NULL;
	Ngap_UEContextReleaseCommand_IEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t     *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t     *AMF_UE_NGAP_ID = NULL;
	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
//	d_assert(initiatingMessage, return false,);
	
	N2UEContextReleaseCommand = &initiatingMessage->value.choice.UEContextReleaseCommand;
//	d_assert(N2UEContextReleaseCommand, return false,);


//	union Ngap_UEContextReleaseCommand_IEs__Ngap_value_u {
//		Ngap_UE_NGAP_IDs_t	 UE_NGAP_IDs;                      -------------  NOT SAME
//		Ngap_Cause_t	 Cause;
//	} choice;

	for (i = 0; i < N2UEContextReleaseCommand->protocolIEs.list.count; i++)
	{
		ie = N2UEContextReleaseCommand->protocolIEs.list.array[i];
		switch (ie->id)
		{
			case Ngap_ProtocolIE_ID_id_UE_NGAP_IDs:
				if (ie->value.choice.UE_NGAP_IDs.present == Ngap_UE_NGAP_IDs_PR_uE_NGAP_ID_pair)
				{
					RanUeNgapId = &(ie->value.choice.UE_NGAP_IDs.choice.uE_NGAP_ID_pair->rAN_UE_NGAP_ID);
					AMF_UE_NGAP_ID = &(ie->value.choice.UE_NGAP_IDs.choice.uE_NGAP_ID_pair->aMF_UE_NGAP_ID);
				}
				else if (ie->value.choice.UE_NGAP_IDs.present == Ngap_UE_NGAP_IDs_PR_aMF_UE_NGAP_ID)
				{
					AMF_UE_NGAP_ID = &(ie->value.choice.UE_NGAP_IDs.choice.aMF_UE_NGAP_ID);
				}
				else 
				{
    				agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"AMF_UEContextReleaseCommand unkown NGAP ID\n");
					return false;
				}
				break;
			default:
				break;
		
		}
	
	}

	if (NULL != RanUeNgapId) //HAS Ran UE ID
	{
		if (AMF_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
		{
	    	agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"AMF_UEContextReleaseCommand fail to AMF_ConvertNgapId ngap session\n");
			return false;
		}

		GetNsmManager().SendDownLayerSctp(info);
		
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
	}
	else  // No RAN UE ID
	{

	}
	

	return true;
}


//lyb,2020-4-16
bool AMF_UEContextModificationReq(NgapMessage& info)
{
	Ngap_InitiatingMessage_t	*initiatingMessage = NULL;
	Ngap_UEContextModificationRequest_t	 *N2UEContextModificationRequest = NULL;
	Ngap_UEContextModificationRequestIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t     *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t     *AMF_UE_NGAP_ID = NULL;
	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
//	d_assert(initiatingMessage, return,);
	
	N2UEContextModificationRequest = &initiatingMessage->value.choice.UEContextModificationRequest;
//	d_assert(N2UEContextModificationRequest, return,);


//	union Ngap_UEContextModificationRequestIEs__Ngap_value_u {
//		Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
//		Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
//		Ngap_RANPagingPriority_t	 RANPagingPriority;
//		Ngap_SecurityKey_t	 SecurityKey;
//		Ngap_IndexToRFSP_t	 IndexToRFSP;
//		Ngap_UEAggregateMaximumBitRate_t	 UEAggregateMaximumBitRate;
//		Ngap_UESecurityCapabilities_t	 UESecurityCapabilities;
//		Ngap_CoreNetworkAssistanceInformation_t  CoreNetworkAssistanceInformation;
//		Ngap_EmergencyFallbackIndicator_t	 EmergencyFallbackIndicator;
//		Ngap_RRCInactiveTransitionReportRequest_t	 RRCInactiveTransitionReportRequest;
//	} choice;

	for (i = 0; i < N2UEContextModificationRequest->protocolIEs.list.count; i++)
	{
		ie = N2UEContextModificationRequest->protocolIEs.list.array[i];
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

	if (AMF_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"AMF_UEContextModificationReq fail to AMF_ConvertNgapId ngap session\n");
		return false;
	}

	GetNsmManager().SendDownLayerSctp(info);

	return true;
}

bool AMF_HandoverReq_UpdateNgapSess(NgapMessage &info,
		Ngap_UEAggregateMaximumBitRate_t *UEAggregateMaximumBitRate,
		Ngap_UESecurityCapabilities_t *UESecurityCapabilities,
		Ngap_SecurityContext_t *SecurityContext, Ngap_AllowedNSSAI_t *AllowedNSSAI,
		Ngap_GUAMI_t *GUAMI) {
	int i = 0;
	NgapSession *sess = NULL;
	if (GetNgapSessManager().GetNgapSession(info.sessId, &sess) == false) {
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,
				"AMF_HandoverReq_UpdateNgapSess fail to get sess=%d \n",
				info.sessId);
		return false;
	}

	if (UEAggregateMaximumBitRate != NULL) {
		uintmax_t maxBitrateUl;
		uintmax_t maxBitrateDl;
		asn_INTEGER2umax(
				&UEAggregateMaximumBitRate->uEAggregateMaximumBitRateUL,
				&maxBitrateUl);
		asn_INTEGER2umax(
				&UEAggregateMaximumBitRate->uEAggregateMaximumBitRateDL,
				&maxBitrateDl);

		sess->maxBitrateUl = maxBitrateUl;
		sess->maxBitrateDl = maxBitrateDl;
	}

	if (AllowedNSSAI != NULL) {
		for (i = 0; i < AllowedNSSAI->list.count; i++) {
			Ngap_AllowedNSSAI_Item_t *item =
					(Ngap_AllowedNSSAI_Item_t*) AllowedNSSAI->list.array[i];
			if (item->s_NSSAI.sD) {
				sess->allowedSNnsaiSD |= item->s_NSSAI.sD->buf[0] << 16;
				sess->allowedSNnsaiSD |= item->s_NSSAI.sD->buf[1] << 8;
				sess->allowedSNnsaiSD |= item->s_NSSAI.sD->buf[2];
			}

			sess->allowedSNnsaiSST = *((uint8_t*) item->s_NSSAI.sST.buf);
			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,
					"AMF_HandoverReq_UpdateNgapSess allowedSNnsaiSD=%d allowedSNnsaiSST=%d \n",
					sess->allowedSNnsaiSD, sess->allowedSNnsaiSST);
			break;
		}
	}

	if (GUAMI != NULL) {
		plmn_id_t plmn_id;

		sess->guami.AMFRegionID = *((uint8_t*) GUAMI->aMFRegionID.buf);
		sess->guami.AMFSetId = GUAMI->aMFSetID.buf[0] << 8;
		sess->guami.AMFSetId |= GUAMI->aMFSetID.buf[1];
		sess->guami.AMFPointer = *((uint8_t*) GUAMI->aMFPointer.buf) & 0x3F;

		memcpy(&plmn_id, GUAMI->pLMNIdentity.buf, PLMN_ID_LEN);
		sess->guami.plmn.mcc1 = plmn_id.mcc1;
		sess->guami.plmn.mcc2 = plmn_id.mcc2;
		sess->guami.plmn.mcc3 = plmn_id.mcc3;
		sess->guami.plmn.mnc1 = plmn_id.mnc1;
		sess->guami.plmn.mnc2 = plmn_id.mnc2;
		sess->guami.plmn.mnc3 = plmn_id.mnc3;

		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,
				"AMF_HandoverReq_UpdateNgapSess aMFSetID count=%d \n",
				GUAMI->aMFSetID.buf[0], GUAMI->aMFSetID.buf[1]);
	}

	if (UESecurityCapabilities != NULL) {
		sess->nRencryptionAlgorithms =
				ntohs(
						*((uint16_t* )UESecurityCapabilities->nRencryptionAlgorithms.buf));
		sess->nRintegrityProtectionAlgorithms =
				ntohs(
						*((uint16_t* )UESecurityCapabilities->nRintegrityProtectionAlgorithms.buf));
		sess->eUTRAencryptionAlgorithms =
				ntohs(
						*((uint16_t* )UESecurityCapabilities->eUTRAencryptionAlgorithms.buf));
		sess->eUTRAintegrityProtectionAlgorithms =
				ntohs(
						*((uint16_t* )UESecurityCapabilities->eUTRAintegrityProtectionAlgorithms.buf));
	}

	if (SecurityContext != NULL) {
		// TODO: handle SecurityContext
	}

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,
			"AMF_HandoverReq_UpdateNgapSess maxBitrateUl=%lld maxBitrateDl=%lld allowedSNnsaiSD=%d allowedSNnsaiSST=%d \n",
			sess->maxBitrateUl, sess->maxBitrateDl, sess->allowedSNnsaiSD,
			sess->allowedSNnsaiSST);
	return true;
}


bool AMF_HandoverReq_UpdatePduSess(NgapMessage& info, Ngap_PDUSessionResourceSetupListHOReq_t	*PDUSessionResourceSetupListHOReq)
{
	int i = 0;
	int ivec = 0;
	bool bMediaProcess = false;
	NgapProtocol ngapDecode;
	Ngap_PDUSessionResourceSetupRequestTransfer_t *pRequetTransfer = NULL;
	Ngap_PDUSessionResourceSetupRequestTransferIEs_t *pduie = NULL;
	Ngap_UPTransportLayerInformation_t *UPTransportLayerInformation = NULL;
	Ngap_PDUSessionAggregateMaximumBitRate_t *PDUSessionAggregateMaximumBitRate =
			NULL;
	Ngap_PDUSessionType_t *PDUSessionType = NULL;
	Ngap_QosFlowSetupRequestList_t *QosFlowSetupRequestList = NULL;
	Ngap_PDUSessionResourceSetupItemHOReq *item = NULL;
	pdu_session_resource_t pdu_resource;
	uint32_t teid = 0;
	uint32_t teidv6 = 0;
	socklen_t addrlen = 0;
	socklen_t addrlenv6 = 0;
	agc_std_sockaddr_t addr;
	agc_std_sockaddr_t addrv6;
	uint8_t pdu_resource_id = 0;

	NgapSession *sess = NULL;
	if (GetNgapSessManager().GetNgapSession(info.sessId, &sess) == false) {
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,
				"AMF_HandoverReq_UpdatePduSess fail to get sess=%d \n",
				info.sessId);
		return false;
	}

	memset(&pdu_resource, 0, sizeof(pdu_session_resource_t));

	if (PDUSessionResourceSetupListHOReq == NULL)
		return false;
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,
			"AMF_HandoverReq_UpdatePduSess ListCxtReq count=%d \n",
			PDUSessionResourceSetupListHOReq->list.count);

	sess->pdu_resource_count = PDUSessionResourceSetupListHOReq->list.count;
	sess->pdu_sess_rsp_count = 0;

	for (i = 0; i < PDUSessionResourceSetupListHOReq->list.count; i++) {
		item = (Ngap_PDUSessionResourceSetupItemHOReq*) PDUSessionResourceSetupListHOReq->list.array[i];

		OCTET_STRING_t *tranfer = &item->handoverRequestTransfer;
		pdu_resource_id = item->pDUSessionID;
		int32_t len = tranfer->size;
		ngapDecode.DecodeResourceSetupRequestTransfer((char*) tranfer->buf,
				(int32_t*) &len, (void**) &pRequetTransfer);

		if (pRequetTransfer == NULL) {
			continue;
		}

		for (int j = 0; j < pRequetTransfer->protocolIEs.list.count; j++) {
			pduie = pRequetTransfer->protocolIEs.list.array[j];
			switch (pduie->id) {
			case Ngap_ProtocolIE_ID_id_PDUSessionAggregateMaximumBitRate:
				PDUSessionAggregateMaximumBitRate =
						&pduie->value.choice.PDUSessionAggregateMaximumBitRate;
				break;
			case Ngap_ProtocolIE_ID_id_PDUSessionType:
				PDUSessionType = &pduie->value.choice.PDUSessionType;
				break;
			case Ngap_ProtocolIE_ID_id_QosFlowSetupRequestList:
				QosFlowSetupRequestList =
						&pduie->value.choice.QosFlowSetupRequestList;
				break;
			case Ngap_ProtocolIE_ID_id_UL_NGU_UP_TNLInformation:
				UPTransportLayerInformation =
						&pduie->value.choice.UPTransportLayerInformation;
				break;
			}
		}

		if (PDUSessionType != NULL && UPTransportLayerInformation != NULL
				&& UPTransportLayerInformation->present
						== Ngap_UPTransportLayerInformation_PR_gTPTunnel) {
			struct Ngap_GTPTunnel *gTPTunnel =
					UPTransportLayerInformation->choice.gTPTunnel;

			teid = ((uint32_t) gTPTunnel->gTP_TEID.buf[0] << 24)
					| ((uint32_t) gTPTunnel->gTP_TEID.buf[1] << 16)
					| ((uint32_t) gTPTunnel->gTP_TEID.buf[2] << 8)
					| ((uint32_t) gTPTunnel->gTP_TEID.buf[3]);

			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,
					"AMF_HandoverReq_UpdatePduSess teid=0x%x size=%d, buf=0x%x%x%x%x\n",
					teid, gTPTunnel->gTP_TEID.size, gTPTunnel->gTP_TEID.buf[0],
					gTPTunnel->gTP_TEID.buf[1], gTPTunnel->gTP_TEID.buf[2],
					gTPTunnel->gTP_TEID.buf[3]);

			if (4 == gTPTunnel->transportLayerAddress.size)//only ipv4
			{
				struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in*) &addr;
				local_addr1_v4->sin_family = AF_INET;
				//local_addr1_v4->sin_addr = agc_atoui((char *)gTPTunnel->transportLayerAddress.buf);
				memcpy(&local_addr1_v4->sin_addr,
					gTPTunnel->transportLayerAddress.buf, 4);
				addrlen = sizeof(struct sockaddr_in);

				agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,
					"AMF_HandoverReq_UpdatePduSess ipv4 in HO Req Msg=%d.%d.%d.%d\n",
					gTPTunnel->transportLayerAddress.buf[0],
					gTPTunnel->transportLayerAddress.buf[1],
					gTPTunnel->transportLayerAddress.buf[2],
					gTPTunnel->transportLayerAddress.buf[3]);
			}
			else if (16 == gTPTunnel->transportLayerAddress.size)//only ipv6
			{
				struct sockaddr_in6 *local_addr1_v6 =
					(struct sockaddr_in6*) &addrv6;
				local_addr1_v6->sin6_family = AF_INET6;
				memcpy(&local_addr1_v6->sin6_addr,
					gTPTunnel->transportLayerAddress.buf, 16);
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
				agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AMF_HandoverReq_UpdatePduSess gTP IP.size length is invalid\n");
			}

			bMediaProcess = GetPduSessManager().processSessSetupReq(info,
					pdu_resource_id, teid, addr, addrlen, teidv6, addrv6,
					addrlenv6);
		}

		//first save pdu_resource_id into gateway
		for (ivec = 0; ivec < MAX_PDU_SESSION_SIZE; ivec++) {
			if (sess->vecPduSessionID[ivec] == INVALID_PDU_SESSION_ID) {
				sess->vecPduSessionID[ivec] = pdu_resource_id;
				break;
			}
		}

		if (ivec >= MAX_PDU_SESSION_SIZE)
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR,
					"AMF_HandoverReq_UpdatePduSess ivec=%d \n",
					ivec);

		pdu_resource.pdu_session_id = pdu_resource_id;

		if (item->s_NSSAI.sD) {
			pdu_resource.s_nnsai.sd |= item->s_NSSAI.sD->buf[0] << 16;
			pdu_resource.s_nnsai.sd |= item->s_NSSAI.sD->buf[1] << 8;
			pdu_resource.s_nnsai.sd |= item->s_NSSAI.sD->buf[2];
		}

		pdu_resource.s_nnsai.sst = *((uint8_t*) item->s_NSSAI.sST.buf);
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,
				"AMF_HandoverReq_UpdatePduSess allowedSNnsaiSD=%d allowedSNnsaiSST=%d \n",
				pdu_resource.s_nnsai.sd, pdu_resource.s_nnsai.sst);

		if (PDUSessionAggregateMaximumBitRate != NULL) {
			uintmax_t maxBitrateUl;
			uintmax_t maxBitrateDl;
			asn_INTEGER2umax(
					&PDUSessionAggregateMaximumBitRate->pDUSessionAggregateMaximumBitRateUL,
					&maxBitrateUl);
			asn_INTEGER2umax(
					&PDUSessionAggregateMaximumBitRate->pDUSessionAggregateMaximumBitRateDL,
					&maxBitrateDl);

			pdu_resource.maxbitrate_dl = maxBitrateDl;
			pdu_resource.maxbitrate_ul = maxBitrateUl;
		}

		if (QosFlowSetupRequestList != NULL) {
			Ngap_QosFlowSetupRequestItem_t *item = NULL;
			for (int k = 0; k < QosFlowSetupRequestList->list.count; k++) {
				item = QosFlowSetupRequestList->list.array[k];
				pdu_resource.qos_flow_id = item->qosFlowIdentifier;
				if (item->qosFlowLevelQosParameters.qosCharacteristics.present
						== Ngap_QosCharacteristics_PR_nonDynamic5QI) {
					struct Ngap_NonDynamic5QIDescriptor *nonDynamic5QI =
							item->qosFlowLevelQosParameters.qosCharacteristics.choice.nonDynamic5QI;
					if (nonDynamic5QI) {
						pdu_resource.choice.nonvalue.fiveQI =
								nonDynamic5QI->fiveQI;
						if (nonDynamic5QI->priorityLevelQos)
							pdu_resource.choice.nonvalue.priorityLevelQos =
									*((long*) nonDynamic5QI->priorityLevelQos);
					}
					pdu_resource.qos_character_type =
							pdu_session_resource_t::QOS_NON_DYNAMIC_5QI;
				} else if (item->qosFlowLevelQosParameters.qosCharacteristics.present
						== Ngap_QosCharacteristics_PR_nonDynamic5QI) {
					struct Ngap_Dynamic5QIDescriptor *dynamic5QI =
							item->qosFlowLevelQosParameters.qosCharacteristics.choice.dynamic5QI;
					if (dynamic5QI) {
						pdu_resource.choice.value.priorityLevelQos =
								dynamic5QI->priorityLevelQos;
						pdu_resource.choice.value.packetDelayBudget =
								dynamic5QI->packetDelayBudget;
						pdu_resource.choice.value.pERScalar =
								dynamic5QI->packetErrorRate.pERScalar;
						pdu_resource.choice.value.pERExponent =
								dynamic5QI->packetErrorRate.pERExponent;
					}
					pdu_resource.qos_character_type =
							pdu_session_resource_t::QOS_DYNAMIC_5QI;
				} else {
					agc_log_printf(AGC_LOG, AGC_LOG_ERROR,
							"AMF_HandoverReq_UpdatePduSess present=%d \n",
							item->qosFlowLevelQosParameters.qosCharacteristics.present);
				}

				pdu_resource.priorityLevelARP =
						item->qosFlowLevelQosParameters.allocationAndRetentionPriority.priorityLevelARP;
				pdu_resource.pre_emptionCapability =
						item->qosFlowLevelQosParameters.allocationAndRetentionPriority.pre_emptionCapability;
				pdu_resource.pre_emptionVulnerability =
						item->qosFlowLevelQosParameters.allocationAndRetentionPriority.pre_emptionVulnerability;

			}
		}

		GetPduSessManager().UpdatePduSessResource(info.sessId, pdu_resource_id,
				pdu_resource);
		ngapDecode.FreeResourceSetupRequestTransfer(pRequetTransfer);
	}

	return bMediaProcess;
}


//lyb 2020-5-15
bool AMF_HandoverReq(NgapMessage& info)
{
	Ngap_InitiatingMessage_t	*initiatingMessage = NULL;
	Ngap_HandoverRequest_t	    *HandoverRequest = NULL;
	Ngap_HandoverRequestIEs_t   *ie = NULL;

	Ngap_AMF_UE_NGAP_ID_t     *AMF_UE_NGAP_ID = NULL;
	Ngap_SourceToTarget_TransparentContainer_t *srcTargetContainer = NULL;
	Ngap_UEAggregateMaximumBitRate *UEAggregateMaximumBitRate = NULL;
	Ngap_UESecurityCapabilities_t *UESecurityCapabilities = NULL;
	Ngap_SecurityContext_t *SecurityContext = NULL;
	Ngap_AllowedNSSAI_t *AllowedNSSAI = NULL;
	Ngap_GUAMI_t *GUAMI = NULL;
	Ngap_PDUSessionResourceSetupListHOReq_t *PDUSessionResourceSetupListHOReq = NULL;
	bool bMediaProcess = false;
	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;

	HandoverRequest = &initiatingMessage->value.choice.HandoverRequest;

	for (i = 0; i < HandoverRequest->protocolIEs.list.count; i++) {
		ie = HandoverRequest->protocolIEs.list.array[i];
		switch (ie->id) {
		case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
			AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
			break;

		case Ngap_ProtocolIE_ID_id_SourceToTarget_TransparentContainer:
			srcTargetContainer =
					&ie->value.choice.SourceToTarget_TransparentContainer;
			break;

		case Ngap_ProtocolIE_ID_id_UEAggregateMaximumBitRate:
			UEAggregateMaximumBitRate =
					&ie->value.choice.UEAggregateMaximumBitRate;
			break;

		case Ngap_ProtocolIE_ID_id_AllowedNSSAI:
			AllowedNSSAI = &ie->value.choice.AllowedNSSAI;
			break;

		case Ngap_ProtocolIE_ID_id_GUAMI:
			GUAMI = &ie->value.choice.GUAMI;
			break;

		case Ngap_ProtocolIE_ID_id_UESecurityCapabilities:
			UESecurityCapabilities = &ie->value.choice.UESecurityCapabilities;
			break;

		case Ngap_ProtocolIE_ID_id_SecurityContext:
			SecurityContext = &ie->value.choice.SecurityContext;
			break;

		case Ngap_ProtocolIE_ID_id_PDUSessionResourceSetupListHOReq:
			PDUSessionResourceSetupListHOReq =
					&ie->value.choice.PDUSessionResourceSetupListHOReq;
			break;

		default:
			break;
		}
	}

	if (srcTargetContainer == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AMF_HandoverReq fail to get Ngap_SourceToTarget_TransparentContainer_t.\n");
		return false;
	}

	/* Get target gnbId */
	NgapProtocol ngapDecode;
	Ngap_SourceNGRANNode_ToTargetNGRANNode_TransparentContainer *pContainer = NULL;
	int32_t len = srcTargetContainer->size;
	ngapDecode.DecodeS2TTransparentContainer((char *)srcTargetContainer->buf, (int32_t *)&len, (void **)&pContainer);

	Ngap_NGRAN_CGI_t *pCgi = &pContainer->targetCell_ID;
	if (pCgi->present !=  Ngap_NGRAN_CGI_PR_nR_CGI) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AMF_HandoverReq fail errot Ngap_NGRAN_CGI_t present=%d.\n", pCgi->present);
	}
	Ngap_NR_CGI_t * nrCgi =(Ngap_NR_CGI_t *)pCgi->choice.nR_CGI;
	Ngap_NRCellIdentity_t *nrCell = &nrCgi->nRCellIdentity; /* 36bits nrCgi, 40bits BIT-STRING, 4bits unused*/

	// TODO: 需要RanNodeId的长度可以配置，这里处理成24位长度
	uint32_t gNodeId = 0;
	gNodeId += ((uint32_t)nrCell->buf[0]) << 16;
	gNodeId += ((uint32_t)nrCell->buf[1]) << 8;
	gNodeId += ((uint32_t)nrCell->buf[2]);
	ngapDecode.FreeS2TTransparentContainer(pContainer);

	NgapSession *sess = NULL;
	if (GetNgapSessManager().NewNgapSessionFromAmf(info, &sess, gNodeId) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AMF_HandoverReq fail NewNgapSessionFromAmf().\n");
		return false;
	}
	uint32_t gwSessionId = sess->sessId;
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "AMF_HandoverReq new NgapSession id=%d.\n", gwSessionId);
	if (AMF_ConvertNgapId(info, (unsigned long *)&gwSessionId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AMF_HandoverReq fail AMF_ConvertNgapId().\n");
		return false;
	}

	AMF_HandoverReq_UpdateNgapSess(info, UEAggregateMaximumBitRate,
			UESecurityCapabilities, SecurityContext, AllowedNSSAI, GUAMI);

	bMediaProcess = AMF_HandoverReq_UpdatePduSess(info, PDUSessionResourceSetupListHOReq);

	if (!bMediaProcess) {
		GetNsmManager().SendDownLayerSctp(info);
	}
	return true;
}

bool AMF_HandoverReqByMedia(NgapMessage &info) {

	Ngap_InitiatingMessage_t	*initiatingMessage = NULL;
	Ngap_HandoverRequest_t	    *HandoverRequest = NULL;
	Ngap_HandoverRequestIEs_t   *ie = NULL;

	Ngap_PDUSessionResourceSetupListHOReq_t *PDUSessionResourceSetupListHOReq = NULL;
	Ngap_PDUSessionResourceSetupRequestTransfer_t *pRequetTransfer = NULL;

	int i = 0;
	int j = 0;

	NgapProtocol ngapDecode;
	OCTET_STRING_t *tranfer = NULL;
	uint8_t pdu_resource_id = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
	HandoverRequest =
			&initiatingMessage->value.choice.HandoverRequest;

	for (i = 0; i < HandoverRequest->protocolIEs.list.count; i++) {
		ie = HandoverRequest->protocolIEs.list.array[i];
		switch (ie->id) {
		case Ngap_ProtocolIE_ID_id_PDUSessionResourceSetupListHOReq:
			PDUSessionResourceSetupListHOReq =
					&ie->value.choice.PDUSessionResourceSetupListHOReq;
			break;
		default:
			break;
		}
	}

	if (PDUSessionResourceSetupListHOReq == NULL) {
		GetNsmManager().SendDownLayerSctp(info);
		return true;
	}

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"AMF_HandoverReqByMedia PDUSessionResourceSetupListHOReq->list.count = %d\n",PDUSessionResourceSetupListHOReq->list.count);

	for (i = 0; i < PDUSessionResourceSetupListHOReq->list.count; i++) {
		Ngap_PDUSessionResourceSetupItemHOReq *item =
				(Ngap_PDUSessionResourceSetupItemHOReq*) PDUSessionResourceSetupListHOReq->list.array[i];
		tranfer = &item->handoverRequestTransfer;

		int32_t len = tranfer->size;
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"AMF_HandoverReqByMedia handoverRequestTransfer.len = %d\n",len);

		ngapDecode.DecodeResourceSetupRequestTransfer((char*) tranfer->buf,
				(int32_t*) &len, (void**) &pRequetTransfer);
		pdu_resource_id = item->pDUSessionID;

		if (pRequetTransfer != NULL) {
			uint32_t teid = 0;
			uint32_t teid_v6 = 0;
			agc_std_sockaddr_t addr;
			agc_std_sockaddr_t addr_v6;

			//use gnb info
			GetPduSessManager().GetGnbGtpInfo(info.sessId, pdu_resource_id,
					addr, teid, addr_v6, teid_v6);

			Ngap_PDUSessionResourceSetupRequestTransferIEs_t *pduie = NULL;
			Ngap_UPTransportLayerInformation_t *UPTransportLayerInformation = NULL;

			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"AMF_HandoverReqByMedia pRequetTransfer->protocolIEs.list.count = %d\n", pRequetTransfer->protocolIEs.list.count);

			for (j = 0; j < pRequetTransfer->protocolIEs.list.count; j++) {
				pduie = pRequetTransfer->protocolIEs.list.array[j];
				switch (pduie->id) {
				case Ngap_ProtocolIE_ID_id_UL_NGU_UP_TNLInformation:
					UPTransportLayerInformation =
							&pduie->value.choice.UPTransportLayerInformation;
					break;
				}
			}

			if (UPTransportLayerInformation != NULL
					&& UPTransportLayerInformation->present
							== Ngap_UPTransportLayerInformation_PR_gTPTunnel) {
				struct Ngap_GTPTunnel *gTPTunnel =
						UPTransportLayerInformation->choice.gTPTunnel;

				if ((char*) gTPTunnel->gTP_TEID.buf == NULL) {
					gTPTunnel->gTP_TEID.buf = (uint8_t*) calloc(4,
							sizeof(uint8_t));
					gTPTunnel->gTP_TEID.size = 4;
				}
				gTPTunnel->gTP_TEID.buf[0] = teid >> 24;
				gTPTunnel->gTP_TEID.buf[1] = teid >> 16;
				gTPTunnel->gTP_TEID.buf[2] = teid >> 8;
				gTPTunnel->gTP_TEID.buf[3] = teid;
				
				agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"AMF_HandoverReqByMedia to gnb teid = %d,buf[%x%x%x%x]\n", teid,gTPTunnel->gTP_TEID.buf[0],gTPTunnel->gTP_TEID.buf[1],gTPTunnel->gTP_TEID.buf[2],gTPTunnel->gTP_TEID.buf[3]);
				agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"AMF_HandoverReqByMedia gTP IP.size=%d \n", gTPTunnel->transportLayerAddress.size);
				
				if (gTPTunnel->transportLayerAddress.buf != NULL)
					free(gTPTunnel->transportLayerAddress.buf);

				if(4 == gTPTunnel->transportLayerAddress.size)//ipv4 only
				{
					if (addr.ss_family == AF_INET)
					{
						struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&addr;
					    gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(4, sizeof(uint8_t));
						memcpy(gTPTunnel->transportLayerAddress.buf, &local_addr1_v4->sin_addr, 4);
					}
				}
				else if(16 == gTPTunnel->transportLayerAddress.size)//ipv6 only
				{
					if (addr_v6.ss_family == AF_INET6) 
					{
						struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&addr_v6;
					    gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(16, sizeof(uint8_t));
						memcpy(gTPTunnel->transportLayerAddress.buf, &local_addr1_v6->sin6_addr, 16);
					}
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
					agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"AMF_HandoverReqByMedia gTP IP.size length is invalid\n");
				}
			}

			if (tranfer != NULL) {

				if (tranfer->buf != NULL)      {free(tranfer->buf);}
				
				tranfer->buf = (uint8_t *)calloc(200, sizeof(uint8_t));
			
				NgapProtocol ngapEncode;
				int32_t len = 0;
				ngapEncode.EncodeResourceSetupRequestTransfer(
						(void*) pRequetTransfer, (char*) tranfer->buf,
						(int32_t*) &len);
				tranfer->size = len;
			}
			ngapDecode.FreeResourceSetupRequestTransfer(pRequetTransfer);
		}
	}

	GetNsmManager().SendDownLayerSctp(info);

	return true;
}

//lyb 2020-5-15
bool AMF_HandoverCommand(NgapMessage& info)
{
	Ngap_SuccessfulOutcome_t	*successfulOutcome = NULL;
	Ngap_HandoverCommand_t	    *HandoverCommand = NULL;
	Ngap_HandoverCommandIEs_t   *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t	  *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t	  *AMF_UE_NGAP_ID = NULL;
	int i = 0;

	successfulOutcome = info.ngapMessage.choice.successfulOutcome;
	//d_assert(successfulOutcome, return,);
	
	HandoverCommand = &successfulOutcome->value.choice.HandoverCommand;
	//d_assert(HandoverCommand, return,);


//			union Ngap_HandoverCommandIEs__Ngap_value_u {
//				Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
//				Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
//				Ngap_HandoverType_t  HandoverType;
//				Ngap_NASSecurityParametersFromNGRAN_t	 NASSecurityParametersFromNGRAN;
//				Ngap_PDUSessionResourceHandoverList_t	 PDUSessionResourceHandoverList;
//				Ngap_PDUSessionResourceToReleaseListHOCmd_t  PDUSessionResourceToReleaseListHOCmd;
//				Ngap_TargetToSource_TransparentContainer_t	 TargetToSource_TransparentContainer;
//				Ngap_CriticalityDiagnostics_t	 CriticalityDiagnostics;
//			} choice;


	for (i = 0; i < HandoverCommand->protocolIEs.list.count; i++)
	{
		ie = HandoverCommand->protocolIEs.list.array[i];
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

	if (AMF_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"AMF_HandoverCommand fail to AMF_ConvertNgapId ngap session\n");
		return false;
	}

	GetNsmManager().SendDownLayerSctp(info);

	return true;
}


//lyb 2020-5-15
bool AMF_HandoverPreparationFailure(NgapMessage& info)
{
	Ngap_UnsuccessfulOutcome_t	*unsuccessfulOutcome = NULL;
	Ngap_HandoverPreparationFailure_t	 *HandoverPreparationFailure = NULL;
	Ngap_HandoverPreparationFailureIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t	  *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t	  *AMF_UE_NGAP_ID = NULL;
	int i = 0;

	unsuccessfulOutcome = info.ngapMessage.choice.unsuccessfulOutcome;
	//d_assert(unsuccessfulOutcome, return,);
	
	HandoverPreparationFailure = &unsuccessfulOutcome->value.choice.HandoverPreparationFailure;
	//d_assert(HandoverPreparationFailure, return,);

//			union Ngap_HandoverPreparationFailureIEs__Ngap_value_u {
//				Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
//				Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
//				Ngap_Cause_t	 Cause;
//				Ngap_CriticalityDiagnostics_t	 CriticalityDiagnostics;
//			} choice;


	for (i = 0; i < HandoverPreparationFailure->protocolIEs.list.count; i++)
	{
		ie = HandoverPreparationFailure->protocolIEs.list.array[i];
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

	
	if (AMF_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"AMF_HandoverPreparationFailure fail to AMF_ConvertNgapId ngap session\n");
		return false;
	}

	GetNsmManager().SendDownLayerSctp(info);

	return true;
}



//lyb 2020-5-18
bool AMF_PathSwitchReqAck(NgapMessage& info)
{
	Ngap_SuccessfulOutcome_t	*successfulOutcome = NULL;
	Ngap_PathSwitchRequestAcknowledge_t	    *PathSwitchRequestAcknowledge = NULL;
	Ngap_PathSwitchRequestAcknowledgeIEs_t	*ie = NULL;
	Ngap_PDUSessionResourceSwitchedList_t   *pduSessionResourceSwitchList = NULL;
	OCTET_STRING_t *tranfer = NULL;

	Ngap_PathSwitchRequestAcknowledgeTransfer_t *PathSwitchRequestAckTransfer = NULL;

	NgapProtocol ngapDecode;

	Ngap_RAN_UE_NGAP_ID_t	  *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t	  *AMF_UE_NGAP_ID = NULL;
	Ngap_SecurityContext_t	 *SecurityContext = NULL;
	int i = 0;

	successfulOutcome = info.ngapMessage.choice.successfulOutcome;
	//d_assert(successfulOutcome, return,);
	
	PathSwitchRequestAcknowledge = &successfulOutcome->value.choice.PathSwitchRequestAcknowledge;
	//d_assert(PathSwitchRequestAcknowledge, return,);


	for (i = 0; i < PathSwitchRequestAcknowledge->protocolIEs.list.count; i++)
	{
		ie = PathSwitchRequestAcknowledge->protocolIEs.list.array[i];
		switch (ie->id)
		{
			case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
				RanUeNgapId = &ie->value.choice.RAN_UE_NGAP_ID;
				break;
			
			case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
				AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
				break;
			
			case Ngap_ProtocolIE_ID_id_SecurityContext:
				SecurityContext = &ie->value.choice.SecurityContext;
				break;
			case Ngap_ProtocolIE_ID_id_PDUSessionResourceSwitchedList:
				pduSessionResourceSwitchList = &ie->value.choice.PDUSessionResourceSwitchedList;
				break;
			default:
				break;
		}
	}

	if (AMF_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"AMF_PathSwitchReqAck fail to AMF_ConvertNgapId ngap session\n");
		return false;
	}

	NgapSession *sess = NULL;
	if (GetNgapSessManager().GetNgapSession(info.sessId, &sess) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AMF_PathSwitchReqAck getNgapSession fail to get ngap session, sessId=%d\n", info.sessId);
		return false;
	}

	if (pduSessionResourceSwitchList != NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"AMF_PathSwitchReqAck ListCxtRes count=%d \n",
		 		pduSessionResourceSwitchList->list.count);
		
		for(i = 0; i < pduSessionResourceSwitchList->list.count; i++) {
		
			Ngap_PDUSessionResourceSwitchedItem_t *item = (Ngap_PDUSessionResourceSwitchedItem_t *)pduSessionResourceSwitchList->list.array[i];
			
			tranfer = &item->pathSwitchRequestAcknowledgeTransfer;
			int32_t len = tranfer->size;

			/* see IE details in specs-38.413 #9.3.4.8 */
			ngapDecode.DecodePathSwtichReqAckTransfer((char *)tranfer->buf, (int32_t *)&len, (void **)&PathSwitchRequestAckTransfer);
			uint8_t pdu_resource_id = item->pDUSessionID;


			if (PathSwitchRequestAckTransfer != NULL)
			{
				uint32_t teid;
				uint32_t teid_v6;
				agc_std_sockaddr_t addr;
				agc_std_sockaddr_t addr_v6;

				GetPduSessManager().GetGnbGtpInfo(info.sessId, pdu_resource_id, addr, teid, addr_v6, teid_v6);

				/* change ip */
				struct Ngap_GTPTunnel *gTPTunnel = PathSwitchRequestAckTransfer->uL_NGU_UP_TNLInformation->choice.gTPTunnel;

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

				agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"AMF_PathSwitchReqAck gTP IP.size=%d \n", gTPTunnel->transportLayerAddress.size);

				if(4 == gTPTunnel->transportLayerAddress.size)//ipv4 only
				{
					if (addr.ss_family == AF_INET)
					{
						struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&addr;
						gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(4, sizeof(uint8_t));
						memcpy(gTPTunnel->transportLayerAddress.buf, &local_addr1_v4->sin_addr, 4);
					}
				}
				else if(16 == gTPTunnel->transportLayerAddress.size)//ipv6 only
				{
					if (addr_v6.ss_family == AF_INET6) 
					{
						struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&addr_v6;
						gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(16, sizeof(uint8_t));
						memcpy(gTPTunnel->transportLayerAddress.buf, &local_addr1_v6->sin6_addr, 16);
					}
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
					agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"AMF_PathSwitchReqAck gTP IP.size length is invalid\n");
				}

				if (tranfer != NULL)
				{
					if (tranfer->buf != NULL)
						free(tranfer->buf);
					tranfer->buf = (uint8_t *)calloc(100, sizeof(uint8_t));
					NgapProtocol ngapEncode;
					int32_t len = 0;
					ngapEncode.EncodePathSwtichReqAckTransfer((void *)PathSwitchRequestAckTransfer, (char *)tranfer->buf, (int32_t *)&len);
					//modify  Encode bug of out parameter - len
					tranfer->size = len + 1;
				}
				ngapDecode.FreePatchSwitchReqAckTransfer(PathSwitchRequestAckTransfer);
			}
		}
	}
	
	Ngap_SourceToTarget_TransparentContainer_t	 *SourceToTarget_TransparentContainer = NULL;
	GetNgapSessManager().GetSourceToTarget_TransparentContainer(info.sessId, &SourceToTarget_TransparentContainer);

	if (Ralation_RAN_Handover_IN_SAME_GW == sess->eHandoverflag && SourceToTarget_TransparentContainer != NULL)
	{
		sess->nextHopChainingCount = SecurityContext->nextHopChainingCount;
		memcpy(sess->securityContext, SecurityContext->nextHopNH.buf, SecurityContext->nextHopNH.size);
		
		EsmContext *esmContext = NULL;
		if (GetEsmManager().GetEsmContext(sess->TgtGnbId, &esmContext) == false)
		{
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AMF_PathSwitchReqAck fail to get target esm context.\n");
			return false;
		}
		
		//prepare info for send Handover-Req to target RAN
		info.gnbUeId = INVALIDE_ID;
		info.ranNodeId = esmContext->ranNodeId;
		info.sockTarget = esmContext->sctp_index;
		
		GetEsmManager().SendHandoverRequest(info, SourceToTarget_TransparentContainer);
	}
	else
	{
		GetNsmManager().SendDownLayerSctp(info);
	}
	return true;

}



//lyb 2020-5-18
bool AMF_PathSwitchReqFailure(NgapMessage& info)
{
	Ngap_UnsuccessfulOutcome_t	*unsuccessfulOutcome = NULL;
	Ngap_PathSwitchRequestFailure_t	   *PathSwitchRequestFailure = NULL;
	Ngap_PathSwitchRequestFailureIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t	  *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t	  *AMF_UE_NGAP_ID = NULL;
	int i = 0;

	unsuccessfulOutcome = info.ngapMessage.choice.unsuccessfulOutcome;
	//d_assert(unsuccessfulOutcome, return,);
	
	PathSwitchRequestFailure = &unsuccessfulOutcome->value.choice.PathSwitchRequestFailure;
	//d_assert(PathSwitchRequestFailure, return,);


	for (i = 0; i < PathSwitchRequestFailure->protocolIEs.list.count; i++)
	{
		ie = PathSwitchRequestFailure->protocolIEs.list.array[i];
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

	
	if (AMF_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"AMF_PathSwitchReqFailure fail to AMF_ConvertNgapId ngap session\n");
		return false;
	}

	NgapSession *sess = NULL;
	if (GetNgapSessManager().GetNgapSession(info.sessId, &sess) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AMF_PathSwitchReqFailure getNgapSession fail to get ngap session, sessId=%d\n", info.sessId);
		return false;
	}

	if (Ralation_RAN_Handover_IN_SAME_GW == sess->eHandoverflag)
	{
		info.amfUeId   = sess->sessId;
		info.gnbUeId   = sess->gnbUeId;
		info.ranNodeId = sess->gnbId;
			
		sess->ResetHandoverInfo();
	
		GetEsmManager().SendHOPrepareFailure(info, NULL);
	}
	else
	{
		GetNsmManager().SendDownLayerSctp(info);
	}


	return true;
}


//lyb 2020-5-18
bool AMF_HandoverCancelAck(NgapMessage& info)
{
	Ngap_SuccessfulOutcome_t	          *successfulOutcome = NULL;
	Ngap_HandoverCancelAcknowledge_t      *HandoverCancelAcknowledge = NULL;
	Ngap_HandoverCancelAcknowledgeIEs_t   *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t	  *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t	  *AMF_UE_NGAP_ID = NULL;
	int i = 0;

	successfulOutcome = info.ngapMessage.choice.successfulOutcome;
	//d_assert(successfulOutcome, return,);
	
	HandoverCancelAcknowledge = &successfulOutcome->value.choice.HandoverCancelAcknowledge;
	//d_assert(HandoverCancelAcknowledge, return,);

//			union Ngap_HandoverCancelAcknowledgeIEs__Ngap_value_u {
//				Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
//				Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
//				Ngap_CriticalityDiagnostics_t	 CriticalityDiagnostics;
//			} choice;

	for (i = 0; i < HandoverCancelAcknowledge->protocolIEs.list.count; i++)
	{
		ie = HandoverCancelAcknowledge->protocolIEs.list.array[i];
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

	if (AMF_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"AMF_HandoverCancelAck fail to AMF_ConvertNgapId ngap session\n");
		return false;
	}

	GetNsmManager().SendDownLayerSctp(info);

	return true;

}



//lyb,2020-5-19
bool AMF_DLRanStatusTsfer(NgapMessage& info)
{
	Ngap_InitiatingMessage_t	*initiatingMessage = NULL;
	Ngap_DownlinkRANStatusTransfer_t	 *DownlinkRANStatusTransfer = NULL;
	Ngap_DownlinkRANStatusTransferIEs_t  *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t     *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t     *AMF_UE_NGAP_ID = NULL;
	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
//	d_assert(initiatingMessage, return,);
	
	DownlinkRANStatusTransfer = &initiatingMessage->value.choice.DownlinkRANStatusTransfer;
//	d_assert(DownlinkRANStatusTransfer, return,);

//			union Ngap_DownlinkRANStatusTransferIEs__Ngap_value_u {
//				Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
//				Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
//				Ngap_RANStatusTransfer_TransparentContainer_t	 RANStatusTransfer_TransparentContainer;
//			} choice;

	for (i = 0; i < DownlinkRANStatusTransfer->protocolIEs.list.count; i++)
	{
		ie = DownlinkRANStatusTransfer->protocolIEs.list.array[i];
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

	if (AMF_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"AMF_DLRanStatusTsfer fail to AMF_ConvertNgapId ngap session\n");
		return false;
	}

	GetNsmManager().SendDownLayerSctp(info);
	
	return true;
}




//lyb
bool AMF_Paging(NgapMessage& info)
{

	Ngap_InitiatingMessage_t	*initiatingMessage = NULL;
	Ngap_Paging_t	 *N2Paging = NULL;
	Ngap_PagingIEs_t *ie = NULL;
	Ngap_TAIListForPaging_t  *TAIListForPaging = NULL;
	uint32_t uTac;
	Ngap_AssistanceDataForPaging_t	 *AssistanceDataForPaging = NULL;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
//	d_assert(initiatingMessage, return,);
	
	N2Paging = &initiatingMessage->value.choice.Paging;
//	d_assert(N2Paging, return,);

//			union Ngap_PagingIEs__Ngap_value_u {
//				Ngap_UEPagingIdentity_t  UEPagingIdentity;
//				Ngap_PagingDRX_t	 PagingDRX;
//				Ngap_TAIListForPaging_t  TAIListForPaging;
//				Ngap_PagingPriority_t	 PagingPriority;
//				Ngap_UERadioCapabilityForPaging_t	 UERadioCapabilityForPaging;
//				Ngap_PagingOrigin_t  PagingOrigin;
//				Ngap_AssistanceDataForPaging_t	 AssistanceDataForPaging;
//			} choice;

	for (int i = 0; i < N2Paging->protocolIEs.list.count; i++)
	{
		ie = N2Paging->protocolIEs.list.array[i];
		switch (ie->id)
			{
				case Ngap_ProtocolIE_ID_id_TAIListForPaging:
					TAIListForPaging = &ie->value.choice.TAIListForPaging;
					break;

				case Ngap_ProtocolIE_ID_id_AssistanceDataForPaging:
					AssistanceDataForPaging = &ie->value.choice.AssistanceDataForPaging;
					break;
				
				default:
					break;			
			}
	}

	if(NULL != TAIListForPaging)
	{
		for (int i = 0; i < TAIListForPaging->list.count; i++)
		{	
			Ngap_TAIListForPagingItem_t *item = NULL;
			item = (Ngap_TAIListForPagingItem_t *)TAIListForPaging->list.array[i];
			 
			if(NULL != item)
			{
				uTac = ((item->tAI.tAC.buf[0]) << 16) + ((item->tAI.tAC.buf[1]) << 8)  + item->tAI.tAC.buf[2];

// search ran by tac and send paging to ran.

			}			
		}

	}


/*
	for (int i = 0; i < AssistanceDataForPaging->assistanceDataForRecommendedCells->recommendedCellsForPaging.recommendedCellList.list.count; i++)
	{	
		Ngap_RecommendedCellItem_t *item = NULL;
		item = (Ngap_RecommendedCellItem_t *)AssistanceDataForPaging->assistanceDataForRecommendedCells->recommendedCellsForPaging.recommendedCellList.list.array[i];
		 
		if(Ngap_NGRAN_CGI_PR_nR_CGI == item->nGRAN_CGI.present)
		{
			(item->nGRAN_CGI.choice.nR_CGI->nRCellIdentity.size)
		}			
	}
*/


	return true;
}

//lyb,2020-4-15
bool AMF_DownlinkNASTransport(NgapMessage& info)
{
	Ngap_InitiatingMessage_t	*initiatingMessage = NULL;
	Ngap_DownlinkNASTransport_t	 *N2DownlinkNASTransport = NULL;
	Ngap_DownlinkNASTransport_IEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t     *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t     *AMF_UE_NGAP_ID = NULL;
	Ngap_NAS_PDU_t	 *NAS_PDU = NULL;
	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
//	d_assert(initiatingMessage, return false,);
	
	N2DownlinkNASTransport = &initiatingMessage->value.choice.DownlinkNASTransport;
//	d_assert(N2DownlinkNASTransport, return false,);

//	union Ngap_DownlinkNASTransport_IEs__Ngap_value_u {
//		Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
//		Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
//		Ngap_AMFName_t	 AMFName;
//		Ngap_RANPagingPriority_t	 RANPagingPriority;
//		Ngap_NAS_PDU_t	 NAS_PDU;
//		Ngap_MobilityRestrictionList_t	 MobilityRestrictionList;
//		Ngap_IndexToRFSP_t	 IndexToRFSP;
//		Ngap_UEAggregateMaximumBitRate_t	 UEAggregateMaximumBitRate;
//		Ngap_AllowedNSSAI_t  AllowedNSSAI;
//	} choice;

	for (i = 0; i < N2DownlinkNASTransport->protocolIEs.list.count; i++)
	{
		ie = N2DownlinkNASTransport->protocolIEs.list.array[i];
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
				break;
			
			default:
				break;
		
		}
	
	}
	
	if (AMF_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
    	agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"AMF_DownlinkNASTransport fail to AMF_ConvertNgapId ngap session RanUeNgapId=%d\n", *RanUeNgapId);
		return false;
	}


	GetNsmManager().SendDownLayerSctp(info);

	GetNgapSessManager().StopNgapSessCheck(info.sessId);

	return true;

}

bool AMF_Reset (NgapMessage& info)
{
	Ngap_InitiatingMessage_t	*initiatingMessage = NULL;
	Ngap_NGReset_t	 			*NGReset = NULL;
	Ngap_NGResetIEs_t 			*ie = NULL;
	Ngap_ResetType_t	 		*ResetType = NULL;
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

		GetNgapSessManager().DeleteAllAmfSession(info.amfId);
		GetEsmManager().SendToAllRAN(info);
	}
	else if (ResetType->present == Ngap_ResetType_PR_partOfNG_Interface)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AMF_Reset part of NG isn't process.\n");
	}	
	
	GetAsmManager().SendResetAck(info);
	
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"AMF_Reset finished.\n");
	return true;
}

bool AMF_ResetAck (NgapMessage& info)
{
	
	return true;
}


//lyb,2020-4-15
bool AMF_ErrorIndication(NgapMessage& info)
{
	Ngap_InitiatingMessage_t	*initiatingMessage = NULL;
	Ngap_ErrorIndication_t	 *N2ErrorIndication = NULL;
	Ngap_ErrorIndicationIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t     *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t     *AMF_UE_NGAP_ID = NULL;
	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
//	d_assert(initiatingMessage, return,);
	
	N2ErrorIndication = &initiatingMessage->value.choice.ErrorIndication;
//	d_assert(N2ErrorIndication, return,);


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

//	d_assert(RanUeNgapId, return,);

	if(NULL == RanUeNgapId || NULL == AMF_UE_NGAP_ID)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AMF_ErrorIndication msg content err, ranUeId addr = (%x) , amfUeId addr = (%x).\n",RanUeNgapId,AMF_UE_NGAP_ID);
		return false;
	}
	
	if (AMF_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"AMF_ErrorIndication fail to AMF_ConvertNgapId ngap session\n");
		return false;
	}

	GetNsmManager().SendDownLayerSctp(info);

	return true;
}


bool AMF_NgSetupResponse(NgapMessage& info)
{
	uint32_t amfId = info.amfId;
	uint32_t sctp_index = info.sockSource;
	if (amfId == INVALIDE_ID)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"AMF_NgSetupResponse AMF Context can not be found by sctp\n");
		return false;
	}
	
	if (info.ProcCode == Ngap_ProcedureCode_id_NGSetup)
	{
        GetAsmManager().DelTimer(amfId);
        //PMMgr::Instance().ReportPM(PM_MSGFROM_MME, PM_MSGTO_GW);

		if (info.PDUChoice == Ngap_NGAP_PDU_PR_successfulOutcome)
		{
			GetAsmManager().handleNgSetupResponse(sctp_index, info);
			return true;
		}
		else
		{
			//m_IsSetupToMme = false;
			Ngap_NGSetupFailure_t *NGSetupFailure = &info.ngapMessage.choice.unsuccessfulOutcome->value.choice.NGSetupFailure;
			GetAsmManager().Clear(amfId);
            uint8_t timeToWait = 0;//(uint8_t)LTEConfig::Instance().GetS1APProCfg().TimetoWait_s;;
			Ngap_TimeToWait_t ngTimeToWait = 255;
			for (int i = 0; i < NGSetupFailure->protocolIEs.list.count; i++)
			{
				Ngap_NGSetupFailureIEs_t *ie = NGSetupFailure->protocolIEs.list.array[i];
				switch(ie->id)
				{
					case Ngap_ProtocolIE_ID_id_TimeToWait:
						ngTimeToWait = ie->value.choice.TimeToWait;
						break;
				}
			}		
			
			switch (ngTimeToWait)
			{
			case Ngap_TimeToWait_v1s:
				timeToWait = 1;
				break;
			case Ngap_TimeToWait_v2s:
				timeToWait = 2;
				break;
			case Ngap_TimeToWait_v5s:
				timeToWait = 5;
				break;
			case Ngap_TimeToWait_v10s:
				timeToWait = 10;
				break;
			case Ngap_TimeToWait_v20s:
				timeToWait = 20;
				break;
			case Ngap_TimeToWait_v60s:
				timeToWait = 60;
				break;
			}

			if(timeToWait == 0) 
				timeToWait = 5;

            timeToWait *= 1000;

			//GetAsmManager().AddTimer(amfId,timeToWait);

            //Timer::GetHighTimer().AddInstance((int)(void*)this, MME_SCTP_TIMER_ID, timeToWait);

			return false;
		}
	}
	return true;
}



//lyb,2020-5-11
bool AMF_UERadioCapabilityCheckReq(NgapMessage& info)
{
	Ngap_InitiatingMessage_t	*initiatingMessage = NULL;
	Ngap_UERadioCapabilityCheckRequest_t	 *N2UERadioCapabilityCheckRequest = NULL;
	Ngap_UERadioCapabilityCheckRequestIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t     *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t     *AMF_UE_NGAP_ID = NULL;
	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
//	d_assert(initiatingMessage, return,);
	
	N2UERadioCapabilityCheckRequest = &initiatingMessage->value.choice.UERadioCapabilityCheckRequest;
//	d_assert(N2UERadioCapabilityCheckRequest, return,);

//			union Ngap_UERadioCapabilityCheckRequestIEs__Ngap_value_u {
//				Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
//				Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
//				Ngap_UERadioCapability_t	 UERadioCapability;
//			} choice;

	for (i = 0; i < N2UERadioCapabilityCheckRequest->protocolIEs.list.count; i++)
		{
			ie = N2UERadioCapabilityCheckRequest->protocolIEs.list.array[i];
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

//	d_assert(RanUeNgapId, return,);

	if (AMF_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"AMF_UERadioCapabilityCheckReq fail to AMF_ConvertNgapId ngap session\n");
		return false;
	}

	GetNsmManager().SendDownLayerSctp(info);

	return true;
}



//lyb,2020-5-11
bool AMF_PDUSessionResourceModifyConfirm(NgapMessage& info)
{
	Ngap_SuccessfulOutcome_t	*successfulOutcome = NULL;
	Ngap_PDUSessionResourceModifyConfirm_t	 *N2PDUSessionResourceModifyConfirm = NULL;	
	Ngap_PDUSessionResourceModifyConfirmIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t     *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t     *AMF_UE_NGAP_ID = NULL;	
	int i = 0;

	successfulOutcome = info.ngapMessage.choice.successfulOutcome;
//	d_assert(successfulOutcome, return,);
	
	N2PDUSessionResourceModifyConfirm = &successfulOutcome->value.choice.PDUSessionResourceModifyConfirm;
//	d_assert(N2PDUSessionResourceModifyConfirm, return,);

//			union Ngap_PDUSessionResourceModifyConfirmIEs__Ngap_value_u {
//				Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
//				Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
//				Ngap_PDUSessionResourceModifyListModCfm_t	 PDUSessionResourceModifyListModCfm;
//				Ngap_PDUSessionResourceFailedToModifyListModCfm_t	 PDUSessionResourceFailedToModifyListModCfm;
//				Ngap_CriticalityDiagnostics_t	 CriticalityDiagnostics;
//			} choice;

	for (i = 0; i < N2PDUSessionResourceModifyConfirm->protocolIEs.list.count; i++)
		{
			ie = N2PDUSessionResourceModifyConfirm->protocolIEs.list.array[i];
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

//	d_assert(RanUeNgapId, return,);

	if (AMF_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"AMF_PDUSessionResourceModifyConfirm fail to AMF_ConvertNgapId ngap session\n");
		return false;
	}

	GetNsmManager().SendDownLayerSctp(info);

	return true;
}


//lyb,2020-5-11
bool AMF_TraceStart(NgapMessage& info)
{
	Ngap_InitiatingMessage_t	*initiatingMessage = NULL;
	Ngap_TraceStart_t	 *N2TraceStart = NULL;
	Ngap_TraceStartIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t     *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t     *AMF_UE_NGAP_ID = NULL;
	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
//	d_assert(initiatingMessage, return,);
	
	N2TraceStart = &initiatingMessage->value.choice.TraceStart;
//	d_assert(N2TraceStart, return,);

//			union Ngap_TraceStartIEs__Ngap_value_u {
//				Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
//				Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
//				Ngap_TraceActivation_t	 TraceActivation;
//			} choice;

	for (i = 0; i < N2TraceStart->protocolIEs.list.count; i++)
		{
			ie = N2TraceStart->protocolIEs.list.array[i];
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

//	d_assert(RanUeNgapId, return,);

	if (AMF_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"AMF_TraceStart fail to AMF_ConvertNgapId ngap session\n");
		return false;
	}

	GetNsmManager().SendDownLayerSctp(info);

	return true;
}



//lyb,2020-5-11
bool AMF_DeActTrace(NgapMessage& info)
{
	Ngap_InitiatingMessage_t	*initiatingMessage = NULL;
	Ngap_DeactivateTrace_t	 *N2DeactivateTrace = NULL;
	Ngap_DeactivateTraceIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t     *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t     *AMF_UE_NGAP_ID = NULL;
	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
//	d_assert(initiatingMessage, return,);
	
	N2DeactivateTrace = &initiatingMessage->value.choice.DeactivateTrace;
//	d_assert(N2DeactivateTrace, return,);

//		union Ngap_DeactivateTraceIEs__Ngap_value_u {
//			Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
//			Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
//			Ngap_NGRANTraceID_t	 NGRANTraceID;
//		} choice;

	for (i = 0; i < N2DeactivateTrace->protocolIEs.list.count; i++)
	{
		ie = N2DeactivateTrace->protocolIEs.list.array[i];
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

//	d_assert(RanUeNgapId, return,);

	if (AMF_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"AMF_DeActTrace fail to AMF_ConvertNgapId ngap session\n");
		return false;
	}

	GetNsmManager().SendDownLayerSctp(info);

	return true;
}


//lyb,2020-5-11
bool AMF_LocRepCtr(NgapMessage& info)
{
	Ngap_InitiatingMessage_t	*initiatingMessage = NULL;
	Ngap_LocationReportingControl_t	 *N2LocationReportingControl = NULL;
	Ngap_LocationReportingControlIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t     *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t     *AMF_UE_NGAP_ID = NULL;
	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
//	d_assert(initiatingMessage, return,);
	
	N2LocationReportingControl = &initiatingMessage->value.choice.LocationReportingControl;
//	d_assert(N2LocationReportingControl, return,);

//		union Ngap_LocationReportingControlIEs__Ngap_value_u {
//			Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
//			Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
//			Ngap_LocationReportingRequestType_t	 LocationReportingRequestType;
//		} choice;

	for (i = 0; i < N2LocationReportingControl->protocolIEs.list.count; i++)
	{
		ie = N2LocationReportingControl->protocolIEs.list.array[i];
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

//	d_assert(RanUeNgapId, return,);

	if (AMF_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"AMF_LocRepCtr fail to AMF_ConvertNgapId ngap session\n");
		return false;
	}

	GetNsmManager().SendDownLayerSctp(info);

	return true;
}



//lyb,2020-5-11
bool AMF_UETNLABindingRelReq(NgapMessage& info)
{
	Ngap_InitiatingMessage_t	*initiatingMessage = NULL;
	Ngap_UETNLABindingReleaseRequest_t	 *N2UETNLABindingReleaseRequest = NULL;
	Ngap_UETNLABindingReleaseRequestIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t     *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t     *AMF_UE_NGAP_ID = NULL;
	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
//	d_assert(initiatingMessage, return,);
	
	N2UETNLABindingReleaseRequest = &initiatingMessage->value.choice.UETNLABindingReleaseRequest;
//	d_assert(N2UETNLABindingReleaseRequest, return,);

//		union Ngap_UETNLABindingReleaseRequestIEs__Ngap_value_u {
//			Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
//			Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
//		} choice;

	for (i = 0; i < N2UETNLABindingReleaseRequest->protocolIEs.list.count; i++)
		{
			ie = N2UETNLABindingReleaseRequest->protocolIEs.list.array[i];
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

//	d_assert(RanUeNgapId, return,);

	if (AMF_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"AMF_UETNLABindingRelReq fail to AMF_ConvertNgapId ngap session\n");
		return false;
	}

	GetNsmManager().SendDownLayerSctp(info);

	return true;
}



//lyb,2020-5-12
bool AMF_DLUEAsNRPPaTrspt(NgapMessage& info)
{

	Ngap_InitiatingMessage_t	*initiatingMessage = NULL;
	Ngap_DownlinkUEAssociatedNRPPaTransport_t	 *N2DownlinkUEAssociatedNRPPaTransport = NULL;
	Ngap_DownlinkUEAssociatedNRPPaTransportIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t     *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t     *AMF_UE_NGAP_ID = NULL;
	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
//	d_assert(initiatingMessage, return,);
	
	N2DownlinkUEAssociatedNRPPaTransport = &initiatingMessage->value.choice.DownlinkUEAssociatedNRPPaTransport;
//	d_assert(N2DownlinkUEAssociatedNRPPaTransport, return,);

//		union Ngap_DownlinkUEAssociatedNRPPaTransportIEs__Ngap_value_u {
//			Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
//			Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
//			Ngap_RoutingID_t	 RoutingID;
//			Ngap_NRPPa_PDU_t	 NRPPa_PDU;
//		} choice;

	for (i = 0; i < N2DownlinkUEAssociatedNRPPaTransport->protocolIEs.list.count; i++)
		{
			ie = N2DownlinkUEAssociatedNRPPaTransport->protocolIEs.list.array[i];
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

//	d_assert(RanUeNgapId, return,);

	if (AMF_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"AMF_DownlinkUEAsNRPPaTrspt fail to AMF_ConvertNgapId ngap session\n");
		return false;
	}

	GetNsmManager().SendDownLayerSctp(info);

	return true;
}


//lyb,2020-5-12
bool AMF_RerouteNASReq(NgapMessage& info)
{

	Ngap_InitiatingMessage_t	*initiatingMessage = NULL;
	Ngap_RerouteNASRequest_t	 *N2RerouteNASRequest = NULL;
	Ngap_RerouteNASRequest_IEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t     *RanUeNgapId = NULL;
	Ngap_AMF_UE_NGAP_ID_t     *AMF_UE_NGAP_ID = NULL;
	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
//	d_assert(initiatingMessage, return,);
	
	N2RerouteNASRequest = &initiatingMessage->value.choice.RerouteNASRequest;
//	d_assert(N2RerouteNASRequest, return,);

//		union Ngap_RerouteNASRequest_IEs__Ngap_value_u {
//			Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
//			Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
//			OCTET_STRING_t	 OCTET_STRING;
//			Ngap_AMFSetID_t	 AMFSetID;
//			Ngap_AllowedNSSAI_t	 AllowedNSSAI;
//		} choice;

	for (i = 0; i < N2RerouteNASRequest->protocolIEs.list.count; i++)
		{
			ie = N2RerouteNASRequest->protocolIEs.list.array[i];
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

//	d_assert(RanUeNgapId, return,);

	if (AMF_ConvertNgapId(info, RanUeNgapId, AMF_UE_NGAP_ID) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"AMF_RerouteNASReq fail to AMF_ConvertNgapId ngap session\n");
		return false;
	}

	GetNsmManager().SendDownLayerSctp(info);

	return true;
}


//lyb,2020-5-12
bool AMF_OverloadStart(NgapMessage& info)
{

	GetEsmManager().SendToAllRAN(info);

	return true;
}


//lyb,2020-5-12
bool AMF_OverloadStop(NgapMessage& info)
{

	GetEsmManager().SendToAllRAN(info);

	return true;
}


//lyb,2020-5-13
bool AMF_DLNonUEAsNRPPaTrspt(NgapMessage& info)
{

	GetEsmManager().SendToAllRAN(info);

	return true;
}


//lyb,2020-5-13
bool AMF_StatusInd(NgapMessage& info)
{

	Ngap_InitiatingMessage_t	*initiatingMessage = NULL;
	Ngap_AMFStatusIndication_t	 *N2AMFStatusIndication = NULL;
	Ngap_AMFStatusIndicationIEs_t *ie = NULL;
	Ngap_UnavailableGUAMIList_t	 *UnavailableGUAMIList = NULL;
	Ngap_UnavailableGUAMIItem_t *UnavailableGUAMIItem = NULL;
	int i = 0;


	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
//	d_assert(initiatingMessage, return,);
	
	N2AMFStatusIndication = &initiatingMessage->value.choice.AMFStatusIndication;
//	d_assert(N2AMFStatusIndication, return,);

//		union Ngap_AMFStatusIndicationIEs__Ngap_value_u {
//			Ngap_UnavailableGUAMIList_t	 UnavailableGUAMIList;
//		} choice;

	for (i = 0; i < N2AMFStatusIndication->protocolIEs.list.count; i++)
	{
		ie = N2AMFStatusIndication->protocolIEs.list.array[i];
		switch (ie->id)
			{
				case Ngap_ProtocolIE_ID_id_UnavailableGUAMIList:
					UnavailableGUAMIList = &ie->value.choice.UnavailableGUAMIList;
					break;
				
				default:
					break;
			
			}
	
	}

//	d_assert(UnavailableGUAMIList, return,);
	for (i = 0; i < UnavailableGUAMIList->list.count; i++)
	{
		UnavailableGUAMIItem = UnavailableGUAMIList->list.array[i];
		
//		ToDo, search AMF record and modify each AMF status under GW;
//		UnavailableGUAMIItem->gUAMI;
	
	}

	return true;
}


//lyb,2020-5-13
bool AMF_WriteRepWarningReq(NgapMessage& info)
{

	Ngap_InitiatingMessage_t	*initiatingMessage = NULL;
	Ngap_WriteReplaceWarningRequest_t	 *N2WriteReplaceWarningRequest = NULL;
	Ngap_WriteReplaceWarningRequestIEs_t *ie = NULL;
	Ngap_WarningAreaList_t *WarningAreaList = NULL;
	Ngap_NR_CGI_t *NRCgi = NULL;
	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
//	d_assert(initiatingMessage, return,);
	
	N2WriteReplaceWarningRequest = &initiatingMessage->value.choice.WriteReplaceWarningRequest;
//	d_assert(N2WriteReplaceWarningRequest, return,);

//		union Ngap_WriteReplaceWarningRequestIEs__Ngap_value_u {
//			Ngap_MessageIdentifier_t	 MessageIdentifier;
//			Ngap_SerialNumber_t	 SerialNumber;
//			Ngap_WarningAreaList_t	 WarningAreaList;
//			Ngap_RepetitionPeriod_t	 RepetitionPeriod;
//			Ngap_NumberOfBroadcastsRequested_t	 NumberOfBroadcastsRequested;
//			Ngap_WarningType_t	 WarningType;
//			Ngap_WarningSecurityInfo_t	 WarningSecurityInfo;
//			Ngap_DataCodingScheme_t	 DataCodingScheme;
//			Ngap_WarningMessageContents_t	 WarningMessageContents;
//			Ngap_ConcurrentWarningMessageInd_t	 ConcurrentWarningMessageInd;
//			Ngap_WarningAreaCoordinates_t	 WarningAreaCoordinates;
//		} choice;+

	for (i = 0; i < N2WriteReplaceWarningRequest->protocolIEs.list.count; i++)
	{
		ie = N2WriteReplaceWarningRequest->protocolIEs.list.array[i];
		switch (ie->id)
		{
			case Ngap_ProtocolIE_ID_id_WarningAreaList:
				WarningAreaList = &ie->value.choice.WarningAreaList;
				break;
			
			default:
				break;
		
		}

	}


	if (NULL == WarningAreaList) //WarningAreaList IE not exist
	{
		GetEsmManager().SendToAllRAN(info);
	}
	else
	{
		for (i = 0; i < WarningAreaList->choice.nR_CGIListForWarning->list.count; i++)
		{
			NRCgi = WarningAreaList->choice.nR_CGIListForWarning->list.array[i];
		
			//NRCgi->nRCellIdentity    get gNB ID(NCI left 32 bits)
		}
	
	}

	return true;
}



//lyb,2020-5-13
bool AMF_PWSCancelReq(NgapMessage& info)
{

	Ngap_InitiatingMessage_t	*initiatingMessage = NULL;
	Ngap_PWSCancelRequest_t	 *N2PWSCancelRequest = NULL;
	Ngap_PWSCancelRequestIEs_t *ie = NULL;
	Ngap_WarningAreaList_t *WarningAreaList = NULL;
	Ngap_NR_CGI_t *NRCgi = NULL;
	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
//	d_assert(initiatingMessage, return,);
	
	N2PWSCancelRequest = &initiatingMessage->value.choice.PWSCancelRequest;
//	d_assert(N2PWSCancelRequest, return,);

//		union Ngap_PWSCancelRequestIEs__Ngap_value_u {
//			Ngap_MessageIdentifier_t	 MessageIdentifier;
//			Ngap_SerialNumber_t	 SerialNumber;
//			Ngap_WarningAreaList_t	 WarningAreaList;
//			Ngap_CancelAllWarningMessages_t	 CancelAllWarningMessages;
//		} choice;


	for (i = 0; i < N2PWSCancelRequest->protocolIEs.list.count; i++)
	{
		ie = N2PWSCancelRequest->protocolIEs.list.array[i];
		switch (ie->id)
		{
			case Ngap_ProtocolIE_ID_id_WarningAreaList:
				WarningAreaList = &ie->value.choice.WarningAreaList;
				break;
			
			default:
				break;
		
		}
	
	}

	
	if (NULL == WarningAreaList) //WarningAreaList IE not exist
	{
		GetEsmManager().SendToAllRAN(info);
	}
	else
	{
		
		for (i = 0; i < WarningAreaList->choice.nR_CGIListForWarning->list.count; i++)
		{
			NRCgi = WarningAreaList->choice.nR_CGIListForWarning->list.array[i];

			//NRCgi->nRCellIdentity    get gNB ID(NCI left 32 bits)
		}

	}
	
	return true;
}



bool AMF_CfgUpdate(NgapMessage& info)
{
	Ngap_InitiatingMessage_t	*initiatingMessage = NULL;
	Ngap_AMFConfigurationUpdate_t	 *AMFConfigurationUpdate = NULL;
	Ngap_AMFConfigurationUpdateIEs_t *ie = NULL;
	Ngap_AMFName_t	 *AMFName = NULL;
	Ngap_ServedGUAMIList_t	 *ServedGUAMIList = NULL;
	Ngap_RelativeAMFCapacity_t	 *RelativeAMFCapacity = NULL;
	Ngap_PLMNSupportList_t	 *PLMNSupportList = NULL;
	Ngap_AMF_TNLAssociationToAddList_t	 	 *AMF_TNLAssociationToAddList = NULL;
	Ngap_AMF_TNLAssociationToRemoveList_t	 *AMF_TNLAssociationToRemoveList = NULL;
	Ngap_AMF_TNLAssociationToUpdateList_t	 *AMF_TNLAssociationToUpdateList = NULL;

	int i = 0;

	initiatingMessage = info.ngapMessage.choice.initiatingMessage;
//	d_assert(initiatingMessage, return,);
	
	AMFConfigurationUpdate = &initiatingMessage->value.choice.AMFConfigurationUpdate;
//	d_assert(N2PWSCancelRequest, return,);

//union Ngap_AMFConfigurationUpdateIEs__Ngap_value_u {
//	Ngap_AMFName_t	 AMFName;
//	Ngap_ServedGUAMIList_t	 ServedGUAMIList;
//	Ngap_RelativeAMFCapacity_t	 RelativeAMFCapacity;
//	Ngap_PLMNSupportList_t	 PLMNSupportList;
//	Ngap_AMF_TNLAssociationToAddList_t	 AMF_TNLAssociationToAddList;
//	Ngap_AMF_TNLAssociationToRemoveList_t	 AMF_TNLAssociationToRemoveList;
//	Ngap_AMF_TNLAssociationToUpdateList_t	 AMF_TNLAssociationToUpdateList;
//} choice;


	for (i = 0; i < AMFConfigurationUpdate->protocolIEs.list.count; i++)
	{
		ie = AMFConfigurationUpdate->protocolIEs.list.array[i];
		switch (ie->id)
		{
			case Ngap_ProtocolIE_ID_id_AMFName:
				AMFName = &ie->value.choice.AMFName;
				break;
			case Ngap_ProtocolIE_ID_id_ServedGUAMIList:
				ServedGUAMIList = &ie->value.choice.ServedGUAMIList;
				break;
			case Ngap_ProtocolIE_ID_id_RelativeAMFCapacity:
				RelativeAMFCapacity = &ie->value.choice.RelativeAMFCapacity;
				break;
			case Ngap_ProtocolIE_ID_id_PLMNSupportList:
				PLMNSupportList = &ie->value.choice.PLMNSupportList;
				break;
			case Ngap_ProtocolIE_ID_id_AMF_TNLAssociationToAddList:
				AMF_TNLAssociationToAddList = &ie->value.choice.AMF_TNLAssociationToAddList;
				break;
			case Ngap_ProtocolIE_ID_id_AMF_TNLAssociationToRemoveList:
				AMF_TNLAssociationToRemoveList = &ie->value.choice.AMF_TNLAssociationToRemoveList;
				break;
			case Ngap_ProtocolIE_ID_id_AMF_TNLAssociationToUpdateList:
				AMF_TNLAssociationToUpdateList = &ie->value.choice.AMF_TNLAssociationToUpdateList;
				break;
			
			default:
				break;
		
		}
	
	}

	//TBD.
	//Update infomation on local database.

	//Send Rsp - AMF CONFIGURATION UPDATE ACKNOWLEDGE.
	GetAsmManager().SendCfgUpdAck(info);
	
	return true;
}



bool APM_Init()
{
    APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, 	Ngap_ProcedureCode_id_PDUSessionResourceSetup, 		&AMF_PDUSessionResourceSetupReq);
    APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, 	Ngap_ProcedureCode_id_PDUSessionResourceModify, 	&AMF_PDUSessionResourceModifyReq);
    APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_successfulOutcome,	Ngap_ProcedureCode_id_PDUSessionResourceModifyIndication,&AMF_PDUSessionResourceModifyConfirm);
    APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, 	Ngap_ProcedureCode_id_PDUSessionResourceRelease, 	&AMF_PDUSessionResourceReleaseCmd);
    APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, 	Ngap_ProcedureCode_id_InitialContextSetup, 			&AMF_InitialContextSetupReq);
    APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, 	Ngap_ProcedureCode_id_UEContextRelease, 			&AMF_UEContextReleaseCommand);
    APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, 	Ngap_ProcedureCode_id_UEContextModification, 		&AMF_UEContextModificationReq);
	APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, 	Ngap_ProcedureCode_id_UERadioCapabilityCheck, 		&AMF_UERadioCapabilityCheckReq);
    APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_successfulOutcome, 	Ngap_ProcedureCode_id_HandoverPreparation, 			&AMF_HandoverCommand);
    APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_unsuccessfulOutcome, 	Ngap_ProcedureCode_id_HandoverPreparation, 			&AMF_HandoverPreparationFailure);
    APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, 	Ngap_ProcedureCode_id_HandoverResourceAllocation, 	&AMF_HandoverReq);
    APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_successfulOutcome, 	Ngap_ProcedureCode_id_PathSwitchRequest, 			&AMF_PathSwitchReqAck);
    APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_unsuccessfulOutcome, 	Ngap_ProcedureCode_id_PathSwitchRequest, 			&AMF_PathSwitchReqFailure);
    APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_successfulOutcome, 	Ngap_ProcedureCode_id_HandoverCancel, 				&AMF_HandoverCancelAck);
    APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, 	Ngap_ProcedureCode_id_DownlinkRANStatusTransfer,	&AMF_DLRanStatusTsfer);	
    APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, 	Ngap_ProcedureCode_id_AMFStatusIndication, 			&AMF_StatusInd);	
	APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, 	Ngap_ProcedureCode_id_LocationReportingControl,		&AMF_LocRepCtr);
    APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, 	Ngap_ProcedureCode_id_Paging, 						&AMF_Paging);
    APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, 	Ngap_ProcedureCode_id_DownlinkNASTransport, 		&AMF_DownlinkNASTransport);
	APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage,    Ngap_ProcedureCode_id_DownlinkUEAssociatedNRPPaTransport,&AMF_DLUEAsNRPPaTrspt);
	APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage,    Ngap_ProcedureCode_id_DownlinkNonUEAssociatedNRPPaTransport,&AMF_DLNonUEAsNRPPaTrspt);
    APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, 	Ngap_ProcedureCode_id_NGReset, 						&AMF_Reset);
    APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_successfulOutcome, 	Ngap_ProcedureCode_id_NGReset, 						&AMF_ResetAck);
	APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, 	Ngap_ProcedureCode_id_RerouteNASRequest,			&AMF_RerouteNASReq);
	APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, 	Ngap_ProcedureCode_id_TraceStart, 					&AMF_TraceStart);
	APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, 	Ngap_ProcedureCode_id_DeactivateTrace, 				&AMF_DeActTrace);
	APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, 	Ngap_ProcedureCode_id_UETNLABindingRelease,			&AMF_UETNLABindingRelReq);	
    APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, 	Ngap_ProcedureCode_id_ErrorIndication, 				&AMF_ErrorIndication);
    APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_successfulOutcome, 	Ngap_ProcedureCode_id_NGSetup, 						&AMF_NgSetupResponse);
    APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, 	Ngap_ProcedureCode_id_OverloadStart,				&AMF_OverloadStart);
    APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage, 	Ngap_ProcedureCode_id_OverloadStop,					&AMF_OverloadStop);
	APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage,	Ngap_ProcedureCode_id_WriteReplaceWarning, 			&AMF_WriteRepWarningReq);
	APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage,	Ngap_ProcedureCode_id_PWSCancel,	 				&AMF_PWSCancelReq);	
	APM_REGISTER_APPCALLBACK(Ngap_NGAP_PDU_PR_initiatingMessage,	Ngap_ProcedureCode_id_AMFConfigurationUpdate,		&AMF_CfgUpdate);	

	return true;
}

