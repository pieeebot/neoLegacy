#pragma once

#include "Monster.h"

class Endermite : public Monster
{
public:
	eINSTANCEOF GetType() { return eTYPE_ENDERMITE; }
	static Entity *create(Level *level) { return new Endermite(level); }

private:
	int lifetime;
	
	bool playerSpawned = false;

public:
	Endermite(Level *level);

protected:
	virtual void registerAttributes();
	virtual bool makeStepSound();
	virtual shared_ptr<Entity> findAttackTarget();

	virtual int getAmbientSound();
	virtual int getHurtSound();
	virtual int getDeathSound();

public:
	virtual void tick();
	virtual bool useNewAi() { return true; }
	

protected:
	virtual void playStepSound(int xt, int yt, int zt, int t);
	virtual int getDeathLoot();

public:
	virtual bool canSpawn();
	virtual MobType getMobType();
	bool isSpawnedByPlayer() { return playerSpawned; }
    void setSpawnedByPlayer(bool spawned) { playerSpawned = spawned; }


};