#pragma once

#include <map>
#include "mod_sig.h"
#include "EsmContext.h"
#include "NgapAsn1c.h"
#include "NgapSession.h"
#include "NgapSessManager.h"


class RanNodeId;
struct NgapMessage;


/*
 *	Esm: ng-ENG SCTP message Module
 */

class EsmManager 
{
protected:
	EsmManager(void);

public:
	~EsmManager(void);

	static EsmManager& Instance()
	{
		static EsmManager inst;
		return inst;
	}

	bool Init();
	void Exit();

	bool GetEsmContext(uint32_t sctp_index, EsmContext **esm);
	bool GetEsmContext(RanNodeId& ranNodeId, EsmContext **esm);

	bool GetEsmContextByRanNodeId(RanNodeId& ranNodeId, EsmContext **esm);

	bool FindRanNodeId(uint32_t sctp_index, RanNodeId& ranNodeId);

	bool Setup(const NgapMessage& info, EsmContext *newContext);
	bool Update(const NgapMessage& info, EsmContext *newEsm);
	
	bool SendResetAck(NgapMessage& info);
	bool SendNgSetupFailure(uint32_t cause, NgapMessage& info);
	bool SendNgSetupResponse(NgapMessage& info);
	bool SendHandoverRequest(NgapMessage& info,   Ngap_SourceToTarget_TransparentContainer_t *SourceToTarget_TransparentContainer);
	bool SendHandoverCommand(NgapMessage& info, Ngap_TargetToSource_TransparentContainer_t	 *pTTS_TransparentContainer,
		Ngap_PDUSessionResourceAdmittedList_t *PDUSessionResourceAdmittedList);
	bool SendUCRelCommand(NgapMessage& info);
	bool SendHOPrepareFailure(NgapMessage& info, Ngap_Cause_t *ptCause);
	bool SendHandoverCancelAck(NgapMessage& info);
	bool SendPathSwitchReqAck(NgapMessage& info);
	bool SendDLRanStatusTsfer(NgapMessage& info, Ngap_RANStatusTransfer_TransparentContainer_t	*RANStatusTransfer_TransparentContainer);
	bool SendToAllRAN(NgapMessage& info);
	e_Ralation_RAN_GW CheckRANinGW(NgapMessage& info, RanNodeId& ranid);

protected:

	std::map<RanNodeId, EsmContext*> m_RanMap;//key=HenbId
	std::map<uint32_t, RanNodeId>	m_RanIndexMap;//key=associationID,value=HenbId	

	enum { MAX_ESM_SESSIONS = 2000 };

	EsmContext    *esm_list[MAX_ESM_SESSIONS];


	agc_mutex_t* m_mutex;
};



inline EsmManager& GetEsmManager()
{
	return EsmManager::Instance();
}
	

