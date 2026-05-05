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

static std::wstring tallGrass2PropsToString(int composite)
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
	if (tileId == Tile::woodSlab_Id || tileId == Tile::woodSlabHalf_Id)
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
	if (tileId == Tile::woodSlabHalf_Id || tileId == Tile::stoneSlabHalf_Id || tileId == Tile::stone_slab2_Id)
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

static std::wstring facingToString(int facing)
{
	static const std::wstring facingNames[] = { L"down", L"up", L"north", L"south", L"west", L"east" };
	return (facing >= 0 && facing < 6) ? facingNames[facing] : L"unknown";
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

	if (tileId == Tile::treeTrunk_Id)
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
	registerDecoder(Tile::door_wood_Id, fn);
	registerDecoder(Tile::door_iron_Id, fn);
	registerDecoder(Tile::spruce_door_Id, fn);
	registerDecoder(Tile::birch_door_Id, fn);
	registerDecoder(Tile::jungle_door_Id, fn);
	registerDecoder(Tile::acacia_door_Id, fn);
	registerDecoder(Tile::dark_oak_door_Id, fn);
	return true;
registerDecoder(Tile::torch_Id, [](int composite)->std::wstring { return torchPropsToString(composite); });
registerDecoder(Tile::furnace_Id, [](int composite)->std::wstring { return furnacePropsToString(composite); });
registerDecoder(Tile::furnace_lit_Id, [](int composite)->std::wstring { return furnacePropsToString(composite); });
registerDecoder(Tile::redStoneOre_Id, [](int composite)->std::wstring { return redstoneOrePropsToString(composite); });
registerDecoder(Tile::redStoneOre_lit_Id, [](int composite)->std::wstring { return redstoneOrePropsToString(composite); });
registerDecoder(Tile::redstoneTorch_off_Id, [](int composite)->std::wstring { return redstoneTorchPropsToString(composite); });
registerDecoder(Tile::redstoneTorch_on_Id, [](int composite)->std::wstring { return redstoneTorchPropsToString(composite); });
registerDecoder(Tile::redstoneLight_Id, [](int composite)->std::wstring { return redlightPropsToString(composite); });
registerDecoder(Tile::redstoneLight_lit_Id, [](int composite)->std::wstring { return redlightPropsToString(composite); });
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

	registerDecoder(Tile::stairs_wood_Id, fn);
	registerDecoder(Tile::stairs_stone_Id, fn);
	registerDecoder(Tile::stairs_bricks_Id, fn);
	registerDecoder(Tile::stairs_stoneBrick_Id, fn);
	registerDecoder(Tile::stairs_netherBricks_Id, fn);
	registerDecoder(Tile::stairs_sandstone_Id, fn);
	registerDecoder(Tile::stairs_sprucewood_Id, fn);
	registerDecoder(Tile::stairs_birchwood_Id, fn);
	registerDecoder(Tile::stairs_junglewood_Id, fn);
	registerDecoder(Tile::stairs_quartz_Id, fn);
	registerDecoder(Tile::stairs_acaciawood_Id, fn);
	registerDecoder(Tile::stairs_darkwood_Id, fn);
	registerDecoder(Tile::stairs_red_sandstone_Id, fn);
	return true;
registerDecoder(Tile::torch_Id, [](int composite)->std::wstring { return torchPropsToString(composite); });
registerDecoder(Tile::furnace_Id, [](int composite)->std::wstring { return furnacePropsToString(composite); });
registerDecoder(Tile::furnace_lit_Id, [](int composite)->std::wstring { return furnacePropsToString(composite); });
registerDecoder(Tile::redStoneOre_Id, [](int composite)->std::wstring { return redstoneOrePropsToString(composite); });
registerDecoder(Tile::redStoneOre_lit_Id, [](int composite)->std::wstring { return redstoneOrePropsToString(composite); });
registerDecoder(Tile::redstoneTorch_off_Id, [](int composite)->std::wstring { return redstoneTorchPropsToString(composite); });
registerDecoder(Tile::redstoneTorch_on_Id, [](int composite)->std::wstring { return redstoneTorchPropsToString(composite); });
registerDecoder(Tile::redstoneLight_Id, [](int composite)->std::wstring { return redlightPropsToString(composite); });
registerDecoder(Tile::redstoneLight_lit_Id, [](int composite)->std::wstring { return redlightPropsToString(composite); });
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
	registerDecoder(Tile::netherStalk_Id, ageDecoder);
	registerDecoder(Tile::reeds_Id, ageDecoder);
	registerDecoder(Tile::bed_Id, [](int composite)->std::wstring { return bedPropsToString(composite); });
	registerDecoder(Tile::rail_Id, [](int composite)->std::wstring { return railPropsToString(composite, false); });
	registerDecoder(Tile::goldenRail_Id, [](int composite)->std::wstring { return railPropsToString(composite, true); });
	registerDecoder(Tile::detectorRail_Id, [](int composite)->std::wstring { return railPropsToString(composite, true); });
	registerDecoder(Tile::activatorRail_Id, [](int composite)->std::wstring { return railPropsToString(composite, true); });
	registerDecoder(Tile::dispenser_Id, [](int composite)->std::wstring { return dispenserPropsToString(composite); });
	registerDecoder(Tile::dropper_Id, [](int composite)->std::wstring { return dispenserPropsToString(composite); });
	registerDecoder(Tile::tnt_Id, [](int composite)->std::wstring { return tntPropsToString(composite); });
	registerDecoder(Tile::cake_Id, [](int composite)->std::wstring { return cakePropsToString(composite); });
	registerDecoder(Tile::pressurePlate_stone_Id, [](int composite)->std::wstring { return pressurePlatePropsToString(composite); });
	registerDecoder(Tile::pressurePlate_wood_Id, [](int composite)->std::wstring { return pressurePlatePropsToString(composite); });
	registerDecoder(Tile::weightedPlate_light_Id, [](int composite)->std::wstring { return pressurePlatePropsToString(composite); });
	registerDecoder(Tile::weightedPlate_heavy_Id, [](int composite)->std::wstring { return pressurePlatePropsToString(composite); });
	registerDecoder(Tile::farmland_Id, [](int composite)->std::wstring { return farmPropsToString(composite); });
	registerDecoder(Tile::cocoa_Id, [](int composite)->std::wstring { return cocoaPropsToString(composite); });
	registerDecoder(Tile::brewingStand_Id, [](int composite)->std::wstring { return brewingStandPropsToString(composite); });
	registerDecoder(Tile::fire_Id, [](int composite)->std::wstring { return firePropsToString(composite); });
	registerDecoder(Tile::button_stone_Id, [](int composite)->std::wstring { return buttonPropsToString(composite); });
	registerDecoder(Tile::button_wood_Id, [](int composite)->std::wstring { return buttonPropsToString(composite); });
	registerDecoder(Tile::pumpkinStem_Id, [](int composite)->std::wstring { return stemPropsToString(composite); });
	registerDecoder(Tile::melonStem_Id, [](int composite)->std::wstring { return stemPropsToString(composite); });
	registerDecoder(Tile::vine_Id, [](int composite)->std::wstring { return vinePropsToString(composite); });
	registerDecoder(Tile::flowerPot_Id, [](int composite)->std::wstring { return flowerPotPropsToString(composite); });
	registerDecoder(Tile::sapling_Id, [](int composite)->std::wstring { return saplingPropsToString(composite); });
	registerDecoder(Tile::tallgrass_Id, [](int composite)->std::wstring { return tallGrassPropsToString(composite); });
	registerDecoder(Tile::tallgrass2_Id, [](int composite)->std::wstring { return tallGrass2PropsToString(composite); });
	registerDecoder(Tile::fence_Id, [](int composite)->std::wstring { return fencePropsToString(composite); });
	registerDecoder(Tile::netherFence_Id, [](int composite)->std::wstring { return fencePropsToString(composite); });
	registerDecoder(Tile::spruceFence_Id, [](int composite)->std::wstring { return fencePropsToString(composite); });
	registerDecoder(Tile::birchFence_Id, [](int composite)->std::wstring { return fencePropsToString(composite); });
	registerDecoder(Tile::jungleFence_Id, [](int composite)->std::wstring { return fencePropsToString(composite); });
	registerDecoder(Tile::darkFence_Id, [](int composite)->std::wstring { return fencePropsToString(composite); });
	registerDecoder(Tile::acaciaFence_Id, [](int composite)->std::wstring { return fencePropsToString(composite); });
	registerDecoder(Tile::stoneSlab_Id, [](int composite)->std::wstring { return slabPropsToString(Tile::stoneSlab_Id, composite); });
	registerDecoder(Tile::stoneSlabHalf_Id, [](int composite)->std::wstring { return slabPropsToString(Tile::stoneSlabHalf_Id, composite); });
	registerDecoder(Tile::woodSlab_Id, [](int composite)->std::wstring { return slabPropsToString(Tile::woodSlab_Id, composite); });
	registerDecoder(Tile::woodSlabHalf_Id, [](int composite)->std::wstring { return slabPropsToString(Tile::woodSlabHalf_Id, composite); });
	registerDecoder(Tile::double_stone_slab2_Id, [](int composite)->std::wstring { return slabPropsToString(Tile::double_stone_slab2_Id, composite); });
	registerDecoder(Tile::stone_slab2_Id, [](int composite)->std::wstring { return slabPropsToString(Tile::stone_slab2_Id, composite); });
	registerDecoder(Tile::trapdoor_Id, [](int composite)->std::wstring { return trapDoorPropsToString(composite); });
	registerDecoder(Tile::iron_trapdoor_Id, [](int composite)->std::wstring { return trapDoorPropsToString(composite); });
	registerDecoder(Tile::tripWire_Id, [](int composite)->std::wstring { return tripWirePropsToString(composite); });
	registerDecoder(Tile::tripWireSource_Id, [](int composite)->std::wstring { return tripWireSourcePropsToString(composite); });
	registerDecoder(Tile::hayBlock_Id, [](int composite)->std::wstring { return hayBlockPropsToString(composite); });
	registerDecoder(Tile::treeTrunk_Id, [](int composite)->std::wstring { return logBlockPropsToString(Tile::treeTrunk_Id, composite); });
	registerDecoder(Tile::tree2Trunk_Id, [](int composite)->std::wstring { return logBlockPropsToString(Tile::tree2Trunk_Id, composite); });
	registerDecoder(Tile::lever_Id, [](int composite)->std::wstring { return leverPropsToString(composite); });
	registerDecoder(Tile::pistonBase_Id, [](int composite)->std::wstring { return pistonBasePropsToString(composite); });
	registerDecoder(Tile::pistonStickyBase_Id, [](int composite)->std::wstring { return pistonBasePropsToString(composite); });
	registerDecoder(Tile::pistonExtensionPiece_Id, [](int composite)->std::wstring { return pistonExtensionPropsToString(composite); });
	registerDecoder(Tile::endPortalFrameTile_Id, [](int composite)->std::wstring { return endPortalFramePropsToString(composite); });
	registerDecoder(Tile::diode_off_Id, [](int composite)->std::wstring { return repeaterPropsToString(composite, false); });
	registerDecoder(Tile::diode_on_Id, [](int composite)->std::wstring { return repeaterPropsToString(composite, true); });
	registerDecoder(Tile::comparator_off_Id, [](int composite)->std::wstring { return comparatorPropsToString(composite); });
	registerDecoder(Tile::comparator_on_Id, [](int composite)->std::wstring { return comparatorPropsToString(composite); });
	registerDecoder(Tile::redStoneDust_Id, [](int composite)->std::wstring { return redstoneDustPropsToString(composite); });
	registerDecoder(Tile::hugeMushroom_brown_Id, [](int composite)->std::wstring { return hugeMushroomPropsToString(composite); });
	registerDecoder(Tile::hugeMushroom_red_Id, [](int composite)->std::wstring { return hugeMushroomPropsToString(composite); });
	registerDecoder(Tile::hopper_Id, [](int composite)->std::wstring { return hopperPropsToString(composite); });
	registerDecoder(Tile::jukebox_Id, [](int composite)->std::wstring { return jukeboxPropsToString(composite); });
	registerDecoder(Tile::fenceGate_Id, [](int composite)->std::wstring { return fenceGatePropsToString(composite); });
	registerDecoder(Tile::spruceGate_Id, [](int composite)->std::wstring { return fenceGatePropsToString(composite); });
	registerDecoder(Tile::birchGate_Id, [](int composite)->std::wstring { return fenceGatePropsToString(composite); });
	registerDecoder(Tile::jungleGate_Id, [](int composite)->std::wstring { return fenceGatePropsToString(composite); });
	registerDecoder(Tile::darkGate_Id, [](int composite)->std::wstring { return fenceGatePropsToString(composite); });
	registerDecoder(Tile::acaciaGate_Id, [](int composite)->std::wstring { return fenceGatePropsToString(composite); });
	registerDecoder(Tile::daylightDetector_Id, [](int composite)->std::wstring { return daylightDetectorPropsToString(composite, false); });
	registerDecoder(Tile::invertedDaylightDetector_Id, [](int composite)->std::wstring { return daylightDetectorPropsToString(composite, true); });
	registerDecoder(Tile::snow_Id, [](int composite)->std::wstring { return snowPropsToString(composite); });
	registerDecoder(Tile::topSnow_Id, [](int composite)->std::wstring { return snowPropsToString(composite); });
	registerDecoder(Tile::cauldron_Id, [](int composite)->std::wstring { return cauldronPropsToString(composite); });
	registerDecoder(Tile::torch_Id, [](int composite)->std::wstring { return torchPropsToString(composite); });
	registerDecoder(Tile::furnace_Id, [](int composite)->std::wstring { return furnacePropsToString(composite); });
	registerDecoder(Tile::furnace_lit_Id, [](int composite)->std::wstring { return furnacePropsToString(composite); });
	registerDecoder(Tile::redStoneOre_Id, [](int composite)->std::wstring { return redstoneOrePropsToString(composite); });
	registerDecoder(Tile::redStoneOre_lit_Id, [](int composite)->std::wstring { return redstoneOrePropsToString(composite); });
	registerDecoder(Tile::redstoneTorch_off_Id, [](int composite)->std::wstring { return redstoneTorchPropsToString(composite); });
	registerDecoder(Tile::redstoneTorch_on_Id, [](int composite)->std::wstring { return redstoneTorchPropsToString(composite); });
	registerDecoder(Tile::redstoneLight_Id, [](int composite)->std::wstring { return redlightPropsToString(composite); });
	registerDecoder(Tile::redstoneLight_lit_Id, [](int composite)->std::wstring { return redlightPropsToString(composite); });
	return true;
registerDecoder(Tile::torch_Id, [](int composite)->std::wstring { return torchPropsToString(composite); });
registerDecoder(Tile::furnace_Id, [](int composite)->std::wstring { return furnacePropsToString(composite); });
registerDecoder(Tile::furnace_lit_Id, [](int composite)->std::wstring { return furnacePropsToString(composite); });
registerDecoder(Tile::redStoneOre_Id, [](int composite)->std::wstring { return redstoneOrePropsToString(composite); });
registerDecoder(Tile::redStoneOre_lit_Id, [](int composite)->std::wstring { return redstoneOrePropsToString(composite); });
registerDecoder(Tile::redstoneTorch_off_Id, [](int composite)->std::wstring { return redstoneTorchPropsToString(composite); });
registerDecoder(Tile::redstoneTorch_on_Id, [](int composite)->std::wstring { return redstoneTorchPropsToString(composite); });
registerDecoder(Tile::redstoneLight_Id, [](int composite)->std::wstring { return redlightPropsToString(composite); });
registerDecoder(Tile::redstoneLight_lit_Id, [](int composite)->std::wstring { return redlightPropsToString(composite); });
}

static bool s_plantDecoderRegistered = registerPlantDecoders();
