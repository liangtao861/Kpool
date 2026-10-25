#ifndef KPOOL_H_
#define KPOOL_H_

/*

  /\___/\
 (  oWo  )
 /       \
 \  |||  /
  \/\/\/

KPOOL

*/

#if (defined(THROW))
    #error "FATAL:re-define THROW marco."
#endif

//#define P_KLOG_LOG_OUT

//#include "except.h"
//#include "klog.h"

#include <cstring>
#include <unordered_set>
#include <iostream>
#include <map>
#include <type_traits>
#include <cstdlib>
#include <utility>
#include <vector>
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <string>
#include <stdexcept>
#include <new>
#include <cassert>
#include <climits>
#include <functional>
#include <any>
#include <stack>

#include "klog.h"

#define Q_SIZE 32
#define C_SIZE 4
#define ENABLE_MD false

//Public marcos
#define THROW(name,...) \
{\
    P_KLOG_PUT(logger,klog::logLevel::ERR,"Have a error:" #name " " #__VA_ARGS__) \
    throw name(__VA_ARGS__); \
}

//<NOTE> marco checker
#if Q_SIZE % 4
    #error "FATAL:Q_SIZE is not align."
#endif
//<DO> if Q_SIZE is not align to 4 bytes.In ARM ,it is not align read.
//     will cpu protection error

#if __cplusplus < 201703L
    #error "FATAL:This library need C++ 17 standard."
#endif
//<DO> this library need C++17


//<DO> this library use plenty of __VA_ARGS__

#if (defined(public))||(defined(private))||(defined(protected))
    #error "FATAL:Invalid defines."
#endif
//<DO> avoid class protection crash

#if ((defined(__throw))||(defined(_THROW))||(defined(__THROW)))
    #warning "WARNING:do not define them."
#endif

//----MAIN PROGRAM-----//
typedef char* byte;
typedef void* NON_POD_HANDLE;
P_KLOG_SELF(logger,"./log.txt");
	
//<NOTE> Do not call them in public namespace.
#define __KPOOL_CAST_HANDLE(name,pointer,...) __VA_ARGS__ name = *(static_cast<__VA_ARGS__*>(pointer));
#define __KPOOL_MALLOC_HANDLE(name,...)       void* name = std::malloc(sizeof(__VA_ARGS__));
#define __KPOPL_FREE_HANDLE(name)             std::free(name);
#define __KPOOL_GET_HANDLE_ADR(result,name)   uintptr_t result = (uintptr_t)name;
#define __KPOOL_BUILD_HANDLE_SELF(name,...)   new (name) __VA_ARGS__();
#define __KPOOL_DELETE_HANDLE_SELF(name,...)  static_cast<__VA_ARGS__*>(name)->~__VA_ARGS__();
#define __KPOOL_LIST_IN(name,handle)          name.push_back((uintptr_t)handle);

template <typename T>
class metaSignUp;

class memoryPool;

class poolREG{
	enum errnoReg{
		ENNOT = 6     //Invalid Data
		,EOK  = 4     //???
		,ERRFOD = 3   //Can't found/Invalid par
		,ETRUE  = 2   //????????
		,EFALSE = 0
		,EALC   = 1   //alloc failed(need to clear)
	};
	inline static int& errnoGet(){
		static int err;
		P_KLOG_PUT(logger,klog::logLevel::INF,"errnoGet:" + std::to_string(err));
		return err;
	}
	inline static std::vector<bool>& poolReg(){
		static std::vector<bool> reg;
		return reg;
	}
	inline static int crtNewPool(){
		errnoGet() = EOK;
		try{
		    poolReg().push_back(false);
		}catch(std::bad_alloc){
			P_KLOG_PUT(logger,klog::logLevel::ERR,"poolReg() bad_alloc.")
			errnoGet() = EALC;
			return EALC;
		}
		return EOK;
	}
	inline static int getStus(int id){
		P_KLOG_PUT(logger,klog::logLevel::INF,"request=" + std::to_string(id) + " id need.")
		if(errnoGet() == EALC) return EALC;
		if((id < 0)||(id >= poolReg().size())) return ENNOT;
		return poolReg()[id] ? ETRUE : EFALSE;
	}
	inline static bool getStu(int id){
		P_KLOG_PUT(logger,klog::logLevel::INF,"Basic request=" + std::to_string(id) + "id need.")
		return poolReg()[id];
	}
	inline static int eraPool(int id){
		P_KLOG_PUT(logger,klog::logLevel::INF,"Era Pool" + std::to_string(id))
        if((id < 0)||(id >= poolReg().size())) return ENNOT;
		poolReg()[id] = false;
		return EOK;
	}
	inline static int stuChg(int id,errnoReg stu){
		P_KLOG_PUT(logger,klog::logLevel::DBG,"stuats change" 
				   + std::to_string(id) + std::to_string(poolReg().size()));
		if(errnoGet() == EALC) return EALC;
		if((id < 0)||(id >= poolReg().size())) return ENNOT;
		if((stu != ETRUE)&&(stu != EFALSE)) return ERRFOD;
		poolReg()[id] = (stu == ETRUE ? true : false);
		return EOK;
	}
	//<NOTE> hack.because of the poolID is add 1 easily.So there
	//       use tuple pos to get sign.
	inline static int clear(){
		errnoGet() = EOK;
		return EOK;
	}
	poolREG(){
		poolReg();
	}
	template <typename T>
	friend class metaSignUp;
	friend class memoryPool;
};

template <typename T>
class kpoolEx;

class __tools{
	__tools() = delete;
	__tools(__tools&& in) = delete;
	__tools(__tools& in) = delete;
	~__tools() = delete;
	int seek_line(std::vector<std::pair<int,int>> in,int voff);
	int two_branch_seek(std::vector<std::pair<int,int>> in,int voff);
	int to_basic(int inBlk,int BlkN,int voff);
	template <typename T>
	friend class ptrm;
};
template <typename T>
class ptrm;

template <typename T>
class agent;

class memoryPool;


template <typename T>
class meta{
private:
	enum errnoMeta{
		EINVPR,EOK
	};
	#define P_KPOOL_TLB_SIZE       4
	#define P_KPOOL_TLB_STOP_SIGN -1
	//<WHY> I select 4 to be TLB_SIZE.
	//      because 4 items in the stack is not a big tuple.
	//      and it will make address translate faster.
	class TLB{
	private:

		int  deleteChunk();
		int  size;
		//<DO> return a pos of delete chunk
	public:
		enum errnoTLB{
			EOCT,EOK,EINVPR
		};
		int tlbCore [P_KPOOL_TLB_SIZE];
		int useCount[P_KPOOL_TLB_SIZE];
		
		int pop();
		int push(int chunk);
		int clear();
	    int useit(int pos);
	    //<DO> it will set all tuple to STOP_SIGN.
	    //     default is -1.
		std::pair<int*,int*> useArea();
		inline TLB(){
			std::memset(tlbCore,P_KPOOL_TLB_STOP_SIGN,P_KPOOL_TLB_SIZE * sizeof(int));
			std::memset(useCount,0x00,P_KPOOL_TLB_SIZE * sizeof(int));
			size = 0;
			//<DO> set Size to 0.Clear tlbCore with STOP_SIGN
			//     if don't do it.tuple and size are rubbish.
		}
	};
		
	std::pair<int,int> seg;
	std::vector<std::pair<int,int>> offList;
	int shared_count;
	int id;
	bool thisBlocklive;
	void* startPtr;                         //??????
	static uintptr_t updateOffset; 
	std::vector<bool> spanBlockSee;
	int selfCount;
	memoryPool* host;
	int ver;
	TLB tlb;
	meta(std::pair<int,int> _seg,std::vector<std::pair<int,int>> _page,void* startPtr,int _id,memoryPool* _host);
	inline meta(meta&& in)
		:seg(in.seg),offList(std::move(in.offList)),shared_count(in.shared_count),
	     id(in.id),selfCount(in.selfCount),startPtr(in.startPtr),thisBlocklive(in.thisBlocklive),
	     host(in.host),ver(in.ver),tlb(TLB()){
		 	if constexpr(!(std::is_trivially_copyable_v<T>)){
				P_KLOG_PUT(logger,klog::logLevel::INF,"Non-POD type self build.");
				spanBlockSee.resize(seg.second * Q_SIZE / sizeof(T));
			}
			//<NOTE> the constexpr to the spanBlockSee.
			//       POD type don't need self span.Only Non-POD need
			//       It will reduce running speed.  
		 }
    meta(meta& in) = default;
	meta() = default; 
protected:
	inline meta* getMetaPtr() {return this;}
	inline void scAdd() {shared_count++;}
	inline int scSub() {
		if(shared_count == 0) return EINVPR;
		P_KLOG_PUT(logger,klog::logLevel::INF,"shared_count--");
		shared_count--;
		return EOK;
	}
	inline void scReset() {shared_count = 0;}
	inline void selfAdd() {selfCount++;}
	inline int getMetaID(){return id;}
	inline void updateOfflist(std::vector<std::pair<int,int>> in) {offList = in;}
	inline int segAdd(uintptr_t sadd) {
		seg.second += sadd;
		return EOK;
	}
	inline int segSub(uintptr_t ssub) {
		if((ssub == SIZE_MAX)||(seg.second - ssub == SIZE_MAX)){
			P_KLOG_PUT(logger,klog::logLevel::ERR,"seg is negative now");
			return EINVPR;
		}
		//<NOTE> It can't be nagetive.It will be the biggest value in the world.
		seg.second -= ssub;
		P_KLOG_PUT(logger,klog::logLevel::INF,"seg.first=" + std::to_string(seg.first) +
				                              "seg.second="+std::to_string(seg.second)
				   )
		return EOK;
	}
	inline operator bool() {return shared_count;}
	inline int addPtrOff(int off) {
		if(off == INT_MAX) return EINVPR;
		updateOffset += off;
		return EOK;
	}
	friend class ptrm<T>;
	friend class agent<T>;
	friend class memoryPool;
	friend class metaSignUp<T>;
};

template <typename T>
class metaSignUp;

class memoryPool;

class metaReg{
	inline static std::unordered_multimap<int,std::pair<int,std::pair<void*,void(*)(void*)>>>& ptrtable(){
		static std::unordered_multimap<int,std::pair<int,std::pair<void*,void(*)(void*)>>> table1;
		return table1;
	}
	inline static std::unordered_set<void*>& bufManager(){
		static std::unordered_set<void*> bufManager1;
		return bufManager1;
	}
	metaReg() = default;
	template <typename T>
	friend class metaSignUp;
};



template <typename T>
class metaSignUp{
	inline static void destory(void* in){
		(static_cast<meta<T>*>(in))->~meta<T>();
		std::free(in);	
	}
	//inline static void bufManagerIn(void* in){
		
	inline std::unordered_multimap<int,std::pair<int,std::pair<void*,void(*)(void*)>>>& ptrtable(){
		auto&& reg = metaReg();
		return reg.ptrtable();
	}
	inline static std::unordered_set<void*>& buf(){
		return metaReg::bufManager();
	}
	
	inline static void bufCrt(void* in){
		buf().insert(in);
	}
	inline static bool bufUse(void* in){
		return buf().find(in) == buf().end() ? false : true;
	}
	inline static void bufEra(void* in){
		buf().erase(in);
	}
	inline metaSignUp(int blkID,int poolID,meta<T>&& signUp) {
		void* tmp = malloc(sizeof(meta<T>));
		if(!tmp) {THROW(std::runtime_error,"create Meta Failed.")}
		new (tmp) meta<T>(signUp);
		P_KLOG_PUT(logger,klog::logLevel::INF,"blkID=" + std::to_string(blkID) + " poolID=" + std::to_string(poolID))
		ptrtable().insert(std::pair<int,std::pair<int,std::pair<void*,void(*)(void*)>>>
			(poolID,std::make_pair(blkID,std::make_pair(tmp,destory))));
		//<DO> insert poolID,blockID,pointer to meta,and callback deleter.
	}
	inline metaSignUp() = default;
	inline void metaRemove(int poolID){
		auto&& rg = ptrtable().equal_range(poolID);
		for(auto&& c = rg.first;c != rg.second;c++){
			//if(c->first == poolID) {
				(c->second.second.second)(c->second.second.first);
			//<DO>   a fake condition.Compiler will delete it.
			//       and it will find block in poolID and erase them step
			//       by setp.Time:O(n)
		}
		P_KLOG_PUT(logger,klog::logLevel::INF,"erase Pool.poolID="+std::to_string(poolID)
				   +"blkCnt="+std::to_string(std::distance(rg.first,rg.second)))
		ptrtable().erase(poolID);
	}
	inline void ptrRemove(int poolID,int BlkID){
		auto&& rg = ptrtable().equal_range(poolID);
		P_KLOG_PUT(logger,klog::logLevel::INF,"erase pointer meta.poolID="
				   +std::to_string(poolID)+" blkID="+std::to_string(BlkID))
		for(auto&& c = rg.first;c != rg.second;c++){
			if(c->second.first == BlkID) {
				(c->second.second.second)(c->second.second.first);
				ptrtable().erase(c);
                break;
			}
		}
		//<DO>:This function is O(n-m) the worst situation:O(n)
	}
	inline meta<T>* getMeta(int poolID,int BlkID){
		auto&& rg = ptrtable().equal_range(poolID);
		P_KLOG_PUT(logger,klog::logLevel::INF,"getMeta.poolID=" + std::to_string(poolID)
				   +"BlkID="+std::to_string(BlkID))
		for(auto&& c = rg.first;c != rg.second;c++){
			if(c->second.first == BlkID) return static_cast<meta<T>*>(c->second.second.first);
		}
		P_KLOG_PUT(logger,klog::logLevel::WAR,"meta Not found.Fallback to nullptr")
		return nullptr;
		//<NOTE> like up.When not found return nullptr.Segmentation fault.
		//<PSC>  cdict ptrtable
		//       reault = ptrtable.find(id)
		//       for i in result:
		//           if(c.key.first == BlkID): #find pointer slot == BLKID
		//               return meta(c.key.second.first) #return the pointer's meta
		//<PAR>  int -> your pool's ID,int -> BlkID
		//<DO>   return nullptr(fallback) or meta pointer
	}   
	friend class memoryPool;
	template <typename M>
	friend class ptrm;
};
   

template <typename T>
class ptrm {
private:
	enum errnoPtrm{
		EOK = 0,
		EPTRIN = 1,
		EOTRG = 2,
		EPTRD = 3,
		ENOF = 4
	};
	meta<T>* metaData; //control block
	bool lock;
	int ptrPos;
	int localVer;
	int localId;
	mutable int nowBlock; //<NOTE> TLB(Is planning)
	//<NOTE> if no -mutable.Compile failed.

    inline uintptr_t offset(uintptr_t get);
    inline uintptr_t abs(uintptr_t num);

    ptrm(std::pair<int, int> _seg, void* _bp, int _id, void* _sp = nullptr) = delete;
    ptrm(meta<T>* _md,int _id,int _lV = 0,int _ppos = 0); 

    typedef const void* outPtr;

    uintptr_t offsetGet(uintptr_t voff) const noexcept;
    int bufCopytm(void* dest,void* src,uintptr_t startPos);
    int bufCopytp(void* dest,void* src,uintptr_t startPos);
	//<DO>  this function is copy a buffer to pointer 
	//<PAR> dest -> copy from.src -> copy to.startPos -> startPtr.
	int urandomFill(int startPos,int endPos);
	//<DO> fill a memory block with random number.
	//     This function create for building safety
	//     DO NOT CALL IT WHEN NON-POD NOT DELETE.
	void destory();
	//<DO> destory this pointer and make the block to
	//     urandom.
    friend class memoryPool;
    friend class agent<T>;
    friend class kpoolEx<T>;
public:
	template <typename P>
	class proxyTmp{
	private:
		int startPos;
		P* ptr;
		bool timePool;
		ptrm* host;
		void* basicPtr;
		inline proxyTmp(P* _p,bool _tP,ptrm<P>* _hst,int pos = 0,void* _bp = nullptr) 
			: ptr(_p),timePool(_tP),host(_hst),basicPtr(_bp),startPos(pos) {}
    public:
    	inline ~proxyTmp(){
			if(timePool){
				host->bufCopytm(basicPtr,ptr,startPos);
				free(ptr);
			}
		}
		proxyTmp(const proxyTmp&) = delete;
        proxyTmp& operator=(const proxyTmp&) = delete;
		inline operator P(){
			return *ptr;
		}
		inline P* operator->() { return ptr; }
		inline P& operator*() { return *ptr; }
		//inline operator P() { return *ptr; }
		friend class agent<P>;
		friend class ptrm<P>;
	};
	ptrm(const ptrm& in);
	~ptrm();
	
    outPtr get(uintptr_t offset);
    agent<T> operator*();
    uintptr_t id() const noexcept;
    int blkID();
    int getCount();

    ptrm<T> operator+(int madd);
    ptrm<T> operator-(int msub);
	ptrdiff_t operator-(ptrm<T>& msub) const noexcept;
    ptrm<T>& operator++();
    ptrm<T>& operator--();
	ptrm<T>  operator++(int);
	ptrm<T>  operator--(int);
    ptrm<T>& operator+=(uintptr_t madd);
    ptrm<T>& operator-=(uintptr_t msub);
    bool operator==(ptrm<T>& other) const noexcept;
    bool operator!=(ptrm<T>& other) const noexcept;
    bool operator<(ptrm<T>& other) const noexcept;
    bool operator>(ptrm<T>& other) const noexcept;
	bool operator>=(ptrm<T>& other) const noexcept;
	bool operator<=(ptrm<T>& other) const noexcept;
	bool operator==(ptrm<T>&& other) const noexcept;
	bool operator!=(ptrm<T>&& other) const noexcept;
	bool operator<(ptrm<T>&& other) const noexcept;
	bool operator>(ptrm<T>&& other) const noexcept;
	bool operator>=(ptrm<T>&& other) const noexcept;
	bool operator<=(ptrm<T>&& other) const noexcept;
    bool operator==(void* other) const noexcept;
    bool operator!=(void* other) const noexcept;
    bool operator<(void* other) const noexcept;
    bool operator>(void* other) const noexcept;
	bool operator>=(void* other) const noexcept;
	bool operator<=(void* other) const noexcept;
	inline ptrm<T>& operator=(std::nullptr_t _) {
		if(!lock){
		    lock = true;
		    metaData->scSub();
		    if(!(*metaData)) destory();
			//<NOTE> Do not EDIT.destory() will make fault
			//       not errCode.So I want to stop it.
			//       noexcept will make this program crash
		    return *this;
		}
		//<NOTE> when ptrm is locked.It can't lock again
		//       this control will fail.
		return *this;
	}
	inline bool operator!=(std::nullptr_t _) const noexcept{
		return !lock;
	}
	inline bool operator==(std::nullptr_t _) const noexcept{
		return lock;
	}
	ptrm<T>& operator=(ptrm<T>& in);
	ptrm<T>& operator=(ptrm<T>&& in);
	template <typename U>
	friend bool operator== (void* s1,ptrm<U>& s2) noexcept ;
	template <typename U>
	friend bool operator!= (void* s1,ptrm<U>& s2) noexcept;	
	template <typename U>
	friend bool operator< (void* s1,ptrm<U>& s2) noexcept;
	template <typename U>
	friend bool operator> (void* s1,ptrm<U>& s2) noexcept;
	template <typename U>
	friend bool operator>= (void* s1,ptrm<U>& s2) noexcept;
	template <typename U>
	friend bool operator<= (void* s1,ptrm<U>& s2) noexcept;
	template <typename U>
	friend bool operator== (void* s1,ptrm<U>&& s2) noexcept;
	template <typename U>
	friend bool operator!= (void* s1,ptrm<U>&& s2) noexcept;	
	template <typename U>
	friend bool operator< (void* s1,ptrm<U>&& s2) noexcept;
	template <typename U>
	friend bool operator> (void* s1,ptrm<U>&& s2)noexcept;
	template <typename U>
	friend bool operator>= (void* s1,ptrm<U>&& s2) noexcept;
	template <typename U>
	friend bool operator<= (void* s1,ptrm<U>&& s2) noexcept;
    ptrm::proxyTmp<T> operator->();
    explicit operator int();
    operator bool();
    void reset();
    agent<T> operator[](int madd);
};

template <typename T>
class agent{
private:
	ptrm<T>* host;
	void* ptr;
	int position;
	agent(ptrm<T>* _h,void* _P,int _p): host(_h),ptr(_P),position(_p) {}
public:
	operator T() const;
	agent<T>& operator =(T valueIn);
	agent<T>& operator= (agent<T>& valueIn);
	void* operator&();
	friend class ptrm<T>;
	template <typename P>
	friend std::ostream& operator<<(std::ostream& in,const agent<P>& out);
	template <typename P>
	friend std::istream& operator>>(std::istream& in,agent<P>& out);
	template <typename P>
	friend std::ostream& operator<<(std::ostream& in,const agent<P>&& out);
	template <typename P>
	friend std::istream& operator>>(std::istream& in,agent<P>&& out);
};

class memoryPool {
private:
	struct bPtr {
		void* basicPtr;
		std::multimap<int,std::pair<int,int>> map;
		int needBlock;
		int id;
		int sign;
	};
	enum errPool{
		EOK = 0,
		EALC = 1,
		EINV = 2,
		EINIT = 3		
	};
	class segManager{
	private:
		std::vector<std::pair<int,int>> segTuple;
	public:
		enum errSeg{
			EOK = 0,
			EC = 1,
			ENOF = -2,
			EOCT = -4
		};
		segManager(int segStart,int segEnd);
		int                cut_chunk(int segStart,int size);
		int   /* FUMI */   merge_chunk(int position);
		//<PAR> position -> blocks.blk position.Return a stuats code.
		int                need_chunk_seg(int blocks);
		inline std::pair<int,int> get_addr_pair(int position){
			if(pos >= segTuple.size())
				return std::make_pair(EOCT,EOCT);
			else
				return segTuple[pos];
		}
	}	
	std::vector<std::pair<int,int>> build(const std::multimap<int, std::pair<int, int>>& blocks);
    bPtr getBasicPtr(int size,int id = -1);
    inline int getID();
	inline int getPoolID();
    void eraseFree(int id);
    void signFreeList();
    int needFreeBlk(int blkS);

    void* ptr;
    void* endPtr;
    std::unordered_multimap<int, std::pair<int, int>> allBlock;
    std::unordered_multimap<int, std::pair<int, int>> useBlock;
    int id;
	int localID;
	static int poolID;               //poolID
	static metaSignUp<int>* manager; //manager in int
    int count;
    uintptr_t totalSize;
    bool initSign;
	bool notallowed;
	std::stack<std::pair<int,std::pair<int,int>>> freeQueue;
    std::multimap<int, std::pair<int, int>> freeBlock;
    
    template <typename T>
    friend class ptrm;
    template <typename T>
	friend class kpoolEx;

public:
    memoryPool();
    ~memoryPool();

	//<DO>      basic init.
	//<WARNING> 1.if you don't init this pool.It is a
	//          error.
	//          2.the buffer must satisfy align of 32.The
	//          address should be align of 16
	//          3.Do not size > in.It is vert IVALID.It not a 
	//          pool.It's a UB machine.
	//<TIP>     Please use safe version.If you don't know you
	//          are doing what.
	//<ERR>     std::bad_alloc :: when malloc return errorCode
	//          std::logic_error :: re-init or use a chunk have used
    int init(int size = 2048, void* in = nullptr);
	
	//<DO>      safe Init
	//<PAR>     N -> size1,size -> needSize,buf -> buffer(char)
	//<ERR>     static_assert failed :: buffer is not enough
	template <int N>
	inline int sinit(int size,char buf[N]){
		static_assert(N >= size,"FATAL:memory not enough");
		return init(size,(void*)buf);
	}
	
    void* getPtr();
	
    void kfree(int id);
	
	//<DO>  get valid ID.Only it.
    inline int idGet() const{
		return localID;
	}

	//<DO>  malloc a basic virtual address space.
	//      It don't help you do any thing.
	//<PAR> size -> byte (get a size memory)
	//<ERR> std::bad_alloc / std!:invalid_argument :: pool memory not enough
    template <typename T>
    ptrm<T> kmalloc(int size);

	//<DO>  This func will calloc your memory space
	//<PAR> item -> int,itemSize -> int,totalSize -> item * itemSize
	//      filled -> int = 0x00 This function will use it fill your
	//      memory
    template <typename T>
    ptrm<T> kcalloc(int item, int itemSize,int filled = 0x00);

	//<NOTE> I don't know why need use any there,I only want user fill a template
	//       parameter.But I change it to template now.I want to change it to a [PTRM'S STU]
	//       OK.use std::any to return is pythonic.When a alloc function return int,string
	//       char*,ptrm<T>,agent,and even Lv Xiao Jun's photo(??? I don't know WHY I WRITE THIS.By Alien)
	//       ,it is not a function,but a disaster.SNIAFE can't avoid it
	//       C++ 's template can auto type.So I still use krealloc in template
	//       LOVE DON'T FROM C
	//<PAR>  ptrm<T> oldPtr - the Ptr you want to realloc.int newSize - size.
	//<DO>   realloc your memory.When size < newSize.OffList resize and change,O(n - m)
    //       when size > newSize erase offList O(m) When = O(1) When 0 free the memory and 
	//       return a pointer have be locked
	//<ERR>  like up.And static_assert failed.
	template <typename T>
    ptrm<T> krealloc(ptrm<T> oldPtr, uintptr_t newSize);
	
	//<DO>   For non-POD.If your type is POD.Don't use it
	//<PAR>  size -> self size,funcBuild -> factor function
	//<NOTE> Do not use kcalloc and std::string int none string
	//       It is a zero-byte rubbish.Please use ksalloc.You even
	//       can init Miku Hatsune.
	//<TODO> We need a parameter version ksalloc.
	template <typename T>
	ptrm<T> ksalloc(int size,std::function<T()> funcBuild = 
		[]() -> T{
			static_assert(std::is_default_constructible_v<T>,"FATAL:Please provied a class with a default creator.");
			return T();
			//<NOTE> for avoiding self-alloc with Non-default creator.
			//       But compiler's imformatipn can't prepare for mankind to read.
			//       So I use static_assert to refuse error when complie.
			//<PAR>  int size -> allocSize,funcBuild -> creator.
			//<TODO> no thing need to do.OK.
		});

    [[deprecated]] void unsafe_unlockInit();
	//<WRANING> This is the most dangerous function in
	//          this library.It change the pool's stuats
    
};

class foo {
public:
    int a;
    int b;
    foo(int c, int d);
    void view();
};

template <typename T>
class kpoolEx{
private:
	memoryPool host;
	std::unordered_map<T*,int> ids;
public:
	/*
		BUG REPORT #7
		<PROBLEM> double free
		<SOLVE>   deepCopy kpoolEx STL allocter
		<DECIDE>  Not fix
	*/
	using value_type = T;
	kpoolEx(int size = 2048) {
		host = memoryPool();
		host.init(size);
		//<DO> RAII.init the memoryPool first
		//<PAR>int -> size.
	}
	T* allocate(int size);
	void deallocate(T* in,std::uintptr_t n);
	
	bool operator==(const kpoolEx<T>& in) const noexcept;
	bool operator!=(const kpoolEx<T>& in) const noexcept;
};

#endif
