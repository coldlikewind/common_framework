#pragma once

#include "mod_sig.h"
#include "SingletonT.h"
#include "sig_media_path.h"
#include "CfgStruct.h"
#include <map>
#include <vector>

struct NgapMessage;


class PduSessManager 
{
protected:
	PduSessManager(void);

public:
	~PduSessManager(void);

	static PduSessManager& Instance()
	{
		static PduSessManager inst;
		return inst;
	}

	bool Init();
	void Exit();

	// interface for ngap session
	/*bool Process(NgapMessage& info, uint32_t teid, agc_std_sockaddr_t &addr, socklen_t addrlen,
			uint32_t teid_v6, agc_std_sockaddr_t &addr_v6, socklen_t addrlen_v6);
	bool ReleasePduSess(uint32_t ngap_sessid);
	bool processSessSetupfail(NgapMessage& info);
	bool processSessRelease(NgapMessage& info);

	bool UpdatePduSessResource(uint32_t ngap_sessid, pdu_session_resource_t &pdu_resource);
	bool GetPduSessResource(uint32_t ngap_sessid, pdu_session_resource_t &pdu_resource);
	bool GetGtpInfo(uint32_t ngap_sessid, agc_std_sockaddr_t &local_addr,	uint32_t &to_gnb_teid);
*/

	bool Process(NgapMessage& info, uint8_t pdu_resource_id, uint32_t teid, agc_std_sockaddr_t &addr, socklen_t addrlen,
			uint32_t teid_v6, agc_std_sockaddr_t &addr_v6, socklen_t addrlen_v6);
	bool ReleasePduSess(uint32_t ngap_sessid, uint8_t pdu_resource_id);
	bool processSessSetupfail(NgapMessage& info, uint8_t pdu_resource_id);
	bool processSessSetupReq(NgapMessage& info, uint8_t pdu_resource_id, uint32_t teid, agc_std_sockaddr_t &addr, socklen_t addrlen,
		uint32_t teid_v6, agc_std_sockaddr_t &addr_v6, socklen_t addrlen_v6);
	bool processSessSetupRsp(NgapMessage& info, uint8_t pdu_resource_id, uint32_t teid, agc_std_sockaddr_t &addr, socklen_t addrlen,
		uint32_t teid_v6, agc_std_sockaddr_t &addr_v6, socklen_t addrlen_v6);
	bool processSessUpdateUpf(NgapMessage& info, uint8_t pdu_resource_id, uint32_t teid, agc_std_sockaddr_t &addr, socklen_t addrlen,
		uint32_t teid_v6, agc_std_sockaddr_t &addr_v6, socklen_t addrlen_v6);
	bool processSessUpdateGnb(NgapMessage& info, uint8_t pdu_resource_id, uint32_t teid, agc_std_sockaddr_t &addr, socklen_t addrlen,
		uint32_t teid_v6, agc_std_sockaddr_t &addr_v6, socklen_t addrlen_v6);

	bool prepareHoTargetGnbAddr(uint32_t ngap_sessid, uint8_t pdu_resource_id, uint32_t teid, agc_std_sockaddr_t &addr, socklen_t addrlen,
		uint32_t teid_v6, agc_std_sockaddr_t &addr_v6, socklen_t addrlen_v6);

	bool prepareHoDrb(uint32_t ngap_sessid, uint8_t pdu_resource_id, uint8_t drb);
	bool getHoDrb(uint32_t ngap_sessid, uint8_t pdu_resource_id, uint8_t &drb);

	bool finishHo(uint32_t ngap_sessid, uint8_t pdu_resource_id);
	bool resetHoTargetGnbAddr(uint32_t ngap_sessid, uint8_t pdu_resource_id);

	bool UpdatePduSessResource(uint32_t ngap_sessid, uint8_t pdu_resource_id, pdu_session_resource_t &pdu_resource);
	bool GetPduSessResource(uint32_t ngap_sessid, uint8_t pdu_resource_id, pdu_session_resource_t &pdu_resource);
	bool GetGnbGtpInfo(uint32_t ngap_sessid, uint8_t pdu_resource_id, agc_std_sockaddr_t &local_addr,	uint32_t &to_gnb_teid,
		agc_std_sockaddr_t &local_addr_v6,	uint32_t &to_gnb_teid_v6);
	bool GetUpfGtpInfo(uint32_t ngap_sessid, uint8_t pdu_resource_id, agc_std_sockaddr_t &local_addr,	uint32_t &to_upf_teid, 
		agc_std_sockaddr_t &local_addr_v6,	uint32_t &to_upf_teid_v6);
	

	bool hasPduSession(uint32_t ngap_sessid, uint8_t pdu_resource_id) {
			if (ngap_sessid >= MAX_PDU_SESSIONS || pdu_resource_id >= MAX_PDU_SESSION_ID)
			{
				return false;
			}
			
			agc_mutex_lock(m_mutex);
			uint32_t pdu_sess_id = session_id_map[pdu_resource_id][ngap_sessid];
			agc_mutex_unlock(m_mutex);

			if (pdu_sess_id == INVALIDE_ID)
				return false;
			
			return true;
		}
	
	// interface for sig_media_path
	void handleUpfSetupRsp(uint32_t pdu_sess_id, uint32_t media_sess_id,
		agc_std_sockaddr_t *to_gnb_addr, socklen_t to_gnb_addrlen,
		agc_std_sockaddr_t *to_upf_addr, socklen_t to_upf_addrlen,
		uint32_t to_upf_teid, uint32_t to_gnb_teid,
		agc_std_sockaddr_t *to_gnb_addr_v6, socklen_t to_gnb_addrlen_v6,
		agc_std_sockaddr_t *to_upf_addr_v6, socklen_t to_upf_addrlen_v6,
		uint32_t to_upf_teid_v6, uint32_t to_gnb_teid_v6);
	void handleGnbSetupRsp(uint32_t pdu_sess_id);
	void handleSessCheckRsp(uint32_t pdu_sess_id);
	void handleSessReleaseInd(uint32_t pdu_sess_id);
	void handleUpfUpdateRsp(uint32_t pdu_sess_id);

	sig_media_sess_t *newPduSess(NgapMessage& info);
	inline sig_media_sess_t *getPduSess(uint32_t pdu_sess_id);
	inline sig_media_sess_t *getPduSess(uint32_t ngap_sessid, uint8_t pdu_resource_id);
	void deletePduSess(uint32_t pdu_sess_id);

protected:
	/*bool processSessSetupReq(NgapMessage& info, uint32_t teid, agc_std_sockaddr_t &addr, socklen_t addrlen,
		uint32_t teid_v6, agc_std_sockaddr_t &addr_v6, socklen_t addrlen_v6);
	bool processSessSetupRsp(NgapMessage& info, uint32_t teid, agc_std_sockaddr_t &addr, socklen_t addrlen,
		uint32_t teid_v6, agc_std_sockaddr_t &addr_v6, socklen_t addrlen_v6);
	bool processSessUpdateUpf(NgapMessage& info, uint32_t teid, agc_std_sockaddr_t &addr, socklen_t addrlen,
		uint32_t teid_v6, agc_std_sockaddr_t &addr_v6, socklen_t addrlen_v6);
	bool processSessUpdateGnb(NgapMessage& info, uint32_t teid, agc_std_sockaddr_t &addr, socklen_t addrlen,
		uint32_t teid_v6, agc_std_sockaddr_t &addr_v6, socklen_t addrlen_v6);
*/

	enum { SESS_TIMER_VALUE = 4000 };
	static void onSessTimer(void *data);
	inline void startSessTimer(sig_media_sess_t *sess);
	inline void stopSessTimer(sig_media_sess_t *sess);
	void handleSessTimeout(uint32_t pdu_sess_id);

	inline void newSessMap(uint32_t ngap_sessid, uint8_t pdu_resource_id, uint32_t pdu_sessid);
	void deleteSessMap(uint32_t ngap_sessid, uint8_t pdu_resource_id);
	inline bool convertNgapSess2PduSess(uint32_t ngap_sessid, uint8_t pdu_resource_id, uint32_t& pdu_sessid);

	bool selectMediaGW(NgapMessage& info, char *MediaGWName);

protected:
	uint32_t m_currentSessId;
	agc_mutex_t* m_mutex;
	char hostname[HOST_NAME_MAX];
	
	enum { MAX_PDU_SESSIONS = MAX_SIG_SESSIONS, MAX_PDU_SESSION_ID = 255 };
	sig_media_sess_t    *session_list[MAX_PDU_SESSIONS];
	NgapMessage  		*ngap_msg_list[MAX_PDU_SESSIONS];
	
	std::map<uint32_t, uint8_t>				m_sessDrb;
	
	uint32_t  	session_id_map[MAX_PDU_SESSION_ID][MAX_PDU_SESSIONS];

	// media gateways
	std::vector<stCfg_MEDIA_DATA> 			m_medias;
	uint32_t								m_lstMediaIndex;

};

inline PduSessManager& GetPduSessManager()
{
	return PduSessManager::Instance();
}


