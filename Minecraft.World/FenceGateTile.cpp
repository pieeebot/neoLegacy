#include "stdafx.h"
#include "FenceGateTile.h"
#include "AABB.h"
#include "BlockPos.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.h"
#include "net.minecraft.h"
#include "LevelEvent.h"

FenceGateTile::FenceGateTile(int id) : DirectionalTile(id, Material::wood, isSolidRender() )
{
}

void FenceGateTile::createBlockStateDefinition()
{
	if (!m_blockStateDefinition)
		m_blockStateDefinition = new BlockStateDefinition(this);
}

int FenceGateTile::defaultBlockState()
{
	return 0;
}

int FenceGateTile::convertBlockStateToLegacyData(BlockState *state)
{
	if (!state) return 0;
	return state->value & (DirectionalTile::DIRECTION_MASK | OPEN_BIT | POWERED_BIT);
}

Tile::BlockState FenceGateTile::getBlockState(int data)
{
	return Tile::BlockState(data & (DirectionalTile::DIRECTION_MASK | OPEN_BIT | POWERED_BIT));
}

Tile::BlockState FenceGateTile::getBlockState(LevelSource *level, int x, int y, int z)
{
	int data = level->getData(x, y, z) & (DirectionalTile::DIRECTION_MASK | OPEN_BIT | POWERED_BIT);
	int dir = DirectionalTile::getDirection(data);
	bool inWall = false;

	if (dir == Direction::NORTH || dir == Direction::SOUTH)
	{
		int westTile = level->getTile(x - 1, y, z);
		int eastTile = level->getTile(x + 1, y, z);
		inWall = (westTile == Tile::cobbleWall_Id) || (eastTile == Tile::cobbleWall_Id);
	}
	else
	{
		int northTile = level->getTile(x, y, z - 1);
		int southTile = level->getTile(x, y, z + 1);
		inWall = (northTile == Tile::cobbleWall_Id) || (southTile == Tile::cobbleWall_Id);
	}

	if (inWall)
	{
		data |= IN_WALL_BIT;
	}

	return Tile::BlockState(data);
}

void FenceGateTile::fillVirtualBlockStateProperties(Tile::BlockState *state, LevelSource *level, const BlockPos &pos)
{
	if (!state || !level) return;

	int x = pos.getX();
	int y = pos.getY();
	int z = pos.getZ();

	Tile::BlockState base = getBlockState(level, x, y, z);
	state->value = base.value;

	int dir = DirectionalTile::getDirection(base.value);
	bool inWall = false;

	if (dir == Direction::NORTH || dir == Direction::SOUTH)
	{
		int westTile = level->getTile(x - 1, y, z);
		int eastTile = level->getTile(x + 1, y, z);
		inWall = (westTile == Tile::cobbleWall_Id) || (eastTile == Tile::cobbleWall_Id);
	}
	else
	{
		int northTile = level->getTile(x, y, z - 1);
		int southTile = level->getTile(x, y, z + 1);
		inWall = (northTile == Tile::cobbleWall_Id) || (southTile == Tile::cobbleWall_Id);
	}

	if (inWall)
	{
		state->value |= IN_WALL_BIT;
	}
}

Icon *FenceGateTile::getTexture(int face, int data)
{
	return icon;
}

bool FenceGateTile::mayPlace(Level *level, int x, int y, int z)
{
	if (!level->getMaterial(x, y - 1, z)->isSolid()) return false;
	return Tile::mayPlace(level, x, y, z);
}



AABB *FenceGateTile::getAABB(Level *level, int x, int y, int z)
{
	int data = level->getData(x, y, z);
	if (isOpen(data))
	{
		return nullptr;
	}
	// 4J Brought forward change from 1.2.3 to fix hit box rotation
	if (data == Direction::NORTH || data == Direction::SOUTH)
	{
		return AABB::newTemp(x, y, z + 6.0f / 16.0f, x + 1, y + 1.5f, z + 10.0f / 16.0f);
	}
	return AABB::newTemp(x + 6.0f / 16.0f, y, z, x + 10.0f / 16.0f, y + 1.5f, z + 1);
	//return AABB::newTemp(x, y, z, x + 1, y + 1.5f, z + 1);
}

// 4J - Brought forward from 1.2.3 to fix hit box rotation
void FenceGateTile::updateShape(LevelSource *level, int x, int y, int z, int forceData, shared_ptr<TileEntity> forceEntity) // 4J added forceData, forceEntity param
{
	int data = getDirection(level->getData(x, y, z));
	if (data == Direction::NORTH || data == Direction::SOUTH)
	{
		setShape(0, 0, 6.0f / 16.0f, 1, 1.0f, 10.0f / 16.0f);
	}
	else
	{
		setShape(6.0f / 16.0f, 0, 0, 10.0f / 16.0f, 1.0f, 1);
	}
}

bool FenceGateTile::blocksLight()
{
	return false;
}

bool FenceGateTile::isSolidRender(bool isServerLevel)
{
	return false;
}

bool FenceGateTile::isCubeShaped()
{
	return false;
}

bool FenceGateTile::isPathfindable(LevelSource *level, int x, int y, int z)
{
	return isOpen(level->getData(x, y, z));
}

int FenceGateTile::getRenderShape()
{
	return Tile::SHAPE_FENCE_GATE;
}

void FenceGateTile::setPlacedBy(Level *level, int x, int y, int z, shared_ptr<LivingEntity> by, shared_ptr<ItemInstance> itemInstance)
{
	int dir = (((Mth::floor(by->yRot * 4 / (360) + 0.5)) & 3)) % 4;
	level->setData(x, y, z, dir, Tile::UPDATE_CLIENTS);
}

bool FenceGateTile::use(Level *level, int x, int y, int z, shared_ptr<Player> player, int clickedFace, float clickX, float clickY, float clickZ, bool soundOnly/*=false*/) // 4J added soundOnly param
{
	if (soundOnly)
	{
		// 4J - added - just do enough to play the sound
		level->levelEvent(player, LevelEvent::SOUND_OPEN_DOOR, x, y, z, 0);	// 4J - changed event to pass player rather than nullptr as the source of the event so we can filter the broadcast properly
		return false;
	}

	int data = level->getData(x, y, z);
	if (isOpen(data))
	{
		level->setData(x, y, z, data & ~OPEN_BIT, Tile::UPDATE_CLIENTS);
	}
	else
	{
		// open the door from the player
		int dir = (((Mth::floor(player->yRot * 4 / (360) + 0.5)) & 3)) % 4;
		int current = getDirection(data);
		if (current == ((dir + 2) % 4))
		{
			data = dir;
		}
		level->setData(x, y, z, data | OPEN_BIT, Tile::UPDATE_CLIENTS);
	}
	level->levelEvent(player, LevelEvent::SOUND_OPEN_DOOR, x, y, z, 0);
	return true;
}

void FenceGateTile::neighborChanged(Level *level, int x, int y, int z, int type)
{
	if (level->isClientSide) return;

	int data = level->getData(x, y, z);

	bool signal = level->hasNeighborSignal(x, y, z);
	if (signal || ((type > 0 && Tile::tiles[type]->isSignalSource()) || type == 0))
	{
		if (signal && !isOpen(data))
		{
			level->setData(x, y, z, data | OPEN_BIT, Tile::UPDATE_CLIENTS);
			level->levelEvent(nullptr, LevelEvent::SOUND_OPEN_DOOR, x, y, z, 0);
		}
		else if (!signal && isOpen(data))
		{
			level->setData(x, y, z, data & ~OPEN_BIT, Tile::UPDATE_CLIENTS);
			level->levelEvent(nullptr, LevelEvent::SOUND_OPEN_DOOR, x, y, z, 0);
		}
	}
}

bool FenceGateTile::isOpen(int data)
{
	return (data & OPEN_BIT) != 0;
}

void FenceGateTile::registerIcons(IconRegister *iconRegister)
{
	icon = iconRegister->registerIcon(getIconName());
}

bool FenceGateTile::shouldRenderFace(LevelSource *level, int x, int y, int z, int face)
{
	return true;
}