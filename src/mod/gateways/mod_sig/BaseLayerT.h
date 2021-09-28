/************************************************************
  Date:          2012/09/25
  Desc:         
  Usage:         
  History:
      <author>      <time>       <desc>
***********************************************************/
#pragma once
#include <vector>
#include <cassert>
#include <map>
#include "mod_sig.h"

template<class InfoT>
class BaseLayerT 
{
public:
	BaseLayerT() : allKey(0)
	{
		allKey = GetKey(ALL_PDU_CHOISE, ALL_PROCEDURE_CODE);
		m_recvErrorCallback = NULL;
		m_sendErrorCallback = NULL;
		
		memset(m_recvCallback, 0 , MAX_PDU_KEY * sizeof(AppCallback));
		memset(m_sendCallback, 0 , MAX_PDU_KEY * sizeof(AppCallback));
	}

	virtual ~BaseLayerT()
	{
	}

public:
	bool Encode(InfoT& info, char* buf, int32_t *len)
	{
		bool iRet = InterEncode(info, buf, len);
		if (iRet == false)
		{
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "BaseLayerT::InterEncode\n");
//			m_sendErrorCallback(info);
			return false;
		}

		return true;
	}

	bool ProcessEncode(InfoT& info, char* buf, int32_t *len)
	{
		bool iRet = PreActionEncode(info);
		if (iRet == false)
		{
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "BaseLayerT::PreActionEncode\n");
			m_sendErrorCallback(info);
			return false;
		}
		SendCallback(info.PDUChoice, info.ProcCode, info);
		return true;
	}

    bool Decode(const char* buf, int32_t *len, InfoT& info)
    {
        bool iRet = InterDecode(buf, len, info);
        if (iRet == false)
        {
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "BaseLayerT::InterDecode\n");

			if (m_recvErrorCallback)
	            m_recvErrorCallback(info); 
            return false;
        }

		return true;
    }

	bool ProcessDecode(const char* buf, int32_t *len, InfoT& info)
	{
		bool iRet = PreActionDecode(info);
		if (iRet == false)
		{
			if (m_sendErrorCallback)
				m_sendErrorCallback(info);
			return false;
		}
        return RecvCallback(info.PDUChoice, info.ProcCode, info);
	}

public:
	typedef bool (*AppCallback)(InfoT& info);

	void RegisterRecv(uint16_t pduChoise,uint16_t procedureCode,AppCallback callback)
	{
		uint32_t infoKey = GetKey(pduChoise, procedureCode);
		if (infoKey >= MAX_PDU_KEY)
			return;

		m_recvCallback[infoKey] = callback;
	}

	void RegisterSend(uint16_t pduChoise,uint16_t procedureCode,AppCallback callback)
	{
		uint32_t infoKey = GetKey(pduChoise, procedureCode);
		if (infoKey >= MAX_PDU_KEY)
			return;

		m_sendCallback[infoKey] = callback;
	}

	void RegisterRecvError(AppCallback callback)
	{
		m_recvErrorCallback = callback;
	}

	void RegisterSendError(AppCallback callback)
	{
		m_sendErrorCallback = callback;
	}

protected:
	virtual bool InterDecode(const char* buf, int32_t *len, InfoT& info) = 0;
	virtual bool InterEncode(InfoT& info, char* buf, int32_t *len) = 0;

	virtual bool PreActionDecode(InfoT& info) = 0;
	virtual bool PreActionEncode(InfoT& info) = 0;

    bool RecvCallback(uint16_t pduChoise, uint16_t procedureCode, InfoT& info)
    {
        bool ret = false;
        uint32_t messageKey;

        messageKey = GetKey(pduChoise, procedureCode);	
		if (messageKey >= MAX_PDU_KEY)
			return false;
		
        if (m_recvCallback[messageKey] != NULL)
        {
            ret = m_recvCallback[messageKey](info);
        }

        if (!ret && m_recvErrorCallback != NULL)
        {
            m_recvErrorCallback(info);
        }
	
        return ret;
    }

	bool SendCallback(uint16_t pduChoise, uint16_t procedureCode, InfoT& info)
	{
		bool ret = true;
		uint32_t messageKey;

		messageKey = GetKey(pduChoise, procedureCode);		
		if (messageKey >= MAX_PDU_KEY)
			return false;
		
		if (m_sendCallback[messageKey] != NULL)
		{
			ret = m_sendCallback[messageKey](info);
		}

		if (!ret && m_sendErrorCallback != NULL)
		{
			m_sendErrorCallback(info);
		}
		return ret;
	}

	uint32_t GetKey(uint32_t pduChoise, uint32_t procedureCode)
	{
		if (pduChoise > 4 || procedureCode > 255)
			return MAX_PDU_KEY;
		
		return (pduChoise << 8) + procedureCode;
	}

private:
    uint32_t allKey;

	enum { MAX_PDU_KEY = 1500 };
	AppCallback m_recvCallback[MAX_PDU_KEY];
	AppCallback m_sendCallback[MAX_PDU_KEY];


	AppCallback m_recvErrorCallback;
	AppCallback m_sendErrorCallback;
};

