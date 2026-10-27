# Player Combat Fundamentals — Increment A1: Reliable Hip Fire

**Branch:** `feature/player-hip-fire`
**Base:** `feature/contested-frontier-blockout` (PR #16)
**Status:** Source committed, pending Unreal Engine 5.8 Development Editor build and PIE testing.

## Goal

Make a normal left-mouse click reliably shoot toward the camera's
indicated target **without requiring RMB aim**, while maintaining the
physical muzzle origin and existing aimed-rifle behavior. This is the
first small gameplay-feel step from the post-Prototype-0.1 development
review. Sprint, crouch, reload, ammunition, detailed recoil/bloom and
proper weapon data assets are deliberate subsequent increments.

## Implementation

- `UCMPrototypeWeaponComponent::TraceShot` branches on the **existing**
  `AColonialMercenariesCharacter::IsAiming()` state, which is set
  through the established server-authoritative aim RPC.
- **Hip fire:** Trace the controller's view from the third-person camera
  to find an aim point, then calculate a ray **from the physical world
  muzzle toward that point**. A single Visibility ray from the muzzle
  resolves actual hits, so nearby barricades still intercept fire even
  if the camera sees beyond them.
- A server-validated shot has random angular dispersion inside a
  designer-tunable cone `HipFireSpreadDegrees` (initially **2.5°**
  half-angle). Random shot direction and collision happen only on the
  server; the same approved hit endpoint replicates through the
  existing muzzle/tracer multicast.
- **Aimed fire:** Preserve existing `bFireAlongMuzzleForward` and
  weapon animation system; no regression to barrel-aligned ADS aiming.
  Designers can disable this property to use camera convergence in both
  modes if necessary.
- The existing local predictive `GetPredictedShot` samples the
  **center** of the hip cone without randomness; it never damages
  actors and is not a guaranteed hit indicator.
- The native `UCMPrototypeReticleWidget` now shows a plain
  **hollow-circle** marker when hip firing, and retains the prior
  **plus sign** for aimed shots. The marker projects the predicted
  muzzle-to-target hit point and becomes amber if it is blocked.
  This is **not** the final dynamic recoil/spread reticle.
- No new Input Action, Blueprint, level actor, replicated gameplay
  property or special HUD asset required for this increment.

## Test plan (user in Unreal Editor)

1. Save/commit any uncommitted assets on
   `feature/contested-frontier-blockout` and close the editor.
2. Fetch origin, switch to `feature/player-hip-fire` in GitHub
   Desktop, pull, then build **Development Editor | Win64**.
3. In `Lvl_ContestedFrontier`, start single-player PIE. Without
   holding RMB, the **circular reticle** should be visible.
4. Aim the camera at a wall/ground/target and left click. Shots should
   leave the rifle muzzle and generally travel **toward the viewed
   target**, with plausible hip dispersion rather than drilling the
   ground wherever the lowered rifle happens to point.
5. Stand close behind solid cover with the third-person camera
   looking past it. Shots must still impact the cover when the muzzle
   is blocked (no camera-origin shooting through walls).
6. Hold RMB: crosshair changes to **+** and previous aimed-rifle
   behavior persists. Test aiming up/down, jumping and moving.
7. Test on a 2-player listen server, including shots fired by the
   **client** (second PIE window): hits/damage/tracers should agree
   with server decisions. Verify mission/interactions still work.
8. Optionally tune `Hip Fire Spread Degrees` on the player weapon
   component in `BP_ThirdPersonCharacter` after collecting feedback.

**Remaining work in Milestone A:** Decide hip-vs-ADS spread,
movement inaccuracy, dynamic reticle expansion/recoil recovery,
sprint/crouch with replicated speed/posture, magazine/reload and
ammo inventory. Keep common weapon stats/data separate from
animation and user interface once the first gun feels correct.

**No engine build or runtime test was performed by the assistant.**
