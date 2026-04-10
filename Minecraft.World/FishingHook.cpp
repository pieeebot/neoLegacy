#include "stdafx.h"
#include "net.minecraft.stats.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.level.tile.h"
#include "net.minecraft.world.phys.h"
#include "net.minecraft.world.entity.player.h"
#include "net.minecraft.world.item.h"
#include "net.minecraft.world.entity.item.h"
#include "net.minecraft.world.entity.h"
#include "net.minecraft.world.damagesource.h"
#include "com.mojang.nbt.h"
#include "FishingHook.h"
#include "SoundTypes.h"
#include "../Minecraft.World/FishingHelper.h"
#include "../Minecraft.World/EnchantmentHelper.h"
#include "../Minecraft.World/Enchantment.h"
#include "../Minecraft.World/ItemInstance.h"
#include "SynchedEntityData.h"
#include <algorithm>


const int FishingHook::DATA_FLAG_RENDER_CLIENT_FX = 20;
const int FishingHook::DATA_FISH_APPROACH_ANGLE = 21;
const int FishingHook::DATA_WAKE_TIMER = 22;
const int FishingHook::DATA_NIBBLE_TIMER = 23;
const int FishingHook::DATA_FLAG_NIBBLE = 24;


// 4J - added common ctor code. 
void FishingHook::_init()
{
	// 4J Stu - This function call had to be moved here from the Entity ctor to ensure that
	// the derived version of the function is called
	this->defineSynchedData();

	xTile = -1;
	yTile = -1;
	zTile = -1;
	lastTile = 0;
	inGround = false;
	shakeTime = 0;
	flightTime = 0;

	// TU 31: Fishing rod now has a random nibble timer between 5 and 30 seconds, instead of a 1/500 chance every tick (plus modifiers). Source: https://minecraft.wiki/w/Fishing
	hookedIn = nullptr;
	previousItem = nullptr;

	fishApproachAngle = 0.0f;
	wakeTimer = 0;
	nibble = 0;
	nibbleTimer = 0;

	lureLevel = 0;
	luckLevel = 0;

	lSteps = 0;
	lx = 0.0;
	ly = 0.0;
	lz = 0.0;
	lyr = 0.0;
	lxr = 0.0;
	lxd = 0.0;
	lyd = 0.0; 
	lzd = 0.0;
	owner = nullptr;
	life = 0;

	setSize(0.25f, 0.25f);
	noCulling = true;
}

FishingHook::FishingHook(Level *level) : Entity( level )
{
	_init();
}

FishingHook::FishingHook(Level *level, double x, double y, double z,  std::shared_ptr<Player> owner) : Entity( level )
{
	_init();

	this->owner = owner;
	// 4J Stu - Moved this outside the ctor
	//owner->fishing = dynamic_pointer_cast<FishingHook>( shared_from_this() );

	getEnchantLevels();

	setPos(x, y, z);
}

FishingHook::FishingHook(Level *level,  std::shared_ptr<Player> mob) : Entity( level )
{
	_init();

	owner = mob;
	// 4J Stu - Moved this outside the ctor
	//owner->fishing = dynamic_pointer_cast<FishingHook>( shared_from_this() );
	getEnchantLevels();

	moveTo(mob->x, mob->y + 1.62 - mob->heightOffset, mob->z, mob->yRot, mob->xRot);


	x -= Mth::cos(yRot / 180 * PI) * 0.16f;
	y -= 0.1f;
	z -= Mth::sin(yRot / 180 * PI) * 0.16f;
	setPos(x, y, z);
	heightOffset = 0;


	float speed = 0.4f;
	xd = (-Mth::sin(yRot / 180 * PI) * Mth::cos(xRot / 180 * PI)) * speed;
	zd = (Mth::cos(yRot / 180 * PI) * Mth::cos(xRot / 180 * PI)) * speed;
	yd = (-Mth::sin(xRot / 180 * PI)) * speed;

	shoot(xd, yd, zd, 1.5f, 1);
}

void FishingHook::getEnchantLevels() {
	if (this->owner == nullptr) return;
	std::shared_ptr<ItemInstance> fishingRod = owner->getSelectedItem();
	// TODO; Account for luck effect once implemented.
	this->luckLevel = EnchantmentHelper::getEnchantmentLevel(65, fishingRod); // Luck of the sea
	this->lureLevel = EnchantmentHelper::getEnchantmentLevel(64, fishingRod); // Lure
}

void FishingHook::defineSynchedData()
{
	entityData->define(FishingHook::DATA_FLAG_RENDER_CLIENT_FX, (short)0);
	entityData->define(FishingHook::DATA_FISH_APPROACH_ANGLE, 0.0f);
	entityData->define(FishingHook::DATA_WAKE_TIMER, (short)0);
	entityData->define(FishingHook::DATA_NIBBLE_TIMER, (short)0);
	entityData->define(FishingHook::DATA_FLAG_NIBBLE, (short)0);
}

bool FishingHook::shouldRenderAtSqrDistance(double distance)
{
	double size = bb->getSize() * 4;
	size *= 64.0f;
	return distance < size * size;
}

void FishingHook::shoot(double xd, double yd, double zd, float pow, float uncertainty)
{
	float dist = static_cast<float>(sqrt(xd * xd + yd * yd + zd * zd));

	xd /= dist;
	yd /= dist;
	zd /= dist;

	xd += (random->nextGaussian()) * 0.0075f * uncertainty;
	yd += (random->nextGaussian()) * 0.0075f * uncertainty;
	zd += (random->nextGaussian()) * 0.0075f * uncertainty;

	xd *= pow;
	yd *= pow;
	zd *= pow;

	this->xd = xd;
	this->yd = yd;
	this->zd = zd;

	double sd = sqrt(xd * xd + zd * zd);

	yRotO = yRot = static_cast<float>(atan2(xd, zd) * 180 / PI);
	xRotO = xRot = static_cast<float>(atan2(yd, sd) * 180 / PI);
	life = 0;
}

void FishingHook::lerpTo(double x, double y, double z, float yRot, float xRot, int steps)
{
	lx = x;
	ly = y;
	lz = z;
	lyr = yRot;
	lxr = xRot;

	lSteps = steps;

	xd = lxd;
	yd = lyd;
	zd = lzd;
}

void FishingHook::lerpMotion(double xd, double yd, double zd)
{
	lxd = this->xd = xd;
	lyd = this->yd = yd;
	lzd = this->zd = zd;
}

void FishingHook::tick()
{
	Entity::tick();

	if (lSteps > 0)
	{
		double xt = x + (lx - x) / lSteps;
		double yt = y + (ly - y) / lSteps;
		double zt = z + (lz - z) / lSteps;

		double yrd = Mth::wrapDegrees(lyr - yRot);

		yRot += static_cast<float>((yrd) / lSteps);
		xRot += static_cast<float>((lxr - xRot) / lSteps);

		lSteps--;
		setPos(xt, yt, zt);
		setRot(yRot, xRot);
		return;
	}

	if (!level->isClientSide)
	{
		
		std::shared_ptr<ItemInstance> selectedItem = owner->getSelectedItem();
		if (this->previousItem == nullptr) {
			this->previousItem = selectedItem;
		}
		if (owner->removed || !owner->isAlive() || selectedItem == nullptr || selectedItem->getItem() != Item::fishingRod || distanceToSqr(owner) > 32 * 32 || selectedItem != this->previousItem)
		{
			remove();
			owner->fishing = nullptr;
			return;
		}

		this->previousItem = selectedItem;

		if (hookedIn != nullptr)
		{
			if (hookedIn->removed) hookedIn = nullptr;
			else
			{
				x = hookedIn->x;
				y = hookedIn->bb->y0 + hookedIn->bbHeight * 0.8;
				z = hookedIn->z;
				return;
			}
		}
	}

	if (shakeTime > 0) shakeTime--;

	if (inGround) 
	{
		int tile = level->getTile(xTile, yTile, zTile);
		if (tile != lastTile)
		{
			life++;
			if (life == 20 * 60) remove();
			return;
		}
		else
		{
			inGround = false;

			xd *= random->nextFloat() * 0.2f;
			yd *= random->nextFloat() * 0.2f;
			zd *= random->nextFloat() * 0.2f;
			life = 0;
			flightTime = 0;
		}
	}
	else
	{
		flightTime++;
	}

	Vec3 *from = Vec3::newTemp(x, y, z);
	Vec3 *to = Vec3::newTemp(x + xd, y + yd, z + zd);
	HitResult *res = level->clip(from, to);

	from = Vec3::newTemp(x, y, z);
	to = Vec3::newTemp(x + xd, y + yd, z + zd);
	if (res != nullptr) 
	{
		to = Vec3::newTemp(res->pos->x, res->pos->y, res->pos->z);
	}
	std::shared_ptr<Entity> hitEntity = nullptr;
	vector< std::shared_ptr<Entity> > *objects = level->getEntities(shared_from_this(), bb->expand(xd, yd, zd)->grow(1, 1, 1));
	double nearest = 0;
	for (auto it = objects->begin(); it != objects->end(); it++)
	{
		std::shared_ptr<Entity> e = *it; // objects->at(i);
		if (!e->isPickable() || (e == owner && flightTime < 5)) continue;

		float rr = 0.3f;
		AABB *bb = e->bb->grow(rr, rr, rr);
		HitResult *p = bb->clip(from, to);
		if (p != nullptr)
		{
			double dd = from->distanceTo(p->pos);
			if (dd < nearest || nearest == 0)
			{
				hitEntity = e;
				nearest = dd;
			}
			delete p;
		}
	}

	if (hitEntity != nullptr)
	{
		delete res;
		res = new HitResult(hitEntity);
	}

	if (res != nullptr)
	{
		if (res->entity != nullptr) 
		{
			// 4J Stu Move fix for : fix for #48587 - CRASH: Code: Gameplay: Hitting another player with the fishing bobber crashes the game. [Fishing pole, line]
			// Incorrect dynamic_pointer_cast used around the shared_from_this()
			DamageSource *damageSource = DamageSource::thrown(shared_from_this(), owner);
			if (res->entity->hurt(damageSource, 0))
			{
				hookedIn = res->entity;
			}
			delete damageSource;
		}
		else
		{
			inGround = true;
		}
	}
	delete res;

	if (inGround) return;

	move(xd, yd, zd);

	double sd = sqrt(xd * xd + zd * zd);
	yRot = static_cast<float>(atan2(xd, zd) * 180 / PI);
	xRot = static_cast<float>(atan2(yd, sd) * 180 / PI);

	while (xRot - xRotO < -180)
		xRotO -= 360;
	while (xRot - xRotO >= 180)
		xRotO += 360;

	while (yRot - yRotO < -180)
		yRotO -= 360;
	while (yRot - yRotO >= 180)
		yRotO += 360;

	xRot = xRotO + (xRot - xRotO) * 0.2f;
	yRot = yRotO + (yRot - yRotO) * 0.2f;


	float inertia = 0.92f;

	if (onGround || horizontalCollision)
	{
		inertia = 0.5f;
	}

	int steps = 10;
	double waterPercentage = 0;
	for (int i = 0; i < steps; i++)
	{
		double y0 = bb->y0 + (bb->y1 - bb->y0) * (i + 0) / steps - 2 / 16.0f + 2 / 16.0f;
		double y1 = bb->y0 + (bb->y1 - bb->y0) * (i + 1) / steps - 2 / 16.0f + 2 / 16.0f;
		AABB *bb2 = AABB::newTemp(bb->x0, y0, bb->z0, bb->x1, y1, bb->z1);
		if (level->containsLiquid(bb2, Material::water))
		{
			waterPercentage += 1.0 / steps;
		}
	}

	if (waterPercentage > 0)
	{
		if (!level->isClientSide) {
			catchingFish();
			
		}
	}
	if (level->isClientSide) {
		applyClientFX();
		if (entityData->getShort(FishingHook::DATA_FLAG_NIBBLE) != 0)
		{
			yd -= random->nextFloat() * random->nextFloat() * random->nextFloat() * 0.2;
		}
	}
	else if (nibble > 0)
	{
		yd -= random->nextFloat() * random->nextFloat() * random->nextFloat() * 0.2;
	}

	double bob = waterPercentage * 2 - 1;
	yd += 0.04f * bob;
	if (waterPercentage > 0)
	{
		inertia *= 0.9;
		yd *= 0.8;
	}

	xd *= inertia;
	yd *= inertia;
	zd *= inertia;

	setPos(x, y, z);
}

void FishingHook::addAdditonalSaveData(CompoundTag *tag)
{
	tag->putShort(L"xTile", static_cast<short>(xTile));
	tag->putShort(L"yTile", static_cast<short>(yTile));
	tag->putShort(L"zTile", static_cast<short>(zTile));
	tag->putByte(L"inTile", static_cast<byte>(lastTile));
	tag->putByte(L"shake", static_cast<byte>(shakeTime));
	tag->putByte(L"inGround", static_cast<byte>(inGround ? 1 : 0));
	tag->putFloat(L"fishApproachAngle", fishApproachAngle);
	tag->putShort(L"wakeTimer",  static_cast<short>(wakeTimer));
	tag->putShort(L"nibbleTimer", static_cast<short>(nibbleTimer));
	tag->putShort(L"nibble", static_cast<short>(nibble));
}

void FishingHook::catchingFish() {
	int timerSubtractor = 1;
	// Being under roof increases fishing time
	if (!(level->canSeeSky(Mth::floor(x), Mth::floor(y) + 1, Mth::floor(z)) && random->nextInt(2) == 0)) {
		timerSubtractor--;
	}
	// TU 31: Raining affects the nibble timer by random chance rather than being a fixed rate. Source: https://minecraft.wiki/w/Fishing
	if (level->isRainingAt( Mth::floor(x), Mth::floor(y) + 1, Mth::floor(z)) && random->nextInt(4) == 0) {
		timerSubtractor++;
	}

	if (nibble > 0)
	{
		nibble -= 1;
		if (nibble <= 0)
		{
			nibbleTimer = 0;
			wakeTimer = 0;
		}
	}

	else
	{	
		if (wakeTimer > 0)
		{
			wakeTimer -= timerSubtractor;

			if (wakeTimer <= 0)
			{
				yd -= 0.2f;
				playSound(eSoundType_RANDOM_SPLASH, 0.25f, 1.0f + (random->nextFloat() - random->nextFloat()) * 0.4f);
				nibble = random->nextInt(11) + 30;
			}
			else
			{
				fishApproachAngle += (float)(random->nextGaussian() * 4.0);
			}
		}
		else if ( nibbleTimer>0 ) {
			nibbleTimer -= timerSubtractor;
			if (nibbleTimer <= 0)
			{
				fishApproachAngle = random->nextFloat() * 360.0f;
				wakeTimer = random->nextInt(61) + 20;
			}
		}
		else
		{
			nibbleTimer = random->nextInt(801) + 100 - lureLevel * 100;
		}
	}
	updateSynchedData();
}

// I guess I have to use UK spelling :(
void FishingHook::updateSynchedData() {
	short nibbleFlag = (nibble > 0) ? 1 : 0;
	if (nibble > 0 && entityData->getShort(FishingHook::DATA_FLAG_RENDER_CLIENT_FX) != 1) {
		entityData->set(FishingHook::DATA_FLAG_RENDER_CLIENT_FX, 1); // Render Nibble FX
	}
	// Flags and synced data are only updated if they have changed.
	else if (wakeTimer > 0 && entityData->getShort(FishingHook::DATA_FLAG_RENDER_CLIENT_FX) != 2) {
		entityData->set(FishingHook::DATA_FLAG_RENDER_CLIENT_FX, 2); // Render Wake FX
	}

	else if (nibbleTimer > 0 && entityData->getShort(FishingHook::DATA_FLAG_RENDER_CLIENT_FX) != 3) {
		entityData->set(FishingHook::DATA_FLAG_RENDER_CLIENT_FX, 3); // Render Fishing FX
	}

	else if (entityData->getShort(FishingHook::DATA_FLAG_RENDER_CLIENT_FX) != 0){
		entityData->set(FishingHook::DATA_FLAG_RENDER_CLIENT_FX, 0); // Render Nothing
	}

	if (entityData->getFloat(FishingHook::DATA_FISH_APPROACH_ANGLE) != fishApproachAngle) {
		entityData->set(FishingHook::DATA_FISH_APPROACH_ANGLE, fishApproachAngle);
	}
	if (entityData->getShort(FishingHook::DATA_NIBBLE_TIMER) != nibbleTimer) {
		entityData->set(FishingHook::DATA_NIBBLE_TIMER, nibbleTimer);
	}
	if (entityData->getShort(FishingHook::DATA_WAKE_TIMER) != wakeTimer) {
		entityData->set(FishingHook::DATA_WAKE_TIMER, wakeTimer);
	}

	// Nibble flag is only updated if it has changed.
	if (entityData->getShort(FishingHook::DATA_FLAG_NIBBLE) != nibbleFlag) {
		entityData->set(FishingHook::DATA_FLAG_NIBBLE, nibbleFlag);
	}
}

void FishingHook::applyClientFX() {
	float f;
	float particleY;
	float particleZ;
	float particleX;
	float xDir;
	float zDir;
	float xVel;
	float zVel;
	float yt = static_cast<float>(Mth::floor(bb->y0) + 1.0);
	int clientWakeTimer;
	int clientNibbleTimer;


	switch (entityData->getShort(FishingHook::DATA_FLAG_RENDER_CLIENT_FX)) {
		case 1:
			yd -= 0.2f;
			playSound(eSoundType_RANDOM_SPLASH, 0.25f, 1.0f + (random->nextFloat() - random->nextFloat()) * 0.4f);
			for (int i = 0; i < 1 + bbWidth * 20; i++)
			{
				level->addParticle(eParticleType_bubble, x, yt, z, bbWidth, 0, bbWidth);
			}
			for (int i = 0; i < 1 + bbWidth * 20; i++)
			{
				level->addParticle(eParticleType_wake, x, yt, z, bbWidth, 0, bbWidth);
			}
			break;
		case 2:
			f = entityData->getFloat(FishingHook::DATA_FISH_APPROACH_ANGLE) * 0.017453292f;
			xDir = Mth::sin(f);
			zDir = Mth::cos(f);
			clientWakeTimer = entityData->getShort(FishingHook::DATA_WAKE_TIMER);
			particleX = x + (xDir * (float)clientWakeTimer * 0.1f);
			particleZ = z+ (zDir * (float)clientWakeTimer * 0.1f);

			if (random->nextFloat() < 0.15f)
			{
				level->addParticle(eParticleType_bubble, particleX, yt - 0.10000000149011612, particleZ, xDir, 0.1, zDir);
			}

			zVel = xDir * 0.04F;
			xVel = zDir * 0.04F;

			level->addParticle(eParticleType_wake, particleX, yt, particleZ, xVel, 0.01, (-zVel));
			level->addParticle(eParticleType_wake, particleX, yt, particleZ, (-xVel), 0.01, zVel);
			break;
		case 3:
			f = 0.15F;
			clientNibbleTimer = entityData->getShort(FishingHook::DATA_NIBBLE_TIMER);
			if (clientNibbleTimer < 20)
			{
				f = f + (float)(20 - clientNibbleTimer) * 0.05f;
			}
			else if (clientNibbleTimer < 40)
			{
				f = (float)(40 - clientNibbleTimer) * 0.02f;
			}
			else if (clientNibbleTimer < 60)
			{
				f = (float)(60 - clientNibbleTimer) * 0.01f;
			}

			if (random->nextFloat() < f)
			{
				xDir = random->nextFloat() * 360.0f * 0.017453292f;
				zDir = (random->nextFloat() * 45.0f) + 25.0f;
				particleX = x + (Mth::sin(xDir) * zDir * 0.1f);
				particleY = Mth::floor(bb->y0) + 1.0;
				particleZ = z + (Mth::cos(xDir) * zDir * 0.1f);
				for (int i = 0; i < 2 + random->nextInt(2); i++)
				{
					level->addParticle(eParticleType_splash, particleX, particleY, particleZ, 0.10000000149011612, 0.0, 0.10000000149011612);
				}
			}
			break;
	}
}

void FishingHook::readAdditionalSaveData(CompoundTag *tag)
{
	xTile = tag->getShort(L"xTile");
	yTile = tag->getShort(L"yTile");
	zTile = tag->getShort(L"zTile");
	lastTile = tag->getByte(L"inTile") & 0xff;
	shakeTime = tag->getByte(L"shake") & 0xff;
	inGround = tag->getByte(L"inGround") == 1;
	fishApproachAngle = tag->getFloat(L"fishApproachAngle");
	wakeTimer = tag->getShort(L"wakeTimer");
	nibbleTimer = tag->getShort(L"nibbleTimer");
	nibble = tag->getShort(L"nibble");
	entityData->set(FishingHook::DATA_FLAG_NIBBLE, (nibble > 0) ? 1 : 0);
}

float FishingHook::getShadowHeightOffs()
{
	return 0;
}

int FishingHook::retrieve()
{
	if (level->isClientSide) return 0;

	int dmg = 0;
	if (hookedIn != nullptr)
	{
		double xa = owner->x - x;
		double ya = owner->y - y;
		double za = owner->z - z;

		double dist = sqrt(xa * xa + ya * ya + za * za);
		double speed = 0.1;
		hookedIn->xd += xa * speed;
		hookedIn->yd += ya * speed + sqrt(dist) * 0.08;
		hookedIn->zd += za * speed;
		dmg = 3;
	}
	else if (nibble > 0)
	{
		FishingHelper* helper = FishingHelper::getInstance();
		std::shared_ptr<ItemInstance> fishingItemInstance = helper->getCatch(luckLevel, lureLevel, random);
		std::shared_ptr<ItemEntity> ie = std::make_shared<ItemEntity>(this->Entity::level, x, y, z, fishingItemInstance);
		double xa = owner->x - x;
		double ya = owner->y - y;
		double za = owner->z - z;

		double dist = sqrt(xa * xa + ya * ya + za * za);
		double speed = 0.1;
		ie->Entity::xd = xa * speed;
		ie->Entity::yd = ya * speed + sqrt(dist) * 0.08;
		ie->Entity::zd = za * speed;
		level->addEntity(ie);
		owner->level->addEntity(std::make_shared<ExperienceOrb>(owner->level, owner->x, owner->y + 0.5f, owner->z + 0.5f, random->nextInt(6) + 1)); // 4J Stu brought forward from 1.4
		dmg = 1;
	}
	if (inGround) dmg = 2;

	remove();
	return dmg;
}

// 4J Stu - Brought forward from 1.4
void FishingHook::remove()
{
	Entity::remove();
	if (owner != nullptr) owner->fishing = nullptr;
}