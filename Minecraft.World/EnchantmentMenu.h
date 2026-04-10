#pragma once
#include "AbstractContainerMenu.h"
#include "Random.h"
#include "EnchantmentInstance.h"

class EnchantmentMenu : public AbstractContainerMenu
{
	// 4J Stu Made these public for UI menus, perhaps should make friend class?
public:
	static const int INGREDIENT_SLOT = 0;
	static const int LAPIS_SLOT = 1;
	static const int INV_SLOT_START = EnchantmentMenu::INGREDIENT_SLOT + 2;
	static const int INV_SLOT_END = EnchantmentMenu::INV_SLOT_START + 9 * 3;
	static const int USE_ROW_SLOT_START = EnchantmentMenu::INV_SLOT_END;
	static const int USE_ROW_SLOT_END = EnchantmentMenu::USE_ROW_SLOT_START + 9;

public:
	shared_ptr<Container> enchantSlots;
	shared_ptr<Container> lapisSlot;
	bool alreadyRan = false;
	bool en = false;
	// Header
	std::vector<EnchantmentInstance*>* cachedEnchantments[3] = { nullptr, nullptr, nullptr };
	int getLapisCount();
	//int iPad;
	Player* playerT;

private:
	Level *level;
	int x, y, z;
	

	bool m_costsChanged; // 4J Added
	int bookcasesC = -1;

public:
	int64_t nameSeed;
	bool wasLapis;
	
	Random random;
public:
	int costs[3];
	int tempCosts[3];
	

	EnchantmentMenu(shared_ptr<Inventory> inventory, Level *level, int xt, int yt, int zt);

	virtual void addSlotListener(ContainerListener *listener);
	virtual void broadcastChanges();
	vector<EnchantmentInstance*> getEnchantment();
	virtual void setData(int id, int value);
	virtual void slotsChanged(int a);// 4J used to take a shared_ptr<Container> container but wasn't using it, so removed to simplify things
	virtual bool clickMenuButton(shared_ptr<Player> player, int i);
	virtual EnchantmentInstance* predictEnchantment(shared_ptr<Player> player, int i);
	void removed(shared_ptr<Player> player);
	virtual bool stillValid(shared_ptr<Player> player);
	virtual shared_ptr<ItemInstance> quickMoveStack(shared_ptr<Player> player, int slotIndex);
};