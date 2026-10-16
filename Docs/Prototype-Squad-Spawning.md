# Prototype: Automatic Squad Deployment

## Goal

Turn the verified one-versus-one faction test into a small, self-starting
three-versus-three skirmish without manually placing every soldier.

This **deployment prototype** reuses the independently fighting
`ACMCombatSoldier` actors. It does **not** implement coordinated movement,
leaders, squad orders, formations maintained during combat, cover tactics,
reinforcements, or a persistent population system. Those are later systems.

## Code

- `ACMCombatSquadSpawner`: placeable actor, no Tick.
- `FactionId`: assign the Red or Blue tag for every member.
- `SoldierClass`: defaults to the existing `ACMCombatSoldier`; allows
  a soldier Blueprint class in the future.
- `SoldierCount`: editable 1–12, defaults to 3.
- `SoldiersPerRow`: editable 1–6, defaults to 3.
- `LateralSpacing`, `RowSpacing`, `SpawnHeight`: editable formation tuning.
- Soldiers are spawned **only by the server**, with their factions assigned
  before BeginPlay. The spawned soldiers remain replicated actors.

## Local test after successful C++ compilation

1. Open `Lvl_FactionTest`. The original two placed soldiers may remain for
   now, but for a clean test delete those **two manually placed soldiers**.
2. In Place Actors, search for **CM Combat Squad Spawner**.
3. Add two squad spawners, facing each other, approximately 2,000–3,000
   Unreal units apart on the existing green NavMesh. Avoid placing the
   spawner over an obstacle.
4. On the first spawner set **Faction Id = Faction.Prototype.Red** and
   **Soldier Count = 3**.
5. On the second spawner set **Faction Id = Faction.Prototype.Blue** and
   **Soldier Count = 3**.
6. Press Play. Six cube soldiers should appear in two groups, pursue
   nearby enemies, fire, take damage, and disappear when killed.
7. Save the level, commit its modified .umap and External Actor assets, and
   push them before merging this branch.

## Checks

- [ ] Existing Third Person controls still work.
- [ ] Exactly three Red and three Blue soldiers appear at Play.
- [ ] Spawners do not create extra soldiers on connected client instances.
- [ ] Members select opposing targets and fight independently.
- [ ] Health and deaths occur correctly.
- [ ] No warnings about missing AI controllers or failed navigation.

## Notes

- All members fight using the existing soldier detection radius and
  hitscan weapon. Same-faction soldiers can accidentally hit each other,
  matching the full-friendly-fire requirement.
- Spawn positions are spaced around the actor location and raised by
  SpawnHeight. Keep all member positions on connected walkable ground.
- Spawners are one-shot deployment points, not respawn devices. This avoids
  implicit infinite fighting or troop replenishment before logistics exists.
