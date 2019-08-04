//CentralCache层只有一个，它是一个中心缓存，进行资源的均衡，当ThreadCache没有时可以向CentralCache申请，并且对于ThreadCache的某个资源过剩的时候，可以回收ThreadCache内部的的内存






#ifndef __CENTRAL_CACHE_H__
#define __CENTRAL_CACHE_H__


#include "Common.h"


//使用饿汉模式
class CentralCache
{
public:
	static CentralCache* GetInstance(){
		return &_inst;
	}

	Span* GetOneSpan(SpanList* spanlist, size_t bytes);

	// 从中心缓存获取一定数量的对象给thread cache
	size_t FetchRangeObj(void*& start, void*& end, size_t num, size_t byte);

	// 将一定数量的对象释放到span跨度
	void ReleaseListToSpans(void* start, size_t byte_size);
private:
	// 中心缓存自由链表
	SpanList _spanlist[NLISTS];
private:
	CentralCache() = default;//default可以生成默认的
	CentralCache(const CentralCache&) = delete;//delect不会生成
	CentralCache& operator=(const CentralCache&) = delete;

	static CentralCache _inst;
};


#endif
