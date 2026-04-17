#include "stdafx.h"
#include "FishingHookRenderer.h"
#include "EntityRenderDispatcher.h"
#include "Options.h"
#include "Camera.h"
#include "Minecraft.h"
#include "../Minecraft.World/net.minecraft.world.entity.projectile.h"
#include "../Minecraft.World/net.minecraft.world.entity.player.h"
#include "../Minecraft.World/net.minecraft.world.item.h"
#include "../Minecraft.World/Vec3.h"
#include "../Minecraft.World/Mth.h"
#include "MultiPlayerLocalPlayer.h"

ResourceLocation FishingHookRenderer::PARTICLE_LOCATION = ResourceLocation(TN_PARTICLES);

void FishingHookRenderer::render(shared_ptr<Entity> _hook, double x, double y, double z, float rot, float a)
{
	// 4J - dynamic cast required because we aren't using templates/generics in our version
	shared_ptr<FishingHook> hook = dynamic_pointer_cast<FishingHook>(_hook);

    glPushMatrix();

    glTranslatef(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
    glEnable(GL_RESCALE_NORMAL);
    glScalef(1 / 2.0f, 1 / 2.0f, 1 / 2.0f);
    int xi = 1;
    int yi = 2;
    bindTexture(hook);		// 4J was L"/particles.png"
    Tesselator *t = Tesselator::getInstance();

    float u0 = (xi * 8 + 0) / 128.0f;
    float u1 = (xi * 8 + 8) / 128.0f;
    float v0 = (yi * 8 + 0) / 128.0f;
    float v1 = (yi * 8 + 8) / 128.0f;


    float r = 1.0f;
    float xo = 0.5f;
    float yo = 0.5f;

    glRotatef(180 - entityRenderDispatcher->playerRotY, 0, 1, 0);
    glRotatef(-entityRenderDispatcher->playerRotX, 1, 0, 0);
    t->begin();
    t->normal(0, 1, 0);
    t->vertexUV((float)(0 - xo), (float)( 0 - yo), static_cast<float>(0), (float)( u0), (float)( v1));
    t->vertexUV((float)(r - xo), (float)( 0 - yo), static_cast<float>(0), (float)( u1), (float)( v1));
    t->vertexUV((float)(r - xo), (float)( 1 - yo), static_cast<float>(0), (float)( u1), (float)( v0));
    t->vertexUV((float)(0 - xo), (float)( 1 - yo), static_cast<float>(0), (float)( u0), (float)( v0));
    t->end();

    glDisable(GL_RESCALE_NORMAL);
    glPopMatrix();


    if (hook->owner != nullptr)
	{
        float swing = hook->owner->getAttackAnim(a);
        float swing2 = (float) Mth::sin(sqrt(swing) * PI);

		Minecraft *mc = Minecraft::GetInstance();
		shared_ptr<LivingEntity> cameraTarget = mc != nullptr ? mc->cameraTargetPlayer : nullptr;
		bool ownerIsCameraTarget = cameraTarget != nullptr && hook->owner.get() == cameraTarget.get();
		bool cameraThirdPerson = false;
		int handDir = 1;

		shared_ptr<Player> ownerPlayer = dynamic_pointer_cast<Player>(hook->owner);
		if (ownerPlayer != nullptr)
		{
			shared_ptr<ItemInstance> selected = ownerPlayer->inventory->getSelected();
			if (selected == nullptr || selected->id != Item::fishingRod_Id)
			{
				handDir = -handDir;
			}
		}

		if (ownerIsCameraTarget)
		{
			shared_ptr<LocalPlayer> localCamera = dynamic_pointer_cast<LocalPlayer>(cameraTarget);
			cameraThirdPerson = localCamera != nullptr && localCamera->ThirdPersonView() > 0;
		}

		double xp = 0.0;
		double yp = 0.0;
		double zp = 0.0;
        double yOffset = hook->owner == dynamic_pointer_cast<Player>(Minecraft::GetInstance()->player) ? 0 : hook->owner->getHeadHeight();
        double lineYOffset = 0.0;

		//if (this->entityRenderDispatcher->options->thirdPersonView)
		if (ownerIsCameraTarget && !cameraThirdPerson)
		{
			float fov = 70.0f + mc->options->fov * 40.0f;
			if (fov < 1.0f) fov = 1.0f;
			double handScale = 960.0 / fov;

			float aspect = (mc->height_phys > 0) ? (mc->width_phys / static_cast<float>(mc->height_phys))
			                                     : (mc->width / static_cast<float>(mc->height));
			double nearDist = 0.05;
			double halfH = tan((fov * PI / 180.0) * 0.5) * nearDist;
			double halfW = halfH * aspect;

			float yaw = hook->owner->yRotO + (hook->owner->yRot - hook->owner->yRotO) * a;
			float pitch = hook->owner->xRotO + (hook->owner->xRot - hook->owner->xRotO) * a;
			double yawr = yaw * PI / 180.0;
			double pitchr = pitch * PI / 180.0;

			Vec3 *forward = Vec3::newTemp(-sin(yawr) * cos(pitchr), -sin(pitchr), cos(yawr) * cos(pitchr));
			Vec3 *right = Vec3::newTemp(cos(yawr), 0.0, sin(yawr));
			Vec3 *up = right->cross(forward)->normalize();

			//first person calibration
			double px = handDir * -0.5 * halfW;
            double py = 0.15 * halfH;

			Vec3 *vv = forward->scale(nearDist)->add(right->x * px + up->x * py, right->y * px + up->y * py, right->z * px + up->z * py)->scale(handScale);
			vv = vv->yRot(swing2 * 0.5f);
			vv = vv->xRot(-swing2 * 0.7f);

			xp = hook->owner->xo + (hook->owner->x - hook->owner->xo) * a + vv->x;
			yp = hook->owner->yo + (hook->owner->y - hook->owner->yo) * a + vv->y;
			zp = hook->owner->zo + (hook->owner->z - hook->owner->zo) * a + vv->z;
			lineYOffset = hook->owner->getHeadHeight();
		}
		else
 		{
            float rr = (float) (hook->owner->yBodyRotO + (hook->owner->yBodyRot - hook->owner->yBodyRotO) * a) * PI / 180;
            double ss = Mth::sin((float) rr);
            double cc = Mth::cos((float) rr);
			double d2 = handDir * 0.35;
            xp = hook->owner->xo + (hook->owner->x - hook->owner->xo) * a - cc * d2 - ss * 0.8;
            yp = hook->owner->yo + yOffset + (hook->owner->y - hook->owner->yo) * a - 0.45;
            zp = hook->owner->zo + (hook->owner->z - hook->owner->zo) * a - ss * d2 + cc * 0.8;
			lineYOffset = hook->owner->isSneaking() ? -0.1875 : 0.0;
        }

        double xh = hook->xo + (hook->x - hook->xo) * a;
        double yh = hook->yo + (hook->y - hook->yo) * a + 4 / 16.0f;
        double zh = hook->zo + (hook->z - hook->zo) * a;

        double xa = static_cast<float>(xp - xh);
        double ya = static_cast<float>(yp - yh) + lineYOffset;
        double za = static_cast<float>(zp - zh);

        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);
        t->begin(GL_LINE_STRIP);
        t->color(0x000000);
        int steps = 16;
        for (int i = 0; i <= steps; i++)
		{
            float aa = i / static_cast<float>(steps);
            t->vertex(static_cast<float>(x + xa * aa), static_cast<float>(y + ya * (aa * aa + aa) * 0.5 + 4 / 16.0f), static_cast<float>(z + za * aa));
        }
        t->end();
        glEnable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
    }
}

ResourceLocation *FishingHookRenderer::getTextureLocation(shared_ptr<Entity> mob)
{
    return &PARTICLE_LOCATION;
}
