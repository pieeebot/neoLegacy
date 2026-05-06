#include "stdafx.h"
#include "Rabbit.h"
#include "SynchedEntityData.h" 
#include "AttributeInstance.h"          
#include "SharedMonsterAttributes.h"
#include "net.minecraft.world.item.h"
#include "net.minecraft.world.entity.ai.goal.h"
#include "net.minecraft.world.entity.ai.navigation.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.level.biome.h"
#include "com.mojang.nbt.h"
#include "SoundTypes.h"
#include "MobGroupData.h"

class RabbitGroupData : public MobGroupData {
public:
    int groupSize = 0;
};

Rabbit::Rabbit(Level *level) : Animal(level)
{
    jumpCompletion = 0.0f;
    jumpTicks = 0;
    moreCarrotTicks = 0;

    
    this->defineSynchedData();
    this->registerAttributes();
    this->setHealth(this->getMaxHealth());
    
    this->setSize(0.4f, 0.5f);
    this->getNavigation()->setAvoidWater(true);

    // AI Goals
    goalSelector.addGoal(1, new FloatGoal(this));
    goalSelector.addGoal(2, new PanicGoal(this, 1.2f));
    goalSelector.addGoal(3, new BreedGoal(this, 0.8f));
    goalSelector.addGoal(4, new TemptGoal(this, 1.0f, Item::carrot_Id, false));
    goalSelector.addGoal(5, new FollowParentGoal(this, 1.1f));
    goalSelector.addGoal(6, new RandomStrollGoal(this, 0.6f));
    goalSelector.addGoal(7, new LookAtPlayerGoal(this, typeid(Player), 10.0f));
}

float Rabbit::getJumpCompletion(float partialTick) const {
    return jumpCompletion;
}

Rabbit::Variant Rabbit::getVariant() const {
    
    return (Variant)entityData->getInteger(DATA_TYPE_ID);
}

void Rabbit::setVariant(Variant v) {
    entityData->set(DATA_TYPE_ID, (int)v);
}

void Rabbit::defineSynchedData() {
    Animal::defineSynchedData();
    entityData->define(DATA_TYPE_ID, (int)Variant::BROWN);
}

void Rabbit::registerAttributes() {
    Animal::registerAttributes();
    getAttribute(SharedMonsterAttributes::MAX_HEALTH)->setBaseValue(3.0);
    
    getAttribute(SharedMonsterAttributes::MOVEMENT_SPEED)->setBaseValue(0.3); 
}

bool Rabbit::useNewAi() { 
    return true; 
}

void Rabbit::dropDeathLoot(bool wasKilledByPlayer, int lootingLevel) {
    
    int meatCount = random->nextInt(2) + random->nextInt(lootingLevel + 1);
    int meatId = isOnFire() ? Item::cooked_rabbit_Id : Item::rabbit_Id;
    spawnAtLocation(meatId, meatCount);

    
    int hideCount = random->nextInt(2) + random->nextInt(lootingLevel + 1);
    spawnAtLocation(Item::rabbit_hide_Id, hideCount);

    
    float footChance = 0.10f + (0.03f * lootingLevel);
    if (wasKilledByPlayer && random->nextFloat() < footChance) {
        spawnAtLocation(Item::rabbit_foot_Id, 1);
    }
}

void Rabbit::tick() {
    Animal::tick();

    if (!onGround) {
        jumpTicks++;
    } else {
        
        bool canJump = (tickCount % 5 == 0); 

        if (!getNavigation()->isDone() && jumpTicks == 0 && canJump) {
            
            
            this->yd = 0.32f; 
            
            
            float speed = (float)getAttribute(SharedMonsterAttributes::MOVEMENT_SPEED)->getValue();
            this->moveRelative(0.0f, 1.0f, speed); 

           
            this->hasImpulse = true;
        }
        jumpTicks = 0;
    }

    
    jumpCompletion = onGround ? jumpCompletion * 0.8f : (float)jumpTicks / 5.0f;
    if (jumpCompletion > 1.0f) jumpCompletion = 1.0f;
}

bool Rabbit::isFood(shared_ptr<ItemInstance> item) {
    if (item == nullptr || item->getItem() == nullptr) return false;
    
    
    int id = item->getItem()->id;
    return id == Item::carrot_Id || id == Item::carrot_Id || id == Item::golden_carrot_Id;
}

shared_ptr<AgableMob> Rabbit::getBreedOffspring(shared_ptr<AgableMob> target) {
    if (level->canCreateMore(GetType(), Level::eSpawnType_Breed)) {
        shared_ptr<Rabbit> offspring = std::make_shared<Rabbit>(level);
        shared_ptr<Rabbit> partner = dynamic_pointer_cast<Rabbit>(target);

        if (partner != nullptr) {
            
            offspring->setVariant(random->nextBoolean() ? this->getVariant() : partner->getVariant());
        }

        return offspring;
    }
    return nullptr;
}


int Rabbit::getAmbientSound() { return eSoundType_MOB_RABBIT_IDLE; }
int Rabbit::getHurtSound()    { return eSoundType_MOB_RABBIT_HURT;    }
int Rabbit::getDeathSound()   { return eSoundType_MOB_RABBIT_DEATH;   }
int Rabbit::getHopSound()   { return eSoundType_MOB_RABBIT_HOP;   }

void Rabbit::readAdditionalSaveData(CompoundTag *tag) {
    Animal::readAdditionalSaveData(tag);
    if (tag->contains(L"RabbitType")) {
        setVariant((Variant)tag->getInt(L"RabbitType"));
    }
}

void Rabbit::addAdditonalSaveData(CompoundTag *tag) {
    Animal::addAdditonalSaveData(tag);
    tag->putInt(L"RabbitType", (int)getVariant());
}

MobGroupData *Rabbit::finalizeMobSpawn(MobGroupData *groupData, int extraData) {
    RabbitGroupData *rabbitData = dynamic_cast<RabbitGroupData*>(groupData);
    if (rabbitData == nullptr) {
        rabbitData = new RabbitGroupData();
    } else {
       
        this->setAge(-24000);
    }
    rabbitData->groupSize++;

    Biome *biome = level->getBiome(Mth::floor(x), Mth::floor(z));
    int rnd = random->nextInt(100);

    if (biome == Biome::desert || biome == Biome::desertHills) {
        setVariant(Variant::GOLD);
    } else if (biome == Biome::plains || biome == Biome::forest || biome == Biome::forestHills) {
        if (rnd < 33) {
            setVariant(Variant::BLACK);
        } else if (rnd < 66) {
            setVariant(Variant::BROWN);
        } else {
            setVariant(Variant::SALT);
        }
    } else if (biome == Biome::taiga || biome == Biome::taigaHills) {
        if (rnd < 50) {
            setVariant(Variant::WHITE);
        } else {
            setVariant(Variant::WHITE_SPLOTCHED);
        }
    } else {
        // Default 
        if (rnd < 50) {
            setVariant(Variant::BROWN);
        } else if (rnd < 90) {
            setVariant(Variant::SALT);
        } else {
            setVariant(Variant::BLACK);
        }
    }

    return rabbitData;
}