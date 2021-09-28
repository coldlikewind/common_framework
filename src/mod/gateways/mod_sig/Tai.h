

#ifndef _SYS_TAI_H_
#define _SYS_TAI_H_

#include <list>
#include <string>
#include "mod_sig.h"
using namespace std;

/**
 * @class Tai
 * @brief Tai wrapper class.
 */
class Tai
{
public:
   static const int TAI_LENGTH = 5;	// BYTES or OCTETS

   /*
    * Method : toAsciiStr()
    * Purpose: This method converts the octet stream to the class property
    *          m_TAI_Ascii (std::string contains ASCII stream)
    */
   //static void toAsciiStr(const uint8 taiOctetStream[], string& asciiStr);

   /*
    * Method : toHexStr()
    * Purpose: This method takes in TAI octet stream and returns following class
    *          property and tells if the TAI is valid.
    *          m_TAI_MCC
    *          m_TAI_MNC
    *          m_TAI_TAC
    *          m_TAI_Hex (std::string contains Hex stream)
    */
  // static bool toHexStr(const uint8 taiOctetStream[], uint16_t& mcc, uint16_t& mnc, uint16_t& tac, string& hexStr);

   /*
    * Method : toAsciiStr()
    * Purpose: This method takes in TAI char stream and returns following class
    *          property and tells if the TAI is valid.
    *          m_TAI_MCC
    *          m_TAI_MNC
    *          m_TAI_TAC
    *          m_TAI_Hex (std::string contains Hex stream)
    */
   //static bool toAsciiStr(const char taiCharStream[], uint16_t& mcc, uint16_t& mnc, uint16_t& tac, string& hexStr);

   /*
    * Method : isTaiValid()
    * Purpose: This method should be called before using the TAI
    * to check if the class contains a valid TAI
    */
   bool isTaiValid() const;

public:
   /**
    * Default Constructor
    */
   Tai();

   /**
    * De-constructor
    */
   ~Tai(){

   }


   friend bool operator== (const Tai &a, const Tai &b);

   friend bool operator!= (const Tai &a, const Tai &b);

   friend ostream& operator<< (ostream& os,const Tai& a);

   //  [1/10/2013 zhoupeishi]
   Tai& operator= (const Tai& a)
   {
       m_TAI_MCC = a.m_TAI_MCC; // 12 bits value
       m_TAI_MNC = a.m_TAI_MNC; // 12 bits value
       m_TAI_TAC = a.m_TAI_TAC; // 16 bits value
       m_TaiValid = a.m_TaiValid;
       m_TAI_Ascii = a.m_TAI_Ascii;
       m_TAI_Hex = a.m_TAI_Hex;
       return *this;
   }

   /*
    * Method : setTai()
    * Purpose: This method is used to set the properties of the TAI.
    */
   //bool setTai(const uint8 taiOctetStream[], const uint16_t length);

   /*
    * Method : getMCC() and setTaiMCC()
    * Purpose: These methods are used to set and get the Mobile Country Code of the TAI
    */
   inline uint16_t getTaiMCC() const
   {
      return (m_TAI_MCC);
   }

   inline void setTaiMCC(const uint16_t mcc)
   {
      m_TAI_MCC = mcc;
      doEncode();
   }

   /*
    * Method : getTaiMNC() and setTaiMNC()
    * Purpose: These methods are used to get and set the Mobile Network Code of the TAI
    */
   inline uint16_t getTaiMNC() const
   {
      return (m_TAI_MNC);
   }

   inline void setTaiMNC(const uint16_t mnc)
   {
      m_TAI_MNC = mnc;
      doEncode();
   }

   /*
    * Method : getTaiTAC() and setTaiTAC()
    * Purpose: These methods are used to get and set the Tracking Area Code of the TAI
    */
   inline uint16_t getTaiTAC() const
   {
      return (m_TAI_TAC);
   }

   inline void setTaiTAC(const uint16_t tac)
   {
      m_TAI_TAC = tac;
      doEncode();
   }

   inline void setTaiMccMncTac(const uint16_t mcc, const uint16_t mnc, const uint16_t tac)
   {
      m_TAI_MCC = mcc;
      m_TAI_MNC = mnc;
      m_TAI_TAC = tac;
      doEncode();
   }

   /*
    * Method : getTaiHex()
    * Purpose: This method returns the Hex stream of the TAI
    */
   inline void getTaiHex(unsigned char* buf)
   {
      memcpy(buf, m_TAI_Hex.data(), TAI_LENGTH);
   }

   /*
    * Method : getTaiHex()
    * Purpose: This method returns the std::string which contains the Hex stream of the TAI
    */
   inline string getTaiHex()
   {
      return(m_TAI_Hex);
   }

   /*
    * Method : getTaiAscii()
    * Purpose: This method returns the std::string which contains the ASCII stream of the TAI
    */
   inline string getTaiAscii()
   {
      return(m_TAI_Ascii);
   }

   inline bool taiValid()
   {
      return(m_TaiValid);
   }

   void doEncode();
   void getFqdn(std::string& fqdn);
   //void getFqdnLabels(SYS::Bytes& dname);
   void getApnOIfromTai(std::string& apnoi);
   string toString() const;

private:
   //void buildLabels(std::string fqdn);

private:
   uint16_t m_TAI_MCC; // 12 bits value
   uint16_t m_TAI_MNC; // 12 bits value
   uint16_t m_TAI_TAC; // 16 bits value
   bool m_TaiValid;
   string m_TAI_Ascii;
   string m_TAI_Hex;
   //SYS::List<SYS::Bytes> m_labels;
};

//_SYS_NAMESPACE_END

#endif /* _SYS_TAI_H_ */
