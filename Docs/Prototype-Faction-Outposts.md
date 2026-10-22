# Milestone #14 — Faction outposts and finite reinforcements (UE 5.8)

## Design goal

The *Colonial Mercenaries* GDD's dynamic-war design states that factions
control regional infrastructure, expend resources, reinforce positions,
and **must not generate unlimited troops without plausible logistics**.
This milestone adds the first tangible, local manifestation of that rule.

An editor-placed `CMFactionOutpost` is a lightweight, visually represented
installation that deploys its garrison and can replace combat losses from
a strictly limited manpower reserve. Squad strategy, supplies/convoys,
persistent saving, and contracts remain later milestones.

## C++ implementation

- **New placeable actor:** `ACMFactionOutpost`, located at
  `Source/ColonialMercenaries/World/CMFactionOutpost.h/.cpp`.
- **No added assets:** an engine-mesh platform/structure, a direction arrow,
  and an overhead, colored status label make the outpost identifiable.
  These placeholders have **no collision**, preserving NavMesh movement.
- **Faction assignment:** `FactionId` is set on each placed outpost.
  `SoldierClass` defaults to the already-tested, animated
  `ACMCombatSoldier`. Every spawned soldier's faction is set on the
  **authority before BeginPlay** using deferred spawn.
- **Initial garrison:** default **3** living soldiers. Starting garrison
  **does not consume** the distinct reserve.
- **Target strength:** default **3** maximum living soldiers at one time.
- **Initial reserves:** default **9** additional soldiers. Each
  successfully spawned replacement subtracts exactly one from reserve.
- **Reinforcement wave:** after a default **20-second** cooldown, the
  outpost checks its own living soldiers on a **1-second server timer**.
  If casualties caused a vacancy, it deploys at most
  `ReinforcementWaveSize` replacements, never exceeding target strength,
  and starts another cooldown. Zero reserve = no further replacements.
  Partial losses can be filled without requiring the entire squad to die.
- **Liveness tracking:** only this outpost's weak soldier references
  are checked; health 0 is counted as lost even during corpse cleanup.
  No scanning of all world actors or per-actor Tick.
- **Deployment:** adjustable forward offset, lateral/row spacing,
  spawn height and formation width. Spawn collision is checked; a failed
  spawn does **not** spend reserve and is retried later.
- **Multiplayer:** the outpost replicates faction ID, current garrison,
  remaining reserve and total soldiers deployed. Soldiers remain normal,
  authoritative replicated combatants. The display updates through
  OnRep callbacks; there is no client-side troop spawning.
- **Future logistics seam:** `AddReserves(int32)` succeeds only on the
  server, making it possible for a future convoy or strategic simulation
  to deposit manpower. Nothing calls it automatically today.

## Unreal editor setup (after compiling the branch)

1. GitHub Desktop: **Fetch**, switch to
   `feature/faction-outposts-and-reinforcements`. Close Unreal while
   switching branches. Build UE 5.8 **Development Editor / Win64**.
2. Open `Lvl_FactionTest`.
3. **For a clean test**, delete or temporarily disable the two existing
   `CMCombatSquadSpawner` instances. Those remain fully functional
   one-time spawners, but leaving them in place would double the troops.
   Keep the central `CMContestedPoint`.
4. Place **two** actors named **CM Faction Outpost** (from C++ Classes >
   ColonialMercenaries > World, or search in Place Actors). Place them
   where the previous Red/Blue squad deployments were located, on open
   NavMesh, facing generally toward the contested point.
5. For the first: `Outpost Name = Red Forward Base`;
   `Faction Id = Faction.Prototype.Red`. For the second:
   `Outpost Name = Blue Forward Base`;
   `Faction Id = Faction.Prototype.Blue`.
   Other fields can remain at defaults for the initial test.
6. On the outpost actor, point the `DeploymentDirection` arrow toward
   the center. The forward deployment position begins **400 cm ahead**
   of the actor. Rotate the whole actor so this area is clear, flat,
   and part of connected NavMesh. If needed, tune
   `Deployment Forward Offset` / `Spawn Height`.
7. Play in **1-player PIE**. Verify each outpost spawns exactly
   **3** animated infantry. Their status labels should show faction,
   `Garrison 3/3` and `Reserve 9`. Existing Red/Blue autonomous
   hostilities should still occur.
8. Kill a soldier: after the 1-second server check, expect that
   outpost's garrison to fall to **2/3**. After up to 20 seconds
   (measured from level start/last successful wave), a replacement
   should appear at that base; it should return to **3/3** and
   decrease reserve to **8**. If all three die, the wave can bring back
   as many as three, spending three reserves.
9. Optional fast depletion test: stop PIE, set **Initial Reserves = 3**
   and **Reinforcement Cooldown Seconds = 5** on one base, and restart.
   Keep killing the replenished soldiers until reserves reach zero.
   Verify no new soldiers appear after that.
10. Play in **2-player PIE**. Confirm each outpost deploys soldiers
    only once on the server and both windows agree on reserve numbers,
    health/death, soldier appearance, and faction targeting.
11. Save `Lvl_FactionTest`; commit/push the modified map and its
    External Actor `.uasset` files from GitHub Desktop. Avoid
    editor-generated settings / unrelated files.

## Acceptance criteria

- [ ] UE 5.8 Editor/Win64 C++ build succeeds.
- [ ] Each base is visible with a readable faction/strength/reserve label.
- [ ] 3 Red + 3 Blue infantry spawn on authority, **not** separately per client.
- [ ] Death reduces active garrison without waiting for corpse expiration.
- [ ] Replacements appear after cooldown, never beyond target strength.
- [ ] A successful replacement consumes exactly one soldier from reserve.
- [ ] Reserve reaches zero and spawning ceases; no auto-restock.
- [ ] Both clients show the same outpost state and soldiers.
- [ ] Existing soldier movement, firefights, player damage/revival and capture
      point mechanics still work.

## Deliberately deferred

Outposts are not yet capturable or destructible, and they cannot claim
territory automatically. The **existing contested point** remains the sole
local capture objective. Soldiers still use existing independent combat
logic: they do not wait for their squad or follow strategic orders.
Milestone #15 will add formation and squad coordination; later work will
add logistics convoys, dynamic supply, production/purchase costs,
strategic battle budgets, persistent base state, and contract-linked
objectives. This is an isolated, finite **test-region** system, not
a complete dynamic-war simulation.

**PR must stay draft until local C++ compilation and PIE testing pass.**
