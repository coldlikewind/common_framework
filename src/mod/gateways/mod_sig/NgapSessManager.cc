
#include "RanNodeId.h"
#include "NgapSession.h"
#include "NgapSessManager.h"
#include "NgapMessage.h"
#include "PduSessManager.h"
#include "EsmContext.h"
#include "EsmManager.h"



NgapSessManager::NgapSessManager() 
{
	m_currentSessId = 1;
	m_currentStreamId = 1;
}

NgapSessManager::~NgapSessManager()
{
}

bool NgapSessManager::Init()
{
	agc_memory_pool_t *module_pool = agcsig_get_sig_memory_pool();
	agc_mutex_init(&m_mutex, AGC_MUTEX_NESTED, module_pool);
	
	memset(session_list, 0 , MAX_NGAP_SESSIONS * sizeof(NgapSession *));
	memset(timer_list, 0 , MAX_NGAP_SESSIONS * sizeof(sess_event_context *));

	m_currentSessId = 0;
	m_currentStreamId = 1;
	return true;
}

bool NgapSessManager::NewNgapSession(NgapMessage &info, NgapSession **sess)
{
	uint32_t sessId = 0;
	uint32_t count = 0;
	do
	{
		agc_mutex_lock(m_mutex);
		m_currentSessId = SIG_NEXT_ID(m_currentSessId, 1, MAX_NGAP_SESSIONS - 1);
		sessId = m_currentSessId;
		if (session_list[sessId] == 0)
		{
			agc_mutex_unlock(m_mutex);	
			break;
		}
		
		agc_mutex_unlock(m_mutex);	
		
		
		count++;
		if (count >= MAX_NGAP_SESSIONS)
		{
	        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "NgapSessManager::NewNgapSession session is full！！！！\n");
			return false;
		}
	} while (1);
		
	NgapSession *newsess = new NgapSession;
	newsess->gnbUeId = info.gnbUeId;
	newsess->amfUeId = INVALIDE_ID_X64;
	newsess->gnb_sctp_index = info.sockSource;
	//agc_mutex_lock(m_mutex);
	newsess->sessId = sessId;
	newsess->stream_no = m_currentStreamId++;
	if (m_currentStreamId >= info.max_sctp_stream) m_currentStreamId = 1;
	//agc_mutex_unlock(m_mutex);
	newsess->gnbId = info.ranNodeId;
	newsess->amfId = info.amfId;
	newsess->eHandoverflag = Ralation_RAN_Handover_NULL;

	AddNgapSess(newsess);
	startSessTimer(newsess->sessId);
	return GetNgapSession(newsess->sessId, sess);
}

bool NgapSessManager::NewNgapSessionFromAmf(NgapMessage &info, NgapSession **sess, uint32_t gNodeId)
{
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,
			"NgapSessManager::NewNgapSessionFromAmf() amfid=%ld, gnbid=%04x, amfUeId=%ld.\n",
			info.amfId, gNodeId, info.amfUeId);
	uint32_t sessId = 0;
	uint32_t count = 0;
	do
	{
		agc_mutex_lock(m_mutex);
		m_currentSessId = SIG_NEXT_ID(m_currentSessId, 1, MAX_NGAP_SESSIONS - 1);
		sessId = m_currentSessId;
		if (session_list[sessId] == 0)
		{
			agc_mutex_unlock(m_mutex);
			break;
		}

		agc_mutex_unlock(m_mutex);


		count++;
		if (count >= MAX_NGAP_SESSIONS)
		{
	        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "NgapSessManager::NewNgapSession session is full！！！！\n");
			return false;
		}
	} while (1);

	NgapSession *newsess = new NgapSession;

	// TODO: 尚未从信元中构造完整的RanNodeId类型, 简单的用getRanNodeID()和getRanNodeType()比较
	newsess->gnbUeId = INVALIDE_ID;
	newsess->gnbId.setRanNodeType(RanNodeId::RNT_GNB_ID);
	newsess->gnbId.setRanNodeID(gNodeId);
	EsmContext *esmContext;
	if (GetEsmManager().GetEsmContextByRanNodeId(newsess->gnbId, &esmContext) == false)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR,
				"GNB_HandoverNotify fail to get target esm context. gNodeId=%d.\n",
				newsess->gnbId.getRanNodeID());
		delete(newsess);
		return false;
	}
	newsess->gnb_sctp_index = esmContext->sctp_index;

	newsess->amfId = info.amfId;

	newsess->sessId = sessId;
	newsess->stream_no = m_currentStreamId++;
	if (m_currentStreamId >= info.max_sctp_stream) m_currentStreamId = 1;

	newsess->eHandoverflag = Ralation_RAN_Handover_NULL;

	AddNgapSess(newsess);

	// TODO: Timer
	return GetNgapSession(newsess->sessId, sess);
}

void NgapSessManager::DeleteNgapSession(RanNodeId ranNodeId, uint32_t gnbUeId, uint32_t sessId)
{
	RemoveNgapSess(sessId);
}

void NgapSessManager::DeleteNgapSession(uint32_t amdId, uint64_t amfUeId, uint32_t sessId)
{
	RemoveNgapSess(sessId);
}

void NgapSessManager::DeleteAllAmfSession(uint32_t amfId)
{
	uint32_t sessId = 0;
	
	//agc_mutex_lock(m_mutex);
	for (int i = 0; i < MAX_NGAP_SESSIONS; i++)
	{
		agc_mutex_lock(m_mutex);
		NgapSession *sess = session_list[i];		
		agc_mutex_unlock(m_mutex);
		if (sess != NULL && sess->amfId == amfId)
		{
			for (int j = 0; j < MAX_PDU_SESSION_SIZE; j++)
			{
				if (sess->vecPduSessionID[j] == INVALID_PDU_SESSION_ID)
					continue;
				
				GetPduSessManager().ReleasePduSess(sess->sessId, sess->vecPduSessionID[j]);
			}
			
			RemoveNgapSess(i);
		}
	}
	
	//agc_mutex_unlock(m_mutex);
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"NgapSessManager::DeleteAllAmfSession finished.\n");
}

void NgapSessManager::DeleteAllGnbSession(RanNodeId ranNodeId)
{
	uint32_t sessId = 0;
	
	//agc_mutex_lock(m_mutex);
	for (int i = 0; i < MAX_NGAP_SESSIONS; i++)
	{
		agc_mutex_lock(m_mutex);
		NgapSession *sess = session_list[i];		
		agc_mutex_unlock(m_mutex);
		if (sess != NULL && sess->gnbId == ranNodeId)
		{
			for (int j = 0; j < MAX_PDU_SESSION_SIZE; i++)
			{
				if (sess->vecPduSessionID[j] == INVALID_PDU_SESSION_ID)
					continue;
				
				GetPduSessManager().ReleasePduSess(sess->sessId, sess->vecPduSessionID[j]);
			}
			
			RemoveNgapSess(i);
		}
	}
	
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"NgapSessManager::DeleteAllGnbSession finished.\n");
}

void NgapSessManager::DeleteAllGnbSession(uint32_t sctp_index)
{
	uint32_t sessId = 0;
	
	//agc_mutex_lock(m_mutex);
	for (int i = 0; i < MAX_NGAP_SESSIONS; i++)
	{
		agc_mutex_lock(m_mutex);
		NgapSession *sess = session_list[i];		
		agc_mutex_unlock(m_mutex);
		if (sess != NULL && sess->gnb_sctp_index == sctp_index)
		{
			for (int j = 0; j < MAX_PDU_SESSION_SIZE; i++)
			{
				if (sess->vecPduSessionID[j] == INVALID_PDU_SESSION_ID)
					continue;
				
				GetPduSessManager().ReleasePduSess(sess->sessId, sess->vecPduSessionID[j]);
			}
			
			RemoveNgapSess(i);
		}
	}
	
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"NgapSessManager::DeleteAllGnbSession finished.\n");
}

bool NgapSessManager::GetNgapSession(RanNodeId ranNodeId, uint32_t gnbUeId, NgapSession **sess)
{
	uint32_t sessId = 0;
	bool found = false;
	for (int i = 0; i < MAX_NGAP_SESSIONS; i++)
	{
		agc_mutex_lock(m_mutex);
		NgapSession *tmp_sess = session_list[i];
		agc_mutex_unlock(m_mutex);
		if (tmp_sess != NULL && tmp_sess->gnbId == ranNodeId && tmp_sess->gnbUeId ==  gnbUeId && tmp_sess->amfUeId == INVALIDE_ID_X64)
		{
			found = true;
			sessId = tmp_sess->sessId;
			*sess = tmp_sess;
			break;
		}
	}
	
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG,"NgapSessManager::GetNgapSession gnbUeId=%d finished.\n", gnbUeId);
	return found;
}

bool NgapSessManager::GetNgapSessionId(uint32_t amdId, uint64_t amfUeId, uint32_t& sessId)
{
	return true;
}

bool NgapSessManager::GetNgapSession(uint32_t ngapSessId, NgapSession **sess)
{
	if (ngapSessId >= MAX_NGAP_SESSIONS)
		return false;
	
	agc_mutex_lock(m_mutex);
	*sess = session_list[ngapSessId];
	agc_mutex_unlock(m_mutex);	
	
	if (*sess == NULL)
		return false;
	
	return true;
}

// todo: session id 是全局唯一的，这里需要修改
uint32_t NgapSessManager::NewNgapSessionId()
{
	return m_currentSessId++;
}

bool NgapSessManager::AddNgapSess(NgapSession* sess)
{	
	agc_mutex_lock(m_mutex);
	session_list[sess->sessId] = sess;
	agc_mutex_unlock(m_mutex);

	return true;
}


void NgapSessManager::RemoveNgapSess(uint32_t ngapSessId)
{
	if (ngapSessId > MAX_NGAP_SESSIONS)
		return;
	
	agc_mutex_lock(m_mutex);
	NgapSession* sess = session_list[ngapSessId];
	//delay release session
	if (sess && !sess->release_timer_started)
	{
		startSessReleaseTimer(sess);
		sess->release_timer_started = true;
	}
	
	agc_mutex_unlock(m_mutex);

	//delete sess;
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "NgapSessManager::RemoveNgapSess sessId=%d\n", ngapSessId);
}


NgapSession* NgapSessManager::GetNgapSess(uint32_t ngapSessId)
{
	if (ngapSessId > MAX_NGAP_SESSIONS)
		return NULL;
	
	agc_mutex_lock(m_mutex);	
	NgapSession* sess = session_list[ngapSessId];
	agc_mutex_unlock(m_mutex);	

	return sess;
}


void NgapSessManager::SafeRemoveNgapSess(uint32_t ngapSessId)
{
	if (ngapSessId >= MAX_NGAP_SESSIONS)
		return;
	
	agc_mutex_lock(m_mutex);
	session_list[ngapSessId] = 0;
	agc_mutex_unlock(m_mutex);
}


void NgapSessManager::onSessReleaseTimer(void *data)
{
	if (data == NULL)
		return;

	NgapSession *sess = (NgapSession *)data;
	uint32_t ngapSessId = sess->sessId;
	GetNgapSessManager().SafeRemoveNgapSess(ngapSessId);
	
	delete sess;
}

void NgapSessManager::startSessReleaseTimer(NgapSession *data)
{
	agc_event_t *evt;
	if (agc_event_create_callback(&evt, EVENT_NULL_SOURCEID, data, &NgapSessManager::onSessReleaseTimer) != AGC_STATUS_SUCCESS) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "NgapSessManager::startSessReleaseTimer create timer agc_event_create [fail].\n");
		return;
	}

	agc_timer_add_timer(evt, SESS_RELEASE_TIMER_VALUE);
    agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "NgapSessManager::startSessReleaseTimer sessId=%d\n", data->sessId);
}


void NgapSessManager::onSessTimer(void *data)
{
	GetNgapSessManager().handleSessTimeout(data);
}

void NgapSessManager::startSessTimer(uint32_t ngapSessId)
{	
	sess_event_context *evt_timer = new sess_event_context;
	evt_timer->sessId = ngapSessId;
	if (agc_event_create_callback(&evt_timer->evt, EVENT_NULL_SOURCEID, evt_timer, &NgapSessManager::onSessTimer) != AGC_STATUS_SUCCESS) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "NgapSessManager::startSessTimer create timer agc_event_create [fail].\n");
		return;
	}

	agc_mutex_lock(m_mutex);
	timer_list[ngapSessId] = evt_timer;	
	agc_mutex_unlock(m_mutex);
	
	agc_event_t *evt = evt_timer->evt;
	
	agc_timer_add_timer(evt, SESS_TIMER_VALUE);
	
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "NgapSessManager::startSessTimer add new timer ngapSessId=%d \n", ngapSessId);

}

void NgapSessManager::stopSessTimer(uint32_t ngapSessId)
{	
	sess_event_context *evt_timer = NULL;

	if (ngapSessId > MAX_NGAP_SESSIONS) 
	{
		return;
	}
	
	agc_mutex_lock(m_mutex);
	evt_timer = timer_list[ngapSessId];
		
	// set invalid sessid in sess timer
	if (evt_timer != NULL)
	{
		//agc_timer_del_timer(evt_timer->evt);
		evt_timer->sessId = INVALIDE_ID;
		evt_timer->evt = NULL;
		//delete evt_timer;
	}
	
	timer_list[ngapSessId] = NULL;
	agc_mutex_unlock(m_mutex);
	
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "NgapSessManager::stopSessTimer ngapSessId=%d \n", ngapSessId);
}

void NgapSessManager::handleSessTimeout(void *data)
{
	sess_event_context *evt_timer =  (sess_event_context *)data;
	if (evt_timer == NULL)
		return;
	
	uint32_t ngapSessId = evt_timer->sessId;	
	RemoveNgapSess(ngapSessId);
	stopSessTimer(ngapSessId);
	
	if (evt_timer != NULL)
		delete evt_timer;
}

void NgapSessManager::SaveSourceToTarget_TransparentContainer(uint32_t ngapSessId, Ngap_SourceToTarget_TransparentContainer_t *container)
{
	Ngap_SourceToTarget_TransparentContainer_t *newContainer = NULL;
	agc_mutex_lock(m_mutex);
	if(m_containerMap.find(ngapSessId) != m_containerMap.end()) 
	{
		agc_log_printf(AGC_LOG, AGC_LOG_WARNING, "NgapSessManager::SaveSourceToTarget_TransparentContainer found unvalid data.\n");
		newContainer = m_containerMap[ngapSessId];
		free(newContainer->buf);
		free(newContainer);
		m_containerMap.erase(ngapSessId);
	}

	newContainer = (Ngap_SourceToTarget_TransparentContainer_t *)calloc(1, sizeof(Ngap_SourceToTarget_TransparentContainer_t));
	newContainer->size = container->size;
	newContainer->buf = (uint8_t *)calloc(container->size, sizeof(uint8_t));
	memcpy(newContainer->buf, container->buf, container->size);

	m_containerMap[ngapSessId] = newContainer;
	
	agc_mutex_unlock(m_mutex);
}

bool NgapSessManager::GetSourceToTarget_TransparentContainer(uint32_t ngapSessId, Ngap_SourceToTarget_TransparentContainer_t **container)
{
	agc_mutex_lock(m_mutex);
	if(m_containerMap.find(ngapSessId) == m_containerMap.end()) 
	{
		agc_mutex_unlock(m_mutex);

		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "NgapSessManager::GetSourceToTarget_TransparentContainer failed. \n");
		
		return false;
	}
	
	*container = m_containerMap[ngapSessId];
	m_containerMap.erase(ngapSessId);
	agc_mutex_unlock(m_mutex);
	
	return true;
}


void NgapSessManager::DecodeNasPdu(NgapMessage& info, const uint8_t* nas_pdu, uint32_t len)
{
#define EPD_5GMM_MESSAGE 0x2E
#define EPD_5GSM_MESSAGE 0x7E

	if (nas_pdu == NULL )
		return;

	if (nas_pdu[0] != EPD_5GMM_MESSAGE && nas_pdu[0] != EPD_5GSM_MESSAGE)
		return;

	// plain nas message
	if ((nas_pdu[1] & 0xF) == 0 && len > 2)
	{
		info.nasMessageType = nas_pdu[2];
	}
	else
	{
		if ((nas_pdu[1] & 0xF) == 3 && (nas_pdu[8] & 0xF) == 0 && len > 9)
			info.nasMessageType = nas_pdu[9];

		if (len > 6)
			info.nasSequence = nas_pdu[6];
	}
}

