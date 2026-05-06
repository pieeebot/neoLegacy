#include "stdafx.h"
#include "net.minecraft.world.level.biome.h"
#include "DoublePlantFeature.h"
#include "TallGrass2.h"
#include "Level.h"

PlainsBiome::PlainsBiome(int id,bool plains) : Biome(id)
{
    _plains = plains;
	friendlies.push_back(new MobSpawnerData(eTYPE_HORSE, 5, 2, 6));
	friendlies.push_back(new MobSpawnerData(eTYPE_RABBIT, 4, 2, 3));

	decorator->treeCount = -999;
	decorator->flowerCount = 4;
	decorator->grassCount = 10;
}

Feature* PlainsBiome::getFlowerFeature(Random* random, int x, int y, int z)
{
    double d0 = GRASS_COLOR_NOISE->getValue((double)(x + 8) / 200.0,(double)(z + 8) / 200.0);
    if (d0 < -0.8)
        {
           int j = random->nextInt(4);
            switch (j) {
            case 0: return new FlowerFeature(Tile::red_flower_Id, Rose::ORANGE_TULIP);
            case 1: return new FlowerFeature(Tile::red_flower_Id, Rose::RED_TULIP);
            case 2: return new FlowerFeature(Tile::red_flower_Id, Rose::PINK_TULIP);
            case 3: return new FlowerFeature(Tile::red_flower_Id, Rose::WHITE_TULIP);
            }

        }else if (random->nextInt(3) > 0)
        {
            int i = random->nextInt(3);
            if (i == 1) {
                 return new FlowerFeature(Tile::red_flower_Id,Rose::AZURE_BLUET);
            } else
                 return new FlowerFeature(Tile::red_flower_Id,Rose::OXEYE_DAISY);
                
            
        }
        else
        {
             return new FlowerFeature(Tile::yellow_flower_Id);
        }
    
    return Biome::getFlowerFeature(random, x, y, z);
}
void PlainsBiome::decorate(Level* level, Random* rand, int xo, int zo)
{
     double d0 = GRASS_COLOR_NOISE->getValue((double)(xo + 8) / 200.0,(double)(zo + 8) / 200.0);
    if (d0 < -0.8)
        {
            decorator->flowerCount = 15;
            decorator->grassCount = 5;
        }
    else
    {

        decorator->flowerCount = 4;
        decorator->grassCount = 10;
        DOUBLE_PLANT_GENERATOR->setPlantType(TallGrass2::TALL_GRASS);





        for (int i = 0; i < 7; ++i)
        {
            int x = xo + rand->nextInt(16) + 8;
            int z = zo + rand->nextInt(16) + 8;
            int y = rand->nextInt(level->getHeightmap(x, z) + 32);
            DOUBLE_PLANT_GENERATOR->place(level, rand, x, y, z);
        }
    }

    if (_plains)
        {
            DOUBLE_PLANT_GENERATOR->setPlantType(TallGrass2::SUNFLOWER);

            for (int i1 = 0; i1 < 10; ++i1)
            {
                int j1 = rand->nextInt(16) + 8;
                int k1 = rand->nextInt(16) + 8;
                int l1 = rand->nextInt(level->getHeightmap(j1, k1) + 32);
                DOUBLE_PLANT_GENERATOR->place(level, rand, j1, l1, k1);
            }
        }
    Biome::decorate(level, rand, xo, zo); 
}