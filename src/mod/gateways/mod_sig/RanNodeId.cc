#include <sstream>
#include "RanNodeId.h"
#include "SingletonT.h"


RanNodeId::RanNodeId() : 
	m_eRanNode_Type(RanNodeId::RNT_GNB_ID),
	m_eNgEnbType(RanNodeId::RNET_NULL),
	m_eRanNode_ID(INVALID_RANNODEID)
{
	plmn_id.mcc1 = 0xF;
	plmn_id.mcc2 = 0xF;
	plmn_id.mcc3 = 0xF;
	plmn_id.mnc1 = 0xF;
	plmn_id.mnc2 = 0xF;
	plmn_id.mnc3 = 0xF;
}

/**
 * Construct the RanNodeId from another RanNodeId.
 */
RanNodeId::RanNodeId(const RanNodeId& copy) : 
	plmn_id(copy.plmn_id),
	m_eRanNode_Type(copy.m_eRanNode_Type),
	m_eNgEnbType(copy.m_eNgEnbType),
	m_eRanNode_ID(copy.m_eRanNode_ID)
{
	
}

/*
 * Assignment Operator
 */
RanNodeId& RanNodeId::operator= (const RanNodeId& copy)
{
	plmn_id = copy.plmn_id;
	m_eRanNode_Type = copy.m_eRanNode_Type;
	m_eNgEnbType = copy.m_eNgEnbType;
	m_eRanNode_ID = copy.m_eRanNode_ID;

	return *this;
}

const string RanNodeId::toString() const
{
	std::stringstream ss;
	ss << "RanNodeId:(";
	ss << " MCC1:" << plmn_id.mcc1;
	ss << " MCC2:" << plmn_id.mcc2;
	ss << " MCC3:" << plmn_id.mcc3;
	ss << " MNC1:" << plmn_id.mnc1;
	ss << " MNC2:" << plmn_id.mnc2;
	ss << " MNC3:" << plmn_id.mnc3;

	switch(m_eRanNode_Type)
	{
	case RanNodeId::RNT_GNB_ID:
		ss << " Type:GNB";
		break;
	case RanNodeId::RNT_NG_ENB_ID:
		ss << " Type:NGENB";
		break;
	case RanNodeId::RNT_N3IWF_ID:
		ss << " Type:N3IWF";
		break;
	}
	switch(m_eNgEnbType)
	{
	case RanNodeId::RNET_MACRO_NG_ENB:
		ss << " NgEnbType:MACRO";
		break;
	case RanNodeId::RNET_SHORT_NG_ENB:
		ss << " NgEnbType:SHORT";
		break;
	case RanNodeId::RNET_LONG_NG_ENB:
		ss << " NgEnbType:LONG";
		break;
	case RanNodeId::RNET_NULL:
		ss << " NgEnbType:NULL";
		break;
	}

	ss << " ID:" << m_eRanNode_ID;
	ss << ")";
	return ss.str();
}

uint64_t RanNodeId::touint64_t() const
{
    uint64_t nTrait = 0;
    nTrait = (uint64_t)plmn_id.mcc1 << 58
		| (uint64_t)plmn_id.mcc2 << 54
		| (uint64_t)plmn_id.mcc3 << 50
		| (uint64_t)plmn_id.mnc1 << 46
		| (uint64_t)plmn_id.mnc2 << 42
		| (uint64_t)plmn_id.mnc3 << 38
		| ((uint64_t)m_eRanNode_Type << 36) 
		| ((uint64_t)m_eNgEnbType << 32);

    nTrait += (m_eRanNode_ID & 0xFFFFFFFF);
    return nTrait;
}

bool operator== (const RanNodeId &a, const RanNodeId &b)
{
    
	return ( (a.plmn_id.mcc1 == b.plmn_id.mcc1) &&
            (a.plmn_id.mcc2 == b.plmn_id.mcc2) &&
            (a.plmn_id.mcc3 == b.plmn_id.mcc3) &&
            (a.plmn_id.mnc1 == b.plmn_id.mnc1)  &&
            (a.plmn_id.mnc2 == b.plmn_id.mnc2) &&
            (a.plmn_id.mnc3 == b.plmn_id.mnc3) &&
            (a.m_eRanNode_Type == b.m_eRanNode_Type) ) &&
            (a.m_eNgEnbType == b.m_eNgEnbType) &&
            (a.m_eRanNode_ID == b.m_eRanNode_ID) ;
    
    //return (a.touint64_t() == b.touint64_t());
}

bool operator!= (const RanNodeId &a, const RanNodeId &b)
{
	return !(a == b);
}

ostream& operator<< (ostream& os,const RanNodeId& a)
{
	os << "RanNodeId:(";
	os << " MCC1:" << a.plmn_id.mcc1;
	os << " MCC2:" << a.plmn_id.mcc2;
	os << " MCC3:" << a.plmn_id.mcc3;
	os << " MNC1:" << a.plmn_id.mnc1;
	os << " MNC2:" << a.plmn_id.mnc2;
	os << " MNC3:" << a.plmn_id.mnc3;
	switch(a.m_eRanNode_Type)
	{
	case RanNodeId::RNT_GNB_ID:
		os << " Type:GNB";
		break;
	case RanNodeId::RNT_NG_ENB_ID:
		os << " Type:NGENB";
		break;
	case RanNodeId::RNT_N3IWF_ID:
		os << " Type:N3IWF";
		break;
	}
	switch(a.m_eNgEnbType)
	{
		case RanNodeId::RNET_MACRO_NG_ENB:
			os << " NgEnbType:MACRO";
			break;
		case RanNodeId::RNET_SHORT_NG_ENB:
			os << " NgEnbType:SHORT";
			break;
		case RanNodeId::RNET_LONG_NG_ENB:
			os << " NgEnbType:LONG";
			break;
		case RanNodeId::RNET_NULL:
			os << " NgEnbType:NULL";
			break;
	}

	os << " ID:"<< a.m_eRanNode_ID;
	os << ")";
	return os;
}

bool operator<(const RanNodeId& a, const RanNodeId & b)
{
    /*
	if (a.m_eRanNode_GID_MCC < b.m_eRanNode_GID_MCC) return false;

	if (a.m_eRanNode_GID_MNC < b.m_eRanNode_GID_MNC) return false;

	if (a.m_eNgEnbType < b.m_eNgEnbType) return false;

	return (a.m_eRanNode_ID < b.m_eRanNode_ID);
    */
    return (a.touint64_t() < b.touint64_t());
}
