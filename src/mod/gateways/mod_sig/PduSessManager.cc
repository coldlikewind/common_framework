#include <inttypes.h>

#include "PduSessManager.h"
#include "NgapSessManager.h"
#include "NsmManager.h"
#include "DbManager.h"
#include "EpmManager.h"
#include "ApmManager.h"
#include "NgapSession.h"

PduSessManager::PduSessManager()
{
	time_t t;
	srand((unsigned) time(&t));
	m_currentSessId = rand();

}

PduSessManager::~PduSessManager()
{

}

bool PduSessManager::Init()
{
	agc_memory_pool_t *module_pool = agcsig_get_sig_memory_pool();
	agc_mutex_init(&m_mutex, AGC_MUTEX_NESTED, module_pool);

	memset(session_list, 0 , MAX_PDU_SESSIONS * sizeof(sig_media_sess_t *));
	memset(ngap_msg_list, 0 , MAX_PDU_SESSIONS * sizeof(NgapMessage *));

	//memset(session_id_map, INVALIDE_ID , MAX_PDU_SESSION_ID * MAX_PDU_SESSIONS * sizeof(uint32_t));
	for (int i = 0; i < MAX_PDU_SESSION_ID; i++)
	{
		for (int j = 0; j < MAX_PDU_SESSIONS; j++)
		{
			session_id_map[i][j] = INVALIDE_ID;
		}
	}

	m_medias.clear();
	GetDbManager().QueryMediaGW(m_medias);

	gethostname(hostname, HOST_NAME_MAX);

	sig_media_path_bind();

	m_currentSessId = 0;
	return true;
}


void PduSessManager::Exit()
{
}


bool PduSessManager::processSessSetupReq(NgapMessage& info, uint8_t pdu_resource_id, uint32_t teid, agc_std_sockaddr_t &addr, socklen_t addrlen,
	uint32_t teid_v6, agc_std_sockaddr_t &addr_v6, socklen_t addrlen_v6) 
{
	uint32_t pdu_sessid = 0;
	bool success = false;	

	sig_media_sess_t *sess = newPduSess(info);
	if (sess == NULL) {
		return false;
	}

	pdu_sessid = sess->sig_sess_id;
	newSessMap(info.sessId, pdu_resource_id, pdu_sessid);

	//agc_mutex_lock(m_mutex);

	if (info.ngapMessage.present != Ngap_NGAP_PDU_PR_NOTHING)
	{
		NgapMessage *infoS = new NgapMessage;
		// save msg to infoS
		memcpy(infoS, &info, sizeof(NgapMessage));
		//save info;
		ngap_msg_list[info.sessId] = infoS;

		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"processSessSetupReq saved msg in ngap_msg_list[%d] \n", info.sessId);

		info.ngapMessage.present = Ngap_NGAP_PDU_PR_NOTHING;
	}

	// save upf addr & teid
	sess->ngap_sess_id = info.sessId;
	sess->pdu_resource_id = pdu_resource_id;
	sess->upf.remote_teid = teid;
	sess->upf.remote_addrlen = addrlen;
	memcpy(&sess->upf.remote_addr, &addr, addrlen);

	sess->upfv6.remote_teid = teid_v6;
	sess->upfv6.remote_addrlen = addrlen_v6;
	if (addrlen_v6 > 0)
	{
		memcpy(&sess->upfv6.remote_addr, &addr_v6, addrlen_v6);	
	}

	if (sig_media_path_send_upf_setup_req(sess->sig_sess_id, 
		sess->upf.remote_teid, &sess->upf.remote_addr, sess->upf.remote_addrlen, 
		sess->upfv6.remote_teid, &sess->upfv6.remote_addr, sess->upfv6.remote_addrlen,
		sess->MediaGWName, hostname) == AGC_STATUS_SUCCESS)
		success = true;

	//agc_mutex_unlock(m_mutex);

	startSessTimer(sess);
	return success;
}

bool PduSessManager::processSessSetupRsp(NgapMessage& info, uint8_t pdu_resource_id, uint32_t teid, agc_std_sockaddr_t &addr, socklen_t addrlen,
	uint32_t teid_v6, agc_std_sockaddr_t &addr_v6, socklen_t addrlen_v6) 
{
	sig_media_sess_t *sess = getPduSess(info.sessId, pdu_resource_id);
	if (sess == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, 
			"PduSessManager::processSessSetupRsp get pdu_sessid fail pdu_sess_id=%d pdu_resource_id=%d.\n",
			info.sessId, pdu_resource_id);
		return false;
	}

	if (info.ngapMessage.present != Ngap_NGAP_PDU_PR_NOTHING)
	{
		NgapMessage *infoS = new NgapMessage;
		// save msg to infoS
		memcpy(infoS, &info, sizeof(NgapMessage));
		//save info;
		ngap_msg_list[info.sessId] = infoS;
		info.ngapMessage.present = Ngap_NGAP_PDU_PR_NOTHING;
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"processSessSetupRsp saved msg in ngap_msg_list[%d] \n", info.sessId);
	}

	// save gnb addr & teid
	sess->gnb.remote_teid = teid;
	sess->gnb.remote_addrlen = addrlen;
	memcpy(&sess->gnb.remote_addr, &addr, addrlen);

	sess->gnbv6.remote_teid = teid_v6;
	sess->gnbv6.remote_addrlen = addrlen_v6;
	if (addrlen_v6 > 0)
	{
		memcpy(&sess->gnbv6.remote_addr, &addr_v6, addrlen_v6);	
	}

	if (sig_media_path_send_gnb_setup_req(sess->sig_sess_id, sess->media_sess_id, 
		sess->gnb.remote_teid, &sess->gnb.remote_addr, sess->gnb.remote_addrlen, 
		sess->gnbv6.remote_teid, &sess->gnbv6.remote_addr, sess->gnbv6.remote_addrlen,
		sess->MediaGWName, hostname) != AGC_STATUS_SUCCESS)
		return false;

	//agc_mutex_unlock(m_mutex);

	startSessTimer(sess);
	return true;
}

bool PduSessManager::processSessSetupfail(NgapMessage& info, uint8_t pdu_resource_id) {
	uint32_t pdu_sessid = 0;
	sig_media_sess_t *sess = getPduSess(info.sessId, pdu_resource_id);
	if (sess == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, 
			"PduSessManager::processSessSetupfail get pdu_sessid fail pdu_sess_id=%d pdu_resource_id=%d.\n",
			info.sessId, pdu_resource_id);
		return false;
	}

	// no need to use sess if setup fail.
	//agc_mutex_lock(m_mutex);
	pdu_sessid = sess->sig_sess_id;
	sig_media_path_send_req(sess->sig_sess_id, sess->media_sess_id, MSET_SESS_RELEASE_REQ, sess->MediaGWName, hostname);
	//agc_mutex_unlock(m_mutex);

	deletePduSess(pdu_sessid);
	return true;
}

bool PduSessManager::processSessUpdateUpf(NgapMessage& info, uint8_t pdu_resource_id, uint32_t teid, agc_std_sockaddr_t &addr, socklen_t addrlen,
	uint32_t teid_v6, agc_std_sockaddr_t &addr_v6, socklen_t addrlen_v6) 
{
	sig_media_sess_t *sess = getPduSess(info.sessId, pdu_resource_id);
	if (sess == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, 
			"PduSessManager::processSessUpdateUpf get pdu_sessid fail pdu_sess_id=%d pdu_resource_id=%d.\n",
			info.sessId, pdu_resource_id);
		return false;
	}

	//agc_mutex_lock(m_mutex);
	if (info.ngapMessage.present != Ngap_NGAP_PDU_PR_NOTHING)
	{
		NgapMessage *infoS = new NgapMessage;
		// save msg to infoS
		memcpy(infoS, &info, sizeof(NgapMessage));
		//save info;
		ngap_msg_list[sess->sig_sess_id] = infoS;
		info.ngapMessage.present = Ngap_NGAP_PDU_PR_NOTHING;
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"processSessUpdateUpf saved msg in ngap_msg_list[%d] \n", sess->sig_sess_id);
	}

	// save gnb addr & teid
	sess->upf.remote_teid = teid;
	sess->upf.remote_addrlen = addrlen;
	memcpy(&sess->upf.remote_addr, &addr, addrlen);

	sess->upfv6.remote_teid = teid_v6;
	sess->upfv6.remote_addrlen = addrlen_v6;
	if (addrlen_v6 > 0)
	{
		memcpy(&sess->upfv6.remote_addr, &addr_v6, addrlen_v6);	
	}

	if (sig_media_path_send_upf_update_req(sess->sig_sess_id, sess->media_sess_id, 
		sess->upf.remote_teid, &sess->upf.remote_addr, sess->upf.remote_addrlen, 
		sess->upfv6.remote_teid, &sess->upfv6.remote_addr, sess->upfv6.remote_addrlen,
		sess->MediaGWName, hostname) != AGC_STATUS_SUCCESS)
		return false;

	//agc_mutex_unlock(m_mutex);
	startSessTimer(sess);
	return true;
}


bool PduSessManager::processSessUpdateGnb(NgapMessage& info, uint8_t pdu_resource_id, uint32_t teid, agc_std_sockaddr_t &addr, socklen_t addrlen,
	uint32_t teid_v6, agc_std_sockaddr_t &addr_v6, socklen_t addrlen_v6) {
	return true;
}


bool PduSessManager::ReleasePduSess(uint32_t ngap_sessid, uint8_t pdu_resource_id)
{
	uint32_t pdu_sessid = 0;
	sig_media_sess_t *sess = getPduSess(ngap_sessid, pdu_resource_id);
	if (sess == NULL) {
		return false;
	}

	sig_media_path_send_req(sess->sig_sess_id, sess->media_sess_id, MSET_SESS_RELEASE_REQ, sess->MediaGWName, hostname);
	pdu_sessid    = sess->sig_sess_id;
	deletePduSess(pdu_sessid);
	deleteSessMap(ngap_sessid, pdu_resource_id);
	
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "PduSessManager::ReleasePduSess ngap_sessid=%d pdu_sessid=%d release finished.\n",
			ngap_sessid, pdu_sessid);
	return true;
}

bool PduSessManager::Process(NgapMessage& info, uint8_t pdu_resource_id, uint32_t teid, agc_std_sockaddr_t &addr, socklen_t addrlen,
	uint32_t teid_v6, agc_std_sockaddr_t &addr_v6, socklen_t addrlen_v6)
{	
	if (info.PDUChoice == Ngap_NGAP_PDU_PR_initiatingMessage) {
		switch (info.ProcCode) {
			case Ngap_ProcedureCode_id_InitialContextSetup:
				return processSessSetupReq(info, pdu_resource_id, teid, addr, addrlen, teid_v6, addr_v6, addrlen_v6);
			break;
			case Ngap_ProcedureCode_id_PDUSessionResourceSetup: 
			{
				if (processSessUpdateUpf(info, pdu_resource_id, teid, addr, addrlen, teid_v6, addr_v6, addrlen_v6) == false)
				{
					// InitialContextSetup don't process yet
					return processSessSetupReq(info, pdu_resource_id, teid, addr, addrlen, teid_v6, addr_v6, addrlen_v6);
				}
			}
			break;
			//handover complete
			case Ngap_ProcedureCode_id_HandoverNotification:
				return processSessSetupRsp(info, pdu_resource_id, teid, addr, addrlen, teid_v6, addr_v6, addrlen_v6);
			break;
		}
	}
	else if (info.PDUChoice == Ngap_NGAP_PDU_PR_successfulOutcome){
		switch (info.ProcCode) {
			case Ngap_ProcedureCode_id_InitialContextSetup:
				return processSessSetupRsp(info, pdu_resource_id, teid, addr, addrlen, teid_v6, addr_v6, addrlen_v6);
			break;
			case Ngap_ProcedureCode_id_PDUSessionResourceSetup:
				return processSessSetupRsp(info, pdu_resource_id, teid, addr, addrlen, teid_v6, addr_v6, addrlen_v6);
			break;
		}
	}

	return true;
}


bool PduSessManager::convertNgapSess2PduSess(uint32_t ngap_sessid, uint8_t pdu_resource_id, uint32_t& pdu_sessid)
{
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "PduSessManager::convertNgapSess2PduSess get  ngap_sessid=%d pdu_resource_id=%d.\n",
		ngap_sessid, pdu_resource_id);

	if (ngap_sessid >= MAX_PDU_SESSIONS || pdu_resource_id >= MAX_PDU_SESSION_ID)
	{
		return false;
	}

	agc_mutex_lock(m_mutex);
	pdu_sessid = session_id_map[pdu_resource_id][ngap_sessid];
	agc_mutex_unlock(m_mutex);
	
	return true;
}

bool PduSessManager::UpdatePduSessResource(uint32_t ngap_sessid, uint8_t pdu_resource_id, pdu_session_resource_t &pdu_resource)
{
	sig_media_sess_t *sess = getPduSess(ngap_sessid, pdu_resource_id);
	if (sess == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, 
			"PduSessManager::UpdatePduSessResource get pdu_sessid fail pdu_sess_id=%d pdu_resource_id=%d.\n",
			ngap_sessid, pdu_resource_id);
		return false;
	}

	//agc_mutex_lock(m_mutex);
	memcpy(&sess->pdu_resource, &pdu_resource, sizeof(pdu_session_resource_t));
	//agc_mutex_unlock(m_mutex);

	return true;
}

bool PduSessManager::GetPduSessResource(uint32_t ngap_sessid, uint8_t pdu_resource_id, pdu_session_resource_t &pdu_resource)
{
	sig_media_sess_t *sess = getPduSess(ngap_sessid, pdu_resource_id);
	if (sess == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, 
			"PduSessManager::GetPduSessResource get pdu_sessid fail pdu_sess_id=%d pdu_resource_id=%d.\n",
			ngap_sessid, pdu_resource_id);
		return false;
	}

	//agc_mutex_lock(m_mutex);
	memcpy(&pdu_resource, &sess->pdu_resource, sizeof(pdu_session_resource_t));
	//agc_mutex_unlock(m_mutex);
	
	return true;
}

bool PduSessManager::GetGnbGtpInfo(uint32_t ngap_sessid, uint8_t pdu_resource_id, agc_std_sockaddr_t &local_addr,	uint32_t &to_gnb_teid,
	agc_std_sockaddr_t &local_addr_v6,	uint32_t &to_gnb_teid_v6)
{
	sig_media_sess_t *sess = getPduSess(ngap_sessid, pdu_resource_id);
	if (sess == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, 
			"PduSessManager::GetGnbGtpInfo get pdu_sessid fail pdu_sess_id=%d pdu_resource_id=%d.\n",
			ngap_sessid, pdu_resource_id);
		return false;
	}

	//agc_mutex_lock(m_mutex);
	to_gnb_teid = sess->gnb.media_teid;
	
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "PduSessManager::GetGnbGtpInfo get to_gnb_teid=%d.\n", to_gnb_teid);
	
	to_gnb_teid_v6 = sess->gnbv6.media_teid;
	memcpy(&local_addr, &sess->gnb.local_addr, sess->gnb.local_addrlen);
	if (sess->gnbv6.local_addrlen > 0)
		memcpy(&local_addr_v6, &sess->gnbv6.local_addr, sess->gnbv6.local_addrlen);
	//agc_mutex_unlock(m_mutex);

	return true;
}

bool PduSessManager::GetUpfGtpInfo(uint32_t ngap_sessid, uint8_t pdu_resource_id, agc_std_sockaddr_t &local_addr,	uint32_t &to_upf_teid, 
		agc_std_sockaddr_t &local_addr_v6,	uint32_t &to_upf_teid_v6)
{
	sig_media_sess_t *sess = getPduSess(ngap_sessid, pdu_resource_id);
	if (sess == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, 
			"PduSessManager::GetUpfGtpInfo get pdu_sessid fail pdu_sess_id=%d pdu_resource_id=%d.\n",
			ngap_sessid, pdu_resource_id);
		return false;
	}

	//agc_mutex_lock(m_mutex);
	to_upf_teid = sess->upf.media_teid;
	to_upf_teid_v6 = sess->upfv6.media_teid;
	memcpy(&local_addr, &sess->upf.local_addr, sess->upf.local_addrlen);
	if (sess->upfv6.local_addrlen > 0)
		memcpy(&local_addr_v6, &sess->upfv6.local_addr, sess->upfv6.local_addrlen);
	//agc_mutex_unlock(m_mutex);

	return true;
}

sig_media_sess_t *PduSessManager::newPduSess(NgapMessage& info)
{
	uint32_t pdu_sess_id = 0;
	uint32_t count = 0;
	do
	{
		agc_mutex_lock(m_mutex);
		m_currentSessId = SIG_NEXT_ID(m_currentSessId, 1, MAX_PDU_SESSIONS - 1);
		pdu_sess_id = m_currentSessId;
		
		if (m_currentSessId > MAX_PDU_SESSIONS || session_list[pdu_sess_id] == NULL)
		{
			agc_mutex_unlock(m_mutex);
			break;
		}
		
		agc_mutex_unlock(m_mutex);
		
		count++;
		if (count >= MAX_PDU_SESSIONS)
		{
	        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "PduSessManager::newPduSess session is full！！！！\n");
			return NULL;
		}
	} while (1);
			
	sig_media_sess_t *sess = new sig_media_sess_t;

    if (selectMediaGW(info, sess->MediaGWName) == false)
	{
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "PduSessManager::newPduSess MediaGWName isn't config idSigGW=%d！\n", info.idSigGW);
		return NULL;
	}
		
	sess->gnbv6.media_teid = 0;
	sess->gnbv6.remote_teid = 0;
	sess->gnbv6.local_addrlen = 0;
	sess->gnbv6.remote_addrlen = 0;
	sess->upfv6.media_teid = 0;
	sess->upfv6.remote_teid = 0;
	sess->upfv6.local_addrlen = 0;
	sess->upfv6.remote_addrlen = 0;
	sess->target_teid = 0;
	sess->target_teid_v6 = 0;
	sess->target_addrlen = 0;
	sess->target_addrlen_v6 = 0;
	sess->sig_sess_id = pdu_sess_id;
		
	agc_mutex_lock(m_mutex);
	session_list[pdu_sess_id] = sess;
	agc_mutex_unlock(m_mutex);

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "PduSessManager::newPduSess pdu_sess_id=%d MediaGWName=%s ifo sessId=%d.\n", 
		sess->sig_sess_id,
		sess->MediaGWName,
		info.sessId);

	return sess;
}

sig_media_sess_t *PduSessManager::getPduSess(uint32_t pdu_sess_id)
{
	if (pdu_sess_id > MAX_PDU_SESSIONS)
		return NULL;

	agc_mutex_lock(m_mutex);
	sig_media_sess_t *sess =  session_list[pdu_sess_id];
	agc_mutex_unlock(m_mutex);
	return sess;
}

sig_media_sess_t *PduSessManager::getPduSess(uint32_t ngap_sessid, uint8_t pdu_resource_id)
{
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "PduSessManager::getPduSess get  ngap_sessid=%d pdu_resource_id=%d.\n",
		ngap_sessid, pdu_resource_id);

	if (ngap_sessid >= MAX_PDU_SESSIONS || pdu_resource_id >= MAX_PDU_SESSION_ID)
	{
		return NULL;
	}

	agc_mutex_lock(m_mutex);
	uint32_t pdu_sess_id = session_id_map[pdu_resource_id][ngap_sessid];
	agc_mutex_unlock(m_mutex);
	if (pdu_sess_id > MAX_PDU_SESSIONS)
		return NULL;

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "PduSessManager::getPduSess pdu_sess_id=%d.\n", pdu_sess_id);
	agc_mutex_lock(m_mutex);
	sig_media_sess_t *sess =  session_list[pdu_sess_id];
	agc_mutex_unlock(m_mutex);

	return sess;
}

void PduSessManager::deletePduSess(uint32_t pdu_sess_id)
{
	sig_media_sess_t *sess = NULL;
	if (pdu_sess_id > MAX_PDU_SESSIONS)
		return;

	agc_mutex_lock(m_mutex);
	sess = session_list[pdu_sess_id];
	agc_mutex_unlock(m_mutex);
	session_list[pdu_sess_id] = 0;
	delete sess;
}

void PduSessManager::newSessMap(uint32_t ngap_sessid, uint8_t pdu_resource_id, uint32_t pdu_sessid)
{
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "PduSessManager::newSessMap ngap_sessid=%d pdu_resource_id=%d.\n",
		ngap_sessid, pdu_resource_id);

	if (ngap_sessid >= MAX_PDU_SESSIONS || pdu_resource_id >= MAX_PDU_SESSION_ID)
	{
		return;
	}
	
	agc_mutex_lock(m_mutex);
	session_id_map[pdu_resource_id][ngap_sessid] = pdu_sessid;
	agc_mutex_unlock(m_mutex);
}

void PduSessManager::deleteSessMap(uint32_t ngap_sessid, uint8_t pdu_resource_id)
{
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "PduSessManager::deleteSessMap ngap_sessid=%d pdu_resource_id=%d.\n",
		ngap_sessid, pdu_resource_id);

	if (ngap_sessid >= MAX_PDU_SESSIONS || pdu_resource_id >= MAX_PDU_SESSION_ID)
	{
		return;
	}

	agc_mutex_lock(m_mutex);
	session_id_map[pdu_resource_id][ngap_sessid] = INVALIDE_ID;
	agc_mutex_unlock(m_mutex);
}

void PduSessManager::handleUpfSetupRsp(uint32_t pdu_sess_id, uint32_t media_sess_id,
		agc_std_sockaddr_t *to_gnb_addr, socklen_t to_gnb_addrlen,
		agc_std_sockaddr_t *to_upf_addr, socklen_t to_upf_addrlen,
		uint32_t to_upf_teid, uint32_t to_gnb_teid,
		agc_std_sockaddr_t *to_gnb_addr_v6, socklen_t to_gnb_addrlen_v6,
		agc_std_sockaddr_t *to_upf_addr_v6, socklen_t to_upf_addrlen_v6,
		uint32_t to_upf_teid_v6, uint32_t to_gnb_teid_v6) {

	sig_media_sess_t *sess = getPduSess(pdu_sess_id);
	NgapMessage *info = NULL;
	if (sess == NULL) {
		return;
	}

	NgapSession *ngap_sess = NULL;
	if (GetNgapSessManager().GetNgapSession(sess->ngap_sess_id, &ngap_sess) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"handleUpfSetupRsp fail to get sess=%d \n", sess->ngap_sess_id);
		return;
	}

	ngap_sess->pdu_sess_rsp_count++;
	
	//agc_mutex_lock(m_mutex);

	sess->media_sess_id = media_sess_id;
	sess->gnb.media_teid = to_gnb_teid;
	sess->gnb.local_addrlen = to_gnb_addrlen;
	memcpy(&sess->gnb.local_addr, to_gnb_addr, to_gnb_addrlen);

	sess->upf.media_teid = to_upf_teid;
	sess->upf.local_addrlen = to_upf_addrlen;
	memcpy(&sess->upf.local_addr, to_upf_addr, to_upf_addrlen);
	
	if (to_gnb_addrlen_v6 > 0)
	{
		sess->gnbv6.media_teid = to_gnb_teid_v6;
		sess->gnbv6.local_addrlen = to_gnb_addrlen_v6;
		memcpy(&sess->gnbv6.local_addr, to_gnb_addr_v6, to_gnb_addrlen_v6);
	}

	if (to_upf_addrlen_v6 > 0)
	{
		sess->upfv6.media_teid = to_upf_teid_v6;
		sess->upfv6.local_addrlen = to_upf_addrlen_v6;
		memcpy(&sess->upfv6.local_addr, to_upf_addr_v6, to_upf_addrlen_v6);
	}

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"handleUpfSetupRsp pdu_sess_rsp_count[%d] , pdu_resource_count[%d] , pdu_sess_id[%d] \n", ngap_sess->pdu_sess_rsp_count, ngap_sess->pdu_resource_count,pdu_sess_id);

	if (ngap_sess->pdu_sess_rsp_count >= ngap_sess->pdu_resource_count && ngap_msg_list[ngap_sess->sessId] != NULL ) {
        info = ngap_msg_list[ngap_sess->sessId];
        ngap_msg_list[ngap_sess->sessId] = NULL;

        agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "handleUpfSetupRsp get NgapMessage in ngap_msg_list[%d], count=%d\n",
                       ngap_sess->sessId, ngap_sess->pdu_sess_rsp_count);
    }
	//agc_mutex_unlock(m_mutex);

	if (info)
	{
		if (info->PDUChoice == Ngap_NGAP_PDU_PR_initiatingMessage) 
		{
            agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "handleUpfSetupRsp handler info before, sess_id=%d, ProcCode=%d \n", info->sessId, info->ProcCode);
			switch (info->ProcCode) {
				case Ngap_ProcedureCode_id_InitialContextSetup:
					AMF_InitialContextSetupReqByMedia(*info);
				break;
				case Ngap_ProcedureCode_id_PDUSessionResourceSetup: 
					AMF_PDUSessionResourceSetupReqByMedia(*info);
				break;
				case Ngap_ProcedureCode_id_HandoverResourceAllocation:
					AMF_HandoverReqByMedia(*info);
					break;
				default:
					break;
			}
		}

		delete info;
	}

	stopSessTimer(sess);
}

void PduSessManager::handleGnbSetupRsp(uint32_t pdu_sess_id) {
	sig_media_sess_t *sess = getPduSess(pdu_sess_id);
	NgapMessage *info = NULL;
	
	if (sess == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "handleGnbSetupRsp get pdu sess fail  pdu_sess_id=%d.\n", pdu_sess_id);
		return;
	}

	NgapSession *ngap_sess = NULL;
	if (GetNgapSessManager().GetNgapSession(sess->ngap_sess_id, &ngap_sess) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"handleGnbSetupRsp fail to get ngap sess=%d \n", sess->ngap_sess_id);
		return;
	}

	ngap_sess->pdu_sess_rsp_count++;

	//agc_mutex_lock(m_mutex);

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"handleGnbSetupRsp pdu_sess_rsp_count[%d] , pdu_resource_count[%d] , pdu_sess_id[%d] \n", ngap_sess->pdu_sess_rsp_count, ngap_sess->pdu_resource_count,pdu_sess_id);

    if (ngap_sess->pdu_sess_rsp_count >= ngap_sess->pdu_resource_count && ngap_msg_list[ngap_sess->sessId] != NULL) {
        info = ngap_msg_list[ngap_sess->sessId];
        ngap_msg_list[ngap_sess->sessId] = NULL;

        agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "handleGnbSetupRsp clear NgapMessage, ngap_sess_id=%d, count=%d\n",
                       ngap_sess->sessId, ngap_sess->pdu_sess_rsp_count);
    }
	//agc_mutex_unlock(m_mutex);
	
	if (info)
	{
		switch (info->ProcCode) {
			case Ngap_ProcedureCode_id_InitialContextSetup:
				GNB_InitialContextSetupSuccessRspByMedia(*info);
				break;
			case Ngap_ProcedureCode_id_PDUSessionResourceSetup:
				GNB_PDUSessionSetupRspByMedia(*info);
				break;
			case Ngap_ProcedureCode_id_PathSwitchRequest:
				GNB_PathSwitchRequestByMedia(*info);
				break;
			case Ngap_ProcedureCode_id_HandoverResourceAllocation:
				GNB_HandoverReqAckMedia(*info);
				break;
			default:
				break;
		}
		
		delete info;
	}
	stopSessTimer(sess);
}

void PduSessManager::handleSessCheckRsp(uint32_t pdu_sess_id) {
	sig_media_sess_t *sess = getPduSess(pdu_sess_id);
	if (sess == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "PduSessManager::StartSessTimer get fail  pdu_sess_id=%d.\n", pdu_sess_id);
		return;
	}

}

void PduSessManager::handleSessReleaseInd(uint32_t pdu_sess_id) {

}


void PduSessManager::handleUpfUpdateRsp(uint32_t pdu_sess_id) {
	sig_media_sess_t *sess = getPduSess(pdu_sess_id);
	NgapMessage *info = NULL;
	
	if (sess == NULL || ngap_msg_list[pdu_sess_id] == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "PduSessManager::handleUpfUpdateRsp get fail  pdu_sess_id=%d.\n", pdu_sess_id);
		return;
	}

	//agc_mutex_lock(m_mutex);

	info = ngap_msg_list[pdu_sess_id];
	ngap_msg_list[pdu_sess_id] = NULL;	
	//GetNsmManager().SendUpLayerSctp(*info);
	//AMF_PDUSessionResourceSetupReqByMedia(*info);

	//agc_mutex_unlock(m_mutex);
	
	if (info)
		delete info;

	stopSessTimer(sess);
}

void PduSessManager::onSessTimer(void *data) {
	sig_media_sess_t *sess = (sig_media_sess_t *)data;
	if (sess == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "PduSessManager::onSessTimer sess is invalid.\n");
		return;
	}
	uint32_t pdu_sess_id = sess->sig_sess_id;

	GetPduSessManager().handleSessTimeout(pdu_sess_id);
}

void PduSessManager::handleSessTimeout(uint32_t pdu_sess_id) {
	sig_media_sess_t *sess = getPduSess(pdu_sess_id);
	NgapMessage *info = NULL;
	NgapSession *ngap_sess = NULL;
	if (sess == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "PduSessManager::onSessTimer sess is invalid.\n");
		return;
	}

	if (GetNgapSessManager().GetNgapSession(sess->ngap_sess_id, &ngap_sess) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"handleSessTimeout fail to get sess=%d \n", sess->ngap_sess_id);
		return;
	}

	ngap_sess->pdu_sess_rsp_count++;

	//agc_mutex_lock(m_mutex);
	if (ngap_sess->pdu_sess_rsp_count <= ngap_sess->pdu_resource_count && ngap_msg_list[pdu_sess_id] != NULL)
	{
		info = ngap_msg_list[pdu_sess_id];
		ngap_msg_list[pdu_sess_id] = 0;	
	}
	sess->evt_timer = NULL;
	//agc_mutex_unlock(m_mutex);

	if (info == NULL)
		return;

	if (info->PDUChoice == Ngap_NGAP_PDU_PR_initiatingMessage) {
		switch (info->ProcCode) {
			case Ngap_ProcedureCode_id_InitialContextSetup:
				GetNsmManager().SendDownLayerSctp(*info);
			break;
			case Ngap_ProcedureCode_id_PDUSessionResourceSetup:
				GetNsmManager().SendDownLayerSctp(*info);
			break;
			case Ngap_ProcedureCode_id_PDUSessionResourceRelease:
				GetNsmManager().SendDownLayerSctp(*info);
			break;
		}
	}
	else {
		switch (info->ProcCode) {
			case Ngap_ProcedureCode_id_InitialContextSetup:
				GetNsmManager().SendUpLayerSctp(*info);
			break;
			case Ngap_ProcedureCode_id_PDUSessionResourceSetup:
				GetNsmManager().SendUpLayerSctp(*info);
			break;
			case Ngap_ProcedureCode_id_PDUSessionResourceRelease:
				GetNsmManager().SendUpLayerSctp(*info);
			break;
		}

	}
	
	agc_log_printf(AGC_LOG, AGC_LOG_WARNING,"handleSessTimeout sessId=%d pdu_id=%d\n", sess->ngap_sess_id, sess->pdu_resource_id);
	if (info)
		delete info;
}

void PduSessManager::startSessTimer(sig_media_sess_t *sess) {
	agc_event_t *evt_timer = NULL;
	if (sess == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "PduSessManager::startSessTimer fail.\n");
		return;
	}

	if (agc_event_create_callback(&evt_timer, EVENT_NULL_SOURCEID, sess, &PduSessManager::onSessTimer) != AGC_STATUS_SUCCESS) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "PduSessManager::startSessTimer create timer agc_event_create [fail].\n");
		return;
	}

	sess->evt_timer = evt_timer;
	agc_timer_add_timer(evt_timer, SESS_TIMER_VALUE);

}

void PduSessManager::stopSessTimer(sig_media_sess_t *sess) {
	if (sess == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "PduSessManager::stopSessTimer fail \n");
		return;
	}
	
	//agc_mutex_lock(m_mutex);

	sess->evt_timer = NULL;
	uint32_t pdu_sess_id = sess->sig_sess_id;

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "stopSessTimer(): pdu_sess_id = %d\n", pdu_sess_id);
	if (ngap_msg_list[pdu_sess_id] != NULL)
	{
		NgapMessage *info = NULL;
		info = ngap_msg_list[pdu_sess_id];
		ngap_msg_list[pdu_sess_id] = 0;	
		if (info)
			delete info;
	}
	sess->evt_timer = NULL;

	//agc_mutex_unlock(m_mutex);

}

//  当前轮选媒体网关
bool PduSessManager::selectMediaGW(NgapMessage& info, char *MediaGWName) {
	if (MediaGWName == NULL)
		return false;

	if (m_medias.size() <= 0) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "PduSessManager::selectMediaGW mediaGW size=%d.\n", m_medias.size());
		return false;
	}

	for (int i = 0; i < m_medias.size(); i++)
	{
		if (info.idSigGW == m_medias[i].idSigGW)
		{
			strcpy(MediaGWName, m_medias[i].MediaGWName);
			return true;
		}
	}
	return false;
}

bool PduSessManager::prepareHoTargetGnbAddr(uint32_t ngap_sessid, uint8_t pdu_resource_id, uint32_t teid, agc_std_sockaddr_t &addr, socklen_t addrlen,
	uint32_t teid_v6, agc_std_sockaddr_t &addr_v6, socklen_t addrlen_v6)
{
	sig_media_sess_t *sess = getPduSess(ngap_sessid, pdu_resource_id);
	if (sess == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, 
			"PduSessManager::prepareHoTargetGnbAddr get pdu_sessid fail pdu_sess_id=%d pdu_resource_id=%d.\n",
			ngap_sessid, pdu_resource_id);
		return false;
	}
	//agc_mutex_lock(m_mutex);

	sess->target_teid = teid;
	sess->target_teid_v6 = teid_v6;
	sess->target_addrlen = addrlen;
	sess->target_addrlen_v6 = addrlen_v6;

	if (addrlen > 0)
	{
		memcpy(&sess->target_addr, &addr, addrlen);
	}
	
	if (addrlen_v6 > 0)
	{
		memcpy(&sess->target_addr_v6, &addr_v6, addrlen_v6);
	}
	
	//agc_mutex_unlock(m_mutex);

	return true;
}

bool PduSessManager::prepareHoDrb(uint32_t ngap_sessid, uint8_t pdu_resource_id, uint8_t drb)
{
	uint32_t pdu_sessid = 0;
	if (convertNgapSess2PduSess(ngap_sessid, pdu_resource_id, pdu_sessid) == false) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "PduSessManager::prepareHoDrb get pdu_sessid fail pdu_sess_id=%d pdu_resource_id=%d.\n", ngap_sessid, pdu_resource_id);
		return false;
	}

	//agc_mutex_lock(m_mutex);
	m_sessDrb[pdu_sessid] = drb;
	//agc_mutex_unlock(m_mutex);
	
	return true;
}

bool PduSessManager::getHoDrb(uint32_t ngap_sessid, uint8_t pdu_resource_id, uint8_t &drb)
{
	uint32_t pdu_sessid = 0;
	if (convertNgapSess2PduSess(ngap_sessid, pdu_resource_id, pdu_sessid) == false) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "PduSessManager::getHoDrb get pdu_sessid fail pdu_sess_id=%d pdu_resource_id=%d.\n", ngap_sessid, pdu_resource_id);
		return false;
	}
	
	//agc_mutex_lock(m_mutex);
	if (m_sessDrb.find(pdu_sessid) == m_sessDrb.end()) 
	{
		//agc_mutex_unlock(m_mutex);
		return false;
	}
	
	drb = m_sessDrb[pdu_sessid];
	//agc_mutex_unlock(m_mutex);
	
	return true;
}

bool PduSessManager::finishHo(uint32_t ngap_sessid, uint8_t pdu_resource_id)
{
	uint32_t pdu_sessid = 0;
	if (convertNgapSess2PduSess(ngap_sessid, pdu_resource_id, pdu_sessid) == false) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "PduSessManager::finishHo get pdu_sessid fail pdu_sess_id=%d pdu_resource_id=%d.\n", ngap_sessid, pdu_resource_id);
		return false;
	}

	sig_media_sess_t *sess = getPduSess(pdu_sessid);
	if (sess == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "PduSessManager::finishHo get pdu_sessid fail pdu_sess_id=%d pdu_sessid=%d.\n",
			ngap_sessid, pdu_sessid);
		return false;
	}
	//agc_mutex_lock(m_mutex);

	// save gnb addr & teid
	sess->gnb.remote_teid = sess->target_teid;
	sess->gnb.remote_addrlen = sess->target_addrlen;
	memcpy(&sess->gnb.remote_addr, &sess->target_addr, sess->target_addrlen);

	sess->gnbv6.remote_teid = sess->target_teid_v6;
	sess->gnbv6.remote_addrlen = sess->target_addrlen_v6;
	if (sess->target_addrlen_v6 > 0)
	{
		memcpy(&sess->gnbv6.remote_addr, &sess->target_addr_v6, sess->target_addrlen_v6);	
	}

	sig_media_path_send_gnb_setup_req(sess->sig_sess_id, sess->media_sess_id, 
		sess->gnb.remote_teid, &sess->gnb.remote_addr, sess->gnb.remote_addrlen, 
		sess->gnbv6.remote_teid, &sess->gnbv6.remote_addr, sess->gnbv6.remote_addrlen, sess->MediaGWName, hostname);

	sess->target_teid = 0;
	sess->target_teid_v6 = 0;
	sess->target_addrlen = 0;
	sess->target_addrlen_v6 = 0;

	m_sessDrb.erase(pdu_sessid);	

	//agc_mutex_unlock(m_mutex);
	
	return true;
}


bool PduSessManager::resetHoTargetGnbAddr(uint32_t ngap_sessid, uint8_t pdu_resource_id)
{
	sig_media_sess_t *sess = getPduSess(ngap_sessid, pdu_resource_id);
	if (sess == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, 
			"PduSessManager::resetHoTargetGnbAddr get pdu_sessid fail pdu_sess_id=%d pdu_resource_id=%d.\n",
			ngap_sessid, pdu_resource_id);
		return false;
	}
	//agc_mutex_lock(m_mutex);
	
	sess->target_teid = 0;
	sess->target_teid_v6 = 0;
	sess->target_addrlen = 0;
	sess->target_addrlen_v6 = 0;

	m_sessDrb.erase(sess->sig_sess_id);	

	//agc_mutex_unlock(m_mutex);
	
	return true;
}


