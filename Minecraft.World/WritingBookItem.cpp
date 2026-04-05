#include "stdafx.h"
#include "Item.h"
#include "Player.h"
#include "ItemInstance.h"
#include "WritingBookItem.h"
#include "../Minecraft.Client/Minecraft.h"
#include "../Minecraft.Client/MultiplayerLocalPlayer.h"

WritingBookItem::WritingBookItem(int id) : Item(id)
{
	setMaxStackSize(1);
}

shared_ptr<ItemInstance> WritingBookItem::use(shared_ptr<ItemInstance> instance, Level *level, shared_ptr<Player> player)
{
	//shared_ptr<MultiplayerLocalPlayer> player1 = Minecraft::GetInstance()->player;
	//player1->openItemInstanceGui(instance, player1);
	player->openItemInstanceGui(instance, player);

	return instance;
}

bool WritingBookItem::TestUse(shared_ptr<ItemInstance> itemInstance, Level* level, shared_ptr<Player> player)
{
	return true;
}