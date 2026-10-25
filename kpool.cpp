/*
Copyright 2026 liangtao861
Permission is hereby granted, free of charge, to any person obtaining a copy of this 
software and associated documentation files (the "Software"), to deal in the Software without 
restriction, including without limitation the rights to use, copy, modify, merge, publish, 
distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom 
the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in 
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*
 __  __   ___   _  __  _   _       _   _      _      _____   ____    _   _   _   _   _____ 
|  \/  | |_ _| | |/ / | | | |     | | | |    / \    |_   _| / ___|  | | | | | \ | | | ____|
| |\/| |  | |  | ' /  | | | |     | |_| |   / _ \     | |   \___ \  | | | | |  \| | |  _|  
| |  | |  | |  | . \  | |_| |     |  _  |  / ___ \    | |    ___) | | |_| | | |\  | | |___ 
|_|  |_| |___| |_|\_\  \___/      |_| |_| /_/   \_\   |_|   |____/   \___/  |_| \_| |_____|

*/

#include "kpool.h"
#include <ctime>
#include <random>

#define __KPOOL_FUNC_CALLBACK_SIGNUP(pointer) std::atexit(pointer);
#define __KPOOL_FUNC_CALLBACK_INIT(func) func();
#define __KPOOL_DEBUG false
#define __KPOOL_ASM_GUARD asm volatile("nop":::"memory");

#define __KPOOL_DEAD_CODE_EXT 0

#define __KPOOL_LOG_OUT_VER 0

#define __KPOOL_Q_MAX_SIZE 15

#if __KPOOL_LOG_OUT_VER
    #define __KLOG_PUT_EX(self,level,...) \
	    P_KLOG_PUT(self,level,__VA_ARGS__)
#else
    #define __KLOG_PUT_EX(self,level,...) \
	    ;
#endif
#if __KPOOL_LOG_OUT_VER
    #define __KPOOL_ASSERT(...) \
	    { \
	        bool result = (__VA_ARGS__); \
            if (!result) \
                __KLOG_PUT_EX(logger,klog::logLevel::ERR,"Assertion Failed:" #__VA_ARGS__) \
	        else \
                __KLOG_PUT_EX(logger,klog::logLevel::INF,"Assertion Success:" #__VA_ARGS__) \
            assert(result); \
		}
#else
    #define __KPOOL_ASSERT(...) \
        assert(__VA_ARGS__)
#endif


class poolRd{
public:
	std::mt19937 eng;
	std::uniform_int_distribution<int> dis;
	poolRd()
		:eng(std::random_device()()),dis(0,256) {}
	int rand(){
		return dis(eng);
	}
	//<NOTE> it is kpool library's random class.
	//       old plan:srand have been gave up.
	
};
 
#if __KPOOL_DEAD_CODE_EXT
class ext{
private:
	static void atExit1(){
	    delete memoryPool::manager;
		//<NOTE> when exit program.delete manager self
	}
	static void atExit2(){
		#if __KPOOL_DEBUG
		memoryPool::dump();
		memoryPool::logOut();
	    #endif
		//is planning..
		;
	}
	static void uRandominit(){
		std::srand(std::time(0));
		//<DO> urandom environment init
	}
	static void logInit(){
		#if 0
        klog logOut("\log.txt");
		#endif
		;
	}
	ext(){
		__KPOOL_FUNC_CALLBACK_INIT(uRandominit)
		__KPOOL_FUNC_CALLBACK_INIT(logInit)
		__KPOOL_FUNC_CALLBACK_SIGNUP(atExit1)
		__KPOOL_FUNC_CALLBACK_SIGNUP(atExit2)
	}
	//<NOTE> sign up callback function
	//       when program stop,they will be called.
	//       __KPOOL_FUNC_CALLBACK_SIGNUP is a standard port
	//       do not use it sign up program's other function
	//       DO NOT SIGN UP FUNCTION FROM CLASS WITH HEAP-MEMORY ALLOC
}
#endif


int memoryPool::poolID = 0;
metaSignUp<int>* memoryPool::manager = new metaSignUp<int>();
poolRd& Rd(){
	static poolRd rd1;
	return rd1;
}

namespace{
	[[noreturn]]void terminate_handler(){
		try{
		    P_KLOG_PUT(logger,klog::logLevel::FAT,"Program halted");
		}catch(std::exception &){
		    perror("FATAL:Program stop.");
			std::abort();
		}
        std::abort();
	}
}
				
//ext globalExit = ext();

/* ==================== ptrm<T> ==================== */

template <typename T>
inline uintptr_t ptrm<T>::offset(uintptr_t get) {
    int tmp = sizeof(T);
    return get * tmp;
	/*
	   BUG REPORT #4
	   <PROBLEM> alignof error
	   <SOLVE> use sizeof.Compiler make sure the size is align value.
	*/
}

template <typename T>
inline uintptr_t ptrm<T>::abs(uintptr_t num) {
    return num < 0 ? num * -1 : num;
}

template <typename T>
void ptrm<T>::destory(){
	if constexpr (std::is_object_v<T>){
		for(int _ = 0;_ < metaData->spanBlockSee.size();_++)
		    if (metaData->spanBlockSee[_]) static_cast<T*>((void*)((char*)metaData->startPtr + offsetGet(_ * sizeof(T))))->~T(); 
	}
	urandomFill(0,metaData->seg.second * Q_SIZE);
	metaData->host->kfree(this->blkID());
	metaData->host->manager->ptrRemove(metaData->host->localID,metaData->id);
	metaData = nullptr;
}

template <typename T>
ptrm<T>::ptrm(meta<T>* _md,int _id,int _lV,int _ppos)
	: metaData(_md),ptrPos(_ppos),localVer(_lV),lock(false),localId(_id),nowBlock(0) {
		_md->scAdd();
	}

template <typename T>
ptrm<T>::ptrm(const ptrm<T>& in) 
	: metaData(in.metaData),ptrPos(in.ptrPos),lock(in.lock),localVer(in.localVer),nowBlock(in.nowBlock){
		if(!lock) metaData->scAdd();
		/*
			BUG REPORT #11
			<PROBBLEM> lock member not init rightly.
			<SOLVE>    add set lock member to in.lock member.
		*/
	}

template <typename T>
ptrm<T>::~ptrm(){
	//std::cout << "size:" << shared_count << std::endl;
	if(metaData){
	    metaData->scSub();
	    if(metaData->shared_count == 0){
			P_KLOG_PUT(logger,klog::logLevel::INF,"Free pointer:" + std::to_string(metaData->id))
		    if((!lock)){
	            (*this).reset();
	            if constexpr (std::is_object_v<T>){
					P_KLOG_PUT(logger,klog::logLevel::INF,"To non-POD erase branch.");
	        		for(int _ = 0;_ < metaData->spanBlockSee.size();_++){
	        			if (metaData->spanBlockSee[_]){
							P_KLOG_PUT(logger,klog::logLevel::INF,"free Non-POD self slot=" + std::to_string(_));
							static_cast<T*>((void*)((char*)metaData->startPtr + offsetGet(_ * sizeof(T))))->~T(); 
							#if !NDEBUG
							for(int i = 0;i < 4;i++)
							    *(static_cast<char*>((void*)((char*)metaData->startPtr + offsetGet(_ * sizeof(T) - i))))) = 0xCF;
							//<DO> 0xCF is self destory sign
							#endif
						}
				    }
			        //<NOTE> delete item there.
		    	}
				urandomFill(0,metaData->seg.second * Q_SIZE);
			}
			metaData->host->kfree(metaData->id);
		    metaData->host->manager->ptrRemove(metaData->host->localID,metaData->id);
			metaData = nullptr;
	    }
	}
}

//TODO:????????
template <typename T>
uintptr_t ptrm<T>::offsetGet(uintptr_t voff) const noexcept {
    int i = 0;
	{
		/*
		BUG REPORT #10
		<PROBLEM> it always delete the hottest chunk.
		<SOLVE>   add useit() function
		*/
	    int inBlock = voff / 32+1;
	    if(inBlock == nowBlock){
	    	__KLOG_PUT_EX(logger,klog::logLevel::DBG,"nowBlock=" +
					   std::to_string(nowBlock) + "request voff=" + std::to_string(voff)+
					   "poff=" + std::to_string(voff % 32 + metaData->offList[inBlock].second * Q_SIZE - 
												metaData->offList[1].second * Q_SIZE))
			metaData->tlb.useit(nowBlock);
	     	return voff % Q_SIZE + (uintptr_t)metaData->offList[inBlock].second * Q_SIZE - (uintptr_t)metaData->offList[1].second * Q_SIZE;
	    }
	    //std::cout << "checkMiss" << std::endl;
	}//Fast path.O(1)
	    /* BUG REPORT #3
		   <PROBLEM> TLB calc error.the condition be always true.
		   <SOLVE>   use a member save block now.
		   BUG REPORT #4
		   <PROBLEM> use ptrPos to calc offset
		   <SOLVE>   use voff.and #2 PATH need use ptr - area.first to
		             calc position.and sub 1 to the count tuple.
		   <STU>     have fix.
		*/
	//Two path.Worst situation:O(4)
	{
		__KLOG_PUT_EX(logger,klog::logLevel::INF,"Branch #2:TLB search");
		int inBlock = voff / 32+1;
		auto area = metaData->tlb.useArea();
		__KLOG_PUT_EX(logger,klog::logLevel::INF,"Use area get:Area.start=" + std::to_string((unsigned int)area.first)
				   + " Area.end=" + std::to_string((unsigned int)area.second))
		for(int* ptr = area.first;ptr != area.second;ptr++){
			if((*ptr) == inBlock){
				__KLOG_PUT_EX(logger,klog::logLevel::SUC,"TLB get:" + std::to_string(*ptr))
				metaData->tlb.useit(ptr - area.first);
				return voff % 32 + (uintptr_t)metaData->offList[inBlock].second * Q_SIZE - (uintptr_t)metaData->offList[1].second * Q_SIZE;
			}
		}
	}
    //plan(two_branch_seek)
	__KLOG_PUT_EX(logger,klog::logLevel::INF,"#3 : Fallback to line search.")
    for (i = 1; i <= metaData->offList[0].first; i++) {
        uintptr_t ts = metaData->offList[i].first;
        uintptr_t ns =  metaData->offList[i + 1].first;
		//std::cout << ts << " " << ns << " " << offList[i].second << " " 
		//	<< offList[i + 1].second << " " << i << std::endl;
        if ((ts <= voff) && (ns > voff)) {
            uintptr_t inBlockOffset = voff - ts;	
			uintptr_t physicalOffset = inBlockOffset
				+ (uintptr_t)metaData->offList[i].second * Q_SIZE - (uintptr_t)metaData->offList[1].second * Q_SIZE;
			//<DO>   to physics adr must turn virtual adr into physics adr to startPtr
			//       1. |  IN BLOCK ^ | 2.[ABS OFFSET] ptr -> ^ 3.[ABS OFFSET - START OFFSET] = OFFSET to startPtr
			//<WARNING> the first item is count head.Love from Pascal.
			//std::cout << "result:" << physicalOffset << std::endl;
			nowBlock = i; 
			//<NOTE> Oops:This operator sets newBlock to i
			__KPOOL_ASSERT((metaData->tlb.push(i)) != meta<T>::TLB::errnoTLB::EINVPR);
			return physicalOffset;
        }
    }
	//std::cout << "fallbackl" << std::endl;
    return ENOF;
}

template <typename T>
typename ptrm<T>::outPtr ptrm<T>::get(uintptr_t offset) {
	if((lock)||!(metaData)) THROW(std::logic_error,"undefined ptr")
	if(!(metaData->thisBlocklive)) {
		lock = true;
	}
    return (offset > ((uintptr_t)metaData->seg.second - (uintptr_t)metaData->seg.first) * Q_SIZE)
               ? nullptr
               : (const void* const)((char*)metaData->startPtr + offsetGet(offset));
}

template <typename T>
agent<T> ptrm<T>::operator*() {
	if(lock) THROW(std::logic_error,"undefined ptr")
	if(!(metaData->thisBlocklive)) lock = true;
    return agent<T>(this,metaData->startPtr + offsetGet(ptrPos),ptrPos);
}

template <typename T>
uintptr_t ptrm<T>::id() const noexcept{
	if(lock) return 0;
    return (uintptr_t)(metaData->startPtr + offsetGet(ptrPos));
}

//left copy
template <typename T>
ptrm<T>& ptrm<T>::operator=(ptrm<T>& in){
	if(&in == this){
		return *this;
	}else{
		//if(in.metaData->id == metaData->id);
	    if(metaData){
		    if((in.metaData->id != metaData->id)||(in.metaData->host != metaData->host)) {
			    if(metaData->scSub() == meta<T>::errnoMeta::EINVPR)
					THROW(std::runtime_error,"Invalid shared_count")
			    in.metaData->scAdd();
			    if(!(*metaData)) destory();
		    }
		}else{
			in.metaData->scAdd();
		}
		metaData = in.metaData;
		ptrPos = in.ptrPos;
		lock = in.lock;
		localVer = in.localVer;
		return *this;
	}
	return *this;
}

//right copy
template <typename T>
ptrm<T>& ptrm<T>::operator=(ptrm<T>&& in){
	if(&in == this){
		return *this;
	}else{
		//if(in.metaData->id == metaData->id);
		if(metaData){
		    if((in.metaData->id != metaData->id)||(in.metaData->host != metaData->host)) {
			    if(metaData->scSub() == meta<T>::errnoMeta::EINVPR)
					THROW(std::runtime_error,"Invalid shared_count")
				//<NOTE> fail-fast
			    in.metaData->scAdd();
			    if(!(*metaData))destory();
		    }
		}else{
			in.metaData->scAdd();
		}
		metaData = in.metaData;
		ptrPos = in.ptrPos;
		lock = in.lock;
		localVer = in.localVer;
		in.lock = true;
		in.ptrPos = -1;
		in.metaData = nullptr;
		return *this;
	}
	return *this;
}
		
template <typename T>
int ptrm<T>::blkID() {
	if(lock) THROW(std::logic_error,"ERROR:use a undefined ptr.")
    return metaData->id;
}

template <typename T>
int ptrm<T>::urandomFill(int startPos,int endPos){
	__KPOOL_ASM_GUARD
	__KLOG_PUT_EX(logger,klog::logLevel::INF,"urandomFill function call.");
	unsigned char* endPtr   = ((unsigned char*)(metaData->startPtr)+ offsetGet(endPos));
	std::memset(((unsigned char*)(metaData->startPtr) + offsetGet(startPos)),
				0xff,4);
	//<DO> This memory have been freed.
	unsigned char* ptr =      (unsigned char*)metaData->startPtr + 4;
	int _ = 4;
	while(ptr < endPtr - 4){
		*(ptr) = Rd().rand();
		ptr = (unsigned char*)metaData->startPtr + offsetGet(startPos + _);
		_++;
	}
	std::memset(endPtr - 4,0x6d,4);//guard Page;
	/*
	BUG REPORT #8
    <PROBLEM> signed char use << is Undifined Behavior
	<SOLVE>   use unsigned char.
	<CHANGE>  now is memset to random.
	*/
	//<DO> fill block with random number.
	__KPOOL_ASM_GUARD
	return EOK;
}

template <typename T>
ptrm<T>& ptrm<T>::operator +=(uintptr_t madd){
	if(lock) THROW(std::logic_error,"undefined ptr")
	*this = *this + madd;
	return *this;
}

template <typename T>
ptrm<T>& ptrm<T>::operator -=(uintptr_t msub){
	if(lock) THROW(std::logic_error,"undefined ptr")
	*this = *this - msub;
	return *this;
}

template <typename T>
ptrm<T>::operator bool(){
	return !lock;
}
template <typename T>
bool ptrm<T>::operator==(ptrm<T>& other) const noexcept{
	if(lock) return other.lock;
	return metaData->id == other.metaData->id ? ptrPos == other.ptrPos : id() == other.id();
}

template <typename T>
bool ptrm<T>::operator!=(ptrm<T>& other) const noexcept{
	if(lock) return !other.lock;
	return metaData->id == other.metaData->id ? ptrPos != other.ptrPos : id() != other.id();
}

template <typename T>
bool ptrm<T>::operator<(ptrm<T>& other) const noexcept{
	if(lock) return false;
    return metaData->id == other.metaData->id ? ptrPos < other.ptrPos : id() < other.id();
}

template <typename T>
bool ptrm<T>::operator>(ptrm<T>& other) const noexcept{
	if(lock) return false;
	return metaData->id == other.metaData->id ? ptrPos > other.ptrPos : id() > other.id();
}

template <typename T>
bool ptrm<T>::operator>=(ptrm<T>& other) const noexcept{
	if(lock) return other.lock;
	return metaData->id == other.metaData->id ? ptrPos >= other.ptrPos : id() >= other.id();
}

template <typename T>
bool ptrm<T>::operator<=(ptrm<T>& other) const noexcept{
	if(lock) return other.lock;
	return metaData->id == other.metaData->id ? ptrPos <= other.ptrPos : id() <= other.id();
}

template <typename T>
bool ptrm<T>::operator==(ptrm<T>&& other) const noexcept{
	if(lock) return other.lock;
	return metaData->id == other.metaData->id ? ptrPos == other.ptrPos : id() == other.id();
}

template <typename T>
bool ptrm<T>::operator!=(ptrm<T>&& other) const noexcept{
	if(lock) return !other.lock;
	return metaData->id == other.metaData->id ? ptrPos != other.ptrPos : id() != other.id();
}

template <typename T>
bool ptrm<T>::operator<(ptrm<T>&& other) const noexcept{
	if(lock) return false;
    return metaData->id == other.metaData->id ? ptrPos < other.ptrPos : id() < other.id();
}

template <typename T>
bool ptrm<T>::operator>(ptrm<T>&& other) const noexcept{
	if(lock) return false;
	return metaData->id == other.metaData->id ? ptrPos > other.ptrPos : id() > other.id();
}

template <typename T>
bool ptrm<T>::operator>=(ptrm<T>&& other) const noexcept{
	if(lock) return other.lock;
	return metaData->id == other.metaData->id ? ptrPos >= other.ptrPos : id() >= other.id();
}

template <typename T>
bool ptrm<T>::operator<=(ptrm<T>&& other) const noexcept{
	if(lock) return other.lock;
	return metaData->id == other.metaData->id ? ptrPos <= other.ptrPos : id() <= other.id();
}

template <typename T>
bool ptrm<T>::operator==(void* other) const noexcept{
	if((byte)other == 0) return lock;
	if(lock)             return false;
	return this->id() == (uintptr_t)other;
}

template <typename T>
bool ptrm<T>::operator!=(void* other) const noexcept{
	if((byte)other == 0) return lock;
	if(lock)             return false;
	return this->id() != (uintptr_t)other;
}

template <typename T>
bool ptrm<T>::operator<(void* other) const noexcept{
	if(lock) return false;
    return this->id() < (uintptr_t)other;
}

template <typename T>
bool ptrm<T>::operator>(void* other) const noexcept{
	if(lock) return false;
	return this->id() > (uintptr_t)other;
}

template <typename U>
bool operator==(void* s1,ptrm<U>& s2) noexcept{
	if((byte)s1 == 0) return s2.lock;
	if(s2.lock)       return false;
	return (uintptr_t)s1 == s2.id();
}

template <typename U>
bool operator!=(void* s1,ptrm<U>& s2) noexcept {
	if((byte)s1 == 0) return !s2.lock;
	if(s2.lock)       return false;
    return (uintptr_t)s1 != s2.id();
}

template <typename U>
bool operator<(void* s1,ptrm<U>& s2) noexcept{
	if(s2.lock)       return false;
    return (uintptr_t)s1 < s2.id();
}

template <typename U>
bool operator>(void* s1,ptrm<U>& s2) noexcept{
	if(s2.lock) return false;
    return (uintptr_t)s1 > s2.id();
}

template <typename U>
bool operator<=(void* s1,ptrm<U>& s2) noexcept{
	if(s2.lock) return false;
    return (uintptr_t)s1 <= s2.id();
}

template <typename U>
bool operator>=(void* s1,ptrm<U>& s2) noexcept{
	if(s2.lock) return false;
    return (uintptr_t)s1 >= s2.id();
}

template <typename U>
bool operator==(void* s1,ptrm<U>&& s2) noexcept{
	if(s2.lock) return !s1;
	return (uintptr_t)s1 == s2.id();
}

template <typename U>
bool operator!=(void* s1,ptrm<U>&& s2) noexcept{
	if(s2.lock) return s1;
    return (uintptr_t)s1 != s2.id();
}

template <typename U>
bool operator<(void* s1,ptrm<U>&& s2) noexcept{
	if(s2.lock) return false;
    return (uintptr_t)s1 < s2.id();
}

template <typename U>
bool operator>(void* s1,ptrm<U>&& s2) noexcept{
	if(s2.lock) return false;
    return (uintptr_t)s1 > s2.id();
}

template <typename U>
bool operator<=(void* s1,ptrm<U>&& s2) noexcept{
	if(s2.lock) return !s1;
    return (uintptr_t)s1 <= s2.id();
}

template <typename U>
bool operator>=(void* s1,ptrm<U>&& s2) noexcept{
	if(s2.lock) return !s1;
    return (uintptr_t)s1 >= s2.id();
}

template <typename T>
ptrm<T> ptrm<T>::operator+(int madd) {
	if(lock) THROW(std::logic_error,"undefined ptr")
	int tpos = ptrPos + offset(madd);
	if((tpos >= metaData->seg.second* Q_SIZE)||(tpos < 0)) 
		THROW(std::out_of_range,"Segmentation(virtual) FAULT error(system core dumped).")
	//for(auto&& i : coffList) std::cout << i.first << " " << i.second << std::endl;
	return ptrm(metaData,localId,localVer,tpos); 
}

template <typename T>
ptrm<T> ptrm<T>::operator-(int msub) {
	if(lock) THROW(std::logic_error,"undefined ptr")
	int tpos = ptrPos - offset(msub);
	if((tpos < 0)||(tpos >= metaData->seg.second * Q_SIZE))
		THROW(std::out_of_range,"Segmentation(virtual) FAULT error(system core dumped).")
	return ptrm(metaData,localId,localVer,tpos); 
}

template <typename T>
ptrdiff_t ptrm<T>::operator-(ptrm<T>& msub) const noexcept{
	//if(lock) THROW(std::logic_error,"undefined ptr")
	if(lock) return 0;
	return metaData->id == msub.metaData->id ? 
		(ptrdiff_t)(ptrPos - msub.ptrPos) / sizeof(T) :
	    (ptrdiff_t)((uintptr_t)metaData->startPtr + offsetGet(ptrPos) - (uintptr_t)msub.metaData->startPtr + offsetGet(ptrPos)) / sizeof(T);
}

template <typename T>
ptrm<T>& ptrm<T>::operator++() {
	if(lock) THROW(std::logic_error,"undefined ptr")
	if(ptrPos + offset(1) >= metaData->seg.second * Q_SIZE)
		THROW(std::out_of_range,"ptr out range")
	ptrPos += offset(1);
	return *this;
}

template <typename T>
ptrm<T>& ptrm<T>::operator--() {
	if(lock) THROW(std::logic_error,"undefined ptr")
	if(ptrPos - offset(1) < 0)
		THROW(std::out_of_range,"Ptr out-range")
	ptrPos -= offset(1);
	return *this; 
}

template <typename T>
ptrm<T> ptrm<T>::operator++(int){
	ptrm<T> temp = *this;
	this->operator++();
	return temp;
}

template <typename T>
ptrm<T> ptrm<T>::operator--(int){
	ptrm<T> temp = *this;
	this->operator--();
	return temp;
}

template <typename T>
agent<T> ptrm<T>::operator[](int madd){
	return *(this->operator+(madd));
}
template <typename T>
int ptrm<T>::bufCopytm(void* dest,void* src,uintptr_t startPos){
	char* destPool = static_cast<char*>(dest);
	char* srcPool = static_cast<char*>(src);
	__KLOG_PUT_EX(logger,klog::logLevel::INF,"call bufCopy.dest=" + std::to_string((uintptr_t)dest)
			   + " src=" + std::to_string((uintptr_t)src) + " startPos=" + std::to_string(startPos)
			   + " T_size=" + std::to_string(sizeof(T)))
	if(startPos > metaData->seg.second * Q_SIZE) return EPTRIN;
	for(int i = 0;i < sizeof(T);i++){
		destPool = (char*)metaData->startPtr + offsetGet(startPos + i);
	    *(destPool) = *(srcPool);
		srcPool++;
	}
	return EOK;
}

template <typename T>
int ptrm<T>::bufCopytp(void* dest,void* src,uintptr_t startPos){
	char* destPool = (char*)dest;
	char* srcPool = (char*)src;
	if(startPos > metaData->seg.second * Q_SIZE) return EPTRIN;
	for(int i = 0;i < sizeof(T);i++){
		destPool = (char*)metaData->startPtr + offsetGet(startPos + i);
		*(srcPool) = *destPool;
		srcPool++;
	}
	return EOK; 
}

template <typename T>
ptrm<T>::proxyTmp<T> ptrm<T>::operator->() {
	if(lock) THROW(std::logic_error,"undifiend ptr")
    auto findNeibour = [this](int pos,int size){
		int needBlock = size % 32 ? size / 32 + 1:size / 32;
		int blockPos = 0;
		for (int i = 1; i <= metaData->offList[0].first; i++) {
		    uintptr_t ts = metaData->offList[i].first;
		    uintptr_t ns = metaData->offList[i + 1].first;
		    if ((ts <= pos) && (ns > pos)) {
		        blockPos = metaData->offList[i].second;
		        break;
		    }
		}
		if(metaData->offList[pos / Q_SIZE + needBlock].second != pos + needBlock) return false;
		return true;
	};
	if(!(Q_SIZE / sizeof(T))
	   &&((ptrPos % Q_SIZE)+ sizeof(T))
	   &&(!(findNeibour(ptrPos,sizeof(T))))){
		//std::cout << "there" << std::endl;
		void* mem = malloc(sizeof(T));
		bufCopytp(mem,metaData->startPtr + offsetGet(ptrPos),ptrPos);
		return proxyTmp<T>(static_cast<T*>(mem),true,this,ptrPos,static_cast<T*>(metaData->startPtr + offsetGet(ptrPos)));
	}else{
		return proxyTmp<T>(static_cast<T*>(metaData->startPtr + offsetGet(ptrPos)),false,this);
	}
}

template <typename T>
ptrm<T>::operator int() {
    return blkID();
}

template <typename T>
void ptrm<T>::reset() {
	if(lock) THROW(std::logic_error,"undefined ptr")
    ptrPos = 0;
}

/* ==================== memoryPool ==================== */

std::vector<std::pair<int, int>> memoryPool::build(const std::multimap<int,std::pair<int,int>>& blocks){
    std::vector<std::pair<int,int>> buildArr = {}; 
	{
		
	    for(auto& get : blocks){ 
	        buildArr.push_back(std::make_pair(0,get.second.second)) ;
	    } 
		//buildArr.push_back(std::make_pair(0,0));
		std::sort(buildArr.begin(),buildArr.end(),[](auto& a,auto& b){
			return a.second < b.second;
		});
		for(int _ = 0;_ < buildArr.size();_++){
			buildArr[_] = std::make_pair(0 + (_) * Q_SIZE,buildArr[_].second);
		}
		//for(auto& a : buildArr)
		//	std::cout << a.first << " " << a.second << " " << std::endl;
	}
	buildArr.insert(buildArr.begin(),std::make_pair(buildArr.size(),0));
	buildArr.push_back(std::make_pair((buildArr.end() - 1)->first + 32,(buildArr.end() - 1)->second + 1));
	return buildArr;
}

inline int memoryPool::getID() {
    return (++id);
}

inline int memoryPool::getPoolID(){
	return (++poolID);
}

void memoryPool::eraseFree(int id) {
    freeBlock.erase(id);
}

memoryPool::memoryPool() : ptr(nullptr), endPtr(nullptr), id(0), count(0), initSign(0), notallowed(false) {}

memoryPool::~memoryPool() {
	manager->ptrtable();
	if(initSign){
        if (count != 0) manager->metaRemove(localID);
	    metaSignUp<int>::bufEra(ptr);
	    int get = poolREG::eraPool(localID - 1);
        if(!notallowed) free(ptr);
	    if(get == poolREG::errnoReg::EOK);
	    else if(get == poolREG::errnoReg::ENNOT){
		    get = poolREG::stuChg(localID - 1,poolREG::errnoReg::EFALSE); //fallback there
		    if(get == poolREG::errnoReg::ENNOT) THROW(std::logic_error,"FATAL:Invalid erase reg.")
	    }else if(get == poolREG::errnoReg::ERRFOD){
		    THROW(std::logic_error,"FATAL:Program logic error")
		}
	}
}

int memoryPool::segManager::segManager(int segStart,int segEnd){
	segTuple.push_back(std::make_pair(segStart,segEnd));
}

int memoryPool::segManager::cut_chunk(int segStart,int size){
	int _ = 0;
	for(auto& in : segTuple){
		if(segStart >= in.first)&&(segEnd <= in.second)
			break;
		_++;
		//<NOTE> situation : in this segment.
    }
	if((segTuple[_].second - segTuple[_].first) > size){
		__KLOG_PUT_EX(logger,klog::logLevel::DBG,
					  std::string(std::sprintf("branch#1 : cut chunk: sub_Value=%d,size=%d",
															  segTuple[_].second - segTuple[_].first,size)))
																		
	    segTuple.push_back(std::make_pair(segTuple[_].first,segTuple[_].first + segStart));
	    segTuple.push_back(std::make_pair(segTuple[_].first + segStart + size,segTuple[_].second));
		segTuple.erase(segTuple.start() + _);
	}else if((segTuple[_].second - segTuple[_].first) == size){
		__KLOG_PUT_EX(logger,klog::logLevel::DBG,std::string(std::sprintf("branch#2: cut all chunk : size=%d",size)))
		segTuple.erase(segTuple.start() + _);
	}else if((segTuple[_].second - segTuple[_].first) < size){
		return EC;
	}
	std::sort(segTuple.begin(),segTuple.end());
	return EOK;
}

int memoryPool::segManager::merge_chunk(int chunkStart){
	int pos = (std::find_if(segTuple.begin(),segTuple.end() - 1,[](auto&& in) -> bool{
		return (in.second <= chunkStart)&&((&in + 1)->first >= chunkStart);
	})) - segTuple.begin();
	__KPOOL_ASSERT(pos != segTuple.size())
	__KLOG_PUT_EX(logger,klog::logLevel::DBG,std::string(std::sprintf("merge pos=%d",pos)));
	segTuple.push_back(std::make_pair(segTuple[position].second - segTuple[position].first));
	segTuple.erase(segTuple.begin() + position);
	segTuple.erase(segTuple.begin() + position + 1);
	std::sort(segTuple.begin(),segTuple.end())
	return EOK;
}

int memoryPool::segManager::need_chunk_seg(int blocks){
	int _ = 0;
	std::vector<std::pair<int,int>> bv;
	for(auto& get : segTuple){
		if((get.second - get.first) >= blocks)
			bv.push_back(std::make_pair(get.second - get.first,_));
		_++;
	}
	if(bv.empty()) return ENOF;
	__KLOG_PUT_EX(logger,klog::logLevel::DBG,
				  std::string(std::sprintf("bv get pos=%d",bv.element_min(bv.begin(),bv.end()).second)))
	return *(bv.element_min(bv.begin(),bv.end())).second;
}

int memoryPool::init(int size, void* in) {
	std::set_terminate(terminate_handler);
	metaSignUp<int>::buf();
	if(initSign) THROW(std::logic_error,"ERROR:re-init warning")
    if (in == nullptr) {
        if (size % Q_SIZE) size = size + (Q_SIZE - (size % Q_SIZE));
        ptr = malloc(size);
        if (ptr == NULL) THROW(std::runtime_error,"memory too min")
    } else {
		if (size % Q_SIZE) size = size - (size % Q_SIZE);
		notallowed = true;
		if(!metaSignUp<int>::bufUse(in)){
			metaSignUp<int>::bufCrt(in);
		    ptr = in;
		}else{
			notallowed = false;
			in = nullptr;
			THROW(std::logic_error,"ERROR:use a block next.")
		}
	}
    endPtr = (void*)((char*)ptr + size);
    {
        int segStart = 0;
        int segEnd = 1;
        int id = 0;
        for (int i = 0; i < (size / Q_SIZE); i++) {
            allBlock.insert(std::pair<int, std::pair<int, int>>(id, std::make_pair(segStart, segEnd)));
            segStart += 1;
            segEnd += 1;
            id += 1;
			__KLOG_PUT_EX(logger,klog::logLevel::DBG,"segStart=" + std::to_string(segStart) + 
					   " segEnd=" + std::to_string(segEnd) + " segId=" + std::to_string(id))
        }
		//<NOTE> build a block table in law.
    }
	unsigned char* c = (unsigned char*)ptr;
	while((c++)!= endPtr)
		*c = 0x3d;
    for (auto& getSeg : allBlock) {
        freeBlock.insert(std::pair<int, std::pair<int, int>>(getSeg.first, getSeg.second));
    }
	localID = getPoolID();
    totalSize = size;
    initSign = 1;
    //useBlock.insert(std::pair<int, std::pair<int, int>>(0, std::make_pair(0, 0)));
    manager->ptrtable();
	poolREG();
	//<NOTE> no-op but can initzation static member.
	//<WHY>  CPP standard make sure static member will init 
	//       when first time that call the Meyer.
	int get = poolREG::crtNewPool();
	buddyAlloc = segManager(0,size);
	//<DO>   This sentence init buddyAllocter
	if(get == poolREG::errnoReg::EOK){	
        return 0;
	}else if(get == poolREG::errnoReg::EALC){
		poolREG::clear();
		throw std::bad_alloc();
	}
    assert(false);
}

void* memoryPool::getPtr() {
    return ptr;
}

int memoryPool::needFreeBlk(int blkS){
    std::vector<std::pair<int,int>> blkS;
	if(blkS > freeQueue.size()) return EALC;
	for(int i = 0;i < blkS;i++){
		freeBlock.insert(freeQueue.top());
                blks.push_back(freeQueue.top());
		freeQueue.pop();
	}
	return EOK;
    std::sort(blks.begin(),blks.end(),[](auto&& in) -> bool{
		return in->second.first < in->second.first;
	});
	int run = 0;
	int runPos = 0;
	for(int _ = 0;_ < blks.size() - 1;_++){
	    if(blks[_ + 1]->second.first  - blk[_]->second.first != 1){
		    buddyAlloc.merge_chunk
		   	   (blks[0]->second.first + runPos);
		    run = 0;
		    runPos = _;
		}
		run++;
    }
}

void memoryPool::signFreeList(){
	if(freeQueue.size() > __KPOOL_Q_MAX_SIZE){
		P_KLOG_PUT(logger,klog::logLevel::INF,
            "stack size > __KPOOL_Q_MAX_SIZE.push.")
        for(int i = 0;i < __KPOOL_Q_MAX_SIZE;i++){
            freeBlock.insert(freeQueue.top());
			blks.push_back(freeQueue.top());
			freeQueue.pop();
		}
		std::sort(blks.begin(),blks.end(),[](auto&& in) -> bool{
		    return in->second.first < in->second.first;
	    });
	    int run = 0;
	    int runPos = 0;
	    for(int _ = 0;_ < blks.size() - 1;_++){
		    if(blks[_ + 1]->second.first  - blk[_]->second.first != 1){
			    buddyAlloc.merge_chunk
				   (blks[0]->second.first + runPos);
			    run = 0;
			    runPos = _;
				__KLOG_PUT_EX(logger,klog::logLevel::DBG,
							  "runPos=" + std::to_string(runPos) + "merge chunks.")
		    }
		    run++;
        }
	}
}

void memoryPool::kfree(int id) {
    if (!initSign) THROW(std::logic_error,"uninitializational memory space")
    if (useBlock.empty()) THROW(std::invalid_argument,"useBlock is empty.Please create new blocks.")
    auto rg = useBlock.equal_range(id);
    if ((rg.first == rg.second)&&
		((allBlock.equal_range(id).first) != (allBlock.equal_range(id).second))){
		THROW(std::logic_error,"FATAL:double free.")
	}
	else if(rg.first == rg.second)
		THROW(std::logic_error,"FATAL:Invalid block.")
	//<NOTE> the way is very ugly.But it can sub a left-value.
	//       if(NOT_FOUND_BLK(FREEBLPCK) and (not NOT FOUND_BLK(USEBLOCK)):
    //            throw #can fina in freeBlock.Oops:Double free
    //       elif(NOT_FOUND_BLK(FREEBLOCK))
    //            throw #can't find in freeBlock and useBlock.Oops:invalie block.
    //<PROBLEM> I don't know if it can use errCode because it will be a private method.
    for (auto it = rg.first; it != rg.second; it++)
		freeQueue.push(std::pair<int,std::pair<int,int>>(
			it->first, std::make_pair(it->second.first, it->second.second)));
		
	//<NOTE> easy.insert to freeBlock and erase from useBlock. 
        signFreeList();
	count -= std::distance(rg.first,rg.second);
    useBlock.erase(id);
}

memoryPool::bPtr memoryPool::getBasicPtr(int size,int id){
	P_KLOG_PUT(logger,klog::logLevel::INF,"memoryGet process +++ size="+std::to_string(size)+" id="+std::to_string(id))
	int sign = EOK;
	int needBlock = size % Q_SIZE ? size / Q_SIZE + 1 : size / Q_SIZE;
	/* 
	    BUG REPORT #2 2026.10.17
		<PROBLEM> can't jump to clean point,and control line continues.
		<SOLVE>   planA:use a recall function?
                  planB:can use [goto] --- But the worst solve plan
                  planC:can use [throe] --- (NO)
	*/
	if (!initSign) {
		sign = EINIT;
		goto err;
	}
	if (needBlock * Q_SIZE > totalSize - count * Q_SIZE) {
		sign = EINV;
		goto err;
	}
	//if (id > this->id + 1) sign = EINV;
	signFreeList();
	if(buddyAlloc.need_chunk_seg(needBlock)) == segManager::errSeg::ENOF){
		std::multimap<int, std::pair<int, int>> blocks;
		int _ ;
		{
		    _ = 0;
		    auto basic_bigseg = freeBlock.begin();
		    for (; (_ < needBlock) && (basic_bigseg != freeBlock.end()); _++) {
		        blocks.insert(std::pair<int, std::pair<int, int>>(
		            basic_bigseg->first,
		            std::make_pair(basic_bigseg->second.first, basic_bigseg->second.second)));
		        basic_bigseg = freeBlock.erase(basic_bigseg);
				//<NOTE> this loop insert new blocks and erase old freeBlock
		    }
		    if(id == -1) id = getID();
		    for (auto& bget : blocks) {
		        useBlock.insert(std::pair<int, std::pair<int, int>>(
		            id, std::make_pair(bget.second.first, bget.second.second)));
				__KLOG_PUT_EX(logger,klog::logLevel::DBG,
							  "bget.second.first=" + std::to_string(bget.second.first) +
							  "bget.second.second=" + std::to_string(bget.second.second) +
							  "id=" + std::to_string(id))
		    }
		}
		if(_ < needBlock){
			kfree(id);
			if(needFreeBlk(needBlock) == EALC){
			    sign = EALC;
			    goto err;
			}else{
				return getBasicPtr(size,id);
			}
		}
		count += needBlock;
		//for(auto&& get : blocks) std::cout << get.first << get.second.first << get.second.second << std::endl;
		bPtr s = {(void*)((char*)ptr + (blocks.begin()->second.first) * Q_SIZE),blocks,needBlock,id,EOK}; 
		return s;
	}else{
		if(id == -1) id = getID();
		int   g = buddyAlloc.need_chunk_seg(needBlock);
		auto& c = buddyAlloc.get_addr_pair(g); //planning
		buddyAlloc.cut_chunk(g,needBlock);
		auto it = std::find(freeBlock.begin(),freeBlock.end(),[](auto&& in)->bool{
			in->second.first = c.first;
		});
		for(int _ = 0;_ < needBlock;_++){
			useBlock.insert(it);
			it = freeBlock.erase(it);
		}
		count += needBlock;
		std::multimap<int,std::pair<int,int>> blocks;
		blocks.insert(std::pair<int,std::pair<int,int>>(id,std::make_pair(c.first,c.second)))
		return {(void*)((char*)ptr + c.first * Q_SIZE),,needBlock,id,EOK};
		//std::sort(blks.begin(),blks.end(),[](auto&& in) -> bool{
		//return in->second.first < in->second.first;
	});
	
	//clean point
	err:
	    return {nullptr,std::multimap<int,std::pair<int,int>>(),0,-1,sign};
}

template <typename T>
ptrm<T> memoryPool::kmalloc(int size2) {
	if((size2 == 0)||(size2 < 0)) THROW(std::invalid_argument,"ERROR:Invalid size.")
	uintptr_t size = (uintptr_t)size2;
    memoryPool::bPtr getPtr = getBasicPtr(size);
    if(getPtr.sign == EINV) THROW(std::invalid_argument,"ERROR:Ivalid size parameter")
    else if(getPtr.sign == EALC) throw std::bad_alloc();
    else if(getPtr.sign == EINIT) THROW(std::logic_error,"ERROR:Not Init.")
    //auto tmp = make_shared<meta<T>>(meta<T>(std::make_pair(0,getPtr.needBlock),
	//			        build(getPtr.map),
	//					getPtr.basicPtr,
	//					getPtr.id));
	metaSignUp<T> tmp = metaSignUp<T>(getPtr.id,localID,meta<T>(std::make_pair(0, getPtr.needBlock),
                             build(getPtr.map),
                             getPtr.basicPtr,
                             getPtr.id,this));
    return ptrm<T>(tmp.getMeta(localID,getPtr.id),localID);
}

template <typename T>
ptrm<T> memoryPool::kcalloc(int item, int itemSize, int filled) {
	if((item <= 0)||(itemSize <= 0)) THROW(std::invalid_argument,"Error:Not allowed nagetive size")
	int size = item * itemSize;
    memoryPool::bPtr getPtr = getBasicPtr(size);
    if(getPtr.sign == EINV) THROW(std::invalid_argument,"ERROR:Ivalid size parameter")
	else if(getPtr.sign == EALC) throw std::bad_alloc();
	else if(getPtr.sign == EINIT) THROW(std::logic_error,"ERROR:Not Init.")
	metaSignUp<T> tmp = metaSignUp<T>(getPtr.id,localID,meta<T>(std::make_pair(0, getPtr.needBlock),
	                        build(getPtr.map),
							getPtr.basicPtr,getPtr.id,this));
	ptrm<T> get = ptrm<T>(tmp.getMeta(localID,getPtr.id),localID);
    char* sizeClear = (char*)getPtr.basicPtr;
	__KPOOL_ASM_GUARD
    for(int i = 0;i < getPtr.needBlock * Q_SIZE ;i++)	
		*(sizeClear + get.offsetGet(i)) = (char)filled;
	__KPOOL_ASM_GUARD
    return get;
}

template <typename T>
ptrm<T> memoryPool::krealloc(ptrm<T> oldPtr, uintptr_t newSize) {
    uintptr_t oldSize = (oldPtr.metaData->seg.second - oldPtr.metaData->seg.first) * Q_SIZE;
	uintptr_t newNeedBlock = newSize % Q_SIZE ? newSize / Q_SIZE + 1 : newSize / Q_SIZE;
	if(newSize < oldSize){
		if(newSize == 0){
			oldPtr.destory();
			oldPtr.lock = true;
			return oldPtr;
		}else if(newSize == SIZE_MAX){
			THROW(std::invalid_argument,"can't realloc negative size memory.")
		}
		auto cPtr = oldPtr;
		oldPtr.metaData->seg.second = newNeedBlock;
		oldPtr.metaData->offList[0].first = newNeedBlock;
		if(oldPtr.ptrPos > newNeedBlock * Q_SIZE) {
			oldPtr.ptrPos = 0;
			oldPtr.reset();
		}
		if constexpr(!(std::is_trivially_copyable_v<T>)){
			/*
			   BUG REPORT #9
			   <PROBLEM> the condition is true.FOREVER
			   <SOLVE>   oldSize to the checker head
			   <STU>     have fix now.
			*/
		    for(int _ = 0;_ < (oldSize - (newNeedBlock * Q_SIZE)) / sizeof(T);_++)
			    if (oldPtr.metaData->spanBlockSee[_]) (cPtr + ((newNeedBlock * Q_SIZE - oldSize) / sizeof(T)))->~T();
		}
		{
	        std::vector<std::pair<int,int>> delBlk;
		    for(auto&& get = oldPtr.metaData->offList.begin() + (oldPtr.metaData->offList[0].first - newNeedBlock);
			    get != oldPtr.metaData->offList.end();
			    get++){
				delBlk.push_back(*get);
			}
	        int _ = 0;
	        for(auto&& get = useBlock.begin();
				(get != useBlock.end())&&(_ < delBlk.size());){
				if(get->second.second == delBlk[_].second){
					freeQueue.push(std::pair<int,std::pair<int,int>>(
									 std::make_pair(get->first,
									 std::make_pair(get->second.first,get->second.second)
									 )));
					get = useBlock.erase(get);
					_++;
					continue;
					//<NOTE>   This loop is for delete blocks
                    //<WARNING>But this loop maybe delete empty block and the 
					//         condition can't be true and jump out of this loop
					//<PAR>    get->first : useBlock's ID(In fact,freeBlock need Block ID,but it is never used.)
                    //         get->second.first - segStart get->second.second - segEnd.
					//<WHY>    But the ireator always be not used.So I use erase get next ptr in the if-block.
					//         .When out of it.I use get++ (add 1)Delete block from the tail
					//         so easy.
				}
				get++;
				if(_ == delBlk.size()) break;
			}
		}
		oldPtr.metaData->offList.erase(oldPtr.metaData->offList.begin() + (oldPtr.metaData->offList[0].first - newNeedBlock),
							 oldPtr.metaData->offList.end());
		if constexpr(!(std::is_trivially_copyable_v<T>))
		    oldPtr.metaData->spanBlockSee.erase(oldPtr.metaData->spanBlockSee.begin() + newNeedBlock * Q_SIZE / sizeof(T),
											oldPtr.metaData->spanBlockSee.end());
		(oldPtr->metaData.ver)++;
		(oldPtr.localVer)++;
		//<NOTE>:update master version and update control ptrm's ver
		return oldPtr;
	}else if(newSize > oldSize){
	    auto safeCpy = [oldPtr](int start,int end,int start1,int end1,bool isobject){
			int step = sizeof(T);
			if (isobject){
			    for(int _1 = 0,_2 = 0;(_2 < (start - end) / sizeof(T))&&(_1 < (start1 - end1) / sizeof(T));_1++,_2++)
					new (oldPtr->metaData.startPtr + oldPtr.offsetGet(_1 * sizeof(T))) 
					    T(oldPtr->metaData.startPtr + oldPtr.offsetGet(_2 * sizeof(T)));
			#define __KPOOL_ERR_SIGN 0xCC
			}else if (!isobject){
				char* ptr1 = (char*)oldPtr->metaData.startPtr + oldPtr.offsetGet(start);
				char* ptr2;
				char  reWrite;
				for(int _ = 0;ptr1 < (char*)oldPtr->metaData.startPtr + oldPtr.offsetGet(end);_++){
					ptr2 = (char*)oldPtr->metaData.startPtr + oldPtr.offsetGet(start1 + _);
					if((ptr2 >= oldPtr->metaData.startPtr + oldPtr.offsetGet(end1))
						reWrite = __KPOOL_ERR_SIGN;
					else{
						reWrite = *ptr2;
						ptr2 = (char*)oldPtr->metaData.startPtr + oldPtr.offsetGet(start1 + _);
					}
				    ptr1 = (char*)oldPtr->metaData.startPtr + oldPtr.offsetGet(start + _);
					*ptr1 = reWrite;
				}
			}
			//delete loop
			if (isobject){
				for(int _ = 0;_ < (start - end) / sizeof(T);_++){
				    if(oldPtr->metaData.spanBlockSee[start / sizeof(T) + _])
						static_cast<T*>((char*)(oldPtr->metaData.startPtr + oldPtr.offsetGet(start + _ * sizeof(T))))->~T();
				}
			}
			//move loop
			//for(;;){}
			return;
		};
		auto translateToClassic(std::vector<std::pair<int,int>>& offList ,int size){}
		auto mid_check = [](std::vector<std::pair<int,int>> oldList,
							std::vector<std::pair<int,int>> newList) -> std::vector<std::pair<bool,int>>{
			std::vector<std::pair<int,int>> diff = {}; //<NOTE> create a none diff
			std::vector<std::pair<bool,int>>    mid  = {}; //<NOTE> create a vector pic
			for(int i = 1,a = 1;;){
				if(oldList[i].second != newList[a].second) diff.push_back(std::make_pair(a,newList[a].second - oldList[i].second));
				//blcok Number.
			    if(i < oldList.begin()->first) i++;
				if(a < newList.begin()->first) a++;
				if((i == oldList.begin()->first)&&(a == oldList.begin()->first))
					break;
				//<DO> this function can return a offList and newLffList's Diff
				//      |-------|-------|-------|+++++++|-------|+++++++|
				//      -   1   -   2   -   3   -   4   -   5   -  new  |
				//      |-------|-------|-------|+++++++|-------|+++++++|
				//      false   false   false   true,-32 false  false
				//<PSC> for i in list:
				//           if oldList[i] != newList[i]:
				//                diff.append(oldList[i] - newList[i])
				//<NOTE>Reserve ;)
			}
			for(int i = 1;i < newList.begin()->first;i++)
				mid.push_back(std::make_pair(false,0));
			mid.push_back(std::make_pair(false,0)); //sign place
			for(int i = 0;(i < diff.size())&&(i < newList[0].first);i++){
				if(diff[i].second != 0){
					mid[diff[i].first].first = true;
					mid[diff[i].first].second = diff[i].second;
					mid[mid.size() - 1].first = true;
				}
			}
		    return mid;
		};
		if(newSize == SIZE_MAX){
			THROW(std::invalid_argument,"can't realloc nagetive size memory.")
		}
		bPtr newGet = getBasicPtr(newNeedBlock * Q_SIZE,oldPtr.metaData->id);
		std::vector<std::pair<int,int>> newMap  = build(newGet.map);
		std::vector<std::pair<int,int>> oldList = translateToClassic(oldPtr->metaData.offList,oldSize);
		for(auto&& get = newMap.begin() + 1;get != newMap.end();get++)
			oldPtr.metaData->offList.push_back(*get);
	    std::sort(oldPtr.metaData->offList.begin() + 1,oldPtr.metaData->offList.end(),
				  [](auto& a,auto& b) -> bool{
					  return a.second < b.second;
				  }
				 );
		for(int _ = 1;_ < oldPtr.metaData->offList.first;_++)
			oldPtr.metaData->offList[_].first = _ * Q_SIZE;
		int pos = 0;
		{
		    auto&& get = std::lower_bound(oldPtr.metaData->offList.begin() + 1,
						 oldPtr.metaData->offList.end(),
						 oldList[1].second,
						 [](const auto& p,int val) -> bool{
							 return p.second < val;
						 });
			assert(get == oldPtr.metaData->offList.begin() + 1);
			//<DO> lower_bound the old start position.Don't find begin()
			//     because the first item is the count head
			//     32 0      0 32
			//      ^        ^
			//     BUG:find error
			pos = get - oldPtr.metaData->offList.begin() + 1;
		}
		//<NOTE> this alogrim call will sort offset List for ptrm
		//       But the oldFirst will go to the start and new also.
		//       So I need to make a condition to allow the span repair
		//<PSC>  get = offList.find(pair(0,[BASIC])
		//       if get != offList + 1:
		//           [SORT_LOGIC] #But when the block in the tail.Not have fault
		//       elif func_check(offList_old,offList_new):
		//           [SORT_LOGIC]
		//       else:
		//           return newPtr
		//------------------------------------------------------------------------
		// BUG REPORT #5
		// <PROBLEM> when the get block in the middle or in the head
		//           program can't work well
		// <SOLVE>   use condition check
		auto chk = mid_check(oldList,oldPtr->metaData.offList);
		bool sign = chk[chk.size() - 1];
		
		//<NOTE> debug marcos
	    #define __KPOOL_SORT_LIST false
	    #define __KPOOL_BUILD     false
	    
	    #if __KPOOL_BUILD
		if(sign){
			int _ = 0;
			int lastPos = get.second;
			std::for_each(chk.begin(),chk.end() + 1,[=](auto && in){
				if(in.first){
					safeCpy((in - chk.begin()) * Q_SIZE),
							((in - chk.begin()) * Q_SIZE) + Q_SIZE),
							((in - chk.begin()) * Q_SIZE + in.second),
							((in - chk.begin()) * Q_SIZE + in.second) + Q_SIZE),
					        std::is_trivially_copyable_v<T>
						   );
					_++;
				}
			}
		    oldPtr.metaData->startPtr = (void*)((char*)oldPtr.metaData->startPtr - (_ * Q_SIZE));
			oldPtr.ptrPos -= (_ * Q_SIZE);
		}
		#endif
		oldPtr.metaData->segAdd(newNeedBlock - oldPtr.metaData->seg.second);
		oldPtr.metaData->spanBlockSee.resize(newNeedBlock * Q_SIZE / sizeof(T));
		(oldPtr.metaData->ver)++;
	    (oldPtr.localVer)++;
		return oldPtr;
	}else if(newSize == oldSize){
	    return oldPtr;
	}else{
		THROW(std::logic_error,"unknown error.")
		return oldPtr;
	}
}
 
template <typename T>
ptrm<T> memoryPool::ksalloc(int size,std::function<T()> funcBuild){
	size = !(size % Q_SIZE) ? size : (size / Q_SIZE + 1) * Q_SIZE;
	std::cout << size << std::endl;
	/*
        BUG REPORT #1 2026.10.17
        <PROBLEM> size align to 32 have an error.
        <SOLVE> size must div to needBlock and mul to byte.
    */
	auto ptr = kmalloc<T>(size);
	for(int i = 0;i < size / sizeof(T);i++){
		*(ptr++) = funcBuild();
	}
	ptr.reset();
	return ptr;
}		

	

void memoryPool::unsafe_unlockInit() {
    initSign = 1;
}

/* ==================== foo ==================== */

foo::foo(int c, int d) : a(c), b(d) {}

void foo::view() {
    std::cout << "a:" << a << " b:" << b << std::endl;
}

template <typename P>
std::ostream& operator<<(std::ostream& in,const agent<P>& out){
	in << P(out);
	return in;
}
	
template <typename P>
std::istream& operator>>(std::istream& in,agent<P>& out){
	P temp;
	in >> temp;
	out = temp;
	return in;	
}

template <typename P>
std::ostream& operator<<(std::ostream& in,const agent<P>&& out){
	in << P(out);
	return in;
}
	
template <typename P>
std::istream& operator>>(std::istream& in,agent<P>&& out){
	P temp;
	in >> temp;
	out = temp;
	return in;	
}
		
//<NOTE> easy iostream operator
//       iostream << agent<Type> To iostream << Type




template <typename T>	 		
agent<T>::operator T() const{
	auto findNeibour = [this](int pos,int size){
		int needBlock = size % 32 ? size / 32 + 1:size / 32;
		int blockPos = 0;	
		for (int i = 1; i <= this->host->metaData->offList[0].first; i++) {
		    uintptr_t ts = this->host->metaData->offList[i].first;
		    uintptr_t ns = this->host->metaData->offList[i + 1].first;
		    if ((ts <= pos) && (ns > pos)) {
		        blockPos = this->host->metaData->offList[i].second;
		        break;
		    }
		}
		if(this->host->metaData->offList[pos / Q_SIZE + needBlock].second > pos + needBlock) return false;
		return true;
		//<NOTE> line search find block.So slowly.
	};
	if(!(Q_SIZE / sizeof(T))
	   &&((position % 32)+ sizeof(T))
	   &&(!(findNeibour(position,sizeof(T))))){
		if(!(std::is_trivially_copyable_v<T>)){
		    void* mem = malloc(sizeof(T));
		    host->bufCopytp(mem,ptr,position);
            T getValue = *(static_cast<T*>(mem));
            std::free(mem);
		    return getValue;
		}else{
			assert(false);
			//<NOTE> fallBack there.Because we will use a agent struct in the future.
			//       This if-block is never ran.
		}
	}else{
		return *(static_cast<T*>(ptr));
	}
}

template <typename T>
agent<T>& agent<T>::operator= (T valueIn){
	auto findNeibour = [this](int pos,int size){
		int needBlock = size % 32 ? size / 32 + 1:size / 32;
		int blockPos = 0;
		for (int i = 1; i <= this->host->metaData->offList[0].first; i++) {
		    uintptr_t ts = this->host->metaData->offList[i].first;
		    uintptr_t ns = this->host->metaData->offList[i + 1].first;
		    if ((ts <= pos) && (ns > pos)) {
		        blockPos = this->host->metaData->offList[i].second;
		        break;
		    }
		}
		if(this->host->metaData->offList[pos / Q_SIZE + needBlock].second > pos + needBlock) return false;
		return true;
	};
	if constexpr(!(std::is_trivially_copyable_v<T>)){
		if(host->metaData->spanBlockSee[host->ptrPos / sizeof(T)]){
			(static_cast<T*>(ptr))->~T();
		}
		host->metaData->spanBlockSee[host->ptrPos / sizeof(T)] = true;
	}
	if(!(Q_SIZE /sizeof(T))
	   &&((position % Q_SIZE)+ sizeof(T))
	   &&(!(findNeibour(position,sizeof(T))))){
	//???????
		void* mem = malloc(sizeof(valueIn));
		//auto valueInPtr = std::make_shared<T>(mem);
		new (mem) T(valueIn);
		host->bufCopytm(ptr,mem,position);
		if constexpr(std::is_object_v<T>){
			((T*)mem)->~T();
		}
        std::free(mem);
		return *this; //so need to add!!!
	}else if(!(std::is_trivially_copyable<T>::value)){
		new (ptr) T(valueIn);
	}else{
		*(T*)ptr = valueIn;

	}
	return *this;
	
	
}

template <typename T>
agent<T>& agent<T>::operator= (agent<T>& valueIn){
	this->operator=(*(static_cast<T*>(valueIn.ptr)));
	return *this;
}


template <typename T>
meta<T>::meta(std::pair<int,int> _seg, std::vector<std::pair<int,int>> _page, void* _startPtr, int _id,memoryPool* _host)
    : seg(_seg), offList(std::move(_page)), shared_count(0), id(_id),host(_host),
      thisBlocklive(true), startPtr(_startPtr), selfCount(0),ver(0) {
		if constexpr(!(std::is_trivially_copyable_v<T>)){
			for(int _ = 0;_ < _seg.second * Q_SIZE / sizeof(T);_++) spanBlockSee.push_back(false);
		}
	  }

//stl allocter
template <typename T>
T* kpoolEx<T>::allocate(int size){
	memoryPool::bPtr bp = host.getBasicPtr(size * sizeof(T));
	T* raw = static_cast<T*>(bp.basicPtr);
	ids[raw] = bp.id;
	return raw;
}

template <typename T>
void kpoolEx<T>::deallocate(T* in,std::uintptr_t n){
	host.kfree(ids[in]);
}

template <typename T>
bool kpoolEx<T>::operator==(const kpoolEx<T>& in) const noexcept { 
	return this->host.idGet() == in.host.idGet();
}
    
template <typename T>
bool kpoolEx<T>::operator!=(const kpoolEx<T>& in) const noexcept {
	return this->host.idGet() != in.host.idGet();
}
						  
template <typename T>
int meta<T>::TLB::push(int chunk){
	if((chunk < 0)||(chunk == P_KPOOL_TLB_STOP_SIGN)) return EINVPR;
	if(size >= P_KPOOL_TLB_SIZE){
		__KLOG_PUT_EX(logger,klog::logLevel::INF,"---fallback to delete chunk---")
		int posDel = deleteChunk();
		*(tlbCore + posDel) = chunk;
		*(useCount + posDel) = 0;
		return EOK;
	}
	*(tlbCore + size) = chunk;
	size++;
	return EOK;
}
						  
template <typename T>
int meta<T>::TLB::deleteChunk(){
	if(size == 0) return EINVPR;
	double average = std::accumulate(useCount,useCount + size,0.0);
	average /= size;
	int pos = (std::find_if(useCount,useCount+ size,[average](int in) -> bool{
		return in < average;
	})) - useCount;
	__KLOG_PUT_EX(logger,klog::logLevel::DBG,"candiate delete chunk=" + std::to_string(pos));
	if(pos == 4) //fallback
		return P_KPOOL_TLB_SIZE - 1;
	
	return pos;
}

template <typename T>
int meta<T>::TLB::pop(){
	return *(tlbCore + size);
}

template <typename T>
int meta<T>::TLB::clear(){
	std::memset(tlbCore,P_KPOOL_TLB_STOP_SIGN,sizeof(int) * P_KPOOL_TLB_SIZE);
	std::memset(useCount,0x00,sizeof(int) * P_KPOOL_TLB_SIZE);
	return EOK;
}
						  
template <typename T>
int meta<T>::TLB::useit(int pos){
	if((pos < 0)||(pos >= size)) return EOCT;
	(*(useCount + pos))++;
	return EOK;
}
						  
template <typename T>
std::pair<int*,int*> meta<T>::TLB::useArea(){
	int get = size % 4 ? size % 4 : 4;
	return std::make_pair(tlbCore,tlbCore + get);
}

/* ==================== ???????? ==================== */

// ---------- ptrm<T> ----------
template class ptrm<int>;
template class ptrm<char>;
template class ptrm<foo>;
template class ptrm<const char*>;
template class ptrm<std::string>;

// ---------- agent<T> ----------
template class agent<int>;
template class agent<char>;
template class agent<foo>;
template class agent<const char*>;
template class agent<std::string>;

// ---------- meta<T> ----------
template class meta<int>;
template class meta<char>;
template class meta<foo>;
template class meta<const char*>;
template class meta<std::string>;

// ---------- memoryPool::kmalloc<T> ----------
template ptrm<int> memoryPool::kmalloc<int>(int);
template ptrm<char> memoryPool::kmalloc<char>(int);
template ptrm<foo> memoryPool::kmalloc<foo>(int);
template ptrm<const char*> memoryPool::kmalloc<const char*>(int);
template ptrm<std::string> memoryPool::kmalloc<std::string>(int);

template class kpoolEx<int>;
template class kpoolEx<char>;
template class kpoolEx<foo>;
template class kpoolEx<const char*>;
template class kpoolEx<std::string>;


// ---------- memoryPool::kcalloc<T> ----------
template ptrm<int> memoryPool::kcalloc<int>(int,int,int);
template ptrm<char> memoryPool::kcalloc<char>(int,int,int);
template ptrm<foo> memoryPool::kcalloc<foo>(int,int,int);
//template ptrm<foo2> memoryPool::kcalloc<foo2>(int,int);
template ptrm<const char*> memoryPool::kcalloc<const char*>(int,int,int);
template ptrm<std::string> memoryPool::kcalloc<std::string>(int, int,int);

/* ---------- memoryPool::krealloc<T> ----------
template ptrm<int> memoryPool::krealloc<int>(ptrm<int>, uintptr_t);
template ptrm<char> memoryPool::krealloc<char>(ptrm<char>, uintptr_t);
template ptrm<foo> memoryPool::krealloc<foo>(ptrm<foo>, uintptr_t);
//template ptrm<foo2> memoryPool::krealloc<foo2>(ptrm<foo2>, uintptr_t);
template ptrm<const char*> memoryPool::krealloc<const char*>(ptrm<const char*>, uintptr_t);
template ptrm<std::string> memoryPool::krealloc<std::string>(ptrm<std::string>, uintptr_t);
*/

// ---------- operator<<(ostream&, agent<T> const&) ----------
template std::ostream& operator<< <int>(std::ostream&, agent<int> const&);
//template std::ostream& operator<< <char>(std::ostream&, agent<char> const&);
//template std::ostream& operator<< <foo>(std::ostream&, agent<foo> const&);
//template std::ostream& operator<< <foo2>(std::ostream&, agent<foo2> const&);
//template std::ostream& operator<< <const char*>(std::ostream&, agent<const char*> const&);
template std::ostream& operator<< <std::string>(std::ostream&, agent<std::string> const&);

// ---------- operator<<(ostream&, agent<T> const&&) ----------
template std::ostream& operator<< <int>(std::ostream&, agent<int> const&&);
//template std::ostream& operator<< <char>(std::ostream&, agent<char> const&&);
//template std::ostream& operator<< <foo>(std::ostream&, agent<foo> const&&);
//template std::ostream& operator<< <const char*>(std::ostream&, agent<const char*> const&&);
template std::ostream& operator<< <std::string>(std::ostream&, agent<std::string> const&&);

// ---------- operator>>(istream&, agent<T>&&) ----------
template std::istream& operator>> <int>(std::istream&, agent<int>&&);
//template std::istream& operator>> <char>(std::istream&, agent<char>&&);
//template std::istream& operator>> <foo>(std::istream&, agent<foo>&&);
//template std::istream& operator>> <const char*>(std::istream&, agent<const char*>&&);
template std::istream& operator>> <std::string>(std::istream&, agent<std::string>&&);

template <>
uintptr_t meta<int>::updateOffset = 0;
template <>
uintptr_t meta<char>::updateOffset = 0;
template <>
uintptr_t meta<foo>::updateOffset = 0;
template <>
uintptr_t meta<const char*>::updateOffset = 0;
template <>
uintptr_t meta<std::string>::updateOffset = 0;

// ---------- memoryPool::ksalloc<T> ----------
template ptrm<int>          memoryPool::ksalloc<int>         (int, std::function<int()>);
template ptrm<char>         memoryPool::ksalloc<char>        (int, std::function<char()>);
template ptrm<foo>          memoryPool::ksalloc<foo>         (int, std::function<foo()>);
template ptrm<const char*>  memoryPool::ksalloc<const char*>(int, std::function<const char*()>);
template ptrm<std::string>  memoryPool::ksalloc<std::string> (int, std::function<std::string()>);
