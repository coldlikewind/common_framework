
#include "EsmManager.h"
#include "NsmManager.h"
#include "RanNodeId.h"
#include "NgapMessage.h"
#include "PlmnUtil.h"
#include "DbManager.h"
#include "PduSessManager.h"

using namespace std;

EsmManager::EsmManager() 
{
	agc_memory_pool_t *module_pool = agcsig_get_sig_memory_pool();
	agc_mutex_init(&m_mutex, AGC_MUTEX_NESTED, module_pool);
}

EsmManager::~EsmManager()
{

}

bool EsmManager::Init()
{
	return true;
}


void EsmManager::Exit()
{
	memset(esm_list, 0 , MAX_ESM_SESSIONS * sizeof(EsmContext *));
}


bool EsmManager::GetEsmContext(uint32_t sctp_index, EsmContext **esm)
{	
	EsmContext *context = NULL;

	if (sctp_index >= MAX_ESM_SESSIONS)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "EsmManager::GetEsmContext failed sctp_index=%d\n", sctp_index);
		return false;
	}
	agc_mutex_lock(m_mutex);
	context = esm_list[sctp_index];
	agc_mutex_unlock(m_mutex);
	if (context == NULL)
	{
		//agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "EsmManager::GetEsmContext failed sctp_index=%d context is null.\n", sctp_index);
		return false;
	}

	*esm = context;
	
	return true;
}

bool EsmManager::GetEsmContext(RanNodeId& ranNodeId, EsmContext **esm)
{	
	for (int i = 0; i < MAX_ESM_SESSIONS; i++)
	{
		agc_mutex_lock(m_mutex);
		EsmContext *context = esm_list[i];
		agc_mutex_unlock(m_mutex);
		if (context != NULL && context->ranNodeId == ranNodeId)
		{
			*esm = context;
			return true;
		}
	
	}
	return false;
}

bool EsmManager::GetEsmContextByRanNodeId(RanNodeId& ranNodeId, EsmContext **esm)
{
	for (int i = 0; i < MAX_ESM_SESSIONS; i++)
	{
		agc_mutex_lock(m_mutex);
		EsmContext *context = esm_list[i];
		agc_mutex_unlock(m_mutex);
		if (context != NULL
				&& context->ranNodeId.getRanNodeID()
						== context->ranNodeId.getRanNodeID()
				&& ranNodeId.getRanNgEnbType()
						== context->ranNodeId.getRanNgEnbType()) {
			*esm = context;
			return true;
		}

	}
	return false;
}


bool EsmManager::FindRanNodeId(uint32_t sctp_index, RanNodeId& ranNodeId)
{
	EsmContext *context = NULL;
	if (sctp_index >= MAX_ESM_SESSIONS)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "EsmManager::FindRanNodeId failed sctp_index=%d\n", sctp_index);
		return false;
	}
	agc_mutex_lock(m_mutex);
	context = esm_list[sctp_index];
	agc_mutex_unlock(m_mutex);
	if (context == NULL)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "EsmManager::FindRanNodeId failed sctp_index=%d context is null.\n", sctp_index);
		return false;
	}

	ranNodeId = context->ranNodeId;
	return true;
}


bool EsmManager::Setup(const NgapMessage& info, EsmContext *newContext)
{
	uint32_t ranbMapSize = 0;
    uint32_t ranbIndexMapSize = 0;
	uint32_t sctp_index = info.sockSource;
	//agc_mutex_lock(m_mutex);

	EsmContext *context = NULL;
	if (sctp_index >= MAX_ESM_SESSIONS)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "EsmManager::Setup failed sctp_index=%d\n", sctp_index);
		return false;
	}

	agc_mutex_lock(m_mutex);
	context = esm_list[sctp_index];
	if (context != NULL)
	{
		delete context;
	}

	esm_list[sctp_index] = newContext;
	agc_mutex_unlock(m_mutex);

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "EsmManager::Setup success sctp_index=%d,ranNodeId:%s, is setup\n",
		info.sockSource,
		info.ranNodeId.toString().c_str());

	//agc_mutex_unlock(m_mutex);
	return true;
}

bool EsmManager::Update(const NgapMessage& info, EsmContext *newEsm)
{
	EsmContext *context = NULL;
	uint32_t sctp_index = info.sockSource;

	if (sctp_index >= MAX_ESM_SESSIONS)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "EsmManager::Update failed sctp_index=%d\n", sctp_index);
		return false;
	}

	context = esm_list[sctp_index];
	if (context == NULL)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "EsmManager::Update failed sctp_index=%d context is null.\n", sctp_index);
		return false;
	}
	
	if (info.ranNodeId != newEsm->ranNodeId)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "EsmManager::Update fail to update context ranNodeId:%s, new ranid=%s\n",
			info.ranNodeId.toString().c_str(),
			newEsm->ranNodeId.toString().c_str());
		return false;
	}	
	
	esm_list[sctp_index] = newEsm;
	
	return true;
}

bool EsmManager::SendResetAck(NgapMessage& info)
{
    Ngap_SuccessfulOutcome_t	*successfulOutcome = NULL;
	Ngap_NGResetAcknowledge_t	*NGResetAcknowledge = NULL;

	NgapMessage infognb;
	infognb.amfUeId = 0;
	infognb.gnbUeId = 0;
	infognb.amfId = info.amfId;
	infognb.sockSource = info.sockSource;
	infognb.sockTarget = info.sockSource;
	infognb.idSigGW = info.idSigGW;
	
	info.PDUChoice = Ngap_NGAP_PDU_PR_successfulOutcome;
	info.ProcCode = Ngap_ProcedureCode_id_NGReset;
	
    memset(&info.ngapMessage, 0, sizeof (Ngap_NGAP_PDU_t));
    info.ngapMessage.present = Ngap_NGAP_PDU_PR_successfulOutcome;
    info.ngapMessage.choice.successfulOutcome = (Ngap_SuccessfulOutcome_t *)calloc(1, sizeof(Ngap_SuccessfulOutcome_t));

    successfulOutcome = info.ngapMessage.choice.successfulOutcome;
    successfulOutcome->procedureCode = Ngap_ProcedureCode_id_NGReset;
    successfulOutcome->criticality = Ngap_Criticality_reject;
    successfulOutcome->value.present = Ngap_SuccessfulOutcome__value_PR_NGResetAcknowledge;
		
	GetNsmManager().SendUpLayerSctp(infognb);

	return true;
}

bool EsmManager::SendNgSetupResponse(NgapMessage& info)
{
    NgapMessage ngapInfo;
	Ngap_SuccessfulOutcome_t *successfulOutcome = NULL;
	Ngap_NGSetupResponseIEs_t *ie = NULL;
	
	stCfg_vGWParam_API param;
	std::vector<stCfg_GUAMI_API> guamis;
	std::vector<stCfg_BPlmn_API> plmns;
	uint8_t idSigGW = info.idSigGW;
	if (GetDbManager().QueryVgwParam(idSigGW, param) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "EsmManager::SendNgSetupResponse fail to get param gatewayid=%d\n",
			idSigGW);
		return false;
	}
	if (GetDbManager().QueryBplmn(idSigGW, plmns) == false || plmns.size() <= 0)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "EsmManager::SendNgSetupResponse fail to get plmn gatewayid=%d\n",
			idSigGW);
		return false;
	}
	if (GetDbManager().QueryGuami(idSigGW, guamis) == false || guamis.size() <= 0)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "EsmManager::SendNgSetupResponse fail to get guami gatewayid=%d\n",
			idSigGW);
		return false;
	}

    memcpy(&ngapInfo.sctp_stream, &info.sctp_stream, sizeof(info.sctp_stream));
    ngapInfo.gnbUeId = INVALIDE_ID;
    ngapInfo.amfUeId = INVALIDE_ID_X64;
    ngapInfo.sessId = INVALIDE_ID;
    ngapInfo.sctpIndex = info.sctpIndex;
	ngapInfo.ranNodeId = info.ranNodeId;
    ngapInfo.sockSource = info.sockSource;
    ngapInfo.sockTarget = info.sockSource;
    ngapInfo.stream_no = 0;
	
	memset(&ngapInfo.ngapMessage, 0, sizeof (Ngap_NGAP_PDU_t));	
    ngapInfo.PDUChoice = Ngap_NGAP_PDU_PR_successfulOutcome;
    ngapInfo.ProcCode = Ngap_ProcedureCode_id_NGSetup;
	
    ngapInfo.ngapMessage.present = Ngap_NGAP_PDU_PR_successfulOutcome;
    ngapInfo.ngapMessage.choice.successfulOutcome = (Ngap_SuccessfulOutcome_t *)calloc(1, sizeof(Ngap_SuccessfulOutcome_t));

    successfulOutcome = ngapInfo.ngapMessage.choice.successfulOutcome;
    successfulOutcome->procedureCode = Ngap_ProcedureCode_id_NGSetup;
    successfulOutcome->criticality = Ngap_Criticality_reject;
    successfulOutcome->value.present = Ngap_SuccessfulOutcome__value_PR_NGSetupResponse;

	Ngap_NGSetupResponse_t *NGSetupResponse = &successfulOutcome->value.choice.NGSetupResponse;

	if (strlen(param.AMFName) > 0)
	{
		Ngap_AMFName_t	 *AMFName = NULL;
		ie = (Ngap_NGSetupResponseIEs_t *)calloc(1, sizeof(Ngap_NGSetupResponseIEs_t));
		ASN_SEQUENCE_ADD(&NGSetupResponse->protocolIEs, ie);
		
		ie->id = Ngap_ProtocolIE_ID_id_AMFName;
		ie->criticality = Ngap_Criticality_reject;
		ie->value.present = Ngap_NGSetupResponseIEs__value_PR_AMFName;
		
	    AMFName = &ie->value.choice.AMFName;
		AMFName->size = strlen(param.AMFName);
		AMFName->buf = (uint8_t *)calloc(AMFName->size, sizeof(uint8_t));
		memcpy(AMFName->buf, param.AMFName, strlen(param.AMFName));
	}
	
	ie = (Ngap_NGSetupResponseIEs_t *)calloc(1, sizeof(Ngap_NGSetupResponseIEs_t));
	ASN_SEQUENCE_ADD(&NGSetupResponse->protocolIEs, ie);
	
	ie->id = Ngap_ProtocolIE_ID_id_ServedGUAMIList;
	ie->criticality = Ngap_Criticality_reject;
	ie->value.present = Ngap_NGSetupResponseIEs__value_PR_ServedGUAMIList;
	
	Ngap_ServedGUAMIList_t *ServedGUAMIList = &ie->value.choice.ServedGUAMIList;
	for (uint32_t i = 0; i < guamis.size(); i++)
	{
    	plmn_id_t plmn_id;
		Ngap_ServedGUAMIItem_t *guamiItem = (Ngap_ServedGUAMIItem_t *)calloc(1, sizeof(Ngap_ServedGUAMIItem_t));
		
		//plmn_id_build(&plmn_id, guamis[i].MCC,  guamis[i].MNC, PLMN_MNC_ID_LEN);
		plmn_id.mcc1 = guamis[i].plmn.mcc1;
		plmn_id.mcc2 = guamis[i].plmn.mcc2;
		plmn_id.mcc3 = guamis[i].plmn.mcc3;
		plmn_id.mnc1 = guamis[i].plmn.mnc1;
		plmn_id.mnc2 = guamis[i].plmn.mnc2;
		plmn_id.mnc3 = guamis[i].plmn.mnc3;
		
	    guamiItem->gUAMI.pLMNIdentity.size = PLMN_ID_LEN;
	    guamiItem->gUAMI.pLMNIdentity.buf = (uint8_t *)calloc(guamiItem->gUAMI.pLMNIdentity.size , sizeof(uint8_t));		
    	memcpy(guamiItem->gUAMI.pLMNIdentity.buf, &plmn_id, PLMN_ID_LEN);
		
		guamiItem->gUAMI.aMFRegionID.size = 1;
		guamiItem->gUAMI.aMFRegionID.buf = (uint8_t *)calloc(guamiItem->gUAMI.aMFRegionID.size, sizeof(uint8_t));
		guamiItem->gUAMI.aMFRegionID.buf[0] = guamis[i].AMFRegionID;
		
		guamiItem->gUAMI.aMFSetID.size = 2;
		guamiItem->gUAMI.aMFSetID.buf = (uint8_t *)calloc(guamiItem->gUAMI.aMFSetID.size, sizeof(uint8_t));
		guamiItem->gUAMI.aMFSetID.buf[0] = guamis[i].AMFSetId >> 2;
		guamiItem->gUAMI.aMFSetID.buf[1] = (guamis[i].AMFSetId & 0x03) << 6;
		guamiItem->gUAMI.aMFSetID.bits_unused = 6;
		
		guamiItem->gUAMI.aMFPointer.size = 1;
		guamiItem->gUAMI.aMFPointer.buf = (uint8_t *)calloc(guamiItem->gUAMI.aMFPointer.size, sizeof(uint8_t));
		guamiItem->gUAMI.aMFPointer.buf[0] = guamis[i].AMFPointer << 2;
		guamiItem->gUAMI.aMFPointer.bits_unused = 2;
		
		ASN_SEQUENCE_ADD(&ServedGUAMIList->list, guamiItem);
	}	
	
	ie = (Ngap_NGSetupResponseIEs_t *)calloc(1, sizeof(Ngap_NGSetupResponseIEs_t));
	ASN_SEQUENCE_ADD(&NGSetupResponse->protocolIEs, ie);
	
	ie->id = Ngap_ProtocolIE_ID_id_RelativeAMFCapacity;
	ie->criticality = Ngap_Criticality_ignore;
	ie->value.present = Ngap_NGSetupResponseIEs__value_PR_RelativeAMFCapacity;
	ie->value.choice.RelativeAMFCapacity = param.AMFCapacity;
	
	ie = (Ngap_NGSetupResponseIEs_t *)calloc(1, sizeof(Ngap_NGSetupResponseIEs_t));
	ASN_SEQUENCE_ADD(&NGSetupResponse->protocolIEs, ie);
	
	ie->id = Ngap_ProtocolIE_ID_id_PLMNSupportList;
	ie->criticality = Ngap_Criticality_reject;
	ie->value.present = Ngap_NGSetupResponseIEs__value_PR_PLMNSupportList;
	
	Ngap_PLMNSupportList_t *PLMNSupportList = &ie->value.choice.PLMNSupportList;
	for (uint32_t i = 0; i < plmns.size(); i++)
	{
		std::vector<stCfg_PLMN_NSSAI_DATA> nnsais;
		if (GetDbManager().QueryPlmnNssai(idSigGW, plmns[i].idBplmn, nnsais) == false || nnsais.size() <= 0) {
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "EsmManager::SendNgSetupResponse fail to get PlmnNssai idBplmn=%d\n", plmns[i].idBplmn);
			return false;
		}
		
    	plmn_id_t plmn_id;
		Ngap_PLMNSupportItem_t *plmnItem = (Ngap_PLMNSupportItem_t *)calloc(1, sizeof(Ngap_PLMNSupportItem_t));
		
		//plmn_id_build(&plmn_id, plmns[i].MCC,  plmns[i].MNC, PLMN_MNC_ID_LEN);
		plmn_id.mcc1 = plmns[i].plmn.mcc1;
		plmn_id.mcc2 = plmns[i].plmn.mcc2;
		plmn_id.mcc3 = plmns[i].plmn.mcc3;
		plmn_id.mnc1 = plmns[i].plmn.mnc1;
		plmn_id.mnc2 = plmns[i].plmn.mnc2;
		plmn_id.mnc3 = plmns[i].plmn.mnc3;
    
	    plmnItem->pLMNIdentity.size = PLMN_ID_LEN;
	    plmnItem->pLMNIdentity.buf = (uint8_t *)calloc(plmnItem->pLMNIdentity.size, sizeof(uint8_t));
    	memcpy(plmnItem->pLMNIdentity.buf, &plmn_id, PLMN_ID_LEN);
		
		for (uint32_t j= 0; j < nnsais.size(); j++)
		{
			Ngap_SliceSupportItem_t *ssai = (Ngap_SliceSupportItem_t *)calloc(1, sizeof(Ngap_SliceSupportItem_t));
			ssai->s_NSSAI.sST.size = 1;
			ssai->s_NSSAI.sST.buf = (uint8_t *)calloc(ssai->s_NSSAI.sST.size, sizeof(uint8_t));
			ssai->s_NSSAI.sST.buf[0] = nnsais[j].SST;
			/*
			ssai->s_NSSAI.sD = (Ngap_SD_t *)calloc(1, sizeof(Ngap_SD_t));
			ssai->s_NSSAI.sD->size = 3;
			ssai->s_NSSAI.sD->buf = (uint8_t *)calloc(3, sizeof(uint8_t));
			ssai->s_NSSAI.sD->buf[0] = nnsais[j].SD >> 16;
			ssai->s_NSSAI.sD->buf[1] = nnsais[j].SD >> 8;
			ssai->s_NSSAI.sD->buf[2] = nnsais[j].SD;
			*/
			ASN_SEQUENCE_ADD(&plmnItem->sliceSupportList.list, ssai);
		}
		
		ASN_SEQUENCE_ADD(&PLMNSupportList->list, plmnItem);
	}	
	
	GetNsmManager().SendDownLayerSctp(ngapInfo);
	return true;
}

bool EsmManager::SendNgSetupFailure(uint32_t cause, NgapMessage& info)
{
    NgapMessage ngapInfo;
	
    memcpy(&ngapInfo.sctp_stream, &info.sctp_stream, sizeof(info.sctp_stream));
    ngapInfo.gnbUeId = INVALIDE_ID;
    ngapInfo.amfUeId = INVALIDE_ID_X64;
    ngapInfo.sessId = INVALIDE_ID;
    ngapInfo.sctpIndex = info.sctpIndex;
    ngapInfo.sockSource = info.sockSource;
    ngapInfo.sockTarget = info.sockSource;
    ngapInfo.stream_no = 0;
	
	memset(&ngapInfo.ngapMessage, 0, sizeof (Ngap_NGAP_PDU_t));
	Ngap_UnsuccessfulOutcome_t *unsuccessfulOutcome = NULL;
	Ngap_NGSetupFailureIEs_t *ie = NULL;
	
    ngapInfo.PDUChoice = Ngap_NGAP_PDU_PR_unsuccessfulOutcome;
    ngapInfo.ProcCode = Ngap_ProcedureCode_id_NGSetup;		
    ngapInfo.ngapMessage.present = Ngap_NGAP_PDU_PR_unsuccessfulOutcome;
    ngapInfo.ngapMessage.choice.unsuccessfulOutcome = (Ngap_UnsuccessfulOutcome_t *)calloc(1, sizeof(Ngap_UnsuccessfulOutcome_t));
	
    unsuccessfulOutcome = ngapInfo.ngapMessage.choice.unsuccessfulOutcome;
    unsuccessfulOutcome->procedureCode = Ngap_ProcedureCode_id_NGSetup;
    unsuccessfulOutcome->criticality = Ngap_Criticality_reject;
    unsuccessfulOutcome->value.present = Ngap_UnsuccessfulOutcome__value_PR_NGSetupFailure;
		
	Ngap_NGSetupFailure_t *NGSetupFailure = &unsuccessfulOutcome->value.choice.NGSetupFailure;
	
	ie = (Ngap_NGSetupFailureIEs_t *)calloc(1, sizeof(Ngap_NGSetupFailureIEs_t));
	ASN_SEQUENCE_ADD(&NGSetupFailure->protocolIEs, ie);
	
	ie->id = Ngap_ProtocolIE_ID_id_Cause;
	ie->criticality = Ngap_Criticality_ignore;
	ie->value.present = Ngap_NGSetupFailureIEs__value_PR_Cause;
	
    if (NGAP_CHECK_RNI_PLMN_ERROR == cause)
    {
    	ie->value.choice.Cause.present = Ngap_Cause_PR_misc;
    	ie->value.choice.Cause.choice.misc = Ngap_CauseMisc_unknown_PLMN;
    }
    else 
    {
    	ie->value.choice.Cause.present = Ngap_Cause_PR_radioNetwork;
    	ie->value.choice.Cause.choice.radioNetwork = Ngap_CauseRadioNetwork_unspecified;
    }
	
	ie = (Ngap_NGSetupFailureIEs_t *)calloc(1, sizeof(Ngap_NGSetupFailureIEs_t));
	ASN_SEQUENCE_ADD(&NGSetupFailure->protocolIEs, ie);
	
	ie->id = Ngap_ProtocolIE_ID_id_TimeToWait;
	ie->criticality = Ngap_Criticality_ignore;
	ie->value.present = Ngap_NGSetupFailureIEs__value_PR_TimeToWait;
	ie->value.choice.TimeToWait = Ngap_TimeToWait_v10s;
    
	GetNsmManager().SendDownLayerSctp(ngapInfo);
	return true;
}



//lyb,2020-8-6
bool EsmManager::SendHandoverRequest(NgapMessage& info,  Ngap_SourceToTarget_TransparentContainer_t *SourceToTarget_TransparentContainer)
{
	if(SourceToTarget_TransparentContainer == NULL)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "EsmManager::SendHandoverRequest input pointer is null.\n");
		return false;
	}

	NgapSession *sess = NULL;
	if (GetNgapSessManager().GetNgapSession(info.sessId, &sess) == false)
	{
    	agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "EsmManager::SendHandoverRequest  fail to get ngap session, sessId=%d\n", info.sessId);
		return false;
	}
	
    NgapMessage ngapInfo;
	Ngap_InitiatingMessage_t	*initiatingMessage = NULL;
	Ngap_HandoverRequestIEs_t   *ie = NULL;
	
//    Cfg_gNBInterParam gnbInterParam(LTEConfig::Instance().GetgNBCfg());

//	ngapInfo used for msg sent to RAN/Target.
    ngapInfo.amfUeId = (uint64_t)info.sessId;
    ngapInfo.sessId = info.sessId;
    ngapInfo.gnbUeId = info.gnbUeId;
	ngapInfo.amfId = info.amfId;
	ngapInfo.ranNodeId = info.ranNodeId;
	ngapInfo.sockTarget = info.sockTarget;
	
    ngapInfo.PDUChoice = Ngap_NGAP_PDU_PR_initiatingMessage;
    ngapInfo.ProcCode = Ngap_ProcedureCode_id_HandoverResourceAllocation;
	
	memset(&ngapInfo.ngapMessage, 0, sizeof (Ngap_NGAP_PDU_t));
	
    ngapInfo.ngapMessage.present = Ngap_NGAP_PDU_PR_initiatingMessage;
    ngapInfo.ngapMessage.choice.initiatingMessage = (Ngap_InitiatingMessage_t *)calloc(1, sizeof(Ngap_InitiatingMessage_t));
    initiatingMessage = ngapInfo.ngapMessage.choice.initiatingMessage;
    initiatingMessage->procedureCode = Ngap_ProcedureCode_id_HandoverResourceAllocation;
    initiatingMessage->criticality = Ngap_Criticality_reject;
    initiatingMessage->value.present = Ngap_InitiatingMessage__value_PR_HandoverRequest;

	Ngap_HandoverRequest_t	*HandoverRequest = &initiatingMessage->value.choice.HandoverRequest;

/*
typedef struct Ngap_HandoverRequestIEs {
	Ngap_ProtocolIE_ID_t	 id;
	Ngap_Criticality_t	 criticality;
	struct Ngap_HandoverRequestIEs__value {
		Ngap_HandoverRequestIEs__value_PR present;
		union Ngap_HandoverRequestIEs__Ngap_value_u {
1			Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
2			Ngap_HandoverType_t	 HandoverType;
3			Ngap_Cause_t	 Cause;
4			Ngap_UEAggregateMaximumBitRate_t	 UEAggregateMaximumBitRate;
				//Ngap_CoreNetworkAssistanceInformation_t	 CoreNetworkAssistanceInformation;
5			Ngap_UESecurityCapabilities_t	 UESecurityCapabilities;
6			Ngap_SecurityContext_t	 SecurityContext;
				//Ngap_NewSecurityContextInd_t	 NewSecurityContextInd;
				//Ngap_NAS_PDU_t	 NAS_PDU;
7			Ngap_PDUSessionResourceSetupListHOReq_t	 PDUSessionResourceSetupListHOReq;
8			Ngap_AllowedNSSAI_t	 AllowedNSSAI;
				//Ngap_TraceActivation_t	 TraceActivation;
				//Ngap_MaskedIMEISV_t	 MaskedIMEISV;
9			Ngap_SourceToTarget_TransparentContainer_t	 SourceToTarget_TransparentContainer;
				//Ngap_MobilityRestrictionList_t	 MobilityRestrictionList;
				//Ngap_LocationReportingRequestType_t	 LocationReportingRequestType;
				//Ngap_RRCInactiveTransitionReportRequest_t	 RRCInactiveTransitionReportRequest;
10			Ngap_GUAMI_t	 GUAMI;
				//Ngap_RedirectionVoiceFallback_t	 RedirectionVoiceFallback;
		} choice;
	} value;
} Ngap_HandoverRequestIEs_t;
*/
	
//Start Make IE 

//AMF_UE_NGAP_ID	1
	ie = (Ngap_HandoverRequestIEs_t *)calloc(1, sizeof(Ngap_HandoverRequestIEs_t));
	ASN_SEQUENCE_ADD(&HandoverRequest->protocolIEs, ie);
	
	ie->id = Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
	ie->criticality = Ngap_Criticality_reject;
	ie->value.present = Ngap_HandoverRequestIEs__value_PR_AMF_UE_NGAP_ID;
	asn_int642INTEGER(&ie->value.choice.AMF_UE_NGAP_ID, ngapInfo.amfUeId);
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,
			"EsmManager::SendHandoverRequest IE id=%d, AMF_UE_NGAP_ID=%d\n", ie->id, ngapInfo.amfUeId);


//HandoverType	2
	ie = (Ngap_HandoverRequestIEs_t *)calloc(1, sizeof(Ngap_HandoverRequestIEs_t));
	ASN_SEQUENCE_ADD(&HandoverRequest->protocolIEs, ie);
	
	ie->id = Ngap_ProtocolIE_ID_id_HandoverType;
	ie->criticality = Ngap_Criticality_reject;
	ie->value.present = Ngap_HandoverRequestIEs__value_PR_HandoverType;
	ie->value.choice.HandoverType = Ngap_HandoverType_intra5gs;
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,
					"EsmManager::SendHandoverRequest IE id=%d, HandoverType=%ld\n", ie->id, ie->value.choice.HandoverType);

	
//Cause	3
	ie = (Ngap_HandoverRequestIEs_t *)calloc(1, sizeof(Ngap_HandoverRequestIEs_t));
	ASN_SEQUENCE_ADD(&HandoverRequest->protocolIEs, ie);
	
	ie->id = Ngap_ProtocolIE_ID_id_Cause;
	ie->criticality = Ngap_Criticality_ignore;
	ie->value.present = Ngap_HandoverRequestIEs__value_PR_Cause;
	ie->value.choice.Cause.present = Ngap_Cause_PR_radioNetwork;
	ie->value.choice.Cause.choice.radioNetwork = Ngap_CauseRadioNetwork_handover_desirable_for_radio_reason;
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,
						"EsmManager::SendHandoverRequest IE id=%d, radioNetwork=%ld\n", ie->id,
						Ngap_CauseRadioNetwork_handover_desirable_for_radio_reason);

//UEAggregateMaximumBitRate	4
	ie = (Ngap_HandoverRequestIEs_t *)calloc(1, sizeof(Ngap_HandoverRequestIEs_t));
	ASN_SEQUENCE_ADD(&HandoverRequest->protocolIEs, ie);

	ie->id = Ngap_ProtocolIE_ID_id_UEAggregateMaximumBitRate;
	ie->criticality = Ngap_Criticality_reject;
	ie->value.present = Ngap_HandoverRequestIEs__value_PR_UEAggregateMaximumBitRate;	
	asn_int642INTEGER(&ie->value.choice.UEAggregateMaximumBitRate.uEAggregateMaximumBitRateDL, (uint64_t)sess->maxBitrateDl);
	asn_int642INTEGER(&ie->value.choice.UEAggregateMaximumBitRate.uEAggregateMaximumBitRateUL, (uint64_t)sess->maxBitrateUl);
	

//UESecurityCapabilities	5
	ie = (Ngap_HandoverRequestIEs_t *)calloc(1, sizeof(Ngap_HandoverRequestIEs_t));
	ASN_SEQUENCE_ADD(&HandoverRequest->protocolIEs, ie);

	ie->id = Ngap_ProtocolIE_ID_id_UESecurityCapabilities;
	ie->criticality = Ngap_Criticality_reject;
	ie->value.present = Ngap_HandoverRequestIEs__value_PR_UESecurityCapabilities;
	ie->value.choice.UESecurityCapabilities.nRencryptionAlgorithms.size = 2;
	ie->value.choice.UESecurityCapabilities.nRencryptionAlgorithms.buf = (uint8_t *)calloc(2, sizeof(uint8_t));
	ie->value.choice.UESecurityCapabilities.nRencryptionAlgorithms.buf[0] = sess->nRencryptionAlgorithms >> 8;
	ie->value.choice.UESecurityCapabilities.nRencryptionAlgorithms.buf[1] = sess->nRencryptionAlgorithms;
	
	ie->value.choice.UESecurityCapabilities.nRintegrityProtectionAlgorithms.size = 2;
	ie->value.choice.UESecurityCapabilities.nRintegrityProtectionAlgorithms.buf = (uint8_t *)calloc(2, sizeof(uint8_t));	
	ie->value.choice.UESecurityCapabilities.nRintegrityProtectionAlgorithms.buf[0] = sess->nRintegrityProtectionAlgorithms >> 8;
	ie->value.choice.UESecurityCapabilities.nRintegrityProtectionAlgorithms.buf[1] = sess->nRintegrityProtectionAlgorithms;

	
	ie->value.choice.UESecurityCapabilities.eUTRAencryptionAlgorithms.size = 2;
	ie->value.choice.UESecurityCapabilities.eUTRAencryptionAlgorithms.buf = (uint8_t *)calloc(2, sizeof(uint8_t));
	ie->value.choice.UESecurityCapabilities.eUTRAencryptionAlgorithms.buf[0] = sess->eUTRAencryptionAlgorithms >> 8;
	ie->value.choice.UESecurityCapabilities.eUTRAencryptionAlgorithms.buf[1] = sess->eUTRAencryptionAlgorithms;

	
	ie->value.choice.UESecurityCapabilities.eUTRAintegrityProtectionAlgorithms.size = 2;
	ie->value.choice.UESecurityCapabilities.eUTRAintegrityProtectionAlgorithms.buf = (uint8_t *)calloc(2, sizeof(uint8_t));
	ie->value.choice.UESecurityCapabilities.eUTRAintegrityProtectionAlgorithms.buf[0] = sess->eUTRAintegrityProtectionAlgorithms >> 8;
	ie->value.choice.UESecurityCapabilities.eUTRAintegrityProtectionAlgorithms.buf[1] = sess->eUTRAintegrityProtectionAlgorithms;

//SecurityContext	6
	ie = (Ngap_HandoverRequestIEs_t *)calloc(1, sizeof(Ngap_HandoverRequestIEs_t));
	ASN_SEQUENCE_ADD(&HandoverRequest->protocolIEs, ie);

	ie->id = Ngap_ProtocolIE_ID_id_SecurityContext;
	ie->criticality = Ngap_Criticality_reject;
	ie->value.present = Ngap_HandoverRequestIEs__value_PR_SecurityContext;
	ie->value.choice.SecurityContext.nextHopChainingCount = sess->nextHopChainingCount;
	ie->value.choice.SecurityContext.nextHopNH.size = MAX_SECURITY_CONTEXT_LEN;
	ie->value.choice.SecurityContext.nextHopNH.bits_unused = 0;
	ie->value.choice.SecurityContext.nextHopNH.buf = (uint8_t *)calloc(MAX_SECURITY_CONTEXT_LEN, sizeof(uint8_t));
	memcpy(ie->value.choice.SecurityContext.nextHopNH.buf, sess->securityContext, MAX_SECURITY_CONTEXT_LEN);
	
//PDUSessionResourceSetupListHOReq 	7  TBD
	ie = (Ngap_HandoverRequestIEs_t *)calloc(1, sizeof(Ngap_HandoverRequestIEs_t));
	ASN_SEQUENCE_ADD(&HandoverRequest->protocolIEs, ie);

	ie->id = Ngap_ProtocolIE_ID_id_PDUSessionResourceSetupListHOReq;
	ie->criticality = Ngap_Criticality_reject;
	ie->value.present = Ngap_HandoverRequestIEs__value_PR_PDUSessionResourceSetupListHOReq;

	Ngap_PDUSessionResourceSetupListHOReq_t	 *PDUSessionResourceSetupListHOReq = &(ie->value.choice.PDUSessionResourceSetupListHOReq);
	Ngap_PDUSessionResourceSetupItemHOReq_t *PDUSessionResourceSetupItem = NULL;


	for (int iPdu = 0; iPdu < MAX_PDU_SESSION_SIZE; iPdu++)
	{
		if (sess->vecPduSessionID[iPdu] == INVALID_PDU_SESSION_ID)
			continue;
				
		pdu_session_resource_t pdu_resource;
		uint8_t	pdu_resource_id = sess->vecPduSessionID[iPdu];
		if (GetPduSessManager().GetPduSessResource(info.sessId, pdu_resource_id, pdu_resource) == false)
		{
	    	agc_log_printf(AGC_LOG, AGC_LOG_WARNING, "EsmManager::SendHandoverRequest  fail to get pdu session, sessId=%d pdu_resource_id=%d\n", info.sessId, pdu_resource_id);
			continue;
		}
	
		uint32_t teid;
		uint32_t teid_v6;
		agc_std_sockaddr_t addr_v6;
		agc_std_sockaddr_t addr;
		
		if (GetPduSessManager().GetGnbGtpInfo(info.sessId, pdu_resource_id, addr, teid, addr_v6, teid_v6) == false)
		{
	    	agc_log_printf(AGC_LOG, AGC_LOG_WARNING, "EsmManager::SendHandoverRequest  fail to GetGnbGtpInfo, sessId=%d pdu_resource_id=%d\n", info.sessId, pdu_resource_id);
			continue;
		}


		PDUSessionResourceSetupItem = (Ngap_PDUSessionResourceSetupItemHOReq_t *)calloc(1, sizeof(Ngap_PDUSessionResourceSetupItemHOReq_t));
		PDUSessionResourceSetupItem->pDUSessionID = pdu_resource.pdu_session_id;
		ASN_SEQUENCE_ADD(&PDUSessionResourceSetupListHOReq->list, PDUSessionResourceSetupItem);

		PDUSessionResourceSetupItem->s_NSSAI.sST.size = 1;
		PDUSessionResourceSetupItem->s_NSSAI.sST.buf = (uint8_t *)calloc(PDUSessionResourceSetupItem->s_NSSAI.sST.size, sizeof(uint8_t));
		PDUSessionResourceSetupItem->s_NSSAI.sST.buf[0] = sess->allowedSNnsaiSST;

		PDUSessionResourceSetupItem->s_NSSAI.sD = (Ngap_SD_t *)calloc(1, sizeof(Ngap_SD_t));
		PDUSessionResourceSetupItem->s_NSSAI.sD->size = 3;
		PDUSessionResourceSetupItem->s_NSSAI.sD->buf = (uint8_t *)calloc(PDUSessionResourceSetupItem->s_NSSAI.sD->size, sizeof(uint8_t));
		PDUSessionResourceSetupItem->s_NSSAI.sD->buf[0] = sess->allowedSNnsaiSD >> 16;
		PDUSessionResourceSetupItem->s_NSSAI.sD->buf[1] = sess->allowedSNnsaiSD >> 8;
		PDUSessionResourceSetupItem->s_NSSAI.sD->buf[2] = sess->allowedSNnsaiSD;

		Ngap_PDUSessionResourceSetupRequestTransfer_t* pRequetTransfer = (Ngap_PDUSessionResourceSetupRequestTransfer_t *)calloc(1, sizeof(Ngap_PDUSessionResourceSetupRequestTransfer_t));
		Ngap_PDUSessionResourceSetupRequestTransferIEs_t *pduie = NULL;
		
		pduie = (Ngap_PDUSessionResourceSetupRequestTransferIEs_t *)calloc(1, sizeof(Ngap_PDUSessionResourceSetupRequestTransferIEs_t));
		ASN_SEQUENCE_ADD(&pRequetTransfer->protocolIEs, pduie);
		pduie->id = Ngap_ProtocolIE_ID_id_PDUSessionAggregateMaximumBitRate;
		pduie->criticality = Ngap_Criticality_reject;
		pduie->value.present = Ngap_PDUSessionResourceSetupRequestTransferIEs__value_PR_PDUSessionAggregateMaximumBitRate;
				
		asn_int642INTEGER(&pduie->value.choice.PDUSessionAggregateMaximumBitRate.pDUSessionAggregateMaximumBitRateDL, pdu_resource.maxbitrate_dl);
		asn_int642INTEGER(&pduie->value.choice.PDUSessionAggregateMaximumBitRate.pDUSessionAggregateMaximumBitRateUL, pdu_resource.maxbitrate_ul);
		
		pduie = (Ngap_PDUSessionResourceSetupRequestTransferIEs_t *)calloc(1, sizeof(Ngap_PDUSessionResourceSetupRequestTransferIEs_t));
		ASN_SEQUENCE_ADD(&pRequetTransfer->protocolIEs, pduie);
		pduie->id = Ngap_ProtocolIE_ID_id_UL_NGU_UP_TNLInformation;
		pduie->criticality = Ngap_Criticality_reject;
		pduie->value.present = Ngap_PDUSessionResourceSetupRequestTransferIEs__value_PR_UPTransportLayerInformation;
		Ngap_UPTransportLayerInformation_t	 *UPTransportLayerInformation = &pduie->value.choice.UPTransportLayerInformation;
		UPTransportLayerInformation->present = Ngap_UPTransportLayerInformation_PR_gTPTunnel;
		struct Ngap_GTPTunnel	*gTPTunnel = UPTransportLayerInformation->choice.gTPTunnel = (Ngap_GTPTunnel_t *)calloc(1, sizeof(Ngap_GTPTunnel_t));
		uint32_t PDUSessionType;

	    gTPTunnel->gTP_TEID.buf = (uint8_t *)calloc(4, sizeof(uint8_t));
	    gTPTunnel->gTP_TEID.size = 4;
		gTPTunnel->gTP_TEID.buf[0] = teid >> 24;
		gTPTunnel->gTP_TEID.buf[1] = teid >> 16;
		gTPTunnel->gTP_TEID.buf[2] = teid >> 8;
		gTPTunnel->gTP_TEID.buf[3] = teid ;
		
		if (addr.ss_family == AF_INET && addr_v6.ss_family == AF_INET6)  // ipv4 & ipv6
		{
			struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&addr;
			struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&addr_v6;
		    gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(20, sizeof(uint8_t));
			memcpy(gTPTunnel->transportLayerAddress.buf, &local_addr1_v4->sin_addr, 4);
			memcpy(gTPTunnel->transportLayerAddress.buf+4, &local_addr1_v6->sin6_addr, 16);
			gTPTunnel->transportLayerAddress.size = 20;
			PDUSessionType = Ngap_PDUSessionType_ipv4v6;
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"SendHandoverRequest :both IPv4 and IPv6 are valid\n");
			
		}
		else if (addr.ss_family != AF_INET && addr_v6.ss_family == AF_INET6) //only ipv6
		{
			struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&addr_v6;
		    gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(16, sizeof(uint8_t));
			memcpy(gTPTunnel->transportLayerAddress.buf, &local_addr1_v6->sin6_addr, 16);
			gTPTunnel->transportLayerAddress.size = 16;
			PDUSessionType = Ngap_PDUSessionType_ipv6;
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"SendHandoverRequest :only IPv6 is valid\n");
		}
		else if(addr.ss_family == AF_INET && addr_v6.ss_family != AF_INET6) //only ipv4
		{
			struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&addr;
		    gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(4, sizeof(uint8_t));
			memcpy(gTPTunnel->transportLayerAddress.buf, &local_addr1_v4->sin_addr, 4);
			gTPTunnel->transportLayerAddress.size = 4;
			PDUSessionType = Ngap_PDUSessionType_ipv4;
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"SendHandoverRequest :only IPv4 is valid\n");
		}
		else
		{
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"SendHandoverRequest :GetGnbGtpInfo IPv4 and IPv6 both are invalid\n");
			gTPTunnel->transportLayerAddress.size = 0;
		}										
		
		pduie = (Ngap_PDUSessionResourceSetupRequestTransferIEs_t *)calloc(1, sizeof(Ngap_PDUSessionResourceSetupRequestTransferIEs_t));
		ASN_SEQUENCE_ADD(&pRequetTransfer->protocolIEs, pduie);
		pduie->id = Ngap_ProtocolIE_ID_id_PDUSessionType;
		pduie->criticality = Ngap_Criticality_reject;
		pduie->value.present = Ngap_PDUSessionResourceSetupRequestTransferIEs__value_PR_PDUSessionType;
		pduie->value.choice.PDUSessionType = PDUSessionType;
		
		pduie = (Ngap_PDUSessionResourceSetupRequestTransferIEs_t *)calloc(1, sizeof(Ngap_PDUSessionResourceSetupRequestTransferIEs_t));
		ASN_SEQUENCE_ADD(&pRequetTransfer->protocolIEs, pduie);
		pduie->id = Ngap_ProtocolIE_ID_id_QosFlowSetupRequestList;
		pduie->criticality = Ngap_Criticality_reject;
		pduie->value.present = Ngap_PDUSessionResourceSetupRequestTransferIEs__value_PR_QosFlowSetupRequestList;
		Ngap_QosFlowSetupRequestList_t	 *QosFlowSetupRequestList = &pduie->value.choice.QosFlowSetupRequestList;
		Ngap_QosFlowSetupRequestItem_t *qosItem = (Ngap_QosFlowSetupRequestItem_t *)calloc(1, sizeof(Ngap_QosFlowSetupRequestItem_t));
		ASN_SEQUENCE_ADD(&QosFlowSetupRequestList->list, qosItem);
		qosItem->qosFlowIdentifier = pdu_resource.qos_flow_id;
		if (pdu_resource.qos_character_type == pdu_session_resource_t::QOS_NON_DYNAMIC_5QI)
		{
			qosItem->qosFlowLevelQosParameters.qosCharacteristics.present = Ngap_QosCharacteristics_PR_nonDynamic5QI;
			qosItem->qosFlowLevelQosParameters.qosCharacteristics.choice.nonDynamic5QI = (Ngap_NonDynamic5QIDescriptor_t *)calloc(1, sizeof(Ngap_NonDynamic5QIDescriptor_t));
			qosItem->qosFlowLevelQosParameters.qosCharacteristics.choice.nonDynamic5QI->fiveQI = pdu_resource.choice.nonvalue.fiveQI;
			if (pdu_resource.choice.nonvalue.priorityLevelQos >= 1 && pdu_resource.choice.nonvalue.priorityLevelQos <= 127) {
                qosItem->qosFlowLevelQosParameters.qosCharacteristics.choice.nonDynamic5QI->priorityLevelQos = (Ngap_PriorityLevelQos_t *) calloc(
                        1, sizeof(Ngap_PriorityLevelQos_t));
                *qosItem->qosFlowLevelQosParameters.qosCharacteristics.choice.nonDynamic5QI->priorityLevelQos = pdu_resource.choice.nonvalue.priorityLevelQos;
            }
		}
		else if (pdu_resource.qos_character_type == pdu_session_resource_t::QOS_DYNAMIC_5QI)
		{
			qosItem->qosFlowLevelQosParameters.qosCharacteristics.present = Ngap_QosCharacteristics_PR_dynamic5QI;
			qosItem->qosFlowLevelQosParameters.qosCharacteristics.choice.dynamic5QI = (Ngap_Dynamic5QIDescriptor_t *)calloc(1, sizeof(Ngap_Dynamic5QIDescriptor_t));
			qosItem->qosFlowLevelQosParameters.qosCharacteristics.choice.dynamic5QI->priorityLevelQos = pdu_resource.choice.value.priorityLevelQos;
			qosItem->qosFlowLevelQosParameters.qosCharacteristics.choice.dynamic5QI->packetDelayBudget = pdu_resource.choice.value.packetDelayBudget;
			qosItem->qosFlowLevelQosParameters.qosCharacteristics.choice.dynamic5QI->packetErrorRate.pERScalar = pdu_resource.choice.value.pERScalar;
			qosItem->qosFlowLevelQosParameters.qosCharacteristics.choice.dynamic5QI->packetErrorRate.pERExponent = pdu_resource.choice.value.pERExponent;
		}
		qosItem->qosFlowLevelQosParameters.allocationAndRetentionPriority.priorityLevelARP = pdu_resource.priorityLevelARP;
		qosItem->qosFlowLevelQosParameters.allocationAndRetentionPriority.pre_emptionCapability = pdu_resource.pre_emptionCapability;
		qosItem->qosFlowLevelQosParameters.allocationAndRetentionPriority.pre_emptionVulnerability = pdu_resource.pre_emptionVulnerability;
		
		NgapProtocol ngapEncode;
		int32_t len = 0;
		PDUSessionResourceSetupItem->handoverRequestTransfer.buf = (uint8_t *)calloc(200, sizeof(uint8_t));
		if (ngapEncode.EncodeResourceSetupRequestTransfer((void *)pRequetTransfer, (char*)PDUSessionResourceSetupItem->handoverRequestTransfer.buf, (int32_t *)&len) == false)
		{
	    	agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "EsmManager::SendHandoverRequest fail to EncodeResourceSetupRequestTransfer, buf=%p len=%d\n", PDUSessionResourceSetupItem->handoverRequestTransfer.buf, len);
		}
		
		ngapEncode.FreeResourceSetupRequestTransfer((void *)pRequetTransfer);
		PDUSessionResourceSetupItem->handoverRequestTransfer.size = len;
    	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "EsmManager::SendHandoverRequest handoverRequestTransfer len=%d\n", len);
		
	}

	//AllowedNSSAI 	8  TBD
	ie = (Ngap_HandoverRequestIEs_t *)calloc(1, sizeof(Ngap_HandoverRequestIEs_t));
	ASN_SEQUENCE_ADD(&HandoverRequest->protocolIEs, ie);

	ie->id = Ngap_ProtocolIE_ID_id_AllowedNSSAI;
	ie->criticality = Ngap_Criticality_reject;
	ie->value.present = Ngap_HandoverRequestIEs__value_PR_AllowedNSSAI;

	Ngap_AllowedNSSAI_t	 *AllowedNSSAI = &ie->value.choice.AllowedNSSAI;
	Ngap_AllowedNSSAI_Item_t *NssaiItem = (Ngap_AllowedNSSAI_Item_t *)calloc(1, sizeof(Ngap_AllowedNSSAI_Item_t));
	ASN_SEQUENCE_ADD(&AllowedNSSAI->list, NssaiItem);

	NssaiItem->s_NSSAI.sST.size = 1;
	NssaiItem->s_NSSAI.sST.buf = (uint8_t *)calloc(NssaiItem->s_NSSAI.sST.size, sizeof(uint8_t));
	NssaiItem->s_NSSAI.sST.buf[0] = sess->allowedSNnsaiSST;

	NssaiItem->s_NSSAI.sD = (Ngap_SD_t *)calloc(1, sizeof(Ngap_SD_t));
	NssaiItem->s_NSSAI.sD->size = 3;
	NssaiItem->s_NSSAI.sD->buf = (uint8_t *)calloc(NssaiItem->s_NSSAI.sD->size, sizeof(uint8_t));
	NssaiItem->s_NSSAI.sD->buf[0] = sess->allowedSNnsaiSD >> 16;
	NssaiItem->s_NSSAI.sD->buf[1] = sess->allowedSNnsaiSD >> 8;
	NssaiItem->s_NSSAI.sD->buf[2] = sess->allowedSNnsaiSD;

//SourceToTarget_TransparentContainer	9
	ie = (Ngap_HandoverRequestIEs_t *)calloc(1, sizeof(Ngap_HandoverRequestIEs_t));
	ASN_SEQUENCE_ADD(&HandoverRequest->protocolIEs, ie);

	ie->id = Ngap_ProtocolIE_ID_id_SourceToTarget_TransparentContainer;
	ie->criticality = Ngap_Criticality_reject;
	ie->value.present = Ngap_HandoverRequestIEs__value_PR_SourceToTarget_TransparentContainer;
	ie->value.choice.SourceToTarget_TransparentContainer.size = SourceToTarget_TransparentContainer->size;
	ie->value.choice.SourceToTarget_TransparentContainer.buf  = (uint8_t *)calloc(SourceToTarget_TransparentContainer->size, sizeof(uint8_t));
	memcpy(ie->value.choice.SourceToTarget_TransparentContainer.buf, SourceToTarget_TransparentContainer->buf, SourceToTarget_TransparentContainer->size);

//GUAMI	10
	ie = (Ngap_HandoverRequestIEs_t *)calloc(1, sizeof(Ngap_HandoverRequestIEs_t));
	ASN_SEQUENCE_ADD(&HandoverRequest->protocolIEs, ie);

	ie->id = Ngap_ProtocolIE_ID_id_GUAMI;
	ie->criticality = Ngap_Criticality_reject;
	ie->value.present = Ngap_HandoverRequestIEs__value_PR_GUAMI;
	Ngap_GUAMI_t *GUAMI = &ie->value.choice.GUAMI;
	plmn_id_t plmn_id;
	
	plmn_id.mcc1 = sess->guami.plmn.mcc1;
	plmn_id.mcc2 = sess->guami.plmn.mcc2;
	plmn_id.mcc3 = sess->guami.plmn.mcc3;
	plmn_id.mnc1 = sess->guami.plmn.mnc1;
	plmn_id.mnc2 = sess->guami.plmn.mnc2;
	plmn_id.mnc3 = sess->guami.plmn.mnc3;
	GUAMI->pLMNIdentity.size = PLMN_ID_LEN;
	GUAMI->pLMNIdentity.buf = (uint8_t *)calloc(PLMN_ID_LEN, sizeof(uint8_t));
	memcpy(GUAMI->pLMNIdentity.buf, &plmn_id, PLMN_ID_LEN);
	GUAMI->aMFRegionID.size = 1;
	GUAMI->aMFRegionID.buf = (uint8_t *)calloc(1, sizeof(uint8_t));
	GUAMI->aMFRegionID.buf[0] = sess->guami.AMFRegionID;
	GUAMI->aMFSetID.size = 2;
	GUAMI->aMFSetID.buf = (uint8_t *)calloc(2, sizeof(uint8_t));
	GUAMI->aMFSetID.buf[0] = sess->guami.AMFSetId >> 8;
	GUAMI->aMFSetID.buf[1] = sess->guami.AMFSetId;
	GUAMI->aMFSetID.bits_unused = 6;
	GUAMI->aMFPointer.size = 1;
	GUAMI->aMFPointer.buf = (uint8_t *)calloc(1, sizeof(uint8_t));
	GUAMI->aMFPointer.buf[0] = sess->guami.AMFPointer;
	GUAMI->aMFPointer.bits_unused = 2;
		
//All IE Done
	GetNsmManager().SendDownLayerSctp(ngapInfo);


	return true;
}



//lyb,2020
bool EsmManager::SendHandoverCommand(NgapMessage& info, Ngap_TargetToSource_TransparentContainer_t	 *pTTS_TransparentContainer,
	Ngap_PDUSessionResourceAdmittedList_t *PDUSessionResourceAdmittedList)
{
	NgapMessage ngapInfo;
	Ngap_SuccessfulOutcome_t	*successfulOutcome = NULL;
	Ngap_HandoverCommand_t	    *HandoverCommand = NULL;
	Ngap_PDUSessionResourceHandoverList_t	*PDUSessionResourceHandoverList = NULL;
	Ngap_HandoverCommandIEs_t   *ie = NULL;
	
	ngapInfo.amfUeId = info.amfUeId;
	ngapInfo.sessId = info.sessId;
	ngapInfo.gnbUeId = info.gnbUeId;
	ngapInfo.amfId = info.amfId;
	ngapInfo.ranNodeId = info.ranNodeId;
	ngapInfo.sockTarget = info.sockTarget;
	
	ngapInfo.PDUChoice = Ngap_NGAP_PDU_PR_successfulOutcome;
	ngapInfo.ProcCode = Ngap_ProcedureCode_id_HandoverPreparation;
	
	memset(&ngapInfo.ngapMessage, 0, sizeof(Ngap_NGAP_PDU_t));
	
	ngapInfo.ngapMessage.present = Ngap_NGAP_PDU_PR_successfulOutcome;
	ngapInfo.ngapMessage.choice.successfulOutcome = (Ngap_SuccessfulOutcome_t *)calloc(1, sizeof(Ngap_SuccessfulOutcome_t));
	
	successfulOutcome = ngapInfo.ngapMessage.choice.successfulOutcome;
	
	successfulOutcome->procedureCode = Ngap_ProcedureCode_id_HandoverPreparation;
	successfulOutcome->criticality = Ngap_Criticality_reject;
	successfulOutcome->value.present = Ngap_SuccessfulOutcome__value_PR_HandoverCommand;

	HandoverCommand = &successfulOutcome->value.choice.HandoverCommand;

/*
			union Ngap_HandoverCommandIEs__Ngap_value_u {
				Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
				Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
				Ngap_HandoverType_t  HandoverType;
					//Ngap_NASSecurityParametersFromNGRAN_t	 NASSecurityParametersFromNGRAN;
					//Ngap_PDUSessionResourceHandoverList_t	 PDUSessionResourceHandoverList;
					//Ngap_PDUSessionResourceToReleaseListHOCmd_t  PDUSessionResourceToReleaseListHOCmd;
				Ngap_TargetToSource_TransparentContainer_t	 TargetToSource_TransparentContainer;
					//Ngap_CriticalityDiagnostics_t	 CriticalityDiagnostics;
			} choice;
*/

//Start Make IE

//AMF_UE_NGAP_ID
	ie = (Ngap_HandoverCommandIEs_t *)calloc(1, sizeof(Ngap_HandoverCommandIEs_t));
	ASN_SEQUENCE_ADD(&HandoverCommand->protocolIEs, ie);
	
	ie->id = Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
	ie->criticality = Ngap_Criticality_reject;
	ie->value.present = Ngap_HandoverCommandIEs__value_PR_AMF_UE_NGAP_ID;
	asn_int642INTEGER(&ie->value.choice.AMF_UE_NGAP_ID, info.amfUeId);

//RAN_UE_NGAP_ID
	ie = (Ngap_HandoverCommandIEs_t *)calloc(1, sizeof(Ngap_HandoverCommandIEs_t));
	ASN_SEQUENCE_ADD(&HandoverCommand->protocolIEs, ie);
	
	ie->id = Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
	ie->criticality = Ngap_Criticality_reject;
	ie->value.present = Ngap_HandoverCommandIEs__value_PR_RAN_UE_NGAP_ID;
	ie->value.choice.RAN_UE_NGAP_ID = info.gnbUeId;

//HandoverType
	ie = (Ngap_HandoverCommandIEs_t *)calloc(1, sizeof(Ngap_HandoverCommandIEs_t));
	ASN_SEQUENCE_ADD(&HandoverCommand->protocolIEs, ie);

	ie->id = Ngap_ProtocolIE_ID_id_HandoverType;
	ie->criticality = Ngap_Criticality_reject;
	ie->value.present = Ngap_HandoverCommandIEs__value_PR_HandoverType;
	ie->value.choice.HandoverType = Ngap_HandoverType_intra5gs;

//PDUSessionResourceHandoverList
	ie = (Ngap_HandoverCommandIEs_t *)calloc(1, sizeof(Ngap_HandoverCommandIEs_t));
	ASN_SEQUENCE_ADD(&HandoverCommand->protocolIEs, ie);
	
	ie->id = Ngap_ProtocolIE_ID_id_PDUSessionResourceHandoverList;
	ie->criticality = Ngap_Criticality_reject;
	ie->value.present = Ngap_HandoverCommandIEs__value_PR_PDUSessionResourceHandoverList;
	PDUSessionResourceHandoverList = &ie->value.choice.PDUSessionResourceHandoverList;

	if (PDUSessionResourceAdmittedList != NULL)
	{
		NgapProtocol ngapDecode;
		OCTET_STRING_t *tranfer;
		
	    for (int i = 0; i < PDUSessionResourceAdmittedList->list.count; i++)
	    {
			Ngap_HandoverRequestAcknowledgeTransfer_t *pHandoverReqAckContainer = NULL;
	    	Ngap_PDUSessionResourceAdmittedItem_t *item= (Ngap_PDUSessionResourceAdmittedItem_t *)PDUSessionResourceAdmittedList->list.array[i];
	    	tranfer = &item->handoverRequestAcknowledgeTransfer;

	    	int32_t len = tranfer->size;
	    	ngapDecode.DecodeHandoverReqAckTransfer((char*)tranfer->buf, (int32_t *)&len, (void **)&pHandoverReqAckContainer);

				
			Ngap_PDUSessionResourceHandoverItem_t *hoitem = (Ngap_PDUSessionResourceHandoverItem_t *)calloc(1, sizeof(Ngap_PDUSessionResourceHandoverItem_t));
			ASN_SEQUENCE_ADD(&PDUSessionResourceHandoverList->list, hoitem);
			hoitem->pDUSessionID = item->pDUSessionID;

			if (pHandoverReqAckContainer == NULL)
			{
				continue;
			}
			

			Ngap_HandoverCommandTransfer_t *pHandoverCommandContainer = (Ngap_HandoverCommandTransfer_t *)calloc(1, sizeof(Ngap_HandoverCommandTransfer_t));		
			Ngap_QosFlowToBeForwardedList_t  *QosFlowSetupRequestList = (Ngap_QosFlowToBeForwardedList_t *)calloc(1, sizeof(Ngap_QosFlowToBeForwardedList_t));
/*
			if (pHandoverReqAckContainer->dL_NGU_UP_TNLInformation.present == Ngap_UPTransportLayerInformation_PR_gTPTunnel)
			{
				pHandoverCommandContainer->dLForwardingUP_TNLInformation = (Ngap_UPTransportLayerInformation_t *)calloc(1, sizeof(Ngap_UPTransportLayerInformation_t));
				
				Ngap_GTPTunnel_t *gTPTunnel = pHandoverReqAckContainer->dL_NGU_UP_TNLInformation.choice.gTPTunnel;
				
				pHandoverCommandContainer->dLForwardingUP_TNLInformation->present = Ngap_UPTransportLayerInformation_PR_gTPTunnel;
				pHandoverCommandContainer->dLForwardingUP_TNLInformation->choice.gTPTunnel = (Ngap_GTPTunnel_t *)calloc(1, sizeof(Ngap_GTPTunnel_t));

				pHandoverCommandContainer->dLForwardingUP_TNLInformation->choice.gTPTunnel->gTP_TEID.buf = (uint8_t *)calloc(gTPTunnel->gTP_TEID.size, sizeof(uint8_t));
				pHandoverCommandContainer->dLForwardingUP_TNLInformation->choice.gTPTunnel->gTP_TEID.size = gTPTunnel->gTP_TEID.size;
				memcpy(pHandoverCommandContainer->dLForwardingUP_TNLInformation->choice.gTPTunnel->gTP_TEID.buf, gTPTunnel->gTP_TEID.buf, gTPTunnel->gTP_TEID.size);

 			    pHandoverCommandContainer->dLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(gTPTunnel->transportLayerAddress.size, sizeof(uint8_t));
				pHandoverCommandContainer->dLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.size = gTPTunnel->transportLayerAddress.size;
				memcpy(pHandoverCommandContainer->dLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.buf, gTPTunnel->transportLayerAddress.buf, gTPTunnel->transportLayerAddress.size);
			}
*/
			pHandoverCommandContainer->qosFlowToBeForwardedList = QosFlowSetupRequestList;			
			for (int iQosFlow = 0; iQosFlow < pHandoverReqAckContainer->qosFlowSetupResponseList.list.count; iQosFlow++)
			{
				Ngap_QosFlowItemWithDataForwarding_t *qosFlowItem = (Ngap_QosFlowItemWithDataForwarding_t *)pHandoverReqAckContainer->qosFlowSetupResponseList.list.array[iQosFlow];
				Ngap_QosFlowToBeForwardedItem_t *qosItem = (Ngap_QosFlowToBeForwardedItem_t *)calloc(1, sizeof(Ngap_QosFlowToBeForwardedItem_t));
				ASN_SEQUENCE_ADD(&QosFlowSetupRequestList->list, qosItem);
				qosItem->qosFlowIdentifier = qosFlowItem->qosFlowIdentifier;
			}			

			if (pHandoverReqAckContainer->dataForwardingResponseDRBList != NULL)
			{
				pHandoverCommandContainer->dataForwardingResponseDRBList = (Ngap_DataForwardingResponseDRBList_t *)calloc(1, sizeof(Ngap_DataForwardingResponseDRBList_t));
				
				for (int iDrb = 0; iDrb < pHandoverReqAckContainer->dataForwardingResponseDRBList->list.count; iDrb++)
				{
					Ngap_DataForwardingResponseDRBItem_t *drbRespItem = (Ngap_DataForwardingResponseDRBItem_t *)pHandoverReqAckContainer->dataForwardingResponseDRBList->list.array[iDrb];
					Ngap_DataForwardingResponseDRBItem_t *drbItem = (Ngap_DataForwardingResponseDRBItem_t *)calloc(1, sizeof(Ngap_DataForwardingResponseDRBItem_t));
					ASN_SEQUENCE_ADD(&pHandoverCommandContainer->dataForwardingResponseDRBList->list, drbItem);
					drbItem->dRB_ID = drbRespItem->dRB_ID;
/*
					if (drbRespItem->dLForwardingUP_TNLInformation != NULL && drbRespItem->dLForwardingUP_TNLInformation->present == Ngap_UPTransportLayerInformation_PR_gTPTunnel)
					{
						Ngap_GTPTunnel_t *gTPTunnel = drbRespItem->dLForwardingUP_TNLInformation->choice.gTPTunnel;
						
						drbItem->dLForwardingUP_TNLInformation = (Ngap_UPTransportLayerInformation_t *)calloc(1, sizeof(Ngap_UPTransportLayerInformation_t));
						drbItem->dLForwardingUP_TNLInformation->present = Ngap_UPTransportLayerInformation_PR_gTPTunnel;
						drbItem->dLForwardingUP_TNLInformation->choice.gTPTunnel = (Ngap_GTPTunnel_t *)calloc(1, sizeof(Ngap_GTPTunnel_t));

						drbItem->dLForwardingUP_TNLInformation->choice.gTPTunnel->gTP_TEID.buf = (uint8_t *)calloc(gTPTunnel->gTP_TEID.size, sizeof(uint8_t));
						drbItem->dLForwardingUP_TNLInformation->choice.gTPTunnel->gTP_TEID.size = gTPTunnel->gTP_TEID.size;
						memcpy(drbItem->dLForwardingUP_TNLInformation->choice.gTPTunnel->gTP_TEID.buf, gTPTunnel->gTP_TEID.buf, gTPTunnel->gTP_TEID.size);
						
					    drbItem->dLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(gTPTunnel->transportLayerAddress.size, sizeof(uint8_t));
						drbItem->dLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.size = gTPTunnel->transportLayerAddress.size;
						memcpy(drbItem->dLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.buf, gTPTunnel->transportLayerAddress.buf, gTPTunnel->transportLayerAddress.size);
					}
					*/
					uint32_t teid = 0;
					uint32_t teid_v6 = 0;
					agc_std_sockaddr_t addr;
					agc_std_sockaddr_t addr_v6;
					GetPduSessManager().GetGnbGtpInfo(info.sessId, hoitem->pDUSessionID, addr, teid, addr_v6, teid_v6);
					if (drbRespItem->uLForwardingUP_TNLInformation != NULL && drbRespItem->dLForwardingUP_TNLInformation->present == Ngap_UPTransportLayerInformation_PR_gTPTunnel)
					{
						Ngap_GTPTunnel_t *gTPTunnel = drbRespItem->uLForwardingUP_TNLInformation->choice.gTPTunnel;
									
						drbItem->dLForwardingUP_TNLInformation = (Ngap_UPTransportLayerInformation_t *)calloc(1, sizeof(Ngap_UPTransportLayerInformation_t));
						drbItem->dLForwardingUP_TNLInformation->present = Ngap_UPTransportLayerInformation_PR_gTPTunnel;
						drbItem->dLForwardingUP_TNLInformation->choice.gTPTunnel = (Ngap_GTPTunnel_t *)calloc(1, sizeof(Ngap_GTPTunnel_t));

						drbItem->dLForwardingUP_TNLInformation->choice.gTPTunnel->gTP_TEID.buf = (uint8_t *)calloc(gTPTunnel->gTP_TEID.size, sizeof(uint8_t));
						drbItem->dLForwardingUP_TNLInformation->choice.gTPTunnel->gTP_TEID.size = gTPTunnel->gTP_TEID.size;
						/*memcpy(drbItem->dLForwardingUP_TNLInformation->choice.gTPTunnel->gTP_TEID.buf, gTPTunnel->gTP_TEID.buf, gTPTunnel->gTP_TEID.size);
						
					    drbItem->dLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(gTPTunnel->transportLayerAddress.size, sizeof(uint8_t));
						drbItem->dLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.size = gTPTunnel->transportLayerAddress.size;
						memcpy(drbItem->dLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.buf, gTPTunnel->transportLayerAddress.buf, gTPTunnel->transportLayerAddress.size);
							*/	
						drbItem->uLForwardingUP_TNLInformation = (Ngap_UPTransportLayerInformation_t *)calloc(1, sizeof(Ngap_UPTransportLayerInformation_t));
						drbItem->uLForwardingUP_TNLInformation->present = Ngap_UPTransportLayerInformation_PR_gTPTunnel;					
						drbItem->uLForwardingUP_TNLInformation->choice.gTPTunnel = (Ngap_GTPTunnel_t *)calloc(1, sizeof(Ngap_GTPTunnel_t));
						
						drbItem->uLForwardingUP_TNLInformation->choice.gTPTunnel->gTP_TEID.buf = (uint8_t *)calloc(gTPTunnel->gTP_TEID.size, sizeof(uint8_t));
						drbItem->uLForwardingUP_TNLInformation->choice.gTPTunnel->gTP_TEID.size = gTPTunnel->gTP_TEID.size;
						/*memcpy(drbItem->uLForwardingUP_TNLInformation->choice.gTPTunnel->gTP_TEID.buf, gTPTunnel->gTP_TEID.buf, gTPTunnel->gTP_TEID.size);
						
					    drbItem->uLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(gTPTunnel->transportLayerAddress.size, sizeof(uint8_t));
						drbItem->uLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.size = gTPTunnel->transportLayerAddress.size;
						memcpy(drbItem->uLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.buf, gTPTunnel->transportLayerAddress.buf, gTPTunnel->transportLayerAddress.size);
					*/
						drbItem->dLForwardingUP_TNLInformation->choice.gTPTunnel->gTP_TEID.buf[0] = teid >> 24;
						drbItem->dLForwardingUP_TNLInformation->choice.gTPTunnel->gTP_TEID.buf[1] = teid >> 16;
						drbItem->dLForwardingUP_TNLInformation->choice.gTPTunnel->gTP_TEID.buf[2] = teid >> 8;
						drbItem->dLForwardingUP_TNLInformation->choice.gTPTunnel->gTP_TEID.buf[3] = teid ;
						
						drbItem->uLForwardingUP_TNLInformation->choice.gTPTunnel->gTP_TEID.buf[0] = teid >> 24;
						drbItem->uLForwardingUP_TNLInformation->choice.gTPTunnel->gTP_TEID.buf[1] = teid >> 16;
						drbItem->uLForwardingUP_TNLInformation->choice.gTPTunnel->gTP_TEID.buf[2] = teid >> 8;
						drbItem->uLForwardingUP_TNLInformation->choice.gTPTunnel->gTP_TEID.buf[3] = teid ;
			
						if (addr.ss_family == AF_INET && addr_v6.ss_family != AF_INET6)	// only ipv4
						{
							struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&addr;
							drbItem->dLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(4, sizeof(uint8_t));
							memcpy(drbItem->dLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.buf, &local_addr1_v4->sin_addr, 4);					
							drbItem->dLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.size = 4;
							
							drbItem->uLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(4, sizeof(uint8_t));
							memcpy(drbItem->uLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.buf, &local_addr1_v4->sin_addr, 4);					
							drbItem->uLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.size = 4;
						}
						else if (addr.ss_family != AF_INET && addr_v6.ss_family == AF_INET6) //only ipv6
						{
							struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&addr_v6;
							drbItem->dLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(16, sizeof(uint8_t));
							memcpy(drbItem->dLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.buf, &local_addr1_v6->sin6_addr, 16);
							drbItem->dLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.size = 16;
							
							drbItem->uLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(16, sizeof(uint8_t));
							memcpy(drbItem->uLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.buf, &local_addr1_v6->sin6_addr, 16);
							drbItem->uLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.size = 16;
						}
						else if (addr.ss_family == AF_INET && addr_v6.ss_family == AF_INET6)  // ipv4 & ipv6
						{
							struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&addr;
							struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&addr_v6;

							drbItem->dLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(20, sizeof(uint8_t));
							memcpy(drbItem->dLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.buf, &local_addr1_v4->sin_addr, 4);
							memcpy(drbItem->dLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.buf+4, &local_addr1_v6->sin6_addr, 16);
							drbItem->dLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.size = 20;
							
							drbItem->uLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(20, sizeof(uint8_t));
							memcpy(drbItem->uLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.buf, &local_addr1_v4->sin_addr, 4);
							memcpy(drbItem->uLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.buf+4, &local_addr1_v6->sin6_addr, 16);
							drbItem->uLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.size = 20;
						}
						else
						{
							agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"SendHandoverCommand :GetGnbGtpInfo IPv4 and IPv6 both are invalid\n");
							drbItem->dLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.size = 0;
							drbItem->uLForwardingUP_TNLInformation->choice.gTPTunnel->transportLayerAddress.size = 0;
						}

					}		
				}
			}
			/*else 
			{
				uint8_t drb = 0;

				if (GetPduSessManager().getHoDrb(info.sessId, hoitem->pDUSessionID, drb) == true)
				{
					pHandoverCommandContainer->dataForwardingResponseDRBList = (Ngap_DataForwardingResponseDRBList_t *)calloc(1, sizeof(Ngap_DataForwardingResponseDRBList_t));
					Ngap_DataForwardingResponseDRBItem_t *drbItem = (Ngap_DataForwardingResponseDRBItem_t *)calloc(1, sizeof(Ngap_DataForwardingResponseDRBItem_t));
					ASN_SEQUENCE_ADD(&pHandoverCommandContainer->dataForwardingResponseDRBList->list, drbItem);
					drbItem->dRB_ID = drb;
				}
			}*/
			
			NgapProtocol ngapEncode;
			len = 0;
			hoitem->handoverCommandTransfer.buf = (uint8_t *)calloc(200, sizeof(uint8_t));
			ngapEncode.EncodeHandoverCommandTransfer((void *)pHandoverCommandContainer, (char*)hoitem->handoverCommandTransfer.buf, (int32_t *)&len);
			ngapEncode.FreeHandoverCommandTransfer((void *)pHandoverCommandContainer);
			hoitem->handoverCommandTransfer.size = len;
			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "EsmManager::SendHandoverCommand handoverCommandTransfer len=%d\n", len);
	    }		
	}
	
	
	//TargetToSource_TransparentContainer
	ie = (Ngap_HandoverCommandIEs_t *)calloc(1, sizeof(Ngap_HandoverCommandIEs_t));
	ASN_SEQUENCE_ADD(&HandoverCommand->protocolIEs, ie);

	ie->id = Ngap_ProtocolIE_ID_id_TargetToSource_TransparentContainer;
	ie->criticality = Ngap_Criticality_reject;
	ie->value.present = Ngap_HandoverCommandIEs__value_PR_TargetToSource_TransparentContainer;
//Need to check result of the memcpy.
	ie->value.choice.TargetToSource_TransparentContainer.size = pTTS_TransparentContainer->size;

	ie->value.choice.TargetToSource_TransparentContainer.buf = (uint8_t *)calloc(pTTS_TransparentContainer->size, sizeof(uint8_t));

	memcpy(ie->value.choice.TargetToSource_TransparentContainer.buf, pTTS_TransparentContainer->buf,   pTTS_TransparentContainer->size);


//All IE Done
	GetNsmManager().SendDownLayerSctp(ngapInfo);
	return true;

}



bool EsmManager::SendUCRelCommand(NgapMessage& info)
{

    NgapMessage ngapInfo;
	Ngap_InitiatingMessage_t	*initiatingMessage = NULL;
	Ngap_UEContextReleaseCommand_IEs_t *ie = NULL;
	
//	ngapInfo used for msg sent to RAN/Target.
    ngapInfo.amfUeId = (uint64_t)info.sessId;
    ngapInfo.sessId = info.sessId;
    ngapInfo.gnbUeId = info.gnbUeId;
	ngapInfo.amfId = info.amfId;
	ngapInfo.ranNodeId = info.ranNodeId;
	ngapInfo.sockTarget = info.sockTarget;
	
    ngapInfo.PDUChoice = Ngap_NGAP_PDU_PR_initiatingMessage;
    ngapInfo.ProcCode = Ngap_ProcedureCode_id_UEContextRelease;
	
	memset(&ngapInfo.ngapMessage, 0, sizeof (Ngap_NGAP_PDU_t));
	
    ngapInfo.ngapMessage.present = Ngap_NGAP_PDU_PR_initiatingMessage;
    ngapInfo.ngapMessage.choice.initiatingMessage = (Ngap_InitiatingMessage_t *)calloc(1, sizeof(Ngap_InitiatingMessage_t));
    initiatingMessage = ngapInfo.ngapMessage.choice.initiatingMessage;
    initiatingMessage->procedureCode = Ngap_ProcedureCode_id_UEContextRelease;
    initiatingMessage->criticality = Ngap_Criticality_reject;
    initiatingMessage->value.present = Ngap_InitiatingMessage__value_PR_UEContextReleaseCommand;

	Ngap_UEContextReleaseCommand_t *UEContextReleaseCommand = &initiatingMessage->value.choice.UEContextReleaseCommand;
	
/*	union Ngap_UEContextReleaseCommand_IEs__Ngap_value_u {
			Ngap_UE_NGAP_IDs_t	 UE_NGAP_IDs;
			Ngap_Cause_t	 Cause;
		} choice;
*/
	
//Start Make IE 

//UE_NGAP_IDs
	ie = (Ngap_UEContextReleaseCommand_IEs_t *)calloc(1, sizeof(Ngap_UEContextReleaseCommand_IEs_t));
	ASN_SEQUENCE_ADD(&UEContextReleaseCommand->protocolIEs, ie);
	
	ie->id = Ngap_ProtocolIE_ID_id_UE_NGAP_IDs;
	ie->criticality = Ngap_Criticality_reject;
	ie->value.present = Ngap_UEContextReleaseCommand_IEs__value_PR_UE_NGAP_IDs;
	ie->value.choice.UE_NGAP_IDs.present = Ngap_UE_NGAP_IDs_PR_uE_NGAP_ID_pair;
	ie->value.choice.UE_NGAP_IDs.choice.uE_NGAP_ID_pair = (Ngap_UE_NGAP_ID_pair_t *)calloc(1, sizeof(Ngap_UE_NGAP_ID_pair_t));
	asn_int642INTEGER(&(ie->value.choice.UE_NGAP_IDs.choice.uE_NGAP_ID_pair->aMF_UE_NGAP_ID), ngapInfo.amfUeId);
	ie->value.choice.UE_NGAP_IDs.choice.uE_NGAP_ID_pair->rAN_UE_NGAP_ID = ngapInfo.gnbUeId;

//Cause	
	ie = (Ngap_UEContextReleaseCommand_IEs_t *)calloc(1, sizeof(Ngap_UEContextReleaseCommand_IEs_t));
	ASN_SEQUENCE_ADD(&UEContextReleaseCommand->protocolIEs, ie);
	
	ie->id = Ngap_ProtocolIE_ID_id_Cause;
	ie->criticality = Ngap_Criticality_reject;
	ie->value.present = Ngap_UEContextReleaseCommand_IEs__value_PR_Cause;
	ie->value.choice.Cause.present = Ngap_Cause_PR_radioNetwork;
	ie->value.choice.Cause.choice.radioNetwork = Ngap_CauseRadioNetwork_successful_handover;
		
//All IE Done
	GetNsmManager().SendDownLayerSctp(ngapInfo);

	return true;
}


//lyb,2020-5-19
bool EsmManager::SendHOPrepareFailure(NgapMessage& info, Ngap_Cause_t *ptCause)
{
	NgapMessage ngapInfo;
	Ngap_UnsuccessfulOutcome_t *unsuccessfulOutcome = NULL;
	Ngap_HandoverPreparationFailure_t	 *HandoverPreparationFailure = NULL;
	Ngap_HandoverPreparationFailureIEs_t   *ie = NULL;
	
	ngapInfo.amfUeId = info.amfUeId;
	ngapInfo.sessId = info.sessId;
	ngapInfo.gnbUeId = info.gnbUeId;
	ngapInfo.amfId = info.amfId;
	ngapInfo.ranNodeId = info.ranNodeId;
	ngapInfo.sockTarget = info.sockTarget;
	
	ngapInfo.PDUChoice = Ngap_NGAP_PDU_PR_unsuccessfulOutcome;
	ngapInfo.ProcCode = Ngap_ProcedureCode_id_HandoverPreparation;
	
	memset(&ngapInfo.ngapMessage, 0, sizeof(Ngap_NGAP_PDU_t));
	
	ngapInfo.ngapMessage.present = Ngap_NGAP_PDU_PR_unsuccessfulOutcome;
	ngapInfo.ngapMessage.choice.unsuccessfulOutcome = (Ngap_UnsuccessfulOutcome_t *)calloc(1, sizeof(Ngap_UnsuccessfulOutcome_t));
	
	unsuccessfulOutcome = ngapInfo.ngapMessage.choice.unsuccessfulOutcome;
	
	unsuccessfulOutcome->procedureCode = Ngap_ProcedureCode_id_HandoverPreparation;
	unsuccessfulOutcome->criticality = Ngap_Criticality_reject;
	unsuccessfulOutcome->value.present = Ngap_UnsuccessfulOutcome__value_PR_HandoverPreparationFailure;

	HandoverPreparationFailure = &unsuccessfulOutcome->value.choice.HandoverPreparationFailure;

/*
					union Ngap_HandoverPreparationFailureIEs__Ngap_value_u {
						Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
						Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
						Ngap_Cause_t	 Cause;
							//Ngap_CriticalityDiagnostics_t	 CriticalityDiagnostics;
					} choice;
*/

//Start Make IE

//AMF_UE_NGAP_ID
	ie = (Ngap_HandoverPreparationFailureIEs_t *)calloc(1, sizeof(Ngap_HandoverPreparationFailureIEs_t));
	ASN_SEQUENCE_ADD(&HandoverPreparationFailure->protocolIEs, ie);
	
	ie->id = Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
	ie->criticality = Ngap_Criticality_ignore;
	ie->value.present = Ngap_HandoverPreparationFailureIEs__value_PR_AMF_UE_NGAP_ID;
	asn_int642INTEGER(&ie->value.choice.AMF_UE_NGAP_ID, info.amfUeId);

//RAN_UE_NGAP_ID
	ie = (Ngap_HandoverPreparationFailureIEs_t *)calloc(1, sizeof(Ngap_HandoverPreparationFailureIEs_t));
	ASN_SEQUENCE_ADD(&HandoverPreparationFailure->protocolIEs, ie);
	
	ie->id = Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
	ie->criticality = Ngap_Criticality_ignore;
	ie->value.present = Ngap_HandoverPreparationFailureIEs__value_PR_RAN_UE_NGAP_ID;
	ie->value.choice.RAN_UE_NGAP_ID = info.gnbUeId;

//Cause
	ie = (Ngap_HandoverPreparationFailureIEs_t *)calloc(1, sizeof(Ngap_HandoverPreparationFailureIEs_t));
	ASN_SEQUENCE_ADD(&HandoverPreparationFailure->protocolIEs, ie);

	ie->id = Ngap_ProtocolIE_ID_id_Cause;
	ie->criticality = Ngap_Criticality_ignore;
	ie->value.present = Ngap_HandoverPreparationFailureIEs__value_PR_Cause;
	if (ptCause == NULL)
	{
		ie->value.choice.Cause.present = Ngap_Cause_PR_radioNetwork;
		ie->value.choice.Cause.choice.radioNetwork = Ngap_CauseRadioNetwork_unspecified;
	}
	else
	{
		memcpy(&ie->value.choice.Cause, ptCause, sizeof(Ngap_Cause_t));
	}

//All IE Done

	GetNsmManager().SendDownLayerSctp(ngapInfo);

	return true;

}




//lyb,2020-5-18
bool EsmManager::SendHandoverCancelAck(NgapMessage& info)
{
	NgapMessage ngapInfo;
	Ngap_SuccessfulOutcome_t	*successfulOutcome = NULL;
	Ngap_HandoverCancelAcknowledgeIEs_t	*ie = NULL;
	
	ngapInfo.amfUeId = info.amfUeId;
	ngapInfo.sessId = info.sessId;
	ngapInfo.gnbUeId = info.gnbUeId;
	ngapInfo.amfId = info.amfId;
	ngapInfo.ranNodeId = info.ranNodeId;
	ngapInfo.sockTarget = info.sockTarget;
	
	ngapInfo.PDUChoice = Ngap_NGAP_PDU_PR_successfulOutcome;
	ngapInfo.ProcCode = Ngap_ProcedureCode_id_HandoverCancel;
	
	memset(&ngapInfo.ngapMessage, 0, sizeof(Ngap_NGAP_PDU_t));
	
	ngapInfo.ngapMessage.present = Ngap_NGAP_PDU_PR_successfulOutcome;
	ngapInfo.ngapMessage.choice.successfulOutcome = (Ngap_SuccessfulOutcome_t *)calloc(1, sizeof(Ngap_SuccessfulOutcome_t));
	
	successfulOutcome = ngapInfo.ngapMessage.choice.successfulOutcome;
	
	successfulOutcome->procedureCode = Ngap_ProcedureCode_id_HandoverCancel;
	successfulOutcome->criticality = Ngap_Criticality_reject;
	successfulOutcome->value.present = Ngap_SuccessfulOutcome__value_PR_HandoverCancelAcknowledge;

	Ngap_HandoverCancelAcknowledge_t *HandoverCancelAcknowledge = &successfulOutcome->value.choice.HandoverCancelAcknowledge;


/*
union Ngap_HandoverCancelAcknowledgeIEs__Ngap_value_u {
	Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
	Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
		//Ngap_CriticalityDiagnostics_t	 CriticalityDiagnostics;
} choice;
*/
	
//Start Make IE 

//AMF_UE_NGAP_ID
	ie = (Ngap_HandoverCancelAcknowledgeIEs_t *)calloc(1, sizeof(Ngap_HandoverCancelAcknowledgeIEs_t));
	ASN_SEQUENCE_ADD(&HandoverCancelAcknowledge->protocolIEs, ie);
	
	ie->id = Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
	ie->criticality = Ngap_Criticality_ignore;
	ie->value.present = Ngap_HandoverCancelAcknowledgeIEs__value_PR_AMF_UE_NGAP_ID;
	asn_int642INTEGER(&ie->value.choice.AMF_UE_NGAP_ID, info.sessId);

//RAN_UE_NGAP_ID
	ie = (Ngap_HandoverCancelAcknowledgeIEs_t *)calloc(1, sizeof(Ngap_HandoverCancelAcknowledgeIEs_t));
	ASN_SEQUENCE_ADD(&HandoverCancelAcknowledge->protocolIEs, ie);
	
	ie->id = Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
	ie->criticality = Ngap_Criticality_ignore;
	ie->value.present = Ngap_HandoverCancelAcknowledgeIEs__value_PR_RAN_UE_NGAP_ID;
	ie->value.choice.RAN_UE_NGAP_ID = info.gnbUeId;

//All IE Done

	GetNsmManager().SendDownLayerSctp(ngapInfo);

	return true;

}


bool EsmManager::SendPathSwitchReqAck(NgapMessage& info)
{
	NgapMessage ngapInfo;
	Ngap_SuccessfulOutcome_t	*successfulOutcome = NULL;
	Ngap_PathSwitchRequestAcknowledgeIEs_t	*ie = NULL;
	
	ngapInfo.amfUeId = info.amfUeId;
	ngapInfo.sessId = info.sessId;
	ngapInfo.gnbUeId = info.gnbUeId;
	ngapInfo.amfId = info.amfId;
	ngapInfo.ranNodeId = info.ranNodeId;
	ngapInfo.sockTarget = info.sockTarget;
	
	ngapInfo.PDUChoice = Ngap_NGAP_PDU_PR_successfulOutcome;
	ngapInfo.ProcCode = Ngap_ProcedureCode_id_PathSwitchRequest;
	
	NgapSession *sess = NULL;
	if (GetNgapSessManager().GetNgapSession(info.sessId, &sess) == false)
	{
    	agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "EsmManager::SendPathSwitchReqAck  fail to get ngap session, sessId=%d\n", info.sessId);
		return false;
	}
	
	memset(&ngapInfo.ngapMessage, 0, sizeof(Ngap_NGAP_PDU_t));
	
	ngapInfo.ngapMessage.present = Ngap_NGAP_PDU_PR_successfulOutcome;
	ngapInfo.ngapMessage.choice.successfulOutcome = (Ngap_SuccessfulOutcome_t *)calloc(1, sizeof(Ngap_SuccessfulOutcome_t));
	
	successfulOutcome = ngapInfo.ngapMessage.choice.successfulOutcome;
	
	successfulOutcome->procedureCode = Ngap_ProcedureCode_id_PathSwitchRequest;
	successfulOutcome->criticality = Ngap_Criticality_reject;
	successfulOutcome->value.present = Ngap_SuccessfulOutcome__value_PR_PathSwitchRequestAcknowledge;

	Ngap_PathSwitchRequestAcknowledge_t *PathSwitchRequestAcknowledge = &successfulOutcome->value.choice.PathSwitchRequestAcknowledge;
	
//AMF_UE_NGAP_ID	1
	ie = (Ngap_PathSwitchRequestAcknowledgeIEs_t *)calloc(1, sizeof(Ngap_PathSwitchRequestAcknowledgeIEs_t));
	ASN_SEQUENCE_ADD(&PathSwitchRequestAcknowledge->protocolIEs, ie);
	
	ie->id = Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
	ie->criticality = Ngap_Criticality_ignore;
	ie->value.present = Ngap_PathSwitchRequestAcknowledgeIEs__value_PR_AMF_UE_NGAP_ID;
	asn_int642INTEGER(&ie->value.choice.AMF_UE_NGAP_ID, ngapInfo.sessId);
	
//RAN_UE_NGAP_ID
	ie = (Ngap_PathSwitchRequestAcknowledgeIEs_t *)calloc(1, sizeof(Ngap_PathSwitchRequestAcknowledgeIEs_t));
	ASN_SEQUENCE_ADD(&PathSwitchRequestAcknowledge->protocolIEs, ie);
	
	ie->id = Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
	ie->criticality = Ngap_Criticality_ignore;
	ie->value.present = Ngap_PathSwitchRequestAcknowledgeIEs__value_PR_RAN_UE_NGAP_ID;
	ie->value.choice.RAN_UE_NGAP_ID = info.gnbUeId;
	
//SecurityContext	6
	ie = (Ngap_PathSwitchRequestAcknowledgeIEs_t *)calloc(1, sizeof(Ngap_PathSwitchRequestAcknowledgeIEs_t));
	ASN_SEQUENCE_ADD(&PathSwitchRequestAcknowledge->protocolIEs, ie);

	ie->id = Ngap_ProtocolIE_ID_id_SecurityContext;
	ie->criticality = Ngap_Criticality_reject;
	ie->value.present = Ngap_PathSwitchRequestAcknowledgeIEs__value_PR_SecurityContext;
	ie->value.choice.SecurityContext.nextHopChainingCount = 1;
	ie->value.choice.SecurityContext.nextHopNH.size = MAX_SECURITY_CONTEXT_LEN;
	ie->value.choice.SecurityContext.nextHopNH.bits_unused = 0;
	ie->value.choice.SecurityContext.nextHopNH.buf = (uint8_t *)calloc(MAX_SECURITY_CONTEXT_LEN, sizeof(uint8_t));
	memcpy(ie->value.choice.SecurityContext.nextHopNH.buf, sess->securityContext, MAX_SECURITY_CONTEXT_LEN);
	
	//PDUSessionResourceSwitchedList	7  TBD
	ie = (Ngap_PathSwitchRequestAcknowledgeIEs_t *)calloc(1, sizeof(Ngap_PathSwitchRequestAcknowledgeIEs_t));
	ASN_SEQUENCE_ADD(&PathSwitchRequestAcknowledge->protocolIEs, ie);

	ie->id = Ngap_ProtocolIE_ID_id_PDUSessionResourceSwitchedList;
	ie->criticality = Ngap_Criticality_ignore;
	ie->value.present = Ngap_PathSwitchRequestAcknowledgeIEs__value_PR_PDUSessionResourceSwitchedList;

	Ngap_PDUSessionResourceSwitchedList_t  *PDUSessionResourceSwitchedList = &(ie->value.choice.PDUSessionResourceSwitchedList);
	Ngap_PDUSessionResourceSwitchedItem_t *item = NULL;

	for (int iPdu = 0; iPdu < MAX_PDU_SESSION_SIZE; iPdu++)
	{

		if (sess->vecPduSessionID[iPdu] == INVALID_PDU_SESSION_ID)
			continue;
				
		pdu_session_resource_t pdu_resource;
		uint8_t pdu_resource_id = sess->vecPduSessionID[iPdu];
		if (GetPduSessManager().GetPduSessResource(info.sessId, pdu_resource_id, pdu_resource) == false)
		{
			agc_log_printf(AGC_LOG, AGC_LOG_WARNING, "EsmManager::SendPathSwitchReqAck  fail to get pdu session, sessId=%d pdu_resource_id=%d\n", info.sessId, pdu_resource_id);
			continue;
		}
		
		
		uint32_t teid;
		uint32_t teid_v6;
		agc_std_sockaddr_t addr_v6;
		agc_std_sockaddr_t addr;
		
		if (GetPduSessManager().GetGnbGtpInfo(info.sessId, pdu_resource_id, addr, teid, addr_v6, teid_v6) == false)
		{
			agc_log_printf(AGC_LOG, AGC_LOG_WARNING, "EsmManager::SendPathSwitchReqAck  fail to GetGnbGtpInfo, sessId=%d pdu_resource_id=%d\n", info.sessId, pdu_resource_id);
			continue;
		}		
		Ngap_PathSwitchRequestAcknowledgeTransfer_t *pContainer = (Ngap_PathSwitchRequestAcknowledgeTransfer_t *)calloc(1, sizeof(Ngap_PathSwitchRequestAcknowledgeTransfer_t));
		
		item = (Ngap_PDUSessionResourceSwitchedItem_t *)calloc(1, sizeof(Ngap_PDUSessionResourceSwitchedItem_t));
		item->pDUSessionID = pdu_resource.pdu_session_id;
		ASN_SEQUENCE_ADD(&PDUSessionResourceSwitchedList->list, item);
		
		pContainer->uL_NGU_UP_TNLInformation = (Ngap_UPTransportLayerInformation_t *)calloc(1, sizeof(Ngap_UPTransportLayerInformation_t));
		
		Ngap_UPTransportLayerInformation_t	 *UPTransportLayerInformation = pContainer->uL_NGU_UP_TNLInformation;
		UPTransportLayerInformation->present = Ngap_UPTransportLayerInformation_PR_gTPTunnel;
		Ngap_GTPTunnel_t *gTPTunnel = UPTransportLayerInformation->choice.gTPTunnel = (Ngap_GTPTunnel_t *)calloc(1, sizeof(Ngap_GTPTunnel_t));

	    gTPTunnel->gTP_TEID.buf = (uint8_t *)calloc(4, sizeof(uint8_t));
	    gTPTunnel->gTP_TEID.size = 4;
		gTPTunnel->gTP_TEID.buf[0] = teid >> 24;
		gTPTunnel->gTP_TEID.buf[1] = teid >> 16;
		gTPTunnel->gTP_TEID.buf[2] = teid >> 8;
		gTPTunnel->gTP_TEID.buf[3] = teid ;
		
		if (addr.ss_family == AF_INET && addr_v6.ss_family != AF_INET6)  // only ipv4
		{
			struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&addr;
		    gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(4, sizeof(uint8_t));
			memcpy(gTPTunnel->transportLayerAddress.buf, &local_addr1_v4->sin_addr, 4);
			gTPTunnel->transportLayerAddress.size = 4;
		}
		else if (addr.ss_family != AF_INET && addr_v6.ss_family == AF_INET6) // only ipv6
		{
			struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&addr_v6;
		    gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(16, sizeof(uint8_t));
			memcpy(gTPTunnel->transportLayerAddress.buf, &local_addr1_v6->sin6_addr, 16);
			gTPTunnel->transportLayerAddress.size = 16;
		}
		else if (addr.ss_family == AF_INET && addr_v6.ss_family == AF_INET6)  // ipv4 & ipv6
		{
			struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&addr;
			struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&addr_v6;
			gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(20, sizeof(uint8_t));
			memcpy(gTPTunnel->transportLayerAddress.buf, &local_addr1_v4->sin_addr, 4);			
			memcpy(gTPTunnel->transportLayerAddress.buf+4, &local_addr1_v6->sin6_addr, 16);
			gTPTunnel->transportLayerAddress.size = 20;
		}
		else
		{
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"SendPathSwithReqAck :GetGnbGtpInfo IPv4 and IPv6 both are invalid\n");
			gTPTunnel->transportLayerAddress.size = 0;
		}
		
		NgapProtocol ngapEncode;
		int32_t len = 0;
		item->pathSwitchRequestAcknowledgeTransfer.buf = (uint8_t *)calloc(200, sizeof(uint8_t));
		if (ngapEncode.EncodePathSwtichReqAckTransfer((void *)pContainer, (char*)item->pathSwitchRequestAcknowledgeTransfer.buf, (int32_t *)&len) == false)
		{
	    	agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "EsmManager::SendPathSwitchReqAck fail to EncodeResourceSetupRequestTransfer, buf=%p len=%d\n", item->pathSwitchRequestAcknowledgeTransfer.buf, len);
		}
		
		ngapEncode.FreePatchSwitchReqAckTransfer((void *)pContainer);
		item->pathSwitchRequestAcknowledgeTransfer.size = len;
	}

	
	GetNsmManager().SendDownLayerSctp(ngapInfo);

	return true;
}


//lyb,2020-5-19
bool EsmManager::SendDLRanStatusTsfer(NgapMessage& info, Ngap_RANStatusTransfer_TransparentContainer_t	*RANStatusTransfer_TransparentContainer)
{
	NgapMessage ngapInfo;
	Ngap_InitiatingMessage_t	*initiatingMessage = NULL;
	Ngap_DownlinkRANStatusTransfer_t	 *DownlinkRANStatusTransfer = NULL;
	Ngap_DownlinkRANStatusTransferIEs_t  *ie = NULL;

	//Prepare info of Msg.
	ngapInfo.amfUeId = info.amfUeId;
	ngapInfo.sessId = info.sessId;
	ngapInfo.gnbUeId = info.gnbUeId;
	ngapInfo.amfId = info.amfId;
	ngapInfo.ranNodeId = info.ranNodeId;
	ngapInfo.sockTarget = info.sockTarget;
	
	ngapInfo.PDUChoice = Ngap_NGAP_PDU_PR_initiatingMessage;
	ngapInfo.ProcCode = Ngap_ProcedureCode_id_DownlinkRANStatusTransfer;
	
	memset(&ngapInfo.ngapMessage, 0, sizeof(Ngap_NGAP_PDU_t));
	
	ngapInfo.ngapMessage.present = Ngap_NGAP_PDU_PR_initiatingMessage;
	ngapInfo.ngapMessage.choice.initiatingMessage = (Ngap_InitiatingMessage_t *)calloc(1, sizeof(Ngap_InitiatingMessage_t));
	
	initiatingMessage = ngapInfo.ngapMessage.choice.initiatingMessage;
	
	initiatingMessage->procedureCode = Ngap_ProcedureCode_id_DownlinkRANStatusTransfer;
	initiatingMessage->criticality = Ngap_Criticality_reject;
	initiatingMessage->value.present = Ngap_InitiatingMessage__value_PR_DownlinkRANStatusTransfer;

	DownlinkRANStatusTransfer = &initiatingMessage->value.choice.DownlinkRANStatusTransfer;


/*
union Ngap_DownlinkRANStatusTransferIEs__Ngap_value_u {
	Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
	Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
	Ngap_RANStatusTransfer_TransparentContainer_t	 RANStatusTransfer_TransparentContainer;
} choice;
*/
	
//Start Make IE 

//AMF_UE_NGAP_ID
	ie = (Ngap_DownlinkRANStatusTransferIEs_t *)calloc(1, sizeof(Ngap_DownlinkRANStatusTransferIEs_t));
	ASN_SEQUENCE_ADD(&DownlinkRANStatusTransfer->protocolIEs, ie);
	
	ie->id = Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
	ie->criticality = Ngap_Criticality_reject;
	ie->value.present = Ngap_DownlinkRANStatusTransferIEs__value_PR_AMF_UE_NGAP_ID;
	asn_int642INTEGER(&ie->value.choice.AMF_UE_NGAP_ID, info.amfUeId);

//RAN_UE_NGAP_ID
	ie = (Ngap_DownlinkRANStatusTransferIEs_t *)calloc(1, sizeof(Ngap_DownlinkRANStatusTransferIEs_t));
	ASN_SEQUENCE_ADD(&DownlinkRANStatusTransfer->protocolIEs, ie);
	
	ie->id = Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
	ie->criticality = Ngap_Criticality_reject;
	ie->value.present = Ngap_DownlinkRANStatusTransferIEs__value_PR_RAN_UE_NGAP_ID;
	ie->value.choice.RAN_UE_NGAP_ID = ngapInfo.gnbUeId;

//RANStatusTransfer_TransparentContainer
	ie = (Ngap_DownlinkRANStatusTransferIEs_t *)calloc(1, sizeof(Ngap_DownlinkRANStatusTransferIEs_t));
	ASN_SEQUENCE_ADD(&DownlinkRANStatusTransfer->protocolIEs, ie);

	ie->id = Ngap_ProtocolIE_ID_id_RANStatusTransfer_TransparentContainer;
	ie->criticality = Ngap_Criticality_reject;
	ie->value.present = Ngap_DownlinkRANStatusTransferIEs__value_PR_RANStatusTransfer_TransparentContainer;

	for (int i = 0; i < RANStatusTransfer_TransparentContainer->dRBsSubjectToStatusTransferList.list.count; i++)
	{
		Ngap_DRBsSubjectToStatusTransferItem_t *itemUl = RANStatusTransfer_TransparentContainer->dRBsSubjectToStatusTransferList.list.array[i];		
		Ngap_DRBsSubjectToStatusTransferItem_t *itemDl = (Ngap_DRBsSubjectToStatusTransferItem_t *)calloc(1, sizeof(Ngap_DRBsSubjectToStatusTransferItem_t));
	
		itemDl->dRB_ID = itemUl->dRB_ID;

		if (itemUl->dRBStatusUL.present == Ngap_DRBStatusUL_PR_dRBStatusUL12)
		{
			itemDl->dRBStatusUL.present = Ngap_DRBStatusUL_PR_dRBStatusUL12;
			itemDl->dRBStatusUL.choice.dRBStatusUL12 = (Ngap_DRBStatusUL12_t *)calloc(1, sizeof(Ngap_DRBStatusUL12_t));
			itemDl->dRBStatusUL.choice.dRBStatusUL12->uL_COUNTValue.hFN_PDCP_SN12 = itemUl->dRBStatusUL.choice.dRBStatusUL12->uL_COUNTValue.hFN_PDCP_SN12;
			itemDl->dRBStatusUL.choice.dRBStatusUL12->uL_COUNTValue.pDCP_SN12 = itemUl->dRBStatusUL.choice.dRBStatusUL12->uL_COUNTValue.pDCP_SN12;
		}
		else if (itemUl->dRBStatusUL.present == Ngap_DRBStatusUL_PR_dRBStatusUL18)
		{
			itemDl->dRBStatusUL.present = Ngap_DRBStatusUL_PR_dRBStatusUL18;
			itemDl->dRBStatusUL.choice.dRBStatusUL18 = (Ngap_DRBStatusUL18_t *)calloc(1, sizeof(Ngap_DRBStatusUL18_t));
			itemDl->dRBStatusUL.choice.dRBStatusUL18->uL_COUNTValue.pDCP_SN18 = itemUl->dRBStatusUL.choice.dRBStatusUL18->uL_COUNTValue.pDCP_SN18;
			itemDl->dRBStatusUL.choice.dRBStatusUL18->uL_COUNTValue.hFN_PDCP_SN18 = itemUl->dRBStatusUL.choice.dRBStatusUL18->uL_COUNTValue.hFN_PDCP_SN18;
		}
		
		if (itemUl->dRBStatusDL.present == Ngap_DRBStatusDL_PR_dRBStatusDL12)
		{
			itemDl->dRBStatusDL.present = Ngap_DRBStatusDL_PR_dRBStatusDL12;
			itemDl->dRBStatusDL.choice.dRBStatusDL12 = (Ngap_DRBStatusDL12_t *)calloc(1, sizeof(Ngap_DRBStatusDL12_t));
			itemDl->dRBStatusDL.choice.dRBStatusDL12->dL_COUNTValue.hFN_PDCP_SN12 = itemUl->dRBStatusDL.choice.dRBStatusDL12->dL_COUNTValue.hFN_PDCP_SN12;
			itemDl->dRBStatusDL.choice.dRBStatusDL12->dL_COUNTValue.pDCP_SN12 = itemUl->dRBStatusDL.choice.dRBStatusDL12->dL_COUNTValue.pDCP_SN12;
		}
		else if (itemUl->dRBStatusDL.present == Ngap_DRBStatusDL_PR_dRBStatusDL18)
		{
			itemDl->dRBStatusDL.present = Ngap_DRBStatusDL_PR_dRBStatusDL18;
			itemDl->dRBStatusDL.choice.dRBStatusDL18 = (Ngap_DRBStatusDL18_t *)calloc(1, sizeof(Ngap_DRBStatusDL18_t));
			itemDl->dRBStatusDL.choice.dRBStatusDL18->dL_COUNTValue.pDCP_SN18 = itemUl->dRBStatusDL.choice.dRBStatusDL18->dL_COUNTValue.pDCP_SN18;
			itemDl->dRBStatusDL.choice.dRBStatusDL18->dL_COUNTValue.hFN_PDCP_SN18 = itemUl->dRBStatusDL.choice.dRBStatusDL18->dL_COUNTValue.hFN_PDCP_SN18;
		}
		
		ASN_SEQUENCE_ADD(&ie->value.choice.RANStatusTransfer_TransparentContainer.dRBsSubjectToStatusTransferList.list, itemDl);
	}
//All IE Done
	GetNsmManager().SendDownLayerSctp(ngapInfo);
	return true;

}



//lyb,2020-5-12
bool EsmManager::SendToAllRAN(NgapMessage& info)
{
	for (int i = 0; i < MAX_ESM_SESSIONS; i++)
	{
		EsmContext *esmContext = esm_list[i];
		
		if (esmContext == NULL)
		{
			continue;
		}

		info.sockTarget = esmContext->sctp_index;
		GetNsmManager().SendDownLayerSctp(info);
	}
	 
	return true;

}


//lyb,2020-5-14 
e_Ralation_RAN_GW EsmManager::CheckRANinGW(NgapMessage& info, RanNodeId& ranid)
{
	for (int i = 0; i < MAX_ESM_SESSIONS; i++)
	{
		EsmContext *esmContext = esm_list[i];
		
		if (esmContext == NULL)
		{
			continue;
		}

	    if (ranid == esmContext->ranNodeId)
	    {
			//prepare info for send Handover-Req to target RAN
			info.gnbUeId = INVALIDE_ID;
			info.ranNodeId = esmContext->ranNodeId;
//			info.sockTarget = esmContext->sctp_index;
			return Ralation_RAN_Handover_IN_SAME_GW;
		}
	}
	
	return Ralation_RAN_Handover_NOT_IN_SAME_GW;
}


