#pragma once

#include <map>
#include <vector>
#include "mod_sig.h"
#include "SingletonT.h"
#include "NgapMessage.h"

class NgapSession;
class RanNodeId;
struct NgapMessage;


class NgapSessManager
{
protected:
	NgapSessManager();

public:
	~NgapSessManager();
	static NgapSessManager& Instance()
	{
		static NgapSessManager inst;
		return inst;
	}

	bool Init();
	
	bool NewNgapSession(NgapMessage &info, NgapSession **sess);
	bool NewNgapSessionFromAmf(NgapMessage &info, NgapSession **sess, uint32_t gNodeId);
	void DeleteNgapSession(RanNodeId ranNodeId, uint32_t gnbUeId, uint32_t sessId);
	void DeleteNgapSession(uint32_t amdId, uint64_t amfUeId, uint32_t ngapSessId);
	void DeleteNgapSession(uint32_t ngapSessId)
		{
		RemoveNgapSess(ngapSessId);
		}

	void DeleteAllAmfSession(uint32_t amfId);
	void DeleteAllGnbSession(RanNodeId ranNodeId);
	void DeleteAllGnbSession(uint32_t sctp_index);
	
	bool GetNgapSessionId(uint32_t amdId, uint64_t amfUeId, uint32_t& sessId);
	
	bool GetNgapSession(RanNodeId ranNodeId, uint32_t gnbUeId, NgapSession **sess);
	bool GetNgapSession(uint32_t ngapSessId, NgapSession **sess);

	void StopNgapSessCheck(uint32_t ngapSessId) { stopSessTimer(ngapSessId); }

	void SaveSourceToTarget_TransparentContainer(uint32_t ngapSessId, Ngap_SourceToTarget_TransparentContainer_t *container);
	bool GetSourceToTarget_TransparentContainer(uint32_t ngapSessId, Ngap_SourceToTarget_TransparentContainer_t **container);

	void DecodeNasPdu(NgapMessage& info, const uint8_t* nas_pdu, uint32_t len);
protected:
	uint32_t NewNgapSessionId();
	
	NgapSession* GetNgapSess(uint32_t sessId);
	inline bool AddNgapSess(NgapSession* sess);
	void RemoveNgapSess(uint32_t ngapSessId);
	void SafeRemoveNgapSess(uint32_t ngapSessId);

	enum { SESS_TIMER_VALUE = 30000 };
	static void onSessTimer(void *data);
	inline void startSessTimer(uint32_t ngapSessId);
	void stopSessTimer(uint32_t ngapSessId);
	void handleSessTimeout(void *data);

	enum { SESS_RELEASE_TIMER_VALUE = 3000 };
	static void onSessReleaseTimer(void *data);
	void startSessReleaseTimer(NgapSession *data);

private:
	agc_mutex_t* m_mutex;
	std::map<uint32_t, Ngap_SourceToTarget_TransparentContainer_t*> m_containerMap;
	uint32_t m_currentSessId;
	uint32_t m_currentStreamId;	

	enum { MAX_NGAP_SESSIONS = MAX_SIG_SESSIONS };
	typedef struct sess_event_context
	{	
		agc_event_t *evt;
		uint32_t sessId;
	}sess_event_context;

	NgapSession    *session_list[MAX_NGAP_SESSIONS];
	sess_event_context    *timer_list[MAX_NGAP_SESSIONS];

};


inline NgapSessManager& GetNgapSessManager()
{
	return NgapSessManager::Instance();
}


