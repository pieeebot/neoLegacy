#pragma once
#include "Tile.h"

class Random;

class StoneTile : public Tile
{
public:
	static const int TYPE_DEFAULT = 0;
	static const int GRANITE = 1;
	static const int POLISHED_GRANITE = 2;
	static const int DIORITE = 3;
	static const int POLISHED_DIORITE = 4;
	static const int ANDESITE = 5;
	static const int POLISHED_ANDESITE = 6;

	static const int STONE_NAMES_LENGTH = 7;

	static const unsigned int STONE_DESCRIPTIONS[STONE_NAMES_LENGTH];
	static const unsigned int STONE_NAMES[STONE_NAMES_LENGTH];
	static const wstring TEXTURE_NAMES[];
private:
	Icon** icons;
public:
	StoneTile(int id);
	virtual Icon* getTexture(int face, int data);
	virtual unsigned int getDescriptionId(int iData = -1);
	virtual int getResource(int data, Random* random, int playerBonusLevel);
	virtual int getSpawnResourcesAuxValue(int data);
	void registerIcons(IconRegister* iconRegister);
};