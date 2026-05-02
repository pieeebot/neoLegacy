#include "stdafx.h"
#include "TallGrass2.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.level.biome.h"
#include "net.minecraft.world.item.h"
#include "net.minecraft.world.h"
#include "net.minecraft.h"
#include "../Minecraft.Client/Minecraft.h"
#include "net.minecraft.stats.h"


// tranq please i beg you make sure the ids are correct so we dont get corrupted worlds from people
static const int TILE_IDS[TallGrass2::VARIANT_COUNT] = {
	IDS_TILE_SUNFLOWER,			  // 0 - Sunflower, not implemented yet
	IDS_TILE_LILAC,				  // 1 - Lilac
	IDS_TILE_DOUBLE_TALL_GRASS,   // 2 - Tall Grass
	IDS_TILE_LARGE_FERN,		  // 3 - Large Fern
	IDS_TILE_ROSE_BUSH,			  // 4 - Rose Bush
	IDS_TILE_PEONY,				  // 5 - Peony
};

static const int DESCRIPTION_IDS[TallGrass2::VARIANT_COUNT] = {
	IDS_DESC_SUNFLOWER,			  // 0 - Sunflower, not implemented yet
	IDS_DESC_LILAC,				  // 1 - Lilac
	IDS_DESC_DOUBLE_TALL_GRASS,   // 2 - Tall Grass
	IDS_DESC_LARGE_FERN,		  // 3 - Large Fern
	IDS_DESC_ROSE_BUSH,			  // 4 - Rose Bush
	IDS_DESC_PEONY,				  // 5 - Peony
};


static const wstring TEXTURE_BOTTOM[TallGrass2::VARIANT_COUNT] = {
	L"tallgrass2_tall_grass_lower",		// Sunflower, not implemented yet
	L"tallgrass2_lilac_lower",
	L"tallgrass2_tall_grass_lower",
	L"tallgrass2_large_fern_lower",
	L"tallgrass2_rose_bush_lower",
	L"tallgrass2_peony_lower",
};

static const wstring TEXTURE_TOP[TallGrass2::VARIANT_COUNT] = {
	L"tallgrass2_tall_grass_upper",		// Sunflower, not implemented yet
	L"tallgrass2_lilac_upper",
	L"tallgrass2_tall_grass_upper",
	L"tallgrass2_large_fern_upper",
	L"tallgrass2_rose_bush_upper",
	L"tallgrass2_peony_upper"
};


TallGrass2::TallGrass2(int id) : Bush(id, Material::replaceable_plant)
{
	this->updateDefaultShape();
}

void TallGrass2::createBlockStateDefinition()
{
	if (!m_blockStateDefinition)
		m_blockStateDefinition = new BlockStateDefinition(this);
}

int TallGrass2::defaultBlockState()
{
	return 0;
}

int TallGrass2::convertBlockStateToLegacyData(BlockState *state)
{
	return state ? (state->value & 0xF) : 0;
}

Tile::BlockState TallGrass2::getBlockState(int data)
{
	return Tile::BlockState(data & 0xF);
}

Tile::BlockState TallGrass2::getBlockState(LevelSource* level, int x, int y, int z)
{
	return Tile::BlockState(level->getData(x, y, z) & 0xF);
}


void TallGrass2::updateDefaultShape()
{
	float ss = 0.4f;
	this->setShape(0.5f - ss, 0.0f, 0.5f - ss, 0.5f + ss, 1.0f, 0.5f + ss);
}


bool TallGrass2::blocksLight() { return false; }
bool TallGrass2::isSolidRender(bool) { return false; }
bool TallGrass2::isCubeShaped() { return false; }
int  TallGrass2::getRenderShape() { return Tile::SHAPE_CROSS_TEXTURE; }


bool TallGrass2::isGrassColored(int variant)
{
	return variant == TALL_GRASS || variant == LARGE_FERN;
}


void TallGrass2::registerIcons(IconRegister* iconRegister)
{
	for (int i = 0; i < VARIANT_COUNT; i++)
	{
		iconBottom[i] = iconRegister->registerIcon(TEXTURE_BOTTOM[i]);
		iconTop[i] = iconRegister->registerIcon(TEXTURE_TOP[i]);
	}

	icon = iconTop[TALL_GRASS];
}


Icon* TallGrass2::getTexture(int face, int data)
{
	bool isUpper = (data & UPPER_BIT) != 0;
	int variant = data & ~UPPER_BIT;
	if (variant < 0 || variant >= VARIANT_COUNT) variant = 0;

	if (face == Facing::UP && !isUpper) return iconTop[variant];

	return isUpper ? iconTop[variant] : iconBottom[variant];
}

Icon* TallGrass2::getTexture(LevelSource* level, int x, int y, int z, int face)
{
	int data = level->getData(x, y, z);
	bool isUpper = (data & UPPER_BIT) != 0;
	int variant = data & ~UPPER_BIT;
	if (variant < 0 || variant >= VARIANT_COUNT) variant = 0;
	return isUpper ? iconTop[variant] : iconBottom[variant];
}


int TallGrass2::getVariant(LevelSource* level, int x, int y, int z)
{
	int data = level->getData(x, y, z);
	bool isUpper = (data & UPPER_BIT) != 0;
	int lowerData = isUpper ? level->getData(x, y - 1, z) : data;
	int variant = lowerData & ~UPPER_BIT;
	if (variant < 0 || variant >= VARIANT_COUNT) variant = 0;
	return variant;
}


int TallGrass2::getColor(int auxData)
{
	int variant = auxData & ~UPPER_BIT;
	if (variant < 0 || variant >= VARIANT_COUNT) variant = 0;
	if (!isGrassColored(variant)) return 0xFFFFFF;
	return Minecraft::GetInstance()->getColourTable()->getColor(eMinecraftColour_Grass_Common);
}

int TallGrass2::getColor() const
{
	return Minecraft::GetInstance()->getColourTable()->getColor(eMinecraftColour_Grass_Common);
}

int TallGrass2::getColor(LevelSource* level, int x, int y, int z)
{
	return getColor(level, x, y, z, level->getData(x, y, z));
}

int TallGrass2::getColor(LevelSource* level, int x, int y, int z, int data)
{
	int variant = data & ~UPPER_BIT;
	if (variant < 0 || variant >= VARIANT_COUNT) variant = 0;
	if (!isGrassColored(variant)) return 0xFFFFFF;
	return level->getBiome(x, z)->getGrassColor();
}


bool TallGrass2::mayPlace(Level* level, int x, int y, int z)
{
	if (y >= Level::maxBuildHeight - 1) return false;
	return Bush::mayPlaceOn(level->getTile(x, y - 1, z))
		&& level->getTile(x, y, z) == 0
		&& level->getTile(x, y + 1, z) == 0;
}
void TallGrass2::finalizePlacement(Level* level, int x, int y, int z, int data)
{
	if ((data & UPPER_BIT) != 0) return;
	int variant = data & ~UPPER_BIT;
	level->setTileAndData(x, y + 1, z, id, variant | UPPER_BIT, Tile::UPDATE_ALL);
	level->setTilesDirty(x - 1, y - 1, z - 1, x + 1, y + 2, z + 1);
}

void TallGrass2::onPlace(Level* level, int x, int y, int z)
{
	int data = level->getData(x, y, z);
	if ((data & UPPER_BIT) != 0) return;

	int variant = data & ~UPPER_BIT;
	level->setTileAndData(x, y + 1, z, id, variant | UPPER_BIT, Tile::UPDATE_ALL);
	level->setTilesDirty(x - 1, y - 1, z - 1, x + 1, y + 2, z + 1);
}


bool TallGrass2::canSurvive(Level* level, int x, int y, int z)
{
	int data = level->getData(x, y, z);
	bool isUpper = (data & UPPER_BIT) != 0;

	if (isUpper)
		return level->getTile(x, y - 1, z) == id;

	return (level->getDaytimeRawBrightness(x, y, z) >= 8 || level->canSeeSky(x, y, z))
		&& mayPlaceOn(level->getTile(x, y - 1, z));
}

void TallGrass2::neighborChanged(Level* level, int x, int y, int z, int type)
{
	int data = level->getData(x, y, z);
	bool isUpper = (data & UPPER_BIT) != 0;

	if (!isUpper)
	{
		if (!canSurvive(level, x, y, z))
		{
			spawnResources(level, x, y, z, data, 0);
			level->setTileAndData(x, y, z, 0, 0, Tile::UPDATE_CLIENTS);
			if (level->getTile(x, y + 1, z) == id)
				level->removeTile(x, y + 1, z);
		}
	}
	else
	{
		if (level->getTile(x, y - 1, z) != id)
			level->removeTile(x, y, z);
	}
}

void TallGrass2::tick(Level* level, int x, int y, int z, Random* random)
{
	int data = level->getData(x, y, z);
	bool isUpper = (data & UPPER_BIT) != 0;

	if (!isUpper)
	{
		if (!canSurvive(level, x, y, z))
		{
			spawnResources(level, x, y, z, data, 0);
			level->setTileAndData(x, y, z, 0, 0, Tile::UPDATE_CLIENTS);
			if (level->getTile(x, y + 1, z) == id)
				level->removeTile(x, y + 1, z);
		}
	}
}


int TallGrass2::getResource(int data, Random* random, int playerBonusLevel)
{

	return -1;
}

int TallGrass2::getResourceCountForLootBonus(int bonusLevel, Random* random)
{
	return 1;
}

bool TallGrass2::isSilkTouchable()
{
	return true;
}

shared_ptr<ItemInstance> TallGrass2::getSilkTouchItemInstance(int data)
{

	if ((data & UPPER_BIT) != 0) return nullptr;
	int variant = data & ~UPPER_BIT;
	return std::make_shared<ItemInstance>(this, 1, variant);
}

void TallGrass2::playerDestroy(Level* level, shared_ptr<Player> player, int x, int y, int z, int data)
{
	if (!level->isClientSide
		&& player->getSelectedItem() != nullptr
		&& player->getSelectedItem()->id == Item::shears->id)
	{
		player->awardStat(
			GenericStats::blocksMined(id),
			GenericStats::param_blocksMined(id, data, 1));

		if ((data & UPPER_BIT) == 0)
		{
			int variant = data & ~UPPER_BIT;
			popResource(level, x, y, z, std::make_shared<ItemInstance>(this, 1, variant));
		}
	}
	else
	{

		Tile::playerDestroy(level, player, x, y, z, data);
	}
}

void TallGrass2::playerWillDestroy(Level* level, int x, int y, int z, int data, shared_ptr<Player> player)
{
	if (player->abilities.instabuild)
	{
		if ((data & UPPER_BIT) != 0)
		{
			if (level->getTile(x, y - 1, z) == id)
				level->removeTile(x, y - 1, z);
		}
		else
		{
			if (level->getTile(x, y + 1, z) == id)
				level->removeTile(x, y + 1, z);
		}
	}
}

int TallGrass2::cloneTileData(Level* level, int x, int y, int z)
{
	return getVariant(level, x, y, z);
}

unsigned int TallGrass2::getDescriptionId(int iData)
{
	if (iData < 0) iData = 0;
	if (iData >= VARIANT_COUNT) iData = 0;
	return DESCRIPTION_IDS[iData];
}

int TallGrass2::getPistonPushReaction()
{
	return Material::PUSH_DESTROY;
}