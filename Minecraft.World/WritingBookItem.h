#pragma once

#include "Item.h"

class WritingBookItem : public Item 
{
public:
	WritingBookItem(int id);

	virtual shared_ptr<ItemInstance> use(shared_ptr<ItemInstance> instance, Level *level, shared_ptr<Player> player);
	virtual bool TestUse(shared_ptr<ItemInstance> itemInstance, Level* level, shared_ptr<Player> player);
};