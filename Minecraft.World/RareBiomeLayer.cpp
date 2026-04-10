#include "stdafx.h"
#include "net.minecraft.world.level.biome.h"
#include "IntCache.h"
#include "RareBiomeLayer.h"

RareBiomeLayer::RareBiomeLayer(int64_t seed, shared_ptr<Layer> parent) : Layer(seed)
{
	this->parent = parent;
}

intArray RareBiomeLayer::getArea(int xo, int yo, int w, int h)
{
	intArray aint = this->parent->getArea(xo - 1, yo - 1, w + 2, h + 2);
	intArray aint1 = IntCache::allocate(w * h);

	for (int i = 0; i < h; ++i)
	{
		for (int j = 0; j < w; ++j)
		{
			this->initRandom((int64_t)(j + xo), (int64_t)(i + yo));
			int k = aint[j + 1 + (i + 1) * (w + 2)];

			if (this->nextRandom(57) == 0)
			{
				if (k == Biome::plains->id)
				{
					aint1[j + i * w] = Biome::plains->id + 128;
				}
				else
				{
					aint1[j + i * w] = k;
				}
			}
			else
			{
				aint1[j + i * w] = k;
			}
		}
	}

	return aint1;
}
