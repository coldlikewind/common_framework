
#include "NgapProtocol.h"
#include "agc_log.h"


bool NgapProtocol::DecodeResourceSetupRequestTransfer(const char* buf, int32_t *len, void** ie) 
{
    asn_dec_rval_t dec_ret = {RC_OK};
	Ngap_PDUSessionResourceSetupRequestTransfer_t* pRequetTransfer = NULL;

    dec_ret = aper_decode(NULL, &asn_DEF_Ngap_PDUSessionResourceSetupRequestTransfer, 
    	(void **)&pRequetTransfer, (char*)buf, *len, 0, 0);
    if (dec_ret.code != RC_OK) 
    {
        return false;
    }
	
    //asn_fprint(stdout, &asn_DEF_Ngap_PDUSessionResourceSetupRequestTransfer, pRequetTransfer);
	*ie = pRequetTransfer;
	return true;
}

bool NgapProtocol::EncodeResourceSetupRequestTransfer(void* ie,  char* buf, int32_t *len)
{
    //asn_fprint(stdout, &asn_DEF_Ngap_PDUSessionResourceSetupRequestTransfer, ie);
    asn_enc_rval_t enc_ret = {0};
    enc_ret = aper_encode_to_buffer(&asn_DEF_Ngap_PDUSessionResourceSetupRequestTransfer, NULL, 
    	ie, (char*)buf, MAX_BUFF_LENGTH);

    if (enc_ret.encoded < 0)
    {
        return false;
    }
    *len = (enc_ret.encoded >> 3);
	return true;
}


bool NgapProtocol::DecodeResourceSetupResponseTransfer(const char* buf, int32_t *len, void** ie)
{
    asn_dec_rval_t dec_ret = {RC_OK};
	Ngap_PDUSessionResourceSetupResponseTransfer_t* pResponseTransfer = NULL;

    dec_ret = aper_decode(NULL, &asn_DEF_Ngap_PDUSessionResourceSetupResponseTransfer, 
    	(void **)&pResponseTransfer, (char*)buf, *len, 0, 0);
    if (dec_ret.code != RC_OK) 
    {
        return false;
    }
	
    //asn_fprint(stdout, &asn_DEF_Ngap_PDUSessionResourceSetupResponseTransfer, pResponseTransfer);
	*ie = pResponseTransfer;
	return true;
}

bool NgapProtocol::EncodeResourceSetupResponseTransfer(void* ie,	char* buf, int32_t *len)
{
    //asn_fprint(stdout, &asn_DEF_Ngap_PDUSessionResourceSetupResponseTransfer, ie);
    asn_enc_rval_t enc_ret = {0};
    enc_ret = aper_encode_to_buffer(&asn_DEF_Ngap_PDUSessionResourceSetupResponseTransfer, NULL, 
    	ie, (char*)buf, MAX_BUFF_LENGTH);

    if (enc_ret.encoded < 0)
    {
        return false;
    }
    *len = (enc_ret.encoded >> 3);
	return true;
}


bool NgapProtocol::DecodeResourceModifyRequestTransfer(const char* buf, int32_t *len, void** ie) 
{
    asn_dec_rval_t dec_ret = {RC_OK};
	Ngap_PDUSessionResourceModifyRequestTransfer_t* pRequetTransfer = NULL;

    dec_ret = aper_decode(NULL, &asn_DEF_Ngap_PDUSessionResourceModifyRequestTransfer, 
    	(void **)&pRequetTransfer, (char*)buf, *len, 0, 0);
    if (dec_ret.code != RC_OK) 
    {
        return false;
    }
	
    //asn_fprint(stdout, &asn_DEF_Ngap_PDUSessionResourceModifyRequestTransfer, pRequetTransfer);
	*ie = pRequetTransfer;
	return true;
}

bool NgapProtocol::EncodeResourceModifyRequestTransfer(void* ie,  char* buf, int32_t *len)
{
    //asn_fprint(stdout, &asn_DEF_Ngap_PDUSessionResourceModifyRequestTransfer, ie);
    asn_enc_rval_t enc_ret = {0};
    enc_ret = aper_encode_to_buffer(&asn_DEF_Ngap_PDUSessionResourceModifyRequestTransfer, NULL, 
    	ie, (char*)buf, MAX_BUFF_LENGTH);

    if (enc_ret.encoded < 0)
    {
        return false;
    }
    *len = (enc_ret.encoded >> 3);
	return true;
}


bool NgapProtocol::DecodeResourceModifyResponseTransfer(const char* buf, int32_t *len, void** ie)
{
    asn_dec_rval_t dec_ret = {RC_OK};
	Ngap_PDUSessionResourceModifyResponseTransfer_t* pResponseTransfer = NULL;

    dec_ret = aper_decode(NULL, &asn_DEF_Ngap_PDUSessionResourceModifyResponseTransfer, 
    	(void **)&pResponseTransfer, (char*)buf, *len, 0, 0);
    if (dec_ret.code != RC_OK) 
    {
        return false;
    }
	
    //asn_fprint(stdout, &asn_DEF_Ngap_PDUSessionResourceModifyResponseTransfer, pResponseTransfer);
	*ie = pResponseTransfer;
	return true;
}

bool NgapProtocol::EncodeResourceModifyResponseTransfer(void* ie,	char* buf, int32_t *len)
{
    //asn_fprint(stdout, &asn_DEF_Ngap_PDUSessionResourceModifyResponseTransfer, ie);
    asn_enc_rval_t enc_ret = {0};
    enc_ret = aper_encode_to_buffer(&asn_DEF_Ngap_PDUSessionResourceModifyResponseTransfer, NULL, 
    	ie, (char*)buf, MAX_BUFF_LENGTH);

    if (enc_ret.encoded < 0)
    {
        return false;
    }
    *len = (enc_ret.encoded >> 3);
	return true;
}


bool NgapProtocol::DecodeS2TTransparentContainer(const char* buf, int32_t *len, void** ie)
{
    asn_dec_rval_t dec_ret = {RC_OK};
	Ngap_SourceNGRANNode_ToTargetNGRANNode_TransparentContainer *pContainer = NULL;
    dec_ret = aper_decode(NULL, &asn_DEF_Ngap_SourceNGRANNode_ToTargetNGRANNode_TransparentContainer, 
    	(void **)&pContainer, (char*)buf, *len, 0, 0);
    if (dec_ret.code != RC_OK) 
    {
        return false;
    }
	
    //asn_fprint(stdout, &asn_DEF_Ngap_SourceNGRANNode_ToTargetNGRANNode_TransparentContainer, pContainer);
	*ie = pContainer;
	return true;
}

bool NgapProtocol::EncodeS2TTransparentContainer(void* ie,  char* buf, int32_t *len)
{
    //asn_fprint(stdout, &asn_DEF_Ngap_SourceNGRANNode_ToTargetNGRANNode_TransparentContainer, ie);
    asn_enc_rval_t enc_ret = {0};
    enc_ret = aper_encode_to_buffer(&asn_DEF_Ngap_SourceNGRANNode_ToTargetNGRANNode_TransparentContainer, NULL, 
    	ie, (char*)buf, MAX_BUFF_LENGTH);

    if (enc_ret.encoded < 0)
    {
        return false;
    }
    *len = (enc_ret.encoded >> 3);
	return true;
}


bool NgapProtocol::DecodeT2STransparentContainer(const char* buf, int32_t *len, void** ie)
{
    asn_dec_rval_t dec_ret = {RC_OK};
	Ngap_TargetNGRANNode_ToSourceNGRANNode_TransparentContainer *pContainer = NULL;
    dec_ret = aper_decode(NULL, &asn_DEF_Ngap_TargetNGRANNode_ToSourceNGRANNode_TransparentContainer, 
    	(void **)&pContainer, (char*)buf, *len, 0, 0);
    if (dec_ret.code != RC_OK) 
    {
        return false;
    }
	
    //asn_fprint(stdout, &asn_DEF_Ngap_TargetNGRANNode_ToSourceNGRANNode_TransparentContainer, pContainer);
	*ie = pContainer;
	return true;
}

bool NgapProtocol::EncodeT2STransparentContainer(void* ie,  char* buf, int32_t *len)
{
    //asn_fprint(stdout, &asn_DEF_Ngap_TargetNGRANNode_ToSourceNGRANNode_TransparentContainer, ie);
    asn_enc_rval_t enc_ret = {0};
    enc_ret = aper_encode_to_buffer(&asn_DEF_Ngap_TargetNGRANNode_ToSourceNGRANNode_TransparentContainer, NULL, 
    	ie, (char*)buf, MAX_BUFF_LENGTH);

    if (enc_ret.encoded < 0)
    {
        return false;
    }
    *len = (enc_ret.encoded >> 3);
	return true;
}

bool NgapProtocol::DecodeHandoverCommandTransfer(const char* buf, int32_t *len, void** ie)
{
    asn_dec_rval_t dec_ret = {RC_OK};
	Ngap_HandoverCommandTransfer_t *pContainer = NULL;
    dec_ret = aper_decode(NULL, &asn_DEF_Ngap_HandoverCommandTransfer, 
    	(void **)&pContainer, (char*)buf, *len, 0, 0);
    if (dec_ret.code != RC_OK) 
    {
        return false;
    }
	
    //asn_fprint(stdout, &asn_DEF_Ngap_HandoverCommandTransfer, pContainer);
	*ie = pContainer;
	return true;
}

bool NgapProtocol::EncodeHandoverCommandTransfer(void* ie,  char* buf, int32_t *len)
{
    //asn_fprint(stdout, &asn_DEF_Ngap_HandoverCommandTransfer, ie);
    asn_enc_rval_t enc_ret = {0};
    enc_ret = aper_encode_to_buffer(&asn_DEF_Ngap_HandoverCommandTransfer, NULL, 
    	ie, (char*)buf, MAX_BUFF_LENGTH);

    if (enc_ret.encoded < 0)
    {
        return false;
    }
    *len = (enc_ret.encoded >> 3);
	return true;
}

bool NgapProtocol::DecodeHandoverReqAckTransfer(const char* buf, int32_t *len, void** ie)
{
    asn_dec_rval_t dec_ret = {RC_OK};
	Ngap_HandoverRequestAcknowledgeTransfer_t *pContainer = NULL;
    dec_ret = aper_decode(NULL, &asn_DEF_Ngap_HandoverRequestAcknowledgeTransfer, 
    	(void **)&pContainer, (char*)buf, *len, 0, 0);
    if (dec_ret.code != RC_OK) 
    {
        return false;
    }
	
    //asn_fprint(stdout, &asn_DEF_Ngap_HandoverRequestAcknowledgeTransfer, pContainer);
	*ie = pContainer;
	return true;
}

bool NgapProtocol::EncodeHandoverReqAckTransfer(void* ie,  char* buf, int32_t *len)
{
    //asn_fprint(stdout, &asn_DEF_Ngap_HandoverRequestAcknowledgeTransfer, ie);
    asn_enc_rval_t enc_ret = {0};
    enc_ret = aper_encode_to_buffer(&asn_DEF_Ngap_HandoverRequestAcknowledgeTransfer, NULL, 
    	ie, (char*)buf, MAX_BUFF_LENGTH);

    if (enc_ret.encoded < 0)
    {
        return false;
    }
    *len = (enc_ret.encoded >> 3);
	return true;
}

bool NgapProtocol::DecodePathSwtichReqTransfer(const char* buf, int32_t *len, void** ie)
{
    asn_dec_rval_t dec_ret = {RC_OK};
	Ngap_PathSwitchRequestTransfer_t *pContainer = NULL;
    dec_ret = aper_decode(NULL, &asn_DEF_Ngap_PathSwitchRequestTransfer, 
    	(void **)&pContainer, (char*)buf, *len, 0, 0);
    if (dec_ret.code != RC_OK) 
    {
        return false;
    }
	
    //asn_fprint(stdout, &asn_DEF_Ngap_PathSwitchRequestTransfer, pContainer);
	*ie = pContainer;
	return true;
}

bool NgapProtocol::EncodePathSwtichReqTransfer(void* ie,	char* buf, int32_t *len)
{
   // asn_fprint(stdout, &asn_DEF_Ngap_PathSwitchRequestTransfer, ie);
    asn_enc_rval_t enc_ret = {0};
    enc_ret = aper_encode_to_buffer(&asn_DEF_Ngap_PathSwitchRequestTransfer, NULL, 
    	ie, (char*)buf, MAX_BUFF_LENGTH);

    if (enc_ret.encoded < 0)
    {
        return false;
    }
    *len = (enc_ret.encoded >> 3);
	return true;
}


bool NgapProtocol::DecodePathSwtichReqAckTransfer(const char* buf, int32_t *len, void** ie)
{
    asn_dec_rval_t dec_ret = {RC_OK};
	Ngap_PathSwitchRequestAcknowledgeTransfer_t *pContainer = NULL;
    dec_ret = aper_decode(NULL, &asn_DEF_Ngap_PathSwitchRequestAcknowledgeTransfer, 
    	(void **)&pContainer, (char*)buf, *len, 0, 0);
    if (dec_ret.code != RC_OK) 
    {
        return false;
    }
	
    //asn_fprint(stdout, &asn_DEF_Ngap_PathSwitchRequestAcknowledgeTransfer, pContainer);
	*ie = pContainer;
	return true;
}

bool NgapProtocol::EncodePathSwtichReqAckTransfer(void* ie,	char* buf, int32_t *len)
{
    //asn_fprint(stdout, &asn_DEF_Ngap_PathSwitchRequestAcknowledgeTransfer, ie);
    asn_enc_rval_t enc_ret = {0};
    enc_ret = aper_encode_to_buffer(&asn_DEF_Ngap_PathSwitchRequestAcknowledgeTransfer, NULL, 
    	ie, (char*)buf, MAX_BUFF_LENGTH);

    if (enc_ret.encoded < 0)
    {
        return false;
    }
    *len = (enc_ret.encoded >> 3);
	return true;
}


bool NgapProtocol::InterDecode(const char* buf, int32_t *len,NgapMessage& info)
{
    asn_dec_rval_t dec_ret = {RC_OK};
	NGAP_MESSAGE_T     *ngapMessage = &info.ngapMessage;

    dec_ret = aper_decode(NULL, &asn_DEF_Ngap_NGAP_PDU, 
    	(void **)&ngapMessage, (char*)buf, *len, 0, 0);
    if (dec_ret.code != RC_OK) 
    {
        return false;
    }
	
    //asn_fprint(stdout, &asn_DEF_Ngap_NGAP_PDU, ngapMessage);
	PostActionDecode(info);

	return true;
}

bool NgapProtocol::InterEncode(NgapMessage& info, char* buf, int32_t *len)
{
    //asn_fprint(stdout, &asn_DEF_Ngap_NGAP_PDU, &info.ngapMessage);
    asn_enc_rval_t enc_ret = {0};
    enc_ret = aper_encode_to_buffer(&asn_DEF_Ngap_NGAP_PDU, NULL, 
    	&info.ngapMessage, (char*)buf, MAX_BUFF_LENGTH);

    if (enc_ret.encoded < 0)
    {
    	/* Add error type log */
    	const asn_TYPE_descriptor_s *errType = enc_ret.failed_type;
    	if (errType != NULL){
            agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "InterEncode error: xml_tag=%s, name=%s\n", errType->xml_tag, errType->name);
    	}
        return false;
    }

    *len = (enc_ret.encoded >> 3);

	return true;
}


bool NgapProtocol::PostActionDecode(NgapMessage& info)
{
	info.PDUChoice = (uint16_t)info.ngapMessage.present;
	switch (info.PDUChoice)
	{
	case Ngap_NGAP_PDU_PR_initiatingMessage:
	{
		info.ProcCode = (uint16_t)info.ngapMessage.choice.initiatingMessage->procedureCode;
	}
	case Ngap_NGAP_PDU_PR_successfulOutcome:
	{
		info.ProcCode = (uint16_t)info.ngapMessage.choice.successfulOutcome->procedureCode;
		break;
	}
	case Ngap_NGAP_PDU_PR_unsuccessfulOutcome:
	{
		info.ProcCode = (uint16_t)info.ngapMessage.choice.unsuccessfulOutcome->procedureCode;
	}
	default:
		info.ProcCode = 0;
		break;
	}

	return true;
}

