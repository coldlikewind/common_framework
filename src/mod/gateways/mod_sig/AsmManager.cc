
#include "AsmContext.h"
#include "AsmManager.h"
#include "NsmManager.h"
#include "NgapMessage.h"
#include "PlmnUtil.h"
#include "DbManager.h"
#include "NgapSession.h"
#include "NgapSessManager.h"
#include "PduSessManager.h"

#define AMF_SCTP_TIMER_ID 		90
#define AMF_SCTP_TIMER       	2000
#define ASM_SCTP_TIMER_EVENT_NAME 	"asm_sctp_timer_event"

AsmManager::AsmManager()
{
	agc_memory_pool_t *module_pool = agcsig_get_sig_memory_pool();
	agc_mutex_init(&m_mutex, AGC_MUTEX_NESTED, module_pool);
}

AsmManager::~AsmManager()
{
	/*for(std::map<uint32_t,AsmContext*>::iterator it = m_amfContextMap.begin(); it!=m_amfContextMap.end(); ++it)
	{
		if (it->second != NULL)
		{
			delete it->second;
			it->second = NULL;
		}
	}*/
}

bool AsmManager::Init()
{
	memset(amf_sctp_list, 0 , MAX_AMF_SESSIONS * sizeof(AsmContext *));
	memset(amf_list, 0 , MAX_AMF_SESSIONS * sizeof(AsmContext *));
	return true;
}

bool AsmManager::Clear(uint32_t amfid)
{
	AsmContext *asmContext = NULL;

	if (amfid >= MAX_AMF_SESSIONS)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AsmManager::Clear amfid=%d exceed MAX_AMF_SESSIONS.\n", amfid);		
		return false;
	}
	
	asmContext = amf_list[amfid];
	asmContext->isNgSetup = false;
	CheckSctpToAmf(asmContext);
	return true;
}

bool AsmManager::Restart(uint32_t sctp_index)
{
	AsmContext *asmContext = NULL;

	if (sctp_index >= MAX_AMF_SESSIONS)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AsmManager::Restart sctp_index=%d exceed MAX_AMF_SESSIONS.\n", sctp_index);		
		return false;
	}
	
	asmContext = amf_sctp_list[sctp_index];
	asmContext->isNgSetup = false;
	CheckSctpToAmf(asmContext);

	GetNgapSessManager().DeleteAllAmfSession(asmContext->amfId);

	return true;
}


void AsmManager::Close()
{
}

bool AsmManager::AddAmf(uint8_t idSigGW, uint32_t sctp_index, uint8_t priority)
{
	static uint32_t amfid = 1;
	AsmContext *asmContext = NULL;

	if (sctp_index >= MAX_AMF_SESSIONS)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AsmManager::AddAmf sctp_index=%d exceed MAX_AMF_SESSIONS.\n", sctp_index);		
		return false;
	}

	if (amf_sctp_list[sctp_index] != NULL)
	{
		return true;
	}
	
	asmContext = new AsmContext;
	asmContext->amfId = amfid++;
	asmContext->idSigGW = idSigGW;
	asmContext->isNgSetup = false;
	asmContext->ConnTimes = 0;
	asmContext->relativeCapacity = 0;
	asmContext->sctp_index = sctp_index;
	asmContext->priority = priority;

	amf_sctp_list[sctp_index]    = asmContext;
	amf_list[asmContext->amfId]  = asmContext;

	AddTimer(asmContext, AMF_SCTP_TIMER);

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "AsmManager::AddAmf amfId=%d idSigGW=%d sock=%d priority=%d.\n", 
		asmContext->amfId, idSigGW, sctp_index, priority);
}

uint32_t AsmManager::SelectAmf(uint8_t idSigGW)
{
	uint32_t amfid = 0;
	AsmContext *asmContext = NULL;

	for (int i = 0; i < MAX_AMF_SESSIONS; i++)
	{
		asmContext = amf_sctp_list[i];
		if (asmContext != NULL && asmContext->idSigGW == idSigGW && asmContext->isNgSetup == true)
			amfid = asmContext->amfId;
	}
		
	return amfid;
}

uint32_t AsmManager::GetAmfIdBySock(uint32_t sctp_index)
{
	uint32_t amfid = 0;
	AsmContext *asmContext = NULL;

	if (sctp_index >= MAX_AMF_SESSIONS)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AsmManager::GetAmfIdBySock sctp_index=%d exceed MAX_AMF_SESSIONS.\n", sctp_index);		
		return 0;
	}

	asmContext = amf_sctp_list[sctp_index];
	if (asmContext != NULL)
		amfid = asmContext->amfId;
	
	return amfid;
}

uint32_t AsmManager::GetTargetSock(uint32_t amfid)
{
	uint32_t sctp_index = 0;
	AsmContext *asmContext = NULL;

	if (amfid >= MAX_AMF_SESSIONS)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AsmManager::GetAmfIdBySock amfid=%d exceed MAX_AMF_SESSIONS.\n", amfid);		
		return 0;
	}
	
	asmContext = amf_list[amfid];
	if (asmContext != NULL)
		sctp_index = asmContext->sctp_index;

	return sctp_index;
}
/*
bool AsmManager::UpdateSctpInfo(uint32_t amfid, uint32_t stream_no)
{
	agc_mutex_lock(m_mutex);
	if (m_amfContextMap.find(amfid) == m_amfContextMap.end())
	{
		agc_mutex_unlock(m_mutex);
		return false;
	}
	m_amfContextMap[amfid]->stream_no = stream_no;
	agc_mutex_unlock(m_mutex);
	return true;
}
*/
void AsmManager::AddTimer(AsmContext *context,uint32_t TimeOut)
{
	agc_event_t *new_event = NULL;
	if (agc_event_create_callback(&new_event, EVENT_NULL_SOURCEID, (void *)context, &AsmManager::OnTimer) != AGC_STATUS_SUCCESS) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AsmManager::AddTimer create timer agc_event_create [fail].\n");
		return;
	} 

	agc_timer_add_timer(new_event, TimeOut);

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "AsmManager::AddTimer add new timer %d\n", context->amfId);
}

void AsmManager::DelTimer(uint32_t amfId)
{
}

void AsmManager::OnTimer(void *data)
{
	AsmContext *context = (AsmContext *)data;
	GetAsmManager().CheckSctpToAmf(context);

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "AsmManager::OnTimer timer %d\n", context->amfId);
}

void AsmManager::CheckSctpToAmf(AsmContext *context)
{
	uint8_t idSigGW = 0;
	uint32_t sctp_index = 0;	

	if (context != NULL && context->isNgSetup == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "AsmManager::CheckSctpToAmf amfId %d context=%p\n", context->amfId, context);
		
		bool isNgSetup =   context->isNgSetup;
		sctp_index = context->sctp_index;
		idSigGW = context->idSigGW;
		
		NgapMessage info;
		info.amfUeId = 0;
		info.gnbUeId = 0;
		info.amfId = context->amfId;
		info.sockSource = sctp_index;
		info.sockTarget = sctp_index;
		info.idSigGW = idSigGW;
		SendNgSetup(info);
		AddTimer(context, AMF_SCTP_TIMER);
	}
	
	return;
}
	
bool AsmManager::ConnectAmf(uint32_t amfId)
{

    return true;   
}

bool AsmManager::SendResetAck(NgapMessage& info)
{
    Ngap_SuccessfulOutcome_t	*successfulOutcome = NULL;
	Ngap_NGResetAcknowledge_t	*NGResetAcknowledge = NULL;
	NgapMessage infoamf;
	
	infoamf.amfUeId = 0;
	infoamf.gnbUeId = 0;
	infoamf.amfId = info.amfId;
	infoamf.sockSource = info.sockSource;
	infoamf.sockTarget = info.sockSource;
	infoamf.idSigGW = info.idSigGW;
	
	infoamf.PDUChoice = Ngap_NGAP_PDU_PR_successfulOutcome;
	infoamf.ProcCode = Ngap_ProcedureCode_id_NGReset;
	
    memset(&infoamf.ngapMessage, 0, sizeof (Ngap_NGAP_PDU_t));
    infoamf.ngapMessage.present = Ngap_NGAP_PDU_PR_successfulOutcome;
    infoamf.ngapMessage.choice.successfulOutcome = (Ngap_SuccessfulOutcome_t *)calloc(1, sizeof(Ngap_SuccessfulOutcome_t));

    successfulOutcome = infoamf.ngapMessage.choice.successfulOutcome;
    successfulOutcome->procedureCode = Ngap_ProcedureCode_id_NGReset;
    successfulOutcome->criticality = Ngap_Criticality_reject;
    successfulOutcome->value.present = Ngap_SuccessfulOutcome__value_PR_NGResetAcknowledge;
		
	GetNsmManager().SendUpLayerSctp(infoamf);
	return true;
}



bool AsmManager::SendCfgUpdAck(NgapMessage& info)
{
    Ngap_SuccessfulOutcome_t	*successfulOutcome = NULL;
	Ngap_AMFConfigurationUpdateAcknowledge_t	 *AMFConfigurationUpdateAcknowledge = NULL;
	NgapMessage infoamf;
	
	infoamf.amfUeId = 0;
	infoamf.gnbUeId = 0;
	infoamf.amfId = info.amfId;
	infoamf.sockSource = info.sockSource;
	infoamf.sockTarget = info.sockSource;
	infoamf.idSigGW = info.idSigGW;
	
	infoamf.PDUChoice = Ngap_NGAP_PDU_PR_successfulOutcome;
	infoamf.ProcCode = Ngap_ProcedureCode_id_AMFConfigurationUpdate;
	
    memset(&infoamf.ngapMessage, 0, sizeof (Ngap_NGAP_PDU_t));
    infoamf.ngapMessage.present = Ngap_NGAP_PDU_PR_successfulOutcome;
    infoamf.ngapMessage.choice.successfulOutcome = (Ngap_SuccessfulOutcome_t *)calloc(1, sizeof(Ngap_SuccessfulOutcome_t));

    successfulOutcome = infoamf.ngapMessage.choice.successfulOutcome;
    successfulOutcome->procedureCode = Ngap_ProcedureCode_id_AMFConfigurationUpdate;
    successfulOutcome->criticality = Ngap_Criticality_reject;
    successfulOutcome->value.present = Ngap_SuccessfulOutcome__value_PR_AMFConfigurationUpdateAcknowledge;

// IEs are optional
//			union Ngap_AMFConfigurationUpdateAcknowledgeIEs__Ngap_value_u {
//			Ngap_AMF_TNLAssociationSetupList_t	 AMF_TNLAssociationSetupList;
//			Ngap_TNLAssociationList_t	 TNLAssociationList;
//			Ngap_CriticalityDiagnostics_t	 CriticalityDiagnostics;
//		} choice;
		
	GetNsmManager().SendUpLayerSctp(infoamf);
	return true;
}


bool AsmManager::SendPathSwithRequest(NgapMessage& info, Ngap_NR_CGI_t *nr_cgi, Ngap_TAI_t *tai)
{
    Ngap_InitiatingMessage_t *initiatingMessage = NULL;
    Ngap_PathSwitchRequest_t *PathSwitchRequest = NULL;
	Ngap_PathSwitchRequestIEs_t *ie = NULL;
	Ngap_RAN_UE_NGAP_ID_t	 *RAN_UE_NGAP_ID = NULL;
	Ngap_AMF_UE_NGAP_ID_t	 *AMF_UE_NGAP_ID = NULL;
	Ngap_UserLocationInformation_t	 *UserLocationInformation = NULL;
	Ngap_PDUSessionResourceToBeSwitchedDLList_t	 *PDUSessionResourceToBeSwitchedDLList = NULL;
	
	NgapMessage infoamf;
	
	NgapSession *sess = NULL;
	if (GetNgapSessManager().GetNgapSession(info.sessId, &sess) == false)
	{
    	agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AsmManager::SendPathSwithRequest fail to get ngap session, sessId=%d\n", info.sessId);
		return false;
	}

	//	union Ngap_PathSwitchRequestIEs__Ngap_value_u {
	//		Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;
	//		Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;
	//		Ngap_UserLocationInformation_t	 UserLocationInformation;
	//		Ngap_UESecurityCapabilities_t	 UESecurityCapabilities;
	//		Ngap_PDUSessionResourceToBeSwitchedDLList_t  PDUSessionResourceToBeSwitchedDLList;
		//		Ngap_PDUSessionResourceFailedToSetupListPSReq_t  PDUSessionResourceFailedToSetupListPSReq;
	//	} choice;
	
	infoamf.amfUeId = info.amfUeId;
	infoamf.gnbUeId = info.sessId;
	infoamf.amfId = info.amfId;
	infoamf.sockSource = info.sockSource;
	infoamf.sockTarget = info.sockTarget;
	infoamf.idSigGW = info.idSigGW;
	
	infoamf.PDUChoice = Ngap_NGAP_PDU_PR_initiatingMessage;
	infoamf.ProcCode = Ngap_ProcedureCode_id_PathSwitchRequest;
	
    memset(&infoamf.ngapMessage, 0, sizeof (Ngap_NGAP_PDU_t));
    infoamf.ngapMessage.present = Ngap_NGAP_PDU_PR_initiatingMessage;
    infoamf.ngapMessage.choice.initiatingMessage = (Ngap_InitiatingMessage_t *)calloc(1, sizeof(Ngap_InitiatingMessage_t));

    initiatingMessage = infoamf.ngapMessage.choice.initiatingMessage;
    initiatingMessage->procedureCode = Ngap_ProcedureCode_id_PathSwitchRequest;
    initiatingMessage->criticality = Ngap_Criticality_reject;
    initiatingMessage->value.present = Ngap_InitiatingMessage__value_PR_PathSwitchRequest;
	
    PathSwitchRequest = &initiatingMessage->value.choice.PathSwitchRequest;

//	1	Ngap_RAN_UE_NGAP_ID_t	 RAN_UE_NGAP_ID;	
    ie = (Ngap_PathSwitchRequestIEs_t *)calloc(1, sizeof(Ngap_PathSwitchRequestIEs_t));
    ASN_SEQUENCE_ADD(&PathSwitchRequest->protocolIEs, ie);

    ie->id = Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
    ie->criticality = Ngap_Criticality_reject;
    ie->value.present = Ngap_PathSwitchRequestIEs__value_PR_RAN_UE_NGAP_ID;
	
    RAN_UE_NGAP_ID = &ie->value.choice.RAN_UE_NGAP_ID;
	*RAN_UE_NGAP_ID = info.sessId;

//	2	Ngap_AMF_UE_NGAP_ID_t	 AMF_UE_NGAP_ID;		
    ie = (Ngap_PathSwitchRequestIEs_t *)calloc(1, sizeof(Ngap_PathSwitchRequestIEs_t));
    ASN_SEQUENCE_ADD(&PathSwitchRequest->protocolIEs, ie);

    ie->id = Ngap_ProtocolIE_ID_id_SourceAMF_UE_NGAP_ID;
    ie->criticality = Ngap_Criticality_reject;
    ie->value.present = Ngap_PathSwitchRequestIEs__value_PR_AMF_UE_NGAP_ID;
	
    AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
	asn_int642INTEGER(AMF_UE_NGAP_ID, (uint64_t)info.amfUeId);
	
//	3	Ngap_UserLocationInformation_t	 UserLocationInformation;	
    ie = (Ngap_PathSwitchRequestIEs_t *)calloc(1, sizeof(Ngap_PathSwitchRequestIEs_t));
    ASN_SEQUENCE_ADD(&PathSwitchRequest->protocolIEs, ie);

    ie->id = Ngap_ProtocolIE_ID_id_UserLocationInformation;
    ie->criticality = Ngap_Criticality_ignore;
    ie->value.present = Ngap_PathSwitchRequestIEs__value_PR_UserLocationInformation;
	
    UserLocationInformation = &ie->value.choice.UserLocationInformation;
	UserLocationInformation->present = Ngap_UserLocationInformation_PR_userLocationInformationNR;
	UserLocationInformation->choice.userLocationInformationNR = (Ngap_UserLocationInformationNR_t *)calloc(1, sizeof(Ngap_UserLocationInformationNR_t));

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "AsmManager::SendPathSwithRequest package info: nr_cgi->nRCellIdentity.size=%d, buf=%p, nr_cgi->pLMNIdentity.size=%d, buf=%p, tAC.size=%d, buf=%p, tai->pLMNIdentity.size=%d,buf=%p\n", 
																nr_cgi->nRCellIdentity.size,
																nr_cgi->nRCellIdentity.buf,
																nr_cgi->pLMNIdentity.size,
																nr_cgi->pLMNIdentity.buf,
																tai->tAC.size,
																tai->tAC.buf,
																tai->pLMNIdentity.size,
																tai->pLMNIdentity.buf);

	
	UserLocationInformation->choice.userLocationInformationNR->nR_CGI.nRCellIdentity.size = nr_cgi->nRCellIdentity.size;
	UserLocationInformation->choice.userLocationInformationNR->nR_CGI.nRCellIdentity.buf = (uint8_t *)calloc(nr_cgi->nRCellIdentity.size, sizeof(uint8_t));
	memcpy(UserLocationInformation->choice.userLocationInformationNR->nR_CGI.nRCellIdentity.buf, nr_cgi->nRCellIdentity.buf, nr_cgi->nRCellIdentity.size);
	UserLocationInformation->choice.userLocationInformationNR->nR_CGI.nRCellIdentity.bits_unused = 4;

	
	
	UserLocationInformation->choice.userLocationInformationNR->nR_CGI.pLMNIdentity.size = nr_cgi->pLMNIdentity.size;
	UserLocationInformation->choice.userLocationInformationNR->nR_CGI.pLMNIdentity.buf = (uint8_t *)calloc(nr_cgi->pLMNIdentity.size, sizeof(uint8_t));
	memcpy(UserLocationInformation->choice.userLocationInformationNR->nR_CGI.pLMNIdentity.buf, nr_cgi->pLMNIdentity.buf, nr_cgi->pLMNIdentity.size);

	UserLocationInformation->choice.userLocationInformationNR->tAI.tAC.size = tai->tAC.size;
	UserLocationInformation->choice.userLocationInformationNR->tAI.tAC.buf = (uint8_t *)calloc(tai->tAC.size, sizeof(uint8_t));
	memcpy(UserLocationInformation->choice.userLocationInformationNR->tAI.tAC.buf, tai->tAC.buf, tai->tAC.size);
	
	UserLocationInformation->choice.userLocationInformationNR->tAI.pLMNIdentity.size = tai->pLMNIdentity.size;
	UserLocationInformation->choice.userLocationInformationNR->tAI.pLMNIdentity.buf = (uint8_t *)calloc(tai->pLMNIdentity.size, sizeof(uint8_t));
	memcpy(UserLocationInformation->choice.userLocationInformationNR->tAI.pLMNIdentity.buf, tai->pLMNIdentity.buf, tai->pLMNIdentity.size);
	
//	4	UESecurityCapabilities	
    ie = (Ngap_PathSwitchRequestIEs_t *)calloc(1, sizeof(Ngap_PathSwitchRequestIEs_t));
    ASN_SEQUENCE_ADD(&PathSwitchRequest->protocolIEs, ie);
	
	ie->id = Ngap_ProtocolIE_ID_id_UESecurityCapabilities;
	ie->criticality = Ngap_Criticality_ignore;
	ie->value.present = Ngap_PathSwitchRequestIEs__value_PR_UESecurityCapabilities;
	
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
	
//	5	Ngap_PDUSessionResourceToBeSwitchedDLList_t  PDUSessionResourceToBeSwitchedDLList;
	ie = (Ngap_PathSwitchRequestIEs_t *)calloc(1, sizeof(Ngap_PathSwitchRequestIEs_t));
    ASN_SEQUENCE_ADD(&PathSwitchRequest->protocolIEs, ie);
	
	ie->id = Ngap_ProtocolIE_ID_id_PDUSessionResourceToBeSwitchedDLList;
	ie->criticality = Ngap_Criticality_reject;
	ie->value.present = Ngap_PathSwitchRequestIEs__value_PR_PDUSessionResourceToBeSwitchedDLList;
	
    PDUSessionResourceToBeSwitchedDLList = &ie->value.choice.PDUSessionResourceToBeSwitchedDLList;
	
	for (int iPdu = 0; iPdu < MAX_PDU_SESSION_SIZE; iPdu++)
	{
		if (sess->vecPduSessionID[iPdu] == INVALID_PDU_SESSION_ID)
			continue;

		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "AsmManager::SendPathSwithRequest sess->vecPduSessionID[%d]=%d\n", iPdu, sess->vecPduSessionID[iPdu]);
		
		pdu_session_resource_t pdu_resource;
		uint8_t	pdu_resource_id = sess->vecPduSessionID[iPdu];
		if (GetPduSessManager().GetPduSessResource(info.sessId, pdu_resource_id, pdu_resource) == false)
		{
	    	agc_log_printf(AGC_LOG, AGC_LOG_WARNING, "AsmManager::SendPathSwithRequest fail to get pdu session, sessId=%d pdu_resource_id=%d\n", info.sessId, pdu_resource_id);
			continue;
		}		

		uint32_t teid;
		uint32_t teid_v6;
		agc_std_sockaddr_t addr_v6;
		agc_std_sockaddr_t addr;		
		if (GetPduSessManager().GetUpfGtpInfo(info.sessId, pdu_resource_id, addr, teid, addr_v6, teid_v6) == false)	
		{
	    	agc_log_printf(AGC_LOG, AGC_LOG_WARNING, "AsmManager::SendPathSwithRequest  fail to GetGnbGtpInfo, sessId=%d pdu_resource_id=%d\n", info.sessId, pdu_resource_id);
			continue;
		}
		
		Ngap_PDUSessionResourceToBeSwitchedDLItem_t *item = (Ngap_PDUSessionResourceToBeSwitchedDLItem_t *)calloc(1, sizeof(Ngap_PDUSessionResourceToBeSwitchedDLItem_t));
		ASN_SEQUENCE_ADD(&PDUSessionResourceToBeSwitchedDLList->list, item);

		item->pDUSessionID = pdu_resource_id;
		
		Ngap_PathSwitchRequestTransfer_t *pContainer = (Ngap_PathSwitchRequestTransfer_t *)calloc(1, sizeof(Ngap_PathSwitchRequestTransfer_t));

		// add qos item
		Ngap_QosFlowAcceptedItem_t *qosItem = (Ngap_QosFlowAcceptedItem_t *)calloc(1, sizeof(Ngap_QosFlowAcceptedItem_t));
		ASN_SEQUENCE_ADD(&pContainer->qosFlowAcceptedList.list, qosItem);

		qosItem->qosFlowIdentifier = pdu_resource.qos_flow_id;

		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "AsmManager::SendPathSwithRequest qosFlowIdentifier=%d\n", qosItem->qosFlowIdentifier);

		//add up information;
		Ngap_UPTransportLayerInformation_t	 *dL_NGU_UP_TNLInformation = &pContainer->dL_NGU_UP_TNLInformation;
		dL_NGU_UP_TNLInformation->present = Ngap_UPTransportLayerInformation_PR_gTPTunnel;
		Ngap_GTPTunnel_t *gTPTunnel = dL_NGU_UP_TNLInformation->choice.gTPTunnel = (Ngap_GTPTunnel_t *)calloc(1, sizeof(Ngap_GTPTunnel_t));

	    gTPTunnel->gTP_TEID.buf = (uint8_t *)calloc(4, sizeof(uint8_t));
	    gTPTunnel->gTP_TEID.size = 4;
		gTPTunnel->gTP_TEID.buf[0] = teid >> 24;
		gTPTunnel->gTP_TEID.buf[1] = teid >> 16;
		gTPTunnel->gTP_TEID.buf[2] = teid >> 8;
		gTPTunnel->gTP_TEID.buf[3] = teid ;

		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "AsmManager::SendPathSwithRequest TEID=%d,%d,%d,%d\n", gTPTunnel->gTP_TEID.buf[0],gTPTunnel->gTP_TEID.buf[1],gTPTunnel->gTP_TEID.buf[2],gTPTunnel->gTP_TEID.buf[3]);
		
		if (addr.ss_family == AF_INET && addr_v6.ss_family == AF_INET6)  // ipv4 & ipv6
		{
			struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&addr;
			struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&addr_v6;
		    gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(20, sizeof(uint8_t));
			memcpy(gTPTunnel->transportLayerAddress.buf, &local_addr1_v4->sin_addr, 4);
			memcpy(gTPTunnel->transportLayerAddress.buf+4, &local_addr1_v6->sin6_addr, 16);
			gTPTunnel->transportLayerAddress.size = 20;
			
			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "AsmManager::SendPathSwithRequest gTPTunnel is ipv4&ipv6 type, IPV4=%d,%d,%d,%d\n", gTPTunnel->transportLayerAddress.buf[0],gTPTunnel->transportLayerAddress.buf[1],gTPTunnel->transportLayerAddress.buf[2],gTPTunnel->transportLayerAddress.buf[3]);
		}
		else if (addr.ss_family != AF_INET && addr_v6.ss_family == AF_INET6) //only ipv6
		{
			struct sockaddr_in6 *local_addr1_v6 = (struct sockaddr_in6 *)&addr_v6;
		    gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(16, sizeof(uint8_t));
			memcpy(gTPTunnel->transportLayerAddress.buf, &local_addr1_v6->sin6_addr, 16);
			gTPTunnel->transportLayerAddress.size = 16;

			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "AsmManager::SendPathSwithRequest only IPV6\n");
		}
		else if(addr.ss_family == AF_INET && addr_v6.ss_family != AF_INET6) //only ipv4
		{
			struct sockaddr_in *local_addr1_v4 = (struct sockaddr_in *)&addr;
		    gTPTunnel->transportLayerAddress.buf = (uint8_t *)calloc(4, sizeof(uint8_t));
			memcpy(gTPTunnel->transportLayerAddress.buf, &local_addr1_v4->sin_addr, 4);
			gTPTunnel->transportLayerAddress.size = 4;
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"AsmManager::SendPathSwithRequest only IPV4\n");
		}
		else
		{
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR,"SendPathSwithRequest :GetUpfGtpInfo IPv4 and IPv6 both are invalid\n");
			gTPTunnel->transportLayerAddress.size = 0;
		}
		
		NgapProtocol ngapEncode;
		int32_t len = 0;
		item->pathSwitchRequestTransfer.buf = (uint8_t *)calloc(200, sizeof(uint8_t));
		if (ngapEncode.EncodePathSwtichReqTransfer((void *)pContainer, (char*)item->pathSwitchRequestTransfer.buf, (int32_t *)&len) == false)
		{
	    	agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AsmManager::SendPathSwitchRequest fail to EncodePathSwtichReqTransfer, buf=%p len=%d\n",
				item->pathSwitchRequestTransfer.buf, len);
		}
		
		len = len + 1;
		
		item->pathSwitchRequestTransfer.size = len;
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "AsmManager::SendPathSwithRequest Item->pathSwitchRequestTransfer.size=%d\n", item->pathSwitchRequestTransfer.size);
		
		ngapEncode.FreePatchSwitchReqTransfer((void *)pContainer);

	}

	GetNsmManager().SendUpLayerSctp(infoamf);
	return true;
}

bool AsmManager::SendNgSetup(NgapMessage& info)
{
    Ngap_InitiatingMessage_t *initiatingMessage = NULL;
    Ngap_NGSetupRequest_t *NgSetupRequest = NULL;
    plmn_id_t plmn_id;

	info.PDUChoice = Ngap_NGAP_PDU_PR_initiatingMessage;
	info.ProcCode = Ngap_ProcedureCode_id_NGSetup;
	
	stCfg_vGWParam_API param;
	std::vector<stCfg_TA_DATA> tacs;

	uint8_t idSigGW = info.idSigGW;
	if (GetDbManager().QueryVgwParam(idSigGW, param) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AsmManager::SendNgSetup fail to get param gatewayid=%d\n",
			idSigGW);
		return false;
	}
	if (GetDbManager().QueryTacs(idSigGW, tacs) == false || tacs.size() <= 0)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AsmManager::SendNgSetup fail to get tacs gatewayid=%d tacs=%d\n",
			idSigGW, tacs.size());
		return false;
	}

    Ngap_NGSetupRequestIEs_t *ie = NULL;
    Ngap_GlobalRANNodeID_t *GlobalRANNodeID = NULL;
	Ngap_RANNodeName_t	 *RANNodeName = NULL;
    Ngap_SupportedTAList_t *SupportedTAs = NULL;
    Ngap_SupportedTAItem_t *SupportedTAs_Item = NULL;
    Ngap_PLMNIdentity_t *PLMNidentity = NULL;
    Ngap_PagingDRX_t *PagingDRX = NULL;
	
    memset(&info.ngapMessage, 0, sizeof (Ngap_NGAP_PDU_t));
    info.ngapMessage.present = Ngap_NGAP_PDU_PR_initiatingMessage;
    info.ngapMessage.choice.initiatingMessage = (Ngap_InitiatingMessage_t *)calloc(1, sizeof(Ngap_InitiatingMessage_t));

    initiatingMessage = info.ngapMessage.choice.initiatingMessage;
    initiatingMessage->procedureCode = Ngap_ProcedureCode_id_NGSetup;
    initiatingMessage->criticality = Ngap_Criticality_reject;
    initiatingMessage->value.present =
        Ngap_InitiatingMessage__value_PR_NGSetupRequest;

    NgSetupRequest = &initiatingMessage->value.choice.NGSetupRequest;

    ie = (Ngap_NGSetupRequestIEs_t *)calloc(1, sizeof(Ngap_NGSetupRequestIEs_t));
    ASN_SEQUENCE_ADD(&NgSetupRequest->protocolIEs, ie);

    ie->id = Ngap_ProtocolIE_ID_id_GlobalRANNodeID;
    ie->criticality = Ngap_Criticality_reject;
    ie->value.present = Ngap_NGSetupRequestIEs__value_PR_GlobalRANNodeID;

    GlobalRANNodeID = &ie->value.choice.GlobalRANNodeID;
	switch (param.gNBType)
	{
		case CFG_GNB:
		{
			Ngap_GlobalGNB_ID_t	*globalGNB_ID = NULL;
			BIT_STRING_t	 *gNB_ID;
			
			globalGNB_ID = (Ngap_GlobalGNB_ID_t *)calloc(1, sizeof(Ngap_GlobalGNB_ID_t));
			globalGNB_ID->gNB_ID.present = Ngap_GNB_ID_PR_gNB_ID;
			gNB_ID = &globalGNB_ID->gNB_ID.choice.gNB_ID;
			PLMNidentity = &globalGNB_ID->pLMNIdentity;
			gNB_ID->size = 3;
			gNB_ID->buf = (uint8_t *)calloc(4, sizeof(uint8_t));
	        gNB_ID->buf[0] = param.gNBid >> 16;
	        gNB_ID->buf[1] = param.gNBid >> 8;
	        gNB_ID->buf[2] = param.gNBid;
//	        gNB_ID->buf[3] = param.gNBid;
			
			GlobalRANNodeID->present = Ngap_GlobalRANNodeID_PR_globalGNB_ID;
			GlobalRANNodeID->choice.globalGNB_ID = globalGNB_ID;
			break;
		}
		case CFG_NG_ENB:
		{
			Ngap_GlobalNgENB_ID_t	*globalNgENB_ID = NULL;		
			Ngap_NgENB_ID_t	 *ngENB_ID = NULL;	
			
			globalNgENB_ID = (Ngap_GlobalNgENB_ID_t *)calloc(1, sizeof(Ngap_GlobalNgENB_ID_t));
			ngENB_ID = &globalNgENB_ID->ngENB_ID;			
			PLMNidentity = &globalNgENB_ID->pLMNIdentity;
			
			GlobalRANNodeID->present = Ngap_GlobalRANNodeID_PR_globalNgENB_ID;
			GlobalRANNodeID->choice.globalNgENB_ID = globalNgENB_ID;
			switch (param.ngeNBType)
			{
			case CFG_NGE_MACRO:
			{
				ngENB_ID->present = Ngap_NgENB_ID_PR_macroNgENB_ID;
				ngENB_ID->choice.macroNgENB_ID.size = 3;
				ngENB_ID->choice.macroNgENB_ID.buf = (uint8_t *)calloc(ngENB_ID->choice.macroNgENB_ID.size, sizeof(uint8_t));
		        ngENB_ID->choice.macroNgENB_ID.buf[0] = param.gNBid >> 12;
		        ngENB_ID->choice.macroNgENB_ID.buf[1] = param.gNBid >> 4;
		        ngENB_ID->choice.macroNgENB_ID.buf[2] = (param.gNBid & 0xf) << 4;
         		ngENB_ID->choice.macroNgENB_ID.bits_unused = 4;
				break;
			}
			case CFG_NGE_SHORT_MACRO:
			{
				ngENB_ID->present = Ngap_NgENB_ID_PR_shortMacroNgENB_ID;
				ngENB_ID->choice.macroNgENB_ID.size = 3;
				ngENB_ID->choice.macroNgENB_ID.buf = (uint8_t *)calloc(ngENB_ID->choice.macroNgENB_ID.size, sizeof(uint8_t));
		        ngENB_ID->choice.macroNgENB_ID.buf[0] = param.gNBid >> 10;
		        ngENB_ID->choice.macroNgENB_ID.buf[1] = param.gNBid >> 2;
		        ngENB_ID->choice.macroNgENB_ID.buf[2] = (param.gNBid & 0x3) << 2;
         		ngENB_ID->choice.macroNgENB_ID.bits_unused = 6;
				break;
			}
			case CFG_NGE_LONG_MACRO:
			{
				ngENB_ID->present = Ngap_NgENB_ID_PR_longMacroNgENB_ID;
				ngENB_ID->choice.macroNgENB_ID.size = 3;
				ngENB_ID->choice.macroNgENB_ID.buf = (uint8_t *)calloc(ngENB_ID->choice.macroNgENB_ID.size, sizeof(uint8_t));
		        ngENB_ID->choice.macroNgENB_ID.buf[0] = param.gNBid >> 13;
		        ngENB_ID->choice.macroNgENB_ID.buf[1] = param.gNBid >> 5;
		        ngENB_ID->choice.macroNgENB_ID.buf[2] = (param.gNBid & 0x1f) << 5;
         		ngENB_ID->choice.macroNgENB_ID.bits_unused = 4;
				break;
			}
			default:
				ngENB_ID->present = Ngap_NgENB_ID_PR_NOTHING;
				break;
			}
			
			break;
		}
		case CFG_N3IWF:
			GlobalRANNodeID->present = Ngap_GlobalRANNodeID_PR_globalN3IWF_ID;
			break;
		default:
			GlobalRANNodeID->present = Ngap_GlobalRANNodeID_PR_NOTHING;
			break;
	}
	
	//plmn_id_build(&plmn_id, param.Plmn.MCC, param.Plmn.MNC, PLMN_MNC_ID_LEN);
	plmn_id.mcc1 = param.Plmn.mcc1;
	plmn_id.mcc2 = param.Plmn.mcc2;
	plmn_id.mcc3 = param.Plmn.mcc3;
	plmn_id.mnc1 = param.Plmn.mnc1;
	plmn_id.mnc2 = param.Plmn.mnc2;
	plmn_id.mnc3 = param.Plmn.mnc3;

	if (PLMNidentity)
	{
	    PLMNidentity->size = PLMN_ID_LEN;
	    PLMNidentity->buf = (uint8_t *)calloc(PLMNidentity->size, sizeof(uint8_t));
    	memcpy(PLMNidentity->buf, &plmn_id, PLMN_ID_LEN);
	}

	if (strlen(param.gNBName) > 0)
	{
	    ie = (Ngap_NGSetupRequestIEs_t *)calloc(1, sizeof(Ngap_NGSetupRequestIEs_t));
	    ASN_SEQUENCE_ADD(&NgSetupRequest->protocolIEs, ie);

	    ie->id = Ngap_ProtocolIE_ID_id_RANNodeName;
	    ie->criticality = Ngap_Criticality_ignore;
	    ie->value.present = Ngap_NGSetupRequestIEs__value_PR_RANNodeName;

	    RANNodeName = &ie->value.choice.RANNodeName;
		RANNodeName->size = strlen(param.gNBName);
		RANNodeName->buf = (uint8_t *)calloc(RANNodeName->size, sizeof(uint8_t));
		memcpy(RANNodeName->buf, param.gNBName, strlen(param.gNBName));
	}
	
    ie = (Ngap_NGSetupRequestIEs_t *)calloc(1, sizeof(Ngap_NGSetupRequestIEs_t));
    ASN_SEQUENCE_ADD(&NgSetupRequest->protocolIEs, ie);

    ie->id = Ngap_ProtocolIE_ID_id_SupportedTAList;
    ie->criticality = Ngap_Criticality_reject;
    ie->value.present = Ngap_NGSetupRequestIEs__value_PR_SupportedTAList;

    SupportedTAs = &ie->value.choice.SupportedTAList;

    ie = (Ngap_NGSetupRequestIEs_t *)calloc(1, sizeof(Ngap_NGSetupRequestIEs_t));
    ASN_SEQUENCE_ADD(&NgSetupRequest->protocolIEs, ie);
    
    ie->id = Ngap_ProtocolIE_ID_id_DefaultPagingDRX;
    ie->criticality = Ngap_Criticality_ignore;
    ie->value.present = Ngap_NGSetupRequestIEs__value_PR_PagingDRX;

    PagingDRX = &ie->value.choice.PagingDRX;
	*PagingDRX = param.DRX;

    for (uint8_t i = 0; i < tacs.size(); i++)
    {
		std::vector<stCfg_TA_PLMN_DATA> plmns;
		if (GetDbManager().QueryTacPlmn(idSigGW, tacs[i].idTA, plmns) == false || plmns.size() <= 0)
		{
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AsmManager::SendNgSetup fail to QueryTacPlmn idBplmn=%d  plmns=%d\n", tacs[i].idTA, plmns.size());
			return false;
		}
		
		SupportedTAs_Item = (Ngap_SupportedTAItem_t *) calloc(1, sizeof(Ngap_SupportedTAItem_t));		
	    SupportedTAs_Item->tAC.size = 3;
	    SupportedTAs_Item->tAC.buf = (uint8_t *)calloc(SupportedTAs_Item->tAC.size, sizeof(uint8_t));
	    SupportedTAs_Item->tAC.buf[0] = tacs[i].TAC >> 16;
	    SupportedTAs_Item->tAC.buf[1] = tacs[i].TAC >> 8;
	    SupportedTAs_Item->tAC.buf[2] = tacs[i].TAC;
	
		for (uint8_t j = 0; j < plmns.size(); j++)
		{
			std::vector<stCfg_PLMN_NSSAI_DATA> nnsais;
			if (GetDbManager().QueryPlmnNssai(idSigGW, plmns[j].idBplmn, nnsais) == false || nnsais.size() <= 0)  {
				agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AsmManager::SendNgSetup fail to get PlmnNssai idBplmn=%d nnsais=%d\n", plmns[j].idBplmn, nnsais.size());
				return false;
			}
			
			Ngap_BroadcastPLMNItem_t *broadcastPLMN = (Ngap_BroadcastPLMNItem_t *) calloc(1, sizeof(Ngap_BroadcastPLMNItem_t));
			PLMNidentity = &broadcastPLMN->pLMNIdentity;
		    PLMNidentity->size = PLMN_ID_LEN;
		    PLMNidentity->buf = (uint8_t *)calloc(PLMNidentity->size, sizeof(uint8_t));
			//plmn_id_build(&plmn_id, plmns[j].MCC,  plmns[j].MNC, PLMN_MNC_ID_LEN);
			plmn_id.mcc1 = plmns[j].plmn.mcc1;
			plmn_id.mcc2 = plmns[j].plmn.mcc2;
			plmn_id.mcc3 = plmns[j].plmn.mcc3;
			plmn_id.mnc1 = plmns[j].plmn.mnc1;
			plmn_id.mnc2 = plmns[j].plmn.mnc2;
			plmn_id.mnc3 = plmns[j].plmn.mnc3;
	    	memcpy(PLMNidentity->buf, &plmn_id, PLMN_ID_LEN);				
			
			for (uint8_t k = 0; k < nnsais.size(); k++)
			{
				Ngap_SliceSupportItem_t *item = (Ngap_SliceSupportItem_t *)calloc(1, sizeof(Ngap_SliceSupportItem_t));
				
			    item->s_NSSAI.sST.size = 1;
			    item->s_NSSAI.sST.buf = (uint8_t *)calloc(item->s_NSSAI.sST.size, sizeof(uint8_t));
			    item->s_NSSAI.sST.buf[0] = nnsais[k].SST;
				/*
				item->s_NSSAI.sD = (Ngap_SD_t *)calloc(1, sizeof(Ngap_SD_t));
			    item->s_NSSAI.sD->size = 3;
			    item->s_NSSAI.sD->buf = (uint8_t *)calloc(item->s_NSSAI.sST.size, sizeof(uint8_t));
			    item->s_NSSAI.sD->buf[0] = nnsais[k].SD >> 16;
			    item->s_NSSAI.sD->buf[1] = nnsais[k].SD >> 8;
			    item->s_NSSAI.sD->buf[2] = nnsais[k].SD;
			    */
				ASN_SEQUENCE_ADD(&broadcastPLMN->tAISliceSupportList.list, item);				
			}
			
			ASN_SEQUENCE_ADD(&SupportedTAs_Item->broadcastPLMNList.list, broadcastPLMN);
		}
		
		
		ASN_SEQUENCE_ADD(&SupportedTAs->list, SupportedTAs_Item);

    }
	
	GetNsmManager().SendUpLayerSctp(info);
	return true;
}


bool AsmManager::handleNgSetupResponse(uint32_t sctp_index, NgapMessage& info)
{
	if (sctp_index >= MAX_AMF_SESSIONS)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AsmManager::handleNgSetupResponse sctp_index=%d exceed MAX_AMF_SESSIONS.\n", sctp_index);		
		return false;
	}
	
	AsmContext *asmcontext = amf_sctp_list[sctp_index];
	if (asmcontext == NULL)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "AsmManager::handleNgSetupResponse sctp_index=%d asmcontext is null.\n", sctp_index);		
		return false;
	}
		
	asmcontext->isNgSetup = true;
	
	Ngap_NGSetupResponse_t *NGSetupResponse = &info.ngapMessage.choice.successfulOutcome->value.choice.NGSetupResponse;
	for (int i = 0; i < NGSetupResponse->protocolIEs.list.count; i++)
	{
		Ngap_AMFName_t	 *AMFName = NULL;
		Ngap_NGSetupResponseIEs_t *ie = NGSetupResponse->protocolIEs.list.array[i];
		switch(ie->id)
		{
			case Ngap_ProtocolIE_ID_id_RelativeAMFCapacity:
				asmcontext->relativeCapacity = ie->value.choice.RelativeAMFCapacity;
				break;
			case Ngap_ProtocolIE_ID_id_AMFName:
				AMFName = &ie->value.choice.AMFName;				
				asmcontext->amfName.assign((const char*)AMFName->buf, AMFName->size);
				break;
			case Ngap_ProtocolIE_ID_id_ServedGUAMIList:
				break;
			case Ngap_ProtocolIE_ID_id_PLMNSupportList:
				break;
		}
	}	
		
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"AsmManager::handleNgSetupResponse setup success amfId=%d sctp_index=%d asmcontext=%p\n", asmcontext->amfId, sctp_index, asmcontext);
	return true;
}
