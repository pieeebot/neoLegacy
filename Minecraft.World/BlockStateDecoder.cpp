// extra file dictating the data mappings in the f3 menu
// e.x. facing: north shows up instead of state: 2

#include "BlockStateDecoder.h"
#include "BlockStateDecoderRegistry.h"

#include "CocoaTile.h"
#include "DirectionalTile.h"
#include "Direction.h"
#include "FireTile.h"
#include "ButtonTile.h"
#include "CropTile.h"
#include "BedTile.h"
#include "FenceGateTile.h"
#include "FenceTile.h"
#include "DoorTile.h"
#include "FlowerPotTile.h"
#include "HalfSlabTile.h"
#include "HayBlockTile.h"
#include "HugeMushroomTile.h"
#include "HopperTile.h"
#include "BrewingStandTile.h"
#include "PistonBaseTile.h"
#include "PistonExtensionTile.h"
#include "LeverTile.h"
#include "TorchTile.h"
#include "FurnaceTile.h"
#include "RedStoneOreTile.h"
#include "NotGateTile.h"
#include "RedlightTile.h"
#include "JukeboxTile.h"
#include "CakeTile.h"
#include "DispenserTile.h"
#include "TntTile.h"
#include "BaseRailTile.h"
#include "NetherStalkTile.h"
#include "ReedTile.h"
#include "RepeaterTile.h"
#include "Sapling.h"
#include "StoneSlabTile.h"
#include "StoneSlabTile2.h"
#include "StairTile.h"
#include "StemTile.h"
#include "TreeTile.h"
#include "TreeTile2.h"
#include "TheEndPortalFrameTile.h"
#include "TrapDoorTile.h"
#include "TripWireTile.h"
#include "WoodSlabTile.h"
#include "TallGrass.h"
#include "TallGrass2.h"
#include "VineTile.h"
#include "Facing.h"

#include <sstream>

using namespace BlockStateDecoder;

DoorProps BlockStateDecoder::decodeDoor(int composite)
{
	DoorProps p;
	p.dir = composite & DoorTile::C_DIR_MASK;
	static const std::wstring dirNames[] = { L"south", L"west", L"north", L"east" };
	if (p.dir >= 0 && p.dir < 4) p.dirName = dirNames[p.dir]; else p.dirName = L"unknown";
	p.open = (composite & DoorTile::C_OPEN_MASK) != 0;
	p.upper = (composite & DoorTile::C_IS_UPPER_MASK) != 0;
	p.hingeRight = (composite & DoorTile::C_RIGHT_HINGE_MASK) != 0;
	return p;
}

std::wstring BlockStateDecoder::doorPropsToString(const DoorProps &p)
{
	std::wstringstream ss;
	ss << L"facing: " << p.dirName << L"\n";
	ss << L"open: " << (p.open ? L"true" : L"false") << L"\n";
	ss << L"half: " << (p.upper ? L"upper" : L"lower") << L"\n";
	ss << L"hinge: " << (p.hingeRight ? L"right" : L"left");
	return ss.str();
}

static std::wstring agePropsToString(int age)
{
	std::wstringstream ss;
	ss << L"age: " << age;
	return ss.str();
}

static std::wstring cocoaPropsToString(int composite)
{
	int dir = composite & 0x3;
	int age = (composite >> 2) & 0x3;
	static const std::wstring dirNames[] = { L"south", L"west", L"north", L"east" };
	std::wstring facing = (dir >= 0 && dir < 4) ? dirNames[dir] : L"unknown";
	std::wstringstream ss;
	ss << L"facing: " << facing << L"\n";
	ss << L"age: " << age;
	return ss.str();
}

static std::wstring stemPropsToString(int composite)
{
	int age = composite & 0x7;
	int facingCode = (composite >> 3) & 0x7;
	static const std::wstring facingNames[] = { L"none", L"west", L"east", L"north", L"south" };
	std::wstring facing = (facingCode >= 0 && facingCode < 5) ? facingNames[facingCode] : L"unknown";
	std::wstringstream ss;
	ss << L"age: " << age << L"\n";
	ss << L"facing: " << facing;
	return ss.str();
}

static std::wstring vinePropsToString(int composite)
{
	std::wstringstream ss;
	ss << L"north: " << (((composite & VineTile::VINE_NORTH) != 0) ? L"true" : L"false") << L"\n";
	ss << L"south: " << (((composite & VineTile::VINE_SOUTH) != 0) ? L"true" : L"false") << L"\n";
	ss << L"east: " << (((composite & VineTile::VINE_EAST) != 0) ? L"true" : L"false") << L"\n";
	ss << L"west: " << (((composite & VineTile::VINE_WEST) != 0) ? L"true" : L"false");
	return ss.str();
}

static std::wstring flowerPotTypeToString(int type)
{
	switch (type)
	{
	case FlowerPotTile::TYPE_FLOWER_RED: return L"red_flower";
	case FlowerPotTile::TYPE_FLOWER_YELLOW: return L"yellow_flower";
	case FlowerPotTile::TYPE_SAPLING_DEFAULT: return L"sapling_default";
	case FlowerPotTile::TYPE_SAPLING_EVERGREEN: return L"sapling_evergreen";
	case FlowerPotTile::TYPE_SAPLING_BIRCH: return L"sapling_birch";
	case FlowerPotTile::TYPE_SAPLING_JUNGLE: return L"sapling_jungle";
	case FlowerPotTile::TYPE_MUSHROOM_RED: return L"red_mushroom";
	case FlowerPotTile::TYPE_MUSHROOM_BROWN: return L"brown_mushroom";
	case FlowerPotTile::TYPE_CACTUS: return L"cactus";
	case FlowerPotTile::TYPE_DEAD_BUSH: return L"dead_bush";
	case FlowerPotTile::TYPE_FERN: return L"fern";
	default: return L"empty";
	}
}

static std::wstring flowerPotPropsToString(int composite)
{
	std::wstringstream ss;
	ss << L"type: " << flowerPotTypeToString(composite & 0xF);
	return ss.str();
}

static std::wstring saplingPropsToString(int composite)
{
	int type = composite & 0x7;
	bool grown = (composite & 0x8) != 0;
	static const std::wstring typeNames[] = { L"oak", L"spruce", L"birch", L"jungle", L"acacia", L"dark_oak" };
	std::wstring typeName = (type >= 0 && type < 6) ? typeNames[type] : L"unknown";
	std::wstringstream ss;
	ss << L"type: " << typeName << L"\n";
	ss << L"age: " << (grown ? 1 : 0);
	return ss.str();
}

static std::wstring tallGrassPropsToString(int composite)
{
	int type = composite & 0x3;
	static const std::wstring typeNames[] = { L"dead_shrub", L"tall_grass", L"fern" };
	std::wstring typeName = (type >= 0 && type < 3) ? typeNames[type] : L"unknown";
	std::wstringstream ss;
	ss << L"variant: " << typeName;
	return ss.str();
}

static std::wstring double_plantPropsToString(int composite)
{
	int type = composite & 0x7;
	bool upper = (composite & TallGrass2::UPPER_BIT) != 0;
	static const std::wstring typeNames[] = { L"sunflower", L"lilac", L"tall_grass", L"large_fern", L"rose_bush", L"peony" };
	std::wstring typeName = (type >= 0 && type < TallGrass2::VARIANT_COUNT) ? typeNames[type] : L"unknown";
	std::wstringstream ss;
	ss << L"variant: " << typeName << L"\n";
	ss << L"half: " << (upper ? L"upper" : L"lower");
	return ss.str();
}

static std::wstring brewingStandPropsToString(int composite)
{
	std::wstringstream ss;
	ss << L"bottle_0: " << (((composite & 0x1) != 0) ? L"true" : L"false") << L"\n";
	ss << L"bottle_1: " << (((composite & 0x2) != 0) ? L"true" : L"false") << L"\n";
	ss << L"bottle_2: " << (((composite & 0x4) != 0) ? L"true" : L"false");
	return ss.str();
}

static std::wstring jukeboxPropsToString(int composite)
{
	std::wstringstream ss;
	ss << L"has_record: " << (((composite & 0x1) != 0) ? L"true" : L"false");
	return ss.str();
}

static std::wstring daylightDetectorPropsToString(int composite, bool inverted)
{
	std::wstringstream ss;
	int power = composite & 0xF;
	ss << L"inverted: " << (inverted ? L"true" : L"false") << L"\n";
	ss << L"power: " << power << L"\n";
	ss << L"powered: " << (power > 0 ? L"true" : L"false");
	return ss.str();
}

static std::wstring snowPropsToString(int composite)
{
	std::wstringstream ss;
	int layers = composite & 0x7;
	if (layers == 0) layers = 8;
	ss << L"layers: " << layers;
	return ss.str();
}

static std::wstring cauldronPropsToString(int composite)
{
	std::wstringstream ss;
	int level = composite & 0x3;
	ss << L"level: " << level;
	return ss.str();
}

static std::wstring bedPropsToString(int composite)
{
	int dir = DirectionalTile::getDirection(composite);
	static const std::wstring dirNames[] = { L"south", L"west", L"north", L"east" };
	std::wstring facing = (dir >= 0 && dir < 4) ? dirNames[dir] : L"unknown";
	bool head = (composite & BedTile::HEAD_PIECE_DATA) != 0;
	bool occupied = (composite & BedTile::OCCUPIED_DATA) != 0;
	std::wstringstream ss;
	ss << L"facing: " << facing << L"\n";
	ss << L"part: " << (head ? L"head" : L"foot") << L"\n";
	ss << L"occupied: " << (occupied ? L"true" : L"false");
	return ss.str();
}

static std::wstring railPropsToString(int composite, bool usesDataBit)
{
	int shape = composite & BaseRailTile::RAIL_DIRECTION_MASK;
	std::wstring shapeName = L"unknown";
	static const std::wstring shapeNames[] = {
		L"north_south", L"east_west", L"ascending_east", L"ascending_west",
		L"ascending_north", L"ascending_south", L"south_east", L"south_west",
		L"north_west", L"north_east"
	};
	if (shape >= 0 && shape < 10) shapeName = shapeNames[shape];
	std::wstringstream ss;
	ss << L"shape: " << shapeName;
	if (usesDataBit)
	{
		bool powered = (composite & BaseRailTile::RAIL_DATA_BIT) != 0;
		ss << L"\n";
		ss << L"powered: " << (powered ? L"true" : L"false");
	}
	return ss.str();
}

static std::wstring pressurePlatePropsToString(int composite)
{
	int power = composite & 0xF;
	std::wstringstream ss;
	ss << L"power: " << power << L"\n";
	ss << L"powered: " << (power > 0 ? L"true" : L"false");
	return ss.str();
}

static std::wstring facingToString(int facing)
{
	static const std::wstring facingNames[] = { L"down", L"up", L"north", L"south", L"west", L"east" };
	return (facing >= 0 && facing < 6) ? facingNames[facing] : L"unknown";
}


static std::wstring dispenserPropsToString(int composite)
{
	int facing = composite & DispenserTile::FACING_MASK;
	bool triggered = (composite & DispenserTile::TRIGGER_BIT) != 0;
	std::wstringstream ss;
	ss << L"facing: " << facingToString(facing) << L"\n";
	ss << L"triggered: " << (triggered ? L"true" : L"false");
	return ss.str();
}

static std::wstring tntPropsToString(int composite)
{
	bool explode = (composite & TntTile::EXPLODE_BIT) != 0;
	std::wstringstream ss;
	ss << L"explode: " << (explode ? L"true" : L"false");
	return ss.str();
}

static std::wstring cakePropsToString(int composite)
{
	int bites = composite & 0x7;
	std::wstringstream ss;
	ss << L"bites: " << bites;
	return ss.str();
}

static std::wstring comparatorPropsToString(int composite)
{
	int dir = DirectionalTile::getDirection(composite);
	static const std::wstring dirNames[] = { L"south", L"west", L"north", L"east" };
	std::wstring facing = (dir >= 0 && dir < 4) ? dirNames[dir] : L"unknown";
	bool subtract = (composite & 0x4) != 0;
	bool powered = (composite & 0x8) != 0;
	std::wstringstream ss;
	ss << L"facing: " << facing << L"\n";
	ss << L"mode: " << (subtract ? L"subtract" : L"compare") << L"\n";
	ss << L"powered: " << (powered ? L"true" : L"false");
	return ss.str();
}

static std::wstring farmPropsToString(int composite)
{
	std::wstringstream ss;
	ss << L"moisture: " << (composite & 0x7);
	return ss.str();
}

static std::wstring redstoneDustPropsToString(int composite)
{
	std::wstringstream ss;
	int power = composite & 0xF;
	ss << L"power: " << power << L"\n";
	ss << L"powered: " << (power > 0 ? L"true" : L"false");
	return ss.str();
}

static std::wstring firePropsToString(int composite) {
	std::wstringstream ss;
	ss << L"age: " << (composite & FireTile::AGE_MASK);
	return ss.str();
}

static std::wstring torchPropsToString(int composite)
{
int dir = composite & 0x7;
static const std::wstring dirNames[] = { L"up", L"west", L"east", L"south", L"north", L"unknown" };
std::wstring dirName = (dir >= 0 && dir < 6) ? dirNames[dir > 4 ? 5 : dir] : L"unknown";
std::wstringstream ss;
ss << L"facing: " << dirName;
return ss.str();
}

static std::wstring furnacePropsToString(int composite)
{
int facing = composite & 0x7;
static const std::wstring facingNames[] = { L"unknown", L"unknown", L"north", L"south", L"west", L"east" };
std::wstring facingName = (facing >= 2 && facing <= 5) ? facingNames[facing] : L"unknown";
std::wstringstream ss;
ss << L"facing: " << facingName;
return ss.str();
}

static std::wstring redstoneOrePropsToString(int composite)
{
std::wstringstream ss;
ss << L"lit: " << (((composite & 0x1) != 0) ? L"true" : L"false");
return ss.str();
}

static std::wstring redstoneTorchPropsToString(int composite)
{
int dir = composite & 0x7;
static const std::wstring dirNames[] = { L"up", L"west", L"east", L"south", L"north", L"unknown" };
std::wstring dirName = (dir >= 0 && dir < 6) ? dirNames[dir > 4 ? 5 : dir] : L"unknown";
std::wstringstream ss;
ss << L"facing: " << dirName;
return ss.str();
}

static std::wstring redlightPropsToString(int composite)
{
	std::wstringstream ss;
	ss << L"lit: " << (((composite & 0x1) != 0) ? L"true" : L"false");
	return ss.str();
}

static std::wstring buttonFacingToString(int data)
{
	switch (data & 0x7)
	{
	case 1: return L"east";
	case 2: return L"west";
	case 3: return L"south";
	case 4: return L"north";
	case 5: return L"ceiling";
	case 6: return L"floor";
	default: return L"unknown";
	}
}

static std::wstring buttonPropsToString(int composite)
{
	std::wstringstream ss;
	ss << L"facing: " << buttonFacingToString(composite) << L"\n";
	ss << L"powered: " << (((composite & 0x8) != 0) ? L"true" : L"false");
	return ss.str();
}

static std::wstring leverFacingToString(int data)
{
	static const std::wstring names[] = {
		L"down_south", L"down_east", L"down_west", L"down_north",
		L"up_south", L"up_north", L"ceiling_west", L"ceiling_east"
	};
	int facing = data & 7;
	return (facing >= 0 && facing < 8) ? names[facing] : L"unknown";
}

static std::wstring leverPropsToString(int composite)
{
	std::wstringstream ss;
	ss << L"facing: " << leverFacingToString(composite) << L"\n";
	ss << L"powered: " << (((composite & 0x8) != 0) ? L"true" : L"false");
	return ss.str();
}

static std::wstring fenceGatePropsToString(int composite)
{
	int dir = DirectionalTile::getDirection(composite);
	static const std::wstring dirNames[] = { L"south", L"west", L"north", L"east" };
	std::wstring facing = (dir >= 0 && dir < 4) ? dirNames[dir] : L"unknown";
	bool powered = (composite & 0x8) != 0;
	bool inWall = (composite & 0x10) != 0;
	std::wstringstream ss;
	ss << L"facing: " << facing << L"\n";
	ss << L"open: " << (FenceGateTile::isOpen(composite) ? L"true" : L"false") << L"\n";
	ss << L"powered: " << (powered ? L"true" : L"false") << L"\n";
	ss << L"in_wall: " << (inWall ? L"true" : L"false");
	return ss.str();
}

static std::wstring slabTypeToString(int tileId, int type)
{
	if (tileId == Tile::double_wooden_slab_Id || tileId == Tile::wooden_slab_Id)
	{
		static const std::wstring typeNames[] = { L"oak", L"spruce", L"birch", L"jungle", L"acacia", L"dark_oak" };
		return (type >= 0 && type < 6) ? typeNames[type] : L"unknown";
	}

	if (tileId == Tile::stone_slab2_Id || tileId == Tile::double_stone_slab2_Id)
	{
		return (type == StoneSlabTile2::RED_SANDSTONE_SLAB) ? L"red_sandstone" : L"unknown";
	}

	static const std::wstring typeNames[] = {
		L"stone", L"sandstone", L"wood", L"cobblestone",
		L"brick", L"stone_brick", L"nether_brick", L"quartz"
	};
	return (type >= 0 && type < 8) ? typeNames[type] : L"unknown";
}

static std::wstring slabPropsToString(int tileId, int composite)
{
	int type = composite & HalfSlabTile::TYPE_MASK;
	bool top = (composite & HalfSlabTile::TOP_SLOT_BIT) != 0;
	std::wstringstream ss;
	ss << L"type: " << slabTypeToString(tileId, type);
	if (tileId == Tile::wooden_slab_Id || tileId == Tile::stone_slab_Id || tileId == Tile::stone_slab2_Id)
	{
		ss << L"\n";
		ss << L"half: " << (top ? L"top" : L"bottom");
	}
	else
	{
		ss << L"\n";
		ss << L"half: double";
	}
	return ss.str();
}

static std::wstring trapDoorPropsToString(int composite)
{
	int dir = composite & 0x3;
	static const std::wstring dirNames[] = { L"north", L"south", L"west", L"east" };
	std::wstring facing = (dir >= 0 && dir < 4) ? dirNames[dir] : L"unknown";
	bool open = (composite & 0x4) != 0;
	bool top = (composite & 0x8) != 0;

	std::wstringstream ss;
	ss << L"facing: " << facing << L"\n";
	ss << L"open: " << (open ? L"true" : L"false") << L"\n";
	ss << L"half: " << (top ? L"top" : L"bottom");
	return ss.str();
}

static std::wstring fencePropsToString(int composite)
{
	std::wstringstream ss;
	ss << L"north: " << (((composite & 0x1) != 0) ? L"true" : L"false") << L"\n";
	ss << L"south: " << (((composite & 0x2) != 0) ? L"true" : L"false") << L"\n";
	ss << L"east: " << (((composite & 0x4) != 0) ? L"true" : L"false") << L"\n";
	ss << L"west: " << (((composite & 0x8) != 0) ? L"true" : L"false");
	return ss.str();
}

static std::wstring axisToString(int composite)
{
	switch (composite & RotatedPillarTile::MASK_FACING)
	{
	case RotatedPillarTile::FACING_X: return L"x";
	case RotatedPillarTile::FACING_Z: return L"z";
	default: return L"y";
	}
}

static std::wstring hayBlockPropsToString(int composite)
{
	std::wstringstream ss;
	ss << L"axis: " << axisToString(composite);
	return ss.str();
}

static std::wstring pistonBasePropsToString(int composite)
{
	int facing = PistonBaseTile::getFacing(composite);
	std::wstringstream ss;
	ss << L"facing: " << facingToString(facing) << L"\n";
	ss << L"extended: " << (PistonBaseTile::isExtended(composite) ? L"true" : L"false");
	return ss.str();
}

static std::wstring pistonExtensionPropsToString(int composite)
{
	int facing = PistonExtensionTile::getFacing(composite);
	std::wstringstream ss;
	ss << L"facing: " << facingToString(facing) << L"\n";
	ss << L"type: " << (((composite & PistonExtensionTile::STICKY_BIT) != 0) ? L"sticky" : L"normal");
	return ss.str();
}

static std::wstring endPortalFramePropsToString(int composite)
{
	static const std::wstring dirNames[] = { L"south", L"west", L"north", L"east" };
	int facing = composite & 0x3;
	std::wstring facingName = (facing >= 0 && facing < 4) ? dirNames[facing] : L"unknown";
	std::wstringstream ss;
	ss << L"facing: " << facingName << L"\n";
	ss << L"eye: " << ((composite & TheEndPortalFrameTile::EYE_BIT) != 0 ? L"true" : L"false");
	return ss.str();
}

static std::wstring repeaterPropsToString(int composite, bool powered)
{
	int dir = DirectionalTile::getDirection(composite);
	static const std::wstring dirNames[] = { L"south", L"west", L"north", L"east" };
	std::wstring facing = (dir >= 0 && dir < 4) ? dirNames[dir] : L"unknown";
	int delay = ((composite & RepeaterTile::DELAY_MASK) >> RepeaterTile::DELAY_SHIFT) + 1;

	std::wstringstream ss;
	ss << L"facing: " << facing << L"\n";
	ss << L"delay: " << delay << L"\n";
	ss << L"powered: " << (powered ? L"true" : L"false");
	return ss.str();
}

static std::wstring hugeMushroomPropsToString(int composite)
{
	std::wstringstream ss;
	ss << L"variant: " << (composite & 0xF);
	return ss.str();
}

static std::wstring hopperPropsToString(int composite)
{
	int face = HopperTile::getAttachedFace(composite);
	static const std::wstring faceNames[] = { L"down", L"up", L"north", L"south", L"west", L"east" };
	std::wstring facing = (face >= 0 && face < 6) ? faceNames[face] : L"unknown";
	bool enabled = HopperTile::isTurnedOn(composite);

	std::wstringstream ss;
	ss << L"facing: " << facing << L"\n";
	ss << L"enabled: " << (enabled ? L"true" : L"false");
	return ss.str();
}

static std::wstring logBlockPropsToString(int tileId, int composite)
{
	int type = composite & RotatedPillarTile::MASK_TYPE;
	std::wstring axis = axisToString(composite);
	std::wstring typeName;

	if (tileId == Tile::log_Id)
	{
		static const std::wstring typeNames[] = { L"oak", L"spruce", L"birch", L"jungle" };
		typeName = (type >= 0 && type < 4) ? typeNames[type] : L"unknown";
	}
	else
	{
		static const std::wstring typeNames[] = { L"acacia", L"dark_oak" };
		typeName = (type >= 0 && type < 2) ? typeNames[type] : L"unknown";
	}

	std::wstringstream ss;
	ss << L"type: " << typeName << L"\n";
	ss << L"axis: " << axis;
	return ss.str();
}

static std::wstring tripWirePropsToString(int composite)
{
	std::wstringstream ss;
	ss << L"north: " << (((composite & 0x1) != 0) ? L"true" : L"false") << L"\n";
	ss << L"south: " << (((composite & 0x2) != 0) ? L"true" : L"false") << L"\n";
	ss << L"east: " << (((composite & 0x4) != 0) ? L"true" : L"false") << L"\n";
	ss << L"west: " << (((composite & 0x8) != 0) ? L"true" : L"false") << L"\n";
	ss << L"powered: " << (((composite & TripWireTile::BLOCKSTATE_POWERED_BIT) != 0) ? L"true" : L"false");
	return ss.str();
}

static std::wstring tripWireSourcePropsToString(int composite)
{
	int dir = composite & 0x3;
	static const std::wstring dirNames[] = { L"north", L"east", L"south", L"west" };
	std::wstring facing = (dir >= 0 && dir < 4) ? dirNames[dir] : L"unknown";
	bool attached = (composite & 0x4) != 0;
	bool powered = (composite & 0x8) != 0;

	std::wstringstream ss;
	ss << L"facing: " << facing << L"\n";
	ss << L"attached: " << (attached ? L"true" : L"false") << L"\n";
	ss << L"powered: " << (powered ? L"true" : L"false");
	return ss.str();
}

static bool registerDoorDecoder()
{
	using namespace BlockStateDecoderRegistry;
	DecoderFn fn = [](int composite)->std::wstring {
		return BlockStateDecoder::doorPropsToString(BlockStateDecoder::decodeDoor(composite));
	};
	registerDecoder(Tile::wooden_door_Id, fn);
	registerDecoder(Tile::iron_door_Id, fn);
	registerDecoder(Tile::spruce_door_Id, fn);
	registerDecoder(Tile::birch_door_Id, fn);
	registerDecoder(Tile::jungle_door_Id, fn);
	registerDecoder(Tile::acacia_door_Id, fn);
	registerDecoder(Tile::dark_oak_door_Id, fn);
	return true;
}

static bool s_doorDecoderRegistered = registerDoorDecoder();

static bool registerStairDecoder()
{
	using namespace BlockStateDecoderRegistry;
	DecoderFn fn = [](int composite)->std::wstring {
		int dir = composite & 0x3;
		static const std::wstring dirNames[] = { L"east", L"west", L"south", L"north" };
		std::wstring facing = (dir >= 0 && dir < 4) ? dirNames[dir] : L"unknown";
		bool upside = (composite & StairTile::UPSIDEDOWN_BIT) != 0;
		int shape = (composite >> 3) & 0x7;
		std::wstring shapeName = L"straight";
		if (shape == 1) shapeName = L"inner";
		else if (shape == 2) shapeName = L"outer";

		std::wstringstream ss;
		ss << L"facing: " << facing << L"\n";
		ss << L"half: " << (upside ? L"top" : L"bottom") << L"\n";
		ss << L"shape: " << shapeName;
		return ss.str();
	};

	registerDecoder(Tile::oak_stairs_Id, fn);
	registerDecoder(Tile::stone_stairs_Id, fn);
	registerDecoder(Tile::brick_stairs_Id, fn);
	registerDecoder(Tile::stone_brick_stairs_Id, fn);
	registerDecoder(Tile::nether_brick_stairs_Id, fn);
	registerDecoder(Tile::sandstone_stairs_Id, fn);
	registerDecoder(Tile::spruce_stairs_Id, fn);
	registerDecoder(Tile::birch_stairs_Id, fn);
	registerDecoder(Tile::jungle_stairs_Id, fn);
	registerDecoder(Tile::quartz_stairs_Id, fn);
	registerDecoder(Tile::acacia_stairs_Id, fn);
	registerDecoder(Tile::dark_oak_stairs_Id, fn);
	registerDecoder(Tile::red_sandstone_stairs_Id, fn);
	return true;
}

static bool s_stairDecoderRegistered = registerStairDecoder();

static bool registerPlantDecoders()
{
	using namespace BlockStateDecoderRegistry;
	DecoderFn ageDecoder = [](int composite)->std::wstring {
		return agePropsToString(composite);
	};

	registerDecoder(Tile::wheat_Id, ageDecoder);
	registerDecoder(Tile::carrots_Id, ageDecoder);
	registerDecoder(Tile::potatoes_Id, ageDecoder);
	registerDecoder(Tile::cactus_Id, ageDecoder);
	registerDecoder(Tile::nether_wart_Id, ageDecoder);
	registerDecoder(Tile::reeds_Id, ageDecoder);
	registerDecoder(Tile::bed_Id, [](int composite)->std::wstring { return bedPropsToString(composite); });
	registerDecoder(Tile::rail_Id, [](int composite)->std::wstring { return railPropsToString(composite, false); });
	registerDecoder(Tile::golden_rail_Id, [](int composite)->std::wstring { return railPropsToString(composite, true); });
	registerDecoder(Tile::detector_rail_Id, [](int composite)->std::wstring { return railPropsToString(composite, true); });
	registerDecoder(Tile::activator_rail_Id, [](int composite)->std::wstring { return railPropsToString(composite, true); });
	registerDecoder(Tile::dispenser_Id, [](int composite)->std::wstring { return dispenserPropsToString(composite); });
	registerDecoder(Tile::dropper_Id, [](int composite)->std::wstring { return dispenserPropsToString(composite); });
	registerDecoder(Tile::tnt_Id, [](int composite)->std::wstring { return tntPropsToString(composite); });
	registerDecoder(Tile::cake_Id, [](int composite)->std::wstring { return cakePropsToString(composite); });
	registerDecoder(Tile::stone_pressure_plate_Id, [](int composite)->std::wstring { return pressurePlatePropsToString(composite); });
	registerDecoder(Tile::wooden_pressure_plate_Id, [](int composite)->std::wstring { return pressurePlatePropsToString(composite); });
	registerDecoder(Tile::light_weighted_pressure_plate_Id, [](int composite)->std::wstring { return pressurePlatePropsToString(composite); });
	registerDecoder(Tile::heavy_weighted_pressure_plate_Id, [](int composite)->std::wstring { return pressurePlatePropsToString(composite); });
	registerDecoder(Tile::farmland_Id, [](int composite)->std::wstring { return farmPropsToString(composite); });
	registerDecoder(Tile::cocoa_Id, [](int composite)->std::wstring { return cocoaPropsToString(composite); });
	registerDecoder(Tile::brewing_stand_Id, [](int composite)->std::wstring { return brewingStandPropsToString(composite); });
	registerDecoder(Tile::fire_Id, [](int composite)->std::wstring { return firePropsToString(composite); });
	registerDecoder(Tile::stone_button_Id, [](int composite)->std::wstring { return buttonPropsToString(composite); });
	registerDecoder(Tile::wooden_button_Id, [](int composite)->std::wstring { return buttonPropsToString(composite); });
	registerDecoder(Tile::pumpkin_stem_Id, [](int composite)->std::wstring { return stemPropsToString(composite); });
	registerDecoder(Tile::melon_stem_Id, [](int composite)->std::wstring { return stemPropsToString(composite); });
	registerDecoder(Tile::vine_Id, [](int composite)->std::wstring { return vinePropsToString(composite); });
	registerDecoder(Tile::flower_pot_Id, [](int composite)->std::wstring { return flowerPotPropsToString(composite); });
	registerDecoder(Tile::sapling_Id, [](int composite)->std::wstring { return saplingPropsToString(composite); });
	registerDecoder(Tile::tallgrass_Id, [](int composite)->std::wstring { return tallGrassPropsToString(composite); });
	registerDecoder(Tile::double_plant_Id, [](int composite)->std::wstring { return double_plantPropsToString(composite); });
	registerDecoder(Tile::fence_Id, [](int composite)->std::wstring { return fencePropsToString(composite); });
	registerDecoder(Tile::nether_brick_fence_Id, [](int composite)->std::wstring { return fencePropsToString(composite); });
	registerDecoder(Tile::spruce_fence_Id, [](int composite)->std::wstring { return fencePropsToString(composite); });
	registerDecoder(Tile::birch_fence_Id, [](int composite)->std::wstring { return fencePropsToString(composite); });
	registerDecoder(Tile::jungle_fence_Id, [](int composite)->std::wstring { return fencePropsToString(composite); });
	registerDecoder(Tile::dark_oak_fence_Id, [](int composite)->std::wstring { return fencePropsToString(composite); });
	registerDecoder(Tile::acacia_fence_Id, [](int composite)->std::wstring { return fencePropsToString(composite); });
	registerDecoder(Tile::double_stone_slab_Id, [](int composite)->std::wstring { return slabPropsToString(Tile::double_stone_slab_Id, composite); });
	registerDecoder(Tile::stone_slab_Id, [](int composite)->std::wstring { return slabPropsToString(Tile::stone_slab_Id, composite); });
	registerDecoder(Tile::double_wooden_slab_Id, [](int composite)->std::wstring { return slabPropsToString(Tile::double_wooden_slab_Id, composite); });
	registerDecoder(Tile::wooden_slab_Id, [](int composite)->std::wstring { return slabPropsToString(Tile::wooden_slab_Id, composite); });
	registerDecoder(Tile::double_stone_slab2_Id, [](int composite)->std::wstring { return slabPropsToString(Tile::double_stone_slab2_Id, composite); });
	registerDecoder(Tile::stone_slab2_Id, [](int composite)->std::wstring { return slabPropsToString(Tile::stone_slab2_Id, composite); });
	registerDecoder(Tile::trapdoor_Id, [](int composite)->std::wstring { return trapDoorPropsToString(composite); });
	registerDecoder(Tile::iron_trapdoor_Id, [](int composite)->std::wstring { return trapDoorPropsToString(composite); });
	registerDecoder(Tile::tripwire_Id, [](int composite)->std::wstring { return tripWirePropsToString(composite); });
	registerDecoder(Tile::tripwire_hook_Id, [](int composite)->std::wstring { return tripWireSourcePropsToString(composite); });
	registerDecoder(Tile::hay_block_Id, [](int composite)->std::wstring { return hayBlockPropsToString(composite); });
	registerDecoder(Tile::log_Id, [](int composite)->std::wstring { return logBlockPropsToString(Tile::log_Id, composite); });
	registerDecoder(Tile::log2_Id, [](int composite)->std::wstring { return logBlockPropsToString(Tile::log2_Id, composite); });
	registerDecoder(Tile::lever_Id, [](int composite)->std::wstring { return leverPropsToString(composite); });
	registerDecoder(Tile::piston_Id, [](int composite)->std::wstring { return pistonBasePropsToString(composite); });
	registerDecoder(Tile::sticky_piston_Id, [](int composite)->std::wstring { return pistonBasePropsToString(composite); });
	registerDecoder(Tile::piston_head_Id, [](int composite)->std::wstring { return pistonExtensionPropsToString(composite); });
	registerDecoder(Tile::end_portal_frame_Id, [](int composite)->std::wstring { return endPortalFramePropsToString(composite); });
	registerDecoder(Tile::unpowered_repeater_Id, [](int composite)->std::wstring { return repeaterPropsToString(composite, false); });
	registerDecoder(Tile::powered_repeater_Id, [](int composite)->std::wstring { return repeaterPropsToString(composite, true); });
	registerDecoder(Tile::unpowered_comparator_Id, [](int composite)->std::wstring { return comparatorPropsToString(composite); });
	registerDecoder(Tile::powered_comparator_Id, [](int composite)->std::wstring { return comparatorPropsToString(composite); });
	registerDecoder(Tile::redstone_wire_Id, [](int composite)->std::wstring { return redstoneDustPropsToString(composite); });
	registerDecoder(Tile::brown_mushroom_block_Id, [](int composite)->std::wstring { return hugeMushroomPropsToString(composite); });
	registerDecoder(Tile::red_mushroom_block_Id, [](int composite)->std::wstring { return hugeMushroomPropsToString(composite); });
	registerDecoder(Tile::hopper_Id, [](int composite)->std::wstring { return hopperPropsToString(composite); });
	registerDecoder(Tile::jukebox_Id, [](int composite)->std::wstring { return jukeboxPropsToString(composite); });
	registerDecoder(Tile::fence_gate_Id, [](int composite)->std::wstring { return fenceGatePropsToString(composite); });
	registerDecoder(Tile::spruce_fence_gate_Id, [](int composite)->std::wstring { return fenceGatePropsToString(composite); });
	registerDecoder(Tile::birch_fence_gate_Id, [](int composite)->std::wstring { return fenceGatePropsToString(composite); });
	registerDecoder(Tile::jungle_fence_gate_Id, [](int composite)->std::wstring { return fenceGatePropsToString(composite); });
	registerDecoder(Tile::dark_oak_fence_gate_Id, [](int composite)->std::wstring { return fenceGatePropsToString(composite); });
	registerDecoder(Tile::acacia_fence_gate_Id, [](int composite)->std::wstring { return fenceGatePropsToString(composite); });
	registerDecoder(Tile::daylight_detector_Id, [](int composite)->std::wstring { return daylightDetectorPropsToString(composite, false); });
	registerDecoder(Tile::daylight_detector_inverted_Id, [](int composite)->std::wstring { return daylightDetectorPropsToString(composite, true); });
	registerDecoder(Tile::snow_Id, [](int composite)->std::wstring { return snowPropsToString(composite); });
	registerDecoder(Tile::snow_layer_Id, [](int composite)->std::wstring { return snowPropsToString(composite); });
	registerDecoder(Tile::cauldron_Id, [](int composite)->std::wstring { return cauldronPropsToString(composite); });
	registerDecoder(Tile::torch_Id, [](int composite)->std::wstring { return torchPropsToString(composite); });
	registerDecoder(Tile::furnace_Id, [](int composite)->std::wstring { return furnacePropsToString(composite); });
	registerDecoder(Tile::lit_furnace_Id, [](int composite)->std::wstring { return furnacePropsToString(composite); });
	registerDecoder(Tile::redstone_ore_Id, [](int composite)->std::wstring { return redstoneOrePropsToString(composite); });
	registerDecoder(Tile::lit_redstone_ore_Id, [](int composite)->std::wstring { return redstoneOrePropsToString(composite); });
	registerDecoder(Tile::unlit_redstone_torch_Id, [](int composite)->std::wstring { return redstoneTorchPropsToString(composite); });
	registerDecoder(Tile::redstone_torch_Id, [](int composite)->std::wstring { return redstoneTorchPropsToString(composite); });
	registerDecoder(Tile::redstone_lamp_Id, [](int composite)->std::wstring { return redlightPropsToString(composite); });
	registerDecoder(Tile::lit_redstone_lamp_Id, [](int composite)->std::wstring { return redlightPropsToString(composite); });
	return true;
}

static bool s_plantDecoderRegistered = registerPlantDecoders();
