#pragma once
using namespace std;
#include "WaterAnimal.h"

class Level;
class Player;

class Guardian : public WaterAnimal
{
public:
    eINSTANCEOF GetType() { return eTYPE_GUARDIAN; }
    static Entity *create(Level *level) { return new Guardian(level); }

    void _init();

    
    float clientSideTailAnimation;
    float clientSideTailAnimationO;
    float clientSideSpikesAnimation;
    float clientSideSpikesAnimationO;
    float clientSideAttackTime;
    bool  clientSideTouchedGround;

    
    float xBodyRot, xBodyRotO;
    float zBodyRot, zBodyRotO;

    Guardian(Level *level);

protected:
    virtual void registerAttributes();
    virtual void defineSynchedData();
    virtual int   getAmbientSound();
    virtual int   getHurtSound();
    virtual int   getDeathSound();
    virtual float getSoundVolume();
    virtual int   getDeathLoot();
    virtual bool  makeStepSound();
    virtual void  dropDeathLoot(bool wasKilledByPlayer, int playerBonusLevel);
    virtual void  serverAiStep();

public:
    virtual bool  isInWater();
    virtual void  aiStep();
    virtual void  travel(float xa, float ya);
    virtual void  readAdditionalSaveData(CompoundTag *tag);
    virtual void  addAdditonalSaveData(CompoundTag *tag);
    virtual float getEyeHeight();
    void updateSize(bool isElder);
    
    bool isElder();
    void setElder(bool elder);
    void setElderClient();      

    bool isMoving();
    void setMoving(bool moving);

    
    int                        getTargetedEntityId();
    void                       setTargetedEntityId(int entityId);
    bool                       hasTargetedEntity();
    shared_ptr<LivingEntity>   getTargetedEntity();

   virtual MobGroupData *finalizeMobSpawn(MobGroupData *groupData, int extraData = 0) override;
    int   getAttackDuration();                          
    float getTailAnimation(float partialTicks);
    float getSpikesAnimation(float partialTicks);
    float getAttackAnimationScale(float partialTicks);

protected:
    void lookAt(shared_ptr<Entity> e, float yMax, float xMax);

private:
    float rotlerp(float a, float b, float max);

    float tx, ty, tz;
    int   attackTimer;
    shared_ptr<LivingEntity> targetedEntity;
};