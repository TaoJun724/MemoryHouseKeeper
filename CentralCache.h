#ifndef __CENTRAL_CACHE_H__
#define __CENTRAL_CACHE_H__


#include "Common.h"


//ʹ�ö���ģʽ
class CentralCache
{
public:
	static CentralCache* GetInstance(){
		return &_inst;
	}

	Span* GetOneSpan(SpanList* spanlist, size_t bytes);

	// �����Ļ����ȡһ�������Ķ����thread cache
	size_t FetchRangeObj(void*& start, void*& end, size_t num, size_t byte);

	// ��һ�������Ķ����ͷŵ�span���
	void ReleaseListToSpans(void* start, size_t byte_size);
private:
	// ���Ļ�����������
	SpanList _spanlist[NLISTS];
private:
	CentralCache() = default;//default��������Ĭ�ϵ�
	CentralCache(const CentralCache&) = delete;//delect��������
	CentralCache& operator=(const CentralCache&) = delete;

	static CentralCache _inst;
};


#endif