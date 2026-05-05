#include "stdafx.h"
#include "net.minecraft.stats.h"
#include "net.minecraft.world.item.h"
#include "net.minecraft.world.level.tile.h"
#include "Achievement.h"
#include "Achievements.h"


const int Achievements::ACHIEVEMENT_OFFSET = 0x500000;

// maximum position of achievements (min and max)

int Achievements::xMin = 4294967295; // 4J Stu Was 4294967296 which is 1 larger than maxint. Hopefully no side effects
int Achievements::yMin = 4294967295; // 4J Stu Was 4294967296 which is 1 larger than maxint. Hopefully no side effects
int Achievements::xMax = 0;
int Achievements::yMax = 0;

vector<Achievement *> *Achievements::achievements = new vector<Achievement *>;

Achievement *Achievements::openInventory = nullptr;
Achievement *Achievements::mineWood = nullptr;
Achievement *Achievements::buildWorkbench = nullptr;
Achievement *Achievements::buildPickaxe = nullptr;
Achievement *Achievements::buildFurnace = nullptr;
Achievement *Achievements::acquireIron = nullptr;
Achievement *Achievements::buildHoe = nullptr;
Achievement *Achievements::makeBread = nullptr;
Achievement *Achievements::bakeCake = nullptr;
Achievement *Achievements::buildBetterPickaxe = nullptr;
Achievement *Achievements::cookFish = nullptr;
Achievement *Achievements::onARail = nullptr;
Achievement *Achievements::buildSword = nullptr;
Achievement *Achievements::killEnemy = nullptr;
Achievement *Achievements::killCow = nullptr;
Achievement *Achievements::flyPig = nullptr;

Achievement *Achievements::snipeSkeleton = nullptr;
Achievement *Achievements::diamonds = nullptr;
//Achievement *Achievements::portal = nullptr;
Achievement *Achievements::ghast = nullptr;
Achievement *Achievements::blazeRod = nullptr;
Achievement *Achievements::potion = nullptr;
Achievement *Achievements::theEnd = nullptr;
Achievement *Achievements::winGame = nullptr;
Achievement *Achievements::enchantments = nullptr;
//Achievement *Achievements::overkill = nullptr;
//Achievement *Achievements::bookcase = nullptr;

// 4J : WESTY : Added new acheivements. 
Achievement *Achievements::leaderOfThePack = nullptr;
Achievement *Achievements::MOARTools = nullptr;
Achievement *Achievements::dispenseWithThis = nullptr;
Achievement *Achievements::InToTheNether = nullptr;

// 4J : WESTY : Added other awards. 
//Achievement *Achievements::socialPost = nullptr;
Achievement *Achievements::eatPorkChop = nullptr;
Achievement *Achievements::play100Days = nullptr;
Achievement *Achievements::arrowKillCreeper = nullptr;
//Achievement *Achievements::mine100Blocks = nullptr;
//Achievement *Achievements::kill10Creepers = nullptr;


Achievement *Achievements::overkill = nullptr;	// Restored old achivements.
Achievement *Achievements::bookcase = nullptr; // Restored old achivements.

// 4J-JEV: New Achievements for Orbis.
Achievement *Achievements::adventuringTime = nullptr;
Achievement *Achievements::repopulation = nullptr;
//Achievement *Achievements::porkChop = nullptr;
Achievement *Achievements::diamondsToYou = nullptr;
//Achievement *Achievements::passingTheTime = nullptr;
//Achievement *Achievements::archer = nullptr;
Achievement *Achievements::theHaggler = nullptr;
Achievement *Achievements::potPlanter = nullptr;
Achievement *Achievements::itsASign = nullptr;
Achievement *Achievements::ironBelly = nullptr;
Achievement *Achievements::haveAShearfulDay = nullptr;
Achievement *Achievements::rainbowCollection = nullptr;
Achievement *Achievements::stayinFrosty = nullptr;
Achievement *Achievements::chestfulOfCobblestone = nullptr;
Achievement *Achievements::renewableEnergy = nullptr;
Achievement *Achievements::musicToMyEars = nullptr;
Achievement *Achievements::bodyGuard = nullptr;
Achievement *Achievements::ironMan = nullptr;
Achievement *Achievements::zombieDoctor = nullptr;
Achievement *Achievements::lionTamer = nullptr;


void Achievements::staticCtor()
{
	Achievements::openInventory			= (new Achievement(eAward_TakingInventory,		L"openInventory",		0, 0,	Item::book,			nullptr, "001", IDS_ACHIEVE_NAME_TAKING_INVENTORY, IDS_ACHIEVE_DESC_TAKING_INVENTORY))->setAwardLocallyOnly()->postConstruct();
	Achievements::mineWood				= (new Achievement(eAward_GettingWood,			L"mineWood",			2, 1,	Tile::treeTrunk,	(Achievement *) openInventory, "002", IDS_ACHIEVE_NAME_GETTING_WOOD, IDS_ACHIEVE_DESC_GETTING_WOOD))->postConstruct();
	Achievements::buildWorkbench		= (new Achievement(eAward_Benchmarking,			L"buildWorkBench",		4, -1,	Tile::workBench,	(Achievement *) mineWood, "003", IDS_ACHIEVE_NAME_BENCHMARKING, IDS_ACHIEVE_DESC_BENCHMARKING))->postConstruct();
	Achievements::buildPickaxe			= (new Achievement(eAward_TimeToMine,			L"buildPickaxe",		4, 2,	Item::pickAxe_wood, (Achievement *) buildWorkbench, "004", IDS_ACHIEVE_NAME_TIME_TO_MINE, IDS_ACHIEVE_DESC_TIME_TO_MINE))->postConstruct();
	Achievements::buildFurnace			= (new Achievement(eAward_HotTopic,				L"buildFurnace",		3, 4,	Tile::furnace_lit,	(Achievement *) buildPickaxe, "005", IDS_ACHIEVE_NAME_HOT_TOPIC, IDS_ACHIEVE_DESC_HOT_TOPIC))->postConstruct();
	Achievements::acquireIron			= (new Achievement(eAward_AquireHardware,		L"acquireIron",			1, 4,	Item::ironIngot,	(Achievement *) buildFurnace, "006", IDS_ACHIEVE_NAME_ACQUIRE_HARDWARE, IDS_ACHIEVE_DESC_ACQUIRE_HARDWARE))->postConstruct();
	Achievements::buildHoe				= (new Achievement(eAward_TimeToFarm,			L"buildHoe",			2, -3,	Item::hoe_wood,		(Achievement *) buildWorkbench, "007", IDS_ACHIEVE_NAME_TIME_TO_FARM, IDS_ACHIEVE_DESC_TIME_TO_FARM))->postConstruct();
	Achievements::makeBread				= (new Achievement(eAward_BakeBread,			L"makeBread",			-1, -3, Item::bread,		(Achievement *) buildHoe, "008", IDS_ACHIEVE_NAME_BAKE_BREAD, IDS_ACHIEVE_DESC_BAKE_BREAD))->postConstruct();
	Achievements::bakeCake				= (new Achievement(eAward_TheLie,				L"bakeCake",			0, -5,	Item::cake,			(Achievement *) buildHoe, "009", IDS_ACHIEVE_NAME_THE_LIE, IDS_ACHIEVE_DESC_THE_LIE))->postConstruct();
	Achievements::buildBetterPickaxe	= (new Achievement(eAward_GettingAnUpgrade,		L"buildBetterPickaxe",	6, 2,	Item::pickAxe_stone, (Achievement *) buildPickaxe, "010", IDS_ACHIEVE_NAME_GETTING_AN_UPGRADE, IDS_ACHIEVE_DESC_GETTING_AN_UPGRADE))->postConstruct();
	Achievements::cookFish				= (new Achievement(eAward_DeliciousFish,		L"cookFish",			2, 6,	Item::fish_cooked,	(Achievement *) buildFurnace, "011", IDS_ACHIEVE_NAME_DELICIOUS_FISH, IDS_ACHIEVE_DESC_DELICIOUS_FISH))->postConstruct();
	Achievements::onARail				= (new Achievement(eAward_OnARail,				L"onARail",				2, 3,	Tile::rail,			(Achievement *) acquireIron, "012", IDS_ACHIEVE_NAME_ON_A_RAIL, IDS_ACHIEVE_DESC_ON_A_RAIL))->setGolden()->postConstruct();
	Achievements::buildSword			= (new Achievement(eAward_TimeToStrike,			L"buildSword",			6, -1,	Item::sword_wood,	(Achievement *) buildWorkbench, "013", IDS_ACHIEVE_NAME_TIME_TO_STRIKE, IDS_ACHIEVE_DESC_TIME_TO_STRIKE))->postConstruct();
	Achievements::killEnemy				= (new Achievement(eAward_MonsterHunter,		L"killEnemy",			8, -1,	Item::bone,			(Achievement *) buildSword, "014", IDS_ACHIEVE_NAME_MONSTER_HUNTER, IDS_ACHIEVE_DESC_MONSTER_HUNTER))->postConstruct();
	Achievements::killCow				= (new Achievement(eAward_CowTipper,			L"killCow",				7, -3,	Item::leather,		(Achievement *) buildSword, "015", IDS_ACHIEVE_NAME_COW_TIPPER, IDS_ACHIEVE_DESC_COW_TIPPER))->postConstruct();
	Achievements::flyPig				= (new Achievement(eAward_WhenPigsFly,			L"flyPig",				8, -4,	Item::saddle,		(Achievement *) killCow, "016", IDS_ACHIEVE_NAME_WHEN_PIGS_FLY, IDS_ACHIEVE_DESC_WHEN_PIGS_FLY))->setGolden()->postConstruct();

	// 4J Stu - The order of these achievemnts is very important, as they map directly to data stored in the profile data. New achievements should be added at the end.
	
	// 4J : WESTY : Added new achievements. Note, params "x", "y", "icon" and "requires" are ignored on xbox.
	Achievements::leaderOfThePack		= (new Achievement(eAward_LeaderOfThePack,		L"leaderOfThePack",		0, 0,	Tile::treeTrunk,	(Achievement *) buildSword, "017", IDS_ACHIEVE_NAME_LEADER_OF_THE_PACK, IDS_ACHIEVE_DESC_LEADER_OF_THE_PACK))->setAwardLocallyOnly()->postConstruct();
	Achievements::MOARTools				= (new Achievement(eAward_MOARTools,			L"MOARTools",			0, 0,	Tile::treeTrunk,	(Achievement *) buildSword, "018", IDS_ACHIEVE_NAME_MOAR_TOOLS, IDS_ACHIEVE_DESC_MOAR_TOOLS))->setAwardLocallyOnly()->postConstruct();
	Achievements::dispenseWithThis		= (new Achievement(eAward_DispenseWithThis,		L"dispenseWithThis",	0, 0,	Tile::treeTrunk,	(Achievement *) buildSword, "019", IDS_ACHIEVE_NAME_DISPENSE_WITH_THIS, IDS_ACHIEVE_DESC_DISPENSE_WITH_THIS))->postConstruct();
	Achievements::InToTheNether			= (new Achievement(eAward_InToTheNether,		L"InToTheNether",		0, 0,	Tile::treeTrunk,	(Achievement *) buildSword, "020", IDS_ACHIEVE_NAME_INTO_THE_NETHER, IDS_ACHIEVE_DESC_INTO_THE_NETHER))->postConstruct();

	// 4J : WESTY : Added other awards.
	//Achievements::mine100Blocks			= (new Achievement(eAward_mine100Blocks,		L"mine100Blocks",		0, 0,	Tile::treeTrunk,	(Achievement *) buildSword))->setAwardLocallyOnly()->postConstruct();
	//Achievements::kill10Creepers		= (new Achievement(eAward_kill10Creepers,		L"kill10Creepers",		0, 0,	Tile::treeTrunk,	(Achievement *) buildSword))->setAwardLocallyOnly()->postConstruct();
#ifdef _EXTENDED_ACHIEVEMENTS
	Achievements::eatPorkChop			= (new Achievement(eAward_eatPorkChop,			L"eatPorkChop",			0, 0,	Tile::treeTrunk,	(Achievement *) buildSword))->setAwardLocallyOnly()->postConstruct();
#else
	Achievements::eatPorkChop			= (new Achievement(eAward_eatPorkChop,			L"eatPorkChop",			0, 0,	Tile::treeTrunk,	(Achievement *) buildSword, "034", IDS_ACHIEVE_NAME_EAT_PORKCHOP, IDS_ACHIEVE_DESC_EAT_PORKCHOP))->setAwardLocallyOnly()->postConstruct();
#endif
	Achievements::play100Days			= (new Achievement(eAward_play100Days,			L"play100Days",			0, 0,	Tile::treeTrunk,	(Achievement *) buildSword, "035", IDS_ACHIEVE_NAME_PLAY_100_DAYS, IDS_ACHIEVE_DESC_PLAY_100_DAYS))->setAwardLocallyOnly()->postConstruct();
	Achievements::arrowKillCreeper		= (new Achievement(eAward_arrowKillCreeper,		L"arrowKillCreeper",	0, 0,	Tile::treeTrunk,	(Achievement *) buildSword, "036", IDS_ACHIEVE_NAME_ARROW_KILL_CREEPER, IDS_ACHIEVE_DESC_ARROW_KILL_CREEPER))->postConstruct();
	//Achievements::socialPost			= (new Achievement(eAward_socialPost,			L"socialPost",			0, 0,	Tile::treeTrunk,	(Achievement *) buildSword))->postConstruct();

#ifndef _XBOX
// WARNING: NO NEW ACHIEVMENTS CAN BE ADDED HERE
// These stats (achievements) are directly followed by new stats/achievements in the profile data, so cannot be changed without migrating the profile data

	// 4J Stu - All new Java achievements removed to stop them using the profile data

	// 4J Stu - This achievment added in 1.8.2, but does not map to any Xbox achievements
	Achievements::snipeSkeleton			= (new Achievement(eAward_snipeSkeleton,		L"snipeSkeleton",		7, 0,	Item::bow,			(Achievement *) killEnemy, "021", IDS_ACHIEVE_NAME_SNIPESKELETON, IDS_ACHIEVE_DESC_SNIPESKELETON))->setGolden()->postConstruct();

	// 4J Stu - These added in 1.0.1, but do not map to any Xbox achievements
	Achievements::diamonds				= (new Achievement(eAward_diamonds,				L"diamonds",			-1, 5,	Item::diamond,	(Achievement *) acquireIron, "022", IDS_ACHIEVE_NAME_DIAMONDS, IDS_ACHIEVE_DESC_DIAMONDS) )->postConstruct();
	//Achievements::portal				= (new Achievement(eAward_portal,				L"portal",				-1, 7,	Tile::obsidian,		(Achievement *)diamonds) )->postConstruct();
	Achievements::ghast					= (new Achievement(eAward_ghast,				L"ghast",				-4, 8,	Item::ghastTear,	(Achievement *)ghast, "023", IDS_ACHIEVE_NAME_GHAST, IDS_ACHIEVE_DESC_GHAST) )->setGolden()->postConstruct();
	Achievements::blazeRod				= (new Achievement(eAward_blazeRod,				L"blazeRod",			0, 9,	Item::blazeRod,		(Achievement *)blazeRod, "024", IDS_ACHIEVE_NAME_BLAZEROD, IDS_ACHIEVE_DESC_BLAZEROD) )->postConstruct();
	Achievements::potion				= (new Achievement(eAward_potion,				L"potion",				2, 8,	Item::potion,		(Achievement *)potion, "025", IDS_ACHIEVE_NAME_POTION, IDS_ACHIEVE_DESC_POTION) )->postConstruct();
	Achievements::theEnd				= (new Achievement(eAward_theEnd,				L"theEnd",				3, 10,	Item::eyeOfEnder,	(Achievement *)theEnd, "026", IDS_ACHIEVE_NAME_THE_END, IDS_ACHIEVE_DESC_THE_END) )->setGolden()->postConstruct();
	Achievements::winGame				= (new Achievement(eAward_winGame,				L"theEnd2",				4, 13,	Tile::dragonEgg,	(Achievement *)winGame, "027", IDS_ACHIEVE_NAME_WINGAME, IDS_ACHIEVE_DESC_WINGAME) )->setGolden()->postConstruct();
	Achievements::enchantments			= (new Achievement(eAward_enchantments,			L"enchantments",		-4, 4,	Tile::enchantTable,	(Achievement *)enchantments, "028", IDS_ACHIEVE_NAME_ENCHANTMENTS, IDS_ACHIEVE_DESC_ENCHANTMENTS) )->postConstruct();
 //   Achievements::overkill				= (new Achievement(eAward_overkill,				L"overkill",			-4, 1,	Item::sword_diamond, (Achievement *)enchantments) )->setGolden()->postConstruct();
 //   Achievements::bookcase				= (new Achievement(eAward_bookcase,				L"bookcase",			-3, 6,	Tile::bookshelf,	(Achievement *)enchantments) )->postConstruct();
#endif


	Achievements::overkill				= (new Achievement(eAward_overkill,					L"overkill",				-4,1,	Item::sword_diamond,	(Achievement *)enchantments, "029", IDS_ACHIEVE_NAME_OVERKILL, IDS_ACHIEVE_DESC_OVERKILL) )->setGolden()->postConstruct();
	Achievements::bookcase				= (new Achievement(eAward_bookcase,					L"bookcase",				-3,6,	Tile::bookshelf,		(Achievement *)enchantments, "030", IDS_ACHIEVE_NAME_BOOKCASE, IDS_ACHIEVE_DESC_BOOKCASE) )->postConstruct();

	Achievements::adventuringTime		= (new Achievement(eAward_adventuringTime,			L"adventuringTime",			0,0,	Tile::bookshelf,		(Achievement*) nullptr, "031", IDS_ACHIEVE_NAME_ADVENTURING_TIME, IDS_ACHIEVE_DESC_ADVENTURING_TIME) )->setAwardLocallyOnly()->postConstruct();
	Achievements::repopulation			= (new Achievement(eAward_repopulation,				L"repopulation",			7,-5,	Tile::bookshelf,		(Achievement*) nullptr, "032", IDS_ACHIEVE_NAME_REPOPULATION, IDS_ACHIEVE_DESC_REPOPULATION) )->postConstruct();
	//Achievements::porkChoop			// // //																						// // //
	Achievements::diamondsToYou			= (new Achievement(eAward_diamondsToYou,			L"diamondsToYou",			0,0,	Tile::bookshelf,		(Achievement*) nullptr, "033", IDS_ACHIEVE_NAME_DIAMONDS_TO_YOU, IDS_ACHIEVE_DESC_DIAMONDS_TO_YOU) )->postConstruct();
	//Achievements::passingTheTime		= (new Achievement(eAward_play100Days,				L"passingTheTime",			0,0,	Tile::bookshelf,		(Achievement*) nullptr) )->postConstruct();	
	//Achievements::archer				= (new Achievement(eAward_arrowKillCreeper,			L"archer",					0,0,	Tile::bookshelf,		(Achievement*) nullptr) )->postConstruct();	
	Achievements::theHaggler			= (new Achievement(eAward_theHaggler,				L"theHaggler",				0,0,	Tile::bookshelf,		(Achievement*) nullptr, "037", IDS_ACHIEVE_NAME_THE_HAGGLER, IDS_ACHIEVE_DESC_THE_HAGGLER) )->setAwardLocallyOnly()->postConstruct();
	Achievements::potPlanter			= (new Achievement(eAward_potPlanter,				L"potPlanter",				0,0,	Tile::bookshelf,		(Achievement*) nullptr, "038", IDS_ACHIEVE_NAME_POT_PLANTER, IDS_ACHIEVE_DESC_POT_PLANTER) )->setAwardLocallyOnly()->postConstruct();
	Achievements::itsASign				= (new Achievement(eAward_itsASign,					L"itsASign",				0,0,	Tile::bookshelf,		(Achievement*) nullptr, "039", IDS_ACHIEVE_NAME_ITS_A_SIGN, IDS_ACHIEVE_DESC_ITS_A_SIGN) )->setAwardLocallyOnly()->postConstruct();
	Achievements::ironBelly				= (new Achievement(eAward_ironBelly,				L"ironBelly",				0,0,	Tile::bookshelf,		(Achievement*) nullptr, "040", IDS_ACHIEVE_NAME_IRON_BELLY, IDS_ACHIEVE_DESC_IRON_BELLY) )->postConstruct();
	Achievements::haveAShearfulDay		= (new Achievement(eAward_haveAShearfulDay,			L"haveAShearfulDay",		0,0,	Tile::bookshelf,		(Achievement*) nullptr, "041", IDS_ACHIEVE_NAME_HAVE_A_SHEARFUL_DAY, IDS_ACHIEVE_DESC_HAVE_A_SHEARFUL_DAY) )->postConstruct();
	Achievements::rainbowCollection		= (new Achievement(eAward_rainbowCollection,		L"rainbowCollection",		0,0,	Tile::bookshelf,		(Achievement*) nullptr, "042", IDS_ACHIEVE_NAME_RAINBOW_COLLECTION, IDS_ACHIEVE_DESC_RAINBOW_COLLECTION) )->setAwardLocallyOnly()->postConstruct();
	Achievements::stayinFrosty			= (new Achievement(eAward_stayinFrosty,				L"stayingFrosty",			0,0,	Tile::bookshelf,		(Achievement*) nullptr, "043", IDS_ACHIEVE_NAME_STAYING_FROSTY, IDS_ACHIEVE_DESC_STAYING_FROSTY) )->postConstruct();
	Achievements::chestfulOfCobblestone	= (new Achievement(eAward_chestfulOfCobblestone,	L"chestfulOfCobblestone",	0,0,	Tile::bookshelf,		(Achievement*) nullptr, "044", IDS_ACHIEVE_NAME_CHESTFUL_OF_COBBLESTONE, IDS_ACHIEVE_DESC_CHESTFUL_OF_COBBLESTONE) )->setAwardLocallyOnly()->postConstruct();
	Achievements::renewableEnergy		= (new Achievement(eAward_renewableEnergy,			L"renewableEnergy",			0,0,	Tile::bookshelf,		(Achievement*) nullptr, "045", IDS_ACHIEVE_NAME_RENEWABLE_ENERGY, IDS_ACHIEVE_DESC_RENEWABLE_ENERGY) )->postConstruct();
	Achievements::musicToMyEars			= (new Achievement(eAward_musicToMyEars,			L"musicToMyEars",			0,0,	Tile::bookshelf,		(Achievement*) nullptr, "046", IDS_ACHIEVE_NAME_MUSIC_TO_MY_EARS, IDS_ACHIEVE_DESC_MUSIC_TO_MY_EARS) )->postConstruct();
	Achievements::bodyGuard				= (new Achievement(eAward_bodyGuard,				L"bodyGuard",				0,0,	Tile::bookshelf,		(Achievement*) nullptr, "047", IDS_ACHIEVE_NAME_BODYGUARD, IDS_ACHIEVE_DESC_BODYGUARD) )->postConstruct();
	Achievements::ironMan				= (new Achievement(eAward_ironMan,					L"ironMan",					0,0,	Tile::bookshelf,		(Achievement*) nullptr, "048", IDS_ACHIEVE_NAME_IRON_MAN, IDS_ACHIEVE_DESC_IRON_MAN) )->postConstruct();
	Achievements::zombieDoctor			= (new Achievement(eAward_zombieDoctor,				L"zombieDoctor",			0,0,	Tile::bookshelf,		(Achievement*) nullptr, "049", IDS_ACHIEVE_NAME_ZOMBIE_DOCTOR, IDS_ACHIEVE_DESC_ZOMBIE_DOCTOR) )->postConstruct();
	Achievements::lionTamer				= (new Achievement(eAward_lionTamer,				L"lionTamer",				0,0,	Tile::bookshelf,		(Achievement*) nullptr, "050", IDS_ACHIEVE_NAME_LION_TAMER, IDS_ACHIEVE_DESC_LION_TAMER) )->postConstruct();

}

// Static { System.out.println(achievements.size() + " achievements"); }	TODO


void Achievements::init()
{
}

