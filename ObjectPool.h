
#include <vector>
#include <iostream>
#include <concepts>
#include <map>
#include <stdlib.h>

struct SceneUser
{
	SceneUser(int _id, int _level)
	{
		id = _id;
		level = _level;
		dataMap[id] = level;
		std::cout << "SceneUser(),mapSize:" << dataMap.size() <<  std::endl;
	}
	~SceneUser()
	{ 
		std::cout << "~SceneUser(),id:" << id << std::endl;
		id = 0; 
		level = 0; 
	}

	int id = 0;
	int level = 0;
	std::map<int, int> dataMap;

	bool fromObjPool = false;
};

#define POOL_INIT_SIZE 10

template <typename T>
concept HasPoolFlag = requires(T t)
{
	t.fromObjPool;
};

template <typename T> requires HasPoolFlag<T>
class alignas(8) ObjectNode
{
public:
	T value;
	ObjectNode<T>* next = nullptr; 
};

template <typename T> requires HasPoolFlag<T>
class ObjectPool
{
public:
	ObjectPool(){};

	template <typename... Args>
	T* create(Args&&... args);

	void release(const T* pValue);

	void printFreeNode();
	void expandPool();
private: 
	ObjectNode<T>* freeHead = nullptr;

	int useCount = 0;
	int freeCount = 0;
};


template <typename T>
template <typename... Args>
T* ObjectPool<T>::create(Args&&... args)
{
	if(!freeHead)
	{
		expandPool();

		if(!freeHead)
			return nullptr;
	}

	useCount += 1;
	freeCount -= 1;


	auto node = freeHead;

	std::cout << "create,node:" << node << ",freeCount:" << freeCount << ",useCount:" << useCount << ",next:" << freeHead->next << std::endl;

	freeHead = freeHead->next;
	new (&node->value) T(std::forward<Args>(args)...);

	printFreeNode();

	return &node->value;
}

template <typename T>
void ObjectPool<T>::release(const T* pValue)
{
	if(!pValue)
		return;

	useCount -= 1;
	freeCount += 1;


	auto node = (ObjectNode<T>*)pValue;

	std::cout << "release,node:" << node << ",freeCount:" << freeCount << ",useCount:" << useCount << ",next:" << freeHead->next << std::endl;

	pValue->~T();
	node->next = freeHead;
	freeHead = node;

	printFreeNode();

}

template <typename T>
void ObjectPool<T>::expandPool()
{
	size_t size = POOL_INIT_SIZE;

	//auto nodes = (ObjectNode<T>*)malloc(sizeof(ObjectNode<T>) * size);
	size_t totalSize = sizeof(ObjectNode<T>) * size;
	auto nodes = (ObjectNode<T>*)aligned_alloc(alignof(ObjectNode<T>), totalSize);
	if(!nodes)
		return;

	std::cout << nodes << std::endl;
	std::cout << nodes+1 << std::endl;

	std::cout << "expand:" << std::endl;
	std::cout << sizeof(T) << std::endl;
	std::cout << sizeof(ObjectNode<T>) << std::endl;
	
	
	for(size_t i = 0; i < size; ++i)
	{
		auto node = nodes + i;	
		std::cout << "expand,i:" << i << " node:"<< (void*)(nodes + i) << std::endl;
	//	if(i > 0)
	//		std::cout << "Offset between Node " << i-1 << " and Node " << i << ": " << (uintptr_t)(nodes + i) - (uintptr_t)(nodes + i - 1) << std::endl;
		node->next = freeHead;
		freeHead = node;
		freeCount += 1;
	}
	std::cout << "expandPool,freeCount:" << freeCount << std::endl;
}

template <typename T>
void ObjectPool<T>::printFreeNode()
{
	auto node = freeHead;
	while(node)
	{
		std::cout << "printFreeNode,node:" << node << std::endl;
		node = node->next;
	}
	std::cout << std::endl;
}

