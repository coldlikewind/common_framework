#include "CfgHelper.h"

using namespace std;

std::string GetBoolStr(bool tag)
{
	return (tag ? "true" : "false");
}


//S1AP协议
bool operator== (const Cfg_S1APPro& cfg1,const Cfg_S1APPro& cfg2)
{
	if (cfg1.S1HOCompTimer_s != cfg2.S1HOCompTimer_s) return false;
	if (cfg1.S1HOPreTimer_s != cfg2.S1HOPreTimer_s) return false;
	if (cfg1.X2HOCompTimer_s != cfg2.X2HOCompTimer_s) return false;
	if (cfg1.X2HOPreTimer_s != cfg2.X2HOPreTimer_s) return false;
	if (cfg1.S1APTimeout_ms != cfg2.S1APTimeout_ms) return false;
	if (cfg1.S1APFlowTimeout_ms != cfg2.S1APFlowTimeout_ms) return false;
	if (cfg1.TimetoWait_s != cfg2.TimetoWait_s) return false;
	if (cfg1.TAUReconnectTimes != cfg2.TAUReconnectTimes) return false;
	if (cfg1.SctpHAFailed_s != cfg2.SctpHAFailed_s) return false;
	if (cfg1.IdleHenbStart_s != cfg2.IdleHenbStart_s) return false;
	if (cfg1.UEAliveTimer_h != cfg2.UEAliveTimer_h) return false;
	if (cfg1.HenbAliveTimer_h != cfg2.HenbAliveTimer_h) return false;
	if (cfg1.S1APAuditTimeout_ms != cfg2.S1APAuditTimeout_ms) return false;

	return true;
}

bool operator!= (const Cfg_S1APPro& cfg1,const Cfg_S1APPro& cfg2)
{
	return !(cfg1 == cfg2);
}

std::ostream& operator<<(std::ostream &os,const Cfg_S1APPro& cfg)
{
	os << "\t" <<"S1HOCompTimer_s=" << cfg.S1HOCompTimer_s;
	os << "\t" <<"S1HOPreTimer_s=" << cfg.S1HOPreTimer_s;
	os << "\t" <<"X2HOCompTimer_s=" << cfg.X2HOCompTimer_s;
	os << "\t" <<"X2HOPreTimer_s=" << cfg.X2HOPreTimer_s;
	os << "\t" <<"S1APTimeout_ms=" << cfg.S1APTimeout_ms;
	os << "\t" <<"S1APFlowTimeout_ms=" << cfg.S1APFlowTimeout_ms;
	os << "\t" <<"TimetoWait_s=" << std::to_string((int)cfg.TimetoWait_s);
	os << "\t" <<"TAUReconnectTimes=" << std::to_string((int)cfg.TAUReconnectTimes);
	os << "\t" <<"SctpHAFailed_s=" << cfg.SctpHAFailed_s;
	os << "\t" <<"IdleHenbStart_s=" << cfg.IdleHenbStart_s;
	os << "\t" <<"UEAliveTimer_h=" << cfg.UEAliveTimer_h;
	os << "\t" <<"HenbAliveTimer_h=" << cfg.HenbAliveTimer_h;
	os << "\t" <<"S1APAuditTimeout_ms=" << cfg.S1APAuditTimeout_ms;
	return os;
}

//S1接口配置
bool operator== (const Cfg_HeNBInterData& cfg1,const Cfg_HeNBInterData& cfg2)
{
	if (cfg1.SctpPort != cfg2.SctpPort) return false;
	if (cfg1.S1APProID != cfg2.S1APProID) return false;
	if (cfg1.SctpNum != cfg2.SctpNum) return false;

	return true;
}

bool operator!= (const Cfg_HeNBInterData& cfg1,const Cfg_HeNBInterData& cfg2)
{
	return !(cfg1 == cfg2);
}

std::ostream& operator<<(std::ostream &os,const Cfg_HeNBInterData& cfg)
{
	os << "\t" <<"SctpPort=" << cfg.SctpPort;
	os << "\t" <<"S1APProID=" << std::to_string((int)cfg.S1APProID);
	os << "\t" <<"SctpNum=" << std::to_string((int)cfg.SctpNum);
	return os;
}

bool operator== (const Cfg_MMEInterData& cfg1,const Cfg_MMEInterData& cfg2)
{
	if (cfg1.SctpPort != cfg2.SctpPort) return false;
	if (cfg1.S1APProID != cfg2.S1APProID) return false;
	if (cfg1.SctpNum != cfg2.SctpNum) return false;

	return true;
}

bool operator!= (const Cfg_MMEInterData& cfg1,const Cfg_MMEInterData& cfg2)
{
	return !(cfg1 == cfg2);
}

std::ostream& operator<<(std::ostream &os,const Cfg_MMEInterData& cfg)
{
	os << "\t" <<"SctpPort=" << cfg.SctpPort;
	os << "\t" <<"S1APProID=" << std::to_string((int)cfg.S1APProID);
	os << "\t" <<"SctpNum=" << std::to_string((int)cfg.SctpNum);
	return os;
}

bool operator== (const Cfg_Plmn& cfg1,const Cfg_Plmn& cfg2)
{
	if (cfg1.mcc1 != cfg2.mcc1) return false;
	if (cfg1.mcc2 != cfg2.mcc2) return false;
	if (cfg1.mcc3 != cfg2.mcc3) return false;
	if (cfg1.mnc1 != cfg2.mnc1) return false;
	if (cfg1.mnc2 != cfg2.mnc2) return false;
	if (cfg1.mnc3 != cfg2.mnc3) return false;

	return true;
}

bool operator!= (const Cfg_Plmn& cfg1,const Cfg_Plmn& cfg2)
{
	return !(cfg1 == cfg2);
}

std::ostream& operator<<(std::ostream &os,const Cfg_Plmn& cfg)
{
	os << "\t" <<"mcc1=" << cfg.mcc1;
	os << "\t" <<"mcc2=" << cfg.mcc2;
	os << "\t" <<"mcc3=" << cfg.mcc3;
	os << "\t" <<"mnc1=" << cfg.mnc1;
	os << "\t" <<"mnc2=" << cfg.mnc2;
	os << "\t" <<"mnc3=" << cfg.mnc3;
	return os;
}

bool operator== (const Cfg_TAParamList& cfg1,const Cfg_TAParamList& cfg2)
{
	if (cfg1.TAC != cfg2.TAC) return false;
	if (!CfgCompareVector(cfg1.BPlmnList, cfg2.BPlmnList)) return false;
	return true;
}

bool operator!= (const Cfg_TAParamList& cfg1,const Cfg_TAParamList& cfg2)
{
	return !(cfg1 == cfg2);
}

std::ostream& operator<<(std::ostream &os,const Cfg_TAParamList& cfg)
{
	os << "\t" <<"TAC=" << cfg.TAC;
	CfgPrintVector("BPlmnList", cfg.BPlmnList, os);
	return os;
}

bool operator== (const Cfg_GUMMEI& cfg1,const Cfg_GUMMEI& cfg2)
{
	if (!CfgCompareVector(cfg1.SPlmnList, cfg2.SPlmnList)) return false;
	if (!CfgCompareVector(cfg1.MMEGroupID, cfg2.MMEGroupID)) return false;
	if (!CfgCompareVector(cfg1.MMECode, cfg2.MMECode)) return false;
	return true;
}

bool operator!= (const Cfg_GUMMEI& cfg1,const Cfg_GUMMEI& cfg2)
{
	return !(cfg1 == cfg2);
}

std::ostream& operator<<(std::ostream &os,const Cfg_GUMMEI& cfg)
{
	CfgPrintVector("SPlmnList", cfg.SPlmnList, os);
	CfgPrintVector("MMEGroupID", cfg.MMEGroupID, os);
	CfgPrintVector("MMECode", cfg.MMECode, os);
	return os;
}

bool operator== (const Cfg_HeNBInterParam& cfg1,const Cfg_HeNBInterParam& cfg2)
{
	if (cfg1.MMECapacity != cfg2.MMECapacity) return false;
	if (strcmp(cfg1.MMEName, cfg2.MMEName) != 0) return false;
	if (!CfgCompareVector(cfg1.GUMMEIList, cfg2.GUMMEIList)) return false;
	return true;
}

bool operator!= (const Cfg_HeNBInterParam& cfg1,const Cfg_HeNBInterParam& cfg2)
{
	return !(cfg1 == cfg2);
}

std::ostream& operator<<(std::ostream &os,const Cfg_HeNBInterParam& cfg)
{
	os << "\t" <<"MMECapacity=" << cfg.MMECapacity;
	os << "\t" <<"MMEName=" << cfg.MMEName;
	CfgPrintVector("GUMMEIList", cfg.GUMMEIList, os);
	return os;
}

bool operator== (const Cfg_MMEInterParam& cfg1,const Cfg_MMEInterParam& cfg2)
{
	if (cfg1.Plmn != cfg2.Plmn) return false;
	if (cfg1.HeNBID != cfg2.HeNBID) return false;
	if (cfg1.DRX != cfg2.DRX) return false;
	if (strcmp(cfg1.HeNBName, cfg2.HeNBName) != 0) return false;
	if (!CfgCompareVector(cfg1.TAParamList, cfg2.TAParamList)) return false;
	if (!CfgCompareVector(cfg1.CSGList, cfg2.CSGList)) return false;
	if (cfg1.ENodeBType != cfg2.ENodeBType) return false;
	return true;
}

bool operator!= (const Cfg_MMEInterParam& cfg1,const Cfg_MMEInterParam& cfg2)
{
	return !(cfg1 == cfg2);
}

std::ostream& operator<<(std::ostream &os,const Cfg_MMEInterParam& cfg)
{
	os << "\t" << cfg.Plmn;
	os << "\t" <<"HeNBID=" << cfg.HeNBID;
	os << "\t" <<"DRX=" << std::to_string((int)cfg.DRX);
	os << "\t" <<"HeNBName=" << cfg.HeNBName;
	os << "\t" <<"ENodeBType=" << std::to_string((int)cfg.ENodeBType);
	CfgPrintVector("TAParamList", cfg.TAParamList, os);
	CfgPrintVector("CSGList", cfg.CSGList, os);
	return os;
}

bool operator== (const Cfg_LteSctp& cfg1,const Cfg_LteSctp& cfg2)
{
	if (cfg1.HeartBeatTime_s != cfg2.HeartBeatTime_s) return false;
	if (cfg1.WorkType != cfg2.WorkType) return false;
	if (memcmp(&cfg1.LocalIP1, &cfg2.LocalIP1, sizeof(agc_std_socket_t)) != 0) return false;
	if (memcmp(&cfg1.LocalIP2, &cfg2.LocalIP2, sizeof(agc_std_socket_t)) != 0) return false;
	if (cfg1.LocalPort != cfg2.LocalPort) return false;;
	if (memcmp(&cfg1.PeerIP1, &cfg2.PeerIP1, sizeof(agc_std_socket_t)) != 0) return false;
	if (memcmp(&cfg1.PeerIP2 , &cfg2.PeerIP2, sizeof(agc_std_socket_t)) != 0) return false;
	if (cfg1.PeerPort != cfg2.PeerPort) return false;
	if (cfg1.Priority != cfg2.Priority) return false;
	if (cfg1.SctpNum != cfg2.SctpNum) return false;

	return true;
}

bool operator!= (const Cfg_LteSctp& cfg1,const Cfg_LteSctp& cfg2)
{
	return !(cfg1 == cfg2);
}

std::ostream& operator<<(std::ostream &os,const Cfg_LteSctp& cfg)
{
	os << "\t" <<"HeartBeatTime_s=" << cfg.HeartBeatTime_s;
	os << "\t" <<"WorkType=" << std::to_string((int)cfg.WorkType);
	//os << "\t" <<"LocalIP1=" << cfg.LocalIP1;
	//os << "\t" <<"LocalIP2=" << cfg.LocalIP2;
	os << "\t" <<"LocalPort=" << cfg.LocalPort;
	//os << "\t" <<"PeerIP1=" << cfg.PeerIP1;
	//os << "\t" <<"PeerIP2=" << cfg.PeerIP2;
	os << "\t" <<"PeerPort=" << cfg.PeerPort;
	os << "\t" <<"Priority=" << std::to_string((int)cfg.Priority);
	os << "\t" <<"SctpNum=" << cfg.SctpNum;
	return os;
}

bool operator== (const Cfg_MME_Pool& cfg1,const Cfg_MME_Pool& cfg2)
{
	if (cfg1.MMEPoolID != cfg2.MMEPoolID) return false;
	if (cfg1.VGWID != cfg2.VGWID) return false;
	if (cfg1.UseNNSF != cfg2.UseNNSF) return false;
	if (strcmp(cfg1.MMEPoolName, cfg2.MMEPoolName) != 0) return false;

	return true;
}

bool operator!= (const Cfg_MME_Pool& cfg1,const Cfg_MME_Pool& cfg2)
{
	return !(cfg1 == cfg2);
}

std::ostream& operator<<(std::ostream &os,const Cfg_MME_Pool& cfg)
{
	os << "\t" <<"MMEPoolID=" << cfg.MMEPoolID;
	os << "\t" <<"VGWID=" << cfg.VGWID;
	os << "\t" <<"UseNNSF=" << GetBoolStr(cfg.UseNNSF);
	os << "\t" <<"MMEPoolName=" << cfg.MMEPoolName;

	return os;
}

bool operator== (const Cfg_MME_Param& cfg1,const Cfg_MME_Param& cfg2)
{
	if (cfg1.MMEID != cfg2.MMEID) return false;
	if (cfg1.MMEPoolID != cfg2.MMEPoolID) return false;
	if (cfg1.MMEIP1 != cfg2.MMEIP1) return false;
	if (cfg1.MMEIP2 != cfg2.MMEIP2) return false;
	if (cfg1.MMEPort != cfg2.MMEPort) return false;
	if (cfg1.Status != cfg2.Status) return false;
	if (cfg1.UENum != cfg2.UENum) return false;
	if (cfg1.ERabNum != cfg2.ERabNum) return false;
	if (cfg1.CapFactor != cfg2.CapFactor) return false;

	return true;
}

bool operator!= (const Cfg_MME_Param& cfg1,const Cfg_MME_Param& cfg2)
{
	return !(cfg1 == cfg2);
}

std::ostream& operator<<(std::ostream &os,const Cfg_MME_Param& cfg)
{
	os << "\t" <<"MMEID=" << cfg.MMEID;
	os << "\t" <<"MMEPoolID=" << cfg.MMEPoolID;
	//os << "\t" <<"MMEIP1=" << cfg.MMEIP1;
	//os << "\t" <<"MMEIP2=" << cfg.MMEIP2;
	os << "\t" <<"MMEPort=" << cfg.MMEPort;
	os << "\t" <<"Status=" << std::to_string((int)cfg.Status);
	os << "\t" <<"UENum=" << cfg.UENum;
	os << "\t" <<"ERabNum=" << cfg.ERabNum;
	os << "\t" <<"CapFactor=" << std::to_string((int)cfg.CapFactor);

	return os;
}

bool operator== (const Cfg_gNBInterData& cfg1,const Cfg_gNBInterData& cfg2)
{
	if (cfg1.SctpPort != cfg2.SctpPort) return false;
	if (cfg1.NGAPProID != cfg2.NGAPProID) return false;
	if (cfg1.SctpNum != cfg2.SctpNum) return false;

	return true;
}

bool operator!= (const Cfg_gNBInterData& cfg1,const Cfg_gNBInterData& cfg2)
{
	return !(cfg1 == cfg2);
}

std::ostream& operator<<(std::ostream &os,const Cfg_gNBInterData& cfg)
{
	os << "\t" <<"SctpPort=" << cfg.SctpPort;
	os << "\t" <<"NGAPProID=" << std::to_string((int)cfg.NGAPProID);
	os << "\t" <<"SctpNum=" << std::to_string((int)cfg.SctpNum);
	return os;
}

bool operator== (const Cfg_AMFInterData& cfg1,const Cfg_AMFInterData& cfg2)
{
	if (cfg1.SctpPort != cfg2.SctpPort) return false;
	if (cfg1.NGAPProID != cfg2.NGAPProID) return false;
	if (cfg1.SctpNum != cfg2.SctpNum) return false;

	return true;
}

bool operator!= (const Cfg_AMFInterData& cfg1,const Cfg_AMFInterData& cfg2)
{
	return !(cfg1 == cfg2);
}

std::ostream& operator<<(std::ostream &os,const Cfg_AMFInterData& cfg)
{
	os << "\t" <<"SctpPort=" << cfg.SctpPort;
	os << "\t" <<"NGAPProID=" << std::to_string((int)cfg.NGAPProID);
	os << "\t" <<"SctpNum=" << std::to_string((int)cfg.SctpNum);
	return os;
}

bool operator== (const Cfg_GUAMI& cfg1,const Cfg_GUAMI& cfg2)
{
	if (cfg1.plmn != cfg2.plmn) return false;
	if (cfg1.AMFRegionID != cfg2.AMFRegionID) return false;
	if (cfg1.AMFSetId != cfg2.AMFSetId) return false;
	if (cfg1.AMFPointer != cfg2.AMFPointer) return false;
	return true;
}

bool operator!= (const Cfg_GUAMI& cfg1,const Cfg_GUAMI& cfg2)
{
	return !(cfg1 == cfg2);
}

std::ostream& operator<<(std::ostream &os,const Cfg_GUAMI& cfg)
{
	os << "\t" <<"mcc1=" << cfg.plmn.mcc1;
	os << "\t" <<"mcc2=" << cfg.plmn.mcc2;
	os << "\t" <<"mcc3=" << cfg.plmn.mcc3;
	os << "\t" <<"mnc1=" << cfg.plmn.mnc1;
	os << "\t" <<"mnc2=" << cfg.plmn.mnc2;
	os << "\t" <<"mnc3=" << cfg.plmn.mnc3;
	os << "\t" <<"AMFRegionID=" << cfg.AMFSetId;
	os << "\t" <<"AMFSetId=" << cfg.AMFPointer;
	os << "\t" <<"AMFPointer=" << cfg.AMFPointer;
	return os;
}

bool operator== (const Cfg_gNBInterParam& cfg1,const Cfg_gNBInterParam& cfg2)
{
	if (cfg1.AMFCapacity != cfg2.AMFCapacity) return false;
	if (strcmp(cfg1.AMFName, cfg2.AMFName) != 0) return false;
	if (!CfgCompareVector(cfg1.GUAMIList, cfg2.GUAMIList)) return false;
	if (!CfgCompareVector(cfg1.SPlmnList, cfg2.SPlmnList)) return false;
	return true;
}

bool operator!= (const Cfg_gNBInterParam& cfg1,const Cfg_gNBInterParam& cfg2)
{
	return !(cfg1 == cfg2);
}

std::ostream& operator<<(std::ostream &os,const Cfg_gNBInterParam& cfg)
{
	os << "\t" <<"AMFCapacity=" << cfg.AMFCapacity;
	os << "\t" <<"AMFName=" << cfg.AMFName;
	CfgPrintVector("GUAMIList", cfg.GUAMIList, os);
	CfgPrintVector("SPlmnList", cfg.SPlmnList, os);
	return os;
}

bool operator== (const Cfg_AMFInterParam& cfg1,const Cfg_AMFInterParam& cfg2)
{
	if (cfg1.Plmn != cfg2.Plmn) return false;
	if (cfg1.gID != cfg2.gID) return false;
	if (cfg1.DRX != cfg2.DRX) return false;
	if (strcmp(cfg1.gNBName, cfg2.gNBName) != 0) return false;
	if (!CfgCompareVector(cfg1.TAParamList, cfg2.TAParamList)) return false;
	if (cfg1.gNBType != cfg2.gNBType) return false;
	if (cfg1.ngeNBType != cfg2.ngeNBType) return false;
	return true;
}

bool operator== (const Cfg_BPTAParamList& cfg1,const Cfg_BPTAParamList& cfg2)
{
	if (cfg1.TAC != cfg2.TAC) return false;
	if (!CfgCompareVector(cfg1.BPlmnList, cfg2.BPlmnList)) return false;
	return true;
}

bool operator!= (const Cfg_BPTAParamList& cfg1,const Cfg_BPTAParamList& cfg2)
{
	return !(cfg1 == cfg2);
}

std::ostream& operator<<(std::ostream &os,const Cfg_BPTAParamList& cfg)
{
	os << "\t" <<"TAC=" << cfg.TAC;
	CfgPrintVector("BPlmnList", cfg.BPlmnList, os);
	return os;
}

bool operator!= (const Cfg_AMFInterParam& cfg1,const Cfg_AMFInterParam& cfg2)
{
	return !(cfg1 == cfg2);
}

std::ostream& operator<<(std::ostream &os,const Cfg_AMFInterParam& cfg)
{
	os << "\t" << cfg.Plmn;
	os << "\t" <<"gNBID=" << cfg.gID;
	os << "\t" <<"DRX=" << std::to_string((int)cfg.DRX);
	os << "\t" <<"gNBName=" << cfg.gNBName;
	os << "\t" <<"gNBType=" << std::to_string((int)cfg.gNBType);
	os << "\t" <<"ngeNBType=" << std::to_string((int)cfg.ngeNBType);
	CfgPrintVector("TAParamList", cfg.TAParamList, os);
	return os;
}

bool operator== (const Cfg_BPlmn& cfg1,const Cfg_BPlmn& cfg2)
{
	if (cfg1.plmn != cfg2.plmn) return false;
	if (!CfgCompareVector(cfg1.nnsai, cfg2.nnsai)) return false;

	return true;
}

bool operator!= (const Cfg_BPlmn& cfg1,const Cfg_BPlmn& cfg2)
{
	return !(cfg1 == cfg2);
}

std::ostream& operator<<(std::ostream &os,const Cfg_BPlmn& cfg)
{
	os << "\t" << cfg.plmn;
	CfgPrintVector("nnsai", cfg.nnsai, os);
	return os;
}

bool operator== (const Cfg_NSSAI& cfg1,const Cfg_NSSAI& cfg2)
{
	if (cfg1.SST != cfg2.SST) return false;
	if (cfg1.SD != cfg2.SD) return false;

	return true;
}

bool operator!= (const Cfg_NSSAI& cfg1,const Cfg_NSSAI& cfg2)
{
	return !(cfg1 == cfg2);
}

std::ostream& operator<<(std::ostream &os,const Cfg_NSSAI& cfg)
{
	os << "\t" <<"SST=" << cfg.SST;
	os << "\t" <<"SD=" << cfg.SD;
	return os;
}

