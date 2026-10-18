# Prototype: Rifle fire animation, camera recoil, and spatial audio hooks

## Scope

Add a first layer of firing feedback to the existing third-person muzzle-forward
hitscan weapon without changing server-owned damage, rate limits, or actor aim
logic. The prototype is intentionally split between C++ events and edit-ready
Unreal assets.

### C++ changes

- **Approved shots only:** after server validation, `ClientApplyRecoil` is
  sent to the weapon-owning player and `MulticastShotFeedback` is sent to
  relevant clients. Client clicks rejected for fire rate do not play shot
  feedback.
- **Animation:** `PrototypeWeapon.FireMontage` may reference a rifle-fire
  montage. The montage plays on each client receiving the shot.
- **Camera kick:** editable `RecoilPitchDegrees` and `RecoilYawDegrees` apply
  only to the owning player. These are temporary instantaneous input kicks,
  not a production recoil-spring/camera recovery system.
- **Debug VFX:** brief orange muzzle marker, cyan tracer, and yellow impact
  mark show physical shot start, trajectory, and blocking impact. Each is
  independently configurable. An `OnShotVisuals` Blueprint event provides
  the source/impact for later Niagara effects.
- **Gunfire:** 3D sound plays at the actual server-calculated muzzle location.
- **Impact:** a separate 3D sound plays at the actual blocking-hit location.
- **Near miss:** for the local player (not the shooter), compute closest point
  on the shot segment to the listener; play a whizz in a configurable 60–230
  Unreal-unit band. This has no per-frame scanning and does not simulate
  projectile travel time. Presently it is a *player weapon* feature, not
  enabled for the existing standalone soldier AI gunfire.

### Starter WAV source clips

The branch includes three synthetic, license-independent **temporary**
mono WAV files in `PrototypeAudio/`:

- `Gunshot_Prototype.wav`
- `Impact_Prototype.wav`
- `NearMiss_Prototype.wav`

They are **not** Unreal .uassets. They need to be **imported in Unreal Editor**
to become `SoundWave` assets. They are simple generated placeholders, not
finished firearm or environment sound design. No existing game asset is
overwritten.

## Acceptance test setup in Unreal Engine 5.8

1. Check out `feature/player-firing-feedback`, regenerate VS project files,
   and compile **Development Editor / Win64**.
2. In the Content Browser, create `Content/ColonialMercenaries/Audio`.
   Click **Import** (or drag WAV files from Explorer) and import the three
   files in `PrototypeAudio/` from the repository.
3. For 3D gunfire, create a **Sound Attenuation** asset in the same folder,
   e.g. `ATT_CM_Gunshot`. Set **Enable Volume Attenuation** and **Spatialize**
   to true; choose a sensible inner radius and falloff distance. Exact
   distances can be tuned later.
4. Open `BP_ThirdPersonCharacter`, select **PrototypeWeapon** component.
   In **Weapon > Audio**, set `GunshotSound`, `ImpactSound`,
   `NearMissSound` to imported SoundWave assets, and
   `ShotAttenuation` to `ATT_CM_Gunshot`. Keep **Use Visual Rifle Muzzle**
   and **Fire Along Muzzle Forward** enabled.
5. Under `Content/Characters/Mannequins/Anims/Rifle`, right-click
   `MM_Rifle_Fire` and create an **Anim Montage** (the relevant context-menu
   wording may be **Create Anim Montage**). Save as
   `Content/ColonialMercenaries/Animation/AM_CM_RifleFire`.
   Check the montage's slot is `DefaultSlot`.
6. In `BP_ThirdPersonCharacter`'s **PrototypeWeapon** component, assign
   `FireMontage` to `AM_CM_RifleFire`.
7. In `ABP_CM_Rifle`, **move the existing DefaultSlot node**:
   - Keep `Main States` feeding the **Base Pose** of the existing
     `Layered Blend per Bone` rifle aiming node.
   - Connect `Layered Blend per Bone` **into DefaultSlot**, then
     `DefaultSlot` into `Local To Component` → `Two Bone IK` →
     `Component To Local` → foot `Control Rig` → Output Pose.
   - This lets the montage show *after* rifle aim offset blending; otherwise
     the rifle-aim upper-body layer can override the shot animation.
   - **Limitation:** a full-body montage can temporarily interrupt leg
     locomotion. Later refine to an upper-body-only montage slot and mask.
   - Do not break the existing `AO_Rifle`, `CMRifleAimAlpha`,
     `CMLeftHandGripInRightHandSpace`, or foot IK connections.
8. Compile and save `ABP_CM_Rifle` and `BP_ThirdPersonCharacter`.

## Playtest

- In `Lvl_FactionTest`, test RMB aim and LMB fire. Each accepted shot should
  create a brief muzzle flash/debug tracer and a small camera kick.
- The fire montage should play briefly, then transition to the normal
  rifle stance; normal movement and jumping should resume.
- Test shots into nearby concrete cubes: a yellow hit marker and impact
  audio occur at the actual collision.
- Test shooting a soldier: damage/death still work. Test firing into the
  ground while barrel is down: it should hit the ground, **not** camera aim.
- Check whether gunshot volume changes when moving away from another shooter
  in **two-player PIE** (not yet verified).
- Near misses only trigger for **other players** with a local listener close
  to the segment. In solo PIE, firing your own gun won't trigger whizz audio.
- If audio is unexpectedly silent, verify the import produced SoundWaves,
  check the assigned sound fields, and temporarily remove `ShotAttenuation`
  to test without distance falloff.
- Commit/push imported .uassets, new montage and any edited character or
  animation Blueprints. Exclude generated Visual Studio files and editor
  preview-profile settings.

## Architecture / follow-ups

This intentionally does not implement dynamic reverb, acoustic simulation,
ballistic propagation, physical projectile travel, audio occlusion setup or
different surfaces. Unreal **Audio Volumes**, Submix effects, Sound
Attenuation with occlusion, and/or MetaSounds can provide distance, indoor
reverb, and environmental differentiation in later work.

For the final design, consider weapon-specific audio data assets and
material/surface-dependent hit sounds (Physical Materials); this prototype
has one general impact cue. Unify NPC and player shot presentation when
building the reusable weapon system.

**Crosshair, proper Niagara muzzle flash, animated recoil recovery, impact
decals, and advanced acoustics remain future changes.**
