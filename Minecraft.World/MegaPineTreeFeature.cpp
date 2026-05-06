#include "stdafx.h"
#include "MegaPineTreeFeature.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.level.tile.h"
#include "Random.h"
#include "Level.h"

MegaPineTreeFeature::MegaPineTreeFeature(bool doUpdate, bool useBaseHeight) : Feature(doUpdate), useBaseHeight(useBaseHeight)
{
}

// Port of WorldGenHugeTrees.func_175925_a
// Places a layer of leaves centered on the 2x2 trunk at (x,z),(x+1,z),(x,z+1),(x+1,z+1)
// Equivalent to: keep block if it's within `radius` of at least one of the 4 trunk corners.
void MegaPineTreeFeature::placeLeavesLayer(Level *level, int x, int z, int y, int radius, Random *random)
{
	int rSq = radius * radius;
	// Loop from x-radius to x+radius+1 (extra +1 for 2x2 trunk width)
	for (int xx = x - radius; xx <= x + radius + 1; xx++)
	{
		for (int zz = z - radius; zz <= z + radius + 1; zz++)
		{
			// Distance from each of the 4 trunk corner columns
			int j0 = xx - x,     k0 = zz - z;
			int j1 = xx - (x+1), k1 = zz - z;
			int j2 = xx - x,     k2 = zz - (z+1);
			int j3 = xx - (x+1), k3 = zz - (z+1);
			// Place leaf if within radius of ANY trunk corner
			if (j0*j0 + k0*k0 > rSq &&
			    j1*j1 + k1*k1 > rSq &&
			    j2*j2 + k2*k2 > rSq &&
			    j3*j3 + k3*k3 > rSq)
			{
				continue;
			}
			// Java: only place on air or leaf blocks (isAirLeaves equivalent)
			int t = level->getTile(xx, y, zz);
			if (t == 0 || t == Tile::leaves_Id || t == Tile::leaves2_Id)
			{
				placeBlock(level, xx, y, zz, Tile::leaves_Id, LeafTile::EVERGREEN_LEAF);
			}
		}
	}
}

// Port of WorldGenMegaPineTree.func_150541_c
// Generates conical leaf crown from the top of the tree downward.
// p_150541_5_ is always 0 in the original call (base radius offset)
void MegaPineTreeFeature::placeCrown(Level *level, int x, int z, int treeTop, Random *random)
{
	// Crown span = rand.nextInt(5) + (useBaseHeight ? baseHeight=13 : 3)
	int crownHeight = random->nextInt(5) + (this->useBaseHeight ? 13 : 3);
	int prevRadius = 0;

	// k goes from (treeTop - crownHeight) to treeTop
	for (int k = treeTop - crownHeight; k <= treeTop; k++)
	{
		int l = treeTop - k;  // l = distance from top: goes from crownHeight down to 0
		int i1 = (int)floorf((float)l / (float)crownHeight * 3.5f); // base radius at layer
		// Stagger: if same radius as layer below and even Y, expand by 1
		int layerRadius = i1 + (l > 0 && i1 == prevRadius && (k & 1) == 0 ? 1 : 0);
		placeLeavesLayer(level, x, z, k, layerRadius, random);
		prevRadius = i1;
	}
}

// Port of WorldGenMegaPineTree.func_175934_c
// Searches downward from y+2 to y-3 and replaces first grass/dirt with Podzol
void MegaPineTreeFeature::func_175934_c(Level *level, int x, int z, int y)
{
	for (int i = 2; i >= -3; --i)
	{
		int targetY = y + i;
		int tile = level->getTile(x, targetY, z);
		// canSustainPlant equivalent: grass or dirt
		if (tile == Tile::grass_Id || tile == Tile::dirt_Id)
		{
			placeBlock(level, x, targetY, z, Tile::dirt_Id, DirtTile::PODZOL);
			break;
		}
		if (tile != 0 && i < 0) break;
	}
}

// Port of WorldGenMegaPineTree.func_175933_b
// Spreads Podzol in a 5x5 (minus corners) pattern centered at (x,z)
void MegaPineTreeFeature::func_175933_b(Level *level, int x, int z, int y)
{
	for (int i = -2; i <= 2; ++i)
	{
		for (int j = -2; j <= 2; ++j)
		{
			if (abs(i) != 2 || abs(j) != 2)  // skip the 4 far corners
			{
				func_175934_c(level, x + i, z + j, y);
			}
		}
	}
}

// Port of WorldGenMegaPineTree.func_180711_a
// Places Podzol patches at the 4 corners + 5 random border positions
void MegaPineTreeFeature::placeBase(Level *level, Random *random, int x, int z, int y)
{
	// 4 fixed corners (relative to 2x2 trunk)
	func_175933_b(level, x - 1, z - 1, y);  // west().north()
	func_175933_b(level, x + 2, z - 1, y);  // east(2).north()
	func_175933_b(level, x - 1, z + 2, y);  // west().south(2)
	func_175933_b(level, x + 2, z + 2, y);  // east(2).south(2)

	// 5 random border positions (8x8 grid, only border cells)
	for (int i = 0; i < 5; ++i)
	{
		int j = random->nextInt(64);
		int k = j % 8;
		int l = j / 8;
		if (k == 0 || k == 7 || l == 0 || l == 7)
		{
			func_175933_b(level, x - 3 + k, z - 3 + l, y);
		}
	}
}

// Port of WorldGenHugeTrees.func_175929_a (space check)
// Returns true if the tree can grow: checks all blocks from y to y+height+1
bool MegaPineTreeFeature::canPlace(Level *level, Random *random, int x, int y, int z, int height)
{
	if (y < 1 || y + height + 1 > Level::maxBuildHeight) return false;

	for (int l = 0; l <= height + 1; l++)
	{
		int r = (l == 0) ? 1 : 2;  // bottom layer: radius 1; rest: radius 2
		int yy = y + l;
		for (int xx = x - r; xx <= x + r + 1; xx++)
		{
			for (int zz = z - r; zz <= z + r + 1; zz++)
			{
				if (yy < 0 || yy >= Level::maxBuildHeight) return false;
				int t = level->getTile(xx, yy, zz);
				if (!isReplaceable(t)) return false;
			}
		}
	}

	// Ground check: all 4 blocks below the 2x2 trunk must be grass or dirt
	auto isGround = [&](int bx, int bz) {
		int t = level->getTile(bx, y - 1, bz);
		return t == Tile::grass_Id || t == Tile::dirt_Id;
	};
	return isGround(x, z) && isGround(x+1, z) && isGround(x, z+1) && isGround(x+1, z+1);
}

bool MegaPineTreeFeature::isAirLeaves(Level *level, int x, int y, int z)
{
	int t = level->getTile(x, y, z);
	return t == 0 || t == Tile::leaves_Id || t == Tile::leaves2_Id;
}

// Equivalent to Java's canGrowInto: blocks the tree can grow through
bool MegaPineTreeFeature::isReplaceable(int tileId)
{
	return tileId == 0
		|| tileId == Tile::leaves_Id
		|| tileId == Tile::leaves2_Id
		|| tileId == Tile::grass_Id
		|| tileId == Tile::dirt_Id
		|| tileId == Tile::log_Id
		|| tileId == Tile::sapling_Id
		|| tileId == Tile::deadbush_Id
		|| tileId == Tile::tallgrass_Id
		|| tileId == Tile::snow_Id;
}

bool MegaPineTreeFeature::place(Level *level, Random *random, int x, int y, int z)
{
	// func_150533_a: rand.nextInt(maxVariation - baseHeight) + baseHeight
	// = rand.nextInt(15 - 13) + 13 = rand.nextInt(2) + 13
	int height = random->nextInt(2) + 13;

	// func_175929_a: space check
	if (!canPlace(level, random, x, y, z, height))
	{
		return false;
	}

	// func_150541_c: generate conical leaf crown first
	placeCrown(level, x, z, y + height, random);

	// Trunk: 2x2 column of spruce logs
	for (int j = 0; j < height; j++)
	{
		if (isAirLeaves(level, x, y + j, z))
		{
			placeBlock(level, x, y + j, z, Tile::log_Id, TreeTile::SPRUCE_TRUNK);
		}
		if (j < height - 1) // 3 extra columns stop 1 short of the top
		{
			if (isAirLeaves(level, x + 1, y + j, z))
			{
				placeBlock(level, x + 1, y + j, z, Tile::log_Id, TreeTile::SPRUCE_TRUNK);
			}
			if (isAirLeaves(level, x + 1, y + j, z + 1))
			{
				placeBlock(level, x + 1, y + j, z + 1, Tile::log_Id, TreeTile::SPRUCE_TRUNK);
			}
			if (isAirLeaves(level, x, y + j, z + 1))
			{
				placeBlock(level, x, y + j, z + 1, Tile::log_Id, TreeTile::SPRUCE_TRUNK);
			}
		}
	}

	// func_180711_a: generate Podzol base patches
	placeBase(level, random, x, z, y);

	return true;
}
