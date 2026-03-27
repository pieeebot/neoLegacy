#pragma once
#include "Bush.h"

class TallGrass2 : public Bush
{
	friend class Tile;
public:
	static const int SUNFLOWER = 0;
	static const int LILAC = 1;
	static const int TALL_GRASS = 2;
	static const int LARGE_FERN = 3;
	static const int ROSE_BUSH = 4;
	static const int PEONY = 5;
	static const int VARIANT_COUNT = 6;

	static const int UPPER_BIT = 8;

private:
	Icon* iconBottom[VARIANT_COUNT];
	Icon* iconTop[VARIANT_COUNT];

protected:
	TallGrass2(int id);

public:
	virtual void         updateDefaultShape() override;
	virtual Icon* getTexture(int face, int data) override;
	virtual Icon* getTexture(LevelSource* level, int x, int y, int z, int face) override;
	virtual void         registerIcons(IconRegister* iconRegister) override;
	virtual int          getRenderShape() override;
	virtual bool         blocksLight() override;
	virtual bool         isSolidRender(bool isServerLevel = false) override;
	virtual bool         isCubeShaped() override;

	virtual int          getColor(int auxData) override;
	virtual int          getColor() const override;
	virtual int          getColor(LevelSource* level, int x, int y, int z) override;
	virtual int          getColor(LevelSource* level, int x, int y, int z, int data) override;

	virtual void finalizePlacement(Level* level, int x, int y, int z, int data) override;
	virtual void         onPlace(Level* level, int x, int y, int z) override;
	virtual bool         mayPlace(Level* level, int x, int y, int z) override;
	virtual void         neighborChanged(Level* level, int x, int y, int z, int type) override;
	virtual bool         canSurvive(Level* level, int x, int y, int z) override;
	virtual void         tick(Level* level, int x, int y, int z, Random* random) override;

	virtual int          getResource(int data, Random* random, int playerBonusLevel) override;
	virtual int          getResourceCountForLootBonus(int bonusLevel, Random* random) override;
	virtual void         playerDestroy(Level* level, shared_ptr<Player> player, int x, int y, int z, int data) override;
	virtual void         playerWillDestroy(Level* level, int x, int y, int z, int data, shared_ptr<Player> player) override;

	virtual int          cloneTileData(Level* level, int x, int y, int z) override;
	virtual unsigned int getDescriptionId(int iData = -1) override;
	virtual int          getPistonPushReaction() override;

protected:
	virtual bool                     isSilkTouchable() override;
	virtual shared_ptr<ItemInstance> getSilkTouchItemInstance(int data) override;

private:
	int  getVariant(LevelSource* level, int x, int y, int z);
	bool isGrassColored(int variant);
};