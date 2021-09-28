#pragma once
#include <iostream>
#include <vector>
#include <iostream>	
#include "agc.h"

////////////////////////////////////// ives �ṹ��������������ú��� Begin //////////////////////////////////////
template<typename T>
bool CfgCompareVector(const std::vector<T>& v1, const std::vector<T>& v2)/*��֧��˳���ҵ����*/
{
	if (v1.size() != v2.size()) return false;
	for (unsigned int i = 0; i < v1.size(); i++)
	{
		if (v1[i] != v2[i]) return false;
	}
	return true;
}

//���T����vector�У���tcount=1,���T��vector��vector�У���tcount=2
template<typename T>
void CfgPrintVector(const std::string name, const std::vector<T>& v1, std::ostream& os, unsigned int tcount = 0)
{
	std::string str = "";
	for (uint32_t i = 0; i < tcount; i++)
	{
		str += "\t";
	}
	os << std::endl << str << name << ": size=" << v1.size();
	str += "\t";
	for (uint32_t i = 0; i < v1.size(); i++)
	{
		os << std::endl << str << "[" << i << "]:" << std::endl<< str << v1[i];
	}
}

template<typename T>
struct Cfg_BaseList
{
public:
	std::vector<T> Items;

	friend bool operator ==(const Cfg_BaseList& cfg1, const Cfg_BaseList& cfg2)
	{
		if (!CfgCompareVector(cfg1.Items, cfg2.Items)) return false;
		return true;
	}

	friend bool operator !=(const Cfg_BaseList& cfg1, const Cfg_BaseList& cfg2)
	{
		return !(cfg1 == cfg2);
	}

	friend std::ostream& operator<<(std::ostream &os,const Cfg_BaseList& cfg)
	{
		CfgPrintVector("Items", cfg.Items, os);
		return os;
	}
};



enum Cfg_TimeToWait
{
	CFG_SECOND_1 = 1,            //1s
	CFG_SECOND_2 = 2,            //2s
	CFG_SECOND_5 = 5,            //5s
	CFG_SECOND_10 = 10,          //10s
	CFG_SECOND_20 = 20,          //20s
	CFG_SECOND_60 = 60           //60s
};

typedef struct Cfg_S1APPro
{
    uint16_t				S1HOPreTimer_s;						//S1 RelocAllocTimer ��λ ��
    uint16_t				S1HOCompTimer_s;					//S1 RelocCompleteTimer ��λ ��
    uint16_t				X2HOPreTimer_s;						//X2 RelocAllocTimer ��λ ��
    uint16_t				X2HOCompTimer_s;					//X2 RelocCompleteTimer ��λ ��
	uint16_t				S1APTimeout_ms;					    //S1AP���ʱʱ�� ��λ ����
	uint16_t              S1APFlowTimeout_ms;                 //S1AP���̳�ʱʱ�� ��λ ����
	Cfg_TimeToWait      TimetoWait_s;                       //S1AP����ʧ�ܵȴ�ʱ�� ��λ ��
	uint8_t               TAUReconnectTimes;                  //TAU�ܾ��������� ��λ ��
	uint16_t              SctpHAFailed_s;                     //T(sctp_ha_failed)(�����л���վ�����ȴ���ʱ��)
	uint16_t              IdleHenbStart_s;                    //T(idle_hnb_start)(�����л���ҵ���վ������ʱ��)
	uint16_t              UEAliveTimer_h;                     //T(ue_alive)(UE��ʱ��)	uint16_t(1~168)��λСʱ
	uint16_t              HenbAliveTimer_h;                   //T(hnb_alive)(��վ��ʱ��)	uint16_t(24~168)��λСʱ
	uint32_t			    S1APAuditTimeout_ms;				//T(s1ap_audit)ҵ����Ƴ�ʱ��ʱ��	uint16_t(1~3600s) 
}Cfg_S1APPro;

/********************************************************************************
S1�ӿ�����
********************************************************************************/
typedef struct Cfg_HeNBInterData
{
    uint16_t		SctpPort;						//HeNB to S1 SCTP�˿ں�
    uint8_t			S1APProID;						//S1APЭ��ID
    uint8_t			SctpNum;						//Sctp��������
}Cfg_HeNBInterData;

typedef struct Cfg_MMEInterData
{
    uint16_t		SctpPort;						//S1 to MME SCTP�˿ں�
    uint8_t			S1APProID;						//S1APЭ��ID
    uint8_t			SctpNum;						//Sctp��������
}Cfg_MMEInterData;

typedef struct Cfg_gNBInterData
{
    uint16_t		SctpPort;						//HeNB to S1 SCTP�˿ں�
    uint8_t			NGAPProID;						//S1APЭ��ID
    uint8_t			SctpNum;						//Sctp��������
}Cfg_gNBInterData;

typedef struct Cfg_AMFInterData
{
    uint16_t		SctpPort;						//S1 to MME SCTP�˿ں�
    uint8_t			NGAPProID;						//S1APЭ��ID
    uint8_t			SctpNum;						//Sctp��������
}Cfg_AMFInterData;

typedef struct Cfg_Plmn
{
	//uint16_t          MCC;							//MCC
	//uint16_t          MNC;							//MNC
#if BYTE_ORDER == BIG_ENDIAN
	uint8_t mcc2:4;
    uint8_t mcc1:4;
	uint8_t mnc1:4;
    uint8_t mcc3:4;
	uint8_t mnc3:4;
    uint8_t mnc2:4;
#else
    uint8_t mcc1:4;
	uint8_t mcc2:4;
    uint8_t mcc3:4;
	uint8_t mnc1:4;
    uint8_t mnc2:4;
	uint8_t mnc3:4;
#endif
}Cfg_Plmn;

//������������,���ݡ��㲥BPLMN��ʶ�����㲥BPLMN�����
typedef struct Cfg_TAParamList
{
	uint16_t						TAC;                //��������ʶ
	std::vector<Cfg_Plmn>		BPlmnList;			//�㲥PLMNID
}Cfg_TAParamList;

typedef struct Cfg_NSSAI
{
	uint8_t          SST;							//MCC
	uint32_t          SD;							//MNC
}Cfg_NSSAI;

typedef struct Cfg_BPlmn
{
	//uint16_t          MCC;							//MCC
	//uint16_t          MNC;							//MNC
	Cfg_Plmn          plmn;
	std::vector<Cfg_NSSAI>		nnsai;	
}Cfg_BPlmn;

typedef struct Cfg_BPTAParamList
{
	uint32_t						TAC;                //��������ʶ
	std::vector<Cfg_BPlmn>		BPlmnList;			//�㲥PLMNID
}Cfg_BPTAParamList;

enum Cfg_PagingDRX
{
	DRX_32 = 0,
	DRX_64 = 1,
	DRX_128 = 2,
	DRX_256 = 3
};

enum Cfg_ENodeBType
{
	CFG_MACRO_ENODEB = 0,
	CFG_HOME_ENODEB = 1
};

enum Cfg_GNBType
{
	CFG_GNB = 0,
	CFG_NG_ENB = 1,
	CFG_N3IWF = 2
};

enum Cfg_NG_ENBType
{
	CFG_NGE_MACRO = 0,
	CFG_NGE_SHORT_MACRO = 1,
	CFG_NGE_LONG_MACRO = 2,
	CFG_NGE_NULL = 255
};


typedef struct Cfg_GUMMEI
{
	std::vector<Cfg_Plmn>			SPlmnList;				//����PLMNID(���32��)
	std::vector<uint16_t>				MMEGroupID;				//MME���ʶ(���65536��)
	std::vector<uint8_t>				MMECode;				//MME��ʶ(���32��)
}Cfg_GUMMEI;

typedef struct Cfg_GUAMI
{
	//uint16_t          	MCC;							//MCC
	//uint16_t          	MNC;							//MNC
	Cfg_Plmn			plmn;
	uint8_t				AMFRegionID;				//MME���ʶ(���65536��)
	uint16_t			AMFSetId;				//MME��ʶ(���32��)
	uint8_t				AMFPointer;				//MME��ʶ(���32��)
}Cfg_GUAMI;

//HeNB-GW�ӿڻ�������
typedef struct Cfg_HeNBInterParam
{
	char							MMEName[256];			//MME����
	uint8_t							MMECapacity;			//MME����
	std::vector<Cfg_GUMMEI>			GUMMEIList;				//����GUMMEI������(���8��)
}Cfg_HeNBInterParam;

//GW-MME�ӿڻ�������
typedef struct Cfg_MMEInterParam
{
    Cfg_Plmn						Plmn;					//
    uint32_t						HeNBID;                 //Global HeNBID
    char							HeNBName[256];			//HeNB����
    std::vector<Cfg_TAParamList>	TAParamList;			//������������
    std::vector<uint32_t>			CSGList;				//CSG�б�
    Cfg_PagingDRX					DRX;					//Ѱ��DRX
	Cfg_ENodeBType                  ENodeBType;             //��վ����   
}Cfg_MMEInterParam;

typedef struct Cfg_gNBInterParam
{
	char							AMFName[256];			//MME����
	uint8_t							AMFCapacity;			//MME����
	std::vector<Cfg_GUAMI>			GUAMIList;				//����GUMMEI������(���8��)
	std::vector<Cfg_BPlmn>			SPlmnList;	
}Cfg_gNBInterParam;

//GW-MME�ӿڻ�������
typedef struct Cfg_AMFInterParam
{
    Cfg_Plmn						Plmn;					//
    uint32_t						gID;                 //Global HeNBID
    char							gNBName[256];			//HeNB����
    std::vector<Cfg_BPTAParamList>	TAParamList;			//������������
    Cfg_PagingDRX					DRX;					//Ѱ��DRX
	Cfg_GNBType                     gNBType;             //��վ����   
	Cfg_NG_ENBType					ngeNBType;
}Cfg_AMFInterParam;

enum Cfg_SctpWorkType
{
	SCTP_WT_SERVER = 0,
	SCTP_WT_CLIENT = 1
};

enum Cfg_IPType
{
	IPv4 = 0,
	IPv6 = 1
};



typedef struct Cfg_LteSctp
{
	uint32_t						idSctpIndex;			
	uint8_t							idSigGW;                
	uint8_t							Priority;				
	Cfg_SctpWorkType				WorkType;				
	Cfg_IPType						iptype;
	agc_std_sockaddr_t				LocalIP1;				
	socklen_t						LocalIP1Len;
	agc_std_sockaddr_t				LocalIP2;				
	socklen_t						LocalIP2Len;
	uint16_t						LocalPort;				
	agc_std_sockaddr_t				PeerIP1;			
	socklen_t						PeerIP1Len;
	agc_std_sockaddr_t				PeerIP2;			
	socklen_t						PeerIP2Len;
	uint16_t					    PeerPort;			
	uint16_t						HeartBeatTime_s;	
	uint16_t						SctpNum;	
	uint8_t							gnbOrAmf;           		
}Cfg_LteSctp;

typedef Cfg_BaseList<Cfg_LteSctp> Cfg_LteSctpList;			//sctp��

//MME Pool���ñ�
typedef struct Cfg_MME_Pool
{
	uint16_t MMEPoolID;        //MME Pool ID
	uint16_t VGWID;           //VGWID
	bool UseNNSF;           //�Ƿ�����NNSF����
	char MMEPoolName[512];  //MME Pool ����
}Cfg_MME_Pool;

typedef Cfg_BaseList<Cfg_MME_Pool> Cfg_MME_PoolList;			//MMEPool������

//MME״̬
enum Cfg_MMEStatus
{
	CFG_MME_NORMAL = 0,        //����
	CFG_MME_OFFLOAD = 1,       //ж��
	CFG_MME_NOUSE = 2          //����
};

//MME�������ñ�
typedef struct Cfg_MME_Param
{
	uint16_t MMEID;                 //MME ID
	uint16_t MMEPoolID;             //MME Pool ID
	agc_std_socket_t MMEIP1;           //MME IP1
	agc_std_socket_t MMEIP2;           //MME IP2
	uint16_t MMEPort;              //MME�˿�
	Cfg_MMEStatus Status;        //MME״̬
	uint32_t UENum;                //֧��UE��
	uint32_t ERabNum;             //������
	uint8_t CapFactor;             //��������
}Cfg_MME_Param;

typedef Cfg_BaseList<Cfg_MME_Param> Cfg_MME_ParamList;			//MMEParam������



//Start define struct for API-------------------------------------

typedef struct Cfg_vGWParam_API
{
	uint8_t                         idSigGW;       // vGW id
	char							AMFName[256];  //para between gNB and sigGW
	uint8_t							AMFCapacity;	//para between gNB and sigGW	
	uint32_t						gNBid;        //Global HeNBID, para between sigGW and AMF	
    char							gNBName[256];	
    Cfg_Plmn						Plmn;					
    Cfg_PagingDRX					DRX;				
	Cfg_GNBType                     gNBType;            
	Cfg_NG_ENBType					ngeNBType;
	
}stCfg_vGWParam_API;

typedef struct Cfg_NSSAI_API
{
	uint8_t		     idNssai;
	uint8_t          SST;		
	uint32_t         SD;		
}stCfg_NSSAI_API;


typedef struct Cfg_BPlmn_API
{
	uint8_t           idSigGW;
	uint8_t           idBplmn;
	//uint16_t          MCC;							//MCC
	//uint16_t          MNC;							//MNC
	Cfg_Plmn		  plmn;
	uint8_t		      idNssai;	
}stCfg_BPlmn_API;


//used for NG SETUP RESPONSE.  
typedef struct Cfg_GUAMI_API
{
	uint8_t				idSigGW;
	uint8_t				idGuami;
	//uint16_t          	MCC;
	//uint16_t          	MNC;
	Cfg_Plmn			plmn;
	uint8_t				AMFRegionID;
	uint16_t			AMFSetId;	
	uint8_t				AMFPointer;
}stCfg_GUAMI_API;


//used for NG SETUP REQUEST - Supported TA List.
typedef struct Cfg_TA_PLMN_API
{
	uint8_t				idSigGW;
	uint8_t             idTA;
	uint32_t			TAC;
	uint8_t             idBplmn;
}stCfg_TA_PLMN_API; // One TA maybe has N PLMN;

typedef struct Cfg_TA_DATA
{
	uint8_t				idSigGW;
	uint8_t             idTA;
	uint32_t			TAC;
}stCfg_TA_DATA; 

typedef struct Cfg_TA_PLMN_DATA
{
	uint8_t             idTA;
	uint8_t           	idBplmn;
	//uint16_t          	MCC;						
	//uint16_t          	MNC;	
	Cfg_Plmn			plmn;
}stCfg_TA_PLMN_DATA; 

typedef struct Cfg_PLMN_NSSAI_DATA
{
	uint8_t           	idBplmn;
	uint8_t		     	idNssai;
	uint8_t          	SST;						
	uint32_t         	SD;							
}stCfg_PLMN_NSSAI_DATA; 

typedef struct Cfg_MEDIA_DATA
{
	uint8_t				idSigGW;
	char				MediaGWName[256];
}stCfg_MEDIA_DATA; 
//END define struct for API-----------------------------------------

typedef struct SIG_TAC_RAN
{
	uint32_t	uTAC;
	uint32_t    uSctpIndex;
}stSIG_TAC_RAN;




