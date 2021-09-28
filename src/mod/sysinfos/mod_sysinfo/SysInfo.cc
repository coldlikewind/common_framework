#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <fstream>
#include <algorithm>
#include <cstring>

#include <unistd.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <mntent.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <error.h>
#include <net/route.h>

#include "agc.h"
#include "SysInfo.h"

using namespace std;

static int g_TopCPU = 0;
static int g_TopMEM = 0;

static inline char* skip_token(const char* p)
{
    while(isspace(*p))
    {
        p++;
    }
    while(*p && !isspace(*p))
    {
        p++;	
    }
    return (char*)p;
}

void Sleep(int ms)
{
    struct timeval timeout;
    memset(&timeout,0,sizeof(struct timeval));
    timeout.tv_sec = ms/1000;
    timeout.tv_usec = ms%1000*1000;
    select(0,NULL,NULL,NULL,&timeout);
}

void get_occupy (struct Occupy *o,const string filename)
{
    FILE *fd;
    int n = 0;
    char buff[1024] = {0};                                                                                                   
    fd = fopen (filename.c_str(), "r");
    if (NULL == fd) {return;}

	//取第一行，取总的结果
    //fgets (buff, sizeof(buff), fd);
    for(n = 0; n < 1; n++)
    {
        fgets (buff, sizeof(buff),fd);
        sscanf (buff, "%s %llu %llu %llu %llu", (char*)(&o[n].Name), &o[n].User, &o[n].Nice, &o[n].System, &o[n].Idle);
        //		fprintf (stderr, "%s %u %u %u %u\n", o[n].name, o[n].user, o[n].nice, o[n].system, o[n].idle);
    }
    fclose(fd); 
}

int cal_occupy (struct Occupy *o, struct Occupy *n)
{
    double od, nd;
    //double id, sd;
    //double scale;
    int cpu_used =0;
    od = (double) (o->User + o->Nice + o->System + o->Idle);

    nd = (double) (n->User + n->Nice + n->System + n->Idle);
    /*	scale = 100.0 / (float)(nd-od);
    id = (double) (n->user - o->user);
    sd = (double) (n->system - o->system);
    cpu_used = ((sd+id)*100.0)/(nd-od);*/
    double total = double(nd - od);
    double idle = double(n->Idle - o->Idle);
	if (total > 0.000001 || total < -0.000001)
	{
		double tmp = (total - idle) / total * 100;
		cpu_used = (int)tmp; 
		return cpu_used;
	} 
	else
	{
		return 0;
	}
}

void get_memory(struct MemUse* mu)
{
    char *p;
    int fd;
    int len;
    int i;
    char buffer[2048] = {0};

    fd = open( MEM_INFO , O_RDONLY); 

    if (-1 == fd) {return;}

    len = read(fd, buffer, sizeof(buffer) - 1 ); 

    close(fd); 

    buffer[len] = '\0';

    p = buffer; 
    p = skip_token(p);
    mu->Total = strtoul(p, &p, 10 ); /* total memory */ 
    p = strchr(p, '\n' ); 
    p = skip_token(p); 
    mu->Free = strtoul(p, &p, 10 ); /* free memory */ 
    p = strchr(p, '\n' ); 
    p = skip_token(p);
    p = strchr(p, '\n' );
    p = skip_token(p);
    mu->Buffer = strtoul(p, &p, 10 ); /* buffer memory */ 
    p = strchr(p, '\n' ); 
    p = skip_token(p); 
    mu->Cached = strtoul(p, &p, 10 ); /* cached memory */ 
    for(i = 0 ; i < 8 ;i++) 
    { 
        p ++ ; 
        p = strchr(p, '\n' ); 
    } 
    p = skip_token(p); 
    mu->TotalSwap = strtoul(p, &p, 10 ); /* total swap */ 
    p = strchr(p, '\n' ); 
    p = skip_token(p); 
    mu->FreeSwap = strtoul(p, &p, 10 ); /* free swap */ 
}

bool GetCpuUsage(int &cpuUsage)
{
    struct Occupy ocpu[1];
    struct Occupy ncpu[1];

    get_occupy(ocpu,CPU_INFO);
    Sleep(1000);
    get_occupy(ncpu,CPU_INFO);

    cpuUsage = cal_occupy(&ocpu[0], &ncpu[0]);
    if (g_TopCPU < 100 && cpuUsage > g_TopCPU)
    {
        g_TopCPU = cpuUsage;
    }
    return true;
}

uint64 GetFilesysInfo()
{
    uint64 fileSysMem = 0;
	int fileSys_fd = 0;

	char fileSysBuf[256] = {0};
	char tmpFileSysBuf[256] = {0};

    fileSys_fd = open("/AC/fileSysMem",O_RDONLY);

	if (-1==fileSys_fd)
	{
		return 0;
	}

	read(fileSys_fd,fileSysBuf,sizeof(fileSysBuf)-1);

	sscanf(fileSysBuf,"%[0-9]",tmpFileSysBuf);

    fileSysMem = atoi(tmpFileSysBuf);

	close(fileSys_fd);

	return fileSysMem;
}

bool GetMemUsage(int& memUsage)
{
    struct MemUse mem = {0};
	float tmp = 0;
    get_memory(&mem);
    if(mem.Total == 0 || mem.Total < mem.Free) {return false;}
    
    tmp = ((float)mem.Total - (float)mem.Free - (float)mem.Buffer - (float)mem.Cached ) / (float)mem.Total * 100;

	//float tmp = ((float)mem.Total - (float)mem.Free - (float)mem.Buffer - (float)mem.Cached) / (float)mem.Total * 100;
   
    memUsage = (int)tmp;
    if (g_TopMEM < 100 && memUsage > g_TopMEM)
    {
        g_TopMEM = memUsage;
    }
    return true;
}

bool IsFileExist(char *str)
{
    if( (access( str, F_OK )) == 0 )
    {
        return true;
    }

    return false;

}

bool GetDiskUsage(const std::string& path, int &usage)
{
	struct statfs fs;

	if(0 != statfs(path.c_str(), &fs) || fs.f_blocks <= 0) 
	{
		return false;
	}
	float tmp = (float)(fs.f_blocks -fs.f_bfree)/(float)(fs.f_blocks - fs.f_bfree + fs.f_bavail) * 100 + 1;
	usage = (int)tmp; 

	if (usage > 100)
	{
		usage = 100;
	}

	return true;
}

void GetFileSystemPath(std::vector<std::string>& paths)
{
	paths.clear();

	FILE *mount = NULL;
	mount = setmntent("/etc/mtab", "r");
	if (mount != NULL)
	{
		struct mntent mptr;
		char buf[512] = {0};

		while (getmntent_r(mount, &mptr, buf, sizeof(buf)) != NULL)
		{
			paths.push_back(mptr.mnt_dir);
		}
		endmntent(mount);
	}
}

bool GetDiskUsage(int &maxUsaged)
{
	int tmpUsage;
	bool ret(false);
	std::vector<std::string> paths;
	GetFileSystemPath(paths);

	maxUsaged = 0;
	for (unsigned int i = 0u; i < paths.size(); i++)
	{
		if (GetDiskUsage(paths[i], tmpUsage))
		{
			ret = true;
			if (tmpUsage > maxUsaged)
			{
				maxUsaged = tmpUsage;
			}
		}
	}

	return ret;
}

bool GetUpTime(string &str)
{
    string tmpBuf;
    char buf[512] = {0};
    str = "";

    int fd = open(UPTIME_INFO,O_RDONLY);
    if(fd == -1) 
    {
        return false;
    }
    read(fd,buf,sizeof(buf) - 1);
    close(fd);

    tmpBuf = buf;
    string::size_type it = tmpBuf.find(' ');
    if(it == string::npos) return false;
    if(it + 1 == string::npos) return false;
    str = tmpBuf.substr(0,it);
    return true;
}

void GetSystemTime(string &str)
{
    //todo
	//str = DateTime::Now().ToLongString();
}

void RestartSystem()
{
    char cmd[32] = "reboot";
	::system(cmd);
    return;
}

int GetTopCpuUsage()
{
    return g_TopCPU;
}

int GetTopMemUsage()
{
    return g_TopMEM;
}

void GetTop()
{
	::system("top -bn1 > top.tmp 2> /dev/null");
}


void split(const string& s, vector<std::string>& sv, const char* delim = " ")
{
    sv.clear();
    char* buffer = new char[s.size() + 1];
    buffer[s.size()] = '\0';
    copy(s.begin(), s.end(), buffer);
    char* p = strtok(buffer, delim);
    do
    {
        sv.push_back(p);
    } while ((p = std::strtok(NULL, delim)));
    delete[] buffer;
    return;
}


void GetTopResult_Linux(map<string,TopUse> &result)
{
    try
    {
        FILE *fp;
        char buff[1024];                                                                                                      
        fp = fopen ("top.tmp", "r");
        if (NULL == fp) {return;}
        char *p = NULL;
        for(int i = 0;i < 7;i++)
        {
            fgets(buff,sizeof(buff),fp);
        }
        unsigned int count = 10;
		struct TopUse topresult;
        while((p = fgets(buff, sizeof(buff), fp)) && count--)
        {
            vector<string> vec;
            split(buff, vec);
            vector<string> tmp;
            for (unsigned int i = 0;i < vec.size();i++)
            {
                if (!vec[i].empty())  tmp.push_back(vec[i]);
            }
			if (tmp.size()  < 12u) continue;
            topresult.cpu = tmp[8];
            topresult.mem = tmp[9];
            topresult.name = tmp[11].erase(tmp[11].find_last_not_of("\r\n") + 1);
            result.insert(make_pair(tmp[0],topresult));
        }
        fclose(fp); 
    }
    catch (...)
    {
    	agc_log_printf( AGC_LOG, AGC_LOG_ERROR,"GetTopResult_Linux failed!\n");
    }
}

void GetTopResult(map<string,TopUse> &result)
{
    GetTop();
    if (IsFileExist((char*)"top.tmp"))
    {
        GetTopResult_Linux(result);
    }
}

bool SortCpuResult(const TopUse& info1, const TopUse& info2)
{
	return (info1.cpu > info2.cpu);
}

bool SortMemResult(const TopUse& info1, const TopUse& info2)
{
	return (info1.mem > info2.mem);
}

void GetResourceTop5(string& cpudesc,string& memdesc)
{
	map<string,TopUse> result;
	vector<TopUse> topresult;
	char buf[512] = {0};
	try
	{
		GetTopResult(result);
		map<string,TopUse>::iterator it;
		for (map<string,TopUse>::iterator it = result.begin();it != result.end(); it++)
		{
			topresult.push_back(it->second);
		}

		sort(topresult.begin(),topresult.end(),SortCpuResult);
		vector<TopUse>::iterator item;
		int i(0);
		cpudesc = "";
		for(item = topresult.begin();item != topresult.end();item++)
		{
			if (i < 5)
			{
				if (0 != strcmp(item->name.c_str(),"top"))
				{
					sprintf(buf,"%s %s%%;",item->name.c_str(),item->cpu.c_str());
					cpudesc += buf;
					i++;
				}
			}
		}
		i = 0;
		sort(topresult.begin(),topresult.end(),SortMemResult);
		memdesc = "";
		for(item = topresult.begin();item != topresult.end();item++)
		{
			if (i < 5)
			{
				if (0 != strcmp(item->name.c_str(),"top"))
				{
					sprintf(buf,"%s %s%%;",item->name.c_str(),item->mem.c_str());
					memdesc += buf;
					i++;
				}
			}
		}
	}
	catch(...)
	{
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GetResourceTop5 failed!\n");
	}
}

bool GetProcessStatus(std::vector<ProcStatusItem>& items)
{
	std::string paths = "/proc/";
	items.clear();

	try
	{
	}
	catch (std::exception& ex)
	{
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GetProcessStatus Error [%s]\n",ex.what());
		return false;
	}
	catch(...)
	{
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GetProcessStatus Traversal error!\n");
		return false;
	}
}

bool GetProcStatusItem(unsigned int procid, ProcStatusItem& item)
{
	char filePath[256]={0};
	sprintf(filePath,"/proc/%d/status",procid);

	//todo
	//if (!boost::filesystem::exists(filePath)) return false;
	
	std::fstream outfile;
	outfile.open(filePath, ios::in);
	if (!outfile.is_open())
	{
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "GetProcStatusItem Error opening file! %s\n.", filePath);
		return false;
	}
	if (!outfile) return false;
		
	int ret=0;
	int total=0;
	char buf[512];
	char bufkey[256];
	char bufvalue[256];
	
	while(!outfile.eof()&& outfile.good())
	{
		memset(buf,0,sizeof(buf));
		memset(bufkey,0,sizeof(bufkey));
		memset(bufvalue,0,sizeof(bufvalue));
		
		outfile.getline(buf, sizeof(buf));

		ret=sscanf(buf,"%[^:]: %s",bufkey,bufvalue);
		if (ret == 2)
		{
			if (strcmp(bufkey,"Name") == 0)
			{
				if(1 == sscanf(bufvalue,"%s",item.Name)) total++;
			}
			else if (strcmp(bufkey,"Pid") == 0)
			{
				if(1 == sscanf(bufvalue,"%u",&item.Pid) && item.Pid == procid) total++;
			}
			else if (strcmp(bufkey,"VmSize") == 0)
			{
				if(1 == sscanf(bufvalue,"%u",&item.VmSize)) total++;
			}
			else if (strcmp(bufkey,"VmRSS") == 0)
			{
				if(1 == sscanf(bufvalue,"%u",&item.VmRSS)) total++;
			}
		}
		if (total == 4)
			break;
	} 
	outfile.close();

	return (total == 4);
}





