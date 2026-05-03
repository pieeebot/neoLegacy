#pragma once

#include "BaseEntityTile.h"

class DaylightDetectorTile : public BaseEntityTile
{
	friend class ChunkRebuildData;
private:
	Icon *icons[2];
	bool inverted;

public:
	DaylightDetectorTile(int id, bool inverted);

	virtual void createBlockStateDefinition() override;
	virtual int defaultBlockState() override;
	virtual int convertBlockStateToLegacyData(BlockState *state) override;
	virtual Tile::BlockState getBlockState(int data);
	virtual Tile::BlockState getBlockState(LevelSource *level, int x, int y, int z) override;

	virtual void updateDefaultShape(); // 4J Added override
	virtual void updateShape(LevelSource *level, int x, int y, int z, int forceData = -1, shared_ptr<TileEntity> forceEntity = shared_ptr<TileEntity>());
	virtual int getSignal(LevelSource *level, int x, int y, int z, int dir);
	virtual void tick(Level *level, int x, int y, int z, Random *random);
	virtual void neighborChanged(Level *level, int x, int y, int z, int type);
	virtual void onPlace(Level *level, int x, int y, int z);
	virtual void updateSignalStrength(Level *level, int x, int y, int z);
	virtual bool isCubeShaped();
	virtual bool isSolidRender(bool isServerLevel = false);
	virtual bool isSignalSource();
	virtual shared_ptr<TileEntity> newTileEntity(Level *level);
	virtual Icon *getTexture(int face, int data);
	virtual void registerIcons(IconRegister *iconRegister);
	bool use(Level* level, int x, int y, int z, shared_ptr<Player> player, int clickedFace, float clickX, float clickY, float clickZ, bool soundOnly = false); // 4J added soundOnly param
};