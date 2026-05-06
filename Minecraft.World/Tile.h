#pragma once
#include "Material.h"
#include "Vec3.h"
#include "Definitions.h"
#include "SoundTypes.h"


using namespace std;

class GrassTile;
class LeafTile;
class LeafTile2;
class TallGrass;
class BarrierTile;
class DeadBushTile;
class FireTile;
class PortalTile;
class MycelTile;
class PistonExtensionTile;
class PistonMovingPiece;
class StoneTile;
class stoneBrick;
class Bush;
class StairTile;
class LiquidTile;
class PistonBaseTile;
class ChestTile;
class RedStoneDustTile;
class RepeaterTile;
class CauldronTile;
class TripWireSourceTile;
class BeaconTile;
class ComparatorTile;
class DaylightDetectorTile;
class Random;
class HitResult;
class Level;
class HopperTile;
class Player;
class LevelSource;
class Mob;
class TileEntity;
class HalfSlabTile;
class Icon;
class IconRegister;
class TallGrass2;

class ChunkRebuildData;

class Tile
{
	// 4J Stu - Stair tile accesses the protected members of a Tile object passed in
	friend class StairTile;
	friend class ChunkRebuildData;
	friend class WallTile;

protected:
	// 4J added so we can have separate shapes for different threads
	class ThreadStorage
	{
	public:
		double xx0, yy0, zz0, xx1, yy1, zz1;
		int tileId;
		ThreadStorage();
	};
	static DWORD tlsIdxShape;
public:
	class BlockState {
	public:
		int value;
		BlockState(int v = 0) : value(v) {}
	};

	class BlockStateDefinition {
	public:
		Tile *owner;
		BlockStateDefinition(Tile *ownerTile);
	};

	// create blockstate definition 
	virtual void createBlockStateDefinition();
	// return default tile blockstate
	virtual int defaultBlockState();
	// return blockstate definition
	virtual BlockStateDefinition *getBlockStateDefinition();
	// convert blockstate to legacy bitmask
	virtual int convertBlockStateToLegacyData(BlockState *state);
	// defaultblockstate part 2
	virtual int getBlockState();
	// return blockstate for tile at world position
	virtual BlockState getBlockState(LevelSource *level, int x, int y, int z);
	// Each new thread that needs to use Vec3 pools will need to call one of the following 2 functions, to either create its own
	// local storage, or share the default storage already allocated by the main thread
	static void CreateNewThreadStorage();
	static void ReleaseThreadStorage();

public:
	static const int TILE_NUM_COUNT = 4096;
	static const int TILE_NUM_MASK = 0xfff; // 4096 - 1
	static const int TILE_NUM_SHIFT = 12; // 4096 is 12 bits

	// tile update flags
	// neighbors: notify neighbors the block changed
	static const int UPDATE_NEIGHBORS = (1 << 0);
	// clients: send tile update over network connections
	static const int UPDATE_CLIENTS = (1 << 1);
	// invisible: this update is invisible, so don't rebuild graphics
	static const int UPDATE_INVISIBLE = (1 << 2);
	// clients: send tile update over network connections
	static const int UPDATE_INVISIBLE_NO_LIGHT = (1 << 3) | UPDATE_INVISIBLE;

	static const int UPDATE_NONE = UPDATE_INVISIBLE;
	static const int UPDATE_ALL = UPDATE_NEIGHBORS | UPDATE_CLIENTS;

private:
	// 4J Stu - Was const but had to change it so that we can initialise it in TileStaticInit
	static wstring TILE_DESCRIPTION_PREFIX;
protected:
	static const float INDESTRUCTIBLE_DESTROY_TIME;

	wstring iconName;

public:

	class SoundType
	{
	public:
		//         wstring name;
		// 		wstring breakSound;
		// 		wstring stepSound;
		eMATERIALSOUND_TYPE eMaterialSound;
		int iBreakSound,iStepSound,iPlaceSound;
		float volume;
		float pitch;

		SoundType(eMATERIALSOUND_TYPE eMaterialSound, float volume, float pitch, int iBreakSound = -1, int iPlaceSound = -1);

		float getVolume() const;
		float getPitch() const;
		//wstring getBreakSound() const { return breakSound; }
		//wstring getStepSound()	const { return stepSound; }
		int getBreakSound() const;
		int getStepSound()	const;
		int getPlaceSound() const;
	};

	static SoundType *SOUND_NORMAL;
	static SoundType *SOUND_WOOD;
	static SoundType *SOUND_GRAVEL;
	static SoundType *SOUND_GRASS;
	static SoundType *SOUND_STONE;
	static SoundType *SOUND_METAL;
	static SoundType *SOUND_GLASS;
	static SoundType *SOUND_CLOTH;
	static SoundType *SOUND_SAND;
	static SoundType *SOUND_SNOW;
	static SoundType *SOUND_LADDER;
	static SoundType *SOUND_ANVIL;

	static const int SHAPE_INVISIBLE = -1;
	static const int SHAPE_BLOCK = 0;
	static const int SHAPE_CROSS_TEXTURE = 1;
	static const int SHAPE_TORCH = 2;
	static const int SHAPE_FIRE = 3;
	static const int SHAPE_WATER = 4;
	static const int SHAPE_RED_DUST = 5;
	static const int SHAPE_ROWS = 6;
	static const int SHAPE_DOOR = 7;
	static const int SHAPE_LADDER = 8;
	static const int SHAPE_RAIL = 9;
	static const int SHAPE_STAIRS = 10;
	static const int SHAPE_FENCE = 11;
	static const int SHAPE_LEVER = 12;
	static const int SHAPE_CACTUS = 13;
	static const int SHAPE_BED = 14;
	static const int SHAPE_REPEATER = 15;
	static const int SHAPE_PISTON_BASE = 16;
	static const int SHAPE_PISTON_EXTENSION = 17;
	static const int SHAPE_IRON_FENCE = 18;
	static const int SHAPE_STEM = 19;
	static const int SHAPE_VINE = 20;
	static const int SHAPE_FENCE_GATE = 21;
	static const int SHAPE_ENTITYTILE_ANIMATED = 22;
	static const int SHAPE_LILYPAD = 23;
	static const int SHAPE_CAULDRON = 24;
	static const int SHAPE_BREWING_STAND = 25;
	static const int SHAPE_PORTAL_FRAME = 26;
	static const int SHAPE_EGG = 27;
	static const int SHAPE_COCOA = 28;
	static const int SHAPE_TRIPWIRE_SOURCE = 29;
	static const int SHAPE_TRIPWIRE = 30;
	static const int SHAPE_TREE = 31;
	static const int SHAPE_WALL = 32;
	static const int SHAPE_FLOWER_POT = 33;
	static const int SHAPE_BEACON = 34;
	static const int SHAPE_ANVIL = 35;
	static const int SHAPE_DIODE = 36;
	static const int SHAPE_COMPARATOR = 37;
	static const int SHAPE_HOPPER = 38;
	static const int SHAPE_QUARTZ = 39;
	static const int SHAPE_THIN_PANE = 40;

	static const int SHAPE_COUNT = 41;

	static  Tile **tiles;

	static	bool	mipmapEnable[TILE_NUM_COUNT];
	static  bool	solid[TILE_NUM_COUNT];
	static  int		lightBlock[TILE_NUM_COUNT];
	static  bool	transculent[TILE_NUM_COUNT];
	static  int		lightEmission[TILE_NUM_COUNT];
	static  unsigned char _sendTileData[TILE_NUM_COUNT];		// 4J - was bool, changed to bitfield so we can indicate which bits are important to be sent
	static  bool	propagate[TILE_NUM_COUNT];

	// 4J - this array of simple constants made so the compiler can optimise references to Ids that were previous of the form Tile::<whatever>->id, and are now simply Tile::whatever_Id
	static const int air_Id = 0;
	static const int stone_Id = 1;
	static const int grass_Id = 2;
	static const int dirt_Id = 3;
	static const int cobblestone_Id = 4;
	static const int planks_Id = 5;
	static const int sapling_Id = 6;
	//static const int sapling2_Id = 199;//should go inside sapling.
	static const int bedrock_Id = 7;
	static const int flowing_water_Id = 8;
	static const int water_Id = 9;
	static const int flowing_lava_Id = 10;

	static const int lava_Id = 11;
	static const int sand_Id = 12;
	static const int gravel_Id = 13;
	static const int gold_ore_Id = 14;
	static const int iron_ore_Id = 15;
	static const int coal_ore_Id = 16;
	static const int log_Id = 17;
	static const int leaves_Id = 18;
	static const int leaves2_Id = 161;

	static const int sponge_Id = 19;
	static const int glass_Id = 20;

	static const int lapis_ore_Id = 21;
	static const int lapis_block_Id = 22;
	static const int dispenser_Id = 23;
	static const int sandstone_Id = 24;
	static const int noteblock_Id = 25;
	static const int bed_Id = 26;
	static const int golden_rail_Id = 27;
	static const int detector_rail_Id = 28;
	static const int sticky_piston_Id = 29;
	static const int web_Id = 30;

	static const int tallgrass_Id = 31;
	static const int deadbush_Id = 32;
	static const int piston_Id = 33;
	static const int piston_head_Id = 34;
	static const int wool_Id = 35;
	static const int piston_extension_Id = 36;
	static const int yellow_flower_Id = 37;
	static const int red_flower_Id = 38;
	static const int mushroom_brown_Id = 39;
	static const int mushroom_red_Id = 40;

	static const int gold_block_Id = 41;
	static const int iron_block_Id = 42;
	static const int double_stone_slab_Id = 43;
	static const int stone_slab_Id = 44;
	static const int brick_block_Id = 45;
	static const int tnt_Id = 46;
	static const int bookshelf_Id = 47;
	static const int mossy_cobblestone_Id = 48;
	static const int obsidian_Id = 49;
	static const int torch_Id = 50;

	static const int fire_Id = 51;
	static const int mob_spawner_Id = 52;
	static const int oak_stairs_Id = 53;
	static const int chest_Id = 54;
	static const int redstone_wire_Id = 55;
	static const int diamond_ore_Id = 56;
	static const int diamond_block_Id = 57;
	static const int crafting_table_Id = 58;
	static const int wheat_Id = 59;
	static const int farmland_Id = 60;

	static const int furnace_Id = 61;
	static const int lit_furnace_Id = 62;
	static const int standing_sign_Id = 63;
	static const int wooden_door_Id = 64;
	static const int ladder_Id = 65;
	static const int rail_Id = 66;
	static const int stone_stairs_Id = 67;
	static const int wall_standing_sign_Id = 68;
	static const int lever_Id = 69;
	static const int stone_pressure_plate_Id = 70;

	static const int iron_door_Id = 71;
	static const int wooden_pressure_plate_Id = 72;
	static const int redstone_ore_Id = 73;
	static const int lit_redstone_ore_Id = 74;
	static const int unlit_redstone_torch_Id = 75;
	static const int redstone_torch_Id = 76;
	static const int stone_button_Id = 77;
	static const int snow_layer_Id = 78;
	static const int ice_Id = 79;
	static const int snow_Id = 80;

	static const int cactus_Id = 81;
	static const int clay_Id = 82;
	static const int reeds_Id = 83;
	static const int jukebox_Id = 84;
	static const int fence_Id = 85;
	static const int pumpkin_Id = 86;
	static const int netherrack_Id = 87;
	static const int soul_sand_Id = 88;
	static const int glowstone_Id = 89;
	static const int portal_Id = 90;

	static const int lit_pumpkin_Id = 91;
	static const int cake_Id = 92;
	static const int unpowered_repeater_Id = 93;
	static const int powered_repeater_Id = 94;
	static const int stained_glass_Id = 95;
	static const int trapdoor_Id = 96;
	static const int monster_egg_Id = 97;
	static const int stonebrick_Id = 98;
	static const int brown_mushroom_block_Id = 99;
	static const int red_mushroom_block_Id = 100;

	static const int iron_bars_Id = 101;
	static const int glass_pane_Id = 102;
	static const int melon_block_Id = 103;
	static const int pumpkin_stem_Id = 104;
	static const int melon_stem_Id = 105;
	static const int vine_Id = 106;
	static const int fence_gate_Id = 107;
	static const int brick_stairs_Id = 108;
	static const int stone_brick_stairs_Id = 109;
	static const int mycelium_Id = 110;

	static const int waterlily_Id = 111;
	static const int nether_brick_Id = 112;
	static const int nether_brick_fence_Id = 113;
	static const int nether_brick_stairs_Id = 114;
	static const int nether_wart_Id = 115;
	static const int enchanting_table_Id = 116;
	static const int brewing_stand_Id = 117;
	static const int cauldron_Id = 118;
	static const int end_portal_Id = 119;
	static const int end_portal_frame_Id = 120;

	static const int end_stone_Id = 121;
	static const int dragon_egg_Id = 122;
	static const int redstone_lamp_Id = 123;
	static const int lit_redstone_lamp_Id = 124;
	static const int double_wooden_slab_Id = 125;
	static const int wooden_slab_Id = 126;
	static const int cocoa_Id = 127;
	static const int sandstone_stairs_Id = 128;
	static const int emerald_ore_Id = 129;
	static const int ender_chest_Id = 130;

	static const int tripwire_hook_Id = 131;
	static const int tripwire_Id = 132;
	static const int emerald_block_Id = 133;
	static const int spruce_stairs_Id = 134;
	static const int birch_stairs_Id = 135;
	static const int jungle_stairs_Id = 136;
	static const int command_block_Id = 137;
	static const int beacon_Id = 138;
	static const int cobblestone_wall_Id = 139;
	static const int flower_pot_Id = 140;

	static const int carrots_Id = 141;
	static const int potatoes_Id = 142;
	static const int wooden_button_Id = 143;
	static const int skull_Id = 144;
	static const int anvil_Id = 145;
	static const int chest_trap_Id = 146;
	static const int light_weighted_pressure_plate_Id = 147;
	static const int heavy_weighted_pressure_plate_Id = 148;
	static const int unpowered_comparator_Id = 149;
	static const int powered_comparator_Id = 150;

	static const int daylight_detector_Id = 151;
	static const int redstone_block_Id = 152;
	static const int quartz_ore_Id = 153;
	static const int hopper_Id = 154;
	static const int quartz_block_Id = 155;
	static const int quartz_stairs_Id = 156;
	static const int activator_rail_Id = 157;
	static const int dropper_Id = 158;
	static const int stained_hardened_clay_Id = 159;
	static const int stained_glass_pane_Id = 160;

	static const int log2_Id = 162;

	static const int acacia_stairs_Id = 163;
	static const int dark_oak_stairs_Id = 164;
	//165 slimeblock
	static const int barrier_Id = 166;
	static const int iron_trapdoor_Id = 167;
	static const int prismarine_Id = 168;
	static const int sea_lantern_Id = 169;
	static const int hay_block_Id = 170;
	static const int carpet_Id = 171;
	static const int hardened_clay_Id = 172;
	static const int coal_block_Id = 173;
	static const int packed_ice_Id = 174;
	static const int double_plant_Id = 175;
	//176 standing_banner
	//177 wall_banner
	static const int daylight_detector_inverted_Id = 178;
	static const int red_sandstone_Id = 179;
	static const int red_sandstone_stairs_Id = 180;
	static const int double_stone_slab2_Id = 181;
	static const int stone_slab2_Id = 182;
	static const int spruce_fence_gate_Id = 183;
	static const int birch_fence_gate_Id = 184;
	static const int jungle_fence_gate_Id = 185;
	static const int dark_oak_fence_gate_Id = 186;
	static const int acacia_fence_gate_Id = 187;
	static const int spruce_fence_Id = 188;
	static const int birch_fence_Id = 189;
	static const int jungle_fence_Id = 190;
	static const int dark_oak_fence_Id = 191;
	static const int acacia_fence_Id = 192;
	static const int spruce_door_Id = 193;
	static const int birch_door_Id = 194;
	static const int jungle_door_Id = 195;
	static const int acacia_door_Id = 196;
	static const int dark_oak_door_Id = 197;
	//end_rod 			198
	//chorus_plant 		199
	//chorus_flower 	200
	//purpur_block      201
	//purpur_pillar		202
	//purpur_stairs     203
	//purpur_double_slab 204
	//purpur_slab		205
	//end_bricks		206
	//beetroots         207
	//grass_path		208
	//end_gateway		209
	//frosted_ice		212
	//magma				213
	//nether_wart_block 214
	//red_nether_brick  215
	//bone_block		216
	//structure_void    217
	// 
	//
	

	static Tile *stone;
	static GrassTile *grass;
	static Tile *dirt;
	static Tile *cobblestone;
	static Tile *wood;
	static Tile *sapling;
	//static Tile *sapling2;
	static Tile *unbreakable;
	static LiquidTile *water;
	static Tile *calmWater;
	static LiquidTile *lava;
	static Tile *calmLava;
	static Tile *sand;
	static Tile *gravel;
	static Tile *goldOre;
	static Tile *ironOre;
	static Tile *coalOre;
	static Tile *treeTrunk;
	static LeafTile *leaves;
	static LeafTile2 *leaves2;
	static Tile *sponge;
	static Tile *glass;
	static Tile *lapisOre;
	static Tile *lapisBlock;
	static Tile *dispenser;
	static Tile *sandStone;
	static Tile *noteblock;
	static Tile *bed;
	static Tile *goldenRail;
	static Tile *detectorRail;
	static PistonBaseTile *sticky_piston;
	static Tile *web;
	static TallGrass *tallgrass;
	static DeadBushTile *deadBush;
	static PistonBaseTile *pistonBase;
	static PistonExtensionTile *pistonExtension;
	static Tile *wool;
	static PistonMovingPiece *pistonMovingPiece;
	static Bush *flower;
	static Bush *rose;
	static Bush *mushroom_brown;
	static Bush *mushroom_red;
	static Tile *goldBlock;
	static Tile *ironBlock;
	//     static Tile *stoneSlab;
	//     static Tile *stoneSlabHalf;
	static Tile *redBrick;
	static Tile *tnt;
	static Tile *bookshelf;
	static Tile *mossyCobblestone;
	static Tile *obsidian;
	static Tile *torch;
	static FireTile *fire;
	static Tile *mobSpawner;
	static Tile *stairs_wood;
	static ChestTile *chest;
	static RedStoneDustTile *redStoneDust;
	static Tile *diamondOre;
	static Tile *diamondBlock;
	static Tile *workBench;
	static Tile *wheat;
	static Tile *farmland;
	static Tile *furnace;
	static Tile *furnace_lit;
	static Tile *sign;
	static Tile *wooden_door;
	static Tile *ladder;
	static Tile *rail;
	static Tile *stairs_stone;
	static Tile *wallSign;
	static Tile *lever;
	static Tile *pressurePlate_stone;
	static Tile *iron_door;
	static Tile *pressurePlate_wood;
	static Tile *redStoneOre;
	static Tile *lit_redstone_ore;
	static Tile *redstoneTorch_off;
	static Tile *redstoneTorch_on;
	static Tile *button;
	static Tile *topSnow;
	static Tile *ice;
	static Tile *snow;
	static Tile *cactus;
	static Tile *clay;
	static Tile *reeds;
	static Tile *jukebox;
	static Tile *fence;
	static Tile *pumpkin;
	static Tile *netherRack;
	static Tile *soulsand;
	static Tile *glowstone;
	static PortalTile *portalTile;
	static Tile *litPumpkin;
	static Tile *cake;
	static RepeaterTile *unpowered_repeater;
	static RepeaterTile *powered_repeater;
	static Tile *stained_glass;
	static Tile *trapdoor;

	static Tile *monster_egg;
	static Tile *stoneBrick;
	static Tile *brown_mushroom_block;
	static Tile *red_mushroom_block;
	static Tile *iron_bars;
	static Tile *glass_pane;
	static Tile *melon;
	static Tile *pumpkin_stem;
	static Tile *melon_stem;
	static Tile *vine;
	static Tile *fenceGate;
	static Tile *stairs_bricks;
	static Tile *stone_brick_stairsSmooth;

	static MycelTile *mycel;
	static Tile *waterLily;
	static Tile *netherBrick;
	static Tile *netherFence;
	static Tile *nether_brick_stairs;
	static Tile *netherStalk;
	static Tile *enchantTable;
	static Tile *brewingStand;
	static CauldronTile *cauldron;
	static Tile *end_portal;
	static Tile *end_portal_frame;
	static Tile *endStone;
	static Tile *dragonEgg;
	static Tile *redstoneLight;
	static Tile *redstoneLight_lit;

	static Tile *stairs_sandstone;
	static Tile *woodStairsSpruce;
	static Tile *woodStairsBirch;
	static Tile *woodStairsJungle;
	static Tile *woodStairsAcacia;
	static Tile *woodStairsDark;
	static Tile *commandBlock;
	static BeaconTile *beacon;
	static Tile *button_wood;
	static HalfSlabTile *woodSlab;
	static HalfSlabTile *woodSlabHalf;
	static HalfSlabTile *stoneSlab;
	static HalfSlabTile *stoneSlabHalf;
	static Tile *emeraldOre;
	static Tile *enderChest;
	static TripWireSourceTile *tripWireSource;
	static Tile *tripWire;
	static Tile *emeraldBlock;

	static Tile *cocoa;
	static Tile *skull;

	static Tile *cobbleWall;
	static Tile *flower_pot;
	static Tile *carrots;
	static Tile *potatoes;
	static Tile *anvil;
	static Tile *chest_trap;
	static Tile *light_weighted_pressure_plate;
	static Tile *heavy_weighted_pressure_plate;
	static ComparatorTile *comparator_off;
	static ComparatorTile *comparator_on;

	static DaylightDetectorTile *daylightDetector;
	static Tile *redstoneBlock;

	static Tile *netherQuartz;
	static HopperTile *hopper;
	static Tile *quartzBlock;
	static Tile *stairs_quartz;
	static Tile *activatorRail;
	static Tile *dropper;
	static Tile *stained_hardened_clay;
	static Tile *stained_glass_pane;

	static Tile *hayBlock;
	static Tile *woolCarpet;
	static Tile *clayHardened;
	static Tile *coalBlock;

	static Tile *barrier;
	static Tile *iron_trapdoor;

	static Tile* spruce_door;
	static Tile* birch_door;
	static Tile* jungle_door;
	static Tile* acacia_door;
	static Tile* dark_oak_door;
	
	static Tile* spruceFence;
	static Tile* birchFence;
	static Tile* jungleFence;
	static Tile* acaciaFence;
	static Tile* darkFence;

	static Tile* spruceGate;
	static Tile* birchGate;
	static Tile* jungleGate;
	static Tile* acaciaGate;
	static Tile* darkGate;

	static Tile* daylight_detector_inverted;
	static Tile* red_sandstone;
	static Tile* stairs_red_sandstone;
	static HalfSlabTile* stoneSlab2;
	static HalfSlabTile* stoneSlab2Half;
	static Tile* log2;
	static Tile* packedIce;
	static Tile* seaLantern;
	static Tile* prismarine;

	static TallGrass2* double_plant;
	

	static void staticCtor();

	int id;
protected:
	float destroySpeed;
	float explosionResistance;
	bool isInventoryItem;
	bool collectStatistics;
	bool _isTicking;
	bool _isEntityTile;
	int m_iMaterial;
	int m_iBaseItemType;

	BlockStateDefinition *m_blockStateDefinition;
	int m_defaultBlockState;

	// 4J Stu - Removed this in favour of a TLS version
	//double xx0, yy0, zz0, xx1, yy1, zz1;

public:
	const SoundType *soundType;

	float gravity;
	Material *material;
	float friction;

private:
	unsigned int descriptionId;
	unsigned int useDescriptionId; // 4J Added

protected:
	Icon *icon;

protected:
	void _init(int id, Material *material, bool isSolidRender);
	Tile(int id, Material *material, bool isSolidRender = true);
	virtual ~Tile() {}
protected:
	virtual Tile *sendTileData(unsigned char importantMask=15);		// 4J - added importantMask to indicate which bits in the data are important
protected:
	virtual void init();
	virtual Tile *setSoundType(const SoundType *soundType);
	virtual Tile *setLightBlock(int i);
	virtual Tile *setLightEmission(float f);
	virtual Tile *setExplodeable(float explosionResistance);
	Tile *setBaseItemTypeAndMaterial(int iType,int iMaterial);
public:
	static bool isSolidBlockingTile(int t);
	virtual bool isCubeShaped();
	virtual bool isPathfindable(LevelSource *level, int x, int y, int z);
	virtual int getRenderShape();
	// 4J-PB added
	int getBaseItemType();
	int getMaterial();
protected:
	virtual Tile *setDestroyTime(float destroySpeed);
	virtual Tile *setIndestructible();
public:
	virtual float getDestroySpeed(Level *level, int x, int y, int z);
protected:
	virtual Tile *setTicking(bool tick);
	virtual Tile *disableMipmap();
public:
	virtual bool isTicking();
	virtual bool isEntityTile();
	virtual void setShape(float x0, float y0, float z0, float x1, float y1, float z1);
	virtual float getBrightness(LevelSource *level, int x, int y, int z);
	virtual int getLightColor(LevelSource *level, int x, int y, int z, int tileId=-1);		// 4J - brought forward from 1.8.2
	static bool isFaceVisible(Level *level, int x, int y, int z, int f);
	virtual bool shouldRenderFace(LevelSource *level, int x, int y, int z, int face);
	virtual bool isSolidFace(LevelSource *level, int x, int y, int z, int face);
	virtual Icon *getTexture(LevelSource *level, int x, int y, int z, int face);
	virtual Icon *getTexture(int face, int data);
	virtual Icon *getTexture(int face);
	virtual AABB *getTileAABB(Level *level, int x, int y, int z);
	virtual void addAABBs(Level *level, int x, int y, int z, AABB *box, AABBList *boxes, shared_ptr<Entity> source);
	virtual AABB *getAABB(Level *level, int x, int y, int z);
	virtual bool isSolidRender(bool isServerLevel = false);							// 4J - Added isServerLevel param
	virtual bool mayPick(int data, bool liquid);
	virtual bool mayPick();
	virtual void tick(Level *level, int x, int y, int z, Random *random);
	virtual void animateTick(Level *level, int x, int y, int z, Random *random);
	virtual void destroy(Level *level, int x, int y, int z, int data);
	virtual void neighborChanged(Level *level, int x, int y, int z, int type);
	virtual void addLights(Level *level, int x, int y, int z);
	virtual int getTickDelay(Level *level);
	virtual void onPlace(Level *level, int x, int y, int z);
	virtual void onRemove(Level *level, int x, int y, int z, int id, int data);
	virtual int getResourceCount(Random *random);
	virtual int getResource(int data, Random *random, int playerBonusLevel);
	virtual float getDestroyProgress(shared_ptr<Player> player, Level *level, int x, int y, int z);
	virtual void spawnResources(Level *level, int x, int y, int z, int data, int playerBonusLevel);
	virtual void spawnResources(Level *level, int x, int y, int z, int data, float odds, int playerBonusLevel);
protected:
	virtual void popResource(Level *level, int x, int y, int z, shared_ptr<ItemInstance> itemInstance);
	virtual void popExperience(Level *level, int x, int y, int z, int amount);

public:
	virtual int getSpawnResourcesAuxValue(int data);
	virtual float getExplosionResistance(shared_ptr<Entity> source);
	virtual HitResult *clip(Level *level, int xt, int yt, int zt, Vec3 *a, Vec3 *b);
private:
	virtual bool containsX(Vec3 *v);
	virtual bool containsY(Vec3 *v);
	virtual bool containsZ(Vec3 *v);
public:
	virtual void wasExploded(Level *level, int x, int y, int z, Explosion *explosion);
	virtual int getRenderLayer();
	virtual bool mayPlace(Level *level, int x, int y, int z, int face, shared_ptr<ItemInstance> item);
	virtual bool mayPlace(Level *level, int x, int y, int z, int face);
	virtual bool mayPlace(Level *level, int x, int y, int z);
	virtual bool TestUse();
	virtual bool TestUse(Level *level, int x, int y, int z, shared_ptr<Player> player);
	virtual bool use(Level *level, int x, int y, int z, shared_ptr<Player> player, int clickedFace, float clickX, float clickY, float clickZ, bool soundOnly = false); // 4J added soundOnly param
	virtual void stepOn(Level *level, int x, int y, int z, shared_ptr<Entity> entity);
	virtual int getPlacedOnFaceDataValue(Level *level, int x, int y, int z, int face, float clickX, float clickY, float clickZ, int itemValue);
	virtual void prepareRender(Level *level, int x, int y, int z);
	virtual void attack(Level *level, int x, int y, int z, shared_ptr<Player> player);
	virtual void handleEntityInside(Level *level, int x, int y, int z, shared_ptr<Entity> e, Vec3 *current);
	virtual void updateShape(LevelSource *level, int x, int y, int z, int forceData = -1, shared_ptr<TileEntity> forceEntity = shared_ptr<TileEntity>());	// 4J added forceData, forceEntity param
	virtual double getShapeX0();
	virtual double getShapeX1();
	virtual double getShapeY0();
	virtual double getShapeY1();
	virtual double getShapeZ0();
	virtual double getShapeZ1();
	virtual int getColor() const;
	virtual int getColor(int auxData);
	virtual int getColor(LevelSource *level, int x, int y, int z);
	virtual int getColor(LevelSource *level, int x, int y, int z, int data);		// 4J added
	virtual int getSignal(LevelSource *level, int x, int y, int z, int dir);
	virtual bool isSignalSource();
	virtual void entityInside(Level *level, int x, int y, int z, shared_ptr<Entity> entity);
	virtual int getDirectSignal(LevelSource *level, int x, int y, int z, int dir);
	virtual void updateDefaultShape();
	virtual void playerDestroy(Level *level, shared_ptr<Player> player, int x, int y, int z, int data);
	virtual bool canSurvive(Level *level, int x, int y, int z);
protected:
	virtual bool isSilkTouchable();
	virtual shared_ptr<ItemInstance> getSilkTouchItemInstance(int data);
public:
	virtual int getResourceCountForLootBonus(int bonusLevel, Random *random);
	virtual void setPlacedBy(Level *level, int x, int y, int z, shared_ptr<LivingEntity> by, shared_ptr<ItemInstance> itemInstance);
	virtual void finalizePlacement(Level *level, int x, int y, int z, int data);
	virtual Tile *setDescriptionId(unsigned int id);
	virtual wstring getName();
	virtual unsigned int getDescriptionId(int iData = -1);
	virtual Tile *setUseDescriptionId(unsigned int id); // 4J Added
	virtual unsigned int getUseDescriptionId(); // 4J Added
	virtual bool triggerEvent(Level *level, int x, int y, int z, int b0, int b1);
	virtual bool isCollectStatistics();

	// 4J Added so we can check before we try to add a tile to the tick list if it's actually going to do seomthing
	// Default to true (it's also checking a bool array) and just override when we need to be able to say no
	virtual bool shouldTileTick(Level *level, int x,int y,int z) { return true; }
protected:
	virtual Tile *setNotCollectStatistics();
public:
	virtual int getPistonPushReaction();
	virtual float getShadeBrightness(LevelSource *level, int x, int y, int z);	// 4J - brought forward from 1.8.2
	virtual void fallOn(Level *level, int x, int y, int z, shared_ptr<Entity> entity, float fallDistance);
	virtual int cloneTileId(Level *level, int x, int y, int z);
	virtual int cloneTileData(Level *level, int x, int y, int z);
	virtual void playerWillDestroy(Level *level, int x, int y, int z, int data, shared_ptr<Player> player);
	virtual void onRemoving(Level *level, int x, int y, int z, int data);
	virtual void handleRain(Level *level, int x, int y, int z);
	virtual void levelTimeChanged(Level *level, int64_t delta, int64_t newTime);
	virtual bool useOwnCloneData();
	virtual bool canInstantlyTick();
	virtual bool dropFromExplosion(Explosion *explosion);
	virtual bool isMatching(int id);
	static bool isMatching(int tileIdA, int tileIdB);
	virtual bool hasAnalogOutputSignal();
	virtual int getAnalogOutputSignal(Level *level, int x, int y, int z, int dir);
	virtual bool blocksLight() { return Tile::lightBlock[id] > 0; }

protected:
	virtual Tile *setIconName(const wstring &iconName);
	virtual wstring getIconName();

public:
	virtual void registerIcons(IconRegister *iconRegister);
	virtual wstring getTileItemIconName();

	// AP - added this function so we can generate the faceFlags for a block in a single fast function
	int getFaceFlags(LevelSource *level, int x, int y, int z);
};

class stoneBrick : public Tile {};
