#ifndef __COMMON_H__
#define __COMMON_H__

#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <assert.h>
#include <unordered_map>
#include <map>

#ifdef _WIN32
#include <windows.h>
#endif // _WIN32


using std::cout;
using std::endl;

// 管理对象自由链表的长度
const size_t NLISTS = 240;

//ThreadCache最大一次可以分配最大的内存64k
const size_t MAXBYTES = 64 * 1024;

//一页是4096字节，2的12次方是4096
const size_t PAGE_SHIFT = 12;

//PageCache的最大可以存放NPAGES-1页
const size_t NPAGES = 129;



//向系统申请空间
static void* SystemAlloc(size_t npage)
{
#ifdef _WIN32
	void* ptr = VirtualAlloc(NULL, (NPAGES - 1) << PAGE_SHIFT, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (ptr == nullptr)
	{
		throw std::bad_alloc();
	}
#else
	//brk mmap
#endif
	return ptr;
}


//向系统释放空间
inline static void SystemFree(void* ptr)
{
#ifdef _WIN32
	VirtualFree(ptr, 0, MEM_RELEASE);
	if (ptr == nullptr)
	{
		throw std::bad_alloc();
	}
#else
	//brk mumap
#endif

}

//使用span数据结构管理内存
//一个span包含多页
typedef size_t PageID;
struct Span
{
	PageID _pageid = 0;			 // 页号
	size_t _npage = 0;		     // 页的数量
        //维护一条span对象的双向带头循环链表
	Span*  _next = nullptr;
	Span*  _prev = nullptr;

	void*  _objlist = nullptr;	// 对象自由链表
	size_t _objsize = 0;		// 记录该span上的内存块对象大小
	size_t _usecount = 0;		// 使用计数
};

//双向带头链表
class SpanList
{
public:
	SpanList()
	{
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}

	Span* begin()
	{
		return _head->_next;
	}

	Span* end()
	{
		return _head;
	}

	bool Empty()
	{
		return _head->_next == _head;
	}

	void Insert(Span* cur, Span* newspan)
	{
		assert(cur);
		Span* prev = cur->_prev;

		prev->_next = newspan;
		newspan->_prev = prev;
		newspan->_next = cur;
		cur->_prev = newspan;
	}

	void Erase(Span* cur)
	{
		assert(cur != nullptr && cur != _head);
		Span* prev = cur->_prev;
		Span* next = cur->_next;

		prev->_next = next;
		next->_prev = prev;
	}

	void PushFront(Span* span)
	{
		Insert(begin(), span);
	}


	void PushBack(Span* span)
	{
		Insert(end(), span);//end是最后一个结点的下一个
	}


	Span* PopBack()
	{
		Span* end = _head->_prev;
		Erase(end);
		return end;
	}
	Span* PopFront()
	{
		Span* span = begin();
		Erase(span);
		return span;
	}
        
	std::mutex _mtx;
private:
	Span* _head = nullptr;
};



// 取出指针指向的前4或者前8个字节
static inline void*& NEXT_OBJ(void* obj)
{
	return *((void**)obj);
}



//自由链表类
class FreeList
{
public:
	bool Empty()
	{
		return _list == nullptr;
	}
        //将一段内存对象添加到自由链表中
	void PushRange(void* start, void* end, size_t num)
	{
		NEXT_OBJ(end) = _list;
		_list = start;
		_size += num;
	}

	void* Clear()
	{
		_size = 0;
		void * list = _list;
		_list = nullptr;
		return list;
	}
        //头删
	void* Pop()
	{
		void* obj = _list;
		_list = NEXT_OBJ(obj);
		--_size;

		return obj;
	}
        //头插
	void Push(void* obj)
	{
		NEXT_OBJ(obj) = _list;
		_list = obj;
		++_size;
	}
	size_t Size()
	{
		return _size;
	}

	void SetMaxSize(size_t maxsize)
	{
		_maxsize = maxsize;
	}

	size_t MaxSize()
	{
		return _maxsize;
	}
private:
	void* _list = nullptr;//一个自由链表
	size_t _size = 0;//内存节点个数
	size_t _maxsize = 1;//最多有多少个内存结点
};




// 管理对齐映射
class ClassSize
{
	// 控制在12%左右的内碎片浪费
	// [1,128]				8byte对齐 freelist[0,16)     //自由链表的长队为 128/8 = 16 所以为[0,16）
	// [129,1024]			16byte对齐 freelist[16,72)   //自由链表的长队为 （1024-128 / 16）= 56 所以为[16,72）
	// [1025,8*1024]		128byte对齐 freelist[72,128)
	// [8*1024+1,64*1024]	1024byte对齐 freelist[128,240)
public:
	static inline size_t _Roundup(size_t size, size_t align)
	{
		return (size + align - 1) & ~(align - 1);
		//align是对齐数
		//比如size为15 < 128，对齐数align为8，那么要进行向上取整
	        //（（15 + 7）/8）*8
		// 这个式子就是将(align - 1)加上去
		// 然后再将加上去的二进制的低三位设置为0
		// 15 + 7 = 22 : 10110 
		// 7 : 111 ~7 : 000
		// 22 & ~7 : 10000 (16)就达到了向上取整的效果
	}
        //向上取整
	static inline size_t Roundup(size_t size)
	{
		assert(size <= MAXBYTES);

		if (size <= 128) {
			return _Roundup(size, 8);
		}
		else if (size <= 1024){
			return _Roundup(size, 16);
		}
		else if (size <= 8192){
			return _Roundup(size, 128);
		}
		else if (size <= 65536){
			return _Roundup(size, 512);
		}
		else {
			return -1;
		}
	}

        //求出在该区的第几个
	static inline size_t _Index(size_t bytes, size_t align_shift)
	{
		// 给bytes加上对齐数减一也就是
		// 让其可以跨越到下一个自由链表的数组的元素中
		return ((bytes + (1 << align_shift) - 1) >> align_shift) - 1;
	}
        
	//获取自由链表的下标
	static inline size_t Index(size_t bytes)
	{
		assert(bytes < MAXBYTES);

		// 每个区间有多少个自由链表
		static int group_array[4] = { 16, 56, 56, 112 };

		if (bytes <= 128){
			return _Index(bytes, 3);
		}
		else if (bytes <= 1024){
			return _Index(bytes - 128, 4) + group_array[0];
		}
		else if (bytes <= 8192){
			return _Index(bytes - 1024, 7) + group_array[1] + group_array[0];
		}
		else if (bytes <= 65536){
			return _Index(bytes - 8192, 9) + group_array[2] + group_array[1] +
				group_array[0];
		}
		else{
			return -1;
		}
	}


	//定量申请
	//一次最少向系统申请是2个对象，最多512个对象
	static size_t NumMoveSize(size_t size)
	{
		if (size == 0)
			return 0;

		int num = (int)(MAXBYTES / size);
		if (num < 2)
			num = 2;

		if (num > 512)
			num = 512;

		return num;
	}

	// 计算一次向系统获取几个页
	static size_t NumMovePage(size_t size)
	{
		size_t num = NumMoveSize(size);//多少个对象
		size_t npage = num*size;

		npage >>= 12;//计算出是几页
		if (npage == 0)
			npage = 1;

		return npage;
	}
};
#endif
