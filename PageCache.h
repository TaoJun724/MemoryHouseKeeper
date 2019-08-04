#ifndef __PAGE_CACHE_H__
#define __PAGE_CACHE_H__

#include "Common.h"


//饿汉模式
class PageCache
{
public:
	static PageCache* GetInstance()
	{
		return &_inst;
	}

	//从系统申请span或者大于要申请的npage的Pagespan中申请
	Span* NewSpan(size_t npage);
	Span* _NewSpan(size_t npage);

	// 获取从对象到span的映射
	Span* MapObjectToSpan(void* obj);

	// 释放空闲span回到Pagecache，并合并相邻的span
	void ReleaseSpanToPageCahce(Span* span);

private:
	//NPAGES是129，但是使用128个数据元素，也就是下标从1开始到128分别为1页到128页
	SpanList _pagelist[NPAGES];
private:
	PageCache() = default;
	PageCache(const PageCache&) = delete;
	static PageCache _inst;

	std::mutex _mtx;
	std::unordered_map<PageID, Span*> _id_span_map;
};


#endif
