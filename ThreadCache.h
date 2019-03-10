#ifndef  __THREAD_CACHE_H__
#define  __THREAD_CAHCE_H__


#include"Common.h"

class ThreadCache
{
public:
	void* Allocate(size_t size);
	void Deallocate(void* ptr, size_t size);

	// �����Ļ����ȡ����
	void* FetchFromCentralCache(size_t index, size_t size);
	// �����ж���̫�࣬��ʼ���ա�
	void ListTooLong(FreeList* freelist, size_t byte);
private:
	FreeList _freelist[NLISTS];
};

//��ô��ÿ���̶߳����Լ���һ���̻߳���
//����һ��
//ʹ��ThreadCache* list;ȫ�ֱ��������е�ThreadCache��������������
//ÿ�������tid��int _tid������һ�����ָ�루ThreadCache* _next����
//����ȡThreadCache������Ҫ������

//��������
//ʹ��TLS����(Thread Local Storage)�̱߳��ش洢 
//������һ����̬��TLS�������������ÿ���̶߳���һ��
static _declspec(thread) ThreadCache* tls_threadcache = nullptr;


#endif