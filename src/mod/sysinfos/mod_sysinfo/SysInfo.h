#pragma once
#include <string.h>
#include <iostream>
#include <map>
#include <vector>

using namespace std;

#define CPU_INFO "/proc/stat"
#define MEM_INFO "/proc/meminfo"
#define UPTIME_INFO "/proc/uptime"

typedef unsigned long long uint64;

struct Occupy
{
    char Name[20];
    uint64 User;
    uint64 Nice;
    uint64 System;
    uint64 Idle;
};

struct MemUse
{
    int Total;
    int Free;
    int Buffer;
    int Cached;
    int TotalSwap;
    int FreeSwap; 	
};

struct TopUse
{
    string cpu;
    string mem;
    string name;
};

struct ProcStatusItem
{
	char			Name[256];
	unsigned int	Pid;
	unsigned int	VmSize; //虚拟内存
	unsigned int	VmRSS; //物理内存
};

bool GetCpuUsage(int &cpuUsage);
bool GetMemUsage(int& memUsage);
bool IsFileExist(char *str);
bool GetDiskUsage(int &maxUsaged);//遍历所有文件系统路径，返回使用率最大的
bool GetUpTime(string &str);
void GetSystemTime(string &str);
void RestartSystem();
void ResetSystem();
int GetTopCpuUsage();
int GetTopMemUsage();
void GetResourceTop5(string& cpudesc,string& memdesc);  //System Resource of Top 5

//used to testGWUtility
void get_occupy (struct Occupy *o,const string filename);
int cal_occupy (struct Occupy *o, struct Occupy *n);

bool IsRightDate(const char *str_date);
void MessageSYSTIMEW(const char *str_date);
bool IsRightTime(const char *str_date);
int SetSysTime(tm *t);
void GetTopResult(map<string,TopUse> &result);

bool GetProcessStatus(vector<ProcStatusItem>& items);
bool GetProcStatusItem(unsigned int procid, ProcStatusItem& item);


