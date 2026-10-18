# Prototype: Rifle animation layer (UE 5.8)

## Goal

Replace the prototype's **right-hand, lowered weapon while RMB aiming**
with a convincing rifle-aim pose, without replacing the functioning unarmed
walking/jumping state machine or reworking player damage.

The project already contains mannequin rifle animation assets. No external
animation package is needed.

## Existing animation assets (already tracked in Git)

- `Content/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed`
  — current movement/jump Animation Blueprint.
- `Content/Characters/Mannequins/Anims/Rifle/MF_Rifle_Idle_ADS`
  — initial rifle-aim sequence to blend over the upper body.
- `Content/Characters/Mannequins/Anims/Rifle/AIM/AO_Rifle`
  — aim offset asset for follow-up pitch/yaw adjustment.
- `Content/Characters/Mannequins/Anims/Rifle/Jog/`,
  `Rifle/Walk/` and `Rifle/Jump/` — directional rifle locomotion for
  later improvements.
- `Content/Characters/Mannequins/Anims/Rifle/MM_Rifle_Fire`,
  `MM_Rifle_Reload` — firing and reload animation sequences for next step.

## C++ animation-data bridge

`UCMRifleAnimInstance` extends `UAnimInstance` and exposes values for the
AnimGraph, with unique `CM`-prefixed names to avoid collisions with
existing variables in `ABP_Unarmed`:

- `CMRifleAimAlpha`: interpolated 0→1 when RMB is held
- `CMIsAiming`: replicated character aim state
- `CMGroundSpeed`, `CMMovementDirection`, `CMIsInAir`
- `CMAimPitch`, `CMAimYaw`
- `AimPoseBlendSpeed`: adjustable blend speed

This state is **read** from the existing networked character; no new RPC,
input mapping, or per-Actor Tick.

## UE Editor asset assembly after the C++ build

**Important:** The assistant can commit C++ and instructions to GitHub but
cannot directly author or modify `.uasset` Animation Blueprints through
this connector. Make a *copy* of the starter Animation Blueprint and only
edit that copy.

1. Duplicate `ABP_Unarmed` into a new folder
   `Content/ColonialMercenaries/Animation` and name the copy
   `ABP_CM_Rifle`. Keep the original `ABP_Unarmed` unchanged.
2. In the copy, go to **Class Settings** and set **Parent Class**
   to `CM Rifle Anim Instance` (the new C++ `UCMRifleAnimInstance`).
   Compile. The prefab's current variables, state machine, movement, and
   jump logic should remain intact.
3. Within `ABP_CM_Rifle`'s **AnimGraph**, preserve the existing unarmed
   locomotion/airborne graph. Near the final **Output Pose**, add a
   `Layered Blend per Bone` that uses the existing locomotion as its
   Base Pose and `MF_Rifle_Idle_ADS` (Sequence Player) as one Blend Pose.
   In its Layer Setup, use the mannequin's upper spine (`spine_01`)
   as Branch Filter bone so legs retain locomotion.
4. Drive its layer blend weight from `CMRifleAimAlpha`. When RMB is
   released, the pose fades back to standard locomotion. Holding RMB
   smoothly blends the shoulders/arms into the rifle stance.
5. Set `BP_ThirdPersonCharacter`'s Mesh **Anim Class** to
   `ABP_CM_Rifle`. Save both assets.
6. Test idle, walking, jogging, jumping, RMB aiming, and left-click
   firing in `Lvl_FactionTest`. The rifle remains attached to
   `hand_r`. Adjust `PrototypeRifleRoot`'s relative transform as
   needed; do not permanently offset it to mimic the crosshair.

The exact AnimGraph connections depend on the layout of the original
`ABP_Unarmed`. Inspect a screenshot of its AnimGraph before rewiring the
final pose; do not delete any state machines or Transition Rules.

## Validation before merging

- [ ] Development Editor / Win64 C++ build succeeds.
- [ ] Aiming (RMB) visibly raises both arms and the rifle.
- [ ] Releasing RMB returns the existing movement animation.
- [ ] Movement, jumping and shooting still work.
- [ ] No asset compilation errors or warnings.
- [ ] Aim pose looks reasonable (fine hand and rifle alignment may need
      later left-hand IK and rifle retargeting).
- [ ] Commit/push the **new** `ABP_CM_Rifle` and changed
      `BP_ThirdPersonCharacter` .uassets, excluding VS generated files.

## Current preview + muzzle-direction test

Once the rifle's attachment has been positioned against
`MF_Rifle_Idle_ADS` in the character Blueprint, the **Use Animation Asset**
setting must be reverted. On `BP_ThirdPersonCharacter` → **Mesh**:

- **Animation Mode:** `Use Animation Blueprint`
- **Anim Class:** `ABP_CM_Rifle`

Compile and save. The one-off animation sequence was only for preview:
leaving it assigned prevents the normal locomotion/aim blending from playing.

In `BP_ThirdPersonCharacter` → **PrototypeWeapon** → Tuning:

- Enable **Use Visual Rifle Muzzle** to start each shot at the rifle tip.
- Enable **Fire Along Muzzle Forward** to trace physically along the rifle's
  local +X barrel direction, rather than auto-converging toward the camera.
- For troubleshooting, disable **Fire Along Muzzle Forward** to use the old
  camera-assisted shot direction. Disabling **Use Visual Rifle Muzzle**
  entirely returns to the prior character-relative muzzle position.

The forward-shot mode deliberately hits the ground, walls, or nearby objects
when the gun is physically pointed at them. While rifle pitch/aim-offset
animation is not hooked up, shots will not follow camera look up/down;
this is an expected, temporary limitation, not an aiming correction.

The aim-offset asset `AO_Rifle` and variable `CMAimPitch` already exist.
The next AnimGraph integration will animate vertical weapon pitch. The
asset's actual axis configuration and samples should be verified in the
Unreal Editor before deciding which nodes/values to connect. Muzzle direction
and hit resolution remain server-authoritative; two-player test still pending.

## Rifle support-hand IK (work in progress)

The hand_r-attached weapon now includes an editable scene component named
`PrototypeRifleSupportGrip` under `PrototypeRifleRoot`. Its default
position is roughly beneath the rifle fore-end (X=29, Y=0, Z=-8,
in rifle-root local coordinates).

`UCMRifleAnimInstance` exposes `CMLeftHandGripInRightHandSpace`, a
**bone-space** vector computed from the rifle/root relative transforms.
Because it is expressed relative to `hand_r`, the IK effector follows the
weapon's right-hand animation directly, with no last-frame world-position
dependence. No new actor Tick or replication messages are needed.

### Animation Blueprint setup (after a successful C++ build)

In `ABP_CM_Rifle`'s AnimGraph, keep the existing
`AO_Rifle -> Layered Blend per Bone` connections. Insert arm IK **after**
`Layered Blend per Bone` and **before** the existing foot `Control Rig`:

1. Place `Local To Component` and `Component To Local` nodes between
   Layered Blend and Control Rig; insert `Two Bone IK` between these
   conversion nodes. The flow is:
   `Layered Blend -> Local To Component -> Two Bone IK ->
   Component To Local -> Control Rig -> Output Pose`.
2. Set the `Two Bone IK` node's **IK Bone** to `hand_l`.
3. Set **Effector Location Space** to **Bone Space** and its
   **Effector Space Bone Name / Effector Target** to `hand_r`.
   (The exact property label can vary with the details UI.)
4. Drag **Get CMLeftHandGripInRightHandSpace** into the AnimGraph;
   connect it to `Effector Location` on the IK node.
5. Connect `CMRifleAimAlpha` to the IK node's `Alpha`.
   This leaves the existing unarmed pose alone when RMB is released.
6. Compile. Test with RMB aiming horizontally, looking high and low,
   jogging and jumping. With `PrototypeRifleSupportGrip` selected on
   `BP_ThirdPersonCharacter`, adjust its local position to bring the left
   palm to the foregrip without moving the rifle or muzzle.
7. If the elbow folds the wrong way, configure the IK node's **Joint
   Target Location** and space, or adjust the grip point. Do not use a
   new rotated muzzle as an alignment workaround.

**Known limitations:** A two-bone IK pass primarily fixes hand location.
The left palm's precise orientation and finger gripping need a
rotation-aware hand IK / Control Rig pass later. Foot Control Rig may
need ordering adjustments if it modifies arm transforms unexpectedly.

### Validate before merge

- Left hand tracks the gun as the right hand changes pitch.
- No drifting to the right during up/down aiming.
- Normal locomotion and falling still work when the rifle is lowered.
- The muzzle is not modified by the IK node; physical muzzle-forward
  hitscan remains intact.
- Save and push both `ABP_CM_Rifle` and `BP_ThirdPersonCharacter`
  (new grip target offsets) after local testing.

## Explicitly deferred

Full-body rifle jog/walk blend space, dynamic aim offset up/down,
two-hand IK attachment, fire/recoil montage, reloads, hit flinch, and
full pitch-adjusted muzzle direction under all animations.

Straight physical muzzle firing is now available as an optional prototype
mode; validate its transform in-editor before merging. This mode may need
pitch adjustment in the animation and server mesh animation evaluation.
