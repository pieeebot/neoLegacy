namespace Minecraft.Server.FourKit.Chunk;

using Minecraft.Server.FourKit.Block;

/// <summary>
/// Represents a static, thread-safe snapshot of chunk of blocks.
/// Purpose is to allow clean, efficient copy of a chunk data to be made, and
/// then handed off for processing in another thread (e.g. map rendering).
/// </summary>
public class ChunkSnapshot
{
    private readonly int _chunkX;
    private readonly int _chunkZ;
    private readonly string _worldName;
    private readonly long _captureFullTime;
    private readonly int[] _blockIds;
    private readonly int[] _blockData;
    private readonly int[] _maxBlockY;
    private readonly int[]? _skyLight;
    private readonly int[]? _blockLight;
    private readonly int[]? _biome;
    private readonly double[]? _biomeTemp;
    private readonly double[]? _biomeRainfall;

    internal ChunkSnapshot(int chunkX, int chunkZ, string worldName, long captureFullTime,
                            int[] blockIds, int[] blockData, int[] maxBlockY,
                            int[]? skyLight = null, int[]? blockLight = null,
                            int[]? biome = null, double[]? biomeTemp = null, double[]? biomeRainfall = null)
    {
        _chunkX = chunkX;
        _chunkZ = chunkZ;
        _worldName = worldName;
        _captureFullTime = captureFullTime;
        _blockIds = blockIds;
        _blockData = blockData;
        _maxBlockY = maxBlockY;
        _skyLight = skyLight;
        _blockLight = blockLight;
        _biome = biome;
        _biomeTemp = biomeTemp;
        _biomeRainfall = biomeRainfall;
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
    /// Gets name of the world containing this chunk.
    /// </summary>
    /// <returns>Parent World Name.</returns>
    public string getWorldName() => _worldName;

    /// <summary>
    /// Get block type for block at corresponding coordinate in the chunk.
    /// </summary>
    /// <param name="x">0-15</param>
    /// <param name="y">0-127</param>
    /// <param name="z">0-15</param>
    /// <returns>0-255</returns>
    public int getBlockTypeId(int x, int y, int z)
    {
        int idx = (x * 128 * 16) + (y * 16) + z;
        if (idx < 0 || idx >= _blockIds.Length) return 0;
        return _blockIds[idx];
    }

    /// <summary>
    /// Get block data for block at corresponding coordinate in the chunk.
    /// </summary>
    /// <param name="x">0-15</param>
    /// <param name="y">0-127</param>
    /// <param name="z">0-15</param>
    /// <returns>0-15</returns>
    public int getBlockData(int x, int y, int z)
    {
        int idx = (x * 128 * 16) + (y * 16) + z;
        if (idx < 0 || idx >= _blockData.Length) return 0;
        return _blockData[idx];
    }

    /// <summary>
    /// Gets the highest non-air coordinate at the given coordinates.
    /// </summary>
    /// <param name="x">X-coordinate of the blocks.</param>
    /// <param name="z">Z-coordinate of the blocks.</param>
    /// <returns>Y-coordinate of the highest non-air block.</returns>
    public int getHighestBlockYAt(int x, int z)
    {
        int idx = x * 16 + z;
        if (idx < 0 || idx >= _maxBlockY.Length) return 0;
        return _maxBlockY[idx];
    }

    /// <summary>
    /// Get world full time when chunk snapshot was captured.
    /// </summary>
    /// <returns>Time in ticks.</returns>
    public long getCaptureFullTime() => _captureFullTime;

    /// <summary>
    /// Test if section is empty.
    /// </summary>
    /// <param name="sy">Section Y coordinate (block Y / 16).</param>
    /// <returns>true if empty, false if not.</returns>
    public bool isSectionEmpty(int sy)
    {
        int startY = sy * 16;
        int endY = startY + 16;
        if (endY > 128) endY = 128;
        for (int x = 0; x < 16; x++)
        {
            for (int z = 0; z < 16; z++)
            {
                for (int y = startY; y < endY; y++)
                {
                    int idx = (x * 128 * 16) + (y * 16) + z;
                    if (idx >= 0 && idx < _blockIds.Length && _blockIds[idx] != 0)
                        return false;
                }
            }
        }
        return true;
    }

    /// <summary>
    /// Get sky light level for block at corresponding coordinate in the chunk.
    /// </summary>
    /// <param name="x">0-15</param>
    /// <param name="y">0-127</param>
    /// <param name="z">0-15</param>
    /// <returns>0-15</returns>
    public int getBlockSkyLight(int x, int y, int z)
    {
        if (_skyLight == null) return 0;
        int idx = (x * 128 * 16) + (y * 16) + z;
        if (idx < 0 || idx >= _skyLight.Length) return 0;
        return _skyLight[idx];
    }

    /// <summary>
    /// Get light level emitted by block at corresponding coordinate in the chunk.
    /// </summary>
    /// <param name="x">0-15</param>
    /// <param name="y">0-127</param>
    /// <param name="z">0-15</param>
    /// <returns>0-15</returns>
    public int getBlockEmittedLight(int x, int y, int z)
    {
        if (_blockLight == null) return 0;
        int idx = (x * 128 * 16) + (y * 16) + z;
        if (idx < 0 || idx >= _blockLight.Length) return 0;
        return _blockLight[idx];
    }

    /// <summary>
    /// Get biome at given coordinates.
    /// </summary>
    /// <param name="x">X-coordinate (0-15)</param>
    /// <param name="z">Z-coordinate (0-15)</param>
    /// <returns>Biome at given coordinate.</returns>
    public Biome getBiome(int x, int z)
    {
        if (_biome == null) return Biome.PLAINS;
        int idx = x * 16 + z;
        if (idx < 0 || idx >= _biome.Length) return Biome.PLAINS;
        return BiomeHelper.fromId(_biome[idx]);
    }

    /// <summary>
    /// Get raw biome temperature (0.0-1.0) at given coordinate.
    /// </summary>
    /// <param name="x">X-coordinate (0-15)</param>
    /// <param name="z">Z-coordinate (0-15)</param>
    /// <returns>Temperature at given coordinate.</returns>
    public double getRawBiomeTemperature(int x, int z)
    {
        if (_biomeTemp != null)
        {
            int idx = x * 16 + z;
            if (idx >= 0 && idx < _biomeTemp.Length) return _biomeTemp[idx];
        }
        return getBiome(x, z).getTemperature();
    }

    /// <summary>
    /// Get raw biome rainfall (0.0-1.0) at given coordinate.
    /// </summary>
    /// <param name="x">X-coordinate (0-15)</param>
    /// <param name="z">Z-coordinate (0-15)</param>
    /// <returns>Rainfall at given coordinate.</returns>
    public double getRawBiomeRainfall(int x, int z)
    {
        if (_biomeRainfall != null)
        {
            int idx = x * 16 + z;
            if (idx >= 0 && idx < _biomeRainfall.Length) return _biomeRainfall[idx];
        }
        return getBiome(x, z).getRainfall();
    }
}
