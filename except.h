#include <iostream>
#include <vector>
#include <utility>
#include <stdexcept>
#include <string>
#include <sstream>

class memoryPool_error : public std::runtime_error{
private:
	std::string to_hex(int n);
public:
	explicit memoryPool_error(std::vector<std::pair<int,int> > offList,int outRange,int blocks,int id,void* ptr,int offset);
};
