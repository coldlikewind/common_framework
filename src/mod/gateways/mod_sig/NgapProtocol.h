#pragma once

#include "mod_sig.h"
#include "BaseLayerT.h"
#include "NgapMessage.h"
#include "SingletonT.h"

class NgapProtocol : public BaseLayerT<NgapMessage>
{
public:
	const uint32_t MAX_BUFF_LENGTH = 4096;

	NgapProtocol() : BaseLayerT<NgapMessage>()
	{
	}

    virtual ~NgapProtocol(void)
	{
	}

	bool DecodeResourceSetupRequestTransfer(const char* buf, int32_t *len, void** ie);
	bool EncodeResourceSetupRequestTransfer(void* ie,  char* buf, int32_t *len);
	void FreeResourceSetupRequestTransfer(void* ie) 
	{
    	ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_Ngap_PDUSessionResourceSetupRequestTransfer, ie);
	}


	bool DecodeResourceSetupResponseTransfer(const char* buf, int32_t *len, void** ie);
	bool EncodeResourceSetupResponseTransfer(void* ie,  char* buf, int32_t *len);
	void FreeResourceSetupResponseTransfer(void* ie) 
	{
		ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_Ngap_PDUSessionResourceSetupResponseTransfer, ie);
	}

	
	bool DecodeResourceModifyRequestTransfer(const char* buf, int32_t *len, void** ie);
	bool EncodeResourceModifyRequestTransfer(void* ie,  char* buf, int32_t *len);
	void FreeResourceModifyRequestTransfer(void* ie) 
	{
    	ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_Ngap_PDUSessionResourceModifyRequestTransfer, ie);
	}


	bool DecodeResourceModifyResponseTransfer(const char* buf, int32_t *len, void** ie);
	bool EncodeResourceModifyResponseTransfer(void* ie,  char* buf, int32_t *len);
	void FreeResourceModifyResponseTransfer(void* ie) 
	{
		ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_Ngap_PDUSessionResourceModifyResponseTransfer, ie);
	}
	
	bool DecodeS2TTransparentContainer(const char* buf, int32_t *len, void** ie);
	bool EncodeS2TTransparentContainer(void* ie,  char* buf, int32_t *len);
	void FreeS2TTransparentContainer(void* ie) 
	{
		ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_Ngap_SourceNGRANNode_ToTargetNGRANNode_TransparentContainer, ie);
	}
	
	bool DecodeT2STransparentContainer(const char* buf, int32_t *len, void** ie);
	bool EncodeT2STransparentContainer(void* ie,  char* buf, int32_t *len);
	void FreeT2STransparentContainer(void* ie) 
	{
		ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_Ngap_TargetNGRANNode_ToSourceNGRANNode_TransparentContainer, ie);
	}

	bool DecodeHandoverCommandTransfer(const char* buf, int32_t *len, void** ie);
	bool EncodeHandoverCommandTransfer(void* ie,  char* buf, int32_t *len);
	void FreeHandoverCommandTransfer(void* ie) 
	{
		ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_Ngap_HandoverCommandTransfer, ie);
	}

	bool DecodeHandoverReqAckTransfer(const char* buf, int32_t *len, void** ie);
	bool EncodeHandoverReqAckTransfer(void* ie,  char* buf, int32_t *len);
	void FreeHandoverReqAckTransfer(void* ie) 
	{
		ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_Ngap_HandoverRequestAcknowledgeTransfer, ie);
	}

	bool DecodePathSwtichReqTransfer(const char* buf, int32_t *len, void** ie);
	bool EncodePathSwtichReqTransfer(void* ie,  char* buf, int32_t *len);
	void FreePatchSwitchReqTransfer(void* ie) 
	{
		ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_Ngap_PathSwitchRequestTransfer, ie);
	}

	bool DecodePathSwtichReqAckTransfer(const char* buf, int32_t *len, void** ie);
	bool EncodePathSwtichReqAckTransfer(void* ie,	char* buf, int32_t *len);
	void FreePatchSwitchReqAckTransfer(void* ie) 
	{
		ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_Ngap_PathSwitchRequestAcknowledgeTransfer, ie);
	}

protected:
	virtual bool InterDecode(const char* buf, int32_t *len,NgapMessage& info);
	virtual bool InterEncode(NgapMessage& info, char* buf, int32_t *len);

	virtual bool PreActionDecode(NgapMessage& info) 
	{
		return true;
	}
	virtual bool PreActionEncode(NgapMessage& info)
	{
		return true;
	}

	bool PostActionDecode(NgapMessage& info);
};
