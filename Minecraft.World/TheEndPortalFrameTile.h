#pragma once
#include "Tile.h"

class TheEndPortalFrameTile : public Tile
{
public:
	static const int EYE_BIT = 4;
	static const wstring TEXTURE_EYE;

private:
	Icon *iconTop;
	Icon *iconEye;

public:
	TheEndPortalFrameTile(int id);
	virtual void createBlockStateDefinition() override;
	virtual int defaultBlockState() override;
	virtual int convertBlockStateToLegacyData(BlockState *state) override;
	virtual Tile::BlockState getBlockState(int data);
	virtual Tile::BlockState getBlockState(LevelSource *level, int x, int y, int z) override;
	virtual Icon *getTexture(int face, int data);
	void registerIcons(IconRegister *iconRegister);
	Icon *getEye();
	virtual bool isSolidRender(bool isServerLevel = false);
	virtual int getRenderShape();
	virtual void updateDefaultShape();
	virtual void addAABBs(Level *level, int x, int y, int z, AABB *box, AABBList *boxes, shared_ptr<Entity> source);
	static bool hasEye(int data);
	virtual int getResource(int data, Random *random, int playerBonusLevel);
	virtual void setPlacedBy(Level *level, int x, int y, int z, shared_ptr<LivingEntity> by, shared_ptr<ItemInstance> itemInstance);
	virtual bool hasAnalogOutputSignal();
	virtual int getAnalogOutputSignal(Level *level, int x, int y, int z, int dir);
};