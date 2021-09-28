#pragma once

#include <vector>
#include "mod_sig.h"
#include "SingletonT.h"
#include "CfgStruct.h"

using namespace std;

class DbManager : public SingletonT<DbManager, DbManager>
{
protected:
	DbManager(void);

public:
	~DbManager(void);

	bool QueryMediaGW(std::vector<stCfg_MEDIA_DATA> &medias);

	bool QueryVgwParam(uint8_t idSigGW, stCfg_vGWParam_API &param);
	bool QueryBplmn(uint8_t idSigGw, std::vector<stCfg_BPlmn_API> &plmns);
	bool QueryNssai(uint8_t idNssai, std::vector<stCfg_NSSAI_API> &nnsais);
	bool QueryPlmnNssai(uint8_t idBplmn, std::vector<stCfg_PLMN_NSSAI_DATA> &nnsais);
	bool QueryPlmnNssai(uint8_t idSigGw, uint8_t idBplmn, std::vector<stCfg_PLMN_NSSAI_DATA> &nnsais);

	bool QueryTacs(uint8_t idSigGw, std::vector<stCfg_TA_DATA> &tacs);
	bool QueryTacPlmn(uint8_t idTA, std::vector<stCfg_TA_PLMN_DATA> &plmns);
	bool QueryTacPlmn(uint8_t idSigGw, uint8_t idTA, std::vector<stCfg_TA_PLMN_DATA> &plmns);
	
	bool QueryGuami(uint8_t idSigGw, std::vector<stCfg_GUAMI_API> &guamis);

	//bool QuerySctpLinksByRemoteAddr(agc_std_sockaddr_t &remote_addr, std::vector<Cfg_LteSctp *> &links);
	bool QuerySctpLinksByLocalAddr(agc_std_sockaddr_t &local_addr, std::vector<Cfg_LteSctp *> &links);
	bool QuerySctpLinksByidSigGw(uint8_t idSigGw, std::vector<Cfg_LteSctp *> &links);
	bool QuerySctpLinks(std::vector<Cfg_LteSctp *> &links);
	bool QuerySctpServerLinks(std::vector<Cfg_LteSctp *> &links);
	bool QuerySctpLinksByRemoteAddr(agc_std_sockaddr_t &remote_addr, uint32_t &sctp_index, uint32_t &iSigGW, uint32_t &sctp_num);
    bool QuerySctpLinkById(uint32_t Id, vector<Cfg_LteSctp *> &links);
	void StringToIpAddr(const char* addr, Cfg_IPType iptype, uint16_t LocalPort, agc_std_sockaddr_t &IP, socklen_t &IPLen);

protected:

	bool InsertSctpLinks(agc_db_stmt_t *stmt, std::vector<Cfg_LteSctp *> &links);

	agc_mutex_t* m_mutex;

	friend class SingletonT<DbManager, DbManager>;
	static const char* SingletonName(){ return "DbManager"; } //From SingletonT<InstanceT,classT> 
	static int SingletonPriority(){ return 1000; }     //From SingletonT<InstanceT,classT> 
};


inline DbManager& GetDbManager()
{
	return DbManager::Instance();
}
	
