#pragma once

#include "Item.h"

class ElytraItem : public Item
{
public:
    ElytraItem();

    static bool isFlyEnabled(shared_ptr<ItemInstance> item);

    virtual bool TestUse(shared_ptr<ItemInstance> instance, Level* level, shared_ptr<Player> player) override;
    virtual shared_ptr<ItemInstance> use(shared_ptr<ItemInstance> instance, Level* level, shared_ptr<Player> player) override;
    virtual bool isValidRepairItem(shared_ptr<ItemInstance> source, shared_ptr<ItemInstance> repairItem) override;
    virtual void registerIcons(IconRegister* iconRegister) override;
    virtual Icon* getLayerIcon(int auxValue, int spriteLayer) override;
    virtual Icon* getIcon(int auxValue) override;

    Icon* m_brokenElytraIcon = nullptr;
};
