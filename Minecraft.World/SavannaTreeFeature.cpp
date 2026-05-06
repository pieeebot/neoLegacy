#include "stdafx.h"
#include "SavannaTreeFeature.h"
#include "net.minecraft.world.level.tile.h"
#include "Level.h"
#include "Random.h"
#include "Direction.h"

SavannaTreeFeature::SavannaTreeFeature(bool doUpdate) : AbstractTreeFeature(doUpdate)
{
}

void SavannaTreeFeature::placeLog(Level* level, int x, int y, int z)
{
    placeBlock(level, x, y, z, Tile::log2_Id, 0);
}

void SavannaTreeFeature::placeLeafAt(Level* level, int x, int y, int z)
{
    int tile = level->getTile(x, y, z);
    if (tile == 0 || tile == Tile::leaves_Id || tile == Tile::leaves2_Id)
    {
        placeBlock(level, x, y, z, Tile::leaves2_Id, 0);
    }
}

void SavannaTreeFeature::placeLeavesLayer3(Level* level, int cx, int cy, int cz)
{
    for (int dx = -3; dx <= 3; ++dx)
    {
        for (int dz = -3; dz <= 3; ++dz)
        {
            if (abs(dx) != 3 || abs(dz) != 3)
            {
                placeLeafAt(level, cx + dx, cy, cz + dz);
            }
        }
    }
}

void SavannaTreeFeature::placeLeavesLayer1(Level* level, int cx, int cy, int cz)
{
    for (int dx = -1; dx <= 1; ++dx)
    {
        for (int dz = -1; dz <= 1; ++dz)
        {
            placeLeafAt(level, cx + dx, cy, cz + dz);
        }
    }
    placeLeafAt(level, cx + 2, cy, cz);
    placeLeafAt(level, cx - 2, cy, cz);
    placeLeafAt(level, cx,     cy, cz + 2);
    placeLeafAt(level, cx,     cy, cz - 2);
}

bool SavannaTreeFeature::place(Level* level, Random* random, int x, int y, int z)
{

    int height = random->nextInt(3) + random->nextInt(3) + 5;

    if (y <= 0 || y + height + 1 > 256)
        return false;

    bool canPlace = true;
    for (int j = y; j <= y + 1 + height && canPlace; ++j)
    {
        int radius = 1;
        if (j == y)                    radius = 0;
        if (j >= y + 1 + height - 2)  radius = 2;

        for (int lx = x - radius; lx <= x + radius && canPlace; ++lx)
        {
            for (int lz = z - radius; lz <= z + radius && canPlace; ++lz)
            {
                if (j >= 0 && j < 256)
                {
                    if (!AbstractTreeFeature::isFree(level, lx, j, lz))
                        canPlace = false;
                }
                else
                {
                    canPlace = false;
                }
            }
        }
    }

    if (!canPlace)
        return false;

    int belowTile = level->getTile(x, y - 1, z);
    if (belowTile != Tile::grass_Id && belowTile != Tile::dirt_Id)
        return false;

    if (y >= 255 - height)
        return false;

    setDirtAt(level, x, y - 1, z);


    int facing1     = Direction::Plane::getRandomFace(random);
    int branchStart = height - random->nextInt(4) - 1;  
    int branchLen   = 3 - random->nextInt(3);           

    int curX = x;
    int curZ = z;
    int topY = y;

    for (int l1 = 0; l1 < height; ++l1)
    {
        int curY = y + l1;

        if (l1 >= branchStart && branchLen > 0)
        {
            curX += Direction::getStepX(facing1);
            curZ += Direction::getStepZ(facing1);
            --branchLen;
        }

        int tile = level->getTile(curX, curY, curZ);
        if (tile == 0 || tile == Tile::leaves_Id || tile == Tile::leaves2_Id)
        {
            placeLog(level, curX, curY, curZ);
            topY = curY;
        }
    }


    placeLeavesLayer3(level, curX, topY,     curZ);
    placeLeavesLayer1(level, curX, topY + 1, curZ);


    int curX2   = x;
    int curZ2   = z;
    int facing2 = Direction::Plane::getRandomFace(random);

    if (facing2 != facing1)
    {

        int start2 = branchStart - random->nextInt(2) - 1;
        int steps2 = 1 + random->nextInt(3);
        int topY2  = 0;


        for (int l4 = start2; l4 < height && steps2 > 0; --steps2)
        {
            if (l4 >= 1)
            {
                int curY2 = y + l4;
                curX2 += Direction::getStepX(facing2);
                curZ2 += Direction::getStepZ(facing2);

                int tile2 = level->getTile(curX2, curY2, curZ2);
                if (tile2 == 0 || tile2 == Tile::leaves_Id || tile2 == Tile::leaves2_Id)
                {
                    placeLog(level, curX2, curY2, curZ2);
                    topY2 = curY2;
                }
            }
            ++l4;
        }


        if (topY2 > 0)
        {
            
            for (int dx = -2; dx <= 2; ++dx)
            {
                for (int dz = -2; dz <= 2; ++dz)
                {
                    if (abs(dx) != 2 || abs(dz) != 2)
                    {
                        placeLeafAt(level, curX2 + dx, topY2, curZ2 + dz);
                    }
                }
            }

            for (int dx = -1; dx <= 1; ++dx)
            {
                for (int dz = -1; dz <= 1; ++dz)
                {
                    placeLeafAt(level, curX2 + dx, topY2 + 1, curZ2 + dz);
                }
            }
        }
    }

    return true;
}