using Minecraft.Server.FourKit.Inventory;

namespace Minecraft.Server.FourKit.Enchantments;

public class LuckOfTheSeaEnchantment  : Enchantment
{
    
    public override bool canEnchantItem(ItemStack item) => item.getType().Equals(Material.FISHING_ROD);

    public override bool conflictsWith(Enchantment other) => false;
    
    public override EnchantmentTarget getItemTarget() => EnchantmentTarget.TOOL;

    public override EnchantmentType getEnchantType() => EnchantmentType.LUCK_OF_THE_SEA;

    public override int getMaxLevel() => 3;

    public override string getName() => "luckofthesea";

    public override int getStartLevel() => 1;
}