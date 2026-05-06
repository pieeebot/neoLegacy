#pragma once

#include "RotatedPillarTile.h"

class HayBlockTile : public RotatedPillarTile
{
	friend class ChunkRebuildData;
public:
	HayBlockTile(int id);

	int getRenderShape();
	virtual void createBlockStateDefinition() override;
	virtual int defaultBlockState() override;
	virtual int convertBlockStateToLegacyData(BlockState *state) override;
	virtual Tile::BlockState getBlockState(LevelSource *level, int x, int y, int z) override;
	virtual Tile::BlockState getBlockState(int data);

protected:
	Icon *getTypeTexture(int type);

public:
	void registerIcons(IconRegister *iconRegister);
};