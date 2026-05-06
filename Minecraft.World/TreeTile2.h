#pragma once

#include "RotatedPillarTile.h"

class ChunkRebuildData;
class Player;

class TreeTile2 : public RotatedPillarTile
{
	friend class Tile;
	friend class ChunkRebuildData;
public:
	static const int ACACIA_TRUNK = 0;
	static const int DARK_TRUNK = 1;

	static const int MASK_TYPE = 0x3;
	static const int MASK_FACING = 0xC;
	static const int FACING_Y = 0 << 2;
	static const int FACING_X = 1 << 2;
	static const int FACING_Z = 2 << 2;

	static const int TREE_NAMES_LENGTH = 2;


	static const unsigned int TREE_NAMES[TREE_NAMES_LENGTH];

	static const wstring TREE_STRING_NAMES[TREE_NAMES_LENGTH];

	static const wstring TREE_TEXTURES[];

private:
	Icon* icons_side[TREE_NAMES_LENGTH];
	Icon* icons_top[TREE_NAMES_LENGTH];

protected:
	TreeTile2(int id);

public:
	virtual int getResourceCount(Random* random);
	virtual int getResource(int data, Random* random, int playerBonusLevel);
	virtual void createBlockStateDefinition() override;
	virtual int defaultBlockState() override;
	virtual int convertBlockStateToLegacyData(BlockState *state) override;
	virtual Tile::BlockState getBlockState(LevelSource *level, int x, int y, int z) override;
	virtual Tile::BlockState getBlockState(int data);
	virtual void onRemove(Level* level, int x, int y, int z, int id, int data);

protected:
	virtual Icon* getTypeTexture(int type);
	virtual Icon* getTopTexture(int type);

public:
	static int getWoodType(int data);
	void registerIcons(IconRegister* iconRegister);

protected:
	virtual shared_ptr<ItemInstance> getSilkTouchItemInstance(int data);
};