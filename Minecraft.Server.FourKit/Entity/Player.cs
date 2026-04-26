namespace Minecraft.Server.FourKit.Entity;

using System.Runtime.InteropServices;
using Minecraft.Server.FourKit.Command;
using Minecraft.Server.FourKit.Experimental;
using Minecraft.Server.FourKit.Inventory;
using Minecraft.Server.FourKit.Net;

/// <summary>
/// Represents a player connected to the server.
/// </summary>
public class Player : HumanEntity, OfflinePlayer, CommandSender
{
    private float _saturation = 5.0f;
    private float _walkSpeed = 0.2f;
    private float _exhaustion;
    private int _foodLevel = 20;
    private int _level;
    private float _exp;
    private int _totalExperience;
    private Guid _playerUniqueId;
    private ulong _playerRawOnlineXUID;
    private ulong _playerRawOfflineXUID;
    private string? _displayName;
    private bool _sneaking;
    private bool _sprinting;
    private bool _allowFlight;
    private bool _sleepingIgnored;

    private PlayerConnection _connection;

    internal bool IsOnline { get; set; }

    internal Player(int entityId, string name)
    {
        SetEntityIdInternal(entityId);
        SetEntityTypeInternal(EntityType.PLAYER);
        SetNameInternal(name);
        IsOnline = true;
        _playerInventory._holder = this;
        _connection = new PlayerConnection(this);
    }

    /// <inheritdoc/>
    public override EntityType getType() => EntityType.PLAYER;

    /// <inheritdoc/>
    public override EntityType GetType() => EntityType.PLAYER;

    /// <inheritdoc/>
    public override bool teleport(Location location)
    {
        int targetDimId = location.LocationWorld?.getDimensionId() ?? getLocation().LocationWorld?.getDimensionId() ?? 0;
        NativeBridge.TeleportEntity?.Invoke(getEntityId(), targetDimId, location.X, location.Y, location.Z);
        SetLocation(location);
        return true;
    }

    /// <summary>
    /// <b>Experimental.</b> Gets the player's <see cref="PlayerConnection"/>, which can be used
    /// to send raw packet data directly to the client.
    /// </summary>
    /// <returns>The player's connection.</returns>
    public PlayerConnection getConnection() => _connection;

    /// <inheritdoc/>
    public Player? getPlayer() => IsOnline ? this : null;

    /// <summary>
    /// Gets the "friendly" name to display of this player.
    /// This may include color. If no custom display name has been set,
    /// this returns the player's <see cref="HumanEntity.getName"/>.
    /// </summary>
    /// <returns>The display name.</returns>
    public string getDisplayName() => _displayName ?? getName();

    /// <summary>
    /// Sets the "friendly" name to display of this player.
    /// </summary>
    /// <param name="name">The display name, or <c>null</c> to reset to <see cref="HumanEntity.getName"/>.</param>
    public void setDisplayName(string? name)
    {
        _displayName = name;
    }

    /// <inheritdoc/>
    public bool isOnline() => IsOnline;

    /// <summary>
    /// Returns the UUID that uniquely identifies this player across sessions.
    /// This is the player-specific UUID, not the entity UUID.
    /// </summary>
    /// <returns>The player's unique identifier.</returns>
    public new Guid getUniqueId() => _playerUniqueId;


    /// <summary>
    /// <b>Experimental.</b> Gets the raw online XUID (Xbox User ID) for this player.
    /// The online XUID is used for guests.
    /// </summary>
    /// <returns>The raw online XUID value.</returns>
    public ulong getRawOnlineXUID() => _playerRawOnlineXUID;

    /// <summary>
    /// <b>Experimental.</b> Gets the raw offline XUID (Xbox User ID) for this player.
    /// The offline XUID is the main XUID used by the client.
    /// </summary>
    /// <returns>The raw offline XUID value.</returns>
    public ulong getRawOfflineXUID() => _playerRawOfflineXUID;

    /// <summary>
    /// Gets the player's network round trip time in milliseconds.
    /// On the dedicated Windows server this reads the kernel TCP RTT directly
    /// (microsecond precision, untouched by tick scheduler delay). If the OS
    /// query fails it falls back to a smoothed application keepalive value.
    /// </summary>
    /// <returns>The player's RTT in milliseconds, or -1 if unavailable.</returns>
    public int getPing()
    {
        if (NativeBridge.GetPlayerLatency == null)
            return -1;

        return NativeBridge.GetPlayerLatency(getEntityId());
    }

    /// <summary>
    /// Gets the player's current saturation level.
    /// Saturation acts as a buffer before hunger begins to deplete.
    /// </summary>
    /// <returns>The current saturation level.</returns>
    public float getSaturation() => _saturation;

    /// <summary>
    /// Gets the current allowed speed that a client can walk.
    /// The default value is 0.2.
    /// </summary>
    /// <returns>The current walk speed.</returns>
    public float getWalkSpeed() => _walkSpeed;

    /// <summary>
    /// Sets the speed at which a client will walk.
    /// This calls into the native server to apply the change.
    /// </summary>
    /// <param name="value">The new walk speed.</param>
    public void setWalkSpeed(float value)
    {
        _walkSpeed = value;
        NativeBridge.SetWalkSpeed?.Invoke(getEntityId(), value);
    }

    /// <summary>
    /// Returns if the player is in sneak mode.
    /// </summary>
    /// <returns>True if player is in sneak mode.</returns>
    public bool isSneaking() => _sneaking;

    /// <summary>
    /// Gets whether the player is sprinting or not.
    /// </summary>
    /// <returns>True if player is sprinting.</returns>
    public bool isSprinting() => _sprinting;

    /// <summary>
    /// Sets whether the player is ignored as not sleeping. If everyone is
    /// either sleeping or has this flag set, then time will advance to the
    /// next day. If everyone has this flag set but no one is actually in
    /// bed, then nothing will happen.
    /// </summary>
    /// <param name="isSleeping">Whether to ignore.</param>
    public void setSleepingIgnored(bool isSleeping)
    {
        _sleepingIgnored = isSleeping;
        NativeBridge.SetSleepingIgnored?.Invoke(getEntityId(), isSleeping ? 1 : 0);
    }

    /// <summary>
    /// Returns whether the player is sleeping ignored.
    /// </summary>
    /// <returns>Whether player is ignoring sleep.</returns>
    public bool isSleepingIgnored() => _sleepingIgnored;

    /// <summary>
    /// Play a sound for a player at the location.
    /// This function will fail silently if Location or Sound are null.
    /// </summary>
    /// <param name="location">The location to play the sound.</param>
    /// <param name="sound">The sound to play.</param>
    /// <param name="volume">The volume of the sound.</param>
    /// <param name="pitch">The pitch of the sound.</param>
    public void playSound(Location location, Sound sound, float volume, float pitch)
    {
        if (location == null)
            return;
        NativeBridge.PlaySound?.Invoke(getEntityId(), (int)sound, location.X, location.Y, location.Z, volume, pitch);
    }

    /// <summary>
    /// Determines if the Player is allowed to fly via jump key double-tap
    /// like in creative mode.
    /// </summary>
    /// <returns>True if the player is allowed to fly.</returns>
    public bool getAllowFlight() => _allowFlight;

    /// <summary>
    /// Sets if the Player is allowed to fly via jump key double-tap like
    /// in creative mode.
    /// </summary>
    /// <param name="flight">If flight should be allowed.</param>
    public void setAllowFlight(bool flight)
    {
        _allowFlight = flight;
        NativeBridge.SetAllowFlight?.Invoke(getEntityId(), flight ? 1 : 0);
    }

    /// <inheritdoc/>
    public void sendMessage(string message)
    {
        if (string.IsNullOrEmpty(message) || NativeBridge.SendMessage == null)
            return;
        if (message.Length > FourKit.MAX_CHAT_LENGTH)
            message = message[..FourKit.MAX_CHAT_LENGTH];

        IntPtr ptr = Marshal.StringToCoTaskMemUTF8(message);
        try
        {
            NativeBridge.SendMessage(getEntityId(), ptr, System.Text.Encoding.UTF8.GetByteCount(message));
        }
        finally
        {
            Marshal.FreeCoTaskMem(ptr);
        }
    }

    /// <inheritdoc/>
    public void sendMessage(string[] messages)
    {
        foreach (var msg in messages)
            sendMessage(msg);
    }

    /// <summary>
    /// Kicks player with the default <see cref="DisconnectReason.KICKED"/> reason.
    /// </summary>
    public void kickPlayer()
    {
        NativeBridge.KickPlayer?.Invoke(getEntityId(), (int)DisconnectReason.KICKED);
    }

    /// <summary>
    /// Bans the player by UID with the specified reason and disconnects them.
    /// </summary>
    /// <param name="reason">The ban reason.</param>
    /// <returns><c>true</c> if the ban was applied successfully.</returns>
    public bool banPlayer(string reason)
    {
        if (NativeBridge.BanPlayer == null) return false;
        IntPtr ptr = Marshal.StringToCoTaskMemUTF8(reason ?? string.Empty);
        try
        {
            int byteLen = System.Text.Encoding.UTF8.GetByteCount(reason ?? string.Empty);
            return NativeBridge.BanPlayer(getEntityId(), ptr, byteLen) != 0;
        }
        finally
        {
            Marshal.FreeCoTaskMem(ptr);
        }
    }

    /// <summary>
    /// Bans the player's IP address with the specified reason.
    /// </summary>
    /// <param name="reason">The ban reason.</param>
    /// <returns><c>true</c> if the IP ban was applied successfully.</returns>
    public bool banPlayerIp(string reason)
    {
        if (NativeBridge.BanPlayerIp == null) return false;
        IntPtr ptr = Marshal.StringToCoTaskMemUTF8(reason ?? string.Empty);
        try
        {
            int byteLen = System.Text.Encoding.UTF8.GetByteCount(reason ?? string.Empty);
            return NativeBridge.BanPlayerIp(getEntityId(), ptr, byteLen) != 0;
        }
        finally
        {
            Marshal.FreeCoTaskMem(ptr);
        }
    }

    /// <summary>
    /// Gets the socket address of this player.
    /// </summary>
    /// <returns>The player's socket address, or <c>null</c> if the address could not be determined.</returns>
    public InetSocketAddress? getAddress()
    {
        if (NativeBridge.GetPlayerAddress == null)
            return null;

        const int ipBufSize = 64;
        IntPtr ipBuf = Marshal.AllocCoTaskMem(ipBufSize);
        IntPtr portBuf = Marshal.AllocCoTaskMem(sizeof(int));
        try
        {
            int result = NativeBridge.GetPlayerAddress(getEntityId(), ipBuf, ipBufSize, portBuf);
            if (result == 0)
                return null;

            string? ip = Marshal.PtrToStringAnsi(ipBuf);
            int port = Marshal.ReadInt32(portBuf);

            if (string.IsNullOrEmpty(ip))
                return null;

            return new InetSocketAddress(new InetAddress(ip), port);
        }
        finally
        {
            Marshal.FreeCoTaskMem(ipBuf);
            Marshal.FreeCoTaskMem(portBuf);
        }
    }

    /// <summary>
    /// Gets the players current experience level.
    /// </summary>
    /// <returns>Current experience level.</returns>
    public int getLevel() => _level;

    /// <summary>
    /// Sets the players current experience level.
    /// </summary>
    /// <param name="level">New experience level.</param>
    public void setLevel(int level)
    {
        _level = level;
        NativeBridge.SetLevel?.Invoke(getEntityId(), level);
    }

    /// <summary>
    /// Gets the players current experience points towards the next level.
    /// This is a percentage value. 0 is "no progress" and 1 is "next level".
    /// </summary>
    /// <returns>Current experience points.</returns>
    public float getExp() => _exp;

    /// <summary>
    /// Sets the players current experience points towards the next level.
    /// This is a percentage value. 0 is "no progress" and 1 is "next level".
    /// </summary>
    /// <param name="exp">New experience points.</param>
    public void setExp(float exp)
    {
        _exp = exp;
        NativeBridge.SetExp?.Invoke(getEntityId(), exp);
    }

    /// <summary>
    /// Gives the player the amount of experience specified.
    /// </summary>
    /// <param name="amount">Exp amount to give.</param>
    public void giveExp(int amount)
    {
        NativeBridge.GiveExp?.Invoke(getEntityId(), amount);
    }

    /// <summary>
    /// Gives the player the amount of experience levels specified.
    /// Levels can be taken by specifying a negative amount.
    /// </summary>
    /// <param name="amount">Amount of experience levels to give or take.</param>
    public void giveExpLevels(int amount)
    {
        NativeBridge.GiveExpLevels?.Invoke(getEntityId(), amount);
    }

    /// <summary>
    /// Gets the players current exhaustion level.
    /// Exhaustion controls how fast the food level drops. While you have a
    /// certain amount of exhaustion, your saturation will drop to zero, and
    /// then your food will drop to zero.
    /// </summary>
    /// <returns>Exhaustion level.</returns>
    public float getExhaustion() => _exhaustion;

    /// <summary>
    /// Sets the players current exhaustion level.
    /// </summary>
    /// <param name="value">Exhaustion level.</param>
    public void setExhaustion(float value)
    {
        _exhaustion = value;
        NativeBridge.SetExhaustion?.Invoke(getEntityId(), value);
    }

    /// <summary>
    /// Sets the players current saturation level.
    /// </summary>
    /// <param name="value">Saturation level.</param>
    public void setSaturation(float value)
    {
        _saturation = value;
        NativeBridge.SetSaturation?.Invoke(getEntityId(), value);
    }

    /// <summary>
    /// Gets the players current food level.
    /// </summary>
    /// <returns>Food level.</returns>
    public int getFoodLevel() => _foodLevel;

    /// <summary>
    /// Sets the players current food level.
    /// </summary>
    /// <param name="value">New food level.</param>
    public void setFoodLevel(int value)
    {
        _foodLevel = value;
        NativeBridge.SetFoodLevel?.Invoke(getEntityId(), value);
    }

    /// <summary>
    /// Spawns the particle (the number of times specified by count)
    /// at the target location. Only this player will see the particle.
    /// </summary>
    /// <param name="particle">The particle to spawn.</param>
    /// <param name="location">The location to spawn at.</param>
    /// <param name="count">The number of particles.</param>
    public void spawnParticle(Particle particle, Location location, int count)
    {
        spawnParticleInternal(particle, location.X, location.Y, location.Z, count, 0, 0, 0, 0, null);
    }

    /// <summary>
    /// Spawns the particle (the number of times specified by count)
    /// at the target location. Only this player will see the particle.
    /// </summary>
    /// <param name="particle">The particle to spawn.</param>
    /// <param name="x">The position on the x axis to spawn at.</param>
    /// <param name="y">The position on the y axis to spawn at.</param>
    /// <param name="z">The position on the z axis to spawn at.</param>
    /// <param name="count">The number of particles.</param>
    public void spawnParticle(Particle particle, double x, double y, double z, int count)
    {
        spawnParticleInternal(particle, x, y, z, count, 0, 0, 0, 0, null);
    }

    /// <summary>
    /// Spawns the particle (the number of times specified by count)
    /// at the target location. Only this player will see the particle.
    /// </summary>
    /// <param name="particle">The particle to spawn.</param>
    /// <param name="location">The location to spawn at.</param>
    /// <param name="count">The number of particles.</param>
    /// <param name="data">The data to use for the particle or null.</param>
    /// <typeparam name="T">The type of the particle data.</typeparam>
    public void spawnParticle<T>(Particle particle, Location location, int count, T? data)
    {
        spawnParticleInternal(particle, location.X, location.Y, location.Z, count, 0, 0, 0, 0, data);
    }

    /// <summary>
    /// Spawns the particle (the number of times specified by count)
    /// at the target location. Only this player will see the particle.
    /// </summary>
    /// <param name="particle">The particle to spawn.</param>
    /// <param name="x">The position on the x axis to spawn at.</param>
    /// <param name="y">The position on the y axis to spawn at.</param>
    /// <param name="z">The position on the z axis to spawn at.</param>
    /// <param name="count">The number of particles.</param>
    /// <param name="data">The data to use for the particle or null.</param>
    /// <typeparam name="T">The type of the particle data.</typeparam>
    public void spawnParticle<T>(Particle particle, double x, double y, double z, int count, T? data)
    {
        spawnParticleInternal(particle, x, y, z, count, 0, 0, 0, 0, data);
    }

    /// <summary>
    /// Spawns the particle (the number of times specified by count)
    /// at the target location. The position of each particle will be
    /// randomized positively and negatively by the offset parameters
    /// on each axis. Only this player will see the particle.
    /// </summary>
    /// <param name="particle">The particle to spawn.</param>
    /// <param name="location">The location to spawn at.</param>
    /// <param name="count">The number of particles.</param>
    /// <param name="offsetX">The maximum random offset on the X axis.</param>
    /// <param name="offsetY">The maximum random offset on the Y axis.</param>
    /// <param name="offsetZ">The maximum random offset on the Z axis.</param>
    public void spawnParticle(Particle particle, Location location, int count, double offsetX, double offsetY, double offsetZ)
    {
        spawnParticleInternal(particle, location.X, location.Y, location.Z, count, offsetX, offsetY, offsetZ, 0, null);
    }

    /// <summary>
    /// Spawns the particle (the number of times specified by count)
    /// at the target location. The position of each particle will be
    /// randomized positively and negatively by the offset parameters
    /// on each axis. Only this player will see the particle.
    /// </summary>
    /// <param name="particle">The particle to spawn.</param>
    /// <param name="x">The position on the x axis to spawn at.</param>
    /// <param name="y">The position on the y axis to spawn at.</param>
    /// <param name="z">The position on the z axis to spawn at.</param>
    /// <param name="count">The number of particles.</param>
    /// <param name="offsetX">The maximum random offset on the X axis.</param>
    /// <param name="offsetY">The maximum random offset on the Y axis.</param>
    /// <param name="offsetZ">The maximum random offset on the Z axis.</param>
    public void spawnParticle(Particle particle, double x, double y, double z, int count, double offsetX, double offsetY, double offsetZ)
    {
        spawnParticleInternal(particle, x, y, z, count, offsetX, offsetY, offsetZ, 0, null);
    }

    /// <summary>
    /// Spawns the particle (the number of times specified by count)
    /// at the target location. The position of each particle will be
    /// randomized positively and negatively by the offset parameters
    /// on each axis. Only this player will see the particle.
    /// </summary>
    /// <param name="particle">The particle to spawn.</param>
    /// <param name="location">The location to spawn at.</param>
    /// <param name="count">The number of particles.</param>
    /// <param name="offsetX">The maximum random offset on the X axis.</param>
    /// <param name="offsetY">The maximum random offset on the Y axis.</param>
    /// <param name="offsetZ">The maximum random offset on the Z axis.</param>
    /// <param name="data">The data to use for the particle or null.</param>
    /// <typeparam name="T">The type of the particle data.</typeparam>
    public void spawnParticle<T>(Particle particle, Location location, int count, double offsetX, double offsetY, double offsetZ, T? data)
    {
        spawnParticleInternal(particle, location.X, location.Y, location.Z, count, offsetX, offsetY, offsetZ, 0, data);
    }

    /// <summary>
    /// Spawns the particle (the number of times specified by count)
    /// at the target location. The position of each particle will be
    /// randomized positively and negatively by the offset parameters
    /// on each axis. Only this player will see the particle.
    /// </summary>
    /// <param name="particle">The particle to spawn.</param>
    /// <param name="x">The position on the x axis to spawn at.</param>
    /// <param name="y">The position on the y axis to spawn at.</param>
    /// <param name="z">The position on the z axis to spawn at.</param>
    /// <param name="count">The number of particles.</param>
    /// <param name="offsetX">The maximum random offset on the X axis.</param>
    /// <param name="offsetY">The maximum random offset on the Y axis.</param>
    /// <param name="offsetZ">The maximum random offset on the Z axis.</param>
    /// <param name="data">The data to use for the particle or null.</param>
    /// <typeparam name="T">The type of the particle data.</typeparam>
    public void spawnParticle<T>(Particle particle, double x, double y, double z, int count, double offsetX, double offsetY, double offsetZ, T? data)
    {
        spawnParticleInternal(particle, x, y, z, count, offsetX, offsetY, offsetZ, 0, data);
    }

    /// <summary>
    /// Spawns the particle (the number of times specified by count)
    /// at the target location. The position of each particle will be
    /// randomized positively and negatively by the offset parameters
    /// on each axis. Only this player will see the particle.
    /// </summary>
    /// <param name="particle">The particle to spawn.</param>
    /// <param name="location">The location to spawn at.</param>
    /// <param name="count">The number of particles.</param>
    /// <param name="offsetX">The maximum random offset on the X axis.</param>
    /// <param name="offsetY">The maximum random offset on the Y axis.</param>
    /// <param name="offsetZ">The maximum random offset on the Z axis.</param>
    /// <param name="extra">The extra data for this particle, depends on the particle used (normally speed).</param>
    public void spawnParticle(Particle particle, Location location, int count, double offsetX, double offsetY, double offsetZ, double extra)
    {
        spawnParticleInternal(particle, location.X, location.Y, location.Z, count, offsetX, offsetY, offsetZ, extra, null);
    }

    /// <summary>
    /// Spawns the particle (the number of times specified by count)
    /// at the target location. The position of each particle will be
    /// randomized positively and negatively by the offset parameters
    /// on each axis. Only this player will see the particle.
    /// </summary>
    /// <param name="particle">The particle to spawn.</param>
    /// <param name="x">The position on the x axis to spawn at.</param>
    /// <param name="y">The position on the y axis to spawn at.</param>
    /// <param name="z">The position on the z axis to spawn at.</param>
    /// <param name="count">The number of particles.</param>
    /// <param name="offsetX">The maximum random offset on the X axis.</param>
    /// <param name="offsetY">The maximum random offset on the Y axis.</param>
    /// <param name="offsetZ">The maximum random offset on the Z axis.</param>
    /// <param name="extra">The extra data for this particle, depends on the particle used (normally speed).</param>
    public void spawnParticle(Particle particle, double x, double y, double z, int count, double offsetX, double offsetY, double offsetZ, double extra)
    {
        spawnParticleInternal(particle, x, y, z, count, offsetX, offsetY, offsetZ, extra, null);
    }

    /// <summary>
    /// Spawns the particle (the number of times specified by count)
    /// at the target location. The position of each particle will be
    /// randomized positively and negatively by the offset parameters
    /// on each axis. Only this player will see the particle.
    /// </summary>
    /// <param name="particle">The particle to spawn.</param>
    /// <param name="location">The location to spawn at.</param>
    /// <param name="count">The number of particles.</param>
    /// <param name="offsetX">The maximum random offset on the X axis.</param>
    /// <param name="offsetY">The maximum random offset on the Y axis.</param>
    /// <param name="offsetZ">The maximum random offset on the Z axis.</param>
    /// <param name="extra">The extra data for this particle, depends on the particle used (normally speed).</param>
    /// <param name="data">The data to use for the particle or null.</param>
    /// <typeparam name="T">The type of the particle data.</typeparam>
    public void spawnParticle<T>(Particle particle, Location location, int count, double offsetX, double offsetY, double offsetZ, double extra, T? data)
    {
        spawnParticleInternal(particle, location.X, location.Y, location.Z, count, offsetX, offsetY, offsetZ, extra, data);
    }

    /// <summary>
    /// Spawns the particle (the number of times specified by count)
    /// at the target location. The position of each particle will be
    /// randomized positively and negatively by the offset parameters
    /// on each axis. Only this player will see the particle.
    /// </summary>
    /// <param name="particle">The particle to spawn.</param>
    /// <param name="x">The position on the x axis to spawn at.</param>
    /// <param name="y">The position on the y axis to spawn at.</param>
    /// <param name="z">The position on the z axis to spawn at.</param>
    /// <param name="count">The number of particles.</param>
    /// <param name="offsetX">The maximum random offset on the X axis.</param>
    /// <param name="offsetY">The maximum random offset on the Y axis.</param>
    /// <param name="offsetZ">The maximum random offset on the Z axis.</param>
    /// <param name="extra">The extra data for this particle, depends on the particle used (normally speed).</param>
    /// <param name="data">The data to use for the particle or null.</param>
    /// <typeparam name="T">The type of the particle data.</typeparam>
    public void spawnParticle<T>(Particle particle, double x, double y, double z, int count, double offsetX, double offsetY, double offsetZ, double extra, T? data)
    {
        spawnParticleInternal(particle, x, y, z, count, offsetX, offsetY, offsetZ, extra, data);
    }

    private void spawnParticleInternal(Particle particle, double x, double y, double z, int count, double offsetX, double offsetY, double offsetZ, double extra, object? data)
    {
        if (NativeBridge.SpawnParticle == null)
            return;

        int particleId = (int)particle;
        if (data is ItemStack itemStack &&
            (particle == Particle.ITEM_CRACK || particle == Particle.BLOCK_CRACK))
        {
            int id = itemStack.getTypeId();
            int aux = itemStack.getDurability();
            particleId = (int)particle | ((id & 0x0FFF) << 8) | (aux & 0xFF);
        }

        NativeBridge.SpawnParticle(getEntityId(), particleId,
            (float)x, (float)y, (float)z,
            (float)offsetX, (float)offsetY, (float)offsetZ,
            (float)extra, count);
    }

    // INTERNAL
    internal void SetSaturationInternal(float saturation) => _saturation = saturation;
    internal void SetWalkSpeedInternal(float walkSpeed) => _walkSpeed = walkSpeed;
    internal void SetPlayerUniqueIdInternal(Guid id) => _playerUniqueId = id;
    internal void SetPlayerRawOnlineXUIDInternal(ulong xuid) => _playerRawOnlineXUID = xuid;
    internal void SetPlayerRawOfflineXUIDInternal(ulong xuid) => _playerRawOfflineXUID = xuid;
    internal void SetSneakingInternal(bool sneaking) => _sneaking = sneaking;
    internal void SetSprintingInternal(bool sprinting) => _sprinting = sprinting;
    internal void SetAllowFlightInternal(bool allowFlight) => _allowFlight = allowFlight;
    internal void SetSleepingIgnoredInternal(bool ignored) => _sleepingIgnored = ignored;
    internal void SetLevelInternal(int level) => _level = level;
    internal void SetExpInternal(float exp) => _exp = exp;
    internal void SetTotalExperienceInternal(int totalExp) => _totalExperience = totalExp;
    internal void SetFoodLevelInternal(int foodLevel) => _foodLevel = foodLevel;
    internal void SetExhaustionInternal(float exhaustion) => _exhaustion = exhaustion;
}
