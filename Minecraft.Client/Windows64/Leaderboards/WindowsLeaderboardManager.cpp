#include "stdafx.h"

#include "WindowsLeaderboardManager.h"

#include <algorithm>

#include "../../Minecraft.h"
#include "../../StatsCounter.h"

#include "../../../Minecraft.World/StringHelpers.h"
#include "../../../Minecraft.World/Stats.h"
#include "../../../Minecraft.World/net.minecraft.world.item.h"
#include "../../../Minecraft.World/net.minecraft.world.level.tile.h"

namespace
{
	static const DWORD kLeaderboardCacheMagic = 0x31424C57; // file identifier: WLB1
	static const DWORD kLeaderboardCacheVersion = 1;
	static const unsigned int kDifficultyCount = 4;
	static const unsigned int kPersistedBoardCount =
		static_cast<unsigned int>(LeaderboardManager::eStatsType_MAX) * kDifficultyCount;

	struct PersistedLeaderboardRow
	{
		BYTE valid;
		BYTE statsSize;
		WORD reserved;
		DWORD totalScore;
		ULONGLONG uid;
		DWORD statsData[LeaderboardManager::ReadScore::STATSDATA_MAX];
		wchar_t name[XUSER_NAME_SIZE + 1];
	};

	struct PersistedLeaderboardCache
	{
		DWORD magic;
		DWORD version;
		PersistedLeaderboardRow rows[kPersistedBoardCount];
	};

	static int ClampDifficulty(int difficulty)
	{
		if (difficulty < 0)
			return 0;
		if (difficulty > 3)
			return 3;
		return difficulty;
	}

	static unsigned long ClampToULong(unsigned long long value)
	{
		const unsigned long long maxValue = static_cast<unsigned long long>(ULONG_MAX);
		if (value > maxValue)
			return ULONG_MAX;
		return static_cast<unsigned long>(value);
	}

	static unsigned int CombinePigmanKills(StatsCounter* stats, unsigned int difficulty)
	{
		if (stats == nullptr)
			return 0;

		const unsigned int zombiePigman = stats->getValue(Stats::killsZombiePigman, difficulty);
		const unsigned int netherPigman = stats->getValue(Stats::killsNetherZombiePigman, difficulty);
		return zombiePigman + netherPigman;
	}

	static void BuildLeaderboardCachePath(char* outPath, size_t outPathSize)
	{
		if (outPath == nullptr || outPathSize == 0)
			return;

		outPath[0] = '\0';
		GetModuleFileNameA(nullptr, outPath, static_cast<DWORD>(outPathSize));

		char* lastSlash = strrchr(outPath, '\\');
		char* lastForwardSlash = strrchr(outPath, '/');
		if (lastForwardSlash != nullptr && (lastSlash == nullptr || lastForwardSlash > lastSlash))
			lastSlash = lastForwardSlash;

		if (lastSlash != nullptr)
			*(lastSlash + 1) = '\0';

		strncat_s(outPath, outPathSize, "leaderboards-cache.dat", _TRUNCATE);
	}

	static void InitLeaderboardCache(PersistedLeaderboardCache& cache)
	{
		ZeroMemory(&cache, sizeof(cache));
		cache.magic = kLeaderboardCacheMagic;
		cache.version = kLeaderboardCacheVersion;
	}

	static bool LoadLeaderboardCache(PersistedLeaderboardCache& outCache)
	{
		InitLeaderboardCache(outCache);

		char filePath[MAX_PATH] = {};
		BuildLeaderboardCachePath(filePath, MAX_PATH);

		FILE* f = nullptr;
		if (fopen_s(&f, filePath, "rb") != 0 || f == nullptr)
			return false;

		PersistedLeaderboardCache loaded = {};
		const size_t read = fread(&loaded, 1, sizeof(loaded), f);
		fclose(f);

		if (read != sizeof(loaded))
			return false;

		if (loaded.magic != kLeaderboardCacheMagic || loaded.version != kLeaderboardCacheVersion)
			return false;

		outCache = loaded;
		return true;
	}

	static void SaveLeaderboardCache(const PersistedLeaderboardCache& cache)
	{
		char filePath[MAX_PATH] = {};
		BuildLeaderboardCachePath(filePath, MAX_PATH);

		FILE* f = nullptr;
		if (fopen_s(&f, filePath, "wb") != 0 || f == nullptr)
			return;

		fwrite(&cache, 1, sizeof(cache), f);
		fclose(f);
	}

	static int GetPersistedRowIndex(LeaderboardManager::EStatsType type, unsigned int difficulty)
	{
		if (type < 0 || type >= LeaderboardManager::eStatsType_MAX)
			return -1;

		unsigned int clampedDifficulty = (difficulty > 3) ? 3 : difficulty;
		if (type == LeaderboardManager::eStatsType_Kills && clampedDifficulty == 0)
			clampedDifficulty = 1;

		return static_cast<int>(type) * static_cast<int>(kDifficultyCount) + static_cast<int>(clampedDifficulty);
	}

	static void RecomputeTotalScore(LeaderboardManager::ReadScore& score)
	{
		unsigned long long total = 0;
		const unsigned int columnCount = std::min<unsigned int>(
			static_cast<unsigned int>(score.m_statsSize),
			LeaderboardManager::ReadScore::STATSDATA_MAX);

		for (unsigned int i = 0; i < columnCount; ++i)
			total += score.m_statsData[i];

		score.m_totalScore = ClampToULong(total);
	}

	static bool ScoreHasAnyData(const LeaderboardManager::ReadScore& score)
	{
		if (score.m_totalScore > 0)
			return true;

		const unsigned int columnCount = std::min<unsigned int>(
			static_cast<unsigned int>(score.m_statsSize),
			LeaderboardManager::ReadScore::STATSDATA_MAX);

		for (unsigned int i = 0; i < columnCount; ++i)
		{
			if (score.m_statsData[i] > 0)
				return true;
		}

		return false;
	}

	static void ApplyPersistedRow(const PersistedLeaderboardRow& row, LeaderboardManager::ReadScore& outScore)
	{
		if (row.valid == 0)
			return;

		ZeroMemory(&outScore.m_statsData, sizeof(outScore.m_statsData));

		outScore.m_uid = static_cast<PlayerUID>(row.uid);
		outScore.m_rank = 1;
		outScore.m_idsErrorMessage = 0;
		outScore.m_statsSize = static_cast<unsigned short>(std::min<unsigned int>(
			static_cast<unsigned int>(row.statsSize),
			LeaderboardManager::ReadScore::STATSDATA_MAX));
		outScore.m_totalScore = row.totalScore;
		outScore.m_name = row.name;

		for (unsigned int i = 0; i < outScore.m_statsSize; ++i)
			outScore.m_statsData[i] = row.statsData[i];
	}

	static void PersistRow(PersistedLeaderboardRow& row, const LeaderboardManager::ReadScore& score)
	{
		row.valid = 1;
		row.statsSize = static_cast<BYTE>(std::min<unsigned int>(
			static_cast<unsigned int>(score.m_statsSize),
			LeaderboardManager::ReadScore::STATSDATA_MAX));
		row.reserved = 0;
		row.totalScore = score.m_totalScore;
		row.uid = static_cast<ULONGLONG>(score.m_uid);

		ZeroMemory(row.statsData, sizeof(row.statsData));
		for (unsigned int i = 0; i < row.statsSize; ++i)
			row.statsData[i] = score.m_statsData[i];

		ZeroMemory(row.name, sizeof(row.name));
		if (!score.m_name.empty())
			wcsncpy_s(row.name, score.m_name.c_str(), _TRUNCATE);
	}

	static bool BuildReadScoreFromRegisterScore(const LeaderboardManager::RegisterScore& source,
		LeaderboardManager::ReadScore& outScore)
	{
		ZeroMemory(&outScore, sizeof(outScore));

		PlayerUID uid = INVALID_XUID;
		ProfileManager.GetXUID(source.m_iPad, &uid, true);
		outScore.m_uid = uid;
		outScore.m_rank = 1;
		outScore.m_idsErrorMessage = 0;
		outScore.m_name = convStringToWstring(ProfileManager.GetGamertag(source.m_iPad));

		switch (source.m_commentData.m_statsType)
		{
		case LeaderboardManager::eStatsType_Travelling:
			outScore.m_statsSize = 4;
			outScore.m_statsData[0] = source.m_commentData.m_travelling.m_walked;
			outScore.m_statsData[1] = source.m_commentData.m_travelling.m_fallen;
			outScore.m_statsData[2] = source.m_commentData.m_travelling.m_minecart;
			outScore.m_statsData[3] = source.m_commentData.m_travelling.m_boat;
			break;

		case LeaderboardManager::eStatsType_Mining:
			outScore.m_statsSize = 7;
			outScore.m_statsData[0] = source.m_commentData.m_mining.m_dirt;
			outScore.m_statsData[1] = source.m_commentData.m_mining.m_cobblestone;
			outScore.m_statsData[2] = source.m_commentData.m_mining.m_sand;
			outScore.m_statsData[3] = source.m_commentData.m_mining.m_stone;
			outScore.m_statsData[4] = source.m_commentData.m_mining.m_gravel;
			outScore.m_statsData[5] = source.m_commentData.m_mining.m_clay;
			outScore.m_statsData[6] = source.m_commentData.m_mining.m_obsidian;
			break;

		case LeaderboardManager::eStatsType_Farming:
			outScore.m_statsSize = 6;
			outScore.m_statsData[0] = source.m_commentData.m_farming.m_eggs;
			outScore.m_statsData[1] = source.m_commentData.m_farming.m_wheat;
			outScore.m_statsData[2] = source.m_commentData.m_farming.m_mushroom;
			outScore.m_statsData[3] = source.m_commentData.m_farming.m_sugarcane;
			outScore.m_statsData[4] = source.m_commentData.m_farming.m_milk;
			outScore.m_statsData[5] = source.m_commentData.m_farming.m_pumpkin;
			break;

		case LeaderboardManager::eStatsType_Kills:
			outScore.m_statsSize = 7;
			outScore.m_statsData[0] = source.m_commentData.m_kills.m_zombie;
			outScore.m_statsData[1] = source.m_commentData.m_kills.m_skeleton;
			outScore.m_statsData[2] = source.m_commentData.m_kills.m_creeper;
			outScore.m_statsData[3] = source.m_commentData.m_kills.m_spider;
			outScore.m_statsData[4] = source.m_commentData.m_kills.m_spiderJockey;
			outScore.m_statsData[5] = source.m_commentData.m_kills.m_zombiePigman;
			outScore.m_statsData[6] = source.m_commentData.m_kills.m_slime;
			break;

		default:
			return false;
		}

		RecomputeTotalScore(outScore);
		if (source.m_score > 0)
			outScore.m_totalScore = std::max<unsigned long>(outScore.m_totalScore, static_cast<unsigned long>(source.m_score));

		return true;
	}
}

LeaderboardManager *LeaderboardManager::m_instance = new WindowsLeaderboardManager(); // Singleton instance of the LeaderboardManager

bool WindowsLeaderboardManager::WriteStats(unsigned int viewCount, ViewIn views)
{
	PersistedLeaderboardCache cache = {};
	LoadLeaderboardCache(cache);

	if (views != nullptr)
	{
		for (unsigned int i = 0; i < viewCount; ++i)
		{
			ReadScore score = {};
			if (!BuildReadScoreFromRegisterScore(views[i], score))
				continue;

			unsigned int difficulty = static_cast<unsigned int>(ClampDifficulty(views[i].m_difficulty));
			if (views[i].m_commentData.m_statsType == eStatsType_Kills && difficulty == 0)
				difficulty = 1;

			const int index = GetPersistedRowIndex(views[i].m_commentData.m_statsType, difficulty);
			if (index < 0 || index >= static_cast<int>(kPersistedBoardCount))
				continue;

			PersistRow(cache.rows[index], score);
		}

		delete[] views;
	}

	SaveLeaderboardCache(cache);
	return true;
}

bool WindowsLeaderboardManager::ReadStats_Friends(LeaderboardReadListener* callback, int difficulty,
	EStatsType type, PlayerUID myUID, unsigned int startIndex, unsigned int readCount)
{
	if (!LeaderboardManager::ReadStats_Friends(callback, difficulty, type, myUID, startIndex, readCount))
		return false;

	return ReadLocalStats(callback, difficulty, type, myUID);
}

bool WindowsLeaderboardManager::ReadStats_MyScore(LeaderboardReadListener* callback, int difficulty,
	EStatsType type, PlayerUID myUID, unsigned int readCount)
{
	if (!LeaderboardManager::ReadStats_MyScore(callback, difficulty, type, myUID, readCount))
		return false;

	return ReadLocalStats(callback, difficulty, type, myUID);
}

bool WindowsLeaderboardManager::ReadStats_TopRank(LeaderboardReadListener* callback, int difficulty,
	EStatsType type, unsigned int startIndex, unsigned int readCount)
{
	if (!LeaderboardManager::ReadStats_TopRank(callback, difficulty, type, startIndex, readCount))
		return false;

	PlayerUID uid = INVALID_XUID;
	ProfileManager.GetXUID(ProfileManager.GetPrimaryPad(), &uid, true);
	return ReadLocalStats(callback, difficulty, type, uid);
}

bool WindowsLeaderboardManager::ReadLocalStats(LeaderboardReadListener* callback, int difficulty,
	EStatsType type, PlayerUID uid)
{
	if (callback == nullptr)
		return false;

	ReadView view = {};
	ReadScore row = {};

	if (BuildLocalReadScore(row, difficulty, type, uid))
	{
		view.m_numQueries = 1;
		view.m_queries = &row;

		callback->OnStatsReadComplete(eStatsReturn_Success, 1, view);
	}
	else
	{
		view.m_numQueries = 0;
		view.m_queries = nullptr;

		callback->OnStatsReadComplete(eStatsReturn_NoResults, 0, view);
	}

	return true;
}

bool WindowsLeaderboardManager::BuildLocalReadScore(ReadScore& outScore, int difficulty,
	EStatsType type, PlayerUID uid)
{
	const int primaryPad = ProfileManager.GetPrimaryPad();
	if (primaryPad < 0 || primaryPad >= XUSER_MAX_COUNT)
		return false;

	Minecraft* minecraft = Minecraft::GetInstance();
	if (minecraft == nullptr)
		return false;

	StatsCounter* stats = minecraft->stats[primaryPad];
	if (stats == nullptr)
		return false;

	ZeroMemory(&outScore, sizeof(outScore));

	outScore.m_uid = uid;
	outScore.m_rank = 1;
	outScore.m_idsErrorMessage = 0;

	char* gamertag = ProfileManager.GetGamertag(primaryPad);
	if (gamertag != nullptr && gamertag[0] != 0)
		outScore.m_name = convStringToWstring(gamertag);
	else
		outScore.m_name = L"Player";

	unsigned int diff = static_cast<unsigned int>(ClampDifficulty(difficulty));
	if (type == eStatsType_Kills && diff == 0)
		diff = 1;
	unsigned long long totalScore = 0;

	auto setColumn = [&](unsigned int index, unsigned int value)
	{
		if (index >= ReadScore::STATSDATA_MAX)
			return;

		outScore.m_statsData[index] = value;
		totalScore += value;
	};

	switch (type)
	{
	case eStatsType_Travelling:
		outScore.m_statsSize = 4;
		setColumn(0, stats->getValue(Stats::walkOneM, diff));
		setColumn(1, stats->getValue(Stats::fallOneM, diff));
		setColumn(2, stats->getValue(Stats::minecartOneM, diff));
		setColumn(3, stats->getValue(Stats::boatOneM, diff));
		break;

	case eStatsType_Mining:
		outScore.m_statsSize = 7;
		setColumn(0, stats->getValue(Stats::blocksMined[Tile::dirt_Id], diff));
		setColumn(1, stats->getValue(Stats::blocksMined[Tile::cobblestone_Id], diff));
		setColumn(2, stats->getValue(Stats::blocksMined[Tile::sand_Id], diff));
		setColumn(3, stats->getValue(Stats::blocksMined[Tile::stone_Id], diff));
		setColumn(4, stats->getValue(Stats::blocksMined[Tile::gravel_Id], diff));
		setColumn(5, stats->getValue(Stats::blocksMined[Tile::clay_Id], diff));
		setColumn(6, stats->getValue(Stats::blocksMined[Tile::obsidian_Id], diff));
		break;

	case eStatsType_Farming:
		outScore.m_statsSize = 6;
		setColumn(0, stats->getValue(Stats::itemsCollected[Item::egg_Id], diff));
		setColumn(1, stats->getValue(Stats::blocksMined[Tile::wheat_Id], diff));
		setColumn(2, stats->getValue(Stats::blocksMined[Tile::mushroom_brown_Id], diff));
		setColumn(3, stats->getValue(Stats::blocksMined[Tile::reeds_Id], diff));
		setColumn(4, stats->getValue(Stats::cowsMilked, diff));
		setColumn(5, stats->getValue(Stats::itemsCollected[Tile::pumpkin_Id], diff));
		break;

	case eStatsType_Kills:
		outScore.m_statsSize = 7;
		setColumn(0, stats->getValue(Stats::killsZombie, diff));
		setColumn(1, stats->getValue(Stats::killsSkeleton, diff));
		setColumn(2, stats->getValue(Stats::killsCreeper, diff));
		setColumn(3, stats->getValue(Stats::killsSpider, diff));
		setColumn(4, stats->getValue(Stats::killsSpiderJockey, diff));
		setColumn(5, CombinePigmanKills(stats, diff));
		setColumn(6, stats->getValue(Stats::killsSlime, diff));
		break;

	default:
		return false;
	}

	outScore.m_totalScore = ClampToULong(totalScore);

	PersistedLeaderboardCache cache = {};
	LoadLeaderboardCache(cache);

	const int persistedIndex = GetPersistedRowIndex(type, diff);
	if (persistedIndex >= 0 && persistedIndex < static_cast<int>(kPersistedBoardCount))
	{
		PersistedLeaderboardRow& persistedRow = cache.rows[persistedIndex];
		const bool currentHasData = ScoreHasAnyData(outScore);

		if (persistedRow.valid != 0)
		{
			if (!currentHasData)
			{
				ApplyPersistedRow(persistedRow, outScore);
			}
			else if (persistedRow.statsSize == outScore.m_statsSize)
			{
				const unsigned int columnCount = std::min<unsigned int>(
					static_cast<unsigned int>(outScore.m_statsSize),
					ReadScore::STATSDATA_MAX);

				for (unsigned int i = 0; i < columnCount; ++i)
					outScore.m_statsData[i] = (std::max)(outScore.m_statsData[i], persistedRow.statsData[i]);

				RecomputeTotalScore(outScore);

				if (outScore.m_name.empty() && persistedRow.name[0] != 0)
					outScore.m_name = persistedRow.name;
			}
		}

		PersistRow(persistedRow, outScore);
		SaveLeaderboardCache(cache);
	}

	return true;
}
