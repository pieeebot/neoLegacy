#include "stdafx.h"
#include "RoofedForestBiome.h"
#include "BiomeDecorator.h"
#include "DarkOakFeature.h"
#include "HugeMushroomFeature.h"
#include "net.minecraft.world.level.tile.h"
#include "..\Level.h"

RoofedForestBiome::RoofedForestBiome(int id) : Biome(id)
{
    
    decorator->treeCount = 35;        
    decorator->grassCount = 2;         
    decorator->flowerCount = 1;        
    decorator->mushroomCount = 1;
    decorator->hugeMushrooms = 1;
    
    temperature = 0.7f;
    downfall = 0.8f;
    
    topMaterial = static_cast<byte>(Tile::grass_Id);
    material = static_cast<byte>(Tile::dirt_Id);
    
    
    setColor(0x28340A);
    setLeafColor(0x2D5A27);
}

Feature* RoofedForestBiome::getTreeFeature(Random* random)
{
    ;
    
    
    if (random->nextInt(5) == 0)  
    {
        
        return new HugeMushroomFeature();
    }
    
    
    return new DarkOakFeature(true);
}



int RoofedForestBiome::getGrassColor()
{
    return 0x28340A;
}

int RoofedForestBiome::getFolageColor() 
{
    return 0x2D5A27;
}