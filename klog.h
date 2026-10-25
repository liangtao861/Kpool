#ifndef KLOG_H_
#define KLOG_H_

#include <iostream>
#include <string>
#include <cctype>
#include <fstream>

#if defined(P_KLOG_LOG_OUT)
    #define P_KLOG_PUT(self,level,...)      self.logOut(__VA_ARGS__,level,__LINE__,__func__);
    #define P_KLOG_SET(self,sign)           self.setf(sign);
    #define P_KLOG_DIA(self,level)          self.logIn
    #define P_KLOG_DUMP(self)               self.dumpLog();
    #define P_KLOG_LINE(self,_line)         self.line(_line);
    #define P_KLOG_SELF(name,path)          static klog name = klog(path);
#else
    #define P_KLOG_PUT(self,level,...)      ;
    #define P_KLOG_DUMP(self,sign)          ;
    #define P_KLOG_SET(self,sign)           ;
    #define P_KLOG_DIA(self,level)          ;
    #define P_KLOG_LINE(self,sign)          ;
    #define P_KLOG_SELF(name,path)          ;
#endif

class klog{
private:
	const char*   levelTuple[6] = {"info","error","fatal","debug","success","warning"};
	std::string   logPath;
	std::fstream  of;
	std::uint64_t levels;
	std::uint64_t sets;
	std::string   format(std::string& text,int& level,int& line,const char*& function);
	void          rawWrite(const char* in);
    std::string   rawRead(int _line);
	
public:
	enum logLevel{
		INF = 1,
		ERR = 2,
		FAT = 3,
		DBG = 4,
		SUC = 5,
		WAR = 6
	};
	enum setSign{
		TIME_HAVE = 1,
		LEVEL_HAVE = 4,
		LINE_HAVE = 8
	};	
	klog(std::string _lP)
		: logPath(_lP) {
			//if constexpr(is_same<T,std::fstream>.value)
			of = std::fstream(_lP,std::ios::in | std::ios::out | std::ios::app);
			setLevelIn(INF | FAT | ERR | DBG | SUC | WAR);
			setf(TIME_HAVE | LEVEL_HAVE | LINE_HAVE);
		}
	void        logOut (std::string log,int level,int line,const char* function);
	std::string dumpLog();
    std::string line(int _line);
	void        setf   (std::uint64_t sign);
	void        setLevelIn(std::uint64_t level);
};
		
#endif
