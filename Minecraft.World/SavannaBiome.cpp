#include "stdafx.h"
#include "net.minecraft.world.level.levelgen.feature.h"
#include "net.minecraft.world.level.biome.h"
#include "net.minecraft.world.entity.animal.h"
#include "net.minecraft.world.entity.h"
#include "SavannaBiome.h"
#include "SavannaTreeFeature.h" 

SavannaBiome::SavannaBiome(int id) : Biome(id)
{
    
    friendlies.push_back(new MobSpawnerData(eTYPE_HORSE, 1, 2, 6));

    
    decorator->treeCount = 1;
    decorator->flowerCount = 4;
    decorator->grassCount = 20; 
}

Feature *SavannaBiome::getTreeFeature(Random *random)
{
    
    if (random->nextInt(5) > 0)
    {
        return new SavannaTreeFeature(false);
    }
    
    
    return new TreeFeature(false);
}

int SavannaBiome::getGrassColor()
{
    return 0xBFB755; 
}

int SavannaBiome::getFolageColor() 
{
    return 0xAEA42A; 
}
