#pragma once

#include "Common/Leaderboards/LeaderboardManager.h"

class WindowsLeaderboardManager : public LeaderboardManager
{
public:
	virtual void Tick() override {}

	//Open a session
	virtual bool OpenSession() override { return true; }

	//Close a session
	virtual void CloseSession() override {}

	//Delete a session
	virtual void DeleteSession() override {}

	//Write the given stats
	//This is called synchronously and will not free any memory allocated for views when it is done
	virtual bool WriteStats(unsigned int viewCount, ViewIn views) override;

	virtual bool ReadStats_Friends(LeaderboardReadListener* callback, int difficulty, EStatsType type,
		PlayerUID myUID, unsigned int startIndex, unsigned int readCount) override;
	virtual bool ReadStats_MyScore(LeaderboardReadListener* callback, int difficulty, EStatsType type,
		PlayerUID myUID, unsigned int readCount) override;
	virtual bool ReadStats_TopRank(LeaderboardReadListener* callback, int difficulty, EStatsType type,
		unsigned int startIndex, unsigned int readCount) override;

	//Perform a flush of the stats
	virtual void FlushStats() override {}

	//Cancel the current operation
	virtual void CancelOperation() override {}

	//Is the leaderboard manager idle.
	virtual bool isIdle() override { return true; }

private:
	bool ReadLocalStats(LeaderboardReadListener* callback, int difficulty, EStatsType type, PlayerUID uid);
	bool BuildLocalReadScore(ReadScore& outScore, int difficulty, EStatsType type, PlayerUID uid);
};
