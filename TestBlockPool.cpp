#include "ObjectBlockPool.h"

int main()
{
	{
		ObjectPool<SceneUser> pool;
		SceneUser* a = pool.create(1, 1);
		SceneUser* b = pool.create(2, 2);
		SceneUser* c = pool.create(3, 3);
		//pool.release(a);
		SceneUser* d = pool.create(4, 4);
		//pool.release(b);
		//pool.release(a);
		//pool.release(d);
		pool.releaseSomeBlocks();
		pool.printBlocks();
	}
	
	return 0;
}
