#include "stdafx.h"

#include "MerchantRecipe.h"

void MerchantRecipe::_init(shared_ptr<ItemInstance> buyA, shared_ptr<ItemInstance> buyB, shared_ptr<ItemInstance> sell)
{
	this->buyA = buyA;
	this->buyB = buyB;
	this->sell = sell;
	uses = 0;
	maxUses = 7;
}

MerchantRecipe::MerchantRecipe(CompoundTag *tag)
{
	buyA = nullptr;
	buyB = nullptr;
	sell = nullptr;
	uses = 0;
	load(tag);
}

MerchantRecipe::MerchantRecipe(shared_ptr<ItemInstance> buyA, shared_ptr<ItemInstance> buyB, shared_ptr<ItemInstance> sell, int uses, int maxUses)
{
	_init(buyA, buyB, sell);
	this->uses = uses;
	this->maxUses = maxUses;
}

MerchantRecipe::MerchantRecipe(shared_ptr<ItemInstance> buy, shared_ptr<ItemInstance> sell)
{
	_init(buy, nullptr, sell);
}

MerchantRecipe::MerchantRecipe(shared_ptr<ItemInstance> buy, Item *sell)
{
	_init(buy, nullptr, std::make_shared<ItemInstance>(sell));
}

MerchantRecipe::MerchantRecipe(shared_ptr<ItemInstance> buy, Tile *sell)
{
	_init(buy, nullptr, std::make_shared<ItemInstance>(sell));
}

shared_ptr<ItemInstance> MerchantRecipe::getBuyAItem()
{
	return buyA;
}

shared_ptr<ItemInstance> MerchantRecipe::getBuyBItem()
{
	return buyB;
}

bool MerchantRecipe::hasSecondaryBuyItem()
{
	return buyB != nullptr;
}

shared_ptr<ItemInstance> MerchantRecipe::getSellItem()
{
	return sell;
}

bool MerchantRecipe::isSame(MerchantRecipe *other)
{
	if (!other || !buyA || !sell || !other->buyA || !other->sell)
	{
		return false;
	}
	if (buyA->id != other->buyA->id || sell->id != other->sell->id)
	{
		return false;
	}
	return (buyB == nullptr && other->buyB == nullptr) || (buyB != nullptr && other->buyB != nullptr && buyB->id == other->buyB->id);
}

bool MerchantRecipe::isSameSameButBetter(MerchantRecipe *other)
{
	// same deal, but cheaper
	if (!isSame(other) || !buyA || !other || !other->buyA)
	{
		return false;
	}
	return buyA->count < other->buyA->count || (buyB != nullptr && buyB->count < other->buyB->count);
}

int MerchantRecipe::getUses()
{
	return uses;
}

int MerchantRecipe::getMaxUses()
{
	return maxUses;
}

void MerchantRecipe::increaseUses()
{
	uses++;
}

void MerchantRecipe::increaseMaxUses(int amount)
{
	maxUses += amount;
}

bool MerchantRecipe::isDeprecated()
{
	return uses >= maxUses;
}

void MerchantRecipe::enforceDeprecated()
{
	uses = maxUses;
}

void MerchantRecipe::load(CompoundTag *tag)
{
	CompoundTag *buyTag = tag->getCompound(L"buy");
	buyA = buyTag ? ItemInstance::fromTag(buyTag) : nullptr;
	CompoundTag *sellTag = tag->getCompound(L"sell");
	sell = sellTag ? ItemInstance::fromTag(sellTag) : nullptr;
	if (tag->contains(L"buyB"))
	{
		buyB = ItemInstance::fromTag(tag->getCompound(L"buyB"));
	}
	if (tag->contains(L"uses"))
	{
		uses = tag->getInt(L"uses");
	}
	if (tag->contains(L"maxUses"))
	{
		maxUses = tag->getInt(L"maxUses");
	}
	else
	{
		maxUses = 7;
	}
}

CompoundTag *MerchantRecipe::createTag()
{
	CompoundTag *tag = new CompoundTag();
	if (buyA != nullptr)
	{
		tag->putCompound(L"buy", buyA->save(new CompoundTag(L"buy")));
	}
	if (sell != nullptr)
	{
		tag->putCompound(L"sell", sell->save(new CompoundTag(L"sell")));
	}
	if (buyB != nullptr)
	{
		tag->putCompound(L"buyB", buyB->save(new CompoundTag(L"buyB")));
	}
	tag->putInt(L"uses", uses);
	tag->putInt(L"maxUses", maxUses);
	return tag;
}