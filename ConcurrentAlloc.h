#ifndef __CONCURRENT_ALLOC_H__
#define __CONCURRENT_ALLOC_H__

#include "Common.h"
#include"ThreadCache.h"
#include"PageCache.h"

//申请内存
static void* ConcurrentAlloc(size_t size)
{
	// 申请的内存大于64K,就直接到PageCache中申请内存
	if (size > MAXBYTES)
	{
		//按照页的大小进行对齐，计算出申请的页的个数1页是4k
		size_t roundsize = ClassSize::_Roundup(size, 1 << PAGE_SHIFT);
		size_t npage = roundsize >> PAGE_SHIFT;
		//获得span对象
		Span* span = PageCache::GetInstance()->NewSpan(npage);
		void* ptr = (void*)(span->_pageid << PAGE_SHIFT);
		
		return ptr;
	}
	else//64k以内通过threadcache内申请
	{
		// 通过tls，获取线程自己的threadcache
		if (tls_threadcache == nullptr)
		{
			tls_threadcache = new ThreadCache;
			//cout << std::this_thread::get_id() << "->" << tls_threadcache << endl;
		}
		//返回获取的内存块的地址
		return tls_threadcache->Allocate(size);
		//return nullptr;
	}
}


//释放内存
static void ConcurrentFree(void* ptr)
{
	//获取页号到span的映射
	Span* span = PageCache::GetInstance()->MapObjectToSpan(ptr);
	size_t size = span->_objsize;
	if (size > MAXBYTES)
	{
		//申请大于64k, 直接去PageCache, 大小记录在objsize
		//释放大于64k的内存，直接归还给PageCache,objsize中存着大小
		PageCache::GetInstance()->ReleaseSpanToPageCahce(span);
	}
	else
	{
		tls_threadcache->Deallocate(ptr, size);
	}
}
#endif
