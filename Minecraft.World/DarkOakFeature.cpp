#include "stdafx.h"
#include "DarkOakFeature.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.level.tile.h"
#include "TreeTile2.h"
#include "LeafTile2.h"
#include <cmath>

#include <algorithm>

DarkOakFeature::DarkOakFeature(bool doUpdate) : Feature(doUpdate) {
    this->baseHeight = 6;
}

bool DarkOakFeature::place(Level *worldIn, Random *rand, int x, int y, int z) {
    int i = rand->nextInt(3) + rand->nextInt(2) + 6;
    int j = x;
    int k = y;
    int l = z;

    if (k >= 1 && k + i + 1 < Level::maxBuildHeight) {
        int below = worldIn->getTile(j, k - 1, l);
        bool isSoil = (below == Tile::grass_Id || below == Tile::dirt_Id);

        if (!(isSoil && k < Level::maxBuildHeight - i - 1)) {
            return false;
        } else if (!this->checkSpace(worldIn, j, k, l, i)) {
            return false;
        } else {
           
            int dirX = 0;
            int dirZ = 0;
            int facing = rand->nextInt(4);
            if (facing == 0) dirX = 1;
            else if (facing == 1) dirX = -1;
            else if (facing == 2) dirZ = 1;
            else if (facing == 3) dirZ = -1;

            int i1 = i - rand->nextInt(4);
            int j1 = 2 - rand->nextInt(3);
            int k1 = j;
            int l1 = l;
            int i2 = k + i - 1;

            
            for (int j2 = 0; j2 < i; ++j2) {
                if (j2 >= i1 && j1 > 0) {
                    k1 += dirX;
                    l1 += dirZ;
                    --j1;
                }
                int k2 = k + j2;
                this->placeLog(worldIn, k1, k2, l1);
                this->placeLog(worldIn, k1 + 1, k2, l1);
                this->placeLog(worldIn, k1, k2, l1 + 1);
                this->placeLog(worldIn, k1 + 1, k2, l1 + 1);
            }

            
            for (int i3 = -2; i3 <= 0; ++i3) {
                for (int l3 = -2; l3 <= 0; ++l3) {
                    int k4 = -1;
                    this->placeLeaf(worldIn, k1 + i3, i2 + k4, l1 + l3);
                    this->placeLeaf(worldIn, 1 + k1 - i3, i2 + k4, l1 + l3);
                    this->placeLeaf(worldIn, k1 + i3, i2 + k4, 1 + l1 - l3);
                    this->placeLeaf(worldIn, 1 + k1 - i3, i2 + k4, 1 + l1 - l3);

                    if ((i3 > -2 || l3 > -1) && (i3 != -1 || l3 != -2)) {
                        k4 = 1;
                        this->placeLeaf(worldIn, k1 + i3, i2 + k4, l1 + l3);
                        this->placeLeaf(worldIn, 1 + k1 - i3, i2 + k4, l1 + l3);
                        this->placeLeaf(worldIn, k1 + i3, i2 + k4, 1 + l1 - l3);
                        this->placeLeaf(worldIn, 1 + k1 - i3, i2 + k4, 1 + l1 - l3);
                    }
                }
            }

            
            for (int j3 = -3; j3 <= 4; ++j3) {
                for (int i4 = -3; i4 <= 4; ++i4) {
                    if ((j3 != -3 || i4 != -3) && (j3 != -3 || i4 != 4) && (j3 != 4 || i4 != -3) && (j3 != 4 || i4 != 4) && (std::abs(j3) < 3 || std::abs(i4) < 3)) {
                        this->placeLeaf(worldIn, k1 + j3, i2, l1 + i4);
                    }
                }
            }

            
            for (int k3 = -1; k3 <= 2; ++k3) {
                for (int j4 = -1; j4 <= 2; ++j4) {
                    if ((k3 < 0 || k3 > 1 || j4 < 0 || j4 > 1) && rand->nextInt(3) <= 0) {
                        int l4 = rand->nextInt(3) + 2;

                        
                        for (int i5 = 0; i5 < l4; ++i5) {
                            this->placeLog(worldIn, j + k3, i2 - i5 - 1, l + j4);
                        }

                        
                        for (int j5 = -1; j5 <= 1; ++j5) {
                            for (int l2 = -1; l2 <= 1; ++l2) {
                                this->placeLeaf(worldIn, k1 + k3 + j5, i2, l1 + j4 + l2);
                            }
                        }

                        
                        for (int k5 = -2; k5 <= 2; ++k5) {
                            for (int l5 = -2; l5 <= 2; ++l5) {
                                if (std::abs(k5) != 2 || std::abs(l5) != 2) {
                                    this->placeLeaf(worldIn, k1 + k3 + k5, i2 - 1, l1 + j4 + l5);
                                }
                            }
                        }
                    }
                }
            }

            return true;
        }
    }
    return false;
}


bool DarkOakFeature::checkSpace(Level *worldIn, int x, int y, int z, int height) {
    for (int l = 0; l <= height + 1; ++l) {
        int i1 = 1;
        if (l == 0) i1 = 0;
        if (l >= height - 1) i1 = 2;

        for (int j1 = -i1; j1 <= i1; ++j1) {
            for (int k1 = -i1; k1 <= i1; ++k1) {
                int tile = worldIn->getTile(x + j1, y + l, z + k1);
                
                // ignores grass, dirt e trunks. 
                if (tile != 0 && 
                    tile != Tile::leaves_Id && tile != Tile::leaves2_Id && 
                    tile != Tile::tallgrass_Id && tile != Tile::sapling_Id &&
                    tile != Tile::grass_Id && tile != Tile::dirt_Id && 
                    tile != Tile::treeTrunk_Id && tile != Tile::tree2Trunk_Id) {
                    return false;
                }
            }
        }
    }
    return true;
}

void DarkOakFeature::placeLog(Level *worldIn, int x, int y, int z) {
    int tile = worldIn->getTile(x, y, z);
    
    if (tile == 0 || tile == Tile::leaves_Id || tile == Tile::leaves2_Id || tile == Tile::tallgrass_Id) {
        
        placeBlock(worldIn, x, y, z, Tile::tree2Trunk_Id, TreeTile2::DARK_TRUNK);
    }
}

void DarkOakFeature::placeLeaf(Level *worldIn, int x, int y, int z) {
    int tile = worldIn->getTile(x, y, z);
    
    if (tile == 0) {
        
        placeBlock(worldIn, x, y, z, Tile::leaves2_Id, 1);
    }
}