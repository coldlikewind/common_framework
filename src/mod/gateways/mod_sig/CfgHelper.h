#pragma once
#include <iostream>
#include <vector>
#include "mod_sig.h"
#include "CfgStruct.h"



std::string GetBoolStr(bool tag);

//LTEœ‡πÿ≈‰÷√
//S1AP–≠“È
bool operator== (const Cfg_S1APPro& cfg1,const Cfg_S1APPro& cfg2);
bool operator!= (const Cfg_S1APPro& cfg1,const Cfg_S1APPro& cfg2);
std::ostream& operator<<(std::ostream &os,const Cfg_S1APPro& cfg);

//S1Ω”ø⁄≈‰÷√
bool operator== (const Cfg_HeNBInterData& cfg1,const Cfg_HeNBInterData& cfg2);
bool operator!= (const Cfg_HeNBInterData& cfg1,const Cfg_HeNBInterData& cfg2);
std::ostream& operator<<(std::ostream &os,const Cfg_HeNBInterData& cfg);

bool operator== (const Cfg_MMEInterData& cfg1,const Cfg_MMEInterData& cfg2);
bool operator!= (const Cfg_MMEInterData& cfg1,const Cfg_MMEInterData& cfg2);
std::ostream& operator<<(std::ostream &os,const Cfg_MMEInterData& cfg);

bool operator== (const Cfg_Plmn& cfg1,const Cfg_Plmn& cfg2);
bool operator!= (const Cfg_Plmn& cfg1,const Cfg_Plmn& cfg2);
std::ostream& operator<<(std::ostream &os,const Cfg_Plmn& cfg);

bool operator== (const Cfg_TAParamList& cfg1,const Cfg_TAParamList& cfg2);
bool operator!= (const Cfg_TAParamList& cfg1,const Cfg_TAParamList& cfg2);
std::ostream& operator<<(std::ostream &os,const Cfg_TAParamList& cfg);

bool operator== (const Cfg_GUMMEI& cfg1,const Cfg_GUMMEI& cfg2);
bool operator!= (const Cfg_GUMMEI& cfg1,const Cfg_GUMMEI& cfg2);
std::ostream& operator<<(std::ostream &os,const Cfg_GUMMEI& cfg);

bool operator== (const Cfg_HeNBInterParam& cfg1,const Cfg_HeNBInterParam& cfg2);
bool operator!= (const Cfg_HeNBInterParam& cfg1,const Cfg_HeNBInterParam& cfg2);
std::ostream& operator<<(std::ostream &os,const Cfg_HeNBInterParam& cfg);

bool operator== (const Cfg_MMEInterParam& cfg1,const Cfg_MMEInterParam& cfg2);
bool operator!= (const Cfg_MMEInterParam& cfg1,const Cfg_MMEInterParam& cfg2);
std::ostream& operator<<(std::ostream &os,const Cfg_MMEInterParam& cfg);

bool operator== (const Cfg_LteSctp& cfg1,const Cfg_LteSctp& cfg2);
bool operator!= (const Cfg_LteSctp& cfg1,const Cfg_LteSctp& cfg2);
std::ostream& operator<<(std::ostream &os,const Cfg_LteSctp& cfg);


bool operator== (const Cfg_MME_Pool& cfg1,const Cfg_MME_Pool& cfg2);
bool operator!= (const Cfg_MME_Pool& cfg1,const Cfg_MME_Pool& cfg2);
std::ostream& operator<<(std::ostream &os,const Cfg_MME_Pool& cfg);

bool operator== (const Cfg_MME_Param& cfg1,const Cfg_MME_Param& cfg2);
bool operator!= (const Cfg_MME_Param& cfg1,const Cfg_MME_Param& cfg2);
std::ostream& operator<<(std::ostream &os,const Cfg_MME_Param& cfg);


//NGÔøΩ”øÔøΩÔøΩÔøΩÔøΩÔøΩ
bool operator== (const Cfg_gNBInterData& cfg1,const Cfg_gNBInterData& cfg2);
bool operator!= (const Cfg_gNBInterData& cfg1,const Cfg_gNBInterData& cfg2);
std::ostream& operator<<(std::ostream &os,const Cfg_gNBInterData& cfg);

bool operator== (const Cfg_AMFInterData& cfg1,const Cfg_AMFInterData& cfg2);
bool operator!= (const Cfg_AMFInterData& cfg1,const Cfg_AMFInterData& cfg2);
std::ostream& operator<<(std::ostream &os,const Cfg_AMFInterData& cfg);


bool operator== (const Cfg_GUAMI& cfg1,const Cfg_GUAMI& cfg2);
bool operator!= (const Cfg_GUAMI& cfg1,const Cfg_GUAMI& cfg2);
std::ostream& operator<<(std::ostream &os,const Cfg_GUAMI& cfg);

bool operator== (const Cfg_gNBInterParam& cfg1,const Cfg_gNBInterParam& cfg2);
bool operator!= (const Cfg_gNBInterParam& cfg1,const Cfg_gNBInterParam& cfg2);
std::ostream& operator<<(std::ostream &os,const Cfg_gNBInterParam& cfg);

bool operator== (const Cfg_BPTAParamList& cfg1,const Cfg_BPTAParamList& cfg2);
bool operator!= (const Cfg_BPTAParamList& cfg1,const Cfg_BPTAParamList& cfg2);
std::ostream& operator<<(std::ostream &os,const Cfg_BPTAParamList& cfg);

bool operator== (const Cfg_BPlmn& cfg1,const Cfg_BPlmn& cfg2);
bool operator!= (const Cfg_BPlmn& cfg1,const Cfg_BPlmn& cfg2);
std::ostream& operator<<(std::ostream &os,const Cfg_BPlmn& cfg);

bool operator== (const Cfg_NSSAI& cfg1,const Cfg_NSSAI& cfg2);
bool operator!= (const Cfg_NSSAI& cfg1,const Cfg_NSSAI& cfg2);
std::ostream& operator<<(std::ostream &os,const Cfg_NSSAI& cfg);

bool operator== (const Cfg_AMFInterParam& cfg1,const Cfg_AMFInterParam& cfg2);
bool operator!= (const Cfg_AMFInterParam& cfg1,const Cfg_AMFInterParam& cfg2);
std::ostream& operator<<(std::ostream &os,const Cfg_AMFInterParam& cfg);

