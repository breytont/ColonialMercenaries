# Prototype: physical-muzzle projected reticle (UE 5.8)

## Purpose
Make aim feedback correspond to where the **gun itself** would actually shoot,
rather than a misleading fixed center-screen crosshair. Physical barrel
orientation, muzzle obstruction, and the existing optional camera-assisted
mode all remain intact.

## Design
- Refactor the existing shot trace into one reusable, side-effect-free
  `UCMPrototypeWeaponComponent::TraceShot`.
- The authority uses it for damage, collision and world-position shot FX,
  exactly as before.
- The local player's widget calls `GetPredictedShot` using the same trace,
  then projects the predicted first blocking hit (or maximum range endpoint)
  into screen/UMG coordinates.
- A small `+` marker appears while holding RMB. Its position reflects the
  physical barrel's projected point of impact; it can move noticeably when
  the rifle points at the floor or nearby cover.
- If the trace hits an object (including any blocking actor), the marker
  changes from cyan to amber. If it travels unobstructed to max range,
  the marker is cyan.
- The marker hides when RMB isn't held or the impact lies behind the camera.
- The UI is created on the owning client in `PawnClientRestart` /
  `SetupPlayerInputComponent`; no HUD Blueprint setup required.
- This is the **only** newly per-frame update: the visible local reticle
  tracks animated gun and camera movements using `NativeTick`. There is
  no actor Tick, no global world scan, and only one extra local Visibility
  trace per displayed frame. No reticle state is replicated.
- The widget is a C++ `UUserWidget` and is replaceable through the
  `ReticleWidgetClass` on the character's Class Defaults. It uses a minimal
  native TextBlock as the default presentation.

## Acceptable mismatch
The reticle is a **local prediction**, not a claim that the server will
award a hit. Mesh/aim pose animation on the server may differ slightly from
the owning client due to replication delay or update timing. Server
authoritative muzzle traces remain unchanged. This warrants a separate
multiplayer aim test later, especially if discrepancy is noticeable.

## Local verification
1. Check out `feature/physical-muzzle-reticle`, regenerate VS project
   files, and build Development Editor / Win64 in Unreal Engine 5.8.
2. Load `Lvl_FactionTest` and Play as the usual third-person character.
   No new Unreal assets or editor binding steps should be needed.
3. Hold RMB. A small crosshair should appear at the gun's **actual predicted
   impact** projected onto the screen (not always the screen center).
4. Look up/down and at nearby walls and floors. Crosshair should move with
   the real muzzle. When a wall blocks the barrel, the reticle should
   follow the wall intersection and appear amber.
5. Fire with LMB at a stationary wall / NPC. Compare the cyan actual tracer
   to the reticle's estimated impact, allowing minor movement/recoil lag.
   Existing impacts, enemy damage, gunshot/impact audio and animation
   should still work.
6. Release RMB. Crosshair should hide; movement and physical hipfire
   remain functional.
7. Test Play-in-Editor with two players. Each client should see only
   its own reticle and shouldn't alter the other player's aim.
8. If the widget fails to appear, check errors in Output Log and whether
   `BP_ThirdPersonCharacter` still inherits from the C++ character.
   The class-default `ReticleWidgetClass` can be changed to a Widget
   Blueprint derived from `CMPrototypeReticleWidget` for UI iteration.

## Deferred features
- Proper hit-confirmation feedback, enemy/ally target identification,
  tactical obstruction labels and HUD framing.
- Recoil bloom and weapon-spread visualization.
- Smoothing near-screen-edge aim markers.
- Server/client muzzle alignment correction and lag compensation if needed.
- User-adjustable reticle graphics/colors and accessibility settings.
- More general data-driven weapon UI (the prototype uses one weapon).
