#include "stdafx.h"

#include "WindowsLeaderboardManager.h"

#include <algorithm>
#include <vector>

#include "../../Minecraft.h"
#include "../../StatsCounter.h"

#include "../../../Minecraft.World/StringHelpers.h"
#include "../../../Minecraft.World/Stats.h"
#include "../../../Minecraft.World/net.minecraft.world.item.h"
#include "../../../Minecraft.World/net.minecraft.world.level.tile.h"

namespace
{
	static const DWORD kLeaderboardCacheMagic = 0x31424C57; // file identifier: WLB1
	static const DWORD kLeaderboardCacheVersion = 2;
	static const unsigned int kDifficultyCount = 4;
	static const unsigned int kPersistedBoardCount =
		static_cast<unsigned int>(LeaderboardManager::eStatsType_MAX) * kDifficultyCount;
	static const unsigned int kMaxRowsPerBoard = 64;

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

	struct PersistedLeaderboardBoard
	{
		DWORD rowCount;
		PersistedLeaderboardRow rows[kMaxRowsPerBoard];
	};

	struct PersistedLeaderboardCache
	{
		DWORD magic;
		DWORD version;
		PersistedLeaderboardBoard boards[kPersistedBoardCount];
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

		if (fseek(f, 0, SEEK_END) != 0)
		{
			fclose(f);
			return false;
		}

		const long fileSize = ftell(f);
		if (fileSize <= 0 || fseek(f, 0, SEEK_SET) != 0)
		{
			fclose(f);
			return false;
		}

		if (static_cast<size_t>(fileSize) == sizeof(PersistedLeaderboardCache))
		{
			PersistedLeaderboardCache loaded = {};
			const size_t read = fread(&loaded, 1, sizeof(loaded), f);
			fclose(f);

			if (read != sizeof(loaded))
				return false;

			if (loaded.magic != kLeaderboardCacheMagic || loaded.version != kLeaderboardCacheVersion)
				return false;

			outCache = loaded;
			for (unsigned int boardIndex = 0; boardIndex < kPersistedBoardCount; ++boardIndex)
			{
				if (outCache.boards[boardIndex].rowCount > kMaxRowsPerBoard)
					outCache.boards[boardIndex].rowCount = kMaxRowsPerBoard;
			}

			return true;
		}

		fclose(f);
		return false;
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

	static int GetPersistedBoardIndex(LeaderboardManager::EStatsType type, unsigned int difficulty)
	{
		if (type < 0 || type >= LeaderboardManager::eStatsType_MAX)
			return -1;

		unsigned int clampedDifficulty = (difficulty > 3) ? 3 : difficulty;
		if (type == LeaderboardManager::eStatsType_Kills && clampedDifficulty == 0)
			clampedDifficulty = 1;

		return static_cast<int>(type) * static_cast<int>(kDifficultyCount) + static_cast<int>(clampedDifficulty);
	}

	static unsigned int GetBoardRowCount(const PersistedLeaderboardBoard& board)
	{
		return (board.rowCount > kMaxRowsPerBoard) ? kMaxRowsPerBoard : board.rowCount;
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

	static int FindMatchingRowIndex(const PersistedLeaderboardBoard& board, const LeaderboardManager::ReadScore& score)
	{
		const unsigned int rowCount = GetBoardRowCount(board);

		if (!score.m_name.empty())
		{
			for (unsigned int rowIndex = 0; rowIndex < rowCount; ++rowIndex)
			{
				const PersistedLeaderboardRow& row = board.rows[rowIndex];
				if (row.valid == 0 || row.name[0] == 0)
					continue;

				if (_wcsicmp(row.name, score.m_name.c_str()) == 0)
					return static_cast<int>(rowIndex);
			}

			return -1;
		}

		if (score.m_uid != INVALID_XUID)
		{
			for (unsigned int rowIndex = 0; rowIndex < rowCount; ++rowIndex)
			{
				const PersistedLeaderboardRow& row = board.rows[rowIndex];
				if (row.valid == 0)
					continue;

				if (static_cast<PlayerUID>(row.uid) == score.m_uid)
					return static_cast<int>(rowIndex);
			}
		}

		return -1;
	}

	static int FindWritableRowIndex(PersistedLeaderboardBoard& board, const LeaderboardManager::ReadScore& score)
	{
		if (board.rowCount > kMaxRowsPerBoard)
			board.rowCount = kMaxRowsPerBoard;

		const int matchingRowIndex = FindMatchingRowIndex(board, score);
		if (matchingRowIndex >= 0)
			return matchingRowIndex;

		const unsigned int rowCount = GetBoardRowCount(board);
		for (unsigned int rowIndex = 0; rowIndex < rowCount; ++rowIndex)
		{
			if (board.rows[rowIndex].valid == 0)
				return static_cast<int>(rowIndex);
		}

		if (rowCount < kMaxRowsPerBoard)
		{
			board.rowCount = rowCount + 1;
			return static_cast<int>(rowCount);
		}

		unsigned int replaceIndex = 0;
		DWORD lowestScore = ULONG_MAX;
		for (unsigned int rowIndex = 0; rowIndex < kMaxRowsPerBoard; ++rowIndex)
		{
			if (board.rows[rowIndex].valid == 0)
				return static_cast<int>(rowIndex);

			if (board.rows[rowIndex].totalScore <= lowestScore)
			{
				lowestScore = board.rows[rowIndex].totalScore;
				replaceIndex = rowIndex;
			}
		}

		return static_cast<int>(replaceIndex);
	}

	static void MergeScoreWithPersistedRow(const PersistedLeaderboardRow& persistedRow, LeaderboardManager::ReadScore& inOutScore)
	{
		if (persistedRow.valid == 0)
			return;

		const bool currentHasData = ScoreHasAnyData(inOutScore);
		if (!currentHasData)
		{
			ApplyPersistedRow(persistedRow, inOutScore);
			return;
		}

		if (persistedRow.statsSize == inOutScore.m_statsSize)
		{
			const unsigned int columnCount = std::min<unsigned int>(
				static_cast<unsigned int>(inOutScore.m_statsSize),
				LeaderboardManager::ReadScore::STATSDATA_MAX);

			for (unsigned int i = 0; i < columnCount; ++i)
				inOutScore.m_statsData[i] = (std::max)(inOutScore.m_statsData[i], persistedRow.statsData[i]);

			RecomputeTotalScore(inOutScore);
		}

		if (inOutScore.m_name.empty() && persistedRow.name[0] != 0)
			inOutScore.m_name = persistedRow.name;
	}

	static bool RowMatchesPlayer(const LeaderboardManager::ReadScore& row, const LeaderboardManager::ReadScore& player)
	{
		if (!player.m_name.empty() && !row.m_name.empty())
			return _wcsicmp(row.m_name.c_str(), player.m_name.c_str()) == 0;

		if (player.m_uid != INVALID_XUID)
			return row.m_uid == player.m_uid;

		return false;
	}

	static bool CompareRowsForRank(const LeaderboardManager::ReadScore& left, const LeaderboardManager::ReadScore& right)
	{
		if (left.m_totalScore != right.m_totalScore)
			return left.m_totalScore > right.m_totalScore;

		const wchar_t* leftName = left.m_name.empty() ? L"" : left.m_name.c_str();
		const wchar_t* rightName = right.m_name.empty() ? L"" : right.m_name.c_str();
		const int nameCompare = _wcsicmp(leftName, rightName);
		if (nameCompare != 0)
			return nameCompare < 0;

		return left.m_uid < right.m_uid;
	}

	static void CollectScoresFromBoard(const PersistedLeaderboardBoard& board,
		std::vector<LeaderboardManager::ReadScore>& outScores)
	{
		const unsigned int rowCount = GetBoardRowCount(board);
		outScores.reserve(outScores.size() + rowCount);

		for (unsigned int rowIndex = 0; rowIndex < rowCount; ++rowIndex)
		{
			const PersistedLeaderboardRow& row = board.rows[rowIndex];
			if (row.valid == 0)
				continue;

			LeaderboardManager::ReadScore score = {};
			ApplyPersistedRow(row, score);
			if (score.m_statsSize == 0)
				continue;

			outScores.push_back(score);
		}
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

		char* gamertag = ProfileManager.GetGamertag(source.m_iPad);
		if (gamertag != nullptr && gamertag[0] != 0)
			outScore.m_name = convStringToWstring(gamertag);
		else
			outScore.m_name = L"Player";

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

			const int boardIndex = GetPersistedBoardIndex(views[i].m_commentData.m_statsType, difficulty);
			if (boardIndex < 0 || boardIndex >= static_cast<int>(kPersistedBoardCount))
				continue;

			PersistedLeaderboardBoard& board = cache.boards[boardIndex];
			const int rowIndex = FindWritableRowIndex(board, score);
			if (rowIndex < 0 || rowIndex >= static_cast<int>(kMaxRowsPerBoard))
				continue;

			PersistRow(board.rows[rowIndex], score);
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
	ReadScore localScore = {};

	if (!BuildLocalReadScore(localScore, difficulty, type, uid))
	{
		view.m_numQueries = 0;
		view.m_queries = nullptr;
		callback->OnStatsReadComplete(eStatsReturn_NoResults, 0, view);
		return true;
	}

	unsigned int diff = static_cast<unsigned int>(ClampDifficulty(difficulty));
	if (type == eStatsType_Kills && diff == 0)
		diff = 1;

	std::vector<ReadScore> allRows;
	PersistedLeaderboardCache cache = {};
	LoadLeaderboardCache(cache);

	const int boardIndex = GetPersistedBoardIndex(type, diff);
	if (boardIndex >= 0 && boardIndex < static_cast<int>(kPersistedBoardCount))
		CollectScoresFromBoard(cache.boards[boardIndex], allRows);

	bool hasLocalRow = false;
	for (const ReadScore& row : allRows)
	{
		if (RowMatchesPlayer(row, localScore))
		{
			hasLocalRow = true;
			break;
		}
	}

	if (!hasLocalRow)
		allRows.push_back(localScore);

	if (allRows.empty())
	{
		view.m_numQueries = 0;
		view.m_queries = nullptr;
		callback->OnStatsReadComplete(eStatsReturn_NoResults, 0, view);
		return true;
	}

	std::sort(allRows.begin(), allRows.end(), CompareRowsForRank);
	for (size_t i = 0; i < allRows.size(); ++i)
		allRows[i].m_rank = static_cast<unsigned long>(i + 1);

	size_t playerIndex = 0;
	for (size_t i = 0; i < allRows.size(); ++i)
	{
		if (RowMatchesPlayer(allRows[i], localScore))
		{
			playerIndex = i;
			break;
		}
	}

	size_t pageStart = 0;
	size_t pageCount = allRows.size();

	if (m_eFilterMode == eFM_MyScore)
	{
		pageStart = playerIndex;
		pageCount = 1;
	}
	else if (m_eFilterMode == eFM_TopRank)
	{
		if (m_startIndex > 0)
			pageStart = std::min<size_t>(static_cast<size_t>(m_startIndex - 1), allRows.size());

		pageCount = allRows.size() - pageStart;
		if (m_readCount > 0)
			pageCount = std::min<size_t>(pageCount, static_cast<size_t>(m_readCount));
	}

	std::vector<ReadScore> pageRows;
	if (pageStart < allRows.size() && pageCount > 0)
	{
		pageRows.insert(
			pageRows.end(),
			allRows.begin() + static_cast<std::ptrdiff_t>(pageStart),
			allRows.begin() + static_cast<std::ptrdiff_t>(pageStart + pageCount));
	}

	if (pageRows.empty())
	{
		view.m_numQueries = 0;
		view.m_queries = nullptr;
		callback->OnStatsReadComplete(eStatsReturn_NoResults, 0, view);
		return true;
	}

	view.m_numQueries = static_cast<unsigned int>(pageRows.size());
	view.m_queries = pageRows.data();
	const int totalResults = (m_eFilterMode == eFM_MyScore)
		? static_cast<int>(pageRows.size())
		: static_cast<int>(allRows.size());
	callback->OnStatsReadComplete(
		eStatsReturn_Success,
		totalResults,
		view);

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

	const int boardIndex = GetPersistedBoardIndex(type, diff);
	if (boardIndex >= 0 && boardIndex < static_cast<int>(kPersistedBoardCount))
	{
		PersistedLeaderboardBoard& board = cache.boards[boardIndex];

		const int matchingRowIndex = FindMatchingRowIndex(board, outScore);
		if (matchingRowIndex >= 0 && matchingRowIndex < static_cast<int>(kMaxRowsPerBoard))
			MergeScoreWithPersistedRow(board.rows[matchingRowIndex], outScore);

		const int writeRowIndex = FindWritableRowIndex(board, outScore);
		if (writeRowIndex >= 0 && writeRowIndex < static_cast<int>(kMaxRowsPerBoard))
			PersistRow(board.rows[writeRowIndex], outScore);

		SaveLeaderboardCache(cache);
	}

	return true;
}
