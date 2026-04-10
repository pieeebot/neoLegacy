#include "stdafx.h"
#include "GuardianRenderer.h"
#include "GuardianModel.h"
#include "..\Minecraft.World\Mth.h"
#include "..\Minecraft.World\net.minecraft.world.entity.animal.h"
#include "../Minecraft.World/Guardian.h"
#include "..\Minecraft.World\Level.h"
#include "Tesselator.h"



ResourceLocation GuardianRenderer::GUARDIAN_LOCATION       = ResourceLocation(TN_MOB_GUARDIAN);
ResourceLocation GuardianRenderer::GUARDIAN_ELDER_LOCATION = ResourceLocation(TN_MOB_GUARDIAN_ELDER);
ResourceLocation GuardianRenderer::GUARDIAN_BEAM_LOCATION  = ResourceLocation(TN_MOB_GUARDIAN_BEAM);

GuardianRenderer::GuardianRenderer(Model *model, float shadow)
    : MobRenderer(model, shadow)
{
    guardianModel = static_cast<GuardianModel *>(model);
}




ResourceLocation *GuardianRenderer::getTextureLocation(shared_ptr<Entity> mob)
{
    shared_ptr<Guardian> guardian = dynamic_pointer_cast<Guardian>(mob);
    if (guardian && guardian->isElder())
        return &GUARDIAN_ELDER_LOCATION;
    return &GUARDIAN_LOCATION;
}







bool GuardianRenderer::shouldRender(shared_ptr<Entity> mob, float camX, float camY, float camZ)
{
    if (MobRenderer::shouldRender(mob, camX, camY, camZ))
        return true;

    shared_ptr<Guardian> guardian = dynamic_pointer_cast<Guardian>(mob);
    if (!guardian || !guardian->hasTargetedEntity())
        return false;

    shared_ptr<LivingEntity> target = guardian->getTargetedEntity();
    if (!target)
        return false;

    double tx, ty, tz, gx, gy, gz;
    getInterpolatedPos(target,   (double)target->bbHeight * 0.5, 1.0f, tx, ty, tz);
    getInterpolatedPos(guardian, (double)guardian->getEyeHeight(), 1.0f, gx, gy, gz);

    
    auto inRange = [&](double x, double y, double z) {
        double dx = x - camX, dy = y - camY, dz = z - camZ;
        return (dx*dx + dy*dy + dz*dz) < (64.0 * 64.0);
    };

    return inRange(gx, gy, gz) || inRange(tx, ty, tz);
}



void GuardianRenderer::getInterpolatedPos(shared_ptr<LivingEntity> entity, double yOffset,
                                           float partialTicks,
                                           double &outX, double &outY, double &outZ)
{
    outX = entity->xo + (entity->x - entity->xo) * (double)partialTicks;
    outY = yOffset + entity->yo + (entity->y - entity->yo) * (double)partialTicks;
    outZ = entity->zo + (entity->z - entity->zo) * (double)partialTicks;
}



void GuardianRenderer::setupRotations(shared_ptr<LivingEntity> _mob, float bob,
                                       float bodyRot, float a)
{
    shared_ptr<Guardian> mob = dynamic_pointer_cast<Guardian>(_mob);
    float bodyXRot = mob->xBodyRotO + (mob->xBodyRot - mob->xBodyRotO) * a;


    glTranslatef(0, 1.2f, 0);
    glRotatef(180.0f - bodyRot, 0, 1, 0);
    glRotatef(bodyXRot, 1, 0, 0);
    glTranslatef(0, -1.2f, 0);
}



void GuardianRenderer::render(shared_ptr<Entity> _mob, double x, double y, double z,
                               float rot, float a)
{
   
    MobRenderer::render(_mob, x, y, z, rot, a);

     shared_ptr<Guardian> mob = dynamic_pointer_cast<Guardian>(_mob);
    if (!mob || !mob->hasTargetedEntity()) return;

    shared_ptr<Entity> target = mob->level->getEntity(mob->getTargetedEntityId());
    if (!target) {
        target = mob->level->getNearestPlayer(mob->x, mob->y, mob->z, 64.0);
        if (!target || target->entityId != mob->getTargetedEntityId()) return;
    }

    
    float f = mob->getAttackAnimationScale(a);
    float time = (float)mob->tickCount + a;
    float texVOff = -time * 0.2f - floor(-time * 0.1f);
    float eyeHeight = mob->getEyeHeight();
    if (mob && mob->isElder())
    eyeHeight = mob->bbHeight * 0.5f * 2.35f;


   double targetX = target->xo + (target->x - target->xo) * a;
   double targetZ = target->zo + (target->z - target->zo) * a;

   double targetYBase = target->yo + (target->y - target->yo) * a;
   double targetY = targetYBase - (target->bbHeight * 0.5f);
  

    
    double mobX = mob->xo + (mob->x - mob->xo) * a;
    double mobY = mob->yo + (mob->y - mob->yo) * a + eyeHeight;
    double mobZ = mob->zo + (mob->z - mob->zo) * a;

    double dx = targetX - mobX;
    double dy = targetY - mobY;
    double dz = targetZ - mobZ;
    
    double distOrizzontale = sqrt(dx * dx + dz * dz);
    double distanzaTotale = sqrt(dx * dx + dy * dy + dz * dz);

    
    bindTexture(&GUARDIAN_BEAM_LOCATION);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(true);

    glPushMatrix();
    glTranslatef((float)x, (float)y + eyeHeight, (float)z);

    
    glRotatef((float)(atan2(dx, dz) * 180.0 / PI), 0, 1, 0);
    glRotatef((float)(-atan2(dy, distOrizzontale) * 180.0 / PI), 1, 0, 0);

    Tesselator* t = Tesselator::getInstance();
    
   
    double r1 = 0.2; 
    double r2 = r1 * 1.41;
    
    double rotBeam = time * 0.05 * (1.0 - 2.5);
    float f7 = f * f;
    int red = (int)fmin(255, 64 + (int)(f7 * 240.0f));
    int green = (int)fmin(255, 32 + (int)(f7 * 192.0f));
    int blue = (int)fmax(0, fmin(255, 128 - (int)(f7 * 64.0f)));

    double vMin = 0.0 + texVOff;
    double vMax = distanzaTotale * (0.5 / r1) + texVOff;

    
    double d12 = cos(rotBeam + PI) * r1;
    double d13 = sin(rotBeam + PI) * r1;
    double d14 = cos(rotBeam + 0.0) * r1;
    double d15 = sin(rotBeam + 0.0) * r1;
    double d16 = cos(rotBeam + (PI / 2.0)) * r1;
    double d17 = sin(rotBeam + (PI / 2.0)) * r1;
    double d18 = cos(rotBeam + (PI * 3.0 / 2.0)) * r1;
    double d19 = sin(rotBeam + (PI * 3.0 / 2.0)) * r1;

    
    t->begin();
    t->color(red, green, blue, 255);
    t->vertexUV(d12, d13, distanzaTotale, 0.4999, vMax);
    t->vertexUV(d12, d13, 0.0, 0.4999, vMin);
    t->vertexUV(d14, d15, 0.0, 0.0, vMin);
    t->vertexUV(d14, d15, distanzaTotale, 0.0, vMax);
    t->vertexUV(d16, d17, distanzaTotale, 0.4999, vMax);
    t->vertexUV(d16, d17, 0.0, 0.4999, vMin);
    t->vertexUV(d18, d19, 0.0, 0.0, vMin);
    t->vertexUV(d18, d19, distanzaTotale, 0.0, vMax);
    t->end();

    
    glDepthMask(false);
    double d24 = (mob->tickCount % 2 == 0) ? 0.5 : 0.0;
    
    t->begin();
    t->color(red, green, blue, 64); 

    double g1 = cos(rotBeam + 2.356) * r2;
    double g2 = sin(rotBeam + 2.356) * r2;
    double g3 = cos(rotBeam + 0.785) * r2;
    double g4 = sin(rotBeam + 0.785) * r2;
    double g5 = cos(rotBeam + 3.926) * r2;
    double g6 = sin(rotBeam + 3.926) * r2;
    double g7 = cos(rotBeam + 5.497) * r2;
    double g8 = sin(rotBeam + 5.497) * r2;

    t->vertexUV(g1, g2, distanzaTotale, 0.5, d24 + 0.5);
    t->vertexUV(g1, g2, 0.0, 0.5, d24);
    t->vertexUV(g3, g4, 0.0, 1.0, d24);
    t->vertexUV(g3, g4, distanzaTotale, 1.0, d24 + 0.5);
    t->vertexUV(g7, g8, distanzaTotale, 1.0, d24 + 0.5);
    t->vertexUV(g7, g8, 0.0, 1.0, d24);
    t->vertexUV(g5, g6, 0.0, 0.5, d24);
    t->vertexUV(g5, g6, distanzaTotale, 0.5, d24 + 0.5);
    t->end();

    glDepthMask(true);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glEnable(GL_CULL_FACE);
    glPopMatrix();
}

void GuardianRenderer::renderModel(shared_ptr<LivingEntity> mob, float wp, float ws, float bob,
                                    float headRotMinusBodyRot, float headRotx, float scale)
{
    shared_ptr<Guardian> guardian = dynamic_pointer_cast<Guardian>(mob);
    if (guardian && guardian->isElder())
    {
        glPushMatrix();
        
        glTranslatef(0.0f, -2.0f, 0.0f);  
        glScalef(2.35f, 2.35f, 2.35f);
        LivingEntityRenderer::renderModel(mob, wp, ws, bob, headRotMinusBodyRot, headRotx, scale);
        glPopMatrix();
    }
    else
    {
        LivingEntityRenderer::renderModel(mob, wp, ws, bob, headRotMinusBodyRot, headRotx, scale);
    }
}