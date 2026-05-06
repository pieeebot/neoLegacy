#pragma once
#include "Tile.h"

class HalfSlabTile : public Tile
{
public:
    static const int TYPE_MASK    = 7;
    static const int TOP_SLOT_BIT = 8;

    
    enum class Half
    {
        TOP,
        BOTTOM
    };

public:
    HalfSlabTile(int id, Material *material); 

    virtual void DerivedInit(); 

    virtual int  isFullSize() = 0;

    virtual void createBlockStateDefinition() override;
    virtual int  defaultBlockState() override;
    virtual int  convertBlockStateToLegacyData(BlockState *state) override;
    virtual Tile::BlockState getBlockState(LevelSource *level, int x, int y, int z) override;
    virtual Tile::BlockState getBlockState(int data);

    virtual void updateShape(
        LevelSource *level, int x, int y, int z,
        int forceData = -1,
        shared_ptr<TileEntity> forceEntity = shared_ptr<TileEntity>()) override;

    virtual void updateDefaultShape() override;

    virtual void addAABBs(
        Level *level, int x, int y, int z,
        AABB *box, AABBList *boxes,
        shared_ptr<Entity> source) override;

    virtual bool isSolidRender(bool isServerLevel = false) override;
    virtual bool isSilkTouchable() override; 

    virtual int  getPlacedOnFaceDataValue(
        Level *level, int x, int y, int z,
        int face, float clickX, float clickY, float clickZ,
        int itemValue) override;

    virtual int  getResourceCount(Random *random) override;
    virtual int  getSpawnResourcesAuxValue(int data) override;
    virtual bool isCubeShaped() override;
    virtual bool shouldRenderFace(
        LevelSource *level, int x, int y, int z, int face) override;

    virtual int  cloneTileData(Level *level, int x, int y, int z) override;
    virtual int  cloneTileId(Level *level, int x, int y, int z) override;

    virtual int  getAuxName(int auxValue) = 0; 

private:
    static bool isHalfSlab(int tileId);
};