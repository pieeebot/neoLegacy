#pragma once

#include "DispenserTile.h"

class DropperTile : public DispenserTile
{
private:
	DispenseItemBehavior *DISPENSE_BEHAVIOUR;

public:
	DropperTile(int id);
	virtual void createBlockStateDefinition() override;
	virtual int defaultBlockState() override;
	virtual int convertBlockStateToLegacyData(BlockState *state) override;
	virtual Tile::BlockState getBlockState(LevelSource *level, int x, int y, int z) override;
	virtual Tile::BlockState getBlockState(int data);

	virtual void registerIcons(IconRegister *iconRegister);

protected:
	virtual DispenseItemBehavior *getDispenseMethod(shared_ptr<ItemInstance> item);

public:
	virtual shared_ptr<TileEntity> newTileEntity(Level *level);

protected:
	virtual void dispenseFrom(Level *level, int x, int y, int z);
};