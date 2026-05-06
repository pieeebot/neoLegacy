#include "stdafx.h"
#include "ForestBiome.h"
#include "BiomeDecorator.h"
#include "TreeFeature.h"
#include "BirchFeature.h"
#include "FlowerFeature.h"
#include "RoofTreeFeature.h"
#include "Tile.h"
#include "Rose.h"
#include "HugeMushroomFeature.h"
#include "DoublePlantFeature.h"
#include "TallGrass2.h"
#include "../Minecraft.Client/Minecraft.h"
#include "../Minecraft.Client/Common/Colours/ColourTable.h"
#include "Level.h"
#include "Random.h"
#include "BlockPos.h"
#include "PerlinNoise.h"

static const int LEAF_COLOR_NORMAL = 0x4EBA31;
static const int LEAF_COLOR_MUTATED = 0x7DA225;
static const int WATER_COLOR_NORMAL = 0x307444;
static const int SKY_COLOR_NORMAL = 0x56621;
static const int SKY_COLOR_BIRCH_ALT = 0x23310;
static const float TEMP_NORMAL = 0.7f;
static const float DOWNFALL_NORMAL = 0.8f;
static const float TEMP_BIRCH = 0.6f;
static const float DOWNFALL_BIRCH = 0.6f;

ForestBiome::ForestBiome(int id, int type)
    : Biome(id), biomeType(type)
{
    
    setLeafColor((eMinecraftColour)LEAF_COLOR_NORMAL);
    setTemperatureAndDownfall(TEMP_NORMAL, DOWNFALL_NORMAL);

    if (type == 1) // flower forest
    {
        decorator->treeCount = 6;
        decorator->grassCount = 1;
        decorator->flowerCount = 100;
        friendlies.push_back(new MobSpawnerData(eTYPE_RABBIT, 4, 2, 3));
    }
    else
    {
        decorator->grassCount = 2;
        decorator->treeCount = 10;
    }

    if (type == 2) // birch forest
    {
        m_waterColor = (eMinecraftColour)WATER_COLOR_NORMAL;
        m_skyColor = (eMinecraftColour)SKY_COLOR_NORMAL;
        setTemperatureAndDownfall(TEMP_BIRCH, DOWNFALL_BIRCH);
    }
    else if (type == 0) // normale
    {
        enemies.push_back(new MobSpawnerData(eTYPE_WOLF, 5, 4, 4));
    }
    else if (type == 3) // roofed forest
    {
        decorator->hugeMushrooms = 2;
    }
}

ForestBiome::~ForestBiome() {}

Feature* ForestBiome::getTreeFeature(Random* random)
{
    if (biomeType == 3)
    {
        if (random->nextInt(3) > 0)
            return new RoofTreeFeature(false);
    }
    if (biomeType == 2 || random->nextInt(5) == 0)
        return new BirchFeature(false, false);
    return new TreeFeature(false);
}

Biome* ForestBiome::setColor(int color, bool b)
{
    if (biomeType != 2)
        return Biome::setColor(color, b);

    m_waterColor = (eMinecraftColour)color;
    m_skyColor = (eMinecraftColour)(b ? SKY_COLOR_BIRCH_ALT : SKY_COLOR_NORMAL);
    return this;
}

int ForestBiome::getRandomFlower(Random* rand, const BlockPos& pos) const
{
    if (biomeType == 1)
    {
        double noise = m_temperatureNoise->getValue(pos.getX() / 48.0, pos.getZ() / 48.0);
        double t = (noise + 1.0) * 0.5;
        if (t < 0.0) t = 0.0;
        if (t > 0.9999) t = 0.9999;
        int idx = (int)(t * 10.0);
        
        static const int FLOWER_COLORS[10] = { 0,1,2,3,4,5,6,7,8,9 };
        int color = FLOWER_COLORS[idx];
        return color;
    }
    return 0; 
}

void ForestBiome::decorate(Level* level, Random* rand, int xo, int zo)
{
    BlockPos pos(xo, 0, zo);

    if (biomeType == 3)
    {
        for (int i = 0; i < 4; ++i)
        {
            int baseX = 9 + i * 4;
            int y = 13;
            for (int j = 0; j < 2; ++j)
            {
                
                int dx = baseX + rand->nextInt(3);
                int dz = y - 4 + rand->nextInt(3);
                BlockPos treePos = pos.offset(dx, 0, dz);
                
                
                int highestY = level->getHeightmap(treePos.getX(), treePos.getZ());
                treePos = BlockPos(treePos.getX(), highestY, treePos.getZ());

                if (rand->nextInt(20) == 0)
                {
                    HugeMushroomFeature mushroom;
                    mushroom.place(level, rand, treePos.getX(), treePos.getY(), treePos.getZ());
                }
                else
                {
                    Feature* tree = getTreeFeature(rand);
                    if (tree)
                    {
                        tree->place(level, rand, treePos.getX(), treePos.getY(), treePos.getZ());
                        delete tree;
                    }
                }

                
                dx = baseX + rand->nextInt(3);
                dz = y + rand->nextInt(3);
                treePos = pos.offset(dx, 0, dz);
                
                
                highestY = level->getHeightmap(treePos.getX(), treePos.getZ());
                treePos = BlockPos(treePos.getX(), highestY, treePos.getZ());

                if (rand->nextInt(20) == 0)
                {
                    HugeMushroomFeature mushroom;
                    mushroom.place(level, rand, treePos.getX(), treePos.getY(), treePos.getZ());
                }
                else
                {
                    Feature* tree = getTreeFeature(rand);
                    if (tree)
                    {
                        tree->place(level, rand, treePos.getX(), treePos.getY(), treePos.getZ());
                        delete tree;
                    }
                }
                y += 8;
            }
        }
    }

  
    int count = rand->nextInt(5) - 3;
    if (biomeType == 1) count += 2;
    
    if (count > 0)
    {
        do
        {
            int l1 = rand->nextInt(3);
            DoublePlantFeature plantFeature;
            if (l1 == 0) plantFeature.setPlantType(TallGrass2::LILAC);
            else if (l1 == 1) plantFeature.setPlantType(TallGrass2::ROSE_BUSH);
            else if (l1 == 2) plantFeature.setPlantType(TallGrass2::PEONY);

            for (int attempts = 0; attempts < 5; ++attempts)
            {
                int dx = rand->nextInt(16) + 8;
                int dz = rand->nextInt(16) + 8;
                BlockPos plantPos = pos.offset(dx, 0, dz);
                
                
                int highestY = level->getHeightmap(plantPos.getX(), plantPos.getZ());
                plantPos = BlockPos(plantPos.getX(), highestY, plantPos.getZ());
                
                int plantY = rand->nextInt(plantPos.getY() + 32);
                BlockPos finalPos(plantPos.getX(), plantY, plantPos.getZ());
                
                if (plantFeature.place(level, rand, finalPos.getX(), finalPos.getY(), finalPos.getZ()))
                {
                    break;
                }
            }
            --count;
        } while (count > 0);
    }

    Biome::decorate(level, rand, xo, zo);
}

unsigned int ForestBiome::getGrassColor(const BlockPos& pos) const
{
    unsigned int base = Biome::getGrassColor();
    if (biomeType == 3)
    {
        ColourTable* table = Minecraft::GetInstance()->getColourTable();
        if (table)
        {
            unsigned int color2 = table->getColour(eMinecraftColour_Grass_Forest);
            return ((base & 0xFEFEFE) + (color2 & 0xFEFEFE)) >> 1;
        }
    }
    return base;
}

Biome* ForestBiome::createMutatedCopy(int id) const
{
    ForestBiome* copy = new ForestBiome(id, 1);
    copy->setDepthAndScale(depth, scale + 0.2f);
    copy->setName(L"Forest Mutated");
    
   
    copy->setLeafColor((eMinecraftColour)LEAF_COLOR_MUTATED);
    return copy;
}


// MutatedBirchForestBiome
ForestBiome::MutatedBirchForestBiome::MutatedBirchForestBiome(int id, Biome* baseBiome)
    : MutatedBiome(id, baseBiome) {}

ForestBiome::MutatedBirchForestBiome::~MutatedBirchForestBiome() {}

Feature* ForestBiome::MutatedBirchForestBiome::getTreeFeature(Random* random)
{
    if (random->nextBoolean())
        return new BirchFeature(false, true);
    else
        return new BirchFeature(false, false);
}


// MutatedForestBiome
ForestBiome::MutatedForestBiome::MutatedForestBiome(int id, Biome* baseBiome)
    : MutatedBiome(id, baseBiome) {}

ForestBiome::MutatedForestBiome::~MutatedForestBiome() {}

void ForestBiome::MutatedForestBiome::decorate(Level* level, Random* rand, int xo, int zo)
{
    if (m_baseBiome)
        m_baseBiome->decorate(level, rand, xo, zo);
}

Feature* ForestBiome::getFlowerFeature(Random* random, int x, int y, int z)
{
    if (biomeType == 1) // Flower Forest
    {
       
        int fType = random->nextInt(9); 
        switch (fType) {
            case 0: return new FlowerFeature(Tile::yellow_flower_Id, 0); 
            case 1: return new FlowerFeature(Tile::red_flower_Id, 0);   
            case 2: return new FlowerFeature(Tile::red_flower_Id, Rose::ALLIUM); 
            case 3: return new FlowerFeature(Tile::red_flower_Id, Rose::AZURE_BLUET); 
            case 4: return new FlowerFeature(Tile::red_flower_Id, Rose::RED_TULIP); 
            case 5: return new FlowerFeature(Tile::red_flower_Id, Rose::ORANGE_TULIP); 
            case 6: return new FlowerFeature(Tile::red_flower_Id, Rose::WHITE_TULIP);
            case 7: return new FlowerFeature(Tile::red_flower_Id, Rose::PINK_TULIP); 
            case 8: return new FlowerFeature(Tile::red_flower_Id, Rose::OXEYE_DAISY); 
        }
    }
    else if (biomeType == 2 || biomeType == 3) 
    {
        
        return nullptr; 
    }

   
    return Biome::getFlowerFeature(random, x, y, z);
}

int ForestBiome::getRandomDoublePlantType(Random* random)
{
    if (biomeType == 1 || biomeType == 2 || biomeType == 3) 
    {
        
        return random->nextInt(3) + 2; 
    }
    
    
    return Biome::getRandomDoublePlantType(random);
}