namespace Minecraft.Server.FourKit.Inventory;

using System.Runtime.InteropServices;

// todo: this needs to be removed at some point

internal class EnderChestInventory : Inventory
{
    private readonly int _ownerEntityId;

    internal EnderChestInventory(int ownerEntityId)
        : base("Ender Chest", InventoryType.ENDER_CHEST, 27)
    {
        _ownerEntityId = ownerEntityId;
    }

    protected internal override void EnsureSynced()
    {
        if (NativeBridge.GetEnderChestContents == null)
            return;

        int[] buf = new int[27 * 3];
        var gh = GCHandle.Alloc(buf, GCHandleType.Pinned);
        try
        {
            NativeBridge.GetEnderChestContents(_ownerEntityId, gh.AddrOfPinnedObject());
        }
        finally
        {
            gh.Free();
        }

        for (int i = 0; i < 27; i++)
        {
            int id = buf[i * 3 + 0];
            int aux = buf[i * 3 + 1];
            int packed = buf[i * 3 + 2];

            ushort count = (ushort)((packed >> 8) & 0xFFFF);

            _items[i]?.UnbindFromInventory();
            if (id > 0 && count > 0)
            {
                if (_items[i] == null)
                {
                    _items[i] = new ItemStack(id, count, (short)aux);
                }
                else
                {
                    _items[i]!.setTypeId(id);
                    _items[i]!.setAmount(count);
                    _items[i]!.setDurability((short)aux);
                }
                _items[i]!.BindToInventory(this, i);
            }
            else
            {
                _items[i] = null;
            }
        }
    }

    public override void setItem(int index, ItemStack? item)
    {
        if (index >= 0 && index < _items.Length)
        {
            var old = _items[index];
            if (old != item)
            {
                old?.UnbindFromInventory();
                item?.BindToInventory(this, index);
            }
            _items[index] = item;
            _slotModifiedByPlugin = true;
        }

        if (NativeBridge.SetEnderChestSlot != null && index >= 0 && index < _items.Length)
        {
            int id = item?.getTypeId() ?? 0;
            int count = item?.getAmount() ?? 0;
            int aux = item?.getDurability() ?? 0;
            NativeBridge.SetEnderChestSlot(_ownerEntityId, index, id, count, aux);
        }
    }
}
