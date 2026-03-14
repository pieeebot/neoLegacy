#pragma once
#include "Tile.h"

class Random;
class Level;
class FallingTile;

class SandTile : public Tile
{
public:
	static bool instaFall;

	static const int RED_SAND = 1;

	static const int SAND_NAMES_LENGTH = 2;

	static const unsigned int SAND_NAMES[SAND_NAMES_LENGTH];
	static const wstring TEXTURE_NAMES[];

	SandTile(int type, bool isSolidRender = true);
	virtual void onPlace(Level* level, int x, int y, int z);
	virtual void neighborChanged(Level* level, int x, int y, int z, int type);
	virtual void tick(Level* level, int x, int y, int z, Random* random);
private:
	Icon** icons;
	void checkSlide(Level* level, int x, int y, int z);
protected:
	virtual void falling(shared_ptr<FallingTile> entity);
public:
	virtual int getTickDelay(Level* level);
	static bool isFree(Level* level, int x, int y, int z);
	virtual int getSpawnResourcesAuxValue(int data);
	virtual void onLand(Level* level, int xt, int yt, int zt, int data);
	virtual Icon* getTexture(int face, int data);
	void registerIcons(IconRegister* iconRegister);
};
