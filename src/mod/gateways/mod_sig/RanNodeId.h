#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include "mod_sig.h"
#include "PlmnUtil.h"

using namespace std;


class RanNodeId
{
public:
	typedef enum RanNodeType
	{
		RNT_GNB_ID		= 0x01,
		RNT_NG_ENB_ID	= 0x02,
		RNT_N3IWF_ID	= 0x03,
	}RanNodeType;

	typedef enum RanNgEnbType
	{
		RNET_MACRO_NG_ENB = 0x01,
		RNET_SHORT_NG_ENB = 0x02,
		RNET_LONG_NG_ENB  = 0x03,
		RNET_NULL = 0xF,		
	}RanNgEnbType;
public:
	/**
	* Default Constructor
	*/
	RanNodeId();

	/**
	* De-constructor
	*/
	~RanNodeId(){}

	/**
	* Construct the RanNodeId from another RanNodeId.
	*/
	RanNodeId(const RanNodeId& copy);

	/*
	* Assignment Operator
	*/
	RanNodeId& operator=(const RanNodeId& copy);

	/*
	* == Operator
	*/
	//bool operator==(const RanNodeId& copy) const;

	friend bool operator== (const RanNodeId &a, const RanNodeId &b);

	friend bool operator!= (const RanNodeId &a, const RanNodeId &b);

	friend ostream& operator<< (ostream& os,const RanNodeId& a);

	friend bool operator<(const RanNodeId& a, const RanNodeId & b);

	/*
	* Method : getRanNodeIdMCC() and setRanNodeIdMCC()
	* Purpose: These methods are used to get and set the Mobile Country Code of the eNodeB Global ID
	*/
	/*inline const uint16_t getRanNodeIdMCC()
	{
	 return (m_eRanNode_GID_MCC);
	}

	inline void setRanNodeIdMCC(uint16_t mcc)
	{
	 m_eRanNode_GID_MCC = mcc;
	}
*/
	inline plmn_id_t getRanNodePlmn()
	{
		return this->plmn_id;
	}
	
	inline void setRanNodePlmn(    plmn_id_t plmn_id) 
	{
		this->plmn_id = plmn_id;
	}
	/*
	* Method : getRanNodeIdMNC() and setRanNodeIdMNC()
	* Purpose: These methods are used to get and set the Mobile Network Code of the eNodeB Global ID
	*/
	/*inline const uint16_t getRanNodeIdMNC()
	{
	 return (m_eRanNode_GID_MNC);
	}

	inline void setRanNodeIdMNC(uint16_t mnc)
	{
		m_eRanNode_GID_MNC = mnc;
	}
*/
	/*
	* Method : getENodeID() and setENodeID()
	* Purpose: These methods are used to get and set the eNodeB ID of the eNodeB Global ID
	*/
	inline const uint32_t getRanNodeID()
	{
		return (m_eRanNode_ID);
	}

	inline void setRanNodeID(uint32_t gid)
	{
		m_eRanNode_ID = gid;
	}

	/*
	* Method : getENodeType() and setENodeType()
	* Purpose: These methods are used to get and set the eNodeB type (Macro or Home) of the eNodeB Global ID
	*/
	inline const RanNodeType getRanNodeType()
	{
		return (m_eRanNode_Type);
	}

	inline void setRanNodeType(RanNodeType type)
	{
		m_eRanNode_Type = type;
	}

	inline const RanNgEnbType getRanNgEnbType()
	{
		return (m_eNgEnbType);
	}

	inline void setRanNgEnbType(RanNgEnbType type)
	{
		m_eNgEnbType = type;
	}

	inline void reset() {
		m_eRanNode_Type = RanNodeId::RNT_GNB_ID;
		m_eNgEnbType = RanNodeId::RNET_NULL;
		m_eRanNode_ID = INVALID_RANNODEID;
		plmn_id.mcc1 = 0xF;
		plmn_id.mcc2 = 0xF;
		plmn_id.mcc3 = 0xF;
		plmn_id.mnc1 = 0xF;
		plmn_id.mnc2 = 0xF;
		plmn_id.mnc3 = 0xF;
	}

	const string toString() const;
	uint64_t touint64_t() const;
private:
	//uint16_t m_eRanNode_GID_MCC;
	//uint16_t m_eRanNode_GID_MNC;
    plmn_id_t plmn_id;

	RanNodeType m_eRanNode_Type;
	RanNgEnbType m_eNgEnbType;
	uint32_t m_eRanNode_ID;			// 20 bits value for Macro eND ID and 28 bits value for Home eNB ID
};
