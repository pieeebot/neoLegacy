#pragma once

#include "Item.h"

class WrittenBookItem : public Item 
{
public:

	wstring authorName = L"Unknown";

	WrittenBookItem(int id);
	bool isFoil(shared_ptr<ItemInstance> itemInstance);
	const Rarity* getRarity(shared_ptr<ItemInstance> itemInstance) override;
	void appendHoverText(shared_ptr<ItemInstance> itemInstance, shared_ptr<Player> player, vector<HtmlString>* lines, bool advanced);

	virtual shared_ptr<ItemInstance> use(shared_ptr<ItemInstance> instance, Level* level, shared_ptr<Player> player);
	virtual bool TestUse(shared_ptr<ItemInstance> itemInstance, Level* level, shared_ptr<Player> player);
};