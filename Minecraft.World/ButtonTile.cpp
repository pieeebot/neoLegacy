#include "stdafx.h"
#include "net.minecraft.world.entity.projectile.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.level.redstone.h"
#include "net.minecraft.world.phys.h"
#include "net.minecraft.h"
#include "ButtonTile.h"
#include "SoundTypes.h"

ButtonTile::ButtonTile(int id, bool sensitive) : Tile(id, Material::decoration, isSolidRender())
{
    this->setTicking(true);
    this->sensitive = sensitive;
}

void ButtonTile::createBlockStateDefinition()
{
    if (!m_blockStateDefinition)
        m_blockStateDefinition = new BlockStateDefinition(this);
}

int ButtonTile::defaultBlockState()
{
    return 0;
}

int ButtonTile::convertBlockStateToLegacyData(BlockState *state)
{
    return state ? (state->value & 0xF) : 0;
}

Tile::BlockState ButtonTile::getBlockState(int data)
{
    return Tile::BlockState(data & 0xF);
}

Tile::BlockState ButtonTile::getBlockState(LevelSource *level, int x, int y, int z)
{
    return Tile::BlockState(level->getData(x, y, z) & 0xF);
}

Icon *ButtonTile::getTexture(int face, int data)
{
    if(id == Tile::wooden_button_Id) return Tile::wood->getTexture(Facing::UP);
    else return Tile::stone->getTexture(Facing::UP);
}

AABB *ButtonTile::getAABB(Level *level, int x, int y, int z)
{
    return nullptr;
}

int ButtonTile::getTickDelay(Level *level)
{
    return sensitive ? 30 : 20;
}

bool ButtonTile::blocksLight()
{
    return false;
}

bool ButtonTile::isSolidRender(bool isServerLevel)
{
    return false;
}

bool ButtonTile::isCubeShaped()
{
    return false;
}

bool ButtonTile::mayPlace(Level *level, int x, int y, int z, int face)
{
    // face: 0=bottom, 1=top, 2=north, 3=south, 4=west, 5=east
    if (face == 0 && level->isSolidBlockingTile(x, y + 1, z)) return true; // Soffitto 
    if (face == 1 && level->isSolidBlockingTile(x, y - 1, z)) return true; // Pavimento 
    if (face == 2 && level->isSolidBlockingTile(x, y, z + 1)) return true; // Nord
    if (face == 3 && level->isSolidBlockingTile(x, y, z - 1)) return true; // Sud
    if (face == 4 && level->isSolidBlockingTile(x + 1, y, z)) return true; // Ovest
    if (face == 5 && level->isSolidBlockingTile(x - 1, y, z)) return true; // Est
    return false;
}

bool ButtonTile::mayPlace(Level *level, int x, int y, int z)
{
    if (level->isSolidBlockingTile(x - 1, y, z)) return true;
    if (level->isSolidBlockingTile(x + 1, y, z)) return true;
    if (level->isSolidBlockingTile(x, y, z - 1)) return true;
    if (level->isSolidBlockingTile(x, y, z + 1)) return true;
    if (level->isSolidBlockingTile(x, y - 1, z)) return true;
    if (level->isSolidBlockingTile(x, y + 1, z)) return true;
    return false;
}

int ButtonTile::getPlacedOnFaceDataValue(Level *level, int x, int y, int z, int face, float clickX, float clickY, float clickZ, int itemValue)
{
    int oldFlip = 0;
    int dir = 1;

    // face: 0=bottom, 1=top, 2=north, 3=south, 4=west, 5=east
    if (face == 0 && level->isSolidBlockingTile(x, y + 1, z)) dir = 5; // Soffitto
    else if (face == 1 && level->isSolidBlockingTile(x, y - 1, z)) dir = 6; // Pavimento
    else if (face == 2 && level->isSolidBlockingTile(x, y, z + 1)) dir = 4; // Nord
    else if (face == 3 && level->isSolidBlockingTile(x, y, z - 1)) dir = 3; // Sud
    else if (face == 4 && level->isSolidBlockingTile(x + 1, y, z)) dir = 2; // Ovest
    else if (face == 5 && level->isSolidBlockingTile(x - 1, y, z)) dir = 1; // Est
    else dir = findFace(level, x, y, z);

    return dir + oldFlip;
}

int ButtonTile::findFace(Level *level, int x, int y, int z)
{
    if (level->isSolidBlockingTile(x - 1, y, z)) return 1; // Est
    if (level->isSolidBlockingTile(x + 1, y, z)) return 2; // Ovest
    if (level->isSolidBlockingTile(x, y, z - 1)) return 3; // Sud
    if (level->isSolidBlockingTile(x, y, z + 1)) return 4; // Nord
    if (level->isSolidBlockingTile(x, y - 1, z)) return 5; // Soffitto
    if (level->isSolidBlockingTile(x, y + 1, z)) return 6; // Pavimento
    return 1;
}

void ButtonTile::neighborChanged(Level *level, int x, int y, int z, int type)
{
    if (checkCanSurvive(level, x, y, z))
    {
        int dir = level->getData(x, y, z) & 7;
        bool replace = false;

        if (dir == 1 && !level->isSolidBlockingTile(x - 1, y, z)) replace = true; // Est
        if (dir == 2 && !level->isSolidBlockingTile(x + 1, y, z)) replace = true; // Ovest
        if (dir == 3 && !level->isSolidBlockingTile(x, y, z - 1)) replace = true; // Sud
        if (dir == 4 && !level->isSolidBlockingTile(x, y, z + 1)) replace = true; // Nord
        if (dir == 5 && !level->isSolidBlockingTile(x, y + 1, z)) replace = true; // Soffitto
        if (dir == 6 && !level->isSolidBlockingTile(x, y - 1, z)) replace = true; // Pavimento

        if (replace)
        {
            spawnResources(level, x, y, z, level->getData(x, y, z), 0);
            level->removeTile(x, y, z);
        }
    }
}

bool ButtonTile::checkCanSurvive(Level *level, int x, int y, int z)
{
    if (!mayPlace(level, x, y, z))
    {
        this->spawnResources(level, x, y, z, level->getData(x, y, z), 0);
        level->removeTile(x, y, z);
        return false;
    }
    return true;
}

void ButtonTile::updateShape(LevelSource *level, int x, int y, int z, int forceData, shared_ptr<TileEntity> forceEntity)
{
    int data = level->getData(x, y, z);
    updateShape(data);
}

void ButtonTile::updateShape(int data)
{
    int dir = data & 7;
    bool pressed = (data & 8) > 0;

    
    float width = 3 / 16.0f;        
    float height = 4 / 16.0f;      
    float thickness = pressed ? 1 / 16.0f : 2 / 16.0f; 

    float minY = 6 / 16.0f;
    float maxY = 10 / 16.0f;

    if (dir == 1) // Est 
    {
        setShape(0, minY, 0.5f - width, thickness, maxY, 0.5f + width);
    }
    else if (dir == 2) // Ovest 
    {
        setShape(1 - thickness, minY, 0.5f - width, 1, maxY, 0.5f + width);
    }
    else if (dir == 3) // Sud 
    {
        setShape(0.5f - width, minY, 0, 0.5f + width, maxY, thickness);
    }
    else if (dir == 4) // Nord 
    {
        setShape(0.5f - width, minY, 1 - thickness, 0.5f + width, maxY, 1);
    }
    else if (dir == 5) // Soffitto 
    {
        
        setShape(0.5f - width, 1 - thickness, 0.5f - height/2, 0.5f + width, 1, 0.5f + height/2);
    }
    else if (dir == 6) // Pavimento 
    {
        
        setShape(0.5f - width, 0, 0.5f - height/2, 0.5f + width, thickness, 0.5f + height/2);
    }
}

void ButtonTile::attack(Level *level, int x, int y, int z, shared_ptr<Player> player)
{
    //use(level, x, y, z, player, 0, 0, 0, 0);
}

bool ButtonTile::TestUse()
{
    return true;
}

bool ButtonTile::use(Level *level, int x, int y, int z, shared_ptr<Player> player, int clickedFace, float clickX, float clickY, float clickZ, bool soundOnly/*=false*/)
{
    if (soundOnly)
    {
        level->playSound(x + 0.5, y + 0.5, z + 0.5, eSoundType_RANDOM_CLICK, 0.3f, 0.6f);
        return false;
    }

    int data = level->getData(x, y, z);
    int dir = data & 7;
    int open = 8 - (data & 8);
    if (open == 0) return true;

    level->setData(x, y, z, dir + open, Tile::UPDATE_ALL);
    level->setTilesDirty(x, y, z, x, y, z);

    level->playSound(x + 0.5, y + 0.5, z + 0.5, eSoundType_RANDOM_CLICK, 0.3f, 0.6f);

    updateNeighbours(level, x, y, z, dir);

    level->addToTickNextTick(x, y, z, id, getTickDelay(level));

    return true;
}

void ButtonTile::onRemove(Level *level, int x, int y, int z, int id, int data)
{
    if ((data & 8) > 0)
    {
        int dir = data & 7;
        updateNeighbours(level, x, y, z, dir);
    }
    Tile::onRemove(level, x, y, z, id, data);
}

int ButtonTile::getSignal(LevelSource *level, int x, int y, int z, int dir)
{
    return (level->getData(x, y, z) & 8) > 0 ? Redstone::SIGNAL_MAX : Redstone::SIGNAL_NONE;
}

int ButtonTile::getDirectSignal(LevelSource *level, int x, int y, int z, int dir)
{
    int data = level->getData(x, y, z);
    if ((data & 8) == 0) return Redstone::SIGNAL_NONE;
    
    
    return Redstone::SIGNAL_MAX;
}

bool ButtonTile::isSignalSource()
{
    return true;
}

void ButtonTile::tick(Level *level, int x, int y, int z, Random *random)
{
    if (level->isClientSide) return;
    int data = level->getData(x, y, z);
    if ((data & 8) == 0)
    {
        return;
    }
    if(sensitive)
    {
        checkPressed(level, x, y, z);
    }
    else
    {
        level->setData(x, y, z, data & 7, Tile::UPDATE_ALL);

        int dir = data & 7;
        updateNeighbours(level, x, y, z, dir);

        level->playSound(x + 0.5, y + 0.5, z + 0.5, eSoundType_RANDOM_CLICK, 0.3f, 0.5f);
        level->setTilesDirty(x, y, z, x, y, z);
    }
}

void ButtonTile::updateDefaultShape()
{
    float x = 3 / 16.0f;
    float y = 2 / 16.0f;
    float z = 2 / 16.0f;
    setShape(0.5f - x, 0.5f - y, 0.5f - z, 0.5f + x, 0.5f + y, 0.5f + z);
}

void ButtonTile::entityInside(Level *level, int x, int y, int z, shared_ptr<Entity> entity)
{
    if (level->isClientSide) return;
    if (!sensitive) return;

    if ((level->getData(x, y, z) & 8) != 0)
    {
        return;
    }

    checkPressed(level, x, y, z);
}

void ButtonTile::checkPressed(Level *level, int x, int y, int z)
{
    int data = level->getData(x, y, z);
    int dir = data & 7;
    bool wasPressed = (data & 8) != 0;
    bool shouldBePressed;

    updateShape(data);
    Tile::ThreadStorage *tls = static_cast<Tile::ThreadStorage *>(TlsGetValue(Tile::tlsIdxShape));
    vector<shared_ptr<Entity> > *entities = level->getEntitiesOfClass(typeid(Arrow), AABB::newTemp(x + tls->xx0, y + tls->yy0, z + tls->zz0, x + tls->xx1, y + tls->yy1, z + tls->zz1));
    shouldBePressed = !entities->empty();
    delete entities;

    if (shouldBePressed && !wasPressed)
    {
        level->setData(x, y, z, dir | 8, Tile::UPDATE_ALL);
        updateNeighbours(level, x, y, z, dir);
        level->setTilesDirty(x, y, z, x, y, z);

        level->playSound(x + 0.5, y + 0.5, z + 0.5, eSoundType_RANDOM_CLICK, 0.3f, 0.6f);
    }
    if (!shouldBePressed && wasPressed)
    {
        level->setData(x, y, z, dir, Tile::UPDATE_ALL);
        updateNeighbours(level, x, y, z, dir);
        level->setTilesDirty(x, y, z, x, y, z);

        level->playSound(x + 0.5, y + 0.5, z + 0.5, eSoundType_RANDOM_CLICK, 0.3f, 0.5f);
    }

    if (shouldBePressed)
    {
        level->addToTickNextTick(x, y, z, id, getTickDelay(level));
    }
}

void ButtonTile::updateNeighbours(Level *level, int x, int y, int z, int dir)
{
    level->updateNeighborsAt(x, y, z, id);

    if (dir == 1) // Est
    {
        level->updateNeighborsAt(x - 1, y, z, id);
    }
    else if (dir == 2) // Ovest
    {
        level->updateNeighborsAt(x + 1, y, z, id);
    }
    else if (dir == 3) // Sud
    {
        level->updateNeighborsAt(x, y, z - 1, id);
    }
    else if (dir == 4) // Nord
    {
        level->updateNeighborsAt(x, y, z + 1, id);
    }
    else if (dir == 5) // Soffitto
    {
        level->updateNeighborsAt(x, y + 1, z, id);
        
    }
    else if (dir == 6) // Pavimento
    {
        level->updateNeighborsAt(x, y - 1, z, id);
    }
}

bool ButtonTile::shouldTileTick(Level *level, int x,int y,int z)
{
    int currentData = level->getData(x, y, z);
    return (currentData & 8) != 0;
}

void ButtonTile::registerIcons(IconRegister *iconRegister)
{
    // None
}