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
#include "PerlinSimplexNoise.h"
#include "Random.h"
#include "TreeFeature.h"

#undef max
#undef min

MesaBiome::MesaBiome(int id, bool mesaPlateau, bool hasTrees) : Biome(id)
{
    this->isMesaPlateau = mesaPlateau;
    this->hasTrees      = hasTrees;

    this->setNoRain();
    this->setTemperatureAndDownfall(2.0f, 0.0f);

    this->topMaterial     = static_cast<byte>(Tile::sand_Id);
    this->topMaterialData = static_cast<byte>(SandTile::RED_SAND);
    this->material        = static_cast<byte>(Tile::clayHardened_colored_Id);
    this->materialData    = static_cast<byte>(orangeColoredClayState);

    this->lastSeed             = INVALID_SEED;
    this->clayBands            = nullptr;
    this->pillarNoise          = nullptr;
    this->pillarRoofNoise      = nullptr;
    this->clayBandsOffsetNoise = nullptr;

    if (decorator)
    {
        decorator->treeCount     = hasTrees ? 5 : -999;
        decorator->deadBushCount = 20;
        decorator->reedsCount    = 3;
        decorator->cactusCount   = 5;
        decorator->flowerCount   = 0;
    }
}

MesaBiome::~MesaBiome()
{
    delete[] clayBands;
    clayBands = nullptr;

    delete pillarNoise;
    pillarNoise = nullptr;
    delete pillarRoofNoise;
    pillarRoofNoise = nullptr;
    delete clayBandsOffsetNoise;
    clayBandsOffsetNoise = nullptr;
}

void MesaBiome::generateBands(int64_t seed)
{

    delete[] clayBands;
    clayBands = new BandEntry[BAND_COUNT];

    for (int i = 0; i < BAND_COUNT; ++i)
    {
        clayBands[i].blockId   = Tile::clayHardened_Id;
        clayBands[i].blockData = defaultHardenedClayState;
    }

    Random r(seed);


    delete clayBandsOffsetNoise;
    clayBandsOffsetNoise = new PerlinSimplexNoise(&r, 1);

    {
        int i = 0;
        while (i < BAND_COUNT)
        {
            i += r.nextInt(5) + 1;
            if (i < BAND_COUNT)
            {
                clayBands[i].blockId   = Tile::clayHardened_colored_Id;
                clayBands[i].blockData = orangeColoredClayState;
            }
        }
    }

    {
        int groupCount = r.nextInt(4) + 2;
        for (int g = 0; g < groupCount; ++g)
        {
            int len   = r.nextInt(3) + 1;
            int start = r.nextInt(BAND_COUNT);
            for (int k = 0; start + k < BAND_COUNT && k < len; ++k)
            {
                clayBands[start + k].blockId   = Tile::clayHardened_colored_Id;
                clayBands[start + k].blockData = yellowColoredClayState;
            }
        }
    }

    {
        int groupCount = r.nextInt(4) + 2;
        for (int g = 0; g < groupCount; ++g)
        {
            int len   = r.nextInt(3) + 2;
            int start = r.nextInt(BAND_COUNT);
            for (int k = 0; start + k < BAND_COUNT && k < len; ++k)
            {
                clayBands[start + k].blockId   = Tile::clayHardened_colored_Id;
                clayBands[start + k].blockData = brownColoredClayState;
            }
        }
    }

    {
        int groupCount = r.nextInt(4) + 2;
        for (int g = 0; g < groupCount; ++g)
        {
            int len   = r.nextInt(3) + 1;
            int start = r.nextInt(BAND_COUNT);
            for (int k = 0; start + k < BAND_COUNT && k < len; ++k)
            {
                clayBands[start + k].blockId   = Tile::clayHardened_colored_Id;
                clayBands[start + k].blockData = redColoredClayState;
            }
        }
    }

    {
        int stripeCount = r.nextInt(3) + 3;
        int cursor      = 0;
        for (int g = 0; g < stripeCount; ++g)
        {
            cursor += r.nextInt(16) + 4;
            if (cursor >= BAND_COUNT) break;

            clayBands[cursor].blockId   = Tile::clayHardened_colored_Id;
            clayBands[cursor].blockData = whiteColoredClayState;

            
            if (cursor > 1 && r.nextBoolean())
            {
                clayBands[cursor - 1].blockId   = Tile::clayHardened_colored_Id;
                clayBands[cursor - 1].blockData = silverColoredClayState;
            }
            
            if (cursor < 63 && r.nextBoolean())
            {
                clayBands[cursor + 1].blockId   = Tile::clayHardened_colored_Id;
                clayBands[cursor + 1].blockData = silverColoredClayState;
            }
        }
    }
}


BandEntry MesaBiome::getBand(int x, int y, int z)
{
    if (!clayBandsOffsetNoise || !clayBands)
        return { Tile::clayHardened_Id, 0 };

    
    double noiseVal = clayBandsOffsetNoise->getValue(
        static_cast<double>(x) * 0.001953125,
        static_cast<double>(x) * 0.001953125);

    
    int offset = static_cast<int>(std::round(noiseVal + noiseVal));

    
    int index = ((y + offset) + BAND_COUNT) % BAND_COUNT;
    return clayBands[index];
}

void MesaBiome::decorate(Level* level, Random* random, int xo, int zo)
{
    Biome::decorate(level, random, xo, zo);
}

Feature* MesaBiome::getTreeFeature(Random* random)
{
    return new TreeFeature(false);
}

int MesaBiome::getFolageColor() const
{
    return eMinecraftColour_Foliage_Mesa;
}

int MesaBiome::getGrassColor() const
{
    return eMinecraftColour_Grass_Mesa;
}

void MesaBiome::buildSurfaceAtDefault(Level* level, Random* random,
                                      byte* chunkBlocks, byte* chunkData,
                                      int x, int z, double noiseVal)
{
    int64_t seed = level->getSeed();

    if (!clayBands || lastSeed != seed)
        generateBands(seed);

    if (!pillarNoise || !pillarRoofNoise || lastSeed != seed)
    {
        Random noiseRand(seed);
        delete pillarNoise;
        delete pillarRoofNoise;
        
        pillarNoise     = new PerlinSimplexNoise(&noiseRand, 4);
        pillarRoofNoise = new PerlinSimplexNoise(&noiseRand, 1);
    }

    lastSeed = seed;

    const int localX = x & 0xF;
    const int localZ = z & 0xF;

    double pillarHeight = 0.0;

    
    if (isMesaPlateau)
{
    double nx = static_cast<double>((x & ~0xF) + localZ);
    double nz = static_cast<double>((z & ~0xF) + localX);

    
    double roofVal = pillarNoise->getValue(nx * 0.25, nz * 0.25);
    double d0      = std::abs(noiseVal) - roofVal;

    
    if (d0 > 0.0)
    {
        
        double pillarScale = pillarRoofNoise->getValue(
            nx * 0.001953125,
            nz * 0.001953125);

        
        double scaled = std::ceil(std::abs(pillarScale) * 50.0);
        double height = d0 * d0 * 2.5;
        double cap    = scaled + 14.0;
        if (height > cap) height = cap;
        pillarHeight = height + 64.0;
    }
}

    const int localX2 = x & 0xF;
    const int localZ2 = z & 0xF;
    const int seaLevel = level->getSeaLevel();

    int  noiseDepth  = static_cast<int>(noiseVal / 3.0 + 3.0 + random->nextDouble() * 0.25);
    bool cosFlag     = (std::cos(noiseVal / 3.0 * PI) > 0.0);

    int  run          = -1;
    bool underRedSand = false;

    for (int y = 0x7F; y >= 0; --y)
    {
        int idx = (localZ2 * 16 + localX2) * 128 + y;

        
        byte cur = chunkBlocks[idx];
        if (cur == 0 && pillarHeight > 0.0 && y < static_cast<int>(pillarHeight))
        {
            chunkBlocks[idx] = static_cast<byte>(Tile::stone_Id);
            cur = static_cast<byte>(Tile::stone_Id);
        }

        
        if (y <= random->nextInt(5))
        {
            chunkBlocks[idx] = static_cast<byte>(Tile::unbreakable_Id);
            continue;
        }

        
        cur = chunkBlocks[idx];

       
        if (cur == 0)
        {
            run = -1;
            continue;
        }

       
        if (cur != static_cast<byte>(Tile::stone_Id))
            continue;

        if (run == -1)
        {
            underRedSand = false;
            run = noiseDepth + std::max(0, y - seaLevel);

            
            if (y < seaLevel - 1)
            {
                if (noiseDepth <= 0)
                {
                    chunkBlocks[idx] = static_cast<byte>(Tile::stone_Id);
                }
                else
                {
                    chunkBlocks[idx] = static_cast<byte>(Tile::clayHardened_colored_Id);
                    chunkData[idx]   = static_cast<byte>(BAND_ORANGE);
                }
            }
            else if (hasTrees && y > 86 + noiseDepth * 2)
            {
                
                if (cosFlag)
                {
                    chunkBlocks[idx] = static_cast<byte>(Tile::dirt_Id);
                    chunkData[idx]   = static_cast<byte>(DirtTile::COARSE_DIRT);
                }
                else
                {
                    chunkBlocks[idx] = static_cast<byte>(Tile::grass_Id);
                }
            }
            else if (y <= seaLevel + 3 + noiseDepth)
            {
                chunkBlocks[idx] = static_cast<byte>(Tile::sand_Id);
                chunkData[idx]   = static_cast<byte>(SandTile::RED_SAND);
                underRedSand     = true;
            }
            else
            {
                
                if (y >= 64 && y <= 127)
                {
                    if (cosFlag)
                    {
                        chunkBlocks[idx] = static_cast<byte>(Tile::clayHardened_Id);
                    }
                    else
                    {
                        BandEntry band = getBand(x, y, z);
                        chunkBlocks[idx] = static_cast<byte>(band.blockId);
                        if (band.blockId == Tile::clayHardened_colored_Id)
                            chunkData[idx] = static_cast<byte>(band.blockData);
                    }
                }
                else
                {
                    chunkBlocks[idx] = static_cast<byte>(Tile::clayHardened_colored_Id);
                    chunkData[idx]   = static_cast<byte>(BAND_ORANGE);
                }
            }

            
            if (y < seaLevel && chunkBlocks[idx] == 0)
                chunkBlocks[idx] = static_cast<byte>(Tile::calmWater_Id);
        }
        else if (run > 0)
        {
            --run;

            if (underRedSand)
            {
                chunkBlocks[idx] = static_cast<byte>(Tile::clayHardened_colored_Id);
                chunkData[idx]   = static_cast<byte>(BAND_ORANGE);
            }
            else
            {
                BandEntry band = getBand(x, y, z);
                chunkBlocks[idx] = static_cast<byte>(band.blockId);
                if (band.blockId == Tile::clayHardened_colored_Id)
                    chunkData[idx] = static_cast<byte>(band.blockData);
            }
        }
    }
}

MesaBiome* MesaBiome::createMutatedCopy(int newId)
{

    bool isMesaBase = (this->id == Biome::mesa->id);

    MesaBiome* copy = new MesaBiome(newId, isMesaBase, this->hasTrees);

    if (!isMesaBase)
    {
        
        copy->setDepthAndScale(0.1f, 0.3f);
    }

    copy->setWaterSkyColor(this->getWaterColor(), this->getSkyColor());

    return copy;
}