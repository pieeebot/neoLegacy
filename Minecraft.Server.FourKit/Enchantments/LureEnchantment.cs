using Minecraft.Server.FourKit.Inventory;

namespace Minecraft.Server.FourKit.Enchantments;

public class LureEnchantment  : Enchantment
{
    
    public override bool canEnchantItem(ItemStack item) => item.getType().Equals(Material.FISHING_ROD);

    public override bool conflictsWith(Enchantment other) => false;
    
    public override EnchantmentTarget getItemTarget() => EnchantmentTarget.TOOL;

    public override EnchantmentType getEnchantType() => EnchantmentType.LURE;

    public override int getMaxLevel() => 3;

    public override string getName() => "lure";

    public override int getStartLevel() => 1;
}