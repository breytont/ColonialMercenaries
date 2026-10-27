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

## Workflow reassessment — choose a hybrid landscape approach

### Decision

For the first playable blockout, **prefer an authored 16-bit, 505 x 505
heightmap for macro terrain**, with manual Unreal Landscape editing reserved
for gameplay-sensitive local corrections. The existing 505 x 505 Landscape
is compatible; the import can be applied to an existing Landscape without
re-creating the map. **Make a Save All + Git checkpoint first**, because
importing height values can overwrite existing sculpting.

The macro heightmap should intentionally reserve approachable, reasonably
level footprints near Red FOB (-110m, 0), Blue FOB (+110m, 0), relay
(0, 0), HQ (0, -100m), and AA (0, +95m), plus a broad north ridge and
lower southern maintenance route. Generate this terrain reproducibly
(e.g., a small Python script and exported 16-bit grayscale PNG/R16) rather
than depending exclusively on hundreds of manual brush strokes.

Heightmaps encode only elevation. They **do not** create buildings, roads,
collision-enabled cover, NavMesh, AI tactics or mission sites; those stay
authored and verified in Unreal.

### Hybrid workflow

1. Draw/build an overhead gameplay blockout first: objective distances,
   routes, slopes, sight lines, spawn/retreat pads, then generate/import
   a macro heightmap to suit that plan.
2. Put local adjustments on Landscape Edit Layers where possible. Smooth,
   Flatten and Ramp should maintain navigable approaches; preserve
   rollback options and avoid unreviewed re-import over polished edits.
3. Add road geometry and terrain conforming with **Landscape Splines**
   after the macro terrain is accepted. Avoid hand-painting all road grades.
4. Add buildings/cover as reusable modular pieces. Keep landscape/foliage
   decoration decoupled from the combat logic.
5. Consider the **PCG framework** later for vegetation and rocks, driven
   by exclusion zones around roads, outposts, and NavMesh walkways.
   PCG is not a substitute for encounter layout.
6. Test soldier navigation, firing lines, target acquisition and retreat
   after each major geographic change. The current short 950-cm weapon
   range requires an explicit future ranged-combat tuning pass.

### Decision checkpoints

At the end of **macro terrain**, **first fully navigable graybox**, and
**first successful squad battle**, explicitly review:
- What is currently slow, repetitive, or fragile?
- What alternate tooling could reduce that work (scripts, heightmaps,
  Unreal editor tools, landscape splines, PCG, modular kit)?
- Does the alternative preserve UE5.8 compatibility, source control,
  iteration speed, and the GDD's tactical-grounded intent?
- Can it be validated by a reversible, small test rather than a major
  rewrite?

Prefer the approach that gets a **real gameplay test** sooner, not the
approach that merely produces the most detailed environment.

### UE5.8 references

- https://dev.epicgames.com/documentation/unreal-engine/importing-and-exporting-landscape-heightmaps-in-unreal-engine
- https://dev.epicgames.com/documentation/en-us/unreal-engine/landscape-edit-layers-in-unreal-engine
- https://dev.epicgames.com/documentation/en-us/unreal-engine/landscape-splines-in-unreal-engine
- https://dev.epicgames.com/documentation/unreal-engine/using-pcg-generation-modes-in-unreal-engine

## Relay compound blockout — reusable C++ actor (Stage C)

Added `ACMRelayCompoundBlockout` in
`Source/ColonialMercenaries/World/CMRelayCompoundBlockout.h/.cpp`.
It builds collision-enabled static-mesh instances from the built-in engine
cube, avoiding dozens of manually positioned individual Actors. Its geometry
is created in `OnConstruction` and can be adjusted in the placed actor's
Details panel; no Tick, third-party assets, or network replication.

**Setup:** Save All, close Unreal, commit any uncommitted level changes
(the existing `Relay_Foundation` cube and `Relay_CapturePoint`).
Fetch/pull the feature branch, build Development Editor/Win64, then reopen
`Lvl_ContestedFrontier`. Search Place Actors for
**CM Relay Compound Blockout**. Place one instance at
**Location (0, 0, 100), Rotation (0, 0, 0), Scale (1, 1, 1)**. Keep the
existing **Relay_Foundation** and **Relay_CapturePoint** in the level:
the former is the interior floor; the latter continues to own capture
logic and its current position.

Layout defaults:
- Perimeter: ~44 x 36m, ~1.6m-tall walls, **four 10m-wide entrances**
  to preserve flanking routes and multiple faction attack approaches.
- Staggered interior/exterior low cover plus one utility hut/mast on the
  northern side; the central `CMContestedPoint` stays separate and clear.
- All boxes block gameplay collision and Visibility; their authored
  collision should carve the NavMesh once a bounds volume is placed.
- Use `HalfExtentX`, `HalfExtentY`, `EntranceHalfWidth`,
  `WallHeight`, `CoverHeight`, `FoundationTopOffset`, and
  `bPlaceUtilityHut` to tune the blockout rapidly.
- The actor's geometry starts at its own origin elevation, while
  platform cover is raised an extra 20cm to match
  `Relay_Foundation` (previously placed at Z=110, scale Z=0.2).

**Validation:** Editor placement must produce four visibly clear
entrances and no overlap with the existing capture actor. Once NavMesh
is added, verify green navigation on both sides of each entrance, with
no intended 2m lip or raised platform blocking infantry access. Tweak
outer terrain / foundation flushness or add ramps if navigation fails.
If geometry looks out of place on imported terrain, move or tune this
single Actor; do not edit each generated instance separately.

This is a design-testing tool, not final procedural art.

## Field test #1 — actual squad assembly before assault (Oct 26)

User confirmed both factions traverse the generated terrain, use the relay
compound, engage, withdraw and reinforce. They increased garrison/wave sizes
and shortened reinforcement cooldowns to exercise battles faster. The
remaining issue was that a soldier could leave alone to assault when the
outpost garrison *count* recovered, despite its teammates still being back
at the base. This was consistent with the old `AssemblySeconds` timer-only
transition in `CheckGarrison()`.

### Narrow AI fix on this branch

The outpost commander now requires **physical squad assembly** to switch
from **ASSEMBLING** to **ADVANCING**:

- Every living soldier from this outpost must be inside
  `RallySlotRadius` (default **250 cm**) of its *own* assigned
  `GetSquadSlot(..., Assembly)` position.
- `MinimumAssaultStrength = 0` (default) waits for the full
  `TargetGarrisonSize`; a positive configured value permits deliberate
  understrength assaults, but still requires **every currently living**
  member to rally. This field is in **Faction Outpost | Squad | Assembly**.
- `AssemblySeconds` (existing default **5s**) still enforces the
  minimum time since entering Assembly.
- `RallyReadyHoldSeconds` (default **1s**) requires the full squad to
  stay rallied briefly. If anyone falls out of range or dies, the hold
  timer resets.
- During Assembly, overhead status shows
  **Rallying N/<living garrison>**, replicated for multiplayer.
- No new Actor Ticks, all global actor scans avoided, and no changes to
  existing soldier weapon, retaliation, retreat covering fire, tactics
  or independent spawning code.

**Design consequence:** A formation that cannot reach its assembly
slots, or cannot restore the configured minimum garrison due to exhausted
reserves, will **wait** rather than launching a lone soldier. This is
intentional for this test, and makes broken approach/NavMesh visible.

### Playtest

1. Save & commit the newly placed `Relay_Compound`, NavMesh volume,
   Red/Blue bases and any edited external-actor packages; keep the
   current local map changes when pulling new C++.
2. Close Unreal, pull this feature branch, build
   **Development Editor | Win64**, reopen `Lvl_ContestedFrontier`.
3. Start a one-player PIE with both outposts on Assault, target size
   **3** and accelerated finite reinforcement settings.
4. Observe a squad lose two soldiers and withdraw. After reinforcements,
   the label should say **ASSEMBLING | Rallying N/3** until the
   survivor and both new recruits are near their respective base
   rally slots; only then switch to ADVANCING.
5. Increase target size to **5 or 6**, repeat. Verify no single soldier
   leaves prematurely. If the formation stays assembling, visually
   inspect member NavMesh routes and assigned positions.
6. Retest original damage retaliation and retreat covering fire, and
   two-player authoritative garrisons/phase/label replication.
7. Commit successful UE test map adjustments and document results.

**Limit:** This adds departure cohesion, not a full leader-led formation
in transit. Reinforcements that arrive while a squad is already in
**ADVANCING** are still treated as independent reinforcements,
and a squad may naturally spread during combat. Maintain a future
separate tactical feature for leader selection, spacing/formation
regrouping during maneuvers, cover evaluation/reservation, suppressive
fire, unit roles and personality differences.

### Later tactical-AI work, after contract vertical slice

1. Squad leader & maneuvers — explicit Move/Assault/Hold/Regroup,
   rally completion events, leader incapacitation fallback, staggered
   movement for fire teams.
2. Real cover search — nearby nav-reachable positions with occlusion,
   protection from current threats, occupancy/reservation, and a
   stable return-to-objective policy. Grounded military behavior,
   not supernatural knowledge of enemies.
3. Distinct infantry roles through existing
   `UCMCombatTactics` extension point — rifleman, assault, marksman,
   support, each with different engagement distances, reposition
   priorities and cooldowns.
4. Bounded tactical variation — soldier skill, aggressiveness,
   self-preservation, and cohesion preferences selected via editable
   data/config, while retaining identical authoritative faction
   mechanics. Avoid purely random behavior that undermines squad
   objectives.

## Stage D: Mercenary headquarters graybox (GDD §13)

GDD calls for a **physical, explorable headquarters** from which the
company can inspect vehicles and equipment and reach contract,
storage, personnel and financial interfaces. Prototype 0.1 needs the
HQ as the starting point of the contract/payment/procurement loop.
This first pass intentionally builds only **walkable exterior staging
space**; building interiors, interaction actors, storage, transactions,
company funds and player respawn mechanics are *not yet implemented*.

Added reusable, editor-placeable
`ACMMercenaryHeadquartersBlockout` under `World/`.
It uses instanced engine cube meshes built in `OnConstruction`
(no imported art assets, Tick, gameplay scans or replication):

- Editable **36 × 32m** fenced yard; **10m north gate** faces the
  relay compound and **8m east service gate** keeps a second path open.
- Eastern **operations prefabricated module** marked as the future
  contracts interface. The module is a solid exterior placeholder:
  add a separate, network-authoritative `ContractTerminal` gameplay
  actor later; do not give this graybox fake transaction functions.
- Western **four-pillar open vehicle canopy** with a ~11×8m covered
  footprint, three storage-prop silhouettes and nonblocking floor
  markers indicating the future player assembly/spawn zone.
- Structural geometry uses `BlockAll` collision; visual ground marks
  have no collision and do not influence NavMesh. Adjustable
  HalfWidthCm, HalfDepthCm, NorthGateHalfWidthCm,
  EastGateHalfWidthCm, BarrierHeightCm, facility toggles.
- HQ construction and outpost leadership remain entirely separate.

**User integration:**
1. In Unreal, **Save All**; close editor. Commit any unsaved map
   and external actor files before pulling another remote commit.
2. On `feature/contested-frontier-blockout`, Fetch/Pull origin.
3. Build **Development Editor / Win64**, reopen
   `Lvl_ContestedFrontier`.
4. Search Place Actors for **CM Mercenary Headquarters Blockout**;
   place one and set transform **Location (0, -10000, 0),
   Rotation (0, 0, 0), Scale (1, 1, 1)**. Its primary gate
   should point **north (+Y)** toward the relay.
5. Inspect the footprint, open gates, terrain contact and shelter
   clearance. The generated landscape's HQ plateau has elevation
   ~0m at the facility center. If borders hover slightly over
   curved edges, adjust placement or local terrain using Landscape
   tools; do not alter the shared macro heightmap yet.
6. Later, put an actual **PlayerStart** in the unblocked courtyard
   near **world (0, -10250, ~120)**, verify no overlaps and
   correct spawn in 1 and 2-player PIE. Place *multiple*
   PlayerStarts for multiplayer, not several players at one pad.
7. Before art pass, check ability to move from HQ to relay (open
   north gate), unobstructed courtyard, and Future Contract Office
   marker legibility.
8. Save All and commit `.umap` and associated External Actors.

**Future first mercenary contract seam:** an independently
replicated contract-terminal actor at the operations module,
connected to the contested point's ownership-change event. The
contract should distinguish employer, objective, completion and
payment state and validate all rewards on server. Do not modify
territory capture code to make it a contract system.

## Stage E1 — HQ spawn tested; visual HQ–relay access road

The user confirmed the new **Mercenary Headquarters** blockout built and
spawned their third-person player inside the courtyard. For a first
readable connection to the central relay, added one reusable
`ACMFrontierServiceRoad` (`World/CMFrontierServiceRoad.h/.cpp`).

This editor-time C++ graybox:
- Places a ~**66m** long, **6m** wide, thin road along its local **+Y**
  with an optional gentle ~1.8m sideways deviation.
- Uses 33 short engine-cube instances and world collision traces to
  approximately follow the existing Landscape surface; height sampling
  happens **in OnConstruction** only, not on Tick.
- Uses **NoCollision** and has no navigation effect; player and
  soldiers walk on the real Landscape, not on these visual road tiles.
  It is not a road material, spline-deformed landscape, physical
  paved road, or functioning pathfinding corridor.
- Geometry is separate from gameplay contract and AI systems.
- Editable LengthCm, WidthCm, MidpointLateralOffsetCm,
  SegmentCount and RoadClearanceCm let us iterate without placing
  individual cubes.

**Integration — after checkpointing the successful spawn**
1. `Save All` and commit the HQ actor and PlayerStart external actor
   changes locally. **Close Unreal** before pulling new code.
2. Pull `feature/contested-frontier-blockout`, build
   `Development Editor | Win64`, reopen `Lvl_ContestedFrontier`.
3. Place **CM Frontier Service Road** with transform
   **Location (0,-8400,0), Rotation (0,0,0), Scale (1,1,1)**.
   Local +Y leads from HQ's **north gate at Y≈-8400** to relay's
   **south gate at Y≈-1800**; keep defaults for first test.
4. Observe from ground level, verify a single continuous visual
   route, with no floating tiles, ground clipping or blocked doors.
   If road traces hit props near the endpoints, shorten by a metre
   or adjust individual entry settings and report it before creating
   more roads.
5. Verify PlayerStart can physically **walk from HQ to the relay**.
   Add no AI road-following until we confirm the environmental
   navigation test.
6. Save All, commit new map/external-actor assets on the feature
   branch and push.

**Future art pass:** replace the gray blocks with a proper
Landscape-Spline dirt/gravel road, material layers and local terrain
smoothing once our world layout and encounters are validated.
