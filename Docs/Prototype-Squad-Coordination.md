# Milestone #15 — Faction infantry squad coordination (prototype)

## GDD alignment

The GDD calls for **autonomous tactical faction activity**, including attacks,
defensive actions, retreats, and reinforcement logistics. Its company AI squad
section also identifies straightforward **follow, hold, attack, defend, board,
and disengage** orders. This milestone creates the core **faction infantry**
movement/order primitive and outpost commander first. It is **not yet player
company squad commands**, vehicle boarding, cover tactics, or platoon AI.

## Implemented C++ behavior

### Soldier command mode

`ECMSquadOrder` in `CMCombatSoldier.h`:

- **Independent**: original server-driven AI behavior unchanged. Soldiers placed
  manually or deployed by old squad spawners still pursue opponents normally.
- **Rally (Assemble)**: march to assigned staging slot, ignore enemies while
  assembling.
- **Advance**: head to assigned slot near a specific objective; stop and engage
  enemies **within weapon range with line of sight**. Otherwise keep moving to
  squad objective instead of chasing 6500 cm-away opponents independently.
- **Defend**: return to a designated guard position between engagements;
  attack threats in range.
- **Disengage (Withdraw)**: prioritize return to a withdrawal slot behind the
  base without re-engaging during the move.

Commands are issued only by server authority. The soldier's enum order is
replicated to clients for later squad UI; movement, target selection, combat,
and damage still run on the server. Assigned locations are consumed server-side
by AI navigation, so individual command vectors aren't separately replicated.
No actor Tick or per-frame battlefield scans were added.

### Outpost commander

The existing `ACMFactionOutpost` directs **its own tracked infantry** on
the existing one-second garrison timer. No separate controller Actor or global
actor scans are required. Tuning in the outpost's **Faction Outpost > Squad**
Details category:

- `StartingPosture`: **Assault** (default) or **Defend**.
- `AssaultObjective`: editor-level reference to any actor, normally the
  existing `CMContestedPoint`. **For reliable testing, assign this field**.
- `FallbackAdvanceDistance`: if no objective is set, advance a fixed
  distance in the outpost's forward direction. Defaults to 1800 cm.
- `AssemblySeconds`: 5. Starts in Assembly and issues Rally orders before
  moving to Advance as a group after the staging delay.
- `RetreatAtOrBelow`: 1 living soldier (for a 3-person target squad). When
  the advance loses enough soldiers, remaining infantry withdraw behind base.
- `RegroupSeconds`: after retreat, wait at least 4 seconds AND restore
  full garrison strength from **finite** reserves before assembling again.
- `SquadSlotSpacing`, `AssemblyForwardOffset`,
  `WithdrawalBackOffset`, `SoldiersPerRow`, `RowSpacing`: tunable formation
  deployment. The same order targets each squad's own slots.
- `SetOutpostPosture` (Blueprint-callable, authority only) supports later
  strategic/gameplay calls to switch between Assault and Defend.

### Shared squad defensive contact (feedback refinement)

A defending soldier can see/shoot an opponent within its own 950 cm
rifle range, while squadmates at the opposite end of the formation may be
too far away to return fire. The original Defend logic would leave those
other soldiers idle even though the **squad** was already in combat.

The outpost commander now checks its **own** engaged riflemen for a valid,
living hostile contact on the existing one-second timer. When one is found
within its configured `DefensiveAssistRadius` (default **1900 cm from the
outpost**), it alerts the remaining defenders. Their server-controlled AI:

- Tries to shoot the shared target when actually in weapon range with
  an unobstructed line of fire.
- Otherwise moves toward a **bounded supporting position**, with each
  soldier approaching from its own angle. This is intentionally not a
  single group MoveTo target.
- Stops at the configured defensive perimeter instead of chasing enemies
  indefinitely.
- Holds the last report for `DefensiveContactMemorySeconds` (default **4**)
  to keep moving when another soldier briefly loses line of sight.
- Clears the contact on enemy death, target retreat beyond the perimeter,
  or expiration; surviving defenders resume their original guard slots.
- Does **not** call in help during `Assembly` or `Disengage`, and does
  not alter the independent soldier mode.

In PIE the status label shows **DEFENDING - CONTACT** while the group is
responding to a shared hostile. The contact flag is replicated to clients.
The underlying threat pointer/desired firing positions are authoritative
AI state only.

**Limits:** This is a first-pass shared-contact and supporting fire
behavior, *not* a tactical cover-finding/flanking planner. Pathing is
constrained by available NavMesh and sight lines. The fallback position
is geometric and may need later tuning for difficult terrain. This
first feedback iteration specifically improves outpost `Defend` orders;
a general squad threat coordinator for mobile assaults can follow.

The outpost's replicated overhead status now includes **ASSEMBLING**,
**ADVANCING**, **WITHDRAWING**, or **DEFENDING**. Existing faction, garrison,
reserve and reinforcement state remain replicated.

**Important constraints:** The squad does *not* move as a rigid animated unit,
appoint a squad leader, solve stuck pathing automatically, move in player
commands, reserve personal cover, or select complex tactical routes yet.
Individual soldiers pause formation movement to fire visible threats. The
outpost does not command infantry spawned by another outpost. The system
continues to use local finite manpower reserves from milestone #14.

## Playtest in UE 5.8

1. Fetch and switch to `feature/squad-coordination`. Regenerate Visual
   Studio project files only if the editor requires it, then build
   **Development Editor | Win64** with the editor closed.
2. Open `Lvl_FactionTest`. Keep both outposts and the existing contested
   point. Select **Red Forward Base**, expand
   **Colonial Mercenaries > Faction Outpost > Squad**. Keep
   `Starting Posture = Assault`.
3. Set **Assault Objective** to the already placed `CMContestedPoint`
   level actor (use the drop-down/eyedropper). Repeat for **Blue Forward Base**.
4. Keep `Assembly Seconds = 5`, `Retreat At Or Below = 1`,
   `Regroup Seconds = 4` and finite reserves at **9** on both.
5. PIE with one player: Red and Blue each deploy three infantry, status
   initially **ASSEMBLING**, then changes to **ADVANCING** after
   approximately 5 seconds. Infantry should advance toward the capture point
   from both sides, with visibly related destination slots.
6. Watch a squad lose two of its three soldiers: its outpost label should
   switch to **WITHDRAWING** and its survivor should head back toward the
   base instead of chasing opponents. The losing outpost will continue
   reinforcing with its existing 20-second interval.
7. Once the garrison refills to full (if sufficient reserve remains), the
   label should return to **ASSEMBLING** and then **ADVANCING** for the next
   assault. Verify the reserve decreased for each replacement and reaches
   zero as before.
8. **Shared-defense test:** set Red **Starting Posture = Defend**
   and move the Red outpost temporarily within about 900–1,000 cm of the
   contested point, with Blue remaining on Assault. Confirm one Red
   soldier engages Blue as they enter range, the Red label displays
   **DEFENDING - CONTACT**, and other Red soldiers step out from guard
   slots toward distinct supporting positions and begin firing as soon
   as they have their own line of sight and range. They must not chase
   Blue indefinitely far past the **1900 cm** defense perimeter.
   Once the contact is lost or killed, surviving Red infantry return to
   their guard positions. Restore the original Red location after testing
   if you do not want the close-range setup in the test level.
9. PIE with **2 players**: verify both windows display the same outpost
   orders, walking and firing; no duplicate spawns. Check player shooting,
   revive, and capture-point functionality.
10. Save level, close Unreal, commit/push `Lvl_FactionTest` changed
    External Actor files. No Blueprint graph edits are required.

### Expected rough edges

- On some terrain, objective center may be off NavMesh or blocked by level
  geometry. Adjust the contested point location or fallback path slots on
  navigable ground before debugging C++ AI. Watch the Output Log for
  `cannot navigate to squad`.
- Staging, target, and retreat positions are fixed **world locations**, not
  real-time leader following or cover selection.
- The 5-second assembly is a simple shared timer, not a formation-readiness
  check. Movement may be somewhat staggered by navigation.
- A squad with depleted reserves that withdraws and cannot restore full
  strength stays in **WITHDRAWING**, rather than regenerating troops.

## Acceptance gate before PR merge

- [ ] UE 5.8 C++ build.
- [ ] Two outposts rally, advance, and fight toward assigned contested point.
- [ ] At 1 survivor, retreat order interrupts enemy pursuit.
- [ ] Reserves remain finite; sufficiently replenished units regroup/redeploy.
- [ ] Defending squad shares contacts: nonengaged members reposition,
      acquire line of sight and provide supporting fire inside its perimeter.
- [ ] Defenders return to guard posts after contact is killed/lost.
- [ ] Defend posture still avoids permanent long-range pursuit.
- [ ] Client replication and old independent combat verified in 2-player PIE.
- [ ] Edited map/External Actor assets pushed.
