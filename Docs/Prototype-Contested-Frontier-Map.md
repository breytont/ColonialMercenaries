# Milestone #16 — Contested Frontier playable battlefield graybox

## Goal and relationship to the GDD

Build the **Prototype 0.1: Contested Frontier** region from the v1.0 GDD as a
playable **Unreal Engine 5.8** environment, before adding the first mercenary
contract. The GDD calls for a small region with the company headquarters,
opposing factions, a contested installation, and a strategic AA site. It
prioritizes a dynamic world, readable military geography, and placeholder art
during prototyping. The full open-world region is a later milestone.

This milestone is **map authoring and AI acceptance**, not new world simulation,
contracts/economy, World Partition streaming, final landscape art, or a new
soldier AI rewrite. The purpose is to find environment and encounter problems
with the systems we have *now*.

Keep **`Lvl_FactionTest`** unchanged as the regression test. Create
**`Content/ColonialMercenaries/Maps/Lvl_ContestedFrontier.umap`** for the
new battlefield; use the existing gameplay actor classes.

## Theme and design pillars

**Temperate frontier industrial corridor** on an established colony planet:
weathered rock, rolling hills, patchy vegetation, gravel access roads,
utility structures and improvised fortifications. Grounded military science
fiction rather than fantasy/alien ruins or high-density city streets.

1. **Readable objectives:** visually distinct Red and Blue FOBs, relay
   compound, mercenary HQ and AA site.
2. **Multiple tactical routes:** a central road, north ridge and south
   drainage/service track; no single mandatory choke point.
3. **Fair infantry terrain:** hard cover and partial cover, meaningful
   sight-line breaks, but navigable entrances around all defenses.
4. **Server-AI first:** collision, line-of-sight and NavMesh are validated
   throughout. Do not make ornamental cliffs or walls that trap the
   current 3-soldier squads.
5. **Contract-ready geography:** HQ has a place for a future contract
   interface; the central installation's owner is the first mission
   objective; AA site supports a later support-unlock objective.
6. **Everything committed:** version the `.umap` *and* External Actors,
   Blueprints and configuration changed with it.

## First playable layout (coordinate targets, not rigid placements)

**Origin at relay compound (0, 0, 0)**. X axis = east/west; Y axis =
north/south. Units are centimeters; **100 cm = 1 m**. Aim for an initial
**300 x 260 meter playable core**, with sufficient margins for hills and
staging. Exact locations and elevations should be tuned in-editor:

| Area | Target XY (cm) | Approximate purpose / size |
| --- | --- | --- |
| Red faction FOB | (-11000, 0) | ~30m compound + clear forward spawn/staging area |
| Blue faction FOB | (+11000, 0) | Symmetrical *capability*, not symmetrical scenery |
| Central relay | (0, 0) | ~40m x 32m yard; capture sphere at its core |
| Mercenary HQ | (0, -10000) | ~35m pad in low-risk southern sector |
| AA site | (0, +9500) | ~25m elevated defended installation |

**Orient Red eastward (+X; yaw 0) and Blue westward (-X; yaw 180)** so the
outposts' forward spawning / assembly positions point toward the central
installation. Initially assign both `AssaultObjective` properties to the
central `CMContestedPoint`, then test mixed Assault/Defend postures.

### Playable routes and sightlines

- **Central service road:** broad enough for future vehicle traffic,
  but intermittent barriers, parked industrial machinery, trenches and
  containers prevent a completely unobstructed base-to-base firefight.
- **North ridge:** higher, partially exposed route between the relay
  compound and the AA site, with slopes/ramped approaches for AI.
- **South drainage / maintenance track:** lower, sheltered path connecting
  headquarters vicinity with the relay. Avoid a direct safe firing perch
  overlooking either faction spawn.
- **Relay compound:** open courtyard around `CMContestedPoint`,
  two+ entry routes and several staggered obstructions (never a fully
  enclosed box). The existing capture radius is **1600 cm** by default;
  adjust to cover the intended central fighting space, not both outposts.

The current prototype NPC `WeaponRange = 950 cm` (**9.5 m**), which is
far too short for final military-rifle combat. For now, ensure some
close-range approaches and patrol contacts occur within that limit; range
balancing should be a separate data-driven combat tuning step, *after*
we can observe engagements and pathing in this new environment.

## Build order (small, reviewable increments)

### Stage A — New playable map shell (first user action)

1. With Unreal closed, fetch/switch GitHub Desktop to
   `feature/contested-frontier-blockout` (branched from merged `main`).
   Leave old stashed `ABP_CM_Rifle` changes alone.
2. Open the **correct** repository project under
   `Documents\GitHub\ColonialMercenaries`.
3. In Unreal, **File > New Level**, choose **Basic** for initial lighting
   and ground, then **Save Current Level As**:
   `Content/ColonialMercenaries/Maps/Lvl_ContestedFrontier`.
4. Commit the new `.umap` and any associated `__ExternalActors__`/
   `__ExternalObjects__` files on this feature branch immediately.
   No changes to `Lvl_FactionTest`.

### Stage B — Playable geography

- Expand/replace temporary floor with ground spanning the target footprint.
- Block in only broad hills/ridge and drainage line initially. Favor slopes
  that walking AI can traverse; delay steep decorative cliffs.
- Use engine primitives for pads and roads before investing in art.

### Stage C — Installations and combat lanes

- Block out 4–5 landmarks: two FOBs, relay yard, HQ and AA hill.
- Add lightweight obstacles/cover with valid Visibility collision. No
  need for detailed interiors or separate World Partition cells yet.
- Outpost spawn and retreat pads must have free NavMesh around them.

### Stage D — Reconnect proven gameplay

- Place exactly one `CMContestedPoint` in the relay yard.
- Place one `CMFactionOutpost` for each faction, with **3** starting
  soldiers and **9** finite reserves initially; assign Red/Blue
  Gameplay Tags and shared AssaultObjective.
- **Remove/avoid legacy `CMCombatSquadSpawner`** in this level to
  prevent duplicate soldier groups.
- Set a safe PlayerStart near mercenary HQ, not inside a garrison.
- Place a NavMeshBoundsVolume encompassing all intended infantry routes.
  Use the editor NavMesh visualization (`P`) to ensure **continuous**
  paths from each spawn area to the relay and retreat slots.

### Stage E — Playtest + map polish

- Observe at least three fight cycles: assault, capture, attrition /
  retreat, finite reinforcement, reassault. Verify no pathing stalls.
- Mix one defensive posture and one assault; reverse them; confirm
  soldier response to player provocation (None faction) and to an
  opposing faction player.
- Verify melee-sized cover geometry does not block all Visibility
  traces; improve flanking paths and cover based on live tests.
- Validate 2-player PIE before PR merge; 4-player regression if feasible.
- Only then iterate terrain materials, vegetation, skyline, lighting,
  signage and unique silhouettes using replaceable assets.

## Retention / version control

**Important lesson from prior map loss:** Unreal 5 uses External Actor
packages for level actor modifications. The `.umap` file itself may
appear unchanged after moving a faction base, rotating it or modifying its
settings. A complete save/commit can therefore include new/updated files
in `Content/__ExternalActors__/ColonialMercenaries/Maps/
Lvl_ContestedFrontier/**` as well as the `.umap`.

- Always **Save All** in Unreal before committing.
- In GitHub Desktop inspect all `Content/` changes, especially external
  actors and edited Blueprints. Do not accidentally discard/stash them
  during branch switches.
- Make a commit whenever a stage becomes playable. Pause Unreal/close
  the editor before switching branches or doing large file moves.
- Keep the earlier `Lvl_FactionTest` level as our reliable reference.

## Acceptance criteria for this milestone

- One dedicated playable new map is present and committed along with its
  complete associated actor data.
- The player can navigate between HQ, relay and at least one outpost;
  each planned zone has a recognizable graybox silhouette.
- Both NPC factions navigate the map, engage, capture, retreat under
  fire and reinforce without freezing or teleporting.
- The two assault approaches differ meaningfully; soldiers have cover,
  flank and line-of-sight opportunities.
- Multiplayer produces one authoritative garrison and consistent
  capture/combat state for all players.
- `Lvl_FactionTest` remains available for regression tests.
- The new level provides clear physical locations for the *next*
  milestone: minimal mercenary contract, reward and procurement.
