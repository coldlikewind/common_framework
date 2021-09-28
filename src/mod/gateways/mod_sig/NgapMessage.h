#pragma once

#include <vector>
#include "mod_sig.h"
#include "RanNodeId.h"
#include "NgapAsn1c.h"

typedef Ngap_NGAP_PDU_t NGAP_MESSAGE_T;

struct NgapMessage 
{
	NgapMessage( )
	{
		sockSource	= 0;
		sockTarget	= 0;
		stream_no 	= 0;
		PDUChoice	= 0;
		ProcCode	= 0;
		idSigGW 	= 0;
	    gnbUeId 	= INVALIDE_ID;
	    amfUeId 	= INVALIDE_ID_X64;
	    sessId 		= INVALIDE_ID;
		nasMessageType = 0;
		nasSequence = 0;

		max_sctp_stream = 0;
		
		//ngapMessage.present = Ngap_NGAP_PDU_PR_NOTHING;
	    memset(&ngapMessage, 0, sizeof (NGAP_MESSAGE_T));
	    memset(&sctp_stream, 0, sizeof (agc_sctp_stream_t));
	}
    virtual ~NgapMessage()
	{
		if (ngapMessage.present != Ngap_NGAP_PDU_PR_NOTHING)
		{
    		ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_Ngap_NGAP_PDU, &ngapMessage);
		}
		ngapMessage.present = Ngap_NGAP_PDU_PR_NOTHING;
	}
	
	uint64_t                amfUeId;
	uint32_t                sessId;
	uint32_t                gnbUeId;
	uint32_t                amfId;
	uint32_t				stream_no;
	uint16_t 				PDUChoice;
	uint16_t 				ProcCode;	
	uint8_t                 sctpIndex;
	uint8_t                 idSigGW;
	NGAP_MESSAGE_T      	ngapMessage;
	RanNodeId           	ranNodeId;
	agc_sctp_stream_t		sctp_stream;
	uint32_t 				sockSource;
	uint32_t		 		sockTarget;

	uint8_t					nasMessageType;
	uint8_t					nasSequence;

	uint16_t				max_sctp_stream;
};



enum NGAP_CHECK_RAN_NODE_ID
{
    NGAP_CHECK_RNI_OK = 0x00,
    NGAP_CHECK_RNI_PLMN_ERROR,
    NGAP_CHECK_RNI_UNKNOW_ERROR,
};

