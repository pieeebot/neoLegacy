namespace Minecraft.Server.FourKit.Entity;

using System.Runtime.InteropServices;
using Minecraft.Server.FourKit.Inventory;

/// <summary>
/// Represents a human entity in the world (e.g. a player).
/// </summary>
public abstract class HumanEntity : LivingEntity, InventoryHolder
{
    private GameMode _gameMode = GameMode.SURVIVAL;
    private string _name = string.Empty;
    internal PlayerInventory _playerInventory = new();
    internal EnderChestInventory? _enderChestInventory;
    private ItemStack? _cursorItem;
    private bool _sleeping;
    private int _sleepTicks;

    /// <summary>
    /// Gets this human's current <see cref="GameMode"/>.
    /// </summary>
    /// <returns>The current game mode.</returns>
    public GameMode getGameMode() => _gameMode;

    /// <summary>
    /// Returns the name of this player.
    /// </summary>
    /// <returns>The display name.</returns>
    public string getName() => _name;

    /// <summary>
    /// Sets this human's current <see cref="GameMode"/>.
    /// </summary>
    /// <param name="mode">The new game mode.</param>
    public void setGameMode(GameMode mode)
    {
        NativeBridge.SetPlayerGameMode?.Invoke(getEntityId(), (int)mode);
    }

    /// <summary>
    /// Get the player's inventory.
    /// </summary>
    /// <returns>The inventory of the player, this also contains the armor slots.</returns>
    Inventory InventoryHolder.getInventory() => getInventory();

    /// <summary>
    /// Get the player's inventory.
    /// This also contains the armor slots.
    /// </summary>
    /// <returns>The player's inventory.</returns>
    public PlayerInventory getInventory()
    {
        return _playerInventory;
    }

    /// <summary>
    /// Get the player's EnderChest inventory.
    /// </summary>
    /// <returns>The EnderChest of the player.</returns>
    public Inventory getEnderChest()
    {
        // AAAAAH
        _enderChestInventory ??= new EnderChestInventory(getEntityId());
        return _enderChestInventory;
    }

    /// <summary>
    /// Returns the ItemStack currently in your hand, can be empty.
    /// </summary>
    /// <returns>The ItemStack of the item you are currently holding.</returns>
    public ItemStack? getItemInHand()
    {
        return _playerInventory.getItemInHand();
    }

    /// <summary>
    /// Sets the item to the given ItemStack, this will replace whatever the
    /// user was holding.
    /// </summary>
    /// <param name="item">The ItemStack which will end up in the hand.</param>
    public void setItemInHand(ItemStack? item)
    {
        _playerInventory.setItemInHand(item);
    }

    /// <summary>
    /// Returns the ItemStack currently on your cursor, can be empty.
    /// Will always be empty if the player currently has no open window.
    /// </summary>
    /// <returns>The ItemStack of the item you are currently moving around.</returns>
    public ItemStack? getItemOnCursor()
    {
        if (NativeBridge.GetCarriedItem != null)
        {
            int[] buf = new int[3];
            var gh = System.Runtime.InteropServices.GCHandle.Alloc(buf, System.Runtime.InteropServices.GCHandleType.Pinned);
            try
            {
                NativeBridge.GetCarriedItem(getEntityId(), gh.AddrOfPinnedObject());
            }
            finally
            {
                gh.Free();
            }
            int id = buf[0];
            int aux = buf[1];
            int count = buf[2];
            if (id > 0 && count > 0)
                _cursorItem = new ItemStack(id, count, (short)aux);
            else
                _cursorItem = null;
        }
        return _cursorItem;
    }

    /// <summary>
    /// Sets the item to the given ItemStack, this will replace whatever the
    /// user was moving. Will always be empty if the player currently has no open window.
    /// </summary>
    /// <param name="item">The ItemStack which will end up in the hand.</param>
    public void setItemOnCursor(ItemStack? item)
    {
        _cursorItem = item;
        NativeBridge.SetCarriedItem?.Invoke(getEntityId(),
            item?.getTypeId() ?? 0,
            item?.getAmount() ?? 0,
            item?.getDurability() ?? 0);
    }

    /// <summary>
    /// If the player currently has an inventory window open, this method will
    /// close it on both the server and client side.
    /// </summary>
    public void closeInventory()
    {
        NativeBridge.CloseContainer?.Invoke(getEntityId());
    }

    /// <summary>
    /// Opens an inventory window with the specified inventory on the top.
    /// </summary>
    /// <param name="inventory">The inventory to open.</param>
    /// <returns>The newly opened InventoryView, or null if it could not be opened.</returns>
    public InventoryView? openInventory(Inventory inventory)
    {
        if (NativeBridge.OpenVirtualContainer == null)
            return null;

        closeInventory();

        int nativeType = inventory.getType() switch
        {
            InventoryType.CHEST => 0,
            InventoryType.DISPENSER => 3,
            InventoryType.DROPPER => 10,
            InventoryType.HOPPER => 5,
            _ => 0,
        };

        int size = inventory.getSize();
        int[] buf = new int[size * 3];
        for (int i = 0; i < size; i++)
        {
            var item = inventory._items[i];
            buf[i * 3] = item?.getTypeId() ?? 0;
            buf[i * 3 + 1] = item?.getAmount() ?? 0;
            buf[i * 3 + 2] = item?.getDurability() ?? 0;
        }

        string title = inventory.getName();
        int titleByteLen = System.Text.Encoding.UTF8.GetByteCount(title);
        IntPtr titlePtr = Marshal.StringToCoTaskMemUTF8(title);
        var gh = GCHandle.Alloc(buf, GCHandleType.Pinned);
        try
        {
            NativeBridge.OpenVirtualContainer(getEntityId(), nativeType, titlePtr, titleByteLen, size, gh.AddrOfPinnedObject());
        }
        finally
        {
            gh.Free();
            Marshal.FreeCoTaskMem(titlePtr);
        }

        var view = new InventoryView(inventory, getInventory(), this, inventory.getType());
        return view;
    }

    internal void SetGameModeInternal(GameMode mode) => _gameMode = mode;

    internal void SetNameInternal(string name) => _name = name;

    internal void SetSleepingInternal(bool sleeping) => _sleeping = sleeping;

    internal void SetSleepTicksInternal(int ticks) => _sleepTicks = ticks;

    /// <summary>
    /// Returns whether this player is slumbering.
    /// </summary>
    /// <returns>slumber state</returns>
    public bool isSleeping() => _sleeping;

    /// <summary>
    /// Get the sleep ticks of the player. This value may be capped.
    /// </summary>
    /// <returns>slumber ticks</returns>
    public int getSleepTicks() => _sleepTicks;
}
