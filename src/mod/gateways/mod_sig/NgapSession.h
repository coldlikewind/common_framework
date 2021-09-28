#pragma once

#include <sstream>
#include <vector>
#include "mod_sig.h"
#include "Tai.h"

#include "RanNodeId.h"
#include "CfgStruct.h"

#define MAX_DRB_QOS					2
#define MAX_SECURITY_CONTEXT_LEN  	32
#define MAX_PDU_SESSION_SIZE		20

#define INVALID_PDU_SESSION_ID      INVALIDE_NULL_int8

typedef enum Handover_Ralation_RAN_GW {
	Ralation_RAN_Handover_NULL = 0,
	Ralation_RAN_Handover_IN_SAME_GW	= 1,
	Ralation_RAN_Handover_NOT_IN_SAME_GW	= 2,
	/*
	 * Enumeration is extensible
	 */
} e_Ralation_RAN_GW;

typedef enum Handover_State {
	HO_State_NULL = 0,
	HO_State_IN_SAME_GW,
	HO_State_SendRelCommandWaitRelComplete,
	/*
	 * Enumeration is extensible
	 */
} e_HandoverState;

	


class NgapSession 
{
public:
	uint32_t    sessId;/*Key:contextID*/
	uint32_t    amfId;
	uint64_t    amfUeId;
	uint32_t    gnbUeId;
    Tai    		UeTai;
	RanNodeId   gnbId;
	uint32_t    gnb_sctp_index;

	uint16_t    stream_no;

	//new for handover
	e_Ralation_RAN_GW eHandoverflag;
	RanNodeId   TgtGnbId;
	uint32_t    TgtGnbUeId;
	e_HandoverState eHOState;
	uint32_t    TgtTeid;
	socklen_t 	TgtAddrlen;
	agc_std_sockaddr_t TgtAddr;
	uint32_t    TgtTeidV6;
	socklen_t 	TgtAddrlenV6;
	agc_std_sockaddr_t TgtAddrV6;

	// save the session information
	uint64_t    maxBitrateUl;
	uint64_t    maxBitrateDl;
	uint32_t	allowedSNnsaiSD;
	uint8_t		allowedSNnsaiSST;
	uint16_t	nRencryptionAlgorithms;
	uint16_t	nRintegrityProtectionAlgorithms;
	uint16_t	eUTRAencryptionAlgorithms;
	uint16_t	eUTRAintegrityProtectionAlgorithms;
	uint8_t		securityContext[MAX_SECURITY_CONTEXT_LEN];
	Cfg_GUAMI   guami;		
	uint8_t     vecPduSessionID[MAX_PDU_SESSION_SIZE];
	uint8_t		pdu_resource_count;
	uint8_t     pdu_sess_rsp_count;

	uint32_t 	nextHopChainingCount;
	bool    	release_timer_started;
	
	NgapSession()
	{
        sessId	 	= INVALIDE_ID;
        amfUeId 	= INVALIDE_ID_X64;
		gnbUeId 	= INVALIDE_ID;
		amfId	 	= INVALIDE_ID;
		gnb_sctp_index = INVALIDE_ID;
//		PeerSessId  = INVALIDE_ID;
		maxBitrateUl = 2000000000000;
		maxBitrateDl = 2000000000000;

		stream_no = 0;
		
		plmn_id_t plmn={0};

		gnbId.setRanNodePlmn(plmn);
		gnbId.setRanNodeID(0);
		gnbId.setRanNodeType(RanNodeId::RNT_GNB_ID);
		
//		PeerGnbId.setRanNodeIdMCC(0);
//		PeerGnbId.setRanNodeIdMNC(0);
//		PeerGnbId.setRanNodeID(0);
//		PeerGnbId.setRanNodeType(RanNodeId::RNT_GNB_ID);
		
        UeTai.setTaiMccMncTac(0, 0, 0);

		TgtGnbUeId = INVALIDE_ID;
		eHOState = HO_State_NULL;
		eHandoverflag = Ralation_RAN_Handover_NULL;
		
		TgtGnbId.setRanNodePlmn(plmn);
		TgtGnbId.setRanNodeID(0);
		TgtGnbId.setRanNodeType(RanNodeId::RNT_GNB_ID);

		maxBitrateUl = 0;
		maxBitrateDl = 0;
		allowedSNnsaiSD = 0;
		allowedSNnsaiSST = 0;
		nRencryptionAlgorithms = 0;
		nRintegrityProtectionAlgorithms = 0;
		eUTRAencryptionAlgorithms = 0;
		eUTRAintegrityProtectionAlgorithms = 0;
		pdu_resource_count = 0;
		pdu_sess_rsp_count = 0;

		nextHopChainingCount = 0;
		
		memset(securityContext, 0, MAX_SECURITY_CONTEXT_LEN * sizeof(uint8_t));
		memset(&guami, 0, sizeof(Cfg_GUAMI));

		for (int i = 0; i < MAX_PDU_SESSION_SIZE; i++)
			vecPduSessionID[i] = INVALID_PDU_SESSION_ID;

		release_timer_started = false;
	}
	
	~NgapSession() 
	{
	}
	
	NgapSession(const NgapSession& a)
	{
		sessId = a.sessId;
//		PeerSessId = a.PeerSessId;
		gnbId = a.gnbId;
//		PeerGnbId = a.PeerGnbId;
		gnbUeId = a.gnbUeId;
		amfId = a.amfId;
		amfUeId = a.amfUeId;
		UeTai = a.UeTai;
		gnb_sctp_index = a.gnb_sctp_index;

		stream_no = a.stream_no;
		
		eHandoverflag = a.eHandoverflag;
		TgtGnbId = a.TgtGnbId;
		TgtGnbUeId = a.TgtGnbUeId;
		eHOState = a.eHOState;
			
		maxBitrateUl = a.maxBitrateUl;
		maxBitrateDl = a.maxBitrateDl;
		allowedSNnsaiSD = a.allowedSNnsaiSD;
		allowedSNnsaiSST = a.allowedSNnsaiSST;
		nRencryptionAlgorithms = a.nRencryptionAlgorithms;
		nRintegrityProtectionAlgorithms = a.nRintegrityProtectionAlgorithms;
		eUTRAencryptionAlgorithms = a.eUTRAencryptionAlgorithms;
		eUTRAintegrityProtectionAlgorithms = a.eUTRAintegrityProtectionAlgorithms;
		pdu_resource_count = a.pdu_resource_count;
		pdu_sess_rsp_count = a.pdu_sess_rsp_count;

		nextHopChainingCount = a.nextHopChainingCount;
		
		memcpy(securityContext, a.securityContext, MAX_SECURITY_CONTEXT_LEN * sizeof(uint8_t));

		memcpy(&guami, &a.guami, sizeof(Cfg_GUAMI));
	
		for (int i = 0; i < MAX_PDU_SESSION_SIZE; i++)
			vecPduSessionID[i] = a.vecPduSessionID[i];

		release_timer_started = a.release_timer_started;
		//std::copy(a.vecPduSessionID.begin(), a.vecPduSessionID.end(), back_inserter(vecPduSessionID)); 
	}

	void ResetHandoverInfo()
	{
		TgtGnbUeId = INVALIDE_ID;
		eHOState = HO_State_NULL;
		eHandoverflag = Ralation_RAN_Handover_NULL;
		
		TgtGnbId.reset();
	}

	virtual std::string GetDesc() const
	{
		std::ostringstream os;
		os << *this;
		return os.str().c_str();
	}


	friend bool operator== (const NgapSession& info1,const NgapSession& info2)
	{
		if (info1.sessId != info2.sessId) 	return false;		
		if (info1.gnbId != info2.gnbId) 		return false;
		if (info1.gnbUeId != info2.gnbUeId) 	return false;
		if (info1.amfId != info2.amfId) 		return false;
		if (info1.amfUeId != info2.amfUeId) 	return false;
		if (info1.UeTai != info2.UeTai) 		return false;

		return true;
	}

	friend bool operator!= (const NgapSession& info1,const NgapSession& info2)
	{
		return !(info1 == info2);
	}

	friend ostream& operator<<(ostream& os,const NgapSession& info)
	{
		os << "NgapSession :";
		os << " sessId = " << info.sessId;
//		os << " PeerSessId = " << info.PeerSessId;
		os << " gnbId = " << info.gnbId;
//		os << " PeerGnbId = " << info.PeerGnbId;
		os << " gnbUeId = " << info.gnbUeId;
		os << " amfId = " << info.amfId;
		os << " amfUeId = " << info.amfUeId;	
		os << " UeTai = " << info.UeTai;
	
		return os;
	}


     NgapSession& operator= (const NgapSession& a)
     {
        sessId = a.sessId;
//		PeerSessId = a.PeerSessId;
        gnbId = a.gnbId;
//		PeerGnbId = a.PeerGnbId;
        gnbUeId = a.gnbUeId;
		amfId = a.amfId;
        amfUeId = a.amfUeId;
        UeTai = a.UeTai;
		stream_no = a.stream_no;
		gnb_sctp_index = a.gnb_sctp_index;
		
		eHandoverflag = a.eHandoverflag;
		TgtGnbId = a.TgtGnbId;
		TgtGnbUeId = a.TgtGnbUeId;
		eHOState = a.eHOState;
			
		maxBitrateUl = a.maxBitrateUl;
		maxBitrateDl = a.maxBitrateDl;
		allowedSNnsaiSD = a.allowedSNnsaiSD;
		allowedSNnsaiSST = a.allowedSNnsaiSST;
		nRencryptionAlgorithms = a.nRencryptionAlgorithms;
		nRintegrityProtectionAlgorithms = a.nRintegrityProtectionAlgorithms;
		eUTRAencryptionAlgorithms = a.eUTRAencryptionAlgorithms;
		eUTRAintegrityProtectionAlgorithms = a.eUTRAintegrityProtectionAlgorithms;
		pdu_resource_count = a.pdu_resource_count;
		pdu_sess_rsp_count = a.pdu_sess_rsp_count;

		nextHopChainingCount = a.nextHopChainingCount;
		
		memcpy(securityContext, a.securityContext, MAX_SECURITY_CONTEXT_LEN * sizeof(uint8_t));

		memcpy(&guami, &a.guami, sizeof(Cfg_GUAMI));

		for (int i = 0; i < MAX_PDU_SESSION_SIZE; i++)
			vecPduSessionID[i] = a.vecPduSessionID[i];

		release_timer_started = a.release_timer_started;

		//std::copy(a.vecPduSessionID.begin(), a.vecPduSessionID.end(), back_inserter(vecPduSessionID)); 
	
        return *this;
     }
};

