#include "stdafx.h"
#include "net.minecraft.world.level.biome.h"
#include "IntCache.h"
#include "RegionHillsLayer.h"



RegionHillsLayer::RegionHillsLayer(int64_t seed, shared_ptr<Layer> parent) : Layer(seed)
{
	this->parent = parent;
	this->riverNoise = nullptr;
}

RegionHillsLayer::RegionHillsLayer(int64_t seed, shared_ptr<Layer> parent, shared_ptr<Layer> riverNoise) : Layer(seed)
{
	this->parent     = parent;
	this->riverNoise = riverNoise;
}

void RegionHillsLayer::init(int64_t seed)
{
	Layer::init(seed);
	if (riverNoise != nullptr)
		riverNoise->init(seed);
}


bool RegionHillsLayer::biomesEqualOrMesaPlateau(int a, int b)
{
	return a == b;
}


intArray RegionHillsLayer::getArea(int xo, int yo, int w, int h)
{
	
	intArray b  = parent->getArea(xo - 1, yo - 1, w + 2, h + 2);
	intArray noise = (riverNoise != nullptr)
	    ? riverNoise->getArea(xo - 1, yo - 1, w + 2, h + 2)
	    : IntCache::allocate((w + 2) * (h + 2)); 

	intArray result = IntCache::allocate(w * h);

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			initRandom(x + xo, y + yo);

			int k = b   [x + 1 + (y + 1) * (w + 2)]; 
			int l = noise[x + 1 + (y + 1) * (w + 2)]; 

			
			bool flag = (riverNoise != nullptr) && ((l - 2) % 29 == 0);

			if (riverNoise != nullptr && k != 0 && l >= 2 && (l - 2) % 29 == 1 && k < 128)
			{
				
				result[x + y * w] = k;
			}
			else if (nextRandom(3) != 0 && !flag)
			{
				
				result[x + y * w] = k;
			}
			else
			{
				
				int i1 = k;

				if (k == Biome::desert->id)
				{
					i1 = Biome::desertHills->id;
				}
				else if (k == Biome::forest->id)
				{
					i1 = Biome::forestHills->id;
				}
				else if (k == Biome::birchForest->id)
				{
					i1 = Biome::birchForestHills->id;
				}
				else if (k == Biome::roofedForest->id)
				{
					// Java: roofedForest -> plains 
					i1 = Biome::plains->id;
				}
				else if (k == Biome::taiga->id)
				{
					i1 = Biome::taigaHills->id;
				}
				else if (k == Biome::megaTaiga->id)
				{
					i1 = Biome::megaTaigaHills->id;
				}
				else if (k == Biome::coldTaiga->id)
				{
					i1 = Biome::coldTaigaHills->id;
				}
				else if (k == Biome::plains->id)
				{
					if (nextRandom(3) == 0)
						i1 = Biome::forestHills->id;
					else
						i1 = Biome::forest->id;
				}
				else if (k == Biome::iceFlats->id)
				{
					i1 = Biome::iceMountains->id;
				}
				else if (k == Biome::jungle->id)
				{
					i1 = Biome::jungleHills->id;
				}
				else if (k == Biome::ocean->id)
				{
					// java: ocean → deepOcean
					i1 = Biome::deepOcean->id;
				}
				else if (k == Biome::extremeHills->id)
				{
					i1 = Biome::smallerExtremeHills->id;
				}
				else if (k == Biome::savanna->id)
				{
					// Java: savanna → savannaPlateau
					
					i1 = Biome::savannaPlateau->id;
				}
				else if (k == Biome::deepOcean->id && nextRandom(3) == 0)
				{
					
					i1 = (nextRandom(2) == 0) ? Biome::plains->id : Biome::forest->id;
				}

				

				if (i1 == k)
				{
					result[x + y * w] = k;
				}
				else
				{
					
					int _n = b[x + 1 + (y + 1 - 1) * (w + 2)];
					int _e = b[x + 1 + 1 + (y + 1) * (w + 2)];
					int _w = b[x + 1 - 1 + (y + 1) * (w + 2)];
					int _s = b[x + 1 + (y + 1 + 1) * (w + 2)];

					int neighbours = 0;
					if (biomesEqualOrMesaPlateau(_n, k)) ++neighbours;
					if (biomesEqualOrMesaPlateau(_e, k)) ++neighbours;
					if (biomesEqualOrMesaPlateau(_w, k)) ++neighbours;
					if (biomesEqualOrMesaPlateau(_s, k)) ++neighbours;

					result[x + y * w] = (neighbours >= 3) ? i1 : k;
				}
			}
		}
	}

	return result;
}