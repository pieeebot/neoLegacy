#include "stdafx.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.level.biome.h"
#include "SwampTreeFeature.h"
#include "net.minecraft.world.level.tile.h"
#include "PerlinNoise.h"

SwampBiome::SwampBiome(int id) : Biome(id)
{
	decorator->treeCount = 2;
	decorator->flowerCount = 1;
	decorator->deadBushCount = 1;
	decorator->mushroomCount = 8;
	decorator->reedsCount = 10;
	decorator->clayCount = 1;
	decorator->waterlilyCount = 4;
	decorator->sandCount = 0;
	decorator->grassCount = 5;
	
	enemies.push_back(new MobSpawnerData(eTYPE_SLIME, 1, 1, 1));
}


Feature *SwampBiome::getTreeFeature(Random *random)
{
	return new SwampTreeFeature(); 
}

void SwampBiome::buildSurfaceAtDefault(Level *level, Random *random, byte* chunkBlocks, int x, int z, double noiseVal)
{
    double d0 = GRASS_COLOR_NOISE->getValue(x * 0.25, z * 0.25);

    if (d0 > 0.0)
    {
        int localX = x & 15;
        int localZ = z & 15;

        for (int y = Level::genDepthMinusOne; y >= 0; --y)
        {
            int index = (localZ * 16 + localX) * Level::genDepth + y;
            if (chunkBlocks[index] != 0) 
            {
                if (y == 62 && chunkBlocks[index] != static_cast<byte>(Tile::water_Id))
                {
                    chunkBlocks[index] = static_cast<byte>(Tile::water_Id);
                    if (d0 < 0.12)
                    {
                        chunkBlocks[index + 1] = static_cast<byte>(Tile::waterLily_Id);
                    }
                }
                break;
            }
        }
    }

    Biome::buildSurfaceAtDefault(level, random, chunkBlocks, x, z, noiseVal);
}

// 4J Stu - Not using these any more
//int SwampBiome::getGrassColor()
//{
//    double temp = getTemperature();
//    double rain = getDownfall();
//
//    return ((GrassColor::get(temp, rain) & 0xfefefe) + 0x4e0e4e) / 2;
//}
//
//int SwampBiome::getFolageColor()
//{
//    double temp = getTemperature();
//    double rain = getDownfall();
//
//    return ((FoliageColor::get(temp, rain) & 0xfefefe) + 0x4e0e4e) / 2;
//}