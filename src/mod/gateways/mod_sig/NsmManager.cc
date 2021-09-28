
#include "mod_sig.h"
#include "NsmManager.h"
#include "EsmContext.h"
#include "EsmManager.h"
#include "AsmContext.h"
#include "AsmManager.h"
#include "DbManager.h"
#include "NgapSession.h"
#include "NgapSessManager.h"

#if 0
#define AGC_MAX_MEMORY_BLOCK_SIZE  500

AGC_BEGIN_EXTERN_C


typedef struct agc_memory_block_t
{
	struct agc_memory_block_t *prev;
	struct agc_memory_block_t *next;
	char data[AGC_MAX_MEMORY_BLOCK_SIZE];
}agc_memory_block_t;

typedef struct agc_memory_block_header_t
{
	agc_memory_pool_t *pool;
	agc_mutex_t* mutex;
	agc_memory_block_t *free_block;
	agc_memory_block_t *last_block;
	
}agc_memory_block_header_t;

static agc_memory_block_header_t memory_block_header;

agc_status_t agc_memory_block_add(int block_size)
{
	int i = 0;
	agc_memory_block_t *last_block = NULL;
	agc_memory_block_t *block = NULL;

	for (i = 0; i < block_size; i++)
	{
		block = (agc_memory_block_t *)malloc(sizeof(agc_memory_block_t));
		if (block == NULL)
			return AGC_STATUS_FALSE;

		if (memory_block_header.free_block == NULL)
		{
			memory_block_header.free_block = block;
			last_block = block;

			block->prev = NULL;
			block->next = NULL;
			memory_block_header.last_block = block;
		}
		else
		{
			block->prev = last_block;
			block->next = NULL;

			last_block->next = block;
			memory_block_header.last_block = block;
		}
	}

	return AGC_STATUS_SUCCESS;
}

agc_status_t agc_memory_block_init(agc_memory_pool_t *pool, int block_size)
{
	memory_block_header.pool = pool;
	memory_block_header.free_block = NULL;
	agc_mutex_init(&memory_block_header.mutex, AGC_MUTEX_NESTED, pool);

	agc_memory_block_add(block_size);
	return AGC_STATUS_SUCCESS;
}

agc_status_t agc_memory_block_shutdown()
{
	int i = 0;
	agc_memory_block_t *last_block = NULL;
	agc_memory_block_t *block = NULL;

	block = memory_block_header.free_block;
	while (block)
	{
		last_block = block;
		block = block->next;

		agc_safe_free(last_block);
	}
	
	agc_mutex_destroy(memory_block_header.mutex);
	
	return AGC_STATUS_SUCCESS;
}

agc_memory_block_t* agc_memory_block_alloc(int size)
{
	agc_memory_block_t *block = NULL;

	if (size > AGC_MAX_MEMORY_BLOCK_SIZE)
	{
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_memory_block_alloc memory size %d ecceed max %d", size, AGC_MAX_MEMORY_BLOCK_SIZE);
		return NULL;
	}
	
	agc_mutex_lock(memory_block_header.mutex);

	if (memory_block_header.free_block == NULL)
	{
		agc_memory_block_add(10);
	}

	block = memory_block_header.free_block;	
	memory_block_header.free_block = memory_block_header.free_block->next;
	block->next = NULL;

	if (memory_block_header.free_block)
	{
		memory_block_header.free_block->prev = NULL;;
	}

	// update last block node;
	memory_block_header.last_block = memory_block_header.free_block;
	
	agc_mutex_unlock(memory_block_header.mutex);
	
	return block;
}

agc_status_t agc_memory_block_free(agc_memory_block_t *block)
{
	agc_mutex_lock(memory_block_header.mutex);
	
	if (memory_block_header.last_block == NULL)
	{
		memory_block_header.last_block = block;
		memory_block_header.free_block = block;
	}
	else
	{
		memory_block_header.last_block->next = block;
		block->prev = memory_block_header.last_block;
		memory_block_header.last_block = block;
	}
	
	agc_mutex_unlock(memory_block_header.mutex);
}


AGC_END_EXTERN_C

#endif

NsmManager::NsmManager()
{
	m_exited = false;
	m_isInited = false;

	agc_memory_pool_t *module_pool = agcsig_get_sig_memory_pool();
	agc_mutex_init(&m_mutex, AGC_MUTEX_NESTED, module_pool);

}

NsmManager::~NsmManager()
{
	for (std::vector<Cfg_LteSctp *>::iterator it = links.begin(); it != links.end();) {
        delete *it;
        it = links.erase(it);
    }
}

bool NsmManager::Init()
{
	m_exited = false;
	m_isInited = false;

	//agc_memory_pool_t *module_pool = agcsig_get_sig_memory_pool();
	//agc_memory_block_init(module_pool, 1000);

	GetSctpLayerDrv().Init(NsmManager::handle_gnb_sctp_msg_read,
		NsmManager::handle_gnb_sctp_err,
		NsmManager::handle_amf_sctp_msg_read,
		NsmManager::handle_amf_sctp_err);	
	GetDbManager().QuerySctpLinks(links);
	// start sctp timer
	StartSctpTimer(NSM_SCTP_TIMER_VALUE);
	
	memset(m_recvAmfCallback, 0 , MAX_PDU_KEY * sizeof(NsmAppCallback));
	memset(m_recvGnbCallback, 0 , MAX_PDU_KEY * sizeof(NsmAppCallback));

	m_isInited = true;
	return true;
}

void NsmManager::WaitForExit()
{
	agc_status_t status = AGC_STATUS_SUCCESS;

	if(!m_isInited) return;
}

void NsmManager::Exit()
{
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "NsmManager::Exit Exited!\n");

	if(!m_isInited) 
		return;

	GetSctpLayerDrv().Exit();

	m_exited = true;

	//agc_memory_block_shutdown();
}

void NsmManager::handle_sctp_timeout()
{
  	for(int i = 0; i < links.size(); i++)
	{
		uint32_t sctp_index = links[i]->idSctpIndex;
		if (act_links.find(sctp_index) != act_links.end())
		{
			continue;
		}
		
		if (links[i]->WorkType == SCTP_WT_CLIENT){
			GetAsmManager().AddAmf(links[i]->idSigGW, links[i]->idSctpIndex, links[i]->Priority);
		}
		act_links[sctp_index] = links[i]->idSctpIndex;
	}

}

void NsmManager::StartSctpTimer(uint32_t TimeOut)
{	
	if (links.size() != act_links.size()) {
		agc_event_t *new_event = NULL;
		if (agc_event_create_callback(&new_event, EVENT_NULL_SOURCEID, NULL, &NsmManager::OnSctpTimer) != AGC_STATUS_SUCCESS) {
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "NsmManager::StartSctpTime create timer agc_event_create [fail].\n");
			return;
		} 
		agc_timer_add_timer(new_event, TimeOut);
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "NsmManager::StartSctpTime add new timer \n");
	}
}

void NsmManager::OnSctpTimer(void *data)
{
	GetNsmManager().handle_sctp_timeout();

	// restart sctp timer
	//GetNsmManager().StartSctpTimer(NSM_SCTP_TIMER_VALUE);
}

void NsmManager::handle_amf_sctp_err(
	uint32_t sctp_index)
{
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "NsmManager::handle_amf_sctp_err sctp_index=%d\n", sctp_index );
	GetAsmManager().Restart(sctp_index);;
}

void NsmManager::handle_amf_sctp_msg_read(
		uint32_t sctp_index, 
		uint32_t stream_no,
    	uint32_t ppid, 
    	const char* buf, 
    	int32_t *len,
    	uint32_t idSigGW,
    	uint32_t max_sctp_num)
{
	if (MOD_SIG_SCTP_NGAP_PPID != ppid)
		return;

	GetNsmManager().handle_amf_ngap_msg(sctp_index, stream_no, ppid, buf, len, idSigGW, max_sctp_num);

}

void NsmManager::handle_amf_ngap_msg(
		uint32_t sctp_index, 
		uint32_t stream_no,
    	uint32_t ppid, 
    	const char* buf, 
    	int32_t *len,
    	uint32_t idSigGW,
    	uint32_t max_sctp_num)
{
	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "NsmManager::handle_amf_sctp_msg_read len=%d  stream=%d sctp_index=%d ppid=%d  idSigGW=%d\n",
		*len, stream_no, sctp_index, ppid, idSigGW);

	NgapMessage info;
	info.stream_no = stream_no;
	info.sockSource = sctp_index; 
	info.sockTarget = sctp_index; 
	info.amfId = GetAsmManager().GetAmfIdBySock(sctp_index);
	info.idSigGW = idSigGW;
	info.max_sctp_stream = max_sctp_num;

	// socket is broken, reconnecting ...
	/*if (*len < 0)
	{	
		GetAsmManager().Clear(info.amfId);
		return;
	}
*/
	NgapProtocol decoder;
	if (decoder.Decode(buf, len, info))
	{
		ProcessAmfMsg(info);
		
		agc_log_printf(AGC_LOG, AGC_LOG_INFO, "NsmManager::handle_amf_ngap_msg receive AMF msg ngap_sessId=%d ,ngap_msg(%d %d) nas(%d %d) len=%d sctp_index=%d\n", 
			info.sessId, info.PDUChoice, info.ProcCode, info.nasMessageType, info.nasSequence, *len, sctp_index);
	}
}

void NsmManager::handle_gnb_sctp_err(
	uint32_t sctp_index)
{
	GetNgapSessManager().DeleteAllGnbSession(sctp_index);
}

void NsmManager::handle_gnb_sctp_msg_read(
		uint32_t sctp_index, 
		uint32_t stream_no,
    	uint32_t ppid, 
    	const char* buf, 
    	int32_t *len,
    	uint32_t idSigGW,
    	uint32_t max_sctp_num)
{
	if (MOD_SIG_SCTP_NGAP_PPID != ppid)
		return;

	GetNsmManager().handle_gnb_ngap_msg(sctp_index, stream_no, ppid, buf, len, idSigGW, max_sctp_num);
}

void NsmManager::handle_gnb_ngap_msg(
		uint32_t sctp_index, 
		uint32_t stream_no,
    	uint32_t ppid, 
    	const char* buf, 
    	int32_t *len,
    	uint32_t idSigGW,
    	uint32_t max_sctp_num)
{

	NgapMessage info;
	info.stream_no = stream_no;
	info.sockSource = sctp_index; 
	info.sockTarget = sctp_index; 
	info.idSigGW = idSigGW;
	info.max_sctp_stream = max_sctp_num;

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "NsmManager::handle_gnb_ngap_msg len=%d  stream=%d sctp_index=%d ppid=%d idSigGW=%d max_sctp_stream=%d\n",
		*len, stream_no, sctp_index, ppid, idSigGW, info.max_sctp_stream);

	if (*len <=0)
		return;
	
	EsmContext *esm;
	if (GetEsmManager().GetEsmContext(sctp_index, &esm) == true)
	{
		info.ranNodeId = esm->ranNodeId;
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "NsmManager::handle_gnb_ngap_msg success sctp_index=%d,ranNodeId:%s ----- =%s\n",
			sctp_index,
			info.ranNodeId.toString().c_str(),
			esm->ranNodeId.toString().c_str());
		/*if (esm->stream_no != stream_no && stream_no != 0)
		{
			esm->stream_no = stream_no;
			GetEsmManager().Update(info, esm);
		}*/
	}

	NgapProtocol decoder;
	if (decoder.Decode(buf, len, info))
	{
		ProcessGnbMsg(info);
		
		agc_log_printf(AGC_LOG, AGC_LOG_INFO, "NsmManager::handle_gnb_ngap_msg receive GNB msg ngap_sessId=%d ,ngap_msg(%d %d) nas(%d %d) len=%d sctp_index=%d\n", 
			info.sessId, info.PDUChoice, info.ProcCode, info.nasMessageType, info.nasSequence, *len, sctp_index);
	}
	
}

bool NsmManager::DecodeErrorProcess(NgapMessage& info)
{
    if (info.PDUChoice == Ngap_NGAP_PDU_PR_initiatingMessage && info.ProcCode == Ngap_ProcedureCode_id_ErrorIndication)
    {
        return true;
    }

	

    return true;
}

void NsmManager::RegisterUpLayerRecv(unsigned short pduChoise,unsigned short procedureCode,NsmAppCallback  callback)
{	
	uint32_t infoKey = GetKey(pduChoise, procedureCode);
	if (infoKey >= MAX_PDU_KEY)
		return;
	
	m_recvAmfCallback[infoKey] = callback;
}

void NsmManager::RegisterDownLayerRecv(unsigned short pduChoise,unsigned short procedureCode,NsmAppCallback  callback)
{
	uint32_t infoKey = GetKey(pduChoise, procedureCode);
	if (infoKey >= MAX_PDU_KEY)
		return;

	m_recvGnbCallback[infoKey] = callback;
}


void NsmManager::RegisterUpLayerSend(unsigned short pduChoise,unsigned short procedureCode,NsmAppCallback  callback)
{
    m_amfNgapPro.RegisterSend(pduChoise,procedureCode,callback);
}

void NsmManager::RegisterDownLayerSend(unsigned short pduChoise,unsigned short procedureCode,NsmAppCallback  callback)
{
    m_gnbNgapPro.RegisterSend(pduChoise,procedureCode,callback);
}


void NsmManager::RegisterUpLayerSendError(NsmAppCallback  callback)
{
    m_amfNgapPro.RegisterSendError(callback);
}

void NsmManager::RegisterDownLayerSendError(NsmAppCallback  callback)
{
    m_gnbNgapPro.RegisterSendError(callback);
}

void NsmManager::RegisterUpLayerRecvError(NsmAppCallback  callback)
{
    m_amfNgapPro.RegisterRecvError(callback);
}

void NsmManager::RegisterDownLayerRecvError(NsmAppCallback  callback)
{
    m_gnbNgapPro.RegisterRecvError(callback);
}


bool NsmManager::status_sctp(uint32_t sctpindex)
{
		if (act_links.find(sctpindex) != act_links.end())
			return true;
		else
			return false;
}


bool NsmManager::SendUpLayerSctp(NgapMessage& info)
{
	if (!m_isInited)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "NsmManager::SendUpLayerSctp LayerManger init did not finish\n");
		return false;
	}

	int32_t len = 0;
	char *buf = new char [2000];
	NgapProtocol encoder;
	if (!encoder.Encode(info, buf, &len))
	{
		delete [] buf;
		return false;
	}
	
	GetSctpLayerDrv().AddSctpMessage(info.sockTarget, info.stream_no, MOD_SIG_SCTP_NGAP_PPID, buf, &len);
	
	delete [] buf;
	
	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "NsmManager::SendUpLayerSctp        send AMF msg ngap_sessId=%d ,ngap_msg(%d %d) nas(%d %d) len=%d stream_no=%d sctp_index=%d\n",
		info.sessId, info.PDUChoice, info.ProcCode, info.nasMessageType, info.nasSequence, len, info.stream_no, info.sockTarget);
	return true;
}


bool NsmManager::SendDownLayerSctp(NgapMessage& info)
{
	agc_time_t time_start = agc_time_now();
	uint32_t sctp_index = info.sockTarget;
	if (!m_isInited)
	{
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "NsmManager::SendDownLayerSctp LayerManger init did not finish\n");
		return false;
	}

	// TODO: ngap message buffer size
	char *buf = new char [2000];
	//agc_memory_block_t *block = agc_memory_block_alloc(AGC_MAX_MEMORY_BLOCK_SIZE);

	int32_t len = 0;
	NgapProtocol encoder;
	if (!encoder.Encode(info, buf, &len))
	{
		delete [] buf;
		//agc_memory_block_free(block);
		return false;
	}

	GetSctpLayerDrv().AddSctpMessage(sctp_index, info.stream_no, MOD_SIG_SCTP_NGAP_PPID, buf, &len);
	
	delete [] buf;
	//agc_memory_block_free(block);

	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "NsmManager::SendDownLayerSctp      send GNB msg ngap_sessId=%d ,ngap_msg(%d %d) nas(%d %d) len=%d stream_no=%d sctp_index=%d\n",
		info.sessId, info.PDUChoice, info.ProcCode, info.nasMessageType, info.nasSequence, len, info.stream_no, sctp_index);
	
	return true;
}
