namespace Minecraft.Server.FourKit.Inventory;

using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using Minecraft.Server.FourKit.Entity;

/// <summary>
/// Represents an inventory containing items. Behavior relating to
/// <see cref="Material.AIR"/> is unspecified.
/// </summary>
public class Inventory : IEnumerable<ItemStack>
{
    internal static bool _slotModifiedByPlugin;
    private readonly string _name;
    private readonly InventoryType _type;
    internal readonly ItemStack?[] _items;
    private readonly int _nativeEntityId = -1;

    internal Inventory(string name, InventoryType type, int size)
    {
        _name = name;
        _type = type;
        _items = new ItemStack?[size];
    }

    internal Inventory(string name, InventoryType type, int size, int entityId)
        : this(name, type, size)
    {
        _nativeEntityId = entityId;
    }

    protected internal virtual void EnsureSynced()
    {
        if (_nativeEntityId < 0 || NativeBridge.GetContainerContents == null)
            return;

        int[] buf = new int[_items.Length * 3];
        var gh = GCHandle.Alloc(buf, GCHandleType.Pinned);
        try
        {
            NativeBridge.GetContainerContents(_nativeEntityId, gh.AddrOfPinnedObject(), _items.Length);
        }
        finally
        {
            gh.Free();
        }

        for (int i = 0; i < _items.Length; i++)
        {
            int id = buf[i * 3 + 0];
            int aux = buf[i * 3 + 1];
            int packed = buf[i * 3 + 2];

            ushort count = (ushort)((packed >> 8) & 0xFFFF);

            //byte flags = (byte)((packed >> 24) & 0xFF);
            //bool hasMetadata = (flags & 0x1) != 0; //unused here

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
                _items[i]!.BindToInventory(this, i); //should we unbind and rebind or just keep the bind?
            }
            else
            {
                _items[i] = null;
            }
        }
    }

    /// <summary>
    /// Returns the size of the inventory.
    /// </summary>
    /// <returns>The size of the inventory.</returns>
    public int getSize() => _items.Length;

    /// <summary>
    /// Returns the name of the inventory.
    /// </summary>
    /// <returns>The String with the name of the inventory.</returns>
    public string getName() => _name;

    /// <summary>
    /// Returns the ItemStack found in the slot at the given index.
    /// </summary>
    /// <param name="index">The index of the Slot's ItemStack to return.</param>
    /// <returns>The ItemStack in the slot.</returns>
    public virtual ItemStack? getItem(int index)
    {
        EnsureSynced();
        if (index < 0 || index >= _items.Length) return null;
        var item = _items[index];
        item?.BindToInventory(this, index);
        return item;
    }

    /// <summary>
    /// Stores the ItemStack at the given index of the inventory.
    /// </summary>
    /// <param name="index">The index where to put the ItemStack.</param>
    /// <param name="item">The ItemStack to set.</param>
    public virtual void setItem(int index, ItemStack? item)
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

        if (_nativeEntityId >= 0 && NativeBridge.SetContainerSlot != null && index >= 0 && index < _items.Length)
        {
            int id = item?.getTypeId() ?? 0;
            int count = item?.getAmount() ?? 0;
            int aux = item?.getDurability() ?? 0;
            NativeBridge.SetContainerSlot(_nativeEntityId, index, id, count, aux);
        }
    }

    /// <summary>
    /// Stores the given ItemStacks in the inventory. This will try to fill
    /// existing stacks and empty slots as well as it can.
    /// The returned Dictionary contains what it couldn't store, where the key
    /// is the index of the parameter, and the value is the ItemStack at that
    /// index of the params parameter.
    /// </summary>
    /// <param name="items">The ItemStacks to add.</param>
    /// <returns>A Dictionary containing items that didn't fit.</returns>
    public Dictionary<int, ItemStack> addItem(params ItemStack[] items)
    {
        EnsureSynced();
        var leftover = new Dictionary<int, ItemStack>();
        for (int i = 0; i < items.Length; i++)
        {
            var toAdd = items[i];
            if (toAdd == null) continue;
            int remaining = toAdd.getAmount();

            for (int slot = 0; slot < _items.Length && remaining > 0; slot++)
            {
                var existing = _items[slot];
                if (existing != null && existing.getType() == toAdd.getType() &&
                    existing.getDurability() == toAdd.getDurability())
                {
                    int canFit = 64 - existing.getAmount();
                    if (canFit > 0)
                    {
                        int added = Math.Min(canFit, remaining);
                        existing.setAmount(existing.getAmount() + added);
                        setItem(slot, existing);
                        remaining -= added;
                    }
                }
            }

            for (int slot = 0; slot < _items.Length && remaining > 0; slot++)
            {
                if (_items[slot] == null)
                {
                    int added = Math.Min(64, remaining);
                    var newItem = new ItemStack(toAdd.getType(), added, toAdd.getDurability());
                    newItem.setItemMetaInternal(toAdd.getItemMetaInternal()?.clone());
                    setItem(slot, newItem);
                    remaining -= added;
                }
            }

            if (remaining > 0)
                leftover[i] = new ItemStack(toAdd.getType(), remaining, toAdd.getDurability());
        }
        return leftover;
    }

    /// <summary>
    /// Removes the given ItemStacks from the inventory.
    /// It will try to remove 'as much as possible' from the types and amounts
    /// you give as arguments.
    /// The returned Dictionary contains what it couldn't remove.
    /// </summary>
    /// <param name="items">The ItemStacks to remove.</param>
    /// <returns>A Dictionary containing items that couldn't be removed.</returns>
    public Dictionary<int, ItemStack> removeItem(params ItemStack[] items)
    {
        EnsureSynced();
        var leftover = new Dictionary<int, ItemStack>();
        for (int i = 0; i < items.Length; i++)
        {
            var toRemove = items[i];
            if (toRemove == null) continue;
            int remaining = toRemove.getAmount();

            for (int slot = 0; slot < _items.Length && remaining > 0; slot++)
            {
                var existing = _items[slot];
                if (existing != null && existing.getType() == toRemove.getType() &&
                    existing.getDurability() == toRemove.getDurability())
                {
                    int removed = Math.Min(existing.getAmount(), remaining);
                    existing.setAmount(existing.getAmount() - removed);
                    remaining -= removed;
                    if (existing.getAmount() <= 0)
                        setItem(slot, null);
                    else
                        setItem(slot, existing);
                }
            }

            if (remaining > 0)
                leftover[i] = new ItemStack(toRemove.getType(), remaining, toRemove.getDurability());
        }
        return leftover;
    }

    /// <summary>
    /// Returns all ItemStacks from the inventory.
    /// </summary>
    /// <returns>An array of ItemStacks from the inventory.</returns>
    public ItemStack?[] getContents()
    {
        EnsureSynced();
        return (ItemStack?[])_items.Clone();
    }

    /// <summary>
    /// Completely replaces the inventory's contents. Removes all existing
    /// contents and replaces it with the ItemStacks given in the array.
    /// </summary>
    /// <param name="items">A complete replacement for the contents; the length must
    /// be less than or equal to <see cref="getSize"/>.</param>
    public void setContents(ItemStack?[] items)
    {
        int len = Math.Min(items.Length, _items.Length);
        for (int i = 0; i < _items.Length; i++)
            setItem(i, i < len ? items[i] : null);
    }

    /// <summary>
    /// Checks if the inventory contains any ItemStacks with the given material id.
    /// </summary>
    /// <param name="materialId">The material id to check for.</param>
    /// <returns><c>true</c> if an ItemStack in this inventory contains the material id.</returns>
    public bool contains(int materialId)
    {
        EnsureSynced();
        foreach (var item in _items)
            if (item != null && item.getTypeId() == materialId) return true;
        return false;
    }

    /// <summary>
    /// Checks if the inventory contains any ItemStacks with the given material.
    /// </summary>
    /// <param name="material">The material to check for.</param>
    /// <returns><c>true</c> if an ItemStack is found with the given Material.</returns>
    public bool contains(Material material)
    {
        EnsureSynced();
        foreach (var item in _items)
            if (item != null && item.getType() == material) return true;
        return false;
    }

    /// <summary>
    /// Checks if the inventory contains any ItemStacks matching the given ItemStack.
    /// This will only return true if both the type and the amount of the stack match.
    /// </summary>
    /// <param name="item">The ItemStack to match against.</param>
    /// <returns><c>false</c> if item is null, <c>true</c> if any exactly matching ItemStacks were found.</returns>
    public bool contains(ItemStack? item)
    {
        if (item == null) return false;
        EnsureSynced();
        foreach (var slot in _items)
            if (slot != null && slot.getType() == item.getType() &&
                slot.getAmount() == item.getAmount() &&
                slot.getDurability() == item.getDurability()) return true;
        return false;
    }

    /// <summary>
    /// Checks if the inventory contains any ItemStacks with the given material id,
    /// adding to at least the minimum amount specified.
    /// </summary>
    /// <param name="materialId">The material id to check for.</param>
    /// <param name="amount">The minimum amount to look for.</param>
    /// <returns><c>true</c> if this contains any matching ItemStack with the given material id and amount.</returns>
    public bool contains(int materialId, int amount)
    {
        EnsureSynced();
        int total = 0;
        foreach (var item in _items)
            if (item != null && item.getTypeId() == materialId) total += item.getAmount();
        return total >= amount;
    }

    /// <summary>
    /// Checks if the inventory contains any ItemStacks with the given material,
    /// adding to at least the minimum amount specified.
    /// </summary>
    /// <param name="material">The material to check for.</param>
    /// <param name="amount">The minimum amount.</param>
    /// <returns><c>true</c> if enough ItemStacks were found to add to the given amount.</returns>
    public bool contains(Material material, int amount)
    {
        if (amount <= 0) return true;
        EnsureSynced();
        int total = 0;
        foreach (var item in _items)
            if (item != null && item.getType() == material) total += item.getAmount();
        return total >= amount;
    }

    /// <summary>
    /// Checks if the inventory contains at least the minimum amount specified
    /// of exactly matching ItemStacks. An ItemStack only counts if both the type
    /// and the amount of the stack match.
    /// </summary>
    /// <param name="item">The ItemStack to match against.</param>
    /// <param name="amount">How many identical stacks to check for.</param>
    /// <returns><c>false</c> if item is null, <c>true</c> if amount of exactly matching ItemStacks were found.</returns>
    public bool contains(ItemStack? item, int amount)
    {
        if (item == null) return false;
        if (amount <= 0) return true;
        EnsureSynced();
        int count = 0;
        foreach (var slot in _items)
            if (slot != null && slot.getType() == item.getType() &&
                slot.getAmount() == item.getAmount() &&
                slot.getDurability() == item.getDurability()) count++;
        return count >= amount;
    }

    /// <summary>
    /// Checks if the inventory contains ItemStacks matching the given ItemStack
    /// whose amounts sum to at least the minimum amount specified.
    /// </summary>
    /// <param name="item">The ItemStack to match against.</param>
    /// <param name="amount">The minimum amount.</param>
    /// <returns><c>false</c> if item is null, <c>true</c> if enough ItemStacks were found to add to the given amount.</returns>
    public bool containsAtLeast(ItemStack? item, int amount)
    {
        if (item == null) return false;
        if (amount <= 0) return true;
        EnsureSynced();
        int total = 0;
        foreach (var slot in _items)
            if (slot != null && slot.getType() == item.getType() &&
                slot.getDurability() == item.getDurability())
                total += slot.getAmount();
        return total >= amount;
    }

    /// <summary>
    /// Returns a Dictionary with all slots and ItemStacks in the inventory with given material id.
    /// </summary>
    /// <param name="materialId">The material id to look for.</param>
    /// <returns>A Dictionary containing the slot index, ItemStack pairs.</returns>
    public Dictionary<int, ItemStack> all(int materialId)
    {
        EnsureSynced();
        var result = new Dictionary<int, ItemStack>();
        for (int i = 0; i < _items.Length; i++)
            if (_items[i] != null && _items[i]!.getTypeId() == materialId)
                result[i] = _items[i]!;
        return result;
    }

    /// <summary>
    /// Returns a Dictionary with all slots and ItemStacks in the inventory with the given Material.
    /// </summary>
    /// <param name="material">The material to look for.</param>
    /// <returns>A Dictionary containing the slot index, ItemStack pairs.</returns>
    public Dictionary<int, ItemStack> all(Material material)
    {
        EnsureSynced();
        var result = new Dictionary<int, ItemStack>();
        for (int i = 0; i < _items.Length; i++)
            if (_items[i] != null && _items[i]!.getType() == material)
                result[i] = _items[i]!;
        return result;
    }

    /// <summary>
    /// Finds all slots in the inventory containing any ItemStacks with the given ItemStack.
    /// This will only match slots if both the type and the amount of the stack match.
    /// </summary>
    /// <param name="item">The ItemStack to match against.</param>
    /// <returns>A dictionary from slot indexes to item at index.</returns>
    public Dictionary<int, ItemStack> all(ItemStack? item)
    {
        var result = new Dictionary<int, ItemStack>();
        if (item == null) return result;
        EnsureSynced();
        for (int i = 0; i < _items.Length; i++)
            if (_items[i] != null && _items[i]!.getType() == item.getType() &&
                _items[i]!.getAmount() == item.getAmount() &&
                _items[i]!.getDurability() == item.getDurability())
                result[i] = _items[i]!;
        return result;
    }

    /// <summary>
    /// Finds the first slot in the inventory containing an ItemStack with the given material id.
    /// </summary>
    /// <param name="materialId">The material id to look for.</param>
    /// <returns>The slot index of the given material id or -1 if not found.</returns>
    public int first(int materialId)
    {
        EnsureSynced();
        for (int i = 0; i < _items.Length; i++)
            if (_items[i] != null && _items[i]!.getTypeId() == materialId) return i;
        return -1;
    }

    /// <summary>
    /// Finds the first slot in the inventory containing an ItemStack with the given material.
    /// </summary>
    /// <param name="material">The material to look for.</param>
    /// <returns>The slot index of the given Material or -1 if not found.</returns>
    public int first(Material material)
    {
        EnsureSynced();
        for (int i = 0; i < _items.Length; i++)
            if (_items[i] != null && _items[i]!.getType() == material) return i;
        return -1;
    }

    /// <summary>
    /// Returns the first slot in the inventory containing an ItemStack with the given stack.
    /// This will only match a slot if both the type and the amount of the stack match.
    /// </summary>
    /// <param name="item">The ItemStack to match against.</param>
    /// <returns>The slot index of the given ItemStack or -1 if not found.</returns>
    public int first(ItemStack? item)
    {
        if (item == null) return -1;
        EnsureSynced();
        for (int i = 0; i < _items.Length; i++)
            if (_items[i] != null && _items[i]!.getType() == item.getType() &&
                _items[i]!.getAmount() == item.getAmount() &&
                _items[i]!.getDurability() == item.getDurability()) return i;
        return -1;
    }

    /// <summary>
    /// Returns the first empty Slot.
    /// </summary>
    /// <returns>The first empty Slot found, or -1 if no empty slots.</returns>
    public int firstEmpty()
    {
        EnsureSynced();
        for (int i = 0; i < _items.Length; i++)
            if (_items[i] == null) return i;
        return -1;
    }

    /// <summary>
    /// Removes all stacks in the inventory matching the given material id.
    /// </summary>
    /// <param name="materialId">The material to remove.</param>
    public void remove(int materialId)
    {
        EnsureSynced();
        for (int i = 0; i < _items.Length; i++)
            if (_items[i] != null && _items[i]!.getTypeId() == materialId)
                setItem(i, null);
    }

    /// <summary>
    /// Removes all stacks in the inventory matching the given material.
    /// </summary>
    /// <param name="material">The material to remove.</param>
    public void remove(Material material)
    {
        EnsureSynced();
        for (int i = 0; i < _items.Length; i++)
            if (_items[i] != null && _items[i]!.getType() == material)
                setItem(i, null);
    }

    /// <summary>
    /// Removes all stacks in the inventory matching the given stack.
    /// This will only match a slot if both the type and the amount of the stack match.
    /// </summary>
    /// <param name="item">The ItemStack to match against.</param>
    public void remove(ItemStack? item)
    {
        if (item == null) return;
        EnsureSynced();
        for (int i = 0; i < _items.Length; i++)
            if (_items[i] != null && _items[i]!.getType() == item.getType() &&
                _items[i]!.getAmount() == item.getAmount() &&
                _items[i]!.getDurability() == item.getDurability())
                setItem(i, null);
    }

    /// <summary>
    /// Clears out a particular slot in the index.
    /// </summary>
    /// <param name="index">The index to empty.</param>
    public void clear(int index)
    {
        setItem(index, null);
    }

    /// <summary>
    /// Clears out the whole Inventory.
    /// </summary>
    public void clear()
    {
        for (int i = 0; i < _items.Length; i++)
            setItem(i, null);
    }

    /// <summary>
    /// Returns the title of this inventory.
    /// </summary>
    /// <returns>A String with the title.</returns>
    public string getTitle() => _name;

    /// <summary>
    /// Returns what type of inventory this is.
    /// </summary>
    /// <returns>The InventoryType representing the type of inventory.</returns>
    public InventoryType getType() => _type;

    /// <summary>
    /// Gets a list of players viewing the inventory.
    /// </summary>
    /// <returns>A list of HumanEntities who are viewing this Inventory.</returns>
    public virtual List<HumanEntity> getViewers()
    {
        if (_nativeEntityId < 0 || NativeBridge.GetContainerViewerEntityIds == null)
            return new List<HumanEntity>();

        int[] ids = new int[64];
        var ghIds = GCHandle.Alloc(ids, GCHandleType.Pinned);
        int[] countBuf = new int[1];
        var ghCount = GCHandle.Alloc(countBuf, GCHandleType.Pinned);
        try
        {
            NativeBridge.GetContainerViewerEntityIds(_nativeEntityId, ghIds.AddrOfPinnedObject(), 64, ghCount.AddrOfPinnedObject());
        }
        finally
        {
            ghIds.Free();
            ghCount.Free();
        }

        int count = countBuf[0];
        var viewers = new List<HumanEntity>();
        for (int i = 0; i < count; i++)
        {
            var player = FourKit.GetPlayerByEntityId(ids[i]);
            if (player != null)
                viewers.Add(player);
        }
        return viewers;
    }

    /// <summary>
    /// Returns an iterator starting at the given index.
    /// </summary>
    /// <param name="index">The index.</param>
    /// <returns>An enumerator.</returns>
    public IEnumerator<ItemStack> iterator(int index)
    {
        EnsureSynced();
        int start = index >= 0 ? index : _items.Length + index;
        return GetEnumeratorFrom(start);
    }

    /// <inheritdoc/>
    public IEnumerator<ItemStack> GetEnumerator()
    {
        EnsureSynced();
        return GetEnumeratorFrom(0);
    }

    IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();

    private IEnumerator<ItemStack> GetEnumeratorFrom(int start)
    {
        for (int i = start; i < _items.Length; i++)
            if (_items[i] != null)
            {
                _items[i]!.BindToInventory(this, i);
                yield return _items[i]!;
            }
    }
}
