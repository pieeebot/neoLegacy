namespace Minecraft.Server.FourKit.Block;


public enum Biome
{
    OCEAN = 0,
    PLAINS = 1,
    DESERT = 2,
    EXTREME_HILLS = 3,
    FOREST = 4,
    TAIGA = 5,
    SWAMPLAND = 6,
    RIVER = 7,
    HELL = 8,
    SKY = 9,
    FROZEN_OCEAN = 10,
    FROZEN_RIVER = 11,
    ICE_PLAINS = 12,
    ICE_MOUNTAINS = 13,
    MUSHROOM_ISLAND = 14,
    MUSHROOM_SHORE = 15,
    BEACH = 16,
    DESERT_HILLS = 17,
    FOREST_HILLS = 18,
    TAIGA_HILLS = 19,
    SMALL_MOUNTAINS = 20,
    JUNGLE = 21,
    JUNGLE_HILLS = 22,
}


// more for internal
// eliminates unnecessary overhead
internal static class BiomeHelper
{
    private static readonly double[] _temperatures = new double[23];
    private static readonly double[] _rainfalls = new double[23];

    static BiomeHelper()
    {
        _temperatures[(int)Biome.OCEAN] = 0.5;
        _temperatures[(int)Biome.PLAINS] = 0.8;
        _temperatures[(int)Biome.DESERT] = 2.0;
        _temperatures[(int)Biome.EXTREME_HILLS] = 0.2;
        _temperatures[(int)Biome.FOREST] = 0.7;
        _temperatures[(int)Biome.TAIGA] = 0.05;
        _temperatures[(int)Biome.SWAMPLAND] = 0.8;
        _temperatures[(int)Biome.RIVER] = 0.5;
        _temperatures[(int)Biome.HELL] = 2.0;
        _temperatures[(int)Biome.SKY] = 0.5;
        _temperatures[(int)Biome.FROZEN_OCEAN] = 0.0;
        _temperatures[(int)Biome.FROZEN_RIVER] = 0.0;
        _temperatures[(int)Biome.ICE_PLAINS] = 0.0;
        _temperatures[(int)Biome.ICE_MOUNTAINS] = 0.0;
        _temperatures[(int)Biome.MUSHROOM_ISLAND] = 0.9;
        _temperatures[(int)Biome.MUSHROOM_SHORE] = 0.9;
        _temperatures[(int)Biome.BEACH] = 0.8;
        _temperatures[(int)Biome.DESERT_HILLS] = 2.0;
        _temperatures[(int)Biome.FOREST_HILLS] = 0.7;
        _temperatures[(int)Biome.TAIGA_HILLS] = 0.05;
        _temperatures[(int)Biome.SMALL_MOUNTAINS] = 0.2;
        _temperatures[(int)Biome.JUNGLE] = 1.2;
        _temperatures[(int)Biome.JUNGLE_HILLS] = 1.2;

        _rainfalls[(int)Biome.OCEAN] = 0.5;
        _rainfalls[(int)Biome.PLAINS] = 0.4;
        _rainfalls[(int)Biome.DESERT] = 0.0;
        _rainfalls[(int)Biome.EXTREME_HILLS] = 0.3;
        _rainfalls[(int)Biome.FOREST] = 0.8;
        _rainfalls[(int)Biome.TAIGA] = 0.8;
        _rainfalls[(int)Biome.SWAMPLAND] = 0.9;
        _rainfalls[(int)Biome.RIVER] = 0.5;
        _rainfalls[(int)Biome.HELL] = 0.0;
        _rainfalls[(int)Biome.SKY] = 0.5;
        _rainfalls[(int)Biome.FROZEN_OCEAN] = 0.5;
        _rainfalls[(int)Biome.FROZEN_RIVER] = 0.5;
        _rainfalls[(int)Biome.ICE_PLAINS] = 0.5;
        _rainfalls[(int)Biome.ICE_MOUNTAINS] = 0.5;
        _rainfalls[(int)Biome.MUSHROOM_ISLAND] = 1.0;
        _rainfalls[(int)Biome.MUSHROOM_SHORE] = 1.0;
        _rainfalls[(int)Biome.BEACH] = 0.4;
        _rainfalls[(int)Biome.DESERT_HILLS] = 0.0;
        _rainfalls[(int)Biome.FOREST_HILLS] = 0.8;
        _rainfalls[(int)Biome.TAIGA_HILLS] = 0.8;
        _rainfalls[(int)Biome.SMALL_MOUNTAINS] = 0.3;
        _rainfalls[(int)Biome.JUNGLE] = 0.9;
        _rainfalls[(int)Biome.JUNGLE_HILLS] = 0.9;
    }

    public static double getTemperature(this Biome biome)
    {
        int id = (int)biome;
        if (id >= 0 && id < _temperatures.Length) return _temperatures[id];
        return 0.5;
    }

    public static double getRainfall(this Biome biome)
    {
        int id = (int)biome;
        if (id >= 0 && id < _rainfalls.Length) return _rainfalls[id];
        return 0.5;
    }

    public static Biome fromId(int id)
    {
        if (Enum.IsDefined(typeof(Biome), id)) return (Biome)id;
        return Biome.PLAINS;
    }
}
