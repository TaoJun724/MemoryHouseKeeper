#include "CentralCache.h"
#include "PageCache.h"

CentralCache CentralCache::_inst;

//// ��׮����

Span* CentralCache::GetOneSpan(SpanList* spanlist, size_t bytes)
{
	//CentralCache�д���span
	Span* span = spanlist->begin();
	while (span != spanlist->end())
	{
		if (span->_objlist != nullptr)
			return span;

		span = span->_next;
	}

	// CetralCache����span,��pagecache����һ���µĺ��ʴ�С��span,����ҳ�ţ��µ�span��npageҳ
	size_t npage = ClassSize::NumMovePage(bytes);
	Span* newspan = PageCache::GetInstance()->NewSpan(npage);

	// ��span���ڴ��и��һ����bytes��С�Ķ��������
	char* start = (char*)(newspan->_pageid << PAGE_SHIFT);
	//endΪ���һ���ֽڵ���һ��ַ����ע��
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

	// ��newspan���뵽spanlist
	spanlist->PushFront(newspan);
	return newspan;
}

// �����Ļ����ȡһ�������Ķ����threadcache
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t num, size_t bytes)
{
	//�����span���������е��±�
	size_t index = ClassSize::Index(bytes);
	SpanList* spanlist = &_spanlist[index];

	// �Ե�ǰͰ���м���,���ܴ��ڶ���߳�ͬʱ��ͬһ��SpanList����ȡ�ڴ����
	std::unique_lock<std::mutex> lock(spanlist->_mtx);

	Span* span = GetOneSpan(spanlist, bytes);
	void* cur = span->_objlist;
	void* prev = cur;// prev��¼���һ���ڴ����
	size_t fetchnum = 0;
	while (cur != nullptr && fetchnum < num)
	{
		prev = cur;
		cur = NEXT_OBJ(cur);
		++fetchnum;
	}

	start = span->_objlist;
	end = prev;
	NEXT_OBJ(end) = nullptr;

	span->_objlist = cur;
	span->_usecount += fetchnum;


	//��һ��spanΪ�գ���span�Ƶ�β��
	if (span->_objlist == nullptr)
	{
		spanlist->Erase(span);
		spanlist->PushBack(span);
	}
	return fetchnum;
}

void CentralCache::ReleaseListToSpans(void* start, size_t byte)
{
	// �ҵ���Ӧ��spanlist
	size_t index = ClassSize::Index(byte);
	SpanList* spanlist = &_spanlist[index];

	// CentralCache:�Ե�ǰͰ���м���(Ͱ��)
	// PageCache:���������SpanListȫ�ּ���
	// ��Ϊ���ܴ��ڶ���߳�ͬʱȥϵͳ�����ڴ�����
	std::unique_lock<std::mutex> lock(spanlist->_mtx);

	while (start)
	{
		void* next = NEXT_OBJ(start);
		// ��ȡ�ڴ����span��ӳ��
		Span* span = PageCache::GetInstance()->MapObjectToSpan(start);


		//���ͷŶ���ص��յ�span���ѿյ�span�Ƶ�ͷ��
		if (span->_objlist == nullptr)
		{
			spanlist->Erase(span);
			spanlist->PushFront(span);
		}

		// ���ڴ�������ͷ��黹��CentralCache��span
		NEXT_OBJ(start) = span->_objlist;
		span->_objlist = start;
		// usecount == 0��ʾspan�г�ȥ�Ķ��󶼻�������
		// �ͷ�span�ص�pagecache���кϲ�
		if (--span->_usecount == 0)
		{
			spanlist->Erase(span);

			span->_objlist = nullptr;
			span->_objsize = 0;
			span->_prev = nullptr;
			span->_next = nullptr;

			PageCache::GetInstance()->ReleaseSpanToPageCahce(span);
		}

		start = next;
	}
}
