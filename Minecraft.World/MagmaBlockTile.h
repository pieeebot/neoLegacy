#pragma once
#include "Tile.h"

class MagmaBlockTile : public Tile
{
public:
	MagmaBlockTile(int id);

	virtual void tick(Level *level, int x, int y, int z, Random *random) override;
	virtual void stepOn(Level *level, int x, int y, int z, shared_ptr<Entity> entity) override;

	virtual void animateTick(Level *level, int x, int y, int z, Random *random) override;
	virtual int getTickDelay(Level *level) override;
	virtual int getLightColor(LevelSource *level, int x, int y, int z, int tileId = -1) override;
};