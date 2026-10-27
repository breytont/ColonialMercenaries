# Player Combat Fundamentals — Increment A1: Physical Muzzle Hip Fire and Rifle-Ready Stance

**Branch:** `feature/player-hip-fire`
**Base:** `feature/contested-frontier-blockout` (PR #16)
**Status:** Corrected source committed; pending Unreal Engine 5.8 Development Editor build and PIE verification.

## Feedback that supersedes the initial A1 approach

The user supplied an 11-second Unreal Engine playtest video. In the video,
releasing RMB lowers the character's right hand and physically mounted rifle,
so muzzle-forward shots hit the ground. The first proposed solution redirected
non-ADS bullets toward a camera target, but the user **explicitly rejected**
that approach: the gunshot must follow the **real barrel/muzzle direction**.

Correct design invariant:
- **The shot leaves the actual muzzle along the barrel's forward direction**
  in both hip and RMB aim modes, including when jumping or looking vertically.
- The player's **weapon animation/stance** must make the gun point where the
  player intends to fire. Do not move the muzzle, invent a camera-origin ray,
  or change ballistics to camouflage an incorrect pose.
- Different stance/recoil/sway/spread systems can be introduced later, but
  they must be visually represented through the weapon's orientation,
  recoil or stability model, not produce inexplicable bent trajectories.
- Cover between muzzle and target must still intercept the projectile.

## What changed in the corrected increment

1. Restored the existing physically barrel-aligned `TraceShot` and
   `bFireAlongMuzzleForward = true` behavior for **both** RMB and normal
   firing. No camera-convergence branch for hip fire and no random server
   hip-fire cone. The camera-convergence mode remains a diagnostic editor
   toggle only and should be disabled for this game's intended handling.
2. `UCMRifleAnimInstance` now keeps `CMRifleAimAlpha` targeting **1.0**
   for the armed playable character regardless of RMB state. This drives
   the existing `ABP_CM_Rifle` upper-body pose instead of blending back
   to unarmed arms/rifle-down pose. `CMIsAiming` still reflects RMB and
   governs the original character ADS camera/body behavior.
3. Kept the initial local circular hip reticle (plus while RMB aiming).
   It projects the **actual muzzle-forward predicted collision**. It is
   NOT a camera-aim guarantee and it does not yet expand with recoil.
4. No new Input Actions, AnimGraph assets or map actors for this first
   correction. A unique non-ADS hip-ready pose and more natural turn/aim
   locomotion remain future animation work.

## Critical limitations / next fixes

- Existing `MF_Rifle_Idle_ADS` upper-body pose is used as a stand-in for
  an armed **ready** pose. Hip and ADS shoulder animation may look too
  similar until we author/add a distinct lower ready animation.
- Outside RMB the character still uses orientation-to-movement; turning
  the camera while standing still does not automatically rotate the
  entire character to camera yaw. The muzzle may therefore point to the
  side of the camera's central field of view. This is a pose/turning
  problem, NOT a reason to alter bullet direction. We will validate
  and address controlled body/upper-body rotation in the next iteration.
- Look up/down must continue to agree with the animation aim offset.
  If the rifle itself does not pitch with view, update `AO_Rifle`/
  `ABP_CM_Rifle` rather than steering the trace away from the muzzle.

## Build and validation

1. Commit any outstanding editor map/.uasset changes, save, close Unreal.
2. Fetch origin, check out `feature/player-hip-fire`, then Pull.
3. Build **Development Editor | Win64** and open the existing map.
4. Without RMB: mannequin's rifle should stay raised in its existing
   rifle aiming layer, **not drop beside the legs**. Fire and inspect
   the tracer: it must leave the muzzle straight, not bend/veer
   toward the camera crosshair.
5. Rotate the **camera without moving** and then move/turn the
   character; note any separation between muzzle direction and
   center-screen. Report visual examples: these guide our body turn
   and gun handling work rather than changes to server ballistics.
6. RMB: camera shoulders in; gun still points along its actual barrel.
   Release RMB: **camera** returns but the **gun stays ready**.
7. Inspect shots aimed high/low and from beside a blocking wall. A real
   muzzle ray must be obstructed by the wall and remain straight.
8. Repeat basic test with two-player listen server to catch remote
   skeletal animation or authoritative muzzle misalignment.

**No UE build or runtime test has yet been performed by the assistant.**
