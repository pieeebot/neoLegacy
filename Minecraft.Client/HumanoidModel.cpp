#include "stdafx.h"
#include "HumanoidModel.h"
#include "../Minecraft.World/Mth.h"
#include "../Minecraft.World/Entity.h"
#include "ModelPart.h"

// 4J added 

ModelPart * HumanoidModel::AddOrRetrievePart(SKIN_BOX *pBox)
{
	ModelPart *pAttachTo=nullptr;

	switch(pBox->ePart)
	{
	case eBodyPart_Head:
		pAttachTo=head;
		break;
	case eBodyPart_Body:
		pAttachTo=body;
		break;
	case eBodyPart_Arm0:
		pAttachTo=arm0;
		break;
	case eBodyPart_Arm1:
		pAttachTo=arm1;
		break;
	case eBodyPart_Leg0:
		pAttachTo=leg0;
		break;
	case eBodyPart_Leg1:
		pAttachTo=leg1;
		break;
	case eBodyPart_Jacket:
		pAttachTo=jacket;
		break;
	case eBodyPart_Sleeve0:
		pAttachTo=sleeve0;
		break;
	case eBodyPart_Sleeve1:
		pAttachTo=sleeve1;
		break;
	case eBodyPart_Pants0:
		pAttachTo=pants0;
		break;
	case eBodyPart_Pants1:
		pAttachTo=pants1;
		break;
	}

	// first check this box doesn't already exist
	ModelPart *pNewBox = pAttachTo->retrieveChild(pBox);

	if(pNewBox)
	{
		if((pNewBox->getfU()!=static_cast<int>(pBox->fU)) || (pNewBox->getfV()!=static_cast<int>(pBox->fV)))
		{
			app.DebugPrintf("HumanoidModel::AddOrRetrievePart - Box geometry was found, but with different uvs\n");
			pNewBox=nullptr;
		}
	}
	if(pNewBox==nullptr)
	{
		//app.DebugPrintf("HumanoidModel::AddOrRetrievePart - Adding box to model part\n");

		pNewBox = new ModelPart(this, static_cast<int>(pBox->fU), static_cast<int>(pBox->fV));
		pNewBox->visible=false;
		pNewBox->addHumanoidBox(pBox->fX, pBox->fY, pBox->fZ, pBox->fW, pBox->fH, pBox->fD, 0); 
		// 4J-PB - don't compile here, since the lighting isn't set up. It'll be compiled on first use.
		//pNewBox->compile(1.0f/16.0f);	
		pAttachTo->addChild(pNewBox);
	}

	return pNewBox;
}

void HumanoidModel::_init(float g, float yOffset, int texWidth, int texHeight, bool slimHands, bool mirror, bool force32)
{
	this->texWidth = texWidth;
	this->texHeight = texHeight;

	jacket = nullptr;
	sleeve0 = nullptr;
	sleeve1 = nullptr;
	pants0 = nullptr;
	pants1 = nullptr;

	m_fYOffset=yOffset;
    cloak = new ModelPart(this, 0, 0);
    cloak->addHumanoidBox(-5, -0, -1, 10, 16, 1, g); // Cloak

	elytraRight = new ModelPart(this, 22, 0);
	elytraRight->addHumanoidBox(-10.0f, 0.0f, 0.0f, 10, 20, 2, 0.0f);
	elytraRight->setPos(5.0f, 0.0f + yOffset, 0.0f); // Wing Left

	elytraLeft = new ModelPart(this, 22, 0);
	elytraLeft->bMirror = true;
	elytraLeft->addHumanoidBox(0.0f, 0.0f, 0.0f, 10, 20, 2, 0.0f);
	elytraLeft->setPos(-5.0f, 0.0f + yOffset, 0.0f); // Wing Right



    ear = new ModelPart(this, 24, 0);
    ear->addHumanoidBox(-3, -6, -1, 6, 6, 1, g); // Ear
        
    head = new ModelPart(this, 0, 0);
    head->addHumanoidBox(-4, -8, -4, 8, 8, 8, g); // Head
    head->setPos(0, 0 + yOffset, 0);

    hair = new ModelPart(this, 32, 0);
    hair->addHumanoidBox(-4, -8, -4, 8, 8, 8, g + 0.5f); // Head
    hair->setPos(0, 0 + yOffset, 0);

	if ((texWidth == 64 && texHeight == 64) && !force32)
	{
		jacket = new ModelPart(this, 16, 32);
		jacket->addHumanoidBox(-4, 0, -2, 8, 12, 4, g + 0.5);
		jacket->setPos(0, 0 + yOffset, 0);
	}

    body = new ModelPart(this, 16, 16);
    body->addHumanoidBox(-4, 0, -2, 8, 12, 4, g); // Body
    body->setPos(0, 0 + yOffset, 0);

	if ((texWidth == 64 && texHeight == 64) && !force32)
	{
		arm0 = new ModelPart(this, 24 + 16, 16);
		arm1 = new ModelPart(this, 16 + 16, 48);

		sleeve0 = new ModelPart(this, 24 + 16, 32);
		sleeve1 = new ModelPart(this, 32 + 16, 48);

		if (slimHands == false)
		{
			sleeve0->addHumanoidBox(-3, -2, -2, 4, 12, 4, g + 0.5);
			sleeve1->addHumanoidBox(-1, -2, -2, 4, 12, 4, g + 0.5);
		}
		else if (slimHands == true)
		{
			sleeve0->addHumanoidBox(-2, -2, -2, 3, 12, 4, g + 0.5);
			sleeve1->addHumanoidBox(-1, -2, -2, 3, 12, 4, g + 0.5);
		}

		sleeve0->setPos(-5, 2 + yOffset, 0);
		sleeve1->setPos(5, 2 + yOffset, 0);
	}
	else if ((texWidth == 64 && texHeight == 32) || force32)
	{
		arm0 = new ModelPart(this, 24 + 16, 16);
		arm1 = new ModelPart(this, 24 + 16, 16);
	}

	if (slimHands == false)
	{
		arm0->addHumanoidBox(-3, -2, -2, 4, 12, 4, g);
		arm1->addHumanoidBox(-1, -2, -2, 4, 12, 4, g);
	}
	else if (slimHands == true)
	{
		arm0->addHumanoidBox(-2, -2, -2, 3, 12, 4, g);
		arm1->addHumanoidBox(-1, -2, -2, 3, 12, 4, g);
	}

    arm0->setPos(-5, 2 + yOffset, 0);
    arm1->setPos(5, 2 + yOffset, 0);

	if (mirror == true)
		arm1->bMirror = true;

	if ((texWidth == 64 && texHeight == 64) && !force32)
	{
		leg0 = new ModelPart(this, 0, 16);
		leg1 = new ModelPart(this, 16, 48);

		pants0 = new ModelPart(this, 0, 32);
		pants0->addHumanoidBox(-2, 0, -2, 4, 12, 4, g + 0.5);
		pants0->setPos(-1.9, 12 + yOffset, 0);

		pants1 = new ModelPart(this, 0, 48);
		pants1->addHumanoidBox(-2, 0, -2, 4, 12, 4, g + 0.5);
		pants1->setPos(1.9, 12 + yOffset, 0);
	}
	else if ((texWidth == 64 && texHeight == 32) || force32)
	{
		leg0 = new ModelPart(this, 0, 16);
		leg1 = new ModelPart(this, 0, 16);
	}

    leg0->addHumanoidBox(-2, 0, -2, 4, 12, 4, g); // Leg0
    leg0->setPos(-1.9, 12 + yOffset, 0);

    leg1->addHumanoidBox(-2, 0, -2, 4, 12, 4, g); // Leg1
    leg1->setPos(1.9, 12 + yOffset, 0);

	if (mirror == true)
		leg1->bMirror = true;

	// 4J added - compile now to avoid random performance hit first time cubes are rendered
	// 4J Stu - Not just performance, but alpha+depth tests don't work right unless we compile here
	cloak->compile(1.0f/16.0f);
	elytraLeft->compile(1.0f / 16.0f);
	elytraRight->compile(1.0f / 16.0f);
	ear->compile(1.0f/16.0f);
	head->compile(1.0f/16.0f);
	body->compile(1.0f/16.0f);
	arm0->compile(1.0f/16.0f);
	arm1->compile(1.0f/16.0f);
	leg0->compile(1.0f/16.0f);
	leg1->compile(1.0f/16.0f);
	hair->compile(1.0f/16.0f);

	if (jacket != 0)
		jacket->compile(1.0f/16.0f);
	if (sleeve0 != 0)
		sleeve0->compile(1.0f/16.0f);
	if (sleeve1 != 0)
		sleeve1->compile(1.0f/16.0f);
	if (pants0 != 0)
		pants0->compile(1.0f/16.0f);
	if (pants1 != 0)
		pants1->compile(1.0f/16.0f);

	holdingLeftHand=0;
	holdingRightHand=0;
	sneaking=false;
	idle=false;
	bowAndArrow=false;
	elytraFlying = false;
	elytraCrouching = false;


	// 4J added
	eating = false;
	eating_t = 0.0f;
	eating_swing = 0.0f;
	m_uiAnimOverrideBitmask = 0L;
}


HumanoidModel::HumanoidModel() : Model()
{
	_init(0, 0, 64, 32, false, true, false);
}

HumanoidModel::HumanoidModel(float g) : Model()
{
	_init(g, 0, 64, 32, false, true, false);
}

HumanoidModel::HumanoidModel(float g, float yOffset, int texWidth, int texHeight) : Model()
{
	_init(g,yOffset,texWidth,texHeight, false, true, false);
}

HumanoidModel::HumanoidModel(float g, float yOffset, int texWidth, int texHeight, bool slimHands) : Model()
{
	_init(g,yOffset,texWidth,texHeight, slimHands, true, false);
}

HumanoidModel::HumanoidModel(float g, float yOffset, int texWidth, int texHeight, bool slimHands, bool mirror) : Model()
{
	_init(g,yOffset,texWidth,texHeight, slimHands, mirror, false);
}

HumanoidModel::HumanoidModel(float g, float yOffset, int texWidth, int texHeight, bool slimHands, bool mirror, bool force32) : Model()
{
	_init(g,yOffset,texWidth,texHeight, slimHands, mirror, force32);
}

void HumanoidModel::render(shared_ptr<Entity> entity, float time, float r, float bob, float yRot, float xRot, float scale, bool usecompiled)
{	
	if(entity != nullptr)
	{
		m_uiAnimOverrideBitmask=entity->getAnimOverrideBitmask();
	}

	setupAnim(time, r, bob, yRot, xRot, scale, entity, m_uiAnimOverrideBitmask);

	if (young)
	{
		float ss = 2.0f;
		glPushMatrix();
		glScalef(1.5f / ss, 1.5f / ss, 1.5f / ss);
		glTranslatef(0, 16 * scale, 0);
		head->render(scale, usecompiled);
		glPopMatrix();
		glPushMatrix();
		glScalef(1 / ss, 1 / ss, 1 / ss);
		glTranslatef(0, 24 * scale, 0);
		body->render(scale, usecompiled);
		arm0->render(scale, usecompiled);
		arm1->render(scale, usecompiled);
		leg0->render(scale, usecompiled);
		leg1->render(scale, usecompiled);
		hair->render(scale, usecompiled);

		if (jacket)
			jacket->render(scale, usecompiled);
		if (sleeve0)
			sleeve0->render(scale, usecompiled);
		if (sleeve1)
			sleeve1->render(scale, usecompiled);
		if (pants0)
			pants0->render(scale, usecompiled);
		if (pants1)
			pants1->render(scale, usecompiled);

		glPopMatrix();
	}
	else
	{
		head->render(scale, usecompiled,(m_uiAnimOverrideBitmask&(1<<eAnim_DisableRenderHead))>0);
		body->render(scale, usecompiled,(m_uiAnimOverrideBitmask&(1<<eAnim_DisableRenderTorso))>0);
		arm0->render(scale, usecompiled,(m_uiAnimOverrideBitmask&(1<<eAnim_DisableRenderArm0))>0);
		arm1->render(scale, usecompiled,(m_uiAnimOverrideBitmask&(1<<eAnim_DisableRenderArm1))>0);
		leg0->render(scale, usecompiled,(m_uiAnimOverrideBitmask&(1<<eAnim_DisableRenderLeg0))>0);
		leg1->render(scale, usecompiled,(m_uiAnimOverrideBitmask&(1<<eAnim_DisableRenderLeg1))>0);
		hair->render(scale, usecompiled,(m_uiAnimOverrideBitmask&(1<<eAnim_DisableRenderHair))>0);

		if (jacket)
			jacket->render(scale, usecompiled,(m_uiAnimOverrideBitmask&(1<<eAnim_DisableRenderJacket))>0);
		if (sleeve0)
			sleeve0->render(scale, usecompiled,(m_uiAnimOverrideBitmask&(1<<eAnim_DisableRenderSleeve0))>0);
		if (sleeve1)
			sleeve1->render(scale, usecompiled,(m_uiAnimOverrideBitmask&(1<<eAnim_DisableRenderSleeve1))>0);
		if (pants0)
			pants0->render(scale, usecompiled,(m_uiAnimOverrideBitmask&(1<<eAnim_DisableRenderPants0))>0);
		if (pants1)
			pants1->render(scale, usecompiled,(m_uiAnimOverrideBitmask&(1<<eAnim_DisableRenderPants1))>0);
	}
}

void HumanoidModel::setupAnim(float time, float r, float bob, float yRot, float xRot, float scale, shared_ptr<Entity> entity, unsigned int uiBitmaskOverrideAnim)
{
	//bool bIsAttacking = (attackTime > -9990.0f);
	
	{
		head->yRot = yRot / (float) (180.0f / PI);
		head->xRot = xRot / (float) (180.0f / PI);
		hair->yRot = head->yRot;
		hair->xRot = head->xRot;
		body->z = 0.0f;

		if (jacket)
			jacket->z = 0.0f;

		// Does the skin have an override for anim?

		if(uiBitmaskOverrideAnim&(1<<eAnim_ArmsDown))
		{
			arm0->xRot=0.0f;
			arm1->xRot=0.0f;
			arm0->zRot = 0.0f;
			arm1->zRot = 0.0f;

			if (sleeve0)
			{
				sleeve0->xRot=0.0f;
				sleeve0->zRot=0.0f;
			}

			if (sleeve1)
			{
				sleeve1->xRot=0.0f;
				sleeve1->zRot=0.0f;
			}
		}
		else if(uiBitmaskOverrideAnim&(1<<eAnim_ArmsOutFront))
		{
			arm0->xRot=-HALF_PI;
			arm1->xRot=-HALF_PI;
			arm0->zRot = 0.0f;
			arm1->zRot = 0.0f;

			if (sleeve0)
			{
				sleeve0->xRot=-HALF_PI;
				sleeve0->zRot=0.0f;
			}

			if (sleeve1)
			{
				sleeve1->xRot=-HALF_PI;
				sleeve1->zRot=0.0f;
			}
		}
		else if(uiBitmaskOverrideAnim&(1<<eAnim_SingleArms))
		{
			arm0->xRot = (Mth::cos(time * 0.6662f + PI) * 2.0f) * r * 0.5f;
			arm1->xRot = (Mth::cos(time * 0.6662f + PI) * 2.0f) * r * 0.5f;
			arm0->zRot = 0.0f;
			arm1->zRot = 0.0f;

			if (sleeve0)
			{
				sleeve0->xRot=(Mth::cos(time * 0.6662f + PI) * 2.0f) * r * 0.5f;
				sleeve0->zRot=0.0f;
			}

			if (sleeve1)
			{
				sleeve1->xRot=(Mth::cos(time * 0.6662f + PI) * 2.0f) * r * 0.5f;
				sleeve1->zRot=0.0f;
			}
		}
		// 4J-PB - Weeping Angel - does't look good holding something in the arm that's up
		else if((uiBitmaskOverrideAnim&(1<<eAnim_StatueOfLiberty)) && (holdingRightHand==0) && (attackTime==0.0f))
		{
			arm0->xRot = -PI;
			arm0->zRot = -0.3f;
			arm1->xRot = ( Mth::cos(time * 0.6662f) * 2.0f) * r * 0.5f;
			arm1->zRot = 0.0f;

			if (sleeve0)
			{
				sleeve0->xRot=-PI;
				sleeve0->zRot=-0.3f;
			}

			if (sleeve1)
			{
				sleeve1->xRot=( Mth::cos(time * 0.6662f) * 2.0f) * r * 0.5f;
				sleeve1->zRot=0.0f;
			}
		}
		else
		{
			arm0->xRot = (Mth::cos(time * 0.6662f + PI) * 2.0f) * r * 0.5f;
			arm1->xRot = ( Mth::cos(time * 0.6662f) * 2.0f) * r * 0.5f;
			arm0->zRot = 0.0f;
			arm1->zRot = 0.0f;

			if (sleeve0)
			{
				sleeve0->xRot=(Mth::cos(time * 0.6662f + PI) * 2.0f) * r * 0.5f;
				sleeve0->zRot=0.0f;
			}

			if (sleeve1)
			{
				sleeve1->xRot=( Mth::cos(time * 0.6662f) * 2.0f) * r * 0.5f;
				sleeve1->zRot=0.0f;
			}
		}
		//        arm0.zRot = ((float) (util.Mth.cos(time * 0.2312f) + 1) * 1) * r;


		//        arm1.zRot = ((float) (util.Mth.cos(time * 0.2812f) - 1) * 1) * r;


		leg0->yRot = 0.0f;
		leg1->yRot = 0.0f;

		if (pants0)
			pants0->yRot=0.0f;

		if (pants1)
			pants1->yRot=0.0f;

		if (riding)
		{
			if ((uiBitmaskOverrideAnim&(1<<eAnim_SmallModel)) == 0)
			{
				arm0->xRot += -HALF_PI * 0.4f;
				arm1->xRot += -HALF_PI * 0.4f;
				leg0->xRot = -HALF_PI * 0.8f;
				leg1->xRot = -HALF_PI * 0.8f;
				leg0->yRot = HALF_PI * 0.2f;
				leg1->yRot = -HALF_PI * 0.2f;

				if (sleeve0)
					sleeve0->xRot+=-HALF_PI * 0.4f;

				if (sleeve1)
					sleeve1->xRot+=-HALF_PI * 0.4f;

				if (pants0)
				{
					pants0->xRot=-HALF_PI * 0.8f;
					pants0->yRot=HALF_PI * 0.2f;
				}

				if (pants1)
				{
					pants1->xRot=-HALF_PI * 0.8f;
					pants1->yRot=-HALF_PI * 0.2f;
				}
			}
			else
			{
				arm0->xRot += -HALF_PI * 0.4f;
				arm1->xRot += -HALF_PI * 0.4f;
				leg0->xRot = -HALF_PI * 0.4f;
				leg1->xRot = -HALF_PI * 0.4f;

				if (sleeve0)
					sleeve0->xRot+=-HALF_PI * 0.4f;

				if (sleeve1)
					sleeve1->xRot+=-HALF_PI * 0.4f;

				if (pants0)
					pants0->xRot=-HALF_PI * 0.4f;

				if (pants1)
					pants1->xRot=-HALF_PI * 0.4f;
			}
		}
		else if(idle && !sneaking )
		{
			leg0->xRot = -HALF_PI;
			leg1->xRot = -HALF_PI;
			leg0->yRot = HALF_PI * 0.2f;
			leg1->yRot = -HALF_PI * 0.2f;

			if (pants0)
			{
				pants0->xRot=-HALF_PI;
				pants0->yRot=HALF_PI * 0.2f;
			}

			if (pants1)
			{
				pants1->xRot=-HALF_PI;
				pants1->yRot=-HALF_PI * 0.2f;
			}
		}		
		else if(uiBitmaskOverrideAnim&(1<<eAnim_NoLegAnim))
		{
			leg0->xRot=0.0f;
			leg0->zRot=0.0f;
			leg1->xRot=0.0f;
			leg1->zRot=0.0f;
			leg0->yRot = 0.0f;
			leg1->yRot = 0.0f;	

			if (pants0)
			{
				pants0->xRot=0.0f;
				pants0->zRot=0.0f;
				pants0->yRot=0.0f;
			}

			if (pants1)
			{
				pants1->xRot=0.0f;
				pants1->zRot=0.0f;
				pants1->yRot=0.0f;
			}
		}
		else if(uiBitmaskOverrideAnim&(1<<eAnim_SingleLegs))
		{
			leg0->xRot = ( Mth::cos(time * 0.6662f) * 1.4f) * r;
			leg1->xRot = ( Mth::cos(time * 0.6662f) * 1.4f) * r;

			if (pants0)
				pants0->xRot=( Mth::cos(time * 0.6662f) * 1.4f) * r;

			if (pants1)
				pants1->xRot=( Mth::cos(time * 0.6662f) * 1.4f) * r;
		}
		else
		{
			leg0->xRot = ( Mth::cos(time * 0.6662f) * 1.4f) * r;
			leg1->xRot = ( Mth::cos(time * 0.6662f + PI) * 1.4f) * r;

			if (pants0)
				pants0->xRot=( Mth::cos(time * 0.6662f) * 1.4f) * r;

			if (pants1)
				pants1->xRot=( Mth::cos(time * 0.6662f + PI) * 1.4f) * r;
		}

		if (holdingLeftHand != 0) 
		{
			arm1->xRot = arm1->xRot * 0.5f - HALF_PI * 0.2f * holdingLeftHand;

			if (sleeve1)
				sleeve1->xRot=sleeve1->xRot * 0.5f - HALF_PI * 0.2f * holdingLeftHand;
		}
		if (holdingRightHand != 0) 
		{
			arm0->xRot = arm0->xRot * 0.5f - HALF_PI * 0.2f * holdingRightHand;

			if (sleeve0)
				sleeve0->xRot=sleeve0->xRot * 0.5f - HALF_PI * 0.2f * holdingRightHand;
		}

		arm0->yRot = 0.0f;
		arm1->yRot = 0.0f;

		if (sleeve0)
			sleeve0->yRot=0.0f;
		if (sleeve1)
			sleeve1->yRot=0.0f;

		if (attackTime > -9990.0f)
		{
			float swing = attackTime;
			body->yRot = Mth::sin(sqrt(swing) * PI * 2.0f) * 0.2f;
			arm0->z = Mth::sin(body->yRot) * 5.0f;
			arm0->x = -Mth::cos(body->yRot) * 5.0f;
			arm1->z = -Mth::sin(body->yRot) * 5.0f;
			arm1->x = Mth::cos(body->yRot) * 5.0f;
			arm0->yRot += body->yRot;
			arm1->yRot += body->yRot;
			arm1->xRot += body->yRot;

			if (sleeve0)
			{
				sleeve0->z=Mth::sin(body->yRot) * 5.0f;
				sleeve0->x=-Mth::cos(body->yRot) * 5.0f;
				sleeve0->yRot+=body->yRot;
			}
			if (sleeve1)
			{
				sleeve1->z=-Mth::sin(body->yRot) * 5.0f;
				sleeve1->x=Mth::cos(body->yRot) * 5.0f;
				sleeve1->yRot+=body->yRot;
				sleeve1->xRot+=body->yRot;
			}

			swing = 1.0f - attackTime;
			swing *= swing;
			swing *= swing;
			swing = 1.0f - swing;
			float aa = Mth::sin(swing * PI);
			float bb = Mth::sin(attackTime * PI) * -(head->xRot - 0.7f) * 0.75f;
			arm0->xRot -= aa * 1.2f + bb;	// 4J - changed 1.2 -> 1.2f
			arm0->yRot += body->yRot * 2.0f; 

			if (sleeve0)
			{
				sleeve0->xRot -= aa * 1.2f + bb;
				sleeve0->yRot += body->yRot * 2.0f; 
			}

			if((uiBitmaskOverrideAnim&(1<<eAnim_StatueOfLiberty))&& (holdingRightHand==0) && (attackTime==0.0f))
			{
				arm0->zRot -= Mth::sin(attackTime * PI) * -0.4f;

				if (sleeve0)
					sleeve0->zRot -= Mth::sin(attackTime * PI) * -0.4f;
			}
			else
			{
				arm0->zRot = Mth::sin(attackTime * PI) * -0.4f;

				if (sleeve0)
					sleeve0->zRot = Mth::sin(attackTime * PI) * -0.4f;
			}
		}

		// 4J added
		if( eating )
		{
			// These factors are largely lifted from ItemInHandRenderer to try and keep the 3rd person eating animation as similar as possible
			float is = 1 - eating_swing;
			is = is * is * is;
			is = is * is * is;
			is = is * is * is;
			float iss = 1 - is;
			arm0->xRot = - Mth::abs(Mth::cos(eating_t / 4.0f * PI) * 0.1f) * (eating_swing > 0.2 ? 1.0f : 0.0f) * 2.0f;		// This factor is the chomping bit (conditional factor is so that he doesn't eat whilst the food is being pulled away at the end)
			arm0->yRot -= iss * 0.5f;																			// This factor and the following to the general arm movement through the life of the swing
			arm0->xRot -= iss * 1.2f;

			if (sleeve0)
			{
				sleeve0->xRot = -Mth::abs(Mth::cos(eating_t / 4.0f * PI) * 0.1f) * (eating_swing > 0.2 ? 1.0f : 0.0f) * 2.0f;
				sleeve0->yRot -= iss * 0.5f;
				sleeve0->xRot -= iss * 1.2f;
			}
		}

		if (sneaking)
		{
			if(uiBitmaskOverrideAnim&(1<<eAnim_SmallModel))
			{
				body->xRot = -0.5f;
				leg0->xRot -= 0.0f;
				leg1->xRot -= 0.0f;
				arm0->xRot += 0.4f;
				arm1->xRot += 0.4f;
				leg0->z = -4.0f;
				leg1->z = -4.0f;
				body->z = 2.0f;
				body->y = 0.0f;
				arm0->y = 2.0f;
				arm1->y = 2.0f;
				leg0->y = +9.0f;
				leg1->y = +9.0f;
				head->y = +1.0f;
				hair->y = +1.0f;
				ear->y = +1.0f;
				cloak->y = 0.0f;

				if (jacket)
				{
					jacket->xRot = -0.5f;
					jacket->z = 2.0f;
					jacket->y = 0.0f;
				}

				if (sleeve0)
				{
					sleeve0->xRot += 0.4f;
					sleeve0->y = 2.0f;
				}

				if (sleeve1)
				{
					sleeve1->xRot += 0.4f;
					sleeve1->y = 2.0f;
				}

				if (pants0)
				{
					pants0->xRot -= 0.0f;
					pants0->z = -4.0f;
					pants0->y = +9.0f;
				}

				if (pants1)
				{
					pants1->xRot -= 0.0f;
					pants1->z = -4.0f;
					pants1->y = +9.0f;
				}
			}
			else
			{
				body->xRot = 0.5f;
				leg0->xRot -= 0.0f;
				leg1->xRot -= 0.0f;
				arm0->xRot += 0.4f;
				arm1->xRot += 0.4f;
				leg0->z = +4.0f;
				leg1->z = +4.0f;
				body->y = 0.0f;
				arm0->y = 2.0f;
				arm1->y = 2.0f;
				leg0->y = +9.0f;
				leg1->y = +9.0f;
				head->y = +1.0f;
				hair->y = +1.0f;
				ear->y = +1.0f;
				cloak->y = 0.0f;

				if (jacket)
				{
					jacket->xRot = 0.5f;
					jacket->y = 0.0f;
				}

				if (sleeve0)
				{
					sleeve0->xRot += 0.4f;
					sleeve0->y = 2.0f;
				}

				if (sleeve1)
				{
					sleeve1->xRot += 0.4f;
					sleeve1->y = 2.0f;
				}

				if (pants0)
				{
					pants0->xRot -= 0.0f;
					pants0->z = +4.0f;
					pants0->y = +9.0f;
				}

				if (pants1)
				{
					pants1->xRot -= 0.0f;
					pants1->z = +4.0f;
					pants1->y = +9.0f;
				}
			}
		}
		else
		{
			body->xRot = 0.0f;
			leg0->z = 0.1f;
			leg1->z = 0.1f;

			if (jacket)
				jacket->xRot = 0.0f;

			if (pants0)
				pants0->z = 0.1f;

			if (pants1)
				pants1->z = 0.1f;

			if(!riding && idle)
			{
				leg0->y = 22.0f;
				leg1->y = 22.0f;
				body->y = 10.0f;
				arm0->y = 12.0f;
				arm1->y = 12.0f;
				head->y = 10.0f;
				hair->y = 10.0f;
				ear->y = 11.0f;
				cloak->y = 10.0f;

				if (jacket)
					jacket->y = 10.0f;
				if (sleeve0)
					sleeve0->y = 12.0f;
				if (sleeve1)
					sleeve1->y = 12.0f;
				if (pants0)
					pants0->y = 22.0f;
				if (pants1)
					pants1->y = 22.0f;
			}
			else
			{
				leg0->y = 12.0f;
				leg1->y = 12.0f;
				body->y = 0.0f;
				arm0->y = 2.0f;
				arm1->y = 2.0f;
				head->y = 0.0f;
				hair->y = 0.0f;
				ear->y = 1.0f;
				cloak->y = 0.0f;

				if (jacket)
					jacket->y = 0.0f;
				if (sleeve0)
					sleeve0->y = 2.0f;
				if (sleeve1)
					sleeve1->y = 2.0f;
				if (pants0)
					pants0->y = 12.0f;
				if (pants1)
					pants1->y = 12.0f;
			}
		}

		arm0->zRot += ((Mth::cos(bob * 0.09f)) * 0.05f + 0.05f);
		arm1->zRot -= ((Mth::cos(bob * 0.09f)) * 0.05f + 0.05f);
		arm0->xRot += ((Mth::sin(bob * 0.067f)) * 0.05f);
		arm1->xRot -= ((Mth::sin(bob * 0.067f)) * 0.05f);

		if (sleeve0)
		{
			sleeve0->xRot += ((Mth::sin(bob * 0.067f)) * 0.05f);
			sleeve0->zRot += ((Mth::cos(bob * 0.09f)) * 0.05f + 0.05f);
		}

		if (sleeve1)
		{
			sleeve1->xRot -= ((Mth::sin(bob * 0.067f)) * 0.05f);
			sleeve1->zRot -= ((Mth::cos(bob * 0.09f)) * 0.05f + 0.05f);
		}

		if (bowAndArrow) 
		{
			float attack2 = 0.0f;
			float attack = 0.0f;

			arm0->zRot = 0.0f;
			arm1->zRot = 0.0f;
			arm0->yRot = -(0.1f - attack2 * 0.6f) + head->yRot;
			arm1->yRot = +(0.1f - attack2 * 0.6f) + head->yRot + 0.4f;
			arm0->xRot = -HALF_PI + head->xRot;
			arm1->xRot = -HALF_PI + head->xRot;
			arm0->xRot -= attack2 * 1.2f - attack * 0.4f;
			arm1->xRot -= attack2 * 1.2f - attack * 0.4f;
			arm0->zRot += ((float) (Mth::cos(bob * 0.09f)) * 0.05f + 0.05f);
			arm1->zRot -= ((float) (Mth::cos(bob * 0.09f)) * 0.05f + 0.05f);
			arm0->xRot += ((float) (Mth::sin(bob * 0.067f)) * 0.05f);
			arm1->xRot -= ((float) (Mth::sin(bob * 0.067f)) * 0.05f);

			if (sleeve0)
			{
				sleeve0->zRot = 0.0f;
				sleeve0->yRot = -(0.1f - attack2 * 0.6f) + head->yRot;
				sleeve0->xRot = -HALF_PI + head->xRot;
				sleeve0->xRot -= attack2 * 1.2f - attack * 0.4f;
				sleeve0->zRot += ((float) (Mth::cos(bob * 0.09f)) * 0.05f + 0.05f);
				sleeve0->xRot += ((float) (Mth::sin(bob * 0.067f)) * 0.05f);
			}

			if (sleeve1)
			{
				sleeve1->zRot = 0.0f;
				sleeve1->yRot = +(0.1f - attack2 * 0.6f) + head->yRot + 0.4f;
				sleeve1->xRot = -HALF_PI + head->xRot;
				sleeve1->xRot -= attack2 * 1.2f - attack * 0.4f;
				sleeve1->zRot -= ((float) (Mth::cos(bob * 0.09f)) * 0.05f + 0.05f);
				sleeve1->xRot -= ((float) (Mth::sin(bob * 0.067f)) * 0.05f);
			}
		}
	}

	if (elytraFlying)
	{
		if (elytraCrouching)
		{
			arm0->xRot = PI;  arm0->yRot = 0.0f; arm0->zRot = 0.0f; arm0->y = 2.0f;
			if (sleeve0) { sleeve0->xRot = PI; sleeve0->yRot = 0.0f; sleeve0->zRot = 0.0f; sleeve0->y = 2.0f; }

			arm1->xRot = 0.0f; arm1->yRot = 0.0f; arm1->zRot = 0.0f; arm1->y = 2.0f;
			if (sleeve1) { sleeve1->xRot = 0.0f; sleeve1->yRot = 0.0f; sleeve1->zRot = 0.0f; sleeve1->y = 2.0f; }

			leg0->xRot = 0.0f; leg0->yRot = 0.0f; leg0->zRot = 0.0f;
			leg1->xRot = 0.0f; leg1->yRot = 0.0f; leg1->zRot = 0.0f;
			if (pants0) { pants0->xRot = 0.0f; pants0->yRot = 0.0f; pants0->zRot = 0.0f; }
			if (pants1) { pants1->xRot = 0.0f; pants1->yRot = 0.0f; pants1->zRot = 0.0f; }
		}
		else
		{
			float elytraTime = (float)(entity->tickCount) * 0.3f;  
			float spd2 = (float)(entity->xd * entity->xd + entity->yd * entity->yd + entity->zd * entity->zd);
			float fDamp = spd2 / 0.2f;
			fDamp = fDamp * fDamp * fDamp;
			if (fDamp < 1.0f) fDamp = 1.0f;

			float armAmp = 2.0f * r * 0.5f / fDamp;
			float legAmp = 1.4f * r / fDamp;

			arm0->xRot = Mth::cos(elytraTime + PI) * armAmp;
			arm0->yRot = 0.0f; arm0->zRot = 0.0f; arm0->y = 2.0f;
			if (sleeve0) { sleeve0->xRot = arm0->xRot; sleeve0->yRot = 0.0f; sleeve0->zRot = 0.0f; sleeve0->y = 2.0f; }

			arm1->xRot = Mth::cos(elytraTime) * armAmp;
			arm1->yRot = 0.0f; arm1->zRot = 0.0f; arm1->y = 2.0f;
			if (sleeve1) { sleeve1->xRot = arm1->xRot; sleeve1->yRot = 0.0f; sleeve1->zRot = 0.0f; sleeve1->y = 2.0f; }

			leg0->xRot = Mth::cos(elytraTime) * legAmp;
			leg0->yRot = 0.0f; leg0->zRot = 0.0f;
			leg1->xRot = Mth::cos(elytraTime + PI) * legAmp;
			leg1->yRot = 0.0f; leg1->zRot = 0.0f;
			if (pants0) { pants0->xRot = leg0->xRot; pants0->yRot = 0.0f; pants0->zRot = 0.0f; }
			if (pants1) { pants1->xRot = leg1->xRot; pants1->yRot = 0.0f; pants1->zRot = 0.0f; }
		}

		body->xRot = 0.0f;
		body->z = 0.0f;


		head->xRot = -(float)(PI / 4.0f);   
		hair->xRot = head->xRot;
	}

}

void HumanoidModel::renderHair(float scale,bool usecompiled)
{
    hair->yRot = head->yRot;
    hair->xRot = head->xRot;
    hair->render(scale,usecompiled);
}

void HumanoidModel::renderEars(float scale,bool usecompiled)
{
    ear->yRot = head->yRot;
    ear->xRot = head->xRot;
    ear->x=0;
    ear->y=0;
    ear->render(scale,usecompiled);
}

void HumanoidModel::renderCloak(float scale,bool usecompiled)
{
	cloak->render(scale,usecompiled);
}
void HumanoidModel::renderElytra(float scale, bool usecompiled)
{
	elytraRight->render(scale, usecompiled);
	elytraLeft->render(scale, usecompiled);
}

void HumanoidModel::render(HumanoidModel *model, float scale, bool usecompiled)
{
    head->yRot = model->head->yRot;
    head->y = model->head->y;
    head->xRot = model->head->xRot;
    hair->y = head->y;
    hair->yRot = head->yRot;
    hair->xRot = head->xRot;

    body->yRot = model->body->yRot;

	if (jacket)
		jacket->yRot = model->jacket->yRot;
							
    arm0->xRot = model->arm0->xRot;
    arm0->yRot = model->arm0->yRot;
    arm0->zRot = model->arm0->zRot;

	if (sleeve0)
	{
		sleeve0->xRot = model->sleeve0->xRot;
		sleeve0->yRot = model->sleeve0->yRot;
		sleeve0->zRot = model->sleeve0->zRot;
	}
						  
    arm1->xRot = model->arm1->xRot;
    arm1->yRot = model->arm1->yRot;
    arm1->zRot = model->arm1->zRot;

	if (sleeve1)
	{
		sleeve1->xRot = model->sleeve1->xRot;
		sleeve1->yRot = model->sleeve1->yRot;
		sleeve1->zRot = model->sleeve1->zRot;
	}
						  
    leg0->xRot = model->leg0->xRot;
    leg1->xRot = model->leg1->xRot;

	if (pants0)
		pants0->xRot = model->pants0->xRot;

	if (pants1)
		pants1->xRot = model->pants1->xRot;

	head->render(scale, usecompiled,(m_uiAnimOverrideBitmask&(1<<eAnim_DisableRenderHead))>0);
	body->render(scale, usecompiled,(m_uiAnimOverrideBitmask&(1<<eAnim_DisableRenderTorso))>0);
	arm0->render(scale, usecompiled,(m_uiAnimOverrideBitmask&(1<<eAnim_DisableRenderArm0))>0);
	arm1->render(scale, usecompiled,(m_uiAnimOverrideBitmask&(1<<eAnim_DisableRenderArm1))>0);
	leg0->render(scale, usecompiled,(m_uiAnimOverrideBitmask&(1<<eAnim_DisableRenderLeg0))>0);
	leg1->render(scale, usecompiled,(m_uiAnimOverrideBitmask&(1<<eAnim_DisableRenderLeg1))>0);
	hair->render(scale, usecompiled,(m_uiAnimOverrideBitmask&(1<<eAnim_DisableRenderHair))>0);

	if (jacket)
		jacket->render(scale, usecompiled,(m_uiAnimOverrideBitmask&(1<<eAnim_DisableRenderJacket))>0);
	if (sleeve0)
		sleeve0->render(scale, usecompiled,(m_uiAnimOverrideBitmask&(1<<eAnim_DisableRenderSleeve0))>0);
	if (sleeve1)
		sleeve1->render(scale, usecompiled,(m_uiAnimOverrideBitmask&(1<<eAnim_DisableRenderSleeve1))>0);
	if (pants0)
		pants0->render(scale, usecompiled,(m_uiAnimOverrideBitmask&(1<<eAnim_DisableRenderPants0))>0);
	if (pants1)
		pants1->render(scale, usecompiled,(m_uiAnimOverrideBitmask&(1<<eAnim_DisableRenderPants1))>0);
}

void HumanoidModel::setAllVisible(bool v) {
   
    head->visible  = v;
    hair->visible  = v;
    body->visible  = v;
    arm1->visible  = v;
    arm0->visible  = v;
    leg0->visible  = v;
    leg1->visible  = v;
}

void HumanoidModel::translateToHandItem(float scale) {
   
    arm1->translateTo(scale);
}

bool HumanoidModel::IsBodyPartDisabled(animbits bit) {
    return (m_uiAnimOverrideBitmask & (1u << bit)) != 0;
}

void HumanoidModel::copyPropertiesFrom(HumanoidModel* other) {
    if (!other) return;
   
    idle          = other->idle;
    sneaking      = other->sneaking;
    bowAndArrow   = other->bowAndArrow;
    eating        = other->eating;
    eating_t      = other->eating_t;
    eating_swing  = other->eating_swing;
    holdingLeftHand  = other->holdingLeftHand;
    holdingRightHand = other->holdingRightHand;
    m_uiAnimOverrideBitmask = other->m_uiAnimOverrideBitmask;
    m_fYOffset    = other->m_fYOffset;
}