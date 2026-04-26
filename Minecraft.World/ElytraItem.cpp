#include "stdafx.h"
#include "net.minecraft.world.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.entity.h"
#include "net.minecraft.world.entity.player.h"
#include "net.minecraft.world.item.h"
#include "ElytraItem.h"

// Internal item ID: 187 → public item ID: 443 (256 + 187)
ElytraItem::ElytraItem() : Item(187)
{
    maxStackSize = 1;
    setMaxDamage(432);
}

bool ElytraItem::isFlyEnabled(shared_ptr<ItemInstance> item)
{
    return item->getDamageValue() < item->getMaxDamage() - 1;
}

bool ElytraItem::TestUse(shared_ptr<ItemInstance> instance, Level* level, shared_ptr<Player> player)
{
    return true;
}

shared_ptr<ItemInstance> ElytraItem::use(shared_ptr<ItemInstance> instance, Level* level, shared_ptr<Player> player)
{
    // Elytra equips to the chest slot (SLOT_CHEST = 3, armor array index = SLOT_CHEST - 1 = 2)
    const int chestSlot = LivingEntity::SLOT_CHEST - 1;

    ItemInstance copy = *instance->copy_not_shared();

    if (!player->abilities.instabuild)
    {
        if (player->inventory->armor[chestSlot] == nullptr)
        {
            player->inventory->armor[chestSlot] = make_shared<ItemInstance>(copy);
            player->inventory->removeItemNoUpdate(player->inventory->selected);
            instance->count = 0;
        }
        else
        {
            player->inventory->setItem(player->inventory->selected, player->inventory->armor[chestSlot]);
            player->inventory->armor[chestSlot] = make_shared<ItemInstance>(copy);
        }
    }
    else
    {
        if (player->inventory->armor[chestSlot] == nullptr)
        {
            player->inventory->armor[chestSlot] = make_shared<ItemInstance>(copy);
        }
        else
        {
            player->inventory->setItem(player->inventory->selected, player->inventory->armor[chestSlot]);
            player->inventory->armor[chestSlot] = make_shared<ItemInstance>(copy);
        }
    }

    // Play cloth armor equip sound (range 194–199)
    player->playSound(194, 0.5f, 1.0f);

    return instance;
}

bool ElytraItem::isValidRepairItem(shared_ptr<ItemInstance> source, shared_ptr<ItemInstance> repairItem)
{
    return repairItem->id == Item::leather_Id;
}

void ElytraItem::registerIcons(IconRegister* iconRegister)
{
    Item::registerIcons(iconRegister);
    m_brokenElytraIcon = iconRegister->registerIcon(L"broken_elytra");
}

Icon* ElytraItem::getLayerIcon(int auxValue, int spriteLayer)
{
    // auxValue may be the damage value in certain render paths
    if (auxValue < getMaxDamage() - 1)
        return icon;
    else
        return m_brokenElytraIcon;
}

Icon* ElytraItem::getIcon(int auxValue)
{
    if (auxValue < getMaxDamage() - 1)
        return icon;
    else
        return m_brokenElytraIcon;
}
