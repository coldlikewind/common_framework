#pragma once

#include <sstream>
#include <string>
#include <list>

#include <agc.h>
#include "mod_sig.h"


template<typename InstanceT,typename ClassT=InstanceT>
class SingletonT
{
public:
	typedef InstanceT*(*CreateInstanceFunc)();

	SingletonT() 
	{
	}

	~SingletonT()//ycat: 这里不用virtual,因为没有用到SingletonT的指针，而且有时候会对派生类进行memset操作 
	{
	}

	static InstanceT& Instance(CreateInstanceFunc newFunc = NULL)
	{
		if(s_pThis1 != NULL) return *s_pThis1;
		return GetInstance(&s_pThis1,newFunc);
	}

	//ClearInstance会有多线程问题，所以尽量不要调用，或者仅在测试时调用 
	static void ClearInstance()
	{
		DelInstance(&s_pThis1);
	}

protected:
	static void DelInstance(InstanceT** This)
	{
		agcsig_module_lock();
		if(*This == NULL) 
		{
			agcsig_module_unlock();
			return;
		}
		//SingletonManager::ClearRegister(*This);
		InstanceT* p = *This;
		*This = NULL;
		delete p;
		agcsig_module_lock();
	}

	static InstanceT& GetInstance(InstanceT** This,CreateInstanceFunc newFunc)
	{	
		agcsig_module_lock();
		if(*This == NULL)
		{
			if(newFunc == NULL)
			{
				*This = new InstanceT();
			}
			else
			{
				*This = newFunc();
			}

			//SingletonManager::Register<InstanceT>(*This,ClassT::SingletonName(),ClassT::SingletonPriority());
		}
		agcsig_module_unlock();
		return **This;
	}

private:
	static InstanceT* s_pThis1;
};

template<typename InstanceT,typename ClassT>
InstanceT* SingletonT<InstanceT,ClassT>::s_pThis1 = NULL;
