#ifdef _WIN32
    #define NOMINMAX      
    #define WIN32_LEAN_AND_MEAN 
#endif
//<TIP> This is a C library to LD_LIBRARY
//#include "CAlloc.h"
#include <stdio.h>

#ifdef _WIN32
    #include <windows.h>
#endif

#ifdef __linux__
    #include <sys/mman.h>
    #include <unistd.h>
#endif 

#ifdef _WIN32
    #undef far
    #undef near
    #undef BOOL
    #undef BYTE
#endif

#define Q_SIZE 32
#define BOOL char
#define TRUE 1
#define FALSE 0

#if Q_SIZE % 4
    #error "Q_SIZE must be align of 4."
#endif

static BOOL haveInit = FALSE;
static void* ptr = NULL;
static int id = 0;

int getID(){
	return (++id);
}

typedef struct{
    void* ptr;
	void* end;
	int   id;
}bPtrBoot;


#ifdef _WIN32
void* M_Alloc(size_t size){
	return virtualAlloc(
		NULL,
		size * 4096,
		MEM_LARGE_PAGES|MEM_COMMIT|MEM_RESERVE,
		PAGE_READWRITE
	);
}

int M_FreeW(void* in,size_t size){
	return virtualFree(in, size, MEM_RELEASE);
}

//int M_windowsRealloc(size_t size){
#endif

#ifdef __linux__
void* M_Alloc(size_t size){
	return mmap(NULL,size * 4096,
				PROT_READ|PROT_WRITE,
				MAP_PRIVATE| MAP_ANONYMOUS,-1,0);
	//MAGIC NUMBER!!! Love from 1970
}

int M_FreeL(void* in,size_t size){
	return munmap(in,size);
}
		

#endif

#ifdef _WIN32
    #define M_Free(ptr,size) M_FreeW(ptr,0)
#endif

#ifdef __linux__
    #define M_Free(ptr,size) M_FreeL(ptr,size)
#endif
		
//#define SAFE_USE(...) if (__VA_ARGS__ == NULL) return ERT;
	
enum errnoKpool{
	EOK,
	EINVPR,
	EINIT,
	ERT
};

bPtrBoot m_malloc(int size){
	if(!haveInit){
		ptr = M_Alloc(32);
		if(ptr == NULL) return (bPtrBoot){.ptr = NULL,.end = NULL,.id = -ERT};
		haveInit = TRUE;
	}
	if(size < 0){
		bPtrBoot new = (bPtrBoot){.ptr = NULL,.end = NULL,.id = -EINVPR};
		return new;
	}
	size_t size1 = size;
	ptr = (void*)((char*)ptr + size1);
	return (bPtrBoot){.ptr = (void*)((char*)ptr - size1),.end = ptr,.id = getID()};
}
		
void free(void* ptr){
    ;
}
//<DO> no-op .Because it is a bump allocter in 
//     booting.Can't freed.
		
void* malloc(long unsigned int size){
	return m_malloc(size).ptr;
}

int main(){
	int* ptr = (int*)malloc(36);
	*ptr = 3;
	if(ptr == NULL) return -1;
	printf("ppppp");
	return 0;
}




	
	