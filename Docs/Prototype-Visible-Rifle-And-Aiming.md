# Prototype: visible rifle and smooth aim camera

## Why this step
Prototype 0.1 already supports left-click hitscan fire, RMB shoulder aim,
authoritative damage against NPC soldiers, and autonomous capture fights.
This increment connects **a visible gun** to the character's hand and improves
the aim-camera transition. It is intentionally not the animation, inventory,
or production weapon-equipping system.

## Implementation
- The player C++ character gains a `PrototypeRifleRoot` scene component
  attached to the `hand_r` mannequin bone.
- Basic engine cube / cylinder meshes form a receiver, barrel, stock, grip,
  and sight. No external mesh assets are required.
- Components have collision disabled (weapon geometry does not interfere with
  player capsule, navigation, or hitscan traces).
- `PrototypeRifleMuzzle` marks the tip of the barrel. By default, the existing
  `PrototypeWeapon` component starts server-side rays and cyan tracers here.
  In the weapon component's tuning properties, `Use Visual Rifle Muzzle`
  can be disabled to restore the older fixed `Muzzle Offset`.
- The camera interpolates from existing follow settings into the RMB
  shoulder aim with a customizable **Aim Camera Blend Duration** (default
  0.22 seconds). Releasing RMB reverses the transition smoothly.
- Camera interpolation runs only for local players and only while
  transitioning, using a timer; no permanent Actor Tick.
- Existing Enhanced Input actions, authoritative weapon damage, NPC
  combat AI, faction points and Blueprints are otherwise unchanged.

## How to test (UE 5.8)
1. Close Unreal Engine before switching branches and regenerating project files.
   Check out `feature/visible-rifle-and-aim-transitions`; build
   Development Editor / Win64 in Visual Studio.
2. Open `Lvl_FactionTest` and Play. The third-person player should now
   have a rough rifle-shaped assembly near the right hand.
3. Hold and release **RMB**. Aiming should transition smoothly rather than
   snapping the camera instantaneously. **LMB** should still fire cyan
   traces, hit NPC soldiers, and respect collision with walls.
4. Move and jump while aiming and hip-firing. Ensure there is no regression
   in third-person controls or skeletal animation.
5. If the rifle placement is poor: open
   `Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter`, select the
   inherited **PrototypeRifleRoot** component and adjust its *relative*
   Location and Rotation. All five parts and the muzzle move together.
   You can edit the individual mesh components for finer visual alignment.
   Keep the rifle's barrel oriented generally forward from the player.
6. Optionally tune **Aim Camera Blend Duration**, **Aimed Camera Arm Length**,
   and **Aimed Camera Socket Offset** on character Class Defaults. The
   previously saved BP camera offset is preserved.
7. Check for cyan traces coming out of the weapon tip, not the torso. If
   bullets collide unexpectedly while adjusting the weapon, temporarily
   disable **Use Visual Rifle Muzzle** on the `PrototypeWeapon` component.
8. Once validated, save the Blueprint if modified, then commit/push the
   related asset (exclude Visual Studio generated files) before PR merge.

## Known limitations
- This is a placeholder model made of gray engine primitives. It can look
  rudimentary and will likely need manual attachment-transform tuning.
- The mannequin's existing animation Blueprint does **not** yet have
  rifle-holding, aim-offset, IK, firing or recoil animations. The prop moves
  with the animated right hand but is not held by both hands.
- In networked play the server uses its character-side muzzle transform.
  We still need a multiplayer aiming/alignment test and may need improved
  character animation updates on distant or remote characters.
- Fire rate, damage, and ammunition systems remain the existing prototype.
- Final weapons will be equipment instances driven by game data assets,
  consistent with the GDD; this geometry is intentionally replaceable.
