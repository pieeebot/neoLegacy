#include "stdafx.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.phys.h"
#include "net.minecraft.world.damagesource.h"
#include "net.minecraft.world.entity.ai.attributes.h"
#include "net.minecraft.world.entity.monster.h"
#include "net.minecraft.h"
#include "..\Minecraft.Client\Textures.h"
#include "Endermite.h"
#include "SoundTypes.h"
#include "Random.h" 
#include "net.minecraft.world.entity.ai.goal.h"
#include "net.minecraft.world.entity.ai.goal.target.h"
#include "net.minecraft.world.entity.ai.navigation.h"
#include "net.minecraft.world.entity.player.h"
#include "EnderMan.h"

class EndermiteHurtByTargetGoal : public HurtByTargetGoal
{
public:
    EndermiteHurtByTargetGoal(Endermite* mob, bool callHelp) : HurtByTargetGoal(mob, callHelp) {}

protected:
    virtual bool canAttack(shared_ptr<LivingEntity> target, bool allowInvulnerable) override
    {
        // If the entity that hit us is an Enderman, ignore it
        if (target != nullptr && target->instanceof(eTYPE_ENDERMAN))
        {
            return false;
        }
        
        // or use the standard logic 
        return HurtByTargetGoal::canAttack(target, allowInvulnerable);
    }
};


Endermite::Endermite(Level *level) : Monster(level), lifetime(0), playerSpawned(false)
{
	this->defineSynchedData();
	registerAttributes();
	setHealth(getMaxHealth());

	setSize(0.4f, 0.3f);
    xp = 3;

	getNavigation()->setCanOpenDoors(false);
    goalSelector.addGoal(1, new FloatGoal(this));
    goalSelector.addGoal(2, new MeleeAttackGoal(this, 1.0, false));
    goalSelector.addGoal(3, new RandomStrollGoal(this, 1.0));
    goalSelector.addGoal(4, new LookAtPlayerGoal(this, typeid(Player), 8));
    goalSelector.addGoal(5, new RandomLookAroundGoal(this));

    targetSelector.addGoal(1, new EndermiteHurtByTargetGoal(this, true));
    targetSelector.addGoal(2, new NearestAttackableTargetGoal(this, typeid(Player), 0, true));


}

void Endermite::registerAttributes()
{
    Monster::registerAttributes();

    
    getAttribute(SharedMonsterAttributes::FOLLOW_RANGE)->setBaseValue(8.0f);
    
    getAttribute(SharedMonsterAttributes::MAX_HEALTH)->setBaseValue(8.0f);
    getAttribute(SharedMonsterAttributes::MOVEMENT_SPEED)->setBaseValue(0.25f);
    getAttribute(SharedMonsterAttributes::ATTACK_DAMAGE)->setBaseValue(2.0f);
}

bool Endermite::makeStepSound()
{
	return false;
}

shared_ptr<Entity> Endermite::findAttackTarget()
{
#ifndef _FINAL_BUILD
	if(app.GetMobsDontAttackEnabled())
	{
		return shared_ptr<Player>();
	}
#endif

	double maxDist = 8;
	return level->getNearestAttackablePlayer(shared_from_this(), maxDist);
}

int Endermite::getAmbientSound()
{
	return eSoundType_MOB_SILVERFISH_AMBIENT;
}

int Endermite::getHurtSound()
{
	return eSoundType_MOB_SILVERFISH_HURT;
}

int Endermite::getDeathSound()
{
	return eSoundType_MOB_SILVERFISH_DEATH;
}

void Endermite::playStepSound(int xt, int yt, int zt, int t)
{
	playSound(eSoundType_MOB_SILVERFISH_STEP, 0.15f, 1);
}

int Endermite::getDeathLoot()
{
	return 0;
}

void Endermite::tick()
{
	yBodyRot = yRot; 

	Monster::tick();

	if (level->isClientSide)
	{
		
		for (int i = 0; i < 2; i++)
	{
		level->addParticle(eParticleType_ender, x + (random->nextDouble() - 0.5) * bbWidth, y + random->nextDouble() * bbHeight - 0.25f, z + (random->nextDouble() - 0.5) * bbWidth,
			(random->nextDouble() - 0.5) * 2, -random->nextDouble(), (random->nextDouble() - 0.5) * 2);
	}
	}
	else
	{
		if (!this->isPersistenceRequired()) 
		{
			++lifetime;
		}

		if (lifetime >= 2400)
		{
			remove(); 
		}
	}
}





bool Endermite::canSpawn()
{
	if (Monster::canSpawn())
	{
		shared_ptr<Player> nearestPlayer = level->getNearestPlayer(shared_from_this(), 5.0);
		return nearestPlayer == nullptr;
	}
	return false;
}

MobType Endermite::getMobType()
{
	return ARTHROPOD;
}

