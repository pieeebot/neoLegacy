#pragma once

#include "stdafx.h"   

class ChunkPrimer
{
public:
    
    ChunkPrimer();
    ~ChunkPrimer();

    
    void setBlockAndData(int packedPos, int blockId, int data);

    
    int getState(int packedPos) const;

    
    int getBlockId(int packedPos) const;

   
    int getBlockData(int packedPos) const;

    
    int getHighestNonAirPos(int x, int z) const;

    
    byte* getBlockIds() const { return m_blockIds; }
    byte* getBlockData() const { return m_blockData; }

    
    void setBlockAndData(int x, int y, int z, int blockId, int data);
    int getBlockId(int x, int y, int z) const;
    int getState(int x, int y, int z) const;

private:
    
    static int getIndex(int x, int y, int z);

    byte* m_blockIds;      
    byte* m_blockData;    
    int m_airBlockId;      
};