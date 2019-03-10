#include"ConcurrentAlloc.h"

void TestThreadCache()
{
	//1.验证分配的threadcache不同
	//std::thread t1(ConcurrentAlloc, 10);
	//std::thread t2(ConcurrentAlloc, 10);

	//t1.join();
	//t2.join();

	//2.验证内存的反复使用
	//（1）第一次申请
	std::vector<void*> v;
	for (size_t i = 0; i < 10; ++i)
	{
		v.push_back(ConcurrentAlloc(10));
		cout << v.back() << endl;
	}
	cout << endl << endl;

	for (size_t i = 0; i < 10; ++i)
	{
		ConcurrentFree(v[i]);
	}
	v.clear();
	//（2）第二次申请
	for (size_t i = 0; i < 10; ++i)
	{
		v.push_back(ConcurrentAlloc(10));
		cout << v.back() << endl;
	}

	for (size_t i = 0; i < 10; ++i)
	{
		ConcurrentFree(v[i]);
	}
	v.clear();
}

//验证页号与地址的对应
void TestPageCache()
{
	void* ptr = VirtualAlloc(NULL, (NPAGES - 1) << PAGE_SHIFT, MEM_RESERVE, PAGE_READWRITE);//按页分配
	//void* ptr = malloc((NPAGES - 1) << PAGE_SHIFT);//malloc不可以
	cout << ptr << endl;
	if (ptr == nullptr)
	{
		throw std::bad_alloc();
	}

	PageID pageid = (PageID)ptr >> PAGE_SHIFT;
	cout << pageid << endl;

	void* shiftptr = (void*)(pageid << PAGE_SHIFT);
	cout << shiftptr << endl;
}

//会源源不断的申请内存
void TestConcurrentAlloc()
{
	size_t n = 100000;
	std::vector<void*> v;
	for (size_t i = 0; i < n; ++i)
	{
		v.push_back(ConcurrentAlloc(10));
		cout << v.back() << endl;
	}
	cout << endl << endl;

	for (size_t i = 0; i < n; ++i)
	{
		ConcurrentFree(v[i]);
	}
	v.clear();

	for (size_t i = 0; i < n; ++i)
	{
		v.push_back(ConcurrentAlloc(10));
		cout << v.back() << endl;
	}

	for (size_t i = 0; i < n; ++i)
	{
		ConcurrentFree(v[i]);
	}
	v.clear();
}

void TestLargeAlloc()
{
	void* ptr1 = ConcurrentAlloc(MAXBYTES * 2);
	void* ptr2 = ConcurrentAlloc(129 << PAGE_SHIFT);

	ConcurrentFree(ptr1);
	ConcurrentFree(ptr2);
}

//int main()
//{
//	//TestThreadCache();
//	//TestPageCache();
//	//TestConcurrentAlloc();
//	TestLargeAlloc();
//	system("pause");
//	return 0;
//}