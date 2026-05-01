#include "stdafx.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.h"
#include "net.minecraft.world.entity.h"
#include "net.minecraft.world.damagesource.h"
#include "EnchantmentHelper.h"
#include "MagmaBlockTile.h"

MagmaBlockTile::MagmaBlockTile(int id) : Tile(id, Material::stone)
{
	setTicking(true);
}

void MagmaBlockTile::stepOn(Level *level, int x, int y, int z, shared_ptr<Entity> entity)
{
	if (entity == nullptr)
	{
		return;
	}

	if (entity->isFireImmune())
	{
		return;
	}

	if (entity->instanceof(eTYPE_LIVINGENTITY))
	{
		shared_ptr<LivingEntity> living = dynamic_pointer_cast<LivingEntity>(entity);
		if (living != nullptr && EnchantmentHelper::getWaterWalker(living) > 0)
		{
			return;
		}
	}

	entity->hurt(DamageSource::hotFloor, 1);
}

void MagmaBlockTile::tick(Level *level, int x, int y, int z, Random *random)
{
	int above = level->getTile(x, y + 1, z);
	if (above == Tile::water_Id || above == Tile::calmWater_Id)
	{
		level->removeTile(x, y + 1, z);
		level->playSound(x + 0.5f, y + 0.5f, z + 0.5f, eSoundType_RANDOM_FIZZ, 0.5f, 2.6f + (random->nextFloat() - random->nextFloat()) * 0.8f);

		for (int i = 0; i < 8; ++i)
		{
			float rx = x + 0.5f + (random->nextFloat() - random->nextFloat()) * 0.5f;
			float ry = y + 0.25f + random->nextFloat() * 0.25f;
			float rz = z + 0.5f + (random->nextFloat() - random->nextFloat()) * 0.5f;
			level->addParticle(eParticleType_largesmoke, rx, ry, rz, 0, 0, 0);
		}
	}
	level->addToTickNextTick(x, y, z, id, getTickDelay(level) + random->nextInt(10));
}

void MagmaBlockTile::animateTick(Level *level, int x, int y, int z, Random *random)
{
	// rain -> smoke particles
	if (level->isRainingAt(x, y + 1, z))
	{
		for (int i = 0; i < 4; ++i)
		{
			float rx = x + random->nextFloat();
			float ry = y + 1.0f + random->nextFloat() * 0.5f;
			float rz = z + random->nextFloat();
			level->addParticle(eParticleType_largesmoke, rx, ry, rz, 0, 0, 0);
		}
	}
}

int MagmaBlockTile::getTickDelay(Level *level)
{
	return 160;
}

int MagmaBlockTile::getLightColor(LevelSource *level, int x, int y, int z, int tileId)
{
	return 0x0f000f0; 
}