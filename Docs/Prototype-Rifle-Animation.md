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

## Explicitly deferred

Full-body rifle jog/walk blend space, dynamic aim offset up/down,
two-hand IK attachment, fire/recoil montage, reloads, hit flinch, and
exact muzzle direction under all animations.

Physical muzzle firing remains the long-term design. The existing
`Use Visual Rifle Muzzle` toggle may remain disabled on the prototype
during animation alignment; we will enable it after validation.
