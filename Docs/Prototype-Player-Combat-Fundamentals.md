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

## Milestone A1 refinement: hip-ready pose, ADS stability, dynamic rings (Oct 27)

**User video feedback:** The restored always-ready rifle is materially
better, but the hip pose still looks like ADS. The user wants the camera
to orbit freely when not shooting, to turn toward the cursor only as
hip fire begins, to hold a steadier rifle while walking in ADS, and
to have both reticle modes use expanding circles with sustained fire.

### C++ implemented on this feature branch — pending rebuild and PIE test

- LMB now uses Started/Completed/Canceled plus a 0.05-second held-fire
  timer. The weapon itself still validates fire cadence and damage
  **on the server** (existing 0.2-second FireInterval).
- Without RMB, the player can freely orbit the camera while walking
  normally. During LMB hold only, CharacterMovement uses
  `bUseControllerDesiredRotation` for **smooth body rotation** toward
  camera yaw. Shots are held until the **body heading** is within
  `HipFireAlignmentToleranceDegrees` (default 12°). The weapon
  fires along its **actual barrel forward axis**, not the camera.
  When LMB is released, ordinary free orbit returns.
- RMB ADS remains camera/body facing mode. ADS MaxWalkSpeed is now
  `AimedWalkSpeed` (default 250 cm/s) so locomotion makes fewer
  abrupt weapon/camera movements. The reticle still projects the
  true physical muzzle trace. **Additional** animation stabilization
  will be needed if the weapon visibly bobs in ADS.
- The native predicted-muzzle reticle uses **a circle in both states**:
  smaller ADS, larger hip. It increases with velocity and after
  **server-confirmed** shots, then recovers (`ReticleBloomPerShot`,
  `ReticleBloomRecoveryPerSecond`); the HUD does not perform any
  authority decisions or alter hit results.
- `UCMRifleAnimInstance` exposes `CMShoulderAimAlpha` (0 hip-ready,
  1 ADS) independent of `CMRifleAimAlpha` (equipped upper-body layer).
  Current rifle blend will remain visibly ADS-like until the actual
  hip-ready pose is authored and connected in the Animation Blueprint.
- No new input asset needs to be created.

### Unreal animation authoring required

GitHub source edits cannot directly author or rewire the UE5.8 binary
`ABP_CM_Rifle.uasset`. **Do not randomly rotate PrototypeRifleMuzzle**
to hide a wrong hand/weapon animation.

1. In Unreal make a separate editable copy of
   `MF_Rifle_Idle_ADS` named `AS_CM_Rifle_HipReady` under
   `Content/ColonialMercenaries/Animation` (without modifying the
   original mannequin asset). Author an actual supported
   **at-hip, two-hand hold** pose in the animation editor, adjusting
   the shoulders/elbows/hands to the gun's intended hip location.
   Verify the right-hand anchored rifle mesh and left-hand IK.
   (The exact pose workflow depends on the existing Skeleton,
   animation editor and Control Rig; review a screenshot before
   editing keys or adding procedural Control Rig nodes.)
2. In `ABP_CM_Rifle` AnimGraph, do **not** remove the existing
   locomotion state machine, arm/hand IK, foot control rig, AO_Rifle
   or Layered Blend per Bone. Instead introduce a **Blend** for
   the single upper-body rifle input: Pose A = new hip-ready
   sequence, Pose B = current ADS/AO_Rifle path, Blend Alpha =
   `CMShoulderAimAlpha`. Retain `CMRifleAimAlpha` as the
   Layered Blend per Bone weight. Keep the hand IK downstream.
3. ADS should use the original aim offset and physical barrel.
   Hip-ready should keep a believable low-ready line that remains
   capable of firing forward when the body is turned. Optionally
   build a hip pitch aim offset later; never steer bullets away
   from the barrel to hit the center cursor.
4. Test in editor, save the changed `.uasset` files and commit
   separately to the feature branch.

**Accuracy caveat:** The reticle's temporary bloom reflects shot
cadence/camera recoil and visual instability. We have NOT implemented
a new muzzle-driven bullet spread/physical sway model; all shots
continue exactly along the *actual* muzzle, and the HUD cannot
legitimately claim that a wider circle equals random cone hit
deviation. Physical recoil/sway should be added in a later scoped
increment by actually perturbing the held weapon pose on server
and clients, keeping projectile origin/direction consistent.

### Build/PIE test plan for new C++ layer

1. Close Unreal, fetch/pull `feature/player-hip-fire` and build
   Development Editor / Win64.
2. In `Lvl_ContestedFrontier`, move camera with RMB released while
   stationary: body should **not** automatically face camera.
3. Press-and-hold LMB facing away from body: the character should
   **rotate naturally** toward the camera and then repeatedly fire
   along the physical muzzle. Release LMB: free orbit returns.
4. Hold RMB while walking: slower steadier shoulder-aim movement,
   normal character-facing behavior. Release RMB and observe movement
   speed return to the character BP's original configured value.
5. Both reticles should be circles; diameter increases after repeated
   accepted shots and while moving, and decreases as firing stops.
6. In two-player listen-server PIE, test both windows' shots,
   hit damage and character rotation. Confirm mission/interactions
   and the autonomous Red/Blue battle remain intact.
7. Once code tests pass, author/connect the distinct hip-ready
   animation and re-test muzzle angles and crosshair alignment.

The pose asset is **not implemented by these C++ commits**.
