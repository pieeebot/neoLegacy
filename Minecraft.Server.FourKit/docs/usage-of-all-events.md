@page usage-of-all-events Usage of all Events

This page covers every event that FourKit provides, with descriptions of what they do and code examples showing how to use them. Every event handler method must be `public`, return `void`, take a single event parameter, and be annotated with `[EventHandler]` inside a class that implements \ref Minecraft.Server.FourKit.Event.Listener "Listener".

Events that implement \ref Minecraft.Server.FourKit.Event.Cancellable "Cancellable" can be cancelled by calling `setCancelled(true)`. A cancelled event will not execute its default server action (such as preventing chatting), but will still be passed to other plugins (unless IgnoreCancelled is set to true for a certain event handler)

> **Theres no guarantee this will be fully up to date as new events get added. Be sure to check the actual API documentation. Also some of these functions may not work as intended, as we are still in the process of rewriting everything. If something doesnt work, let us know on github.**

---

@section player_events Player Events

@subsection playerloginevent PlayerLoginEvent

\ref Minecraft.Server.FourKit.Event.Player.PlayerLoginEvent "PlayerLoginEvent" is fired when a player is about to log in, after pre-login checks but before the join event. You can inspect or modify the player's XUIDs, name, and IP address, and cancel the login. The XUID API is **experimental**: you can set and get the raw XUID values. The offline XUID is the main XUID used by the client; the online XUID is used for guests (splitscreen users).

```csharp
[EventHandler]
public void onLogin(PlayerLoginEvent e)
{
    // block a specific XUID
    if (e.getOfflineXuid() == 12345678901234567UL)
    {
        e.setCancelled(true);
    }

    // change the XUID (experimental)
    e.setOfflineXuid(98765432109876543UL);
}
```

| Method | Description |
|--------|-------------|
| `getName()` | The player's username attempting to log in. |
| `getAddress()` | The \ref Minecraft.Server.FourKit.Net.InetSocketAddress "InetSocketAddress" of the connection. |
| `getLoginType()` | The login type (e.g. online, offline). |
| `getOfflineXuid()` | **Experimental.** The main XUID used by the client. |
| `setOfflineXuid(ulong)` | **Experimental.** Set the offline XUID. |
| `getOnlineXuid()` | **Experimental.** The XUID used for guests (splitscreen users). |
| `setOnlineXuid(ulong)` | **Experimental.** Set the online XUID. |
| `hasChangedXuidValues()` | **Experimental.** True if XUID values were changed. |
| `isCancelled()` | Whether the login is cancelled. |
| `setCancelled(bool)` | Cancel or allow the login. |

> **Cancellable:** Yes

@subsection playerpreloginevent PlayerPreLoginEvent

\ref Minecraft.Server.FourKit.Event.Player.PlayerPreLoginEvent "PlayerPreLoginEvent" is fired before a player is allowed to join the server. You can inspect the players name and IP address, and cancel the event to prevent them from joining (such as for bans, whitelists, etc).

```csharp
[EventHandler]
public void onPreLogin(PlayerPreLoginEvent e)
{
    // block by name
    if (e.getName() == "Dumb")
    {
        e.setCancelled(true);
    }

    // block by ip
    if (e.getAddress().getHostAddress() == "127.0.0.1")
    {
        e.setCancelled(true);
    }
}
```

| Method | Description |
|--------|-------------|
| `getName()` | The player's username attempting to join. |
| `getAddress()` | The \ref Minecraft.Server.FourKit.Net.InetSocketAddress "InetSocketAddress" of the connection. |
| `isCancelled()` | Whether the login is cancelled. |
| `setCancelled(bool)` | Cancel or allow the login. |

> **Cancellable:** Yes

@subsection playerjoinevent PlayerJoinEvent

\ref Minecraft.Server.FourKit.Event.Player.PlayerJoinEvent "PlayerJoinEvent" is fired when a player joins the server. You can read or change the join message that is broadcast to all online players.

```csharp
[EventHandler]
public void onJoin(PlayerJoinEvent e)
{
    e.setJoinMessage("Welcome, " + e.getPlayer().getName() + "!");
}
```

| Method | Description |
|--------|-------------|
| `getPlayer()` | The player who joined. |
| `getJoinMessage()` | The join message that will be broadcast. |
| `setJoinMessage(string)` | Change or suppress the join message. Pass `null` to suppress. |

> **Cancellable:** No

---

@subsection playerquitevent PlayerQuitEvent

\ref Minecraft.Server.FourKit.Event.Player.PlayerQuitEvent "PlayerQuitEvent" is fired when a player disconnects from the server. You can change the quit message or suppress it.

```csharp
[EventHandler]
public void onQuit(PlayerQuitEvent e)
{
    e.setQuitMessage(e.getPlayer().getName() + " has left.");
}
```

| Method | Description |
|--------|-------------|
| `getPlayer()` | The player who left. |
| `getQuitMessage()` | The quit message that will be broadcast. |
| `setQuitMessage(string)` | Change or suppress the quit message. Pass `null` to suppress. |

> **Cancellable:** No

---

@subsection playerkickevent PlayerKickEvent

\ref Minecraft.Server.FourKit.Event.Player.PlayerKickEvent "PlayerKickEvent" is fired when a player is about to be kicked. Cancelling this event keeps the player connected.

```csharp
[EventHandler]
public void onKick(PlayerKickEvent e)
{
    // Prevent all kicks
    e.setCancelled(true);
}
```

| Method | Description |
|--------|-------------|
| `getPlayer()` | The player being kicked. |
| `getReason()` | The \ref Minecraft.Server.FourKit.Entity.DisconnectReason "DisconnectReason". |
| `setReason(DisconnectReason)` | Change the kick reason. |
| `getLeaveMessage()` | The message broadcast to other players. |
| `setLeaveMessage(string)` | Change the broadcast message. |

> **Cancellable:** Yes

---

@subsection playerchatevent PlayerChatEvent

\ref Minecraft.Server.FourKit.Event.Player.PlayerChatEvent "PlayerChatEvent" is fired when a player sends a chat message. You can modify the message, change the format, or cancel it entirely.

Format is the same as bukkit, with `%1$s` being the player name and ` %2$s` being the message.

```csharp
[EventHandler]
public void onChat(PlayerChatEvent e)
{
    // [PlayerName] message
    e.setFormat("[%1$s] %2$s");

    // or modify the message itself
    e.setMessage(e.getMessage().ToUpper()); // ALL CAPS
}
```

```csharp
[EventHandler]
public void onMute(PlayerChatEvent e)
{
    // Mute a specific player
    if (e.getPlayer().getName() == "DumbLoserFatty123")
    {
        e.setCancelled(true);
        e.getPlayer().sendMessage("You are muted!");
    }
}
```

| Method | Description |
|--------|-------------|
| `getPlayer()` | The player who sent the message. |
| `getMessage()` | The chat message. |
| `setMessage(string)` | Change the chat message. |
| `getFormat()` | The format string (e.g. `"<%1$s> %2$s"`). |
| `setFormat(string)` | Change the format string. |

> **Cancellable:** Yes

---

@subsection playermoveevent PlayerMoveEvent

\ref Minecraft.Server.FourKit.Event.Player.PlayerMoveEvent "PlayerMoveEvent" is fired whenever a player moves. You can redirect the player or cancel the movement.

```csharp
[EventHandler]
public void onMove(PlayerMoveEvent e)
{
    // Freeze players in place
    e.setCancelled(true);
}
```

```csharp
[EventHandler]
public void onMove(PlayerMoveEvent e)
{
    // prevent players from going above Y=100
    Location to = e.getTo();
    if (to.getY() > 100)
    {
        to.setY(100);
        e.setTo(to);
    }
}
```

| Method | Description |
|--------|-------------|
| `getPlayer()` | The player who moved. |
| `getFrom()` | The location the player moved from. |
| `getTo()` | The location the player is moving to. |
| `setFrom(Location)` | Override the from location. |
| `setTo(Location)` | Redirect the player to a different destination. |

> **Cancellable:** Yes

---

@subsection playerteleportevent PlayerTeleportEvent

\ref Minecraft.Server.FourKit.Event.Player.PlayerTeleportEvent "PlayerTeleportEvent" extends `PlayerMoveEvent` and is fired when a player teleports. It includes a \ref Minecraft.Server.FourKit.Event.Player.PlayerTeleportEvent.TeleportCause "TeleportCause" describing why the teleport happened.

```csharp
[EventHandler]
public void onTeleport(PlayerTeleportEvent e)
{
    if (e.getCause() == PlayerTeleportEvent.TeleportCause.ENDER_PEARL)
    {
        e.setCancelled(true);
        e.getPlayer().sendMessage("ender pearls are a big nono!");
    }
}
```

| Method | Description |
|--------|-------------|
| `getPlayer()` | The player who teleported. |
| `getFrom()` | The origin location. |
| `getTo()` | The destination location. |
| `setTo(Location)` | Redirect the teleport destination. |
| `getCause()` | The `TeleportCause` (`ENDER_PEARL`, `COMMAND`, `PLUGIN`, `NETHER_PORTAL`, `END_PORTAL`, `UNKNOWN`). |

> **Cancellable:** Yes (inherited from `PlayerMoveEvent`)

---

@subsection playerportalevent PlayerPortalEvent

\ref Minecraft.Server.FourKit.Event.Player.PlayerPortalEvent "PlayerPortalEvent" extends `PlayerTeleportEvent` and is fired when a player enters a portal (nether or end). You can cancel it to prevent dimension travel.

```csharp
[EventHandler]
public void onPortal(PlayerPortalEvent e)
{
    // no nether travel
    if (e.getCause() == PlayerTeleportEvent.TeleportCause.NETHER_PORTAL)
    {
        e.setCancelled(true);
        e.getPlayer().sendMessage("you cannot go to the nether!!!");
    }
}
```

| Method | Description |
|--------|-------------|
| `getPlayer()` | The player entering the portal. |
| `getFrom()` | Where the player is coming from. |
| `getTo()` | Where the player will arrive. |
| `setTo(Location)` | Override the destination. |
| `getCause()` | The teleport cause. |

> **Cancellable:** Yes (inherited)

---

@subsection playerdropitemevent PlayerDropItemEvent

\ref Minecraft.Server.FourKit.Event.Player.PlayerDropItemEvent "PlayerDropItemEvent" is fired when a player drops an item from their inventory. You can cancel it or change what item is dropped.

```csharp
[EventHandler]
public void onDrop(PlayerDropItemEvent e)
{
    // do not let a user drop diamonds
    if (e.getItemDrop().getType() == Material.DIAMOND)
    {
        e.setCancelled(true);
        e.getPlayer().sendMessage("You can't drop that!");
    }
}
```

```csharp
[EventHandler]
public void onDrop(PlayerDropItemEvent e)
{
    // replace dropped item with dirt
    e.setItemDrop(new ItemStack(Material.DIRT, 1));
}
```

| Method | Description |
|--------|-------------|
| `getPlayer()` | The player who dropped the item. |
| `getItemDrop()` | The `ItemStack` being dropped. |
| `setItemDrop(ItemStack)` | Change which item is dropped. |

> **Cancellable:** Yes

---

@subsection playerpickupitemevent PlayerPickupItemEvent

\ref Minecraft.Server.FourKit.Event.Player.PlayerPickupItemEvent "PlayerPickupItemEvent" is fired when a player picks up an item from the ground.

```csharp
[EventHandler]
public void onPickup(PlayerPickupItemEvent e)
{
    // disable picking up items
    e.setCancelled(true);
}
```

| Method | Description |
|--------|-------------|
| `getPlayer()` | The player picking up the item. |
| `getItem()` | The \ref Minecraft.Server.FourKit.Entity.Item "Item" entity on the ground. |

> **Cancellable:** Yes

---

@subsection playerinteractevent PlayerInteractEvent

\ref Minecraft.Server.FourKit.Event.Player.PlayerInteractEvent "PlayerInteractEvent" is fired when a player interacts with an object, block, or air. The \ref Minecraft.Server.FourKit.Block.Action "Action" enum tells you what kind of click it was.

```csharp
[EventHandler]
public void onInteract(PlayerInteractEvent e)
{
    if (e.getAction() == Action.RIGHT_CLICK_BLOCK && e.hasBlock())
    {
        e.getPlayer().sendMessage("You clicked on " + e.getClickedBlock().getType());
    }
}
```

```csharp
[EventHandler]
public void onInteract(PlayerInteractEvent e)
{
    // no using flint and steel
    if (e.getMaterial() == Material.FLINT_AND_STEEL)
    {
        e.setCancelled(true);
    }
}
```

| Method | Description |
|--------|-------------|
| `getPlayer()` | The player who interacted. |
| `getAction()` | The `Action` (`LEFT_CLICK_AIR`, `LEFT_CLICK_BLOCK`, `RIGHT_CLICK_AIR`, `RIGHT_CLICK_BLOCK`, `PHYSICAL`). |
| `getItem()` | The `ItemStack` in the player's hand, or `null`. |
| `getMaterial()` | CONVENIENCE. the `Material` of the held item, or `AIR`. |
| `hasBlock()` | Whether a block was involved. |
| `hasItem()` | Whether an item was in hand. |
| `isBlockInHand()` | Whether the held item is a block (id 1-255). |
| `getClickedBlock()` | The `Block` that was clicked, or `null`. |
| `getBlockFace()` | The `BlockFace` that was clicked. |
| `useItemInHand()` | Whether the item in hand should be used. |
| `setUseItemInHand(bool)` | Override whether the item in hand is used. |

> **Cancellable:** Yes

---

@subsection playerinteractentityevent PlayerInteractEntityEvent

\ref Minecraft.Server.FourKit.Event.Player.PlayerInteractEntityEvent "PlayerInteractEntityEvent" is fired when a player right-clicks an entity.

```csharp
[EventHandler]
public void onInteractEntity(PlayerInteractEntityEvent e)
{
    e.getPlayer().sendMessage("You interacted with entity id " + e.getRightClicked().getEntityId());
}
```

```csharp
[EventHandler]
public void onInteractEntity(PlayerInteractEntityEvent e)
{
    // do not let user interact with a villager
    if (e.getRightClicked().getType() == EntityType.VILLAGER)
    {
        e.setCancelled(true);
    }
}
```

| Method | Description |
|--------|-------------|
| `getPlayer()` | The player who interacted. |
| `getRightClicked()` | The entity that was right-clicked. |

> **Cancellable:** Yes

---

@subsection playerbedenterevent PlayerBedEnterEvent

\ref Minecraft.Server.FourKit.Event.Player.PlayerBedEnterEvent "PlayerBedEnterEvent" is fired when a player is about to enter a bed.

```csharp
[EventHandler]
public void onBedEnter(PlayerBedEnterEvent e)
{
    e.setCancelled(true);
    e.getPlayer().sendMessage("Sleeping is disabled on this server.");
}
```

| Method | Description |
|--------|-------------|
| `getPlayer()` | The player entering the bed. |
| `getBed()` | The bed `Block`. |

> **Cancellable:** Yes

---

@subsection playerbedleaveevent PlayerBedLeaveEvent

\ref Minecraft.Server.FourKit.Event.Player.PlayerBedLeaveEvent "PlayerBedLeaveEvent" is fired when a player leaves a bed.

```csharp
[EventHandler]
public void onBedLeave(PlayerBedLeaveEvent e)
{
    e.getPlayer().sendMessage("Bro left the bed");
}
```

| Method | Description |
|--------|-------------|
| `getPlayer()` | The player leaving the bed. |
| `getBed()` | The bed `Block`. |

> **Cancellable:** No

---

@subsection playercommandpreprocessevent PlayerCommandPreprocessEvent

\ref Minecraft.Server.FourKit.Event.Player.PlayerCommandPreprocessEvent "PlayerCommandPreprocessEvent" is fired early in the command handling process when a player sends a command. You can modify the command, cancel it, or use it for logging. This fires before the command is dispatched to any command handler.

```csharp
[EventHandler]
public void onCommand(PlayerCommandPreprocessEvent e)
{
    // log all commands
    Console.WriteLine(e.getPlayer().getName() + " issued command: " + e.getMessage());
}
```

```csharp
[EventHandler]
public void onCommand(PlayerCommandPreprocessEvent e)
{
    // block a specific command
    if (e.getMessage().StartsWith("/secret"))
    {
        e.setCancelled(true);
        e.getPlayer().sendMessage("That command is disabled!");
    }
}
```

```csharp
[EventHandler]
public void onCommand(PlayerCommandPreprocessEvent e)
{
    // redirect a command alias
    if (e.getMessage().StartsWith("/gm "))
    {
        e.setMessage("/gamemode " + e.getMessage().Substring(4));
    }
}
```

| Method | Description |
|--------|-------------|
| `getPlayer()` | The player who issued the command. |
| `getMessage()` | The full command string the player is sending. |
| `setMessage(string)` | Change the command that will be processed. |
| `isCancelled()` | Whether the command is cancelled. |
| `setCancelled(bool)` | Cancel or allow the command. |

> **Cancellable:** Yes

---

@section entity_events Entity Events

@subsection entitydamageevent EntityDamageEvent

\ref Minecraft.Server.FourKit.Event.Entity.EntityDamageEvent "EntityDamageEvent" is fired when any entity takes damage. The entity can be a `Player` or any `LivingEntity`. Use `getEntityType()` to check before casting.

```csharp
[EventHandler]
public void onDamage(EntityDamageEvent e)
{
    // halve damage
    e.setDamage(e.getDamage() / 2);
}
```

```csharp
[EventHandler]
public void onDamage(EntityDamageEvent e)
{
    // no fall damage
    if (e.getEntityType() == EntityType.PLAYER &&
        e.getCause() == EntityDamageEvent.DamageCause.FALL)
    {
        e.setCancelled(true);
    }
    
    // another method for negating fall damage is in playermoveevent and using player.setFallDistance(0)
}
```

```csharp
[EventHandler]
public void onDamage(EntityDamageEvent e)
{
    // cast to living entity
    LivingEntity entity = (LivingEntity)e.getEntity();
    if (entity.getHealth() - e.getFinalDamage() <= 0)
    {
        // entity is going to die
    }
}
```

| Method | Description |
|--------|-------------|
| `getEntity()` | The entity that was damaged. |
| `getEntityType()` | The `EntityType` of the damaged entity. |
| `getCause()` | The `DamageCause` (e.g. `FALL`, `FIRE`, `ENTITY_ATTACK`, `VOID`, `DROWNING`, etc.). |
| `getDamage()` | The raw damage amount. |
| `setDamage(double)` | Change the raw damage amount. |
| `getFinalDamage()` | The damage after reductions. |

> **Cancellable:** Yup. cancelling prevents any damage.

---

@subsection entitydamagebyentityevent EntityDamageByEntityEvent

\ref Minecraft.Server.FourKit.Event.Entity.EntityDamageByEntityEvent "EntityDamageByEntityEvent" extends `EntityDamageEvent` and is fired when an entity is damaged by another entity (e.g. player hits a mob, or mob hits a player).

```csharp
[EventHandler]
public void onPvP(EntityDamageByEntityEvent e)
{
    // no PVP
    if (e.getEntity().getType() == EntityType.PLAYER &&
        e.getDamager().getType() == EntityType.PLAYER)
    {
        e.setCancelled(true);
        Player attacker = (Player)e.getDamager();
        attacker.sendMessage("PvP is disabled!");
    }
}
```

| Method | Description |
|--------|-------------|
| `getEntity()` | The entity that was damaged. |
| `getDamager()` | The entity that dealt the damage. |
| `getCause()` | The `DamageCause`. |
| `getDamage()` / `setDamage(double)` | Read/modify the damage. |

> **Cancellable:** Yes

---

@subsection playerdeathevent PlayerDeathEvent

\ref Minecraft.Server.FourKit.Event.Entity.PlayerDeathEvent "PlayerDeathEvent" extends `EntityDeathEvent` and is fired when a player dies. It adds control over the death message, respawn experience, and keep-inventory.

```csharp
[EventHandler]
public void onPlayerDeath(PlayerDeathEvent e)
{
    e.setDeathMessage(e.getEntity().getName() + " was eliminated!");
    e.setKeepInventory(true); // player keeps their items
    e.setKeepLevel(true); // player keeps their XP level
    e.setDroppedExp(0); // dont drop any XP orbs
}
```

```csharp
[EventHandler]
public void onPlayerDeath(PlayerDeathEvent e)
{
    // clear all item drops and set new respawn XP
    e.getDrops().Clear();
    e.setNewExp(0);
    e.setNewLevel(0);
}
```

| Method | Description |
|--------|-------------|
| `getEntity()` | The `Player` who died. |
| `getDeathMessage()` | The death message shown to all players. |
| `setDeathMessage(string)` | Change the death message. |
| `getDroppedExp()` / `setDroppedExp(int)` | XP orbs dropped on death. |
| `getDrops()` | Mutable list of item drops. |
| `getNewExp()` / `setNewExp(int)` | XP the player has after respawning. |
| `getNewLevel()` / `setNewLevel(int)` | Level the player has after respawning. |
| `getKeepLevel()` / `setKeepLevel(bool)` | Whether the player keeps their XP level. Overrides other XP settings. |
| `getKeepInventory()` / `setKeepInventory(bool)` | Whether the player keeps their inventory on death. |

> **Cancellable:** No

---


@section world_events World Events

@subsection structuregrowevent StructureGrowEvent

\ref Minecraft.Server.FourKit.Event.World.StructureGrowEvent "StructureGrowEvent" is fired when an organic structure attempts to grow (Sapling -> Tree), (Mushroom -> Huge Mushroom), either naturally or using bonemeal. You can cancel it, inspect the location, species, and optionally the player who caused it.

```csharp
[EventHandler]
public void onStructureGrow(StructureGrowEvent e)
{
    // prevent huge mushrooms from growing
    if (e.getSpecies() == TreeType.BROWN_MUSHROOM || e.getSpecies() == TreeType.RED_MUSHROOM)
    {
        e.setCancelled(true);
    }

    // send a message if grown by player
    var player = e.getPlayer();
    if (player != null)
    {
        player.sendMessage($"You grew a {e.getSpecies()} at {e.getLocation()}");
    }
}
```

| Method | Description |
|--------|-------------|
| `getLocation()` | The location of the structure. |
| `getSpecies()` | The species type (birch, normal, pine, red mushroom, brown mushroom). |
| `isFromBonemeal()` | True if the structure was grown using bonemeal. |
| `getPlayer()` | The player that created the structure, null if not created manually. |
| `isCancelled()` | Whether the event is cancelled. |
| `setCancelled(bool)` | Cancel or allow the structure growth. |

> **Cancellable:** Yes

---
@section block_events Block Events

@subsection blockbreakevent BlockBreakEvent

\ref Minecraft.Server.FourKit.Event.Block.BlockBreakEvent "BlockBreakEvent" is fired when a player breaks a block. You can cancel it, change the XP dropped, or inspect the block.

```csharp
[EventHandler]
public void onBreak(BlockBreakEvent e)
{
    // protect stone
    if (e.getBlock().getType() == Material.STONE)
    {
        e.setCancelled(true);
        e.getPlayer().sendMessage("You can't break stone!");
    }
}
```

```csharp
[EventHandler]
public void onBreak(BlockBreakEvent e)
{
    // drop 15 XP for any block broken in survival
    e.setExpToDrop(15);
}
```

| Method | Description |
|--------|-------------|
| `getBlock()` | The `Block` being broken. |
| `getPlayer()` | The player who broke the block. |
| `getExpToDrop()` | The XP that will be dropped. |
| `setExpToDrop(int)` | Set the XP dropped (1+ to drop, 0 for none). |

> **Cancellable:** Yes

---

@subsection blockplaceevent BlockPlaceEvent

\ref Minecraft.Server.FourKit.Event.Block.BlockPlaceEvent "BlockPlaceEvent" is fired when a player places a block. You can cancel the placement or change the placed block.

```csharp
[EventHandler]
public void onPlace(BlockPlaceEvent e)
{
    // replace everything placed with stone
    e.getBlock().setType(Material.STONE);
}
```

```csharp
[EventHandler]
public void onPlace(BlockPlaceEvent e)
{
    // NO TNT
    if (e.getBlock().getType() == Material.TNT)
    {
        e.setCancelled(true);
        e.getPlayer().sendMessage("TNT is NOT ALLOWED!");
    }
}
```

| Method | Description |
|--------|-------------|
| `getBlock()` / `getBlockPlaced()` | The `Block` that was placed. |
| `getBlockAgainst()` | The block this was placed against. |
| `getItemInHand()` | The `ItemStack` used to place the block. |
| `getPlayer()` | The player who placed the block. |

> **Cancellable:** Yes

---

@subsection signchangeevent SignChangeEvent

\ref Minecraft.Server.FourKit.Event.Block.SignChangeEvent "SignChangeEvent" is fired when a player edits a sign. You can read, modify, or cancel the sign text.

```csharp
[EventHandler]
public void onSign(SignChangeEvent e)
{
    // log what was written in the sign
    for (int i = 0; i < 4; i++)
    {
        Console.WriteLine($"Line {i}: {e.getLine(i)}");
    }
}
```

```csharp
[EventHandler]
public void onSign(SignChangeEvent e)
{
    // censor a word
    for (int i = 0; i < 4; i++)
    {
        if (e.getLine(i).Contains("shit"))
            e.setLine(i, "****");
    }
}
```

| Method | Description |
|--------|-------------|
| `getBlock()` | The sign `Block`. |
| `getPlayer()` | The player who edited the sign. |
| `getLines()` | All 4 lines as a `string[]`. |
| `getLine(int)` | A single line (0-3). |
| `setLine(int, string)` | Change a single line (0-3). |

> **Cancellable:** Yes

---


---

@subsection blockgrowevent BlockGrowEvent

\ref Minecraft.Server.FourKit.Event.Block.BlockGrowEvent "BlockGrowEvent" is fired when a block grows naturally in the world. This includes crops (wheat, nether wart, cocoa beans), sugar cane, cactus, and melon/pumpkin fruit placement.

```csharp
[EventHandler]
public void onGrow(BlockGrowEvent e)
{
    // prevent all crop growth
    e.setCancelled(true);
}
```

```csharp
[EventHandler]
public void onGrow(BlockGrowEvent e)
{
    // log growth events
    var b = e.getBlock();
    Console.WriteLine($"Block grew at {b.getX()}, {b.getY()}, {b.getZ()} type={b.getType()}");
}
```

| Method | Description |
|--------|-------------|
| `getBlock()` | The `Block` that is growing. |
| `getNewState()` | The `BlockState` representing what the block will become. |
| `isCancelled()` | Whether the growth is cancelled. |
| `setCancelled(bool)` | Cancel or allow the growth. |

> **Cancellable:** Yes

---

@subsection blockformevent BlockFormEvent

\ref Minecraft.Server.FourKit.Event.Block.BlockFormEvent "BlockFormEvent" extends `BlockGrowEvent` and is fired when a block forms due to world conditions. Examples include snow forming during a storm and ice forming in cold biomes. Use \ref Minecraft.Server.FourKit.Event.Block.BlockSpreadEvent "BlockSpreadEvent" to catch blocks that actually spread instead of randomly forming.

```csharp
[EventHandler]
public void onForm(BlockFormEvent e)
{
    // prevent snow and ice from forming
    e.setCancelled(true);
}
```

| Method | Description |
|--------|-------------|
| `getBlock()` | The `Block` that is forming. |
| `getNewState()` | The `BlockState` representing what the block will become. |
| `isCancelled()` | Whether the formation is cancelled. |
| `setCancelled(bool)` | Cancel or allow the formation. |

> **Cancellable:** Yes (inherited from `BlockGrowEvent`)

---

@subsection blockspreadevent BlockSpreadEvent

\ref Minecraft.Server.FourKit.Event.Block.BlockSpreadEvent "BlockSpreadEvent" extends `BlockFormEvent` and is fired when a block spreads from one location to another. Examples include fire spreading, mushrooms spreading, and grass spreading to dirt.

```csharp
[EventHandler]
public void onSpread(BlockSpreadEvent e)
{
    // prevent fire from spreading
    if (e.getSource().getType() == Material.FIRE)
    {
        e.setCancelled(true);
    }
}
```

```csharp
[EventHandler]
public void onSpread(BlockSpreadEvent e)
{
    // prevent grass from spreading
    if (e.getBlock().getType() == Material.GRASS)
    {
        e.setCancelled(true);
    }
}
```

| Method | Description |
|--------|-------------|
| `getBlock()` | The `Block` where the spread is occurring (destination). |
| `getSource()` | The source `Block` that is spreading. |
| `getNewState()` | The `BlockState` representing what the block will become. |
| `isCancelled()` | Whether the spread is cancelled. |
| `setCancelled(bool)` | Cancel or allow the spread. |

> **Cancellable:** Yes (inherited)

---

@subsection blockburnevent BlockBurnEvent

\ref Minecraft.Server.FourKit.Event.Block.BlockBurnEvent "BlockBurnEvent" is fired when a block is destroyed as a result of being burnt by fire.

```csharp
[EventHandler]
public void onBurn(BlockBurnEvent e)
{
    // prevent wooden planks from burning
    if (e.getBlock().getType() == Material.WOOD)
    {
        e.setCancelled(true);
    }
}
```

```csharp
[EventHandler]
public void onBurn(BlockBurnEvent e)
{
    // prevent all fire destruction
    e.setCancelled(true);
}
```

| Method | Description |
|--------|-------------|
| `getBlock()` | The `Block` that is being burnt. |
| `isCancelled()` | Whether the burn is cancelled. |
| `setCancelled(bool)` | Cancel or allow the burn. |

> **Cancellable:** Yes

---

@subsection blockfromtoevent BlockFromToEvent

\ref Minecraft.Server.FourKit.Event.Block.BlockFromToEvent "BlockFromToEvent" is fired when a block moves from one location to another. This currently only applies to liquid flow (lava and water) and teleporting dragon eggs.

```csharp
[EventHandler]
public void onFromTo(BlockFromToEvent e)
{
    // prevent water from flowing
    if (e.getBlock().getType() == Material.WATER || e.getBlock().getType() == Material.STATIONARY_WATER)
    {
        e.setCancelled(true);
    }
}
```

```csharp
[EventHandler]
public void onFromTo(BlockFromToEvent e)
{
    // log liquid flow
    var from = e.getBlock();
    var to = e.getToBlock();
    Console.WriteLine($"Block moving from {from.getX()},{from.getY()},{from.getZ()} to {to.getX()},{to.getY()},{to.getZ()}");
}
```

| Method | Description |
|--------|-------------|
| `getBlock()` | The source `Block` that is moving. |
| `getToBlock()` | The destination `Block`. |
| `getFace()` | The `BlockFace` direction the block is moving to. |
| `isCancelled()` | Whether the move is cancelled. |
| `setCancelled(bool)` | Cancel or allow the move. |

> **Cancellable:** Yes

---

@subsection blockpistonextendevent BlockPistonExtendEvent

\ref Minecraft.Server.FourKit.Event.Block.BlockPistonExtendEvent "BlockPistonExtendEvent" extends `BlockPistonEvent` and is fired when a piston extends. You can inspect the piston direction, stickiness, number of blocks being pushed, and cancel the extension.

```csharp
[EventHandler]
public void onPiston(BlockPistonExtendEvent e)
{
    // prevent sticky pistons from extending
    if (e.isSticky())
    {
        e.setCancelled(true);
    }
}
```

```csharp
[EventHandler]
public void onPiston(BlockPistonExtendEvent e)
{
    // log piston activity
    Console.WriteLine($"Piston extending {e.getDirection()} pushing {e.getLength()} blocks");
}
```

| Method | Description |
|--------|-------------|
| `getBlock()` | The piston `Block`. |
| `getDirection()` | The `BlockFace` direction the piston is extending. |
| `isSticky()` | Whether the piston is a sticky piston. |
| `getLength()` | The number of blocks being pushed. |
| `getBlocks()` | List of `Block` objects that will be moved. |
| `isCancelled()` | Whether the extension is cancelled. |
| `setCancelled(bool)` | Cancel or allow the extension. |

> **Cancellable:** Yes

---

@subsection blockpistonretractevent BlockPistonRetractEvent

\ref Minecraft.Server.FourKit.Event.Block.BlockPistonRetractEvent "BlockPistonRetractEvent" extends `BlockPistonEvent` and is fired when a piston retracts. For sticky pistons, the retract location indicates where the block being pulled is located.

```csharp
[EventHandler]
public void onPiston(BlockPistonRetractEvent e)
{
    // prevent all sticky piston retractions
    if (e.isSticky())
    {
        e.setCancelled(true);
    }
}
```

```csharp
[EventHandler]
public void onPiston(BlockPistonRetractEvent e)
{
    // log where the retract is pulling from
    var loc = e.getRetractLocation();
    Console.WriteLine($"Piston retracting, pull location: {loc.getBlockX()}, {loc.getBlockY()}, {loc.getBlockZ()}");
}
```

| Method | Description |
|--------|-------------|
| `getBlock()` | The piston `Block`. |
| `getDirection()` | The `BlockFace` direction the piston is retracting. |
| `isSticky()` | Whether the piston is a sticky piston. |
| `getRetractLocation()` | The `Location` of the block that may be pulled (for sticky pistons). |
| `isCancelled()` | Whether the retraction is cancelled. |
| `setCancelled(bool)` | Cancel or allow the retraction. |

> **Cancellable:** Yes

---

@section inventory_events Inventory Events

@subsection inventoryopenevent InventoryOpenEvent

\ref Minecraft.Server.FourKit.Event.Inventory.InventoryOpenEvent "InventoryOpenEvent" is fired when a player opens an inventory (chest, furnace, crafting table, etc.). Cancelling this prevents the inventory screen from showing.

```csharp
[EventHandler]
public void onOpen(InventoryOpenEvent e)
{
    e.getPlayer().sendMessage("You opened: " + e.getInventory().getName());
}
```

```csharp
[EventHandler]
public void onOpen(InventoryOpenEvent e)
{
    // Lock all chests
    if (e.getInventory().getType() == InventoryType.CHEST)
    {
        e.setCancelled(true);
    }
}
```

| Method | Description |
|--------|-------------|
| `getPlayer()` | The `HumanEntity` who opened the inventory. |
| `getInventory()` | The top `Inventory` that was opened. |
| `getView()` | The full `InventoryView`. |
| `getViewers()` | List of players viewing this inventory. |

> **Cancellable:** Yes

---

@subsection inventoryclickevent InventoryClickEvent

\ref Minecraft.Server.FourKit.Event.Inventory.InventoryClickEvent "InventoryClickEvent" is fired when a player clicks a slot in any open inventory. It provides detailed information about the click type, slot, and item.

> **Warning:** Do not call `closeInventory()` or `openInventory()` from inside this event handler. The inventory is mid-modification. You should do this in a seperate thread fired later on.

```csharp
[EventHandler]
public void onClick(InventoryClickEvent e)
{
    // do not let user take item from first slot
    if (e.getRawSlot() == 0)
    {
        e.setCancelled(true);
    }
}
```

```csharp
[EventHandler]
public void onClick(InventoryClickEvent e)
{
    // log every click
    Player player = (Player)e.getWhoClicked();
    Console.WriteLine($"{player.getName()} clicked slot {e.getSlot()} with {e.getClick()}");
}
```

| Method | Description |
|--------|-------------|
| `getWhoClicked()` | The `HumanEntity` who clicked. |
| `getClickedInventory()` | The `Inventory` that was clicked, or `null` if outside the window. |
| `getSlotType()` | The `SlotType` of the clicked slot. |
| `getCurrentItem()` | The `ItemStack` in the clicked slot. |
| `setCurrentItem(ItemStack)` | Change the item in the clicked slot. |
| `getCursor()` | The `ItemStack` on the cursor. |
| `getSlot()` | The slot index relative to the clicked inventory. |
| `getRawSlot()` | The raw slot number unique to the view. |
| `getClick()` | The `ClickType` (e.g. `LEFT`, `RIGHT`, `SHIFT_LEFT`). |
| `getAction()` | The `InventoryAction` describing the outcome. |
| `isLeftClick()` / `isRightClick()` / `isShiftClick()` | Convenience click-type checks. |
| `getHotbarButton()` | The hotbar key pressed (0-8), or -1 if not a `NUMBER_KEY` click. |
| `getInventory()` | The top inventory of the view. |
| `getView()` | The full `InventoryView`. |

> **Cancellable:** Yes


---

@section chunk_events Chunk Events

@subsection chunkloadevent ChunkLoadEvent

\ref Minecraft.Server.FourKit.Event.World.ChunkLoadEvent "ChunkLoadEvent" is fired when a chunk is loaded. If the chunk is newly generated it will not yet be populated when this event fires.

```csharp
[EventHandler]
public void onChunkLoad(ChunkLoadEvent e)
{
    if (e.isNewChunk())
    {
        Console.WriteLine($"New chunk generated at {e.getChunk().getX()}, {e.getChunk().getZ()}");
    }
}
```

| Method | Description |
|--------|-------------|
| `getChunk()` | The `Chunk` that was loaded. |
| `isNewChunk()` | True if this chunk was newly generated. Note that new chunks will not yet be populated at this time. |

> **Cancellable:** No

---

@subsection chunkunloadevent ChunkUnloadEvent

\ref Minecraft.Server.FourKit.Event.World.ChunkUnloadEvent "ChunkUnloadEvent" is fired when a chunk is about to be unloaded. You can cancel it to prevent the chunk from being unloaded.

```csharp
[EventHandler]
public void onChunkUnload(ChunkUnloadEvent e)
{
    // keep chunks near world origin loaded
    Chunk chunk = e.getChunk();
    if (Math.Abs(chunk.getX()) <= 2 && Math.Abs(chunk.getZ()) <= 2)
    {
        e.setCancelled(true);
    }
}
```

| Method | Description |
|--------|-------------|
| `getChunk()` | The `Chunk` that is about to be unloaded. |
| `isCancelled()` | Whether the unload is cancelled. |
| `setCancelled(bool)` | Cancel or allow the chunk unload. |

> **Cancellable:** Yes

---

<h1>Page currently under construction</h1>