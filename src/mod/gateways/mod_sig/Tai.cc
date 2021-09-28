/****************************************************************************************************
 * Copyright (c) 2011 by ASTRI Corp.
 ****************************************************************************************************
 * @file: Tai.cpp
 *
 * @description: Implemention source for IMSI wrapper class
 *
 * @date: Mar 28, 2011
 * @author: root
 * @last modified by:
 *        $Author: alexmui $  $DateTime: 2012/03/29 00:44:33 $
 ****************************************************************************************************
 */
#include <sstream>
#include <iostream>
//#include <stdio.h>
//#include <stdlib.h>
#include <string>
//#include "comm/Coder.h"
#include "Tai.h"

using namespace std;
//using namespace SYS;

/**
 * Default Constructor
 */
Tai::Tai() : m_TAI_MCC(INVALID_MCCMNC),m_TAI_MNC(INVALID_MCCMNC),m_TAI_TAC(INVALID_TAC), m_TaiValid(true)
{
    m_TAI_Ascii = "";
    m_TAI_Hex = "";
}


bool Tai::isTaiValid() const
{
	return m_TaiValid;
}

void Tai::doEncode()
{
 //   uint8_t bufHex[TAI_LENGTH];
	//uint8_t bufEncode[TAI_LENGTH];

	//bufHex[0] = (m_TAI_MCC >> 8) + (m_TAI_MCC & 0xF0);
	//if( (m_TAI_MNC >> 8) == 0) //MNC is 2 digit
	//{
	//	bufHex[1] = (m_TAI_MCC & 0x0F) + 0xF0;
	//}
	//else
	//{
	//	bufHex[1] = (m_TAI_MCC & 0x0F) + ((m_TAI_MNC & 0x0F) << 4);
	//}
	//bufHex[2] = (m_TAI_MNC >> 8) + (m_TAI_MNC & 0xF0);

	//Coder<uint16_t>::encode(m_TAI_TAC, bufEncode, 0);
	//bufHex[3] = bufEncode[0];
	//bufHex[4] = bufEncode[1];

	//m_TAI_Ascii.clear();
	//toAsciiStr(bufHex, m_TAI_Ascii);
	//m_TAI_Hex.clear();
	//m_TAI_Hex.insert(0, (const char *)bufHex, TAI_LENGTH);
}

/*
 * Method : getFqdn()
 *
 * Purpose: This method returns the std::string which contains the FQDN of the TAI.
 *
 * e.g. <TAI>  = <MCC><MNC><TAC>
 *             = <311><990><0000 0100 1000 0000>
 *      <FQDN> = "tac-lb80.tac-hb04.tac.epc.mnc990.mcc311.3gppnetwork.org"
 */
void Tai::getFqdn(std::string& fqdn)
{
   uint16_t pos = 0;
   string tac_lb_str("tac-lb");
   string tac_hb_str(".tac-hb");
   string tac_epc_mnc(".tac.epc.mnc");
   string tac_mcc(".mcc");
   string fqdn_suffix(".3gppnetwork.org");
   uint8_t tac_lb = m_TAI_TAC & 0x00ff;
   uint8_t tac_hb = (m_TAI_TAC & 0xff00) >> 8;

   char mccBuf[8];
   sprintf(mccBuf,"%x",m_TAI_MCC);
   string mcc(mccBuf);

   char mncBuf[8];
   sprintf(mncBuf,"%x",m_TAI_MNC);
   string mnc(mncBuf);
   if ( mnc.size() < 2 )
      mnc.insert(0, "0", 1);

   char lbBuf[4];
   sprintf(lbBuf,"%x",tac_lb);
   string lb(lbBuf);
   if ( lb.size() < 2 )
      lb.insert(0, "0", 1);

   char hbBuf[4];
   sprintf(hbBuf,"%x",tac_hb);
   string hb(hbBuf);
   if ( hb.size() < 2 )
      hb.insert(0, "0", 1);

   fqdn.clear();

   fqdn.insert(pos, tac_lb_str);
   pos += tac_lb_str.size();

   fqdn.insert(pos, lb);
   pos += lb.size();

   fqdn.insert(pos, tac_hb_str);
   pos += tac_hb_str.size();

   fqdn.insert(pos, hb);
   pos += hb.size();

   fqdn.insert(pos, tac_epc_mnc);
   pos += tac_epc_mnc.size();

   fqdn.insert(pos, mnc);
   pos += mnc.size();

   fqdn.insert(pos, tac_mcc);
   pos += tac_mcc.size();

   fqdn.insert(pos, mcc);
   pos += mcc.size();

   fqdn.insert(pos, fqdn_suffix);
}

//void Tai::buildLabels(std::string fqdn)
//{
//   size_t pos;
//   string stemp;
//   Bytes lLabel;
//   uint8_t lsize = 0;
//
//   pos = fqdn.find(".");
//
//   while ( pos != string::npos )
//   {
//      uint8_t size = 0;
//      Bytes bLabel;
//      string sLabel;
//      sLabel.clear();
//      sLabel.insert(0, fqdn, 0, pos);
//      size = sLabel.size();
//      bLabel.resize(size + 1);
//      bLabel.writeuint8_t(size, 0);
//      bLabel.write((unsigned char*)sLabel.c_str(), 0, 1, size);
//      m_labels.push_back(bLabel);
//      stemp.clear();
//      stemp.insert(0, fqdn, pos+1, fqdn.size());
//      fqdn.clear();
//      fqdn = stemp;
//      pos = fqdn.find(".");
//   }
//   lsize = stemp.size();
//   lLabel.resize(lsize + 1);
//   lLabel.writeuint8_t(lsize, 0);
//   lLabel.write((unsigned char*)stemp.c_str(), 0, 1, lsize);
//   m_labels.push_back(lLabel);
//}

//void Tai::getFqdnLabels(SYS::Bytes& dname)
//{
//   uint16_t size = 0;
//   uint16_t pos = 0;
//   string fqdn;
//
//   // get FQDN
//   getFqdn(fqdn);
//
//   // build label list in Bytes
//   buildLabels(fqdn);
//
//   // calculate the size for dname
//   for (SYS::List<SYS::Bytes>::iterator iter = m_labels.first(); !iter.eol(); ++iter)
//   {
//      Bytes label = *iter;
//      size += label.size();
//   }
//
//   // prepare dname
//   dname.resize(size);
//   for (SYS::List<SYS::Bytes>::iterator iter = m_labels.first(); !iter.eol(); ++iter)
//   {
//      Bytes label = *iter;
//      dname.write(label.addr(), 0, pos, label.size());
//      pos += label.size();
//   }
//}

void Tai::getApnOIfromTai(std::string& apnoi)
{
   //uint16_t size = 0;
   uint32_t pos = 0;
   string fqdn;
   string stemp;

   // get FQDN
   getFqdn(fqdn);

   pos = fqdn.find(".mnc");
   if ( pos != string::npos )
   {
      stemp.clear();
      stemp.insert(0, fqdn, pos+1, fqdn.size());
   }

   pos = stemp.find(".3gppnetwork");
   if ( pos != string::npos )
   {
      apnoi.insert(0, stemp, 0, pos);
      apnoi.insert(pos, ".gprs", 0, 5);
   }
}

string Tai::toString() const
{
	stringstream ss;
	ss << "Tai:(";
	ss << " MCC:" << m_TAI_MCC;
	ss << " MNC:" << m_TAI_MNC;
	ss << " TAC:" << m_TAI_TAC;
	ss << ")";
	return ss.str();
}

bool operator== (const Tai &a, const Tai &b)
{
	if (!a.m_TaiValid || !b.m_TaiValid)
      return false;

   return ( (a.m_TAI_MCC == b.m_TAI_MCC) &&
            (a.m_TAI_MNC == b.m_TAI_MNC) &&
            (a.m_TAI_TAC == b.m_TAI_TAC) &&
            (a.m_TAI_Ascii == b.m_TAI_Ascii) &&
            (a.m_TAI_Hex == b.m_TAI_Hex) );
}

bool operator!= (const Tai &a, const Tai &b)
{
	return !(a == b);
}


ostream& operator<< (ostream& os,const Tai& a)
{
	os << "Tai:(";
	os << " MCC:" << a.m_TAI_MCC;
	os << " MNC:" << a.m_TAI_MNC;
	os << " TAC:" << a.m_TAI_TAC;
	os << ")";
	return os;
}

