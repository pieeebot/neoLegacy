#pragma once
#include "HalfTransparentTile.h"

class Random;

class BarrierTile : public HalfTransparentTile
{
public:
	using HalfTransparentTile::isSolidRender;

	BarrierTile(int id, Material *material, bool allowSame);
	virtual int getResourceCount(Random *random);
	virtual int getRenderLayer();
	virtual bool isSolidRender();
	virtual bool isCubeShaped();
	virtual bool isSilkTouchable();
};
