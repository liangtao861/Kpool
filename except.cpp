#include "except.h"

std::string memoryPool_error::to_hex(int n){
	std::ostringstream oss;
	
	oss << std::hex << n << std::uppercase << "";
	if(n < 16) return "0" + oss.str();
	return oss.str();
}

memoryPool_error::memoryPool_error(std::vector<std::pair<int,int> > offList,int outRange,int blocks,int id,void* ptr,int offset):
	std::runtime_error("MemoryPool FAULT."){
	std::string cerr = "";
	cerr = cerr + "\n---HEX DUMP---\n";
	char* ptrGo = (char*)ptr;
	char text = ' ';
	for(int i = 0;i < offset / 10;i++){
		cerr += std::to_string((size_t)ptrGo);
		for(int j = 0;j < 10;j++){
			cerr = cerr + to_hex((int)*ptrGo) + " ";
			ptrGo++;
		}
		ptrGo -= 10;
		cerr += " | ";
		for(int j = 0;j < 10;j++){
			if((int)*ptrGo == 0) text = '.' ;
			else text = *ptrGo;
			cerr = cerr + text;
		}
	ptrGo += 10;
	cerr += "\n";
	}
	cerr += "---OFF DUMP---\noffList[";
	for(auto&& get : offList)
		cerr = cerr + "[" + std::to_string(get.first) + "," + std::to_string(get.second) + "]" + ",";
	cerr += "]";
	cerr += "\n---OTHER PAR---\n";
	cerr = cerr + "id:" + std::to_string(id) 
				+ ",blocks:" + std::to_string(blocks)
				+ ",offset:" + std::to_string(offset);
	cerr += "\n---FAULT---\n";
	cerr = cerr + "Reason:ptr out-range:" + std::to_string(outRange) + "\nSystem Halted.";
	std::cerr << cerr << std::endl;
}
