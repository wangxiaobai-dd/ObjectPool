
#include <vector>
#include <iostream>
#include <concepts>
#include <map>
#include <stdlib.h>
#include <cassert>

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

#define BLOCK_T_SIZE 3

template <typename T>
concept HasPoolFlag = requires(T t)
{
	t.fromObjPool;
};

/******** ObjectNode ********/

template <typename T> requires HasPoolFlag<T>
class alignas(8) ObjectNode
{
	template <typename C> friend class ObjectPool;
	template <typename C> friend class ObjectBlock;
private:
	T value;
	ObjectNode<T>* next = nullptr;
	int blockIndex = -1;
};


/******** ObjectBlock ********/

template <typename T> requires HasPoolFlag<T>
class ObjectBlock
{
	template <typename C> friend class ObjectPool;
public:
	void releaseSomeBlocks();
	ObjectBlock(){};
	~ObjectBlock();

private:
	ObjectBlock(const ObjectBlock&) = delete;
	ObjectBlock& operator=(const ObjectBlock&) = delete;
	
	int freeCount = 0;
	ObjectNode<T>* start = nullptr;
	ObjectNode<T>* freeHead = nullptr;

	void printFreeNode();
};

template <typename T>
ObjectBlock<T>::~ObjectBlock()
{
	if(start)
		free(start);
}

template <typename T>
void ObjectBlock<T>::printFreeNode()
{
	auto pNode = freeHead;
	while(pNode)
	{
		std::cout << __FUNCTION__ << ",pNode:" << pNode << std::endl;
		pNode = pNode->next;
	}
	std::cout << std::endl;
}

/******** ObjectPool ********/

template <typename T> requires HasPoolFlag<T>
class ObjectPool
{
public:
	ObjectPool(){};
	~ObjectPool();

	template <typename... Args>
	T* create(Args&&... args);
	void release(T*& pValue);
	void releaseSomeBlocks();
	void printBlocks(); 
private: 
	ObjectPool(const ObjectPool&) = delete;
	ObjectPool& operator= (const ObjectPool&) = delete;

	std::vector<ObjectBlock<T>*> blocks;
	bool createNewBlock();

	int freeBlockIndex = 0;
	void nextFreeBlock();
};

template <typename T>
ObjectPool<T>::~ObjectPool()
{
	for(const auto& pBlock : blocks)
	{
		if(pBlock->freeCount == BLOCK_T_SIZE)	
			delete(pBlock);
		else
			std::cout << __FUNCTION__ << "调用者未释放," << pBlock << std::endl;
	}
	std::cout << __FUNCTION__ << std::endl;
}

template <typename T>
template <typename... Args>
T* ObjectPool<T>::create(Args&&... args)
{
	std::cout << std::endl;
	if(freeBlockIndex >= blocks.size())
	{
		if(!createNewBlock())
			return nullptr;
	}
	auto pBlock = blocks[freeBlockIndex];
	auto pNode = pBlock->freeHead;
	pNode->blockIndex = freeBlockIndex;
	pBlock->freeHead = pNode->next;
	pNode->next = nullptr;
	pBlock->freeCount -= 1;

	std::cout << __FUNCTION__ << ",block[" << freeBlockIndex << "]:" << pBlock << ",pNode:" << pNode << std::endl;

	if(pBlock->freeCount == 0)
		nextFreeBlock();
	
	new (&pNode->value) T(std::forward<Args>(args)...);
	return &pNode->value;
}

template <typename T>
void ObjectPool<T>::release(T*& pValue)
{
	std::cout << std::endl;
	assert(pValue);

	auto pNode = (ObjectNode<T>*)pValue;

	assert(pNode->blockIndex != -1 && pNode->blockIndex < blocks.size());

	pValue->~T();
	pValue = nullptr;

	auto pBlock = blocks[pNode->blockIndex];
	if(pNode->blockIndex < freeBlockIndex)
		freeBlockIndex = pNode->blockIndex;

	std::cout << __FUNCTION__ << ",block[" << pNode->blockIndex << "]:" << pBlock << ",pNode:" << pNode << std::endl;

	pNode->next = pBlock->freeHead;
	pNode->blockIndex = -1;
	pBlock->freeHead = pNode;
	pBlock->freeCount += 1;

	pBlock->printFreeNode();
}

template <typename T>
bool ObjectPool<T>::createNewBlock()
{
	ObjectBlock<T>* pBlock = new ObjectBlock<T>();
	if(!pBlock)
		return false;
	int size = BLOCK_T_SIZE;
	int totalSize = sizeof(ObjectNode<T>) * size;
	auto pNodes = (ObjectNode<T>*)aligned_alloc(alignof(ObjectNode<T>), totalSize);
	if(!pNodes)
		return false;
	pBlock->start = pNodes;
	for(int i = 0; i < size; ++i)
	{
		auto pNode = pNodes + i;
		pNode->next = pBlock->freeHead;
		pNode->blockIndex = -1;
		pBlock->freeHead = pNode;
		pBlock->freeCount += 1;
	}
	freeBlockIndex = blocks.size();
	blocks.push_back(pBlock);

	std::cout << __FUNCTION__ << ",block[" << freeBlockIndex << "]:" << pBlock << ",start:" << pNodes << std::endl;
	pBlock->printFreeNode();

	return true;
}

template <typename T>
void ObjectPool<T>::nextFreeBlock()
{
	for(int i = 0; i < blocks.size(); ++i)
	{
		if(blocks[i]->freeCount > 0)
		{
			freeBlockIndex = i;
			break;	
		}
	}
	freeBlockIndex = blocks.size();
}

template <typename T>
void ObjectPool<T>::releaseSomeBlocks()
{
	std::cout << __FUNCTION__ << ",start:" << blocks.size() <<  std::endl;
	// 至少保留1个 Block
	for(auto iter = blocks.rbegin(); iter != blocks.rend() && blocks.size() > 1; )
	{
		if((*iter)->freeCount == BLOCK_T_SIZE)	
		{
			delete(*iter);
			iter = typename std::vector<ObjectBlock<T>*>::reverse_iterator(blocks.erase((iter + 1).base()));
		}
		else
			++iter;
	}
	std::cout << __FUNCTION__ << ",end:" << blocks.size() <<  std::endl;
}

template <typename T>
void ObjectPool<T>::printBlocks()
{
	for(int i = 0; i < blocks.size(); ++i)
	{
		std::cout << std::endl;
		std::cout << __FUNCTION__ << ",block[" << i << "]:" << blocks[i] << std::endl; 
		blocks[i]->printFreeNode();
	}
}
