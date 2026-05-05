#include "stdafx.h"
#include "MobAppearanceParticle.h"
#include "EntityRenderDispatcher.h"
#include "../Minecraft.World/Guardian.h"
#include "../Minecraft.World/Level.h"


MobAppearanceParticle::MobAppearanceParticle(Level* level, double x, double y, double z)
    : Particle(level, x, y, z, 0.0, 0.0, 0.0)
{
    this->rCol    = 1.0f;
    this->gCol    = 1.0f;
    this->bCol    = 1.0f;

    this->xd      = 0.0;
    this->yd      = 0.0;
    this->zd      = 0.0;
    this->gravity = 0.0f;

    this->lifetime = 30;
    this->alpha = 0.5;
}

MobAppearanceParticle::~MobAppearanceParticle() {}


int MobAppearanceParticle::getParticleTexture()
{
    return 3;   
}

eINSTANCEOF MobAppearanceParticle::GetType()
{

    return (eINSTANCEOF)eTYPE_ELDER_GUARDIAN;
}

void MobAppearanceParticle::tick()
{
    if (guardianEntity == nullptr)
    {
        auto g = std::make_shared<Guardian>(level);
        g->setElderClient();
        guardianEntity = g;
    }
    Particle::tick();
}


void MobAppearanceParticle::render(
    Tesselator* /*t*/,
    float       partialTicks,
    float       /*xa*/,
    float       /*ya*/,
    float       /*za*/,
    float       /*xa2*/,
    float       /*za2*/)
{
    if (guardianEntity == nullptr)
        return;

    shared_ptr<LivingEntity> cameraEntity = EntityRenderDispatcher::instance->cameraEntity;
    if (cameraEntity == nullptr)
        return;

    
    {
        double dx = cameraEntity->x - x;
        double dy = cameraEntity->y - y;
        double dz = cameraEntity->z - z;
        if (dx*dx + dy*dy + dz*dz > 4.0 * 4.0)
            return;
    }

    float progress = ((float)age + partialTicks) / (float)lifetime;
    float alpha    = 0.05f + 0.5f * sinf(progress * (float)PI);
    const float scale      = 1.0f / 2.35f;
    const float fullBright = 240.0f;

    
    double camX = cameraEntity->xOld + (cameraEntity->x - cameraEntity->xOld) * partialTicks;
    double camY = cameraEntity->yOld + (cameraEntity->y - cameraEntity->yOld) * partialTicks;
    double camZ = cameraEntity->zOld + (cameraEntity->z - cameraEntity->zOld) * partialTicks;

   
    float camYaw   = cameraEntity->yRotO   + (cameraEntity->yRot   - cameraEntity->yRotO)   * partialTicks;
    float camPitch = cameraEntity->xRotO   + (cameraEntity->xRot   - cameraEntity->xRotO)   * partialTicks;

    
    double savedXOff = EntityRenderDispatcher::xOff;
    double savedYOff = EntityRenderDispatcher::yOff;
    double savedZOff = EntityRenderDispatcher::zOff;

    EntityRenderDispatcher::xOff = camX;
    EntityRenderDispatcher::yOff = camY;
    EntityRenderDispatcher::zOff = camZ;

    glMultiTexCoord2f(GL_TEXTURE1, fullBright, fullBright);
    glDepthMask(true);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPushMatrix();

    
    float px = (float)(camX - Particle::xOff);
    float py = (float)(camY - Particle::yOff);
    float pz = (float)(camZ - Particle::zOff);
    glTranslatef(px, py, pz);

    glColor4f(1.0f, 1.0f, 1.0f, alpha);

    
    glTranslatef(0.0f, 0.0f, 0.0f);
    glRotatef(180.0f - camYaw,   0.0f, 1.0f, 0.0f);
    glRotatef(60.0f - 150.0f * progress - camPitch, 1.0f, 0.0f, 0.0f);
    glTranslatef(0.0f, -0.4f, -1.5f);
    glScalef(scale, scale, scale);

    guardianEntity->yRot  = guardianEntity->yRotO  = 0.0f;
    guardianEntity->xRot  = guardianEntity->xRotO  = 0.0f;

    
    EntityRenderDispatcher::instance->render(
        guardianEntity,
        0.0, 0.0, 0.0,
        0.0f,
        partialTicks);

    glPopMatrix();

    EntityRenderDispatcher::xOff = savedXOff;
    EntityRenderDispatcher::yOff = savedYOff;
    EntityRenderDispatcher::zOff = savedZOff;

    glEnable(GL_DEPTH_TEST);
    glDepthMask(true);
}


shared_ptr<Particle> MobAppearanceParticle::Provider::createParticle(
    ePARTICLE_TYPE        /*type*/,
    Level*                level,
    double                x,
    double                y,
    double                z,
    arrayWithLength<int>  /*params*/)
{
    return std::make_shared<MobAppearanceParticle>(level, x, y, z);
}