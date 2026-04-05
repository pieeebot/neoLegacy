#include "stdafx.h"
#include "HtmlString.h"
#include "WrittenBook.h"
#include "../Minecraft.Client/Minecraft.h"
#include "../Minecraft.Client/MultiplayerLocalPlayer.h"

WrittenBookItem::WrittenBookItem(int id) : Item(id)
{
	
}

bool WrittenBookItem::isFoil(shared_ptr<ItemInstance> itemInstance)
{
	return true;
}

const Rarity* WrittenBookItem::getRarity(shared_ptr<ItemInstance> itemInstance)
{
	return Rarity::common;
}

shared_ptr<ItemInstance> WrittenBookItem::use(shared_ptr<ItemInstance> instance, Level* level, shared_ptr<Player> player)
{
	//shared_ptr<MultiplayerLocalPlayer> player1 = Minecraft::GetInstance()->player;
	//player1->openItemInstanceGui(instance, player1);
	player->openItemInstanceGui(instance, player);

	return instance;
}
bool WrittenBookItem::TestUse(shared_ptr<ItemInstance> itemInstance, Level* level, shared_ptr<Player> player)
{
	return true;
}

void WrittenBookItem::appendHoverText(shared_ptr<ItemInstance> itemInstance, shared_ptr<Player> player, vector<HtmlString>* lines, bool advanced)
{
	if (!itemInstance->hasTag())
	{
		return;
	}
	HtmlString stringd = HtmlString(L"By " + (itemInstance->tag->getString(L"author")), eHTMLColor_7, false, false);
	HtmlString stringf = HtmlString(L"Original", eHTMLColor_7, false, false);


	lines->push_back(stringd);
	lines->push_back(stringf);
	//lines->push_back(wstring(L"tone"));
}