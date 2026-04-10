#include "stdafx.h"
#include "net.minecraft.world.inventory.h"
#include "EnchantmentContainer.h"



EnchantmentContainer::EnchantmentContainer(EnchantmentMenu *menu, int size) : SimpleContainer(IDS_ENCHANT, L"", false, 2), m_menu( menu )
{
	MAX_STACK_SIZE = size;
}

int EnchantmentContainer::getMaxStackSize() const
{
	return MAX_STACK_SIZE;
}

void EnchantmentContainer::setChanged()
{
	SimpleContainer::setChanged();
	m_menu->slotsChanged(MAX_STACK_SIZE);
}

bool EnchantmentContainer::canPlaceItem(int slot, shared_ptr<ItemInstance> item)
{
	return true;
}