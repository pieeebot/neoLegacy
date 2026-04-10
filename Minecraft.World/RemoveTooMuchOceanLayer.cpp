#include "stdafx.h"
#include "net.minecraft.world.level.biome.h"
#include "IntCache.h"
#include "RemoveTooMuchOceanLayer.h"

RemoveTooMuchOceanLayer::RemoveTooMuchOceanLayer(int64_t seed, shared_ptr<Layer> parent) : Layer(seed)
{
	this->parent = parent;
}

intArray RemoveTooMuchOceanLayer::getArea(int xo, int yo, int w, int h)
{
	int i = xo - 1;
	int j = yo - 1;
	int k = w + 2;
	int l = h + 2;
	intArray aint = this->parent->getArea(i, j, k, l);
	intArray aint1 = IntCache::allocate(w * h);

	for (int i1 = 0; i1 < h; ++i1)
	{
		for (int j1 = 0; j1 < w; ++j1)
		{
			int k1 = aint[j1 + 1 + (i1 + 1 - 1) * (w + 2)];
			int l1 = aint[j1 + 1 + 1 + (i1 + 1) * (w + 2)];
			int i2 = aint[j1 + 1 - 1 + (i1 + 1) * (w + 2)];
			int j2 = aint[j1 + 1 + (i1 + 1 + 1) * (w + 2)];
			int k2 = aint[j1 + 1 + (i1 + 1) * k];
			aint1[j1 + i1 * w] = k2;
			this->initRandom((int64_t)(j1 + xo), (int64_t)(i1 + yo));

			if (k2 == 0 && k1 == 0 && l1 == 0 && i2 == 0 && j2 == 0)
			{
				aint1[j1 + i1 * w] = 1;
			}
		}
	}

	return aint1;
}
