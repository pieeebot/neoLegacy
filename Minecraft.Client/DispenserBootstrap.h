#pragma once
#include "../Minecraft.World/net.minecraft.world.item.h"
#include "../Minecraft.World/DispenserTile.h"
#include "../Minecraft.World/net.minecraft.core.h"
#include "../Minecraft.World/LevelEvent.h"

class DispenserBootstrap
{
public:
	static void bootStrap()
	{
		DispenserTile::REGISTRY.add(Item::arrow, new ArrowDispenseBehavior());
		DispenserTile::REGISTRY.add(Item::egg, new EggDispenseBehavior());	
		DispenserTile::REGISTRY.add(Item::snowball, new SnowballDispenseBehavior());
		DispenserTile::REGISTRY.add(Item::experience_bottle, new ExpBottleDispenseBehavior());

		DispenserTile::REGISTRY.add(Item::potion, new PotionDispenseBehavior());
		DispenserTile::REGISTRY.add(Item::spawn_egg, new SpawnEggDispenseBehavior());
		DispenserTile::REGISTRY.add(Item::fireworks, new FireworksDispenseBehavior());
		DispenserTile::REGISTRY.add(Item::fireball, new FireballDispenseBehavior());
		DispenserTile::REGISTRY.add(Item::boat, new BoatDispenseBehavior());
		DispenserTile::REGISTRY.add(Item::lava_bucket, new FilledBucketDispenseBehavior());
		DispenserTile::REGISTRY.add(Item::water_bucket, new FilledBucketDispenseBehavior());
		DispenserTile::REGISTRY.add(Item::bucket, new EmptyBucketDispenseBehavior());
		DispenserTile::REGISTRY.add(Item::flint_and_steel, new FlintAndSteelDispenseBehavior());
		DispenserTile::REGISTRY.add(Item::dye, new DyeDispenseBehavior());
		DispenserTile::REGISTRY.add(Item::items[Tile::tnt_Id], new TntDispenseBehavior());
	}
};