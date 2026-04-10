#include "stdafx.h"
#include "com.mojang.nbt.h"
#include "net.minecraft.world.level.tile.h"
#include "net.minecraft.world.phys.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.item.h"
#include "net.minecraft.world.entity.h"
#include "net.minecraft.world.entity.player.h"
#include "net.minecraft.world.entity.ai.attributes.h"
#include "net.minecraft.world.entity.monster.h"
#include "net.minecraft.world.damagesource.h"
#include "SharedConstants.h"
#include "Guardian.h"
#include "..\Minecraft.Client\Textures.h"
#include "..\Minecraft.World\Mth.h"
#include "..\Minecraft.World\net.minecraft.world.entity.animal.h"
#include "MobEffect.h"
#include "MobEffectInstance.h"


void Guardian::_init()
{
    clientSideTailAnimation    = 0.0f;
    clientSideTailAnimationO   = 0.0f;
    clientSideSpikesAnimation  = 0.0f;
    clientSideSpikesAnimationO = 0.0f;
    clientSideAttackTime       = 0.0f;
    clientSideTouchedGround    = false;

    xBodyRot = xBodyRotO = 0.0f;
    zBodyRot = zBodyRotO = 0.0f;

    tx = ty = tz = 0.0f;
    attackTimer  = 0;
}

Guardian::Guardian(Level *level) : WaterAnimal(level)
{
    this->defineSynchedData();
    registerAttributes();
    setHealth(getMaxHealth());
    _init();
    if (isElder()) {
       this->setSize(1.9975f, 1.9975f);
    }
    else {
        this->setSize(0.85f, 0.85f);
    }
}



void Guardian::registerAttributes()
{
    WaterAnimal::registerAttributes();
    getAttributes()->registerAttribute(SharedMonsterAttributes::ATTACK_DAMAGE);
    getAttribute(SharedMonsterAttributes::ATTACK_DAMAGE)->setBaseValue(6.0);
    getAttribute(SharedMonsterAttributes::MOVEMENT_SPEED)->setBaseValue(0.5);
    getAttribute(SharedMonsterAttributes::FOLLOW_RANGE)->setBaseValue(16.0);
    getAttribute(SharedMonsterAttributes::MAX_HEALTH)->setBaseValue(30.0);
}



void Guardian::defineSynchedData()
{
    WaterAnimal::defineSynchedData();
    entityData->define(16, (byte)0);
    entityData->define(17, 0);
}


//  NBT


void Guardian::readAdditionalSaveData(CompoundTag *tag)
{
    WaterAnimal::readAdditionalSaveData(tag);
    setElder(tag->getBoolean(L"Elder"));
}

void Guardian::addAdditonalSaveData(CompoundTag *tag)
{
    WaterAnimal::addAdditonalSaveData(tag);
    tag->putBoolean(L"Elder", isElder());
}



bool Guardian::isElder()
{
    return (entityData->getByte(16) & 4) != 0;
}

void Guardian::setElder(bool elder)
{
    byte flags = entityData->getByte(16);
    if (elder)
    {
        entityData->set(16, (byte)(flags | 4));

        
        this->setSize(1.9975f, 1.9975f);
        getAttribute(SharedMonsterAttributes::MOVEMENT_SPEED)->setBaseValue(0.30000001192092896);
        getAttribute(SharedMonsterAttributes::ATTACK_DAMAGE)->setBaseValue(8.0);
        getAttribute(SharedMonsterAttributes::MAX_HEALTH)->setBaseValue(80.0);
        setHealth(getMaxHealth());
    }
    else
    {
       entityData->set(16, (byte)(flags & ~4));
        this->setSize(0.85f, 0.85f);
    }
}


void Guardian::setElderClient()
{
    setElder(true);
    clientSideSpikesAnimation = clientSideSpikesAnimationO = 1.0f;
}

bool Guardian::isMoving()
{
    return (entityData->getByte(16) & 2) != 0;
}

void Guardian::setMoving(bool moving)
{
    byte flags = entityData->getByte(16);
    if (moving)
        entityData->set(16, (byte)(flags | 2));
    else
        entityData->set(16, (byte)(flags & ~2));
}



int Guardian::getTargetedEntityId()
{
    return entityData->getInteger(17);
}

void Guardian::setTargetedEntityId(int id)
{
    entityData->set(17, id);
    entityData->markDirty(17);
}

bool Guardian::hasTargetedEntity()
{
    return entityData->getInteger(17) != 0;
}

shared_ptr<LivingEntity> Guardian::getTargetedEntity()
{
    if (!hasTargetedEntity())
        return nullptr;

    if (level->isClientSide)
    {
        if (targetedEntity != nullptr)
            return targetedEntity;

        shared_ptr<Entity> entity = level->getEntity(getTargetedEntityId());
        if (entity != nullptr && dynamic_pointer_cast<LivingEntity>(entity))
        {
            targetedEntity = dynamic_pointer_cast<LivingEntity>(entity);
            return targetedEntity;
        }
        return nullptr;
    }

    shared_ptr<Entity> e = level->getEntity(getTargetedEntityId());
    return dynamic_pointer_cast<LivingEntity>(e);
}



int Guardian::getAttackDuration()
{
    // Java func_175464_ck
    return isElder() ? 60 : 80;
}

float Guardian::getAttackAnimationScale(float partialTicks)
{
    return (clientSideAttackTime + partialTicks) / (float)getAttackDuration();
}

// Java func_175471_a
float Guardian::getTailAnimation(float partialTicks)
{
    return clientSideSpikesAnimationO + (clientSideSpikesAnimation - clientSideSpikesAnimationO) * partialTicks;
}

// Java func_175469_o
float Guardian::getSpikesAnimation(float partialTicks)
{
    return isMoving() ? 1.0f : 0.0f;
}



int Guardian::getAmbientSound()  { return -1; }
int Guardian::getHurtSound()     { return -1; }
int Guardian::getDeathSound()    { return -1; }
float Guardian::getSoundVolume() { return 1.0f; }
int Guardian::getDeathLoot()     { return 0; }
bool Guardian::makeStepSound()   { return false; }

void Guardian::dropDeathLoot(bool wasKilledByPlayer, int playerBonusLevel)
{
    //loot (prismarine, sponge) 
}

float Guardian::getEyeHeight()
{
    return bbHeight * 0.5f;
}

bool Guardian::isInWater()
{
    return level->checkAndHandleWater(bb->grow(0, -0.6f, 0), Material::water, shared_from_this());
}

float Guardian::rotlerp(float a, float b, float max)
{
    float f = Mth::wrapDegrees(b - a);
    if (f >  max) f =  max;
    if (f < -max) f = -max;
    return a + f;
}

void Guardian::lookAt(shared_ptr<Entity> e, float yMax, float xMax)
{
    double d0 = e->x - x;
    double d1 = e->z - z;
    double d2;

    if (dynamic_pointer_cast<LivingEntity>(e))
    {
        shared_ptr<LivingEntity> living = dynamic_pointer_cast<LivingEntity>(e);
        d2 = living->y + living->getEyeHeight() - (y + getEyeHeight());
    }
    else
    {
        d2 = (e->bb->y0 + e->bb->y1) / 2.0 - (y + getEyeHeight());
    }

    double d3 = sqrt(d0 * d0 + d1 * d1);
    float f  = (float)(atan2(d1, d0) * 180.0 / PI) - 90.0f;
    float f1 = (float)(-(atan2(d2, d3) * 180.0 / PI));
    xRot = rotlerp(xRot, f1, xMax);
    yRot = rotlerp(yRot, f,  yMax);
}



void Guardian::serverAiStep()
{
    WaterAnimal::serverAiStep();

    if (isElder() && !level->isClientSide)
    {
        if (((tickCount + entityId) % 1200 == 0))
        {
            auto& players = level->players;
            for (int i = 0; i < (int)players.size(); i++)
            {
                shared_ptr<Player> player = players[i];
                if (!player) continue;

                if (distanceToSqr(player) >= 2500.0)
                    continue;

                if (player->abilities.invulnerable)
                    continue;

                
                MobEffectInstance *existing = player->getEffect(MobEffect::digSlowdown);
                if (existing == nullptr || existing->getAmplifier() < 2 || existing->getDuration() < 1200)
                {
                    player->addEffect(new MobEffectInstance(MobEffect::digSlowdown->id, 6000, 2));
                }
            }
        }
    }


    if (!hasTargetedEntity())
    {
        shared_ptr<Player> nearest = level->getNearestPlayer(x, y, z, 16.0);
        if (nearest != nullptr && canSee(nearest) && !nearest->abilities.invulnerable)
        {
            setTargetedEntityId(nearest->entityId);
            attackTimer = 0;
        }
    }

    shared_ptr<LivingEntity> target = getTargetedEntity();
    if (target != nullptr)
    {
        if (!target->isAlive() || distanceToSqr(target) > 256.0 || !canSee(target))
        {
            setTargetedEntityId(0);
            target = nullptr;
        }
    }

    if (target != nullptr)
    {
        lookAt(target, 90.0f, 90.0f);
        setMoving(false);
        tx = ty = tz = 0.0f;

        attackTimer++;

       
        if (attackTimer == 1)
        {
            level->broadcastEntityEvent(shared_from_this(), 21);
        }
        else if (attackTimer >= getAttackDuration())
        {
            
            float dmg = 1.0f;
            if (isElder()) dmg += 2.0f;

            target->hurt(DamageSource::mobAttack(dynamic_pointer_cast<LivingEntity>(shared_from_this())), dmg);
            target->hurt(DamageSource::magic,
                (float)getAttribute(SharedMonsterAttributes::ATTACK_DAMAGE)->getValue());

            setTargetedEntityId(0);
            attackTimer = 0;
        }
    }
    else
    {
       
        attackTimer = 0;
        setTargetedEntityId(0);

        noActionTime++;
        if (noActionTime > 100 || random->nextInt(50) == 0 ||
            (tx == 0.0f && ty == 0.0f && tz == 0.0f))
        {
            float angle = random->nextFloat() * (float)PI * 2.0f;
            tx = Mth::cos(angle) * 0.2f;
            ty = -0.1f + random->nextFloat() * 0.2f;
            tz = Mth::sin(angle) * 0.2f;
            noActionTime = 0;
        }

        setMoving(true);
    }

  
    if (isMoving())
    {
        xd += (tx - xd) * 0.1f;
        yd += (ty - yd) * 0.1f;
        zd += (tz - zd) * 0.1f;

        double horizontalMovement = sqrt(xd * xd + zd * zd);
        yBodyRot += ((-static_cast<float>(atan2(xd, zd)) * 180.0f / (float)PI) - yBodyRot) * 0.1f;
        yRot      = yBodyRot;
        zBodyRot  = zBodyRot + (float)PI * 0.05f;
        xBodyRot += ((-static_cast<float>(atan2(horizontalMovement, yd)) * 180.0f / (float)PI) - xBodyRot) * 0.1f;
    }
}



void Guardian::aiStep()
{
    if (level->isClientSide)
    {
        clientSideTailAnimationO = clientSideTailAnimation;

        if (!isInWater())
        {
            clientSideTailAnimation    = 2.0f;
            clientSideTouchedGround    = yd < 0.0 &&
                level->hasChunkAt(Mth::floor(x), Mth::floor(y - 1), Mth::floor(z));
        }
        else if (isMoving())
        {
            if (clientSideTailAnimation < 0.5f)
                clientSideTailAnimation = 4.0f;
            else
                clientSideTailAnimation += (0.5f - clientSideTailAnimation) * 0.1f;
        }
        else
        {
            clientSideTailAnimation += (0.125f - clientSideTailAnimation) * 0.2f;
        }

        clientSideSpikesAnimationO  = clientSideSpikesAnimation;
        clientSideSpikesAnimation  += clientSideTailAnimation;

        if (hasTargetedEntity() && clientSideAttackTime < (float)getAttackDuration())
            clientSideAttackTime++;
        else if (!hasTargetedEntity())
            clientSideAttackTime = 0.0f;
    }

    
    if (isInWater())
    {
        setAirSupply(300);
    }
    else if (onGround)
    {
        yd += 0.5;
        xd += (random->nextFloat() * 2.0f - 1.0f) * 0.4f;
        zd += (random->nextFloat() * 2.0f - 1.0f) * 0.4f;
        yRot       = random->nextFloat() * 360.0f;
        onGround   = false;
        hasImpulse = true;
    }

    if (hasTargetedEntity())
        yRot = yHeadRot;

    xBodyRotO = xBodyRot;
    zBodyRotO = zBodyRot;

    WaterAnimal::aiStep();
}

void Guardian::updateSize(bool elder)
{
    if (elder)
        this->setSize(1.9975f, 1.9975f);
    else
        setSize(0.85f, 0.85f);
}

void Guardian::travel(float xa, float ya)
{
    if (level->isClientSide)
    {
        if (isInWater())
        {
            move(xd, yd, zd);
            xd *= 0.9f;
            yd *= 0.9f;
            zd *= 0.9f;
        }
        else
        {
            WaterAnimal::travel(xa, ya);
        }
    }
    else
    {
        if (isInWater())
        {
            moveRelative(xa, ya, 0.1f);
            move(xd, yd, zd);
            xd *= 0.9f;
            yd *= 0.9f;
            zd *= 0.9f;
        }
        else
        {
            WaterAnimal::travel(xa, ya);
        }
    }
    
}

MobGroupData *Guardian::finalizeMobSpawn(MobGroupData *groupData, int extraData)
{
    WaterAnimal::finalizeMobSpawn(groupData, extraData);
    if (extraData == 1)
        setElder(true);
    return groupData;
}