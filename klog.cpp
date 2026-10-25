
#include <ctime>
#include <cstdlib>
#include "klog.h"

void klog::logOut(std::string log,int level,int line,const char* function){
	std::string get = format(log,level,line,function);
    rawWrite(get.c_str());
}

void klog::setLevelIn(uint64_t level){
	levels = level;
}

void klog::setf(std::uint64_t sign){
	sets = sign;
}

#define __KLOG_TIME_BUF_SIZE 100
std::string klog::format(std::string& text,int& level,int& line,const char*& function){
	std::string get = "";
	if(levels & level){
	    char* bufTime = (char*)std::malloc(__KLOG_TIME_BUF_SIZE);
	    time_t rawTime = std::time(nullptr);
	    std::tm* rTime = std::localtime(&rawTime);
	    std::strftime(bufTime,__KLOG_TIME_BUF_SIZE,"%y-%m-%d %H:%M:%S",rTime);
	    //get = "[" + std::string(bufTime) + "]" + "[" + std::string(levelTuple[level]) + "] message:" + text + "\n";
		if(sets & TIME_HAVE)
			get = get + "[" + std::string(bufTime) + "]";
		if(sets & LEVEL_HAVE)
			get = get + "[" + std::string(levelTuple[level - 1]) + "]";
		if(sets & LINE_HAVE)
			get = get + "[" + std::to_string(line)
			      + "]" + "[" + std::string(function) + "]";
		get = get + "message:" + text + "\n";
	    std::free(bufTime);
	}
	return get;
}

void klog::rawWrite(const char* in){
	of << in << std::flush;
}

std::string klog::rawRead(int _line){
	of.seekg(0);
	std::string text = "";
	int cnt = 1;
	while(std::getline(of,text)){
		if(cnt == _line)
			return text;
		cnt++;
	}
	return text;
}

std::string klog::line(int _line){
	return rawRead(_line);
}

std::string klog::dumpLog(){
	of.seekg(0);
	std::string log = "";
	std::string ret = "";
	while(std::getline(of,ret))
		log += ret;
	of.clear();
	return log;
}
