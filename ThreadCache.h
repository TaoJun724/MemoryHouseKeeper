#ifndef  __THREAD_CACHE_H__
#define  __THREAD_CAHCE_H__


#include"Common.h"

class ThreadCache
{
public:
	//分配内存
	void* Allocate(size_t size);
	
	//释放内存
	void Deallocate(void* ptr, size_t size);

	// 从中心缓存获取对象
	void* FetchFromCentralCache(size_t index, size_t size);
	
	// 链表中对象太多，开始回收。
	void ListTooLong(FreeList* freelist, size_t byte);
private:
	//创建一个自由链表数据，长度NLISTS是240，长度是根据对齐规则得来的
	FreeList _freelist[NLISTS];
};

//怎么让每个线程都有自己的一个线程缓存
//方法一：
//使用ThreadCache* list;全局变量将所有的ThreadCache对象链接起来。
//每个对象存tid（int _tid）及下一对象的指针（ThreadCache* _next）。
//但获取ThreadCache对象需要加锁。

//方法二：
//使用TLS技术(Thread Local Storage)线程本地存储 
//定义了一个静态的TLS变量，这个变量每个线程都有一个
static _declspec(thread) ThreadCache* tls_threadcache = nullptr;


#endif
