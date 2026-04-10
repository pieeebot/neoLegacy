#include "stdafx.h"
#include "net.minecraft.world.level.biome.h"
#include "IntCache.h"
#include "DeepOceanLayer.h"

DeepOceanLayer::DeepOceanLayer(int64_t seed, shared_ptr<Layer> parent) : Layer(seed)
{
	this->parent = parent;
}

intArray DeepOceanLayer::getArea(int xo, int yo, int w, int h)
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
			int l2 = 0;

			if (k1 == 0)
			{
				++l2;
			}

			if (l1 == 0)
			{
				++l2;
			}

			if (i2 == 0)
			{
				++l2;
			}

			if (j2 == 0)
			{
				++l2;
			}

			if (k2 == 0 && l2 > 3)
			{
				aint1[j1 + i1 * w] = Biome::deepOcean->id;
			}
			else
			{
				aint1[j1 + i1 * w] = k2;
			}
		}
	}

	return aint1;
}
