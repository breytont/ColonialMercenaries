# Prototype: tactical line of sight and repositioning

## Goal
Improve the validated autonomous squad firefight without adding cover-taking or
a full tactical planner yet. Soldiers should **not stand still firing into
walls**, and they should prefer an enemy they can actually hit.

## What changed
- Soldiers use the existing server-side Visibility raycasts to check whether
  the target capsule is the *first blocking actor* along the muzzle-to-target
  line.
- Among detected hostiles, a visible one takes priority over an obstructed
  one. If no hostiles have clear lines, the nearest obstructed target is used
  as a pursuit goal.
- A soldier in weapon range but unable to see its target now moves closer
  using NavMesh, with a separate editable **Blocked Approach Distance**.
- Ordinary pursuit still stops at weapon range when line of fire is available.
- Movement requests are throttled by **Path Retry Interval**.
- Optional **Draw Line Of Sight** displays green (clear) and red (blocked)
  sightline debug rays on the authority. Yellow lines remain actual shots.
- Server ownership and regular friendly-fire damage rules are unchanged.
  The AI tries not to intentionally fire through a squadmate, but moving units
  can still cross an actual shot and be damaged.

## Test in Unreal Engine 5.8
1. Compile Development Editor / Win64 on this branch.
2. Load `Lvl_FactionTest` and Play without editing the map. Verify the
   existing Red/Blue 3v3 firefight still occurs and soldiers die normally.
3. Stop Play. Add a **Cube** or wall-shaped static mesh in the fighting
   area, between the two spawners. It must block Visibility collision and
   leave a connected navigable route **around** it. Check green NavMesh with P.
4. Play. Soldiers initially obstructed by the wall should navigate around it
   instead of spraying yellow shots into the wall. They should resume firing
   when an opposing soldier becomes visible and within range.
5. If needed, in the **CM Combat Squad Spawner** details, temporarily use a
   Blueprint subclass of `CM Combat Soldier` with **Draw Line Of Sight**
   enabled to see green/red debug lines for spawned units. Alternatively,
   temporarily place two standalone `CM Combat Soldier` actors with the
   debug property set to true for a simpler test.
6. Confirm the Output Log does not repeatedly warn about failing NavMesh paths.
7. If the test map was modified, save/commit the `.umap` and External Actor
   assets deliberately, excluding generated solution files.

## Constraints and intentionally deferred behavior
- Detection still knows about enemies within the radius regardless of cover;
  sightline checking controls **target preference and shots**, not perception.
- This is *obstacle-aware repositioning*, not true flanking or seeking hard
  cover. Combatants will move toward hidden enemies until a firing lane opens.
- No new per-frame actor Tick and no GetAllActorsOfClass queries.
- No forced level asset changes. You can test with temporary obstacles
  without saving them to the persistent test map.
- All weapon trace damage still happens on the authority.
