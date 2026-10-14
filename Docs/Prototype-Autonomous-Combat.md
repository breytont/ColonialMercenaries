# Prototype: Autonomous Faction Combat

## Purpose

Two or more prototype soldiers recognize different faction tags, seek one
another, and exchange server-authoritative hitscan fire **without a player
starting the encounter**.

This is a temporary combat behavior for proving the living battlefield. The
project's future modular weapon definitions and specialized AI controllers
will replace the prototype shot loop. No final weapon or AI systems are
committed to by this implementation.

## What's new

- `ACMCombatSoldier` (C++ actor, placeable in levels).
- NavMesh-based pursuit via built-in `AAIController`.
- Timer-driven overlap queries for hostile soldiers; **no actor Tick** and
  no repeated GetAllActorsOfClass calls.
- Raycast shots that respect ordinary level collision, with
  `ApplyDamage` on the server; soldier capsules explicitly block Visibility.
- Expanded prototype detection to 6,500 units so soldiers engage across
  the starter test map; editable per soldier.
- Output Log warnings for missing AI controllers / failed navigation, and
  health-change messages for verified damage.
- Unreliable multicast debug tracer for connected clients.
- Simple engine cube and overhead faction label, no external assets.
- Fully editable AI range, damage, rate, speed, and timing per actor.
- Temporary Red and Blue faction tags configurable on placed components.

## Build test (required before merge)

1. Close Unreal Editor.
2. Check out `feature/autonomous-faction-combat` in GitHub Desktop.
3. Regenerate Visual Studio project files if necessary.
4. Build `ColonialMercenariesEditor` for Development Editor / Win64.
5. Open the project and ensure the default Third Person map still plays.

## Small editor assembly after the build test

1. In the Level Editor, open `Lvl_ThirdPerson` and **Save As**
   `Content/ColonialMercenaries/Maps/Lvl_FactionTest`, preserving the
   starter map. Avoid renaming or moving starter assets.
2. Place a **Nav Mesh Bounds Volume** and size it to cover the floor or
   open area where soldiers will fight. Press P to view green navigation.
3. Add two **CM Combat Soldier** actors to the level, separated by about
   1,200 Unreal units on walkable ground.
4. For the first soldier, select the **Faction** component and set
   **Faction Id** to `Faction.Prototype.Red`.
5. For the second, set **Faction Id** to `Faction.Prototype.Blue`.
6. Press Play. Soldiers should pursue each other and fire yellow
   debug tracers until one is killed. A faction tag left empty is neutral.

## Caveats and future work

- The trace uses ordinary Visibility collision. Soldier capsules now block
  Visibility by default; other intervening blocking geometry still stops shots.
- If soldiers do not pursue each other, open Window > Developer Tools >
  Output Log and search for `CM combat soldier`; warnings name missing AI
  controllers or unsuccessful MoveToActor requests.
- If a soldier is hit, the server logs its health total for confirmation.
- This initial prototype targets **other CM Combat Soldiers**, not the
  player or arbitrary vehicles. That will be extended after validation.
- The head label is assigned on BeginPlay for placed actors and isn't a
  general runtime-replicated identity widget.
- Line-of-sight and target selection are intentionally basic; cover,
  tactical behavior, suppression, ammo, and advanced perception come later.
- A NavMesh is required for chasing but not for shooting at enemies already
  within weapon range.
- Dead NPCs disappear after a short configurable delay; salvage and
  persistent casualties will be developed separately.
