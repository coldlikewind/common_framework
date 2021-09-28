#pragma once

#include "mod_sig.h"
#include "SingletonT.h"
#include "CfgStruct.h"
#include "SigAlarm.h"
#include <agc_sctp.h>
#include <list>
#include <map>

using namespace std;

#define MAX_SCTP_MSG_LEN 1400

typedef struct sctp_message_
{
	uint32_t stream_no;
    uint32_t ppid;
	int32_t len;
	char buf[MAX_SCTP_MSG_LEN];
}sctp_message_t;

typedef void (*sctp_msg_read_handler_t)(
		uint32_t sctp_index, 
		uint32_t stream_no,
    	uint32_t ppid, 
    	const char* buf, 
    	int32_t *len,
    	uint32_t idSigGW,
    	uint32_t max_stream_no);

typedef void (*sctp_error_handler_t)(
		uint32_t sctp_index);

typedef enum
{
    SCTP_NOT_CONFIG = 0,
    SCTP_CONN_INACTIVE,
    SCTP_CONN_ACTIVE
}sctp_status_e;

typedef struct sctp_context_
{
    sctp_status_e sctpStatus;
    ENUM_ALARM_STATUS alarmStatus;
	uint32_t context_id;
	agc_std_sockaddr_t local_addr;
	socklen_t local_addrlen;
	agc_std_sockaddr_t remote_addr;
	socklen_t remote_addrlen;
	agc_std_sockaddr_t local_addr2;
	socklen_t local_addrlen2;
	agc_std_sockaddr_t remote_addr2;
	socklen_t remote_addrlen2;
	std::list<sctp_message_t*>  buf; 
	sctp_msg_read_handler_t handler;
	sctp_error_handler_t err_handler;
	agc_connection_t *connection;
	agc_connection_t *connection2;
	void *context;
	agc_sctp_sock_t sock;
	agc_sctp_sock_t sock2;
	agc_sctp_config_t sctpCfg;
	bool client;
	uint32_t sctp_index;
	uint32_t idSigGW;
	uint32_t max_stream_no;
	agc_mutex_t* mutex;
	uint32_t addr_reference;
    agc_event_t *alarm_event;
}sctp_context_t;

class SctpLayerDrv 
{
	SctpLayerDrv();
	
public:
	virtual ~SctpLayerDrv();

	static SctpLayerDrv& Instance()
	{
		static SctpLayerDrv inst;
		return inst;
	}

	bool Init(sctp_msg_read_handler_t server_recv_handler, sctp_error_handler_t server_err_handler,
		sctp_msg_read_handler_t client_recv_handler, sctp_error_handler_t client_err_handler);
	void Exit();

	agc_status_t CfgAddSctp(Cfg_LteSctp &CfgSctp);
	agc_status_t CfgRmvSctp(Cfg_LteSctp &CfgSctp);
    agc_status_t CfgDisplaySctp(int sctpIndex, sctp_status_e& sctpStatus);
    agc_status_t CfgGetFreeIndex(uint32_t &index);
	
	bool AddSctpServer(agc_sctp_config_t *sctpCfg, 
		agc_std_sockaddr_t *local_addr,
		socklen_t addrlen,
		sctp_msg_read_handler_t handler,
		sctp_error_handler_t server_err_handler);
/*
	bool AddSctpClient(agc_sctp_config_t *sctpCfg, 
		agc_std_sockaddr_t *local_addr, socklen_t addrlen,
		agc_std_sockaddr_t *remote_addr, socklen_t remote_addrlen,
		sctp_msg_read_handler_t handler,
		void *context,
		uint32_t sctp_index,
		uint32_t idSigGW,
		uint32_t max_stream_no,
		sctp_error_handler_t err_handler);
	*/
	bool AddSctpClient(agc_sctp_config_t *sctpCfg, 
		uint32_t sctp_index,
		Cfg_LteSctp *sctp,
		sctp_msg_read_handler_t handler,
		void *context,
		sctp_error_handler_t err_handler);	
		

	bool AddSctpMessage(uint32_t sctp_index, 
		uint32_t stream_no,
    	uint32_t ppid, 
    	const char* buf, 
    	int32_t *len);

protected:

	static void handle_newconnection(agc_connection_t *c);
	static void handle_read(void *data);
	static void handle_write(void *data);
	static void handle_error(void *data);	
static void event_callback(void *data);

static agc_status_t event_create_callback(void *evt);

	enum { SCTP_CLIENT_TIMER_VALUE = 10000 };
	static void OnSctpClientTimer(void *data);
	void StartSctpClientTimer(uint32_t TimeOut, void *data);
	bool ReconnectSctpClient(sctp_context_t* context);

	bool SctpClientConnect(sctp_context_t* context);

	agc_sctp_sock_t clientConnect(agc_sctp_config_t *sctpCfg,  	agc_std_sockaddr_t *local_addr, socklen_t addrlen,
		agc_std_sockaddr_t *remote_addr, socklen_t remote_addrlen);

	enum { SCTPLAYER_SCTP_TIMER_VALUE = 2000 };
	enum { SCTPLAYER_ALARM_TIMER_VALUE = 10000};
	static void OnSctpTimer(void *data);
	void StartSctpTimer(uint32_t TimeOut);
	void handle_sctp_timeout();

	void SctpContextInit(sctp_context_t *context);
    int SocketCmp(agc_std_sockaddr_t left, agc_std_sockaddr_t right);
    int FindServerAddr(agc_std_sockaddr_t addr, socklen_t addr_len);
    void ReleaseClientContext(int sctpIndex);
    void ReleaseServerContext(agc_std_sockaddr_t addr, socklen_t addr_len);
    static void OnSctpAlarmTimer(void *data);
    void StartAlarmTimer(void *data);

protected:
	//agc_sctp_sock_t listenfd;
	//agc_listening_t *listening;
	//agc_connection_t *connection;
	sctp_msg_read_handler_t read_handler;
	sctp_error_handler_t err_handler;
	sctp_msg_read_handler_t client_read_handler;
	sctp_error_handler_t client_err_handler;
    sctp_msg_read_handler_t server_read_handler;
    sctp_error_handler_t server_err_handler;
	
	void *server_context;

	agc_mutex_t* m_mutex;

	uint32_t m_curContextId;

	enum { MAX_SCTP_CONTEXTS = 2000 };
	sctp_context_t *m_sctpContexts;
	std::vector<Cfg_LteSctp *> links;
	std::map<uint32_t, uint32_t> act_links;
    std::map<uint32_t, Cfg_LteSctp *> clients_links;
	vector<sctp_context_t*> serverContextList;
};

inline SctpLayerDrv& GetSctpLayerDrv()
{
	return SctpLayerDrv::Instance();
}



