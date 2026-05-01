using Minecraft.Server.FourKit.Block;
using Minecraft.Server.FourKit.Entity;
using System.Runtime.InteropServices;

namespace Minecraft.Server.FourKit.Chunk;


/// <summary>
/// Represents a chunk of blocks.
/// </summary>
public class Chunk
{
    private readonly World _world;
    private readonly int _chunkX;
    private readonly int _chunkZ;

    internal Chunk(World world, int chunkX, int chunkZ)
    {
        _world = world;
        _chunkX = chunkX;
        _chunkZ = chunkZ;
    }

    /// <summary>
    /// Gets the X-coordinate of this chunk.
    /// </summary>
    /// <returns>X-coordinate.</returns>
    public int getX() => _chunkX;

    /// <summary>
    /// Gets the Z-coordinate of this chunk.
    /// </summary>
    /// <returns>Z-coordinate.</returns>
    public int getZ() => _chunkZ;

    /// <summary>
    /// Gets the world containing this chunk.
    /// </summary>
    /// <returns>Parent World.</returns>
    public World getWorld() => _world;

    /// <summary>
    /// Gets a block from this chunk.
    /// </summary>
    /// <param name="x">0-15</param>
    /// <param name="y">0-127</param>
    /// <param name="z">0-15</param>
    /// <returns>The Block.</returns>
    public Block.Block getBlock(int x, int y, int z)
    {
        return _world.getBlockAt((_chunkX << 4) + x, y, (_chunkZ << 4) + z);
    }

    /// <summary>
    /// Capture thread-safe read-only snapshot of chunk data.
    /// </summary>
    /// <returns>ChunkSnapshot.</returns>
    public ChunkSnapshot getChunkSnapshot()
    {
        return getChunkSnapshot(false, false);
    }

    /// <summary>
    /// Capture thread-safe read-only snapshot of chunk data.
    /// </summary>
    /// <param name="includeBiome">If true, snapshot includes per-coordinate biome type.</param>
    /// <param name="includeBiomeTempRain">If true, snapshot includes per-coordinate raw biome temperature and rainfall.</param>
    /// <returns>ChunkSnapshot.</returns>
    public ChunkSnapshot getChunkSnapshot(bool includeBiome, bool includeBiomeTempRain)
    {
        // this has a lot of overhead
        // (SYLV)todo: clean this up
        int dimId = _world.getDimensionId();
        int[] blockIds = new int[16 * 128 * 16];
        int[] blockData = new int[16 * 128 * 16];
        int[] maxBlockY = new int[16 * 16];
        int[] skyLight = new int[16 * 128 * 16];
        int[] blockLight = new int[16 * 128 * 16];
        int[]? biomeIds = includeBiome ? new int[16 * 16] : null;
        double[]? biomeTemp = includeBiomeTempRain ? new double[16 * 16] : null;
        double[]? biomeRainfall = includeBiomeTempRain ? new double[16 * 16] : null;

        if (NativeBridge.GetChunkSnapshot != null)
        {
            var hIds  = GCHandle.Alloc(blockIds,  GCHandleType.Pinned);
            var hData = GCHandle.Alloc(blockData, GCHandleType.Pinned);
            var hMaxY = GCHandle.Alloc(maxBlockY, GCHandleType.Pinned);
            try
            {
                NativeBridge.GetChunkSnapshot(dimId, _chunkX, _chunkZ,
                    hIds.AddrOfPinnedObject(),
                    hData.AddrOfPinnedObject(),
                    hMaxY.AddrOfPinnedObject());
            }
            finally
            {
                hIds.Free();
                hData.Free();
                hMaxY.Free();
            }
        }
        else
        {
            for (int lx = 0; lx < 16; lx++)
            {
                for (int lz = 0; lz < 16; lz++)
                {
                    int worldX = (_chunkX << 4) + lx;
                    int worldZ = (_chunkZ << 4) + lz;
                    int highest = 0;
                    for (int ly = 0; ly < 128; ly++)
                    {
                        int idx = (lx * 128 * 16) + (ly * 16) + lz;
                        if (NativeBridge.GetTileId != null)
                            blockIds[idx] = NativeBridge.GetTileId(dimId, worldX, ly, worldZ);
                        if (NativeBridge.GetTileData != null)
                            blockData[idx] = NativeBridge.GetTileData(dimId, worldX, ly, worldZ);
                        if (blockIds[idx] != 0)
                            highest = ly;
                    }
                    maxBlockY[lx * 16 + lz] = highest;
                }
            }
        }

        for (int lx = 0; lx < 16; lx++)
        {
            for (int lz = 0; lz < 16; lz++)
            {
                int worldX = (_chunkX << 4) + lx;
                int worldZ = (_chunkZ << 4) + lz;
                for (int ly = 0; ly < 128; ly++)
                {
                    int idx = (lx * 128 * 16) + (ly * 16) + lz;
                    if (NativeBridge.GetSkyLight != null)
                        skyLight[idx] = NativeBridge.GetSkyLight(dimId, worldX, ly, worldZ);
                    if (NativeBridge.GetBlockLight != null)
                        blockLight[idx] = NativeBridge.GetBlockLight(dimId, worldX, ly, worldZ);
                }
                if (includeBiome && NativeBridge.GetBiomeId != null)
                {
                    int colIdx = lx * 16 + lz;
                    int biomeId = NativeBridge.GetBiomeId(dimId, worldX, worldZ);
                    biomeIds![colIdx] = biomeId;
                    if (includeBiomeTempRain)
                    {
                        var biome = Block.BiomeHelper.fromId(biomeId);
                        biomeTemp![colIdx] = biome.getTemperature();
                        biomeRainfall![colIdx] = biome.getRainfall();
                    }
                }
            }
        }

        long captureTime = 0;
        if (NativeBridge.GetWorldInfo != null)
        {
            double[] info = new double[7];
            var hInfo = GCHandle.Alloc(info, GCHandleType.Pinned);
            try
            {
                NativeBridge.GetWorldInfo(dimId, hInfo.AddrOfPinnedObject());
            }
            finally
            {
                hInfo.Free();
            }
            captureTime = (long)info[4];
        }

        return new ChunkSnapshot(_chunkX, _chunkZ, _world.getName(), captureTime,
                                  blockIds, blockData, maxBlockY,
                                  skyLight, blockLight, biomeIds, biomeTemp, biomeRainfall);
    }

    /// <summary>
    /// Capture thread-safe read-only snapshot of chunk data.
    /// </summary>
    /// <param name="includeMaxblocky">(NONFUNCTIONAL) Only here for parity.</param>
    /// <param name="includeBiome">If true, snapshot includes per-coordinate biome type.</param>
    /// <param name="includeBiomeTempRain">If true, snapshot includes per-coordinate raw biome temperature and rainfall.</param>
    /// <returns>ChunkSnapshot.</returns>
    public ChunkSnapshot getChunkSnapshot(bool includeMaxblocky, bool includeBiome, bool includeBiomeTempRain)
    {
        return getChunkSnapshot(includeBiome, includeBiomeTempRain);
    }

    /// <summary>
    /// Get a list of all entities in the chunk.
    /// </summary>
    /// <returns>The entities.</returns>
    public Entity.Entity[] getEntities()
    {
        if (NativeBridge.GetChunkEntities == null) return Array.Empty<Entity.Entity>();

        int dimId = _world.getDimensionId();
        int count = NativeBridge.GetChunkEntities(dimId, _chunkX, _chunkZ, out IntPtr buf);
        if (count <= 0 || buf == IntPtr.Zero) return Array.Empty<Entity.Entity>();

        var result = new Entity.Entity[count];
        try
        {
            int[] data = new int[count * 3];
            Marshal.Copy(buf, data, 0, count * 3);

            for (int i = 0; i < count; i++)
            {
                int entityId  = data[i * 3];
                int mappedType = data[i * 3 + 1];
                int isLiving   = data[i * 3 + 2];

                var entityType = Enum.IsDefined(typeof(Entity.EntityType), mappedType)
                    ? (Entity.EntityType)mappedType
                    : Entity.EntityType.UNKNOWN;

                if (entityType == Entity.EntityType.PLAYER)
                {
                    var player = FourKit.GetPlayerByEntityId(entityId);
                    if (player != null)
                    {
                        result[i] = player;
                        continue;
                    }
                }

                if (isLiving == 1)
                {
                    result[i] = new Entity.LivingEntity(entityId, entityType, dimId, 0, 0, 0);
                }
                else
                {
                    var entity = new Entity.Entity();
                    entity.SetEntityIdInternal(entityId);
                    entity.SetEntityTypeInternal(entityType);
                    entity.SetDimensionInternal(dimId);
                    result[i] = entity;
                }
            }
        }
        finally
        {
            Marshal.FreeCoTaskMem(buf);
        }

        return result;
    }

    /// <summary>
    /// Checks if the chunk is loaded.
    /// </summary>
    /// <returns>True if it is loaded.</returns>
    public bool isLoaded()
    {
        if (NativeBridge.IsChunkLoaded != null)
            return NativeBridge.IsChunkLoaded(_world.getDimensionId(), _chunkX, _chunkZ) != 0;
        return false;
    }

    /// <summary>
    /// Loads the chunk.
    /// </summary>
    /// <param name="generate">Whether or not to generate a chunk if it doesn't already exist.</param>
    /// <returns>true if the chunk has loaded successfully, otherwise false.</returns>
    public bool load(bool generate)
    {
        if (NativeBridge.LoadChunk != null)
            return NativeBridge.LoadChunk(_world.getDimensionId(), _chunkX, _chunkZ, generate ? 1 : 0) != 0;
        return false;
    }

    /// <summary>
    /// Loads the chunk.
    /// </summary>
    /// <returns>true if the chunk has loaded successfully, otherwise false.</returns>
    public bool load()
    {
        return load(true);
    }

    /// <summary>
    /// Unloads and optionally saves the Chunk.
    /// </summary>
    /// <param name="save">Controls whether the chunk is saved.</param>
    /// <param name="safe">Controls whether to unload the chunk when players are nearby.</param>
    /// <returns>true if the chunk has unloaded successfully, otherwise false.</returns>
    public bool unload(bool save, bool safe)
    {
        if (NativeBridge.UnloadChunk != null)
            return NativeBridge.UnloadChunk(_world.getDimensionId(), _chunkX, _chunkZ, save ? 1 : 0, safe ? 1 : 0) != 0;
        return false;
    }

    /// <summary>
    /// Unloads and optionally saves the Chunk.
    /// </summary>
    /// <param name="save">Controls whether the chunk is saved.</param>
    /// <returns>true if the chunk has unloaded successfully, otherwise false.</returns>
    public bool unload(bool save)
    {
        return unload(save, true);
    }

    /// <summary>
    /// Unloads and optionally saves the Chunk.
    /// </summary>
    /// <returns>true if the chunk has unloaded successfully, otherwise false.</returns>
    public bool unload()
    {
        return unload(true, true);
    }
}
