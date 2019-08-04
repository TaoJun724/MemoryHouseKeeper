#include "CentralCache.h"
#include "PageCache.h"

CentralCache CentralCache::_inst;

//// 打桩测试
//// 从中心缓存获取一定数量的对象给thread cache
//size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t n, size_t bytes)
//{
//	start = malloc(bytes * n);
//	end = (char*)start + bytes*(n-1);
//
//	void* cur = start;
//	while (cur <= end)
//	{
//		void* next = (char*)cur + bytes;
//		NEXT_OBJ(cur) = next;
//		
//		cur = next;
//	}
//
//	NEXT_OBJ(end) = nullptr;
//
//	return n;
//}


// 当Central Cache中存在span的时候，直接返回span,否则要到PageCache中获取span
Span* CentralCache::GetOneSpan(SpanList* spanlist, size_t bytes)
{
	//CentralCache中存在span
	Span* span = spanlist->begin();
	while (span != spanlist->end())
	{
		if (span->_objlist != nullptr)
			return span;

		span = span->_next;
	}

	// CetralCache中无span,向pagecache申请一个新的合适大小的span,计算页号，新的span有npage页
	size_t npage = ClassSize::NumMovePage(bytes);
	Span* newspan = PageCache::GetInstance()->NewSpan(npage);

	// 获取到了newSpan之后，这个newSpan是npage数量的页
	// 需要将这个span分割成一个个的bytes大小的内存块连接起来
	// 地址设置为char*是为了一会儿可以方便的挂链，每次加数字的时候，可以直接移动这么多的字节
	char* start = (char*)(newspan->_pageid << PAGE_SHIFT);
	//end为最后一个字节的下一地址
	char* end = start + (newspan->_npage << PAGE_SHIFT);
	
	char* cur = start;
	char* next = cur + bytes;
	while (next < end)
	{
		NEXT_OBJ(cur) = next;
		cur = next;
		next = cur + bytes;
	}
	NEXT_OBJ(cur) = nullptr;
	newspan->_objlist = start;
	newspan->_objsize = bytes;
	newspan->_usecount = 0;

	// 将newspan插入到spanlist
	spanlist->PushFront(newspan);
	return newspan;
}

// 从中心缓存获取一定数量的对象给threadcache
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t num, size_t bytes)
{
	//求出在span链表数组中的下标
	size_t index = ClassSize::Index(bytes);
	SpanList* spanlist = &_spanlist[index];

	// 对当前桶进行加锁,可能存在多个线程同时来同一个SpanList来获取内存对象
	std::unique_lock<std::mutex> lock(spanlist->_mtx);

	Span* span = GetOneSpan(spanlist, bytes);
	void* cur = span->_objlist;
	void* prev = cur;// prev记录最后一个内存对象
	size_t fetchnum = 0;
	// 可能该span中没有那么多的内存对象
	// 这种情况下有多少返给多少
	while (cur != nullptr && fetchnum < num)
	{
		prev = cur;
		cur = NEXT_OBJ(cur);
		++fetchnum;
	}

	start = span->_objlist;
	end = prev;
	NEXT_OBJ(end) = nullptr;
        // 将剩下来的内存对象再次接span的objlist上
	span->_objlist = cur;
	span->_usecount += fetchnum;


	// 每次将span中的内存块拿出来的时候,判断这个span中还有没有内存块
	// 没有就放到最后，这样做可以提高下一次的检索效率
	if (span->_objlist == nullptr)
	{
		spanlist->Erase(span);
		spanlist->PushBack(span);
	}
	return fetchnum;
}

void CentralCache::ReleaseListToSpans(void* start, size_t byte)
{
	// 找到对应的spanlist
	size_t index = ClassSize::Index(byte);
	SpanList* spanlist = &_spanlist[index];

	// CentralCache:对当前桶进行加锁(桶锁)
	// PageCache:必须对整个SpanList全局加锁
	// 因为可能存在多个线程同时去系统申请内存的情况
	std::unique_lock<std::mutex> lock(spanlist->_mtx);
        
	
	// 遍历start那条空闲链表，重新连到span的_objlist中
	while (start)
	{
		void* next = NEXT_OBJ(start);
		// 获取内存对象到span的映射
		Span* span = PageCache::GetInstance()->MapObjectToSpan(start);


		//当释放对象回到空的span，把空的span移到头上
		if (span->_objlist == nullptr)
		{
			spanlist->Erase(span);
			spanlist->PushFront(span);
		}

		// 将内存对象采用头插归还给CentralCache的span
		NEXT_OBJ(start) = span->_objlist;
		span->_objlist = start;
		// usecount == 0表示span切出去的对象都还回来了
		// 释放span回到pagecache进行合并
		if (--span->_usecount == 0)
		{
			spanlist->Erase(span);
                        
			// 将一个span从CentralCache归还到PageCache的时候只需要页号和页数
			// 不需要其他的东西所以对于其他的数据进行赋空
			span->_objlist = nullptr;
			span->_objsize = 0;
			span->_prev = nullptr;
			span->_next = nullptr;

			PageCache::GetInstance()->ReleaseSpanToPageCahce(span);
		}

		start = next;
	}
}

