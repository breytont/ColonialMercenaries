# Prototype 0.1: Contested territory point

## Purpose
The existing Red/Blue squad firefight should produce a **world-state outcome**:
a local installation changes faction ownership when surviving soldiers occupy
it. This is the GDD's first dynamic territory-control acceptance test, not the
planetary strategic-war system.

## Implementation
- Placeable C++ actor **CM Contested Point**, with a Sphere component marking
  its capture radius and a floating status label.
- Server-authoritative, timer-driven evaluation of *living* `CM Combat Soldier`
  faction members in the volume. Dead soldiers are excluded immediately.
- Neutral point initially, or set **Initial Owner Faction** in the actor Details.
- With one faction present and no opponents, capture advances over an editable
  **Capture Time Seconds** (8 seconds by default).
- If both factions remain within the zone, progress **pauses** as contested.
- If all soldiers leave, progress **pauses** rather than instantly resetting.
- If the owner is the only faction present, it retains control and any partial
  hostile capture attempt is canceled.
- On successful capture, **Owner Faction** changes and `OnOwnerChanged`
  broadcasts on the server and clients.
- The captured owner and capture progress replicate to clients; a status label
  reports the owner, current attacking faction, and progress.
- No actor Tick. No `GetAllActorsOfClass`. No changes to soldier weapon AI.

## Unreal Editor acceptance test
1. Build **Development Editor / Win64** for the new branch.
2. Open `Lvl_FactionTest`. Both squad spawners should still be present.
3. Add one **CM Contested Point** midway between the Red and Blue
   spawners. Select its **Capture Area** component and check its radius
   (default 1,600 units). Adjust it to include the engagement area and
   likely surviving soldiers. Set **Capture Time Seconds** to 8, or lower
   for a quick test.
4. Press Play. The status label should say **CONTESTED** whenever living
   members of both factions occupy the sphere.
5. As soldiers die and one faction remains, the capture progress advances.
   When it reaches 100%, the label should update to that faction's owner tag.
6. Stop and Play again to verify that the test starts from its configured
   initial owner.
7. Save the test map only after successful validation. Commit its `.umap`
   and associated External Actor assets, excluding generated VS files.

## Important limits
- Only the existing **CM Combat Soldier** counts for now. Players and vehicles
  are not counted until their faction systems are wired up.
- No dynamic AI orders to attack/defend the point yet. The nearby firefight
  should naturally generate an ownership outcome.
- This prototype captures a *local site*. It cannot conclude or override the
  narrative campaign's strategic outcomes.
- No save-game persistence yet; owner resets to the actor's **Initial Owner
  Faction** each Play session. Save persistence is a separate milestone.
- No client authority over ownership. Zone state is calculated server-side.
- This is intentionally limited. The next priority is player shooting and
  combat controls, before drafting a full Technical Design Document.
