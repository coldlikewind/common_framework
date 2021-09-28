#pragma once

#include <sstream>
#include "mod_sig.h"

using namespace std;

class AsmContext
{
public:
	uint32_t				sctp_index;
	uint32_t				stream_no;
	bool         isNgSetup; 
	bool         isPeerNgSetup; 
	uint8_t      relativeCapacity;
	uint32_t     ConnTimes;
	std::string	 amfName;
	uint32_t     amfId;
	uint8_t		 idSigGW;
	uint8_t		 priority;

	virtual std::string GetDesc() const
	{
		std::ostringstream os;
		os << *this;
		return os.str().c_str();
	}

	friend bool operator== (const AsmContext& info1,const AsmContext& info2)
	{
        if (info1.idSigGW != info2.idSigGW) 					return false;
        if (info1.sctp_index != info2.sctp_index) 				return false;
        if (info1.stream_no != info2.stream_no) 				return false;
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
		os << " idSigGW = " << info.idSigGW;	
		os << " sctp_index = " << info.sctp_index;	
		os << " stream_no = " << info.stream_no;	
		os << " isNgSetup = " << info.isNgSetup;	
		os << " isPeerNgSetup = " << info.isPeerNgSetup;
		os << " ConnTimes = " << info.ConnTimes;	
		os << " amfId = " << info.amfId;
		os << " amfName = " << info.amfName;
		os << "relativeCapacity = " << info.relativeCapacity;

		return os;
	}

	AsmContext& operator= (const AsmContext& a)
	{
		idSigGW = a.idSigGW;
        sctp_index = a.sctp_index;
        stream_no = a.stream_no;
        isNgSetup = a.isNgSetup;
        isPeerNgSetup = a.isPeerNgSetup;
        ConnTimes = a.ConnTimes;
        amfId = a.amfId;
        amfName = a.amfName;
        relativeCapacity = a.relativeCapacity;

		return *this;
	}

	AsmContext()
	{
		idSigGW = 0;
        sctp_index = 0;
        stream_no = 0;
        isNgSetup = false;
        isPeerNgSetup = false;
        ConnTimes = 0;
        amfId = 0;
        amfName = "";
        relativeCapacity = 0;
	}

	AsmContext(const AsmContext& a)
	{
		idSigGW = a.idSigGW;
        sctp_index = a.sctp_index;
        stream_no = a.stream_no;
        isNgSetup = a.isNgSetup;
        isPeerNgSetup = a.isPeerNgSetup;
        ConnTimes = a.ConnTimes;
        amfId = a.amfId;
        amfName = a.amfName;
        relativeCapacity = a.relativeCapacity;
	}
};