namespace Minecraft.Server.FourKit.Enchantments;

using Minecraft.Server.FourKit.Inventory;

/// <summary>
/// Represents the applicable target for a Enchantment
/// </summary>
public enum EnchantmentTarget
{
    /// <summary>
    /// Allows the Enchantment to be placed on all items
    /// </summary>
    ALL,
    /// <summary>
    /// Allows the Enchantment to be placed on armor
    /// </summary>
    ARMOR,
    /// <summary>
    /// Allows the Enchantment to be placed on feet slot armor
    /// </summary>
    ARMOR_FEET,
    /// <summary>
    /// Allows the Enchantment to be placed on head slot armor
    /// </summary>
    ARMOR_HEAD,
    /// <summary>
    /// Allows the Enchantment to be placed on leg slot armor
    /// </summary>
    ARMOR_LEGS,
    /// <summary>
    /// Allows the Enchantment to be placed on torso slot armor
    /// </summary>
    ARMOR_TORSO,
    /// <summary>
    /// Allows the Enchantment to be placed on bows.
    /// </summary>
    BOW,
    /// <summary>
    /// Allows the Enchantment to be placed on tools (spades, pickaxe, hoes, axes)
    /// </summary>
    TOOL,
    /// <summary>
    /// Allows the Enchantment to be placed on weapons (swords)
    /// </summary>
    WEAPON,
}

//these numbers match the same ones that the enchants register as, Enchantment.cpp - 41

/// <summary>
/// The various type of enchantments that may be added to armour or weapons 
/// </summary>
public enum EnchantmentType
{
    /// <summary>
    /// Provides extra damage when shooting arrows from bows
    /// </summary>
    ARROW_DAMAGE = 48,
    /// <summary>
    /// Sets entities on fire when hit by arrows shot from a bow
    /// </summary>
    ARROW_FIRE = 50,
    /// <summary>
    /// Provides infinite arrows when shooting a bow
    /// </summary>
    ARROW_INFINITE = 51,
    /// <summary>
    /// Provides a knockback when an entity is hit by an arrow from a bow
    /// </summary>
    ARROW_KNOCKBACK = 49,
    /// <summary>
    /// Increases damage against all targets
    /// </summary>
    DAMAGE_ALL = 16,
    /// <summary>
    /// Increases damage against arthropod targets
    /// </summary>
    DAMAGE_ARTHOPODS = 18,
    /// <summary>
    /// Increases damage against undead targets
    /// </summary>
    DAMAGE_UNDEAD = 17,
    /// <summary>
    /// Increases the rate at which you mine/dig
    /// </summary>
    DIG_SPEAD = 32,
    /// <summary>
    /// Decreases the rate at which a tool looses durability
    /// </summary>
    DURABILITY = 34,
    /// <summary>
    /// When attacking a target, has a chance to set them on fire
    /// </summary>
    FIRE_ASPECT = 20,
    /// <summary>
    /// All damage to other targets will knock them back when hit
    /// </summary>
    KNOCKBACK = 19,
    /// <summary>
    /// Provides a chance of gaining extra loot when destroying blocks
    /// </summary>
    LOOT_BONUS_BLOCKS = 35,
    /// <summary>
    /// Provides a chance of gaining extra loot when killing monsters
    /// </summary>
    LOOT_BONUS_MOBS = 21,
    /// <summary>
    /// Decreases the rate of air loss whilst underwater
    /// </summary>
    OXYGEN = 5,
    /// <summary>
    /// Provides protection against environmental damage
    /// </summary>
    PROTECTION_ENVIRONMENTAL = 0,
    /// <summary>
    /// Provides protection against explosive damage
    /// </summary>
    PROTECTION_EXPLOSIVE = 3,
    /// <summary>
    /// Provides protection against fall damage
    /// </summary>
    PROTECTION_FALL = 2,
    /// <summary>
    /// Provides protection against fire damage
    /// </summary>
    PROTECTION_FIRE = 1,
    /// <summary>
    /// Provides protection against projectile damage
    /// </summary>
    PROTECTION_PROJECTILE = 4,
    /// <summary>
    /// Allows blocks to drop themselves instead of fragments (for example, stone instead of cobblestone)
    /// </summary>
    SILK_TOUCH = 33,
    /// <summary>
    /// Damages the attacker
    /// </summary>
    THORNS = 7,
    /// <summary>
    /// Increases the speed at which a player may mine underwater
    /// </summary>
    WATER_WORKER = 6,
    /// <summary>
    /// Increases the rate at which fish bite the hook while fishing
    /// </summary>
    LURE = 64,
    /// <summary>
    /// Increases the chance of catching valuable items while fishing
    /// </summary>
    LUCK_OF_THE_SEA = 65
}

public abstract class Enchantment
{
    public static Enchantment PowerEnchantment => _registry[EnchantmentType.ARROW_DAMAGE];
    public static Enchantment FlameEnchantment => _registry[EnchantmentType.ARROW_FIRE];
    public static Enchantment InfinityEnchantment => _registry[EnchantmentType.ARROW_INFINITE];
    public static Enchantment PunchEnchantment => _registry[EnchantmentType.ARROW_KNOCKBACK];
    public static Enchantment SharpnessEnchantment => _registry[EnchantmentType.DAMAGE_ALL];
    public static Enchantment BaneOfArthropodsEnchantment => _registry[EnchantmentType.DAMAGE_ARTHOPODS];
    public static Enchantment SmiteEnchantment => _registry[EnchantmentType.DAMAGE_UNDEAD];
    public static Enchantment EfficiencyEnchantment => _registry[EnchantmentType.DIG_SPEAD];
    public static Enchantment UnbreakingEnchantment => _registry[EnchantmentType.DURABILITY];
    public static Enchantment FireAspectEnchantment => _registry[EnchantmentType.FIRE_ASPECT];
    public static Enchantment KnockbackEnchantment => _registry[EnchantmentType.KNOCKBACK];
    public static Enchantment FortuneEnchantment => _registry[EnchantmentType.LOOT_BONUS_BLOCKS];
    public static Enchantment LootingEnchantment => _registry[EnchantmentType.LOOT_BONUS_MOBS];
    public static Enchantment RespirationEnchantment => _registry[EnchantmentType.OXYGEN];
    public static Enchantment ProtectionEnchantment => _registry[EnchantmentType.PROTECTION_ENVIRONMENTAL];
    public static Enchantment BlastProtectionEnchantment => _registry[EnchantmentType.PROTECTION_EXPLOSIVE];
    public static Enchantment FeatherFallingEnchantment => _registry[EnchantmentType.PROTECTION_FALL];
    public static Enchantment FireProtectionEnchantment => _registry[EnchantmentType.PROTECTION_FIRE];
    public static Enchantment ProjectileProtectionEnchantment => _registry[EnchantmentType.PROTECTION_PROJECTILE];
    public static Enchantment SilkTouchEnchantment => _registry[EnchantmentType.SILK_TOUCH];
    public static Enchantment ThornsEnchantment => _registry[EnchantmentType.THORNS];
    public static Enchantment AquaAffinityEnchantment => _registry[EnchantmentType.WATER_WORKER];
    public static Enchantment LureEnchantment => _registry[EnchantmentType.LURE];
    public static Enchantment LuckOfTheSeaEnchantment => _registry[EnchantmentType.LUCK_OF_THE_SEA];


    private static Dictionary<EnchantmentType, Enchantment> _registry = new Dictionary<EnchantmentType, Enchantment>()
    {
        { EnchantmentType.ARROW_DAMAGE, new PowerEnchantment() },
        { EnchantmentType.ARROW_FIRE, new FlameEnchantment() },
        { EnchantmentType.ARROW_INFINITE, new InfinityEnchantment() },
        { EnchantmentType.ARROW_KNOCKBACK, new PunchEnchantment() },
        { EnchantmentType.DAMAGE_ALL, new SharpnessEnchantment() },
        { EnchantmentType.DAMAGE_ARTHOPODS, new BaneOfArthropodsEnchantment() },
        { EnchantmentType.DAMAGE_UNDEAD, new SmiteEnchantment() },
        { EnchantmentType.DIG_SPEAD, new EfficiencyEnchantment() },
        { EnchantmentType.DURABILITY, new UnbreakingEnchantment() },
        { EnchantmentType.FIRE_ASPECT, new FireAspectEnchantment() },
        { EnchantmentType.KNOCKBACK, new KnockbackEnchantment() },
        { EnchantmentType.LOOT_BONUS_BLOCKS, new FortuneEnchantment() },
        { EnchantmentType.LOOT_BONUS_MOBS, new LootingEnchantment() },
        { EnchantmentType.OXYGEN, new RespirationEnchantment() },
        { EnchantmentType.PROTECTION_ENVIRONMENTAL, new ProtectionEnchantment() },
        { EnchantmentType.PROTECTION_EXPLOSIVE, new BlastProtectionEnchantment() },
        { EnchantmentType.PROTECTION_FALL, new FeatherFallingEnchantment() },
        { EnchantmentType.PROTECTION_FIRE, new FireProtectionEnchantment() },
        { EnchantmentType.PROTECTION_PROJECTILE, new ProjectileProtectionEnchantment() },
        { EnchantmentType.SILK_TOUCH, new SilkTouchEnchantment() },
        { EnchantmentType.THORNS, new ThornsEnchantment() },
        { EnchantmentType.WATER_WORKER, new AquaAffinityEnchantment() },
        { EnchantmentType.LURE, new LureEnchantment() },
        { EnchantmentType.LUCK_OF_THE_SEA, new LuckOfTheSeaEnchantment() },
    };

    /// <summary>
    /// Checks if this Enchantment may be applied to the given <see cref="ItemStack"/>. This does not check if it conflicts with any enchantments already applied to the item. 
    /// </summary>
    /// <param name="item">Item to test</param>
    /// <returns>True if the enchantment may be applied, otherwise False</returns>
    public abstract bool canEnchantItem(ItemStack item);

    /// <summary>
    /// Check if this enchantment conflicts with another enchantment.
    /// </summary>
    /// <param name="other">The enchantment to check against </param>
    /// <returns>True if there is a conflict.</returns>
    public abstract bool conflictsWith(Enchantment other);
    //public abstract Enchantment getById(int id); //deprecated by bukkit

    /// <summary>
    /// Gets the Enchantment at the specified name 
    /// </summary>
    /// <param name="name">Name to fetch.</param>
    /// <returns>Resulting Enchantment, or null if not found</returns>
    public static Enchantment? getByName(string name)
    {
        foreach (KeyValuePair<EnchantmentType, Enchantment> enchantmentPair in _registry)
        {
            if (enchantmentPair.Value.getName().Equals(name)) return enchantmentPair.Value;
        }

        return null;
    }

    /// <summary>
    /// Gets the Enchantment at the specified type 
    /// </summary>
    /// <param name="type">Type to fetch.</param>
    /// <returns>Resulting Enchantment, or null if not found</returns>
    public static Enchantment getByType(EnchantmentType type)
    {
        return _registry[type]; //we should always have the enchant based on the type
    }

    //public abstract int getId(); //deprecated by bukkit

    /// <summary>
    /// Gets the type of <see cref="ItemStack"/> that may fit this Enchantment.
    /// </summary>
    /// <returns>Gets the type of <see cref="ItemStack"/> that may fit this Enchantment.</returns>
    public abstract EnchantmentTarget getItemTarget();

    /// <summary>
    /// Returns the <see cref="EnchantmentType"/>.
    /// </summary>
    /// <returns>Gets the enchantment type.</returns>
    public abstract EnchantmentType getEnchantType();

    /// <summary>
    /// Gets the maximum level that this Enchantment may become.
    /// </summary>
    /// <returns>Maximum level of the Enchantment</returns>
    public abstract int getMaxLevel();


    /// <summary>
    /// Gets the unique name of this enchantment
    /// </summary>
    /// <returns>Unique name</returns>
    public abstract string getName();

    /// <summary>
    /// Gets the level that this Enchantment should start at
    /// </summary>
    /// <returns>Starting level of the Enchantment</returns>
    public abstract int getStartLevel();

    //public static bool isAcceptingRegistrations(); //we dont have enchant registrations
    //public static void registerEnchantment(Enchantment enchantment); //we dont have enchant registrations
    //public static void stopAcceptingRegistrations(); //we dont have enchant registrations

}