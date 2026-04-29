#include "stdafx.h"
#include "BarrierTile.h"

BarrierTile::BarrierTile(int id, Material *material, bool allowSame) : HalfTransparentTile(id, L"barrier", material, allowSame)
{
}

int BarrierTile::getResourceCount(Random *random)
{
	return 0;
}

int BarrierTile::getRenderLayer()
{
	return 0;
}

bool BarrierTile::isSolidRender()
{
	return false;
}

bool BarrierTile::isCubeShaped()
{
	return false;
}

bool BarrierTile::isSilkTouchable()
{
	return false;
}