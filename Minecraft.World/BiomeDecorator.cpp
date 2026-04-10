#include "stdafx.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.level.tile.h"
#include "net.minecraft.world.level.levelgen.feature.h"
#include "net.minecraft.world.level.biome.h"
#include "Rose.h"
#include "TallGrass.h"
#include "TallGrass2.h"
#include "DoublePlantFeature.h"

BiomeDecorator::BiomeDecorator(Biome *biome)
{
    _init();

    // 4J inits
    level = nullptr;
    random = nullptr;
    xo = 0;
    zo = 0;

    this->biome = biome;
}

void BiomeDecorator::decorate(Level *level, Random *random, int xo, int zo)
{
    if (this->level != nullptr)
    {
        app.DebugPrintf("BiomeDecorator::decorate - Already decorating!!\n");
#ifndef _CONTENT_PACKAGE
        __debugbreak();
        //throw new RuntimeException("Already decorating!!");
#endif
    }
    this->level = level;
    this->random = random;
    this->xo = xo;
    this->zo = zo;

    decorate();

    this->level = nullptr;
    this->random = nullptr;
}

void BiomeDecorator::_init()
{
    clayFeature = new ClayFeature(4);
    sandFeature = new SandFeature(7, Tile::sand_Id);
    gravelFeature = new SandFeature(6, Tile::gravel_Id);
    dirtOreFeature = new OreFeature(Tile::dirt_Id, 32);
    gravelOreFeature = new OreFeature(Tile::gravel_Id, 32);
    coalOreFeature = new OreFeature(Tile::coalOre_Id, 16);
    ironOreFeature = new OreFeature(Tile::ironOre_Id, 8);
    goldOreFeature = new OreFeature(Tile::goldOre_Id, 8);
    redStoneOreFeature = new OreFeature(Tile::redStoneOre_Id, 7);
    diamondOreFeature = new OreFeature(Tile::diamondOre_Id, 7);
    lapisOreFeature = new OreFeature(Tile::lapisOre_Id, 6);

    graniteOreFeature = new OreFeature(Tile::stone_Id, StoneTile::GRANITE, 33);
    dioriteOreFeature = new OreFeature(Tile::stone_Id, StoneTile::DIORITE, 33);
    andesiteOreFeature = new OreFeature(Tile::stone_Id, StoneTile::ANDESITE, 33);

    yellowFlowerFeature = new FlowerFeature(Tile::flower_Id);
    roseFlowerFeature = new FlowerFeature(Tile::rose_Id);
    brownMushroomFeature = new FlowerFeature(Tile::mushroom_brown_Id);
    redMushroomFeature = new FlowerFeature(Tile::mushroom_red_Id);
    hugeMushroomFeature = new HugeMushroomFeature();
    reedsFeature = new ReedsFeature();
    cactusFeature = new CactusFeature();
    waterlilyFeature = new WaterlilyFeature();
    
    blueOrchidFeature    = new FlowerFeature(Tile::rose_Id, Rose::BLUE_ORCHID);  
    alliumFeature        = new FlowerFeature(Tile::rose_Id, Rose::ALLIUM);
    azureBluetFeature    = new FlowerFeature(Tile::rose_Id, Rose::AZURE_BLUET);
    oxeyeDaisyFeature    = new FlowerFeature(Tile::rose_Id, Rose::OXEYE_DAISY);
    tulipRedFeature      = new FlowerFeature(Tile::rose_Id, Rose::RED_TULIP);
    tulipOrangeFeature   = new FlowerFeature(Tile::rose_Id, Rose::ORANGE_TULIP);
    tulipWhiteFeature    = new FlowerFeature(Tile::rose_Id, Rose::WHITE_TULIP);
    tulipPinkFeature     = new FlowerFeature(Tile::rose_Id, Rose::PINK_TULIP);

    doublePlantFeature = new DoublePlantFeature(false);

    doublePlantCount = 0;
    waterlilyCount = 0;
    treeCount = 0;
    flowerCount = 2;
    grassCount = 1;
    deadBushCount = 0;
    mushroomCount = 0;
    reedsCount = 0;
    cactusCount = 0;
    gravelCount = 1;
    sandCount = 3;
    clayCount = 1;
    hugeMushrooms = 0;
    blueOrchidCount  = 0;
    alliumCount      = 0;
    azureBluetCount  = 0;
    oxeyeDaisyCount  = 0;
    liquids = true;
}

void BiomeDecorator::decorate()
{
    PIXBeginNamedEvent(0,"Decorate ores");
    decorateOres();
    PIXEndNamedEvent();

    PIXBeginNamedEvent(0,"Decorate sand/clay/gravel");
    for (int i = 0; i < sandCount; i++)
    {
        int x = xo + random->nextInt(16) + 8;
        int z = zo + random->nextInt(16) + 8;
        sandFeature->place(level, random, x, level->getTopSolidBlock(x, z), z);
    }

    for (int i = 0; i < clayCount; i++)
    {
        int x = xo + random->nextInt(16) + 8;
        int z = zo + random->nextInt(16) + 8;
        clayFeature->place(level, random, x, level->getTopSolidBlock(x, z), z);
    }

    for (int i = 0; i < gravelCount; i++)
    {
        int x = xo + random->nextInt(16) + 8;
        int z = zo + random->nextInt(16) + 8;
        sandFeature->place(level, random, x, level->getTopSolidBlock(x, z), z);
    }
    PIXEndNamedEvent();

    PIXBeginNamedEvent(0, "Decorate forests");
    int forests = treeCount;
    if (random->nextInt(10) == 0) forests += 1;

    for (int i = 0; i < forests; i++)
    {
        int x = xo + random->nextInt(16) + 8;
        int z = zo + random->nextInt(16) + 8;
        Feature *tree = biome->getTreeFeature(random);
        tree->init(1, 1, 1);
        tree->place(level, random, x, level->getHeightmap(x, z), z);
        delete tree;
    }
    PIXEndNamedEvent();

    PIXBeginNamedEvent(0,"Decorate mushrooms/flowers/grass");
    for (int i = 0; i < hugeMushrooms; i++)
    {
        int x = xo + random->nextInt(16) + 8;
        int z = zo + random->nextInt(16) + 8;
        hugeMushroomFeature->place(level, random, x, level->getHeightmap(x, z), z);
    }

   
    for (int i = 0; i < flowerCount; i++)
    {
        int x = xo + random->nextInt(16) + 8;
        int z = zo + random->nextInt(16) + 8;
        int y = random->nextInt(Level::genDepth); 
        
        Feature* selectedFlower = biome->getFlowerFeature(random, x, y, z);
        if (selectedFlower != nullptr)
        {
            selectedFlower->place(level, random, x, y, z);
            delete selectedFlower; 
        }
    }

    //forced biomes
    for (int i = 0; i < blueOrchidCount; i++)
    {
        int x = xo + random->nextInt(16) + 8;
        int y = random->nextInt(Level::genDepth);
        int z = zo + random->nextInt(16) + 8;
        blueOrchidFeature->place(level, random, x, y, z);
    }

    for (int i = 0; i < alliumCount; i++)
    {
        int x = xo + random->nextInt(16) + 8;
        int y = random->nextInt(Level::genDepth);
        int z = zo + random->nextInt(16) + 8;
        alliumFeature->place(level, random, x, y, z);
    }

    for (int i = 0; i < azureBluetCount; i++)
    {
        int x = xo + random->nextInt(16) + 8;
        int y = random->nextInt(Level::genDepth);
        int z = zo + random->nextInt(16) + 8;
        azureBluetFeature->place(level, random, x, y, z);
    }

    for (int i = 0; i < oxeyeDaisyCount; i++)
    {
        int x = xo + random->nextInt(16) + 8;
        int y = random->nextInt(Level::genDepth);
        int z = zo + random->nextInt(16) + 8;
        oxeyeDaisyFeature->place(level, random, x, y, z);
    }

    // 1block grass
    for (int i = 0; i < grassCount; i++)
    {
        int x = xo + random->nextInt(16) + 8;
        int y = random->nextInt(Level::genDepth);
        int z = zo + random->nextInt(16) + 8;
        
        MemSect(50);
        Feature *grassFeature = biome->getGrassFeature(random);
        MemSect(0);
        
        if (grassFeature) 
        {
            grassFeature->place(level, random, x, y, z);
            delete grassFeature;
        }
    }

    //int doublePlantsToGen = doublePlantCount; // 

    //for (int i = 0; i < doublePlantsToGen; i++) 
    //{
    //    
    //    int x = xo + random->nextInt(16) + 8;
    //    int z = zo + random->nextInt(16) + 8;
    //    int y = random->nextInt(Level::genDepth); 
    //    
    //    int plantType = biome->getRandomDoublePlantType(random); 
    //    
    //    DoublePlantFeature* dpf = static_cast<DoublePlantFeature*>(doublePlantFeature);
    //    dpf->setPlantType(plantType);
    //    
    //    
    //    dpf->place(level, random, x, y, z);
    //}

    PIXEndNamedEvent();

    PIXBeginNamedEvent(0,"Decorate bush/waterlily/mushroom/reeds/pumpkins/cactuses");

    DeadBushFeature *deadBushFeature = nullptr;
    if(deadBushCount > 0) deadBushFeature = new DeadBushFeature(Tile::deadBush_Id);
    for (int i = 0; i < deadBushCount; i++)
    {
        int x = xo + random->nextInt(16) + 8;
        int y = random->nextInt(Level::genDepth);
        int z = zo + random->nextInt(16) + 8;
        deadBushFeature->place(level, random, x, y, z);
    }
    if(deadBushFeature != nullptr) delete deadBushFeature;

    for (int i = 0; i < waterlilyCount; i++)
    {
        int x = xo + random->nextInt(16) + 8;
        int z = zo + random->nextInt(16) + 8;
        int y = random->nextInt(Level::genDepth);
        while (y > 0 && level->getTile(x, y - 1, z) == 0)
            y--;
        waterlilyFeature->place(level, random, x, y, z);
    }

    for (int i = 0; i < mushroomCount; i++)
    {
        if (random->nextInt(4) == 0)
        {
            int x = xo + random->nextInt(16) + 8;
            int z = zo + random->nextInt(16) + 8;
            int y = level->getHeightmap(x, z);
            brownMushroomFeature->place(level, random, x, y, z);
        }

        if (random->nextInt(8) == 0)
        {
            int x = xo + random->nextInt(16) + 8;
            int z = zo + random->nextInt(16) + 8;
            int y = random->nextInt(Level::genDepth);
            redMushroomFeature->place(level, random, x, y, z);
        }
    }

    if (random->nextInt(4) == 0)
    {
        int x = xo + random->nextInt(16) + 8;
        int y = random->nextInt(Level::genDepth);
        int z = zo + random->nextInt(16) + 8;
        brownMushroomFeature->place(level, random, x, y, z);
    }

    if (random->nextInt(8) == 0)
    {
        int x = xo + random->nextInt(16) + 8;
        int y = random->nextInt(Level::genDepth);
        int z = zo + random->nextInt(16) + 8;
        redMushroomFeature->place(level, random, x, y, z);
    }

    for (int i = 0; i < reedsCount; i++)
    {
        int x = xo + random->nextInt(16) + 8;
        int z = zo + random->nextInt(16) + 8;
        int y = random->nextInt(Level::genDepth);
        reedsFeature->place(level, random, x, y, z);
    }

    for (int i = 0; i < 10; i++)
    {
        int x = xo + random->nextInt(16) + 8;
        int y = random->nextInt(Level::genDepth);
        int z = zo + random->nextInt(16) + 8;
        reedsFeature->place(level, random, x, y, z);
    }

    if (random->nextInt(32) == 0)
    {
        int x = xo + random->nextInt(16) + 8;
        int y = random->nextInt(Level::genDepth);
        int z = zo + random->nextInt(16) + 8;        
        PumpkinFeature *pumpkinFeature = new PumpkinFeature();
        pumpkinFeature->place(level, random, x, y, z);
        delete pumpkinFeature;
    }

    for (int i = 0; i < cactusCount; i++)
    {
        int x = xo + random->nextInt(16) + 8;
        int y = random->nextInt(Level::genDepth);
        int z = zo + random->nextInt(16) + 8;
        cactusFeature->place(level, random, x, y, z);
    }

    PIXEndNamedEvent();
    PIXBeginNamedEvent(0,"Decorate liquids");

    if( liquids )
    {
        SpringFeature *waterSpringFeature = new SpringFeature(Tile::water_Id);
        for (int i = 0; i < 50; i++)
        {
            int x = xo + random->nextInt(16) + 8;
            int y = random->nextInt(random->nextInt(Level::genDepth - 8) + 8);
            int z = zo + random->nextInt(16) + 8;
            waterSpringFeature->place(level, random, x, y, z);
        }
        delete waterSpringFeature;

        SpringFeature *lavaSpringFeature = new SpringFeature(Tile::lava_Id);
        for (int i = 0; i < 20; i++)
        {
            int x = xo + random->nextInt(16) + 8;
            int y = random->nextInt(random->nextInt(random->nextInt(Level::genDepth - 16) + 8) + 8);
            int z = zo + random->nextInt(16) + 8;
            lavaSpringFeature->place(level, random, x, y, z);
        }
        delete lavaSpringFeature;
    }
    PIXEndNamedEvent();
}

void BiomeDecorator::decorate(int count, Feature *feature)
{
    decorateDepthSpan(count, feature, 0, Level::genDepth);
}

void BiomeDecorator::decorateDepthSpan(int count, Feature *feature, int y0, int y1)
{
    for (int i = 0; i < count; i++)
    {
        int x = xo + random->nextInt(16);
        int y = random->nextInt(y1 - y0) + y0;
        int z = zo + random->nextInt(16);
        feature->place(level, random, x, y, z);
    }
}

void BiomeDecorator::decorateDepthAverage(int count, Feature *feature, int yMid, int ySpan)
{
    for (int i = 0; i < count; i++)
    {
        int x = xo + random->nextInt(16);
        int y = random->nextInt(ySpan) + random->nextInt(ySpan) + (yMid - ySpan);
        int z = zo + random->nextInt(16);
        feature->place(level, random, x, y, z);
    }
}

void BiomeDecorator::decorateOres()
{
    level->setInstaTick(true);        // 4J - optimisation
    decorateDepthSpan(20, dirtOreFeature, 0, Level::genDepth);
    decorateDepthSpan(10, gravelOreFeature, 0, Level::genDepth);
    decorateDepthSpan(20, coalOreFeature, 0, Level::genDepth);
    decorateDepthSpan(20, ironOreFeature, 0, Level::genDepth / 2);
    decorateDepthSpan(2, goldOreFeature, 0, Level::genDepth / 4);
    decorateDepthSpan(8, redStoneOreFeature, 0, Level::genDepth / 8);
    decorateDepthSpan(1, diamondOreFeature, 0, Level::genDepth / 8);
    decorateDepthAverage(1, lapisOreFeature, Level::genDepth / 8, Level::genDepth / 8);

    decorateDepthSpan(10, graniteOreFeature, 0, 80);
    decorateDepthSpan(10, dioriteOreFeature, 0, 80);
    decorateDepthSpan(10, andesiteOreFeature, 0, 80);

    level->setInstaTick(false);
}
