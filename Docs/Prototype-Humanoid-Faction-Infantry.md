# Milestone #13 — Humanoid faction infantry (UE 5.8)

## Scope and intent

Replace the faction-war prototype's cube placeholders with readable,
networked **humanoid combatants**, without changing health, hostility,
navigation, territorial control, squad-spawner quantities, or damage rules.
The GDD explicitly permits placeholder content in Prototype 0.1; this pass
reuses the existing Third Person mannequin assets and player rifle animation
to establish a common visual foundation before faction outposts and squad AI.

## Implementation

- Existing `ACMCombatSoldier` (not a second NPC class) now initializes its
  inherited skeletal Mesh with the checked-in
  `/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple` asset.
- Uses the checked-in `ABP_CM_Rifle` aiming/locomotion graph and
  `AM_CM_RifleFire` montage. The original `UCMRifleAnimInstance` read-only
  animation bridge now supports both players and faction infantry, with
  distinct sources for aim pose and support-hand position.
- The mannequin is positioned at Z=-90 and yaw=-90 relative to the Character
  capsule. The prior cube is kept as a hidden fallback if the mesh asset
  fails to load (collision continues to use the capsule, not meshes).
- Placeholder rifle receiver, barrel, stock and pistol grip use stock
  no-collision engine shapes attached to `hand_r`. The left-hand IK
  support-grip point matches the player's existing `ABP_CM_Rifle` setup.
  Weapon components can be adjusted/replaced in a derived infantry BP.
- When a soldier holds position and fires, it turns its body toward its
  current enemy and replicates its visual aim rotation (including pitch).
  This drives the aiming pose even on non-owning multiplayer clients.
- Existing debug shot multicast also triggers the rifle firing montage
  on relevant clients. All damage, trajectory tracing, hit detection
  and navigation remain server-authoritative and unchanged.
- Death hides the mannequin, rifle parts and faction label, while retaining
  the existing 7-second actor cleanup.
- Existing RED and BLUE overhead text labels stay visible and colored for
  faction readability. Customized gear, uniform materials, and armbands
  should be authored as visual assets in later milestones.

## No new Unreal Editor assets required

After a successful C++ build, existing `CMCombatSquadSpawner` actors
automatically spawn the updated soldier class, assuming their Soldier Class
is still `CMCombatSoldier` or a Blueprint derived from it that does not
override the mesh/animation settings.

## Test sequence

1. GitHub Desktop: fetch and switch to
   `feature/humanoid-faction-infantry`. Regenerate Visual Studio solution
   files if necessary (new C++ source files are **not** added in this branch).
2. Build Unreal Engine 5.8 **Development Editor / Win64** in Visual Studio.
3. Start `Lvl_FactionTest` in **1-player PIE**. Confirm both Red and Blue
   soldiers have human-shaped bodies, visible placeholder rifles, and
   colored faction labels rather than the previous cube bodies.
4. Observe their standing and walking/running. They should follow existing
   navigation, move legs rather than slide, and hold rifles at the ready.
5. Watch them engage. Firing poses/montages should play with the existing
   yellow tracers. Verify they correctly face enemies when shooting.
6. Kill some soldiers: original HP/damage/death/despawn semantics persist;
   weapon geometry should disappear with the body. Capture-point logic
   should not change.
7. Play **2-player PIE**: both windows should show the same humanoid
   infantry, motion and combat effects. Friendly fire, player casualty
   feedback, and hold-to-revive should still function.
8. Verify FPS/memory usage; let the two sides fight for several minutes.
   Report animation warnings in Output Log, odd attachment rotations,
   static T-poses, incorrect mannequin orientation, or extra GPU pressure.

## Known limitations and follow-on milestones

- This is an **animation/visual pass**, not a squad-AI overhaul.
  Soldiers still acquire their own targets and advance individually.
  Squad readiness/rally points are milestone #15.
- Base spawners still deploy one initial group. Resource-limited outposts
  and reinforcements are milestone #14.
- NPC shots still trace from the server's torso-height origin rather than
  using exact gun barrel ballistic simulation. The new hand-attached
  rifle is cosmetic and may not be perfectly aligned with the existing
  center-of-mass tracer under all animation poses.
- Shared rifle placeholders and Manny models are for iteration, not
  production-quality soldier gear. We can later author BP subclasses
  with faction uniforms and modular weapon assets.
- A firing montage uses the same shared DefaultSlot as the player; test
  the animation in PIE before merging.
- The native class references known assets already tracked in the repo;
  a visual-data/Blueprint-driven setup should eventually replace these
  transitional constructor defaults once multiple factions/soldier types
  are introduced.

**Review status:** source change is not locally UE-compiled by the assistant.
User build and playtest are required before merge.
