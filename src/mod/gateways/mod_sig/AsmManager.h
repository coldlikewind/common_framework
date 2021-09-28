#pragma once

#include <map>
#include <vector>
#include "SingletonT.h"
#include "SctpLayer.h"
#include "NgapMessage.h"

class SctpLayer;
class AsmContext;
class Timer;
struct NgapMessage;
struct SctpInfo;


/*
 * ASM：AMF SCTP message Module
 */ 

class AsmManager 
{
protected:
	AsmManager(void);

public:
	~AsmManager(void);

	static AsmManager& Instance()
	{
		static AsmManager inst;
		return inst;
	}

	bool Init();
	bool Clear(uint32_t amfid);
	bool Restart(uint32_t sctp_index);
	void Close();
	uint32_t SelectAmf(uint8_t idSigGW);	

	void GetAmfId(std::vector<uint32_t>& amfIdVec);
	//AsmContext* GetContext(uint32_t amfid);
	uint32_t GetAmfIdBySock(uint32_t sctp_index);
	//bool UpdateSctpInfo(uint32_t amfid, uint32_t stream_no);
	
	uint32_t GetTargetSock(uint32_t amfid);

	bool SendNgSetup(NgapMessage& info);
	bool handleNgSetupResponse(uint32_t sctp_index, NgapMessage& info);

	bool SendResetAck(NgapMessage& info);
	bool SendCfgUpdAck(NgapMessage& info);
	
	
	void AddTimer(AsmContext *context, uint32_t TimeOut);
	void DelTimer(uint32_t amfId);
	
	void CheckSctpToAmf(AsmContext *context);
	bool ConnectAmf(uint32_t amfId);

	bool SendPathSwithRequest(NgapMessage& info, Ngap_NR_CGI_t *nr_cgi, Ngap_TAI_t *tai);

	
	//bool Update(const NgapMessage& info, AsmContext &newAsm);

	static void OnTimer(void *data);
	
	bool AddAmf(uint8_t idSigGW, uint32_t sctp_index, uint8_t priority);
protected:

	agc_mutex_t* m_mutex;

	enum { MAX_AMF_SESSIONS = 2000 };

	AsmContext    *amf_list[MAX_AMF_SESSIONS];
	AsmContext    *amf_sctp_list[MAX_AMF_SESSIONS];

};

inline AsmManager& GetAsmManager()
{
	return AsmManager::Instance();
}
	
