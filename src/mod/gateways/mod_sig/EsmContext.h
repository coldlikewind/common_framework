#pragma once

#include <sstream>
#include <vector>
#include "mod_sig.h"
#include "RanNodeId.h"


class EsmContext 
{
public:
	uint32_t				sctp_index;
	uint32_t				stream_no;
	RanNodeId           	ranNodeId;
	uint8_t					pagingDRX;
	std::string         	gnbName;
    uint64_t              	LastActiveTime;
	std::vector<uint32_t>  	tac; 
	uint8_t					idSigGW;

	EsmContext() 
	{
		idSigGW = 0;
		sctp_index = 0;
		stream_no = 0;
	}

	~EsmContext()
	{
	}

	EsmContext(const EsmContext& a)
	{
		idSigGW = a.idSigGW;
        sctp_index = a.sctp_index;
        stream_no = a.stream_no;
        pagingDRX = a.pagingDRX;
        gnbName = a.gnbName;
        LastActiveTime = a.LastActiveTime;
		
        tac.clear();
        for(uint32_t i=0; i< a.tac.size(); i++)
        {
            tac.push_back(a.tac[i]);
        }
	}

	virtual std::string GetDesc() const
	{
		std::ostringstream os;
		os << *this;
		return os.str().c_str();
	}

	friend bool operator== (const EsmContext& info1,const EsmContext& info2)
	{
        if (info1.idSigGW != info2.idSigGW) 	return false;
        if (info1.stream_no != info2.stream_no) 	return false;
        if (info1.sctp_index != info2.sctp_index) 				return false;
        if (info1.pagingDRX != info2.pagingDRX) 	return false;
        if (info1.gnbName != info2.gnbName) 		return false;
		if(!EqualVector(info1.tac,info2.tac)) 		return false;

		return true;
	}

	friend bool operator!= (const EsmContext& info1,const EsmContext& info2)
	{
		return !(info1 == info2);
	}

	friend ostream& operator<<(ostream& os,const EsmContext& info)
	{
		os << "EsmContext :";
		os << " idSigGW = " << info.idSigGW;
		os << " sctp_index = " << info.sctp_index;
		os << " stream_no = " << info.stream_no;
		os << " ranNodeId = " << info.ranNodeId;	
		os << " gnbName = " << info.pagingDRX;
		os << " isNgSetup = " << info.gnbName;	
		os << " LastActiveTime = " << info.LastActiveTime;	
		os << " tac = ";
		for (uint32_t i =0; i<info.tac.size(); i++)
		{
			os<< info.tac[i]<<" | ";
		}

		return os;
	}

	EsmContext& operator= (const EsmContext& a)
	{
		idSigGW = a.idSigGW;
        sctp_index = a.sctp_index;
        stream_no = a.stream_no;
        pagingDRX = a.pagingDRX;
        gnbName = a.gnbName;
        LastActiveTime = a.LastActiveTime;
		ranNodeId = a.ranNodeId;
		
        tac.clear();
        for(uint32_t i=0; i< a.tac.size(); i++)
        {
            tac.push_back(a.tac[i]);
        }
		return *this;
	}

};
