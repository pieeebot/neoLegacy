#include "stdafx.h"
#include "net.minecraft.world.level.biome.h"
#include "net.minecraft.world.level.newbiome.layer.h"
#include "net.minecraft.world.level.h"
#include "BiomeInitLayer.h"
#include "Biome.h"

struct CustomizableSourceSettings {
    char pad[100];
    int biome;
};

BiomeInitLayer::BiomeInitLayer(int64_t seed, shared_ptr<Layer> parent, int64_t seedMixup, LevelType *levelType, void* superflatConfig) : Layer(seedMixup)
{
    this->parent = parent;
    this->customSettings = nullptr;
     desertBiomes = BiomeArray(6);
     warmBiomes = BiomeArray(6);
     coolBiomes = BiomeArray(4);
     icyBiomes = BiomeArray(4);
    if (levelType == LevelType::lvl_normal_1_1)
    {
        
         desertBiomes[0] = Biome::desert;
		desertBiomes[1] = Biome::forest;
		desertBiomes[2] = Biome::extremeHills;
		desertBiomes[3] = Biome::swampland;
		desertBiomes[4] = Biome::plains;
		desertBiomes[5] = Biome::taiga;
        
    }
    else
    {
       
        desertBiomes[0] = Biome::desert;
		desertBiomes[1] = Biome::desert;
		desertBiomes[2] = Biome::desert;
		desertBiomes[3] = Biome::savanna;
		desertBiomes[4] = Biome::savanna;
		desertBiomes[5] = Biome::plains;
    }

        warmBiomes[0] = Biome::forest;
		warmBiomes[1] = Biome::roofedForest;
		warmBiomes[2] = Biome::extremeHills;
		warmBiomes[3] = Biome::plains;
		warmBiomes[4] = Biome::birchForest;
		warmBiomes[5] = Biome::swampland;
		

        
		coolBiomes[0] = Biome::forest;
		coolBiomes[1] = Biome::extremeHills;
		coolBiomes[2] = Biome::taiga;
		coolBiomes[3] = Biome::plains;
	    
		icyBiomes[0] = Biome::iceFlats;
		icyBiomes[1] = Biome::iceFlats;
		icyBiomes[2] = Biome::iceFlats;
		icyBiomes[3] = Biome::coldTaiga;


    if (levelType != LevelType::lvl_normal_1_1 && levelType == LevelType::lvl_customized)
    {
        this->customSettings = (CustomizableSourceSettings*)superflatConfig; 
    }
}

BiomeInitLayer::~BiomeInitLayer()
{
    
	delete [] desertBiomes.data;
	delete [] warmBiomes.data;
	delete [] coolBiomes.data;
	delete [] icyBiomes.data;
}

intArray BiomeInitLayer::getArea(int xo, int yo, int w, int h)
{
    intArray b = parent->getArea(xo, yo, w, h);
    intArray result = IntCache::allocate(w * h);

    if (h > 0)
    {
        for (int y = 0; y < h; y++)
        {
            if (w > 0)
            {
                for (int x = 0; x < w; x++)
                {
                    initRandom(x + xo, y + yo);

                    int val = b[x + y * w];
                    int v18 = (val >> 8) & 0xF;
                    int v19 = val & 0xFFFFF0FF;

                    if (customSettings && customSettings->biome >= 0)
                    {
                        result[x + y * w] = customSettings->biome;
                        continue;
                    }

                    if (isOcean(v19) || v19 == Biome::mushroomIsland->id)
                    {
                        result[x + y * w] = v19;
                        continue;
                    }

                    if (v19 == 1)
                    {
                        if (v18 <= 0)
                        {
                            result[x + y * w] = desertBiomes[nextRandom(desertBiomes.length)]->id;
                        }
                        else if (nextRandom(3))
                        {
                            result[x + y * w] = Biome::mesaPlateauF->id;
                        }
                        else
                        {
                            result[x + y * w] = Biome::plains->id;
                        }
                    }
                    else if (v19 == 2)
                    {
                        if (v18 <= 0)
                        {
                            result[x + y * w] = warmBiomes[nextRandom(warmBiomes.length)]->id;
                        }
                        else
                        {
                            result[x + y * w] = Biome::jungle->id;
                        }
                    }
                    else if (v19 == 3)
                    {
                        if (v18 <= 0)
                        {
                            result[x + y * w] = coolBiomes[nextRandom(coolBiomes.length)]->id;
                        }
                        else
                        {
                            result[x + y * w] = Biome::megaTaiga->id;
                        }
                    }
                    else if (v19 == 4)
                    {
                        result[x + y * w] = icyBiomes[nextRandom(icyBiomes.length)]->id;
                    }
                    else
                    {
                        result[x + y * w] = v19; 
                    }
                }
            }
        }
    }

    return result;
}