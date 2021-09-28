
#pragma once

#include "mod_sig.h"

template<class TMessage,class AppCallback>
class IStackManager  
{
public:
	IStackManager() { }
	virtual ~IStackManager() { }

public:
	virtual bool Init() = 0;
	virtual void WaitForExit() = 0;
	virtual void Exit() = 0;

public:
	virtual bool SendUpLayerSctp(TMessage& info) = 0;
	virtual bool SendDownLayerSctp(TMessage& info) = 0;

	virtual void RegisterUpLayerRecv(uint16_t pduChoise,uint16_t procedureCode, AppCallback callback)= 0;
	virtual void RegisterDownLayerRecv(uint16_t pduChoise,uint16_t procedureCode, AppCallback callback)= 0;

	virtual void RegisterUpLayerSend(uint16_t pduChoise,uint16_t procedureCode, AppCallback callback)= 0;
	virtual void RegisterDownLayerSend(uint16_t pduChoise,uint16_t procedureCode, AppCallback callback)= 0;

	virtual void RegisterUpLayerSendError(AppCallback callback)= 0;
	virtual void RegisterDownLayerSendError(AppCallback callback)= 0;

	virtual void RegisterUpLayerRecvError(AppCallback callback)= 0;
	virtual void RegisterDownLayerRecvError(AppCallback callback)= 0;


	//virtual void RegisterMmeSctp(SctpAssocChanged func)= 0;
	//virtual void RegisterDownLayerSctp(SctpAssocChanged func, uint8_t SctpIndex = 0)= 0;
	
	virtual void Reset()
	{
	}
};
