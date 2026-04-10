#include "stdafx.h"
#include "ChunkPrimer.h"
#include "Tile.h"          // for Tile::air (or your air tile)


ChunkPrimer::ChunkPrimer()
{
    m_blockIds = new byte[65536];
    m_blockData = new byte[32768];
    memset(m_blockIds, 0, 65536);
    memset(m_blockData, 0, 32768);
    m_airBlockId = 0;  // adjust if your air tile has a different ID
}

ChunkPrimer::~ChunkPrimer()
{
    delete[] m_blockIds;
    delete[] m_blockData;
}

int ChunkPrimer::getIndex(int x, int y, int z)
{
    // Match the indexing used in the decompiled code: (x << 12) | (z << 8) | y
    return (x << 12) | (z << 8) | y;
}

void ChunkPrimer::setBlockAndData(int packedPos, int blockId, int data)
{
    if (packedPos < 0 || packedPos >= 65536)
    {
        // Handle error (log, assert, etc.)
        return;
    }
    m_blockIds[packedPos] = static_cast<byte>(blockId);
    int byteIdx = packedPos >> 1;
    if (packedPos & 1)
        m_blockData[byteIdx] = (m_blockData[byteIdx] & 0x0F) | ((data & 0x0F) << 4);
    else
        m_blockData[byteIdx] = (m_blockData[byteIdx] & 0xF0) | (data & 0x0F);
}

int ChunkPrimer::getState(int packedPos) const
{
    if (packedPos < 0 || packedPos >= 65536)
        return 0;  // air
    int id = m_blockIds[packedPos];
    int byteIdx = packedPos >> 1;
    int data;
    if (packedPos & 1)
        data = (m_blockData[byteIdx] >> 4) & 0x0F;
    else
        data = m_blockData[byteIdx] & 0x0F;
    // Return a packed state (can be used in place of BlockState* for material checks)
    return (id << 4) | data;
}

int ChunkPrimer::getBlockId(int packedPos) const
{
    if (packedPos < 0 || packedPos >= 65536)
        return 0;
    return m_blockIds[packedPos];
}

int ChunkPrimer::getBlockData(int packedPos) const
{
    if (packedPos < 0 || packedPos >= 65536)
        return 0;
    int byteIdx = packedPos >> 1;
    if (packedPos & 1)
        return (m_blockData[byteIdx] >> 4) & 0x0F;
    else
        return m_blockData[byteIdx] & 0x0F;
}

int ChunkPrimer::getHighestNonAirPos(int x, int z) const
{
    // We iterate from top (y=127) down to 0, checking the block ID.
    // The decompiled code uses: (x << 11) | (z << 7) | 1 + y + 126? 
    // In the decompiled version, they had:
    //   base = (x << 11) | (z << 7) | 1;
    //   for (int y = 127; y >= 0; y--)
    //       if (Block::byId(m_blockIds[base + y + 126]) != Blocks::AIR)
    //           return y;
    // We'll use the same indexing: linear index = (x << 12) | (z << 8) | y.
    // To find the topmost non‑air block for given x,z, we need to check all y.
    // We can compute the base offset for this column: (x << 12) | (z << 8)
    int base = (x << 12) | (z << 8);
    for (int y = 127; y >= 0; --y)
    {
        int idx = base | y;
        if (m_blockIds[idx] != m_airBlockId)
            return y;
    }
    return 0;
}

void ChunkPrimer::setBlockAndData(int x, int y, int z, int blockId, int data)
{
    setBlockAndData(getIndex(x, y, z), blockId, data);
}

int ChunkPrimer::getBlockId(int x, int y, int z) const
{
    return getBlockId(getIndex(x, y, z));
}

int ChunkPrimer::getState(int x, int y, int z) const
{
    return getState(getIndex(x, y, z));
}