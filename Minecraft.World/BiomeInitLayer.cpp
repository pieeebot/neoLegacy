#include "stdafx.h"
#include "net.minecraft.world.level.biome.h"
#include "net.minecraft.world.level.newbiome.layer.h"
#include "net.minecraft.world.level.h"
#include "BiomeInitLayer.h"

BiomeInitLayer::BiomeInitLayer(int64_t seed, shared_ptr<Layer> parent, LevelType *levelType) : Layer(seed)
{
	this->parent = parent;
	bLegacy1_1 = (levelType == LevelType::lvl_normal_1_1);

	if (bLegacy1_1)
	{
		// 1.1 mode: flat list
		startBiomes = BiomeArray(6);
		startBiomes[0] = Biome::desert;
		startBiomes[1] = Biome::forest;
		startBiomes[2] = Biome::extremeHills;
		startBiomes[3] = Biome::swampland;
		startBiomes[4] = Biome::plains;
		startBiomes[5] = Biome::taiga;
	}
	else
	{
		
		//
		// RareBiomeLayer encodes a flag by setting k = plains->id + 128
		
		// When rareBit is set and we would pick plains -> pick sunflowersPlains 

		// desert biomes (Java: desert 30, savanna 20, plains 10  -> total 60)
		desertBiomes = BiomeArray(6);
		desertBiomes[0] = Biome::desert;
		desertBiomes[1] = Biome::desert;
		desertBiomes[2] = Biome::desert;
		desertBiomes[3] = Biome::savanna;
		desertBiomes[4] = Biome::savanna;
		desertBiomes[5] = Biome::plains;

		// warm biomes (Java: forest, swamp, jungle, roofedForest, birchForest, plains...)
		warmBiomes = BiomeArray(7);
		warmBiomes[0] = Biome::forest;
		warmBiomes[1] = Biome::swampland;
		warmBiomes[2] = Biome::jungle;
		warmBiomes[3] = Biome::roofedForest;
		warmBiomes[4] = Biome::birchForest;
		warmBiomes[5] = Biome::plains;
		warmBiomes[6] = Biome::mesaPlateauF;

		// cool (Java: extremeHills, taiga, megaTaiga, forest, birchForest)
		coolBiomes = BiomeArray(5);
		coolBiomes[0] = Biome::extremeHills;
		coolBiomes[1] = Biome::taiga;
		coolBiomes[2] = Biome::megaTaiga;
		coolBiomes[3] = Biome::forest;
		coolBiomes[4] = Biome::birchForest;

		// icy (Java: iceFlats, coldTaiga)
		icyBiomes = BiomeArray(3);
		icyBiomes[0] = Biome::iceFlats;
		icyBiomes[1] = Biome::iceFlats;
		icyBiomes[2] = Biome::coldTaiga;
	}
}

BiomeInitLayer::~BiomeInitLayer()
{
	delete [] startBiomes.data;
	delete [] desertBiomes.data;
	delete [] warmBiomes.data;
	delete [] coolBiomes.data;
	delete [] icyBiomes.data;
}

intArray BiomeInitLayer::getArea(int xo, int yo, int w, int h)
{
	intArray b = parent->getArea(xo, yo, w, h);

	intArray result = IntCache::allocate(w * h);
	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			initRandom(x + xo, y + yo);
			int k = b[x + y * w];

			// flat list
			if (bLegacy1_1)
			{
				if (k == 0)
				{
					result[x + y * w] = 0;
				}
				else if (k == Biome::mushroomIsland->id)
				{
					result[x + y * w] = k;
				}
				else if (k == 1)
				{
					result[x + y * w] = startBiomes[nextRandom(startBiomes.length)]->id;
				}
				else // icy / cold
				{
					int picked = startBiomes[nextRandom(startBiomes.length)]->id;
					if (picked == Biome::taiga->id || picked == Biome::coldTaiga->id ||
					    picked == Biome::megaTaiga->id || picked == Biome::iceSpikes->id)
					{
						result[x + y * w] = picked;
					}
					else
					{
						result[x + y * w] = Biome::iceFlats->id;
					}
				}
				continue;
			}

			

			
			// RareBiomeLayer sets k = plains->id + 128 when it picks a rare slot.
			// plains->id = 1, so a rare plains = 129. We extract the high bits flag.
			int rareBit = (k & 3840) >> 8;   // Java: (k & 0xF00) >> 8
			k = k & ~3840;      

			
			if (k == 0 || k == Biome::ocean->id || k == Biome::deepOcean->id ||
			    k == Biome::frozenOcean->id || k == Biome::frozenRiver->id)
			{
				result[x + y * w] = k;
			}
			
			else if (k == Biome::mushroomIsland->id || k == Biome::mushroomIslandShore->id)
			{
				result[x + y * w] = k;
			}
			// Climate 1 & 2 & 3
			else if (k == 1 || k == 2 || k == 3)
			{
				if (rareBit > 0)
				{
					// If rare: pick from high-value variants
					int r = nextRandom(3);
					if (r == 0) result[x + y * w] = Biome::jungle->id;
					else if (r == 1) result[x + y * w] = Biome::megaTaiga->id;
					else result[x + y * w] = Biome::desertHills->id;
				}
				else
				{
					
					int r = nextRandom(20);
					if (r < 6) result[x + y * w] = desertBiomes[nextRandom(desertBiomes.length)]->id;
					else if (r < 13) result[x + y * w] = warmBiomes[nextRandom(warmBiomes.length)]->id;
					else result[x + y * w] = coolBiomes[nextRandom(coolBiomes.length)]->id;
				}
			}
			// Climate 4 
			else if (k == 4 || k == Biome::iceFlats->id)
			{
				result[x + y * w] = icyBiomes[nextRandom(icyBiomes.length)]->id;
			}
			// Rare variant from RareBiomeLayer
			else if (k == Biome::plains->id + 128)
			{
				result[x + y * w] = Biome::sunflowersPlains->id;
			}
			else
			{
				
				result[x + y * w] = k;
			}
		}
	}

	return result;
}