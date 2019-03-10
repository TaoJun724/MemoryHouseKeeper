#ifndef __CONCURRENT_ALLOC_H__
#define __CONCURRENT_ALLOC_H__

#include "Common.h"
#include"ThreadCache.h"
#include"PageCache.h"

static void* ConcurrentAlloc(size_t size)
{
	// ������ڴ����64K,��ֱ�ӵ�PageCache�������ڴ�
	if (size > MAXBYTES)
	{
		size_t roundsize = ClassSize::_Roundup(size, 1 << PAGE_SHIFT);
		size_t npage = roundsize >> PAGE_SHIFT;

		Span* span = PageCache::GetInstance()->NewSpan(npage);
		void* ptr = (void*)(span->_pageid << PAGE_SHIFT);
		return ptr;
	}
	else//64k����ͨ��threadcache������
	{
		// ͨ��tls����ȡ�߳��Լ���threadcache
		if (tls_threadcache == nullptr)
		{
			tls_threadcache = new ThreadCache;
			//cout << std::this_thread::get_id() << "->" << tls_threadcache << endl;
		}
		return tls_threadcache->Allocate(size);
		//return nullptr;
	}
}



static void ConcurrentFree(void* ptr)
{
	Span* span = PageCache::GetInstance()->MapObjectToSpan(ptr);
	size_t size = span->_objsize;
	if (size > MAXBYTES)
	{
		//�������64k, ֱ��ȥPageCache, ��С��¼��objsize
		//�ͷŴ���64k���ڴ棬ֱ�ӹ黹��PageCache,objsize�д��Ŵ�С
		PageCache::GetInstance()->ReleaseSpanToPageCahce(span);
	}
	else
	{
		tls_threadcache->Deallocate(ptr, size);
	}
}
#endif