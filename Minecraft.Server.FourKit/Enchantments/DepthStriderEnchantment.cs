using Minecraft.Server.FourKit.Inventory;

namespace Minecraft.Server.FourKit.Enchantments;

public class DepthStriderEnchantment : Enchantment
{
    static readonly Material[] supportedItems = {
        Material.LEATHER_BOOTS, Material.CHAINMAIL_BOOTS, Material.GOLD_BOOTS, Material.IRON_BOOTS, Material.DIAMOND_BOOTS,
    };

    static readonly EnchantmentType[] conflictedEnchants = { };

    public override bool canEnchantItem(ItemStack item) => supportedItems.Contains(item.getType());

    public override bool conflictsWith(Enchantment other) => conflictedEnchants.Contains(other.getEnchantType());

    public override EnchantmentTarget getItemTarget() => EnchantmentTarget.ARMOR_FEET;

    public override EnchantmentType getEnchantType() => EnchantmentType.WATER_WALKER;

    public override int getMaxLevel() => 1;

    public override string getName() => "depthstrider";

    public override int getStartLevel() => 1;
}