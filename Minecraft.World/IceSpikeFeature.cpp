#include "IceSpikeFeature.h"
#include <cmath>
#include "Level.h"
#include "Tile.h"
#include <algorithm>

IceSpikeFeature::IceSpikeFeature() {}

bool IceSpikeFeature::place(Level *level, Random *random, int x, int y, int z)
{
    
    while (level->isEmptyTile(x, y, z) && y > 2)
    {
        y--;
    }

  
    if (level->getTile(x, y, z) != Tile::snow_Id) 
    {
        return false;
    }

    y += random->nextInt(4); 
    int height = random->nextInt(4) + 7;
    int baseRadius = height / 4 + random->nextInt(2);

    
    if (baseRadius > 1 && random->nextInt(60) == 0)
    {
        y += 10 + random->nextInt(30);
    }

    
    for (int k = 0; k < height; ++k)
    {
        
        float f = (1.0F - (float)k / (float)height) * (float)baseRadius;
        int l = ceil(f);

        for (int ix = -l; ix <= l; ++ix)
        {
            float f1 = (float)abs(ix) - 0.25F;
            for (int iz = -l; iz <= l; ++iz)
            {
                float f2 = (float)abs(iz) - 0.25F;

               
                if ((ix == 0 && iz == 0 || f1 * f1 + f2 * f2 <= f * f) && 
                    (ix != -l && ix != l && iz != -l && iz != l || random->nextFloat() <= 0.75F))
                {
                    int currentTile = level->getTile(x + ix, y + k, z + iz);
                    
                   
                    if (level->isEmptyTile(x + ix, y + k, z + iz) || currentTile == Tile::dirt_Id || 
                        currentTile == Tile::snow_Id || currentTile == Tile::ice_Id)
                    {
                        level->setTileAndData(x + ix, y + k, z + iz, Tile::packed_ice_Id, 0, 3);
                    }

                    
                    if (k != 0 && l > 1)
                    {
                        currentTile = level->getTile(x + ix, y - k, z + iz);
                        if (level->isEmptyTile(x + ix, y - k, z + iz) || currentTile == Tile::dirt_Id || 
                            currentTile == Tile::snow_Id || currentTile == Tile::ice_Id)
                        {
                            level->setTileAndData(x + ix, y - k, z + iz, Tile::packed_ice_Id, 0, 3);
                        }
                    }
                }
            }
        }
    }

    
    int rootRadius = baseRadius - 1;
    if (rootRadius < 0) rootRadius = 0;
    else if (rootRadius > 1) rootRadius = 1;

    for (int rx = -rootRadius; rx <= rootRadius; ++rx)
    {
        for (int rz = -rootRadius; rz <= rootRadius; ++rz)
        {
            int curY = y - 1;
            int depthCounter = 50;

            if (abs(rx) == 1 && abs(rz) == 1) depthCounter = random->nextInt(5);

            while (curY > 50)
            {
                int t = level->getTile(x + rx, curY, z + rz);
                if (!level->isEmptyTile(x + rx, curY, z + rz) && t != Tile::dirt_Id && 
                    t != Tile::snow_Id && t != Tile::ice_Id && t != Tile::packed_ice_Id)
                {
                    break;
                }

                level->setTileAndData(x + rx, curY, z + rz, Tile::packed_ice_Id, 0, 3);
                curY--;
                depthCounter--;
                if (depthCounter <= 0)
                {
                    curY -= (random->nextInt(5) + 1);
                    depthCounter = random->nextInt(5);
                }
            }
        }
    }

    return true;
}