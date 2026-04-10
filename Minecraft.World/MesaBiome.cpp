#include "stdafx.h"


#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>

#include "MesaBiome.h"
#include "BiomeDecorator.h"         
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.level.tile.h"
#include "ColoredTile.h"
#include "SandTile.h"
#include "DirtTile.h"
#include "DyePowderItem.h"
#include "PerlinNoise.h"
#include "Random.h"


#undef max
#undef min


MesaBiome::MesaBiome(int id, bool mesaPlateau, bool hasTrees) : Biome(id)
{
    this->isMesaPlateau = mesaPlateau;
    this->hasTrees      = hasTrees;

    this->setNoRain();
    this->setTemperatureAndDownfall(2.0f, 0.0f);

    // Red Sand  (id=12, data=1) 
    // Orange Stained Hardened Clay (id=159, data=1) 
    this->topMaterial     = static_cast<byte>(Tile::sand_Id);
    this->topMaterialData = static_cast<byte>(SandTile::RED_SAND); // 1
    this->material        = static_cast<byte>(Tile::clayHardened_colored_Id);
    this->materialData    = static_cast<byte>(BAND_ORANGE);        // 1

    this->lastSeed             = INVALID_SEED;
    this->pillarNoise          = nullptr;
    this->pillarRoofNoise      = nullptr;
    this->clayBandsOffsetNoise = nullptr;

   
    /*friendlies.clear();
    friendlies_chicken.clear();
    enemies.clear();
    waterFriendlies.clear();
    ambientFriendlies.clear();*/

    
    if (decorator)
    {
        decorator->treeCount = hasTrees ? 5 : -999;
        decorator->deadBushCount = 20;
        decorator->reedsCount     = 3;
        decorator->cactusCount   = 5;
        decorator->flowerCount   = 0;
    }
}

MesaBiome::~MesaBiome()
{
    delete pillarNoise;
    delete pillarRoofNoise;
    delete clayBandsOffsetNoise;
}


void MesaBiome::initBands(int64_t /*seed*/) {}




int MesaBiome::getBandColor(int x, int y, int z)
{
    if (!clayBandsOffsetNoise)
        return BAND_ORANGE;

    double noiseX = static_cast<double>(x) / 512.0;
    int offset    = static_cast<int>(std::round(
        clayBandsOffsetNoise->getValue(noiseX, noiseX) * 2.0));

    int index = ((y + offset) % 64 + 64) % 64;
    return static_cast<int>(static_cast<unsigned char>(clayBands[index]));
}



void MesaBiome::decorate(Level* level, Random* random, int xo, int zo)
{
    Biome::decorate(level, random, xo, zo);
}

Feature* MesaBiome::getTreeFeature(Random* random)
{
    return Biome::getTreeFeature(random);
}



void MesaBiome::buildSurfaceAtDefault(Level* level, Random* random,
                                      byte* chunkBlocks, byte* chunkData,
                                      int x, int z, double noiseVal)
{
   
    int64_t seed = level->getSeed();
    if (lastSeed != seed)
    {
        lastSeed = seed;

       
        std::fill(std::begin(clayBands), std::end(clayBands),
                  static_cast<byte>(BAND_HARDENED_CLAY));
        {
            Random r(seed);
            delete clayBandsOffsetNoise;
            clayBandsOffsetNoise = new PerlinNoise(&r, 1);

            // Orange sparse
            for (int i = 0; i < 64; )
            {
                i += r.nextInt(5) + 1;
                if (i < 64) clayBands[i] = static_cast<byte>(BAND_ORANGE);
            }
            // Yellow groups
            int yg = r.nextInt(4) + 2;
            for (int g = 0; g < yg; ++g) {
                int t = r.nextInt(3) + 1, s = r.nextInt(64);
                for (int k = 0; s + k < 64 && k < t; ++k)
                    clayBands[s + k] = static_cast<byte>(BAND_YELLOW);
            }
            // Brown groups
            int bg = r.nextInt(4) + 2;
            for (int g = 0; g < bg; ++g) {
                int t = r.nextInt(3) + 2, s = r.nextInt(64);
                for (int k = 0; s + k < 64 && k < t; ++k)
                    clayBands[s + k] = static_cast<byte>(BAND_BROWN);
            }
            // Red groups
            int rg = r.nextInt(4) + 2;
            for (int g = 0; g < rg; ++g) {
                int t = r.nextInt(3) + 1, s = r.nextInt(64);
                for (int k = 0; s + k < 64 && k < t; ++k)
                    clayBands[s + k] = static_cast<byte>(BAND_RED);
            }
            // White stripes
            int ws = r.nextInt(3) + 3, cursor = 0;
            for (int g = 0; g < ws; ++g) {
                cursor += r.nextInt(16) + 4;
                if (cursor >= 64) break;
                clayBands[cursor] = static_cast<byte>(BAND_WHITE);
                if (cursor > 1  && r.nextBoolean()) clayBands[cursor - 1] = static_cast<byte>(BAND_SILVER);
                if (cursor < 63 && r.nextBoolean()) clayBands[cursor + 1] = static_cast<byte>(BAND_SILVER);
            }
        }

        
        delete pillarNoise;     pillarNoise     = nullptr;
        delete pillarRoofNoise; pillarRoofNoise = nullptr;
        {
            Random r(seed);
            pillarNoise     = new PerlinNoise(&r, 4); // field_150623_aE
            pillarRoofNoise = new PerlinNoise(&r, 1); // field_150624_aF
        }
    }

   
    const int seaLevel  = level->seaLevel;   
    const int localX    = x & 15;
    const int localZ    = z & 15;

   
    int  noiseDepth = (int)(noiseVal / 3.0 + 3.0 + random->nextDouble() * 0.25);
    bool flag       = (cos(noiseVal / 3.0 * PI) > 0.0);

    int  run   = -1;
    bool flag1 = false; 

    for (int y = Level::genDepthMinusOne; y >= 0; --y)
    {
        int index = (localX * 16 + localZ) * Level::genDepth + y;  

        
        if (y <= 1 + random->nextInt(2))
        {
            chunkBlocks[index] = static_cast<byte>(Tile::unbreakable_Id);
            continue;
        }

        byte cur = chunkBlocks[index];

        if (cur == 0) 
        {
            run = -1;
        }
        else if (cur == static_cast<byte>(Tile::stone_Id))
        {
            if (run == -1) 
            {
                flag1 = false;

                
                run = noiseDepth + (y > seaLevel ? (y - seaLevel) : 0);

                
                if (y < seaLevel - 1)
                {
                    if (noiseDepth <= 0)
                    {
                        chunkBlocks[index] = static_cast<byte>(Tile::stone_Id);
                    }
                    else
                    {
                        chunkBlocks[index] = static_cast<byte>(Tile::clayHardened_colored_Id);
                        chunkData[index]   = static_cast<byte>(BAND_ORANGE);
                    }
                }
                else if (hasTrees && y > 86 + noiseDepth * 2)
                {
                    if (flag)
                    {
                        chunkBlocks[index] = static_cast<byte>(Tile::grass_Id);
                    }
                    else
                    {
                        chunkBlocks[index] = static_cast<byte>(Tile::dirt_Id);
                        chunkData[index]   = static_cast<byte>(DirtTile::COARSE_DIRT);
                    }
                }
                else if (y <= seaLevel + 3 + noiseDepth)
                {
                    chunkBlocks[index] = static_cast<byte>(Tile::sand_Id);
                    chunkData[index]   = static_cast<byte>(SandTile::RED_SAND); // 1
                    flag1 = true;
                }
                else
                {
                    if (y >= 64)
                    {
                        if (flag)
                        {
                            chunkBlocks[index] = static_cast<byte>(Tile::clayHardened_Id);
                        }
                        else
                        {
                            int band = getBandColor(x, y, z);
                            if (band == BAND_HARDENED_CLAY)
                            {
                                chunkBlocks[index] = static_cast<byte>(Tile::clayHardened_Id);
                            }
                            else
                            {
                                chunkBlocks[index] = static_cast<byte>(Tile::clayHardened_colored_Id);
                                chunkData[index]   = static_cast<byte>(band);
                            }
                        }
                    }
                    else
                    {
                        chunkBlocks[index] = static_cast<byte>(Tile::clayHardened_colored_Id);
                        chunkData[index]   = static_cast<byte>(BAND_ORANGE);
                    }
                }

                if (y < seaLevel && chunkBlocks[index] == 0)
                {
                    chunkBlocks[index] = (getTemperature(x, y, z) < 0.15f)
                        ? static_cast<byte>(Tile::ice_Id)
                        : static_cast<byte>(Tile::calmWater_Id);
                }
            }
            else if (run > 0)
            {
                --run;

                if (flag1)
                {
                    chunkBlocks[index] = static_cast<byte>(Tile::clayHardened_colored_Id);
                    chunkData[index]   = static_cast<byte>(BAND_ORANGE);
                }
                else
                {
                    int band = getBandColor(x, y, z);
                    if (band == BAND_HARDENED_CLAY)
                        chunkBlocks[index] = static_cast<byte>(Tile::clayHardened_Id);
                    else
                    {
                        chunkBlocks[index] = static_cast<byte>(Tile::clayHardened_colored_Id);
                        chunkData[index]   = static_cast<byte>(band);
                    }
                }
            }
        }
    }
}