#pragma once

#include <sstream>
#include "agc.h"
#include "SctpInfo.h"
#include "Context.h"

class MgwContext 
{
public:
	uint8_t          sctpIndex;
	SctpCfg     sctpCfg; //SCTP配置信息
	SctpInfo    sctpInfo;
	bool        IsConnected; //是否连接
	bool        IsPeerConnected;//备板是否连接
	uint32_t         ConnTimes;
	uint64_t			pduSess;
	uint32_t			gnb_teid;
	uint32_t			mgw_gnb_teid;	// media gateway's teid connect to GNB
	uint32_t			mgw_upf_teid;   // media gateway's teid connect to UPF
	uint32_t			upf_teid;


	virtual std::string GetDesc() const
	{
		std::ostringstream os;
		os << *this;
		return os.str().c_str();
	}

	friend bool operator== (const MgwContext& info1,const MgwContext& info2)
	{
        if (info1.sctpIndex != info2.sctpIndex) 				return false;

        if (info1.IsConnected != info2.IsConnected) 			return false;
        if (info1.IsPeerConnected != info2.IsPeerConnected) 	return false;
        if (info1.isNgSetup != info2.isNgSetup) 				return false;
		if (info1.isPeerNgSetup != info2.isPeerNgSetup) 		return false;
		if (info1.ConnTimes != info2.ConnTimes) 				return false;
		if (info1.amfName != info2.amfName) 					return false;
		if (info1.amfId != info2.amfId) 						return false;
		if (info1.relativeCapacity != info2.relativeCapacity) 	return false;
		
		return true;
	}

	friend bool operator!= (const AsmContext& info1,const AsmContext& info2)
	{
		return !(info1 == info2);
	}

	friend ostream& operator<<(ostream& os,const AsmContext& info)
	{
		os << "AsmContext :";
		os << " sctpIndex = " << info.sctpIndex;
		os << " IsConnected = " << info.IsConnected;	
		os << " IsPeerConnected = " << info.IsPeerConnected;
		os << " isNgSetup = " << info.isNgSetup;	
		os << " isPeerNgSetup = " << info.isPeerNgSetup;
		os << " ConnTimes = " << info.ConnTimes;	
		os << " amfId = " << info.amfId;
		os << " amfName = " << info.amfName;
		os << "relativeCapacity = " << info.relativeCapacity;

		return os;
	}
};

