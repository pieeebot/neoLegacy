#pragma once
#include <memory>
#include "ChatPacket.h"
using namespace std;

class LivingEntity;
class Entity;
class Arrow;
class Fireball;
class Player;
class Explosion;

class DamageSource
{
public:
   
    static DamageSource *inFire;
    static DamageSource *onFire;
    static DamageSource *lava;
    static DamageSource *inWall;
    static DamageSource *drown;
    static DamageSource *starve;
    static DamageSource *cactus;
    static DamageSource *fall;
    static DamageSource *outOfWorld;
    static DamageSource *genericSource;
    static DamageSource *magic;
    static DamageSource *dragonbreath;
    static DamageSource *wither;
    static DamageSource *anvil;
    static DamageSource *fallingBlock;
    static DamageSource *hotFloor;

    static DamageSource *mobAttack(shared_ptr<LivingEntity> mob);
    static DamageSource *playerAttack(shared_ptr<Player> player);
    static DamageSource *arrow(shared_ptr<Arrow> arrow, shared_ptr<Entity> owner);
    static DamageSource *fireball(shared_ptr<Fireball> fireball, shared_ptr<Entity> owner);
    static DamageSource *thrown(shared_ptr<Entity> entity, shared_ptr<Entity> owner);
    static DamageSource *indirectMagic(shared_ptr<Entity> entity, shared_ptr<Entity> owner);
    static DamageSource *thorns(shared_ptr<Entity> source);
    static DamageSource *explosion(Explosion *explosion);

private:
   
    bool _bypassArmor;
   
    bool _bypassInvul;          
   
    float exhaustion;
   
    bool isFireSource;
    
    bool _isProjectile;         
    
    bool _scalesWithDifficulty;
    
    bool _isMagic;
   
    bool _isExplosion;
   
    bool _isCritical;
   
    public:
    ChatPacket::EChatPacketMessage m_msgId;

    ChatPacket::EChatPacketMessage m_msgWithItemId;

protected:
    DamageSource(ChatPacket::EChatPacketMessage msgId,
                 ChatPacket::EChatPacketMessage msgWithItemId = ChatPacket::e_ChatCustom);

public:
    virtual ~DamageSource() {}

  
    virtual shared_ptr<Entity> getDirectEntity();
    virtual shared_ptr<Entity> getEntity();
    virtual bool scalesWithDifficulty();
    virtual shared_ptr<ChatPacket> getDeathMessagePacket(shared_ptr<LivingEntity> player);
    virtual DamageSource *copy();

   
    bool isBypassArmor();
    bool isBypassInvul();
    bool isBypassMagic();           

    bool isFire();
    DamageSource *setIsFire();

    bool isFireProjectile();        
    DamageSource *setIsFireProjectile();

    bool isProjectile();           
    DamageSource *setProjectile();

    bool isMagic();
    DamageSource *setMagic();

    bool isExplosion();
    DamageSource *setExplosion();

    bool isCritical();
    DamageSource *setIsCritical();

    bool isCreativePlayer() const;

    float getFoodExhaustion();
    ChatPacket::EChatPacketMessage getMsgId();
    bool equals(DamageSource *source);

protected:
    DamageSource *bypassArmor();
    DamageSource *bypassInvul();
    DamageSource *setScalesWithDifficulty();
};