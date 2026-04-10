#include "stdafx.h"
#include "net.minecraft.world.entity.player.h"
#include "net.minecraft.world.inventory.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.level.tile.h"
#include "net.minecraft.world.item.h"
#include "net.minecraft.world.item.enchantment.h"
#include "EnchantmentMenu.h"
#include "../../../Minecraft.Client/ServerPlayer.h"
#include "../../../Minecraft.Client/MinecraftServer.h"
#include "../../../Minecraft.Client/PlayerList.h"

EnchantmentMenu::EnchantmentMenu(shared_ptr<Inventory> inventory, Level *level, int xt, int yt, int zt)
{
	enchantSlots = std::make_shared<EnchantmentContainer>(this);
	lapisSlot = std::make_shared<EnchantmentContainer>(this, 64);
	playerT = inventory->player;
	for(int i = 0; i < 3; ++i)
	{
		costs[i] = 0;
	}

	this->level = level;
	x = xt;
	y = yt;
	z = zt;
	addSlot(new EnchantmentSlot(enchantSlots, 0, 21 + 4, 43 + 4));
	addSlot(new EnchantmentSlot(lapisSlot, 1, 40 + 4, 43 + 4));

	for (int y = 0; y < 3; y++)
	{
		for (int x = 0; x < 9; x++)
		{
			addSlot(new Slot(inventory, x + y * 9 + 9, 8 + x * 18, 84 + y * 18));
		}
	}
	for (int x = 0; x < 9; x++)
	{
		addSlot(new Slot(inventory, x, 8 + x * 18, 142));
	}

	m_costsChanged = false;
}

void EnchantmentMenu::addSlotListener(ContainerListener *listener)
{
	AbstractContainerMenu::addSlotListener(listener);

	listener->setContainerData(this, 0, costs[0]);
	listener->setContainerData(this, 1, costs[1]);
	listener->setContainerData(this, 2, costs[2]);
}

void EnchantmentMenu::broadcastChanges()
{
	AbstractContainerMenu::broadcastChanges();

	// 4J Added m_costsChanged to stop continually sending update packets even when no changes have been made
	if(m_costsChanged)
	{
		/*if (EnchantmentMenu::lapisSlot->getItem(1) == nullptr) {
			return;
		}*/
		for (size_t i = 0; i < containerListeners.size(); i++)
		{
			ContainerListener *listener = containerListeners.at(i);
			listener->setContainerData(this, 0, costs[0]);
			listener->setContainerData(this, 1, costs[1]);
			listener->setContainerData(this, 2, costs[2]);
		}
		m_costsChanged = false;
	}
}

void EnchantmentMenu::setData(int id, int value)
{
	if (id >= 0 && id <= 2)
	{
		costs[id] = value;
		m_costsChanged = true;
	}
	else
	{
		AbstractContainerMenu::setData(id, value);
	}
}

vector<EnchantmentInstance*> EnchantmentMenu::getEnchantment() {
	random.setSeed(playerT->enchantmentSeed);
	shared_ptr<ItemInstance> item = enchantSlots->getItem(0);
	return *EnchantmentHelper::selectEnchantment(&random, item, costs[2]);
}

void EnchantmentMenu::slotsChanged(int a) // 4J used to take a shared_ptr<Container> container but wasn't using it, so removed to simplify things
{
	shared_ptr<ItemInstance> item = enchantSlots->getItem(0);

	shared_ptr<ItemInstance> lapis = lapisSlot->getItem(1);

	if (item == nullptr || !item->isEnchantable())
	{
		for (int i = 0; i < 3; i++)
		{
			tempCosts[i] = costs[i];
			costs[i] = 0;
		}
		m_costsChanged = true;
		alreadyRan = false;
		
	}
	else
	{
		if (!level->isClientSide)
		{
			// find book cases
			int bookcases = 0;
			for (int oz = -1; oz <= 1; oz++)
			{
				for (int ox = -1; ox <= 1; ox++)
				{
					if (oz == 0 && ox == 0)
					{
						continue;
					}

					if (level->isEmptyTile(x + ox, y, z + oz) && level->isEmptyTile(x + ox, y + 1, z + oz))
					{
						if (level->getTile(x + ox * 2, y, z + oz * 2) == Tile::bookshelf_Id)
						{
							bookcases++;
						}
						if (level->getTile(x + ox * 2, y + 1, z + oz * 2) == Tile::bookshelf_Id)
						{
							bookcases++;
						}
						// corners
						if (ox != 0 && oz != 0)
						{
							if (level->getTile(x + ox * 2, y, z + oz) == Tile::bookshelf_Id)
							{
								bookcases++;
							}
							if (level->getTile(x + ox * 2, y + 1, z + oz) == Tile::bookshelf_Id)
							{
								bookcases++;
							}
							if (level->getTile(x + ox, y, z + oz * 2) == Tile::bookshelf_Id)
							{
								bookcases++;
							}
							if (level->getTile(x + ox, y + 1, z + oz * 2) == Tile::bookshelf_Id)
							{
								bookcases++;
							}
						}
					}
				}
			}
			bookcasesC = bookcases;
			random.setSeed(playerT->enchantmentSeed);

			for (int i = 0; i < 3; i++)
			{
				costs[i] = EnchantmentHelper::getEnchantmentCost(&random, i, bookcases, item);
			}
			if (!alreadyRan) {
				en = false;
				for (int i = 0; i < 3; i++)
				{
					//delete cachedEnchantments[i]; // clean up old one first <- doing it elsewhere
					cachedEnchantments[i] = EnchantmentHelper::selectEnchantment(&random, item, costs[i]);
				}
				
			}
			
			alreadyRan = true;
			m_costsChanged = true;
			broadcastChanges();
		}
	}

	wasLapis = lapis != nullptr;
}

bool EnchantmentMenu::clickMenuButton(shared_ptr<Player> player, int i)
{
	shared_ptr<ItemInstance> item = enchantSlots->getItem(0);
	if (costs[i] > 0 && item != nullptr && (player->experienceLevel >= costs[i] || player->abilities.instabuild) )
	{
		bool enough = getLapisCount() >= i+1;
		if (!enough && !player->abilities.instabuild) return false;
		if (!level->isClientSide)
		{
			bool isBook = item->id == Item::book_Id;

			vector<EnchantmentInstance *> *newEnchantment = cachedEnchantments[i];
			if (newEnchantment != nullptr)
			{
				
				player->giveExperienceLevels(-costs[i]);
				if (isBook) item->id = Item::enchantedBook_Id;
				int randomIndex = isBook ? random.nextInt(newEnchantment->size()) : -1;
				
				for (int index = 0; index < newEnchantment->size(); index++)
				{
					EnchantmentInstance* e = newEnchantment->at(index);
					if (isBook)
					{
						Item::enchantedBook->addEnchantment(item, e);
						en = true;
					}
					else
					{
						item->enchant(e->enchantment, e->level);
						en = true;
					}
					delete e;
				}
				delete newEnchantment;
				slotsChanged(1);// Removed enchantSlots parameter as the function can reference it directly
			}
		}
		if (!player->abilities.instabuild) lapisSlot->removeItem(1, i+1);
		nameSeed = (long)nameSeed + random.nextInt(10000);
		player->enchantmentSeed = random.nextInt(1000000);
		alreadyRan = false;
		return true;
	}
	return false;
}

EnchantmentInstance* EnchantmentMenu::predictEnchantment(shared_ptr<Player> player, int i)
{
	shared_ptr<ItemInstance> item = enchantSlots->getItem(0);
	if (costs[i] > 0 && item != nullptr && (player->experienceLevel >= costs[i] || player->abilities.instabuild))
	{
		if (0 == 0)
		{
			bool isBook = item->id == Item::book_Id;
			Random tempRandom = random;
			vector<EnchantmentInstance*>* newEnchantment = cachedEnchantments[i];
			if (newEnchantment != nullptr)
			{
				int randomIndex = isBook ? tempRandom.nextInt(newEnchantment->size()) : -1;
				for (int index = 0; index < newEnchantment->size(); index++)
				{
					EnchantmentInstance* e = newEnchantment->at(index);
					if (isBook)
					{
						return e;
						//en = true;
					}
					else
					{
						//item->enchant(e->enchantment, e->level);
						return e;
						//en = true;
					}
					//delete e;
				}
				//delete newEnchantment;
				//slotsChanged(1);// Removed enchantSlots parameter as the function can reference it directly
			}
		}
		return nullptr;
	}
	return nullptr;
}


void EnchantmentMenu::removed(shared_ptr<Player> player)
{
	AbstractContainerMenu::removed(player);
	if (level->isClientSide) return;

	shared_ptr<ItemInstance> item = enchantSlots->removeItemNoUpdate(0);
	if (item != nullptr)
	{
		player->drop(item);
	}
	item = lapisSlot->removeItemNoUpdate(1);
	if (item != nullptr)
	{
		player->drop(item);
	}
}

bool EnchantmentMenu::stillValid(shared_ptr<Player> player) 
{
	if (level->getTile(x, y, z) != Tile::enchantTable_Id) return false;
	if (player->distanceToSqr(x + 0.5, y + 0.5, z + 0.5) > 8 * 8) return false;
	return true;
}

int EnchantmentMenu::getLapisCount() {
	if (lapisSlot->getItem(1) == nullptr) return 0;
	return lapisSlot->getItem(1)->count;
}

shared_ptr<ItemInstance> EnchantmentMenu::quickMoveStack(shared_ptr<Player> player, int slotIndex)
{
	if (slotIndex < 0 || slotIndex >= static_cast<int>(slots.size()))
	{
		return nullptr;
	}

	shared_ptr<ItemInstance> clicked = nullptr;
	Slot *slot = slots.at(slotIndex);
	Slot *IngredientSlot = nullptr;
	Slot *LapisSlot = nullptr;
	if (INGREDIENT_SLOT >= 0 && INGREDIENT_SLOT < static_cast<int>(slots.size()))
	{
		IngredientSlot = slots.at(INGREDIENT_SLOT);
	}
	if (LAPIS_SLOT >= 0 && LAPIS_SLOT < static_cast<int>(slots.size()))
	{
		LapisSlot = slots.at(LAPIS_SLOT);
	}

	if (slot != nullptr && slot->hasItem())
	{
		shared_ptr<ItemInstance> stack = slot->getItem();
		clicked = stack->copy();

		if (slotIndex == INGREDIENT_SLOT)
		{
			if (!moveItemStackTo(stack, INV_SLOT_START, INV_SLOT_END, true))
			{
				if (!moveItemStackTo(stack, USE_ROW_SLOT_START, USE_ROW_SLOT_END, false))
				{
					return nullptr;
				}

			}
		}
		else if (slotIndex == LAPIS_SLOT) {
			if (!moveItemStackTo(stack, INV_SLOT_START, INV_SLOT_END, true))
			{
				if (!moveItemStackTo(stack, USE_ROW_SLOT_START, USE_ROW_SLOT_END, false))
				{
					return nullptr;
				}

			}
		}
		else if (slotIndex >= INV_SLOT_START && slotIndex < INV_SLOT_END)
		{
			// if the item is an enchantable tool
			
			if(stack->isEnchantable() && (!IngredientSlot->hasItem() )  ) 
			{
				if(!moveItemStackTo(stack, INGREDIENT_SLOT, INGREDIENT_SLOT+1, false))
				{
					return nullptr;
				}
			}
			//DyePowderItem::BLACK
			else if (stack->getItem()->id == 351 && stack->getItem()->getMaterial() == 11)
			{
				if(!moveItemStackTo(stack, LAPIS_SLOT, LAPIS_SLOT+1, false))
				{
					return nullptr;
				}
			}
			else
			{
				if(!moveItemStackTo(stack, USE_ROW_SLOT_START, USE_ROW_SLOT_END, false))
				{
					return nullptr;
				}
			}
		}
		else if (slotIndex >= USE_ROW_SLOT_START && slotIndex < USE_ROW_SLOT_END)
		{
			// if the item is an enchantable tool

			if(stack->isEnchantable() && (!IngredientSlot->hasItem() )  ) 
			{
				if(!moveItemStackTo(stack, INGREDIENT_SLOT, INGREDIENT_SLOT+1, false))
				{
					return nullptr;
				}
			}
			else if (stack->getItem()->id == 351 && stack->getItem()->getMaterial() == 11)
			{
				if (!moveItemStackTo(stack, LAPIS_SLOT, LAPIS_SLOT + 1, false))
				{
					return nullptr;
				}
			}
			else
			{
				if(!moveItemStackTo(stack, INV_SLOT_START, INV_SLOT_END, false))
				{
					return nullptr;
				}
			}
		}
		else
		{
			return nullptr;
		}

		if (stack->count == 0)
		{
			slot->set(nullptr);
		}
		else
		{
			slot->setChanged();
		}
		if (stack->count == clicked->count)
		{
			return nullptr;
		}
		else
		{
			slot->onTake(player, stack);
		}
	}
	return clicked;
}