#pragma once
#include "mod_sig.h"
#include "IStackManager.h"
#include "CfgStruct.h"
#include "NgapProtocol.h"
#include "SctpLayer.h"
#include <vector>
#include <map>

class NsmManager
{
	NsmManager(void);


public:
	virtual ~NsmManager(void);

	static NsmManager& Instance()
	{
		static NsmManager nsm;
		return nsm;
	}

public:
	virtual bool Init();
	virtual void WaitForExit();
	virtual void Exit();

	enum 
	{
		NSM_MAX_BUFFER_LEN = 4096
	};

public:
	typedef bool (*NsmAppCallback)(NgapMessage& info);
	
	virtual bool SendUpLayerSctp(NgapMessage& info);
	virtual bool SendDownLayerSctp(NgapMessage& info);

	virtual void RegisterUpLayerRecv(unsigned short pduChoise,unsigned short procedureCode,NsmAppCallback callback);
	virtual void RegisterDownLayerRecv(unsigned short pduChoise,unsigned short procedureCode,NsmAppCallback callback);

	virtual void RegisterUpLayerSend(unsigned short pduChoise,unsigned short procedureCode,NsmAppCallback callback);
	virtual void RegisterDownLayerSend(unsigned short pduChoise,unsigned short procedureCode,NsmAppCallback callback);

	virtual void RegisterUpLayerSendError(NsmAppCallback callback);
	virtual void RegisterDownLayerSendError(NsmAppCallback callback);

	virtual void RegisterUpLayerRecvError(NsmAppCallback callback);
	virtual void RegisterDownLayerRecvError(NsmAppCallback callback);
	bool status_sctp(uint32_t sctpindex);


	//virtual void RegisterMmeSctp(SctpAssocChanged func)= 0;
	//virtual void RegisterDownLayerSctp(SctpAssocChanged func, uint8_t SctpIndex = 0);
	

	static void handle_amf_sctp_err(
		uint32_t sctp_index);

	static void handle_amf_sctp_msg_read(
		uint32_t sctp_index, 
		uint32_t stream_no,
    	uint32_t ppid, 
    	const char* buf, 
    	int32_t *len,
    	uint32_t idSigGW,
    	uint32_t max_sctp_num);

	static void handle_gnb_sctp_err(
		uint32_t sctp_index);

	static void handle_gnb_sctp_msg_read(
		uint32_t sctp_index, 
		uint32_t stream_no,
    	uint32_t ppid, 
    	const char* buf, 
    	int32_t *len,
    	uint32_t idSigGW,
    	uint32_t max_sctp_num);

private:
	bool DecodeErrorProcess(NgapMessage& info);

	void handle_amf_ngap_msg(
		uint32_t sctp_index, 
		uint32_t stream_no,
    	uint32_t ppid, 
    	const char* buf, 
    	int32_t *len,
    	uint32_t idSigGW,
    	uint32_t max_sctp_num);

	void handle_gnb_ngap_msg(
		uint32_t sctp_index, 
		uint32_t stream_no,
    	uint32_t ppid, 
    	const char* buf, 
    	int32_t *len,
    	uint32_t idSigGW,
    	uint32_t max_sctp_num);

	enum { NSM_SCTP_TIMER_VALUE = 2000 };
	static void OnSctpTimer(void *data);
	void StartSctpTimer(uint32_t TimeOut);
	void handle_sctp_timeout();
private:
	bool m_exited;
	bool m_isInited;

private:
	NgapProtocol  m_amfNgapPro;
	NgapProtocol  m_gnbNgapPro;

	agc_mutex_t* m_mutex;


	std::vector<Cfg_LteSctp *> links;
	std::map<uint32_t, uint32_t> act_links;

	SctpLayerDrv*  m_sctpLayerDrv;
	
private:
	uint32_t GetKey(uint32_t pduChoise, uint32_t procedureCode)
	{
		if (pduChoise > 4 || procedureCode > 255)
			return MAX_PDU_KEY;
		
		return (pduChoise << 8) + procedureCode;
	}


    bool ProcessAmfMsg(NgapMessage& info)
    {
        uint32_t messageKey;
		bool ret = false;

        messageKey = GetKey(info.PDUChoice, info.ProcCode);	
		if (messageKey >= MAX_PDU_KEY)
			return ret;
		
        if (m_recvAmfCallback[messageKey] != NULL)
        {
            ret = m_recvAmfCallback[messageKey](info);
        }
	
        return ret;
    }
	
    bool ProcessGnbMsg(NgapMessage& info)
    {
        uint32_t messageKey;
		bool ret = false;

        messageKey = GetKey(info.PDUChoice, info.ProcCode);	
		if (messageKey >= MAX_PDU_KEY)
			return ret;
		
        if (m_recvGnbCallback[messageKey] != NULL)
        {
            ret = m_recvGnbCallback[messageKey](info);
        }
	
        return ret;
    }
	
	enum { MAX_PDU_KEY = 1500 };	
	NsmAppCallback m_recvAmfCallback[MAX_PDU_KEY];
	NsmAppCallback m_recvGnbCallback[MAX_PDU_KEY];
};



inline NsmManager& GetNsmManager()
{
	return NsmManager::Instance();
}



