#include "stdafx.h"
#include "RoofTreeFeature.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.level.tile.h"
#include "TreeTile2.h"
#include "LeafTile2.h"
#include <cmath>
#include <algorithm>

RoofTreeFeature::RoofTreeFeature(bool doUpdate) : Feature(doUpdate) {
}


static bool isReplaceable(int tile) {
    return tile == 0 ||
           tile == Tile::tallgrass_Id ||
           tile == Tile::sapling_Id ||
           tile == Tile::grass_Id ||
           tile == Tile::dirt_Id ||
           tile == Tile::leaves_Id ||
           tile == Tile::leaves2_Id;
}




bool RoofTreeFeature::checkSpace(Level *worldIn, int x, int y, int z, int height) {
    for (int dy = 0; dy <= height + 1; ++dy) {
        int radius = 1;
        if (dy == 0) radius = 0;          
        if (dy >= height - 1) radius = 2; 

        for (int dx = -radius; dx <= radius; ++dx) {
            for (int dz = -radius; dz <= radius; ++dz) {
                
                if (dy == 0 && dx == 0 && dz == 0) continue;

                int tile = worldIn->getTile(x + dx, y + dy, z + dz);
                
                if (tile != 0 && !isReplaceable(tile)) {
                    return false;
                }
                
                if (tile == Tile::flowing_water_Id) {
                    return false;
                }
            }
        }
    }
    return true;
}



void RoofTreeFeature::placeLog(Level *worldIn, int x, int y, int z) {
    int tile = worldIn->getTile(x, y, z);
    if (tile == 0 || tile == Tile::leaves_Id || tile == Tile::leaves2_Id || tile == Tile::tallgrass_Id) {
        placeBlock(worldIn, x, y, z, Tile::log2_Id, TreeTile2::DARK_TRUNK);
    }
}




void RoofTreeFeature::placeLeaf(Level *worldIn, int x, int y, int z) {
    int tile = worldIn->getTile(x, y, z);
    if (tile == 0) {
        placeBlock(worldIn, x, y, z, Tile::leaves2_Id, 1);
    }
}



bool RoofTreeFeature::place(Level *level, Random *rand, int x, int y, int z) {
    
    int height = rand->nextInt(3) + 6;
    int startX = x, startY = y, startZ = z;

    
    if (startY < 1 || startY + height + 1 >= Level::maxBuildHeight) return false;
    if (level->getTile(startX, startY - 1, startZ) != Tile::grass_Id &&
        level->getTile(startX, startY - 1, startZ) != Tile::dirt_Id) return false;
    if (startY >= Level::maxBuildHeight - height - 1) return false;
    if (!checkSpace(level, startX, startY, startZ, height)) return false;

    
    if (app.getLevelGenerationOptions() != nullptr) {
        LevelGenerationOptions *options = app.getLevelGenerationOptions();
        int radius = 4;
        if (options->checkIntersects(x - radius, y - 1, z - radius,
                                      x + radius + 1, y + height, z + radius + 1))
            return false;
    }

    
    placeBlock(level, startX,     startY - 1, startZ,     Tile::dirt_Id, 0);
    placeBlock(level, startX + 1, startY - 1, startZ,     Tile::dirt_Id, 0);
    placeBlock(level, startX,     startY - 1, startZ + 1, Tile::dirt_Id, 0);
    placeBlock(level, startX + 1, startY - 1, startZ + 1, Tile::dirt_Id, 0);

    
    int face = rand->nextInt(4);          
    int dx = 0, dz = 0;
    if (face == 0)      dz = -1;
    else if (face == 1) dx =  1;
    else if (face == 2) dz =  1;
    else if (face == 3) dx = -1;

    int bendHeight = height - rand->nextInt(2) - 1; 
    int bendLength = rand->nextInt(2);  
    int trunkX = startX, trunkZ = startZ;
    int topY = startY + height - 1;

    
    for (int dy = 0; dy < height; ++dy) {
        if (dy >= bendHeight && bendLength > 0) {
            trunkX += dx;
            trunkZ += dz;
            --bendLength;
        }
        int currentY = startY + dy;
        placeLog(level, trunkX,     currentY, trunkZ);
        placeLog(level, trunkX + 1, currentY, trunkZ);
        placeLog(level, trunkX,     currentY, trunkZ + 1);
        placeLog(level, trunkX + 1, currentY, trunkZ + 1);
    }

    
    for (int i3 = -2; i3 <= 0; ++i3) {
        for (int l3 = -2; l3 <= 0; ++l3) {
            int yOffset = -1;   
            placeLeaf(level, trunkX + i3,     topY + yOffset, trunkZ + l3);
            placeLeaf(level, trunkX + 1 - i3, topY + yOffset, trunkZ + l3);
            placeLeaf(level, trunkX + i3,     topY + yOffset, trunkZ + 1 - l3);
            placeLeaf(level, trunkX + 1 - i3, topY + yOffset, trunkZ + 1 - l3);

           
            if ((i3 > -2 || l3 > -1) && (i3 != -1 || l3 != -2)) {
                yOffset = 1;
                placeLeaf(level, trunkX + i3,     topY + yOffset, trunkZ + l3);
                placeLeaf(level, trunkX + 1 - i3, topY + yOffset, trunkZ + l3);
                placeLeaf(level, trunkX + i3,     topY + yOffset, trunkZ + 1 - l3);
                placeLeaf(level, trunkX + 1 - i3, topY + yOffset, trunkZ + 1 - l3);
            }
        }
    }

    
    if (rand->nextInt(2) == 0) {
        placeLeaf(level, trunkX,     topY + 2, trunkZ);
        placeLeaf(level, trunkX + 1, topY + 2, trunkZ);
        placeLeaf(level, trunkX + 1, topY + 2, trunkZ + 1);
        placeLeaf(level, trunkX,     topY + 2, trunkZ + 1);
    }

   
    for (int dxOff = -3; dxOff <= 4; ++dxOff) {
        for (int dzOff = -3; dzOff <= 4; ++dzOff) {
           
            if ((dxOff == -3 || dxOff == 4) && (dzOff == -3 || dzOff == 4))
                continue;
            if (std::abs(dxOff) < 3 || std::abs(dzOff) < 3) {
                placeLeaf(level, trunkX + dxOff, topY, trunkZ + dzOff);
            }
        }
    }

    
    for (int dxOff = -1; dxOff <= 2; ++dxOff) {
        for (int dzOff = -1; dzOff <= 2; ++dzOff) {
            
            if ((dxOff < 0 || dxOff > 1 || dzOff < 0 || dzOff > 1) && rand->nextInt(3) <= 0) {
                int branchLength = rand->nextInt(2) + 2;  

                
                for (int l = 0; l < branchLength; ++l) {
                    placeLog(level, startX + dxOff, topY - l - 1, startZ + dzOff);
                }

                
                for (int lx = -1; lx <= 1; ++lx) {
                    for (int lz = -1; lz <= 1; ++lz) {
                        placeLeaf(level, trunkX + dxOff + lx, topY, trunkZ + dzOff + lz);
                    }
                }
                
                for (int lx = -2; lx <= 2; ++lx) {
                    for (int lz = -2; lz <= 2; ++lz) {
                        if (std::abs(lx) != 2 || std::abs(lz) != 2) {
                            placeLeaf(level, trunkX + dxOff + lx, topY - 1, trunkZ + dzOff + lz);
                        }
                    }
                }
            }
        }
    }

    return true;
}