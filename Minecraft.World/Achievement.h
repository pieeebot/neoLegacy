#pragma once
using namespace std;

#include "Stat.h"

class DescFormatter;

class Achievement : public Stat
{
public:
	const int x, y;
	Achievement *reqs;
	string iconInt;
	int nameID;
	int descID;

private: 
	const wstring desc;
    DescFormatter *descFormatter;

public:
	const shared_ptr<ItemInstance> icon;

private:
    bool isGoldenVar;
	void _init();

public:
	Achievement(int id, const wstring& name, int x, int y, Item* icon, Achievement* reqs, string iconI = "", int nameID = 0, int descID = 0);
	Achievement(int id, const wstring& name, int x, int y, Tile* icon, Achievement* reqs, string iconI = "", int nameID = 0, int descID = 0);
	Achievement(int id, const wstring& name, int x, int y, shared_ptr<ItemInstance> icon, Achievement* reqs, string iconI = "", int nameID = 0, int descID = 0);

	Achievement *setAwardLocallyOnly();
	Achievement *setGolden();
	Achievement *postConstruct();
	bool isAchievement();
	wstring getDescription();
	Achievement *setDescFormatter(DescFormatter *descFormatter);
	bool isGolden();
	int getAchievementID();
};
