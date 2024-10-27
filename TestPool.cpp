#include "ObjectPool.h"

int main()
{
	ObjectPool<SceneUser> pool;
	//SceneUser* a = pool.create(1, 1);
	//SceneUser* b = pool.create(2, 2);
	//pool.release(a);
	//pool.release(b);
	//SceneUser* c = pool.create(3, 3);
	pool.expandPool();

	return 0;
}
