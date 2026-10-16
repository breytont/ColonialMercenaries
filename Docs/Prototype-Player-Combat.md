# Prototype 0.1 — Player hitscan combat

## Goal

Let the player participate in the existing third-person faction firefights:
hold right mouse to shoulder-aim, click left mouse to fire at hostile or allied
soldiers, and damage them through their existing server-authoritative health
component.

This slice **does not** implement an inventory weapon instance, ammo,
reloads, recoil animations, sound, player health, or NPC retaliation toward the
player. These are separate iterations.

## Implementation

- `UCMPrototypeWeaponComponent` lives on the existing C++
  `AColonialMercenariesCharacter`; no separate test pawn required.
- Component owns tunable **Base Damage**, **Max Range**, **Fire Interval**,
  **Muzzle Offset**, and **Draw Shot Tracers** defaults, exposed on the
  `BP_ThirdPersonCharacter`'s **PrototypeWeapon** component.
- Client input calls `RequestFire` on the owned replicated component.
  The server enforces fire rate, computes the view trace and muzzle trace,
  determines collisions, and applies `ApplyDamage` only to actors with a
  `UCMHealthComponent`.
- A first trace targets the center of the server-reconstructed follow camera.
  A second from the character's muzzle offset prevents shooting through cover
  that blocks the character's weapon origin.
- A cyan debug tracer is multicast for feedback. AI yellow debug tracers are
  unchanged. Shots are single-click / semi-automatic; holding the mouse button
  does not continuously shoot.
- Right mouse aiming temporarily rotates the character toward controller yaw,
  shortens and shoulders the camera, then restores movement-orienting and the
  normal camera on release. Aim state replicates.

## Editor assembly (required: Enhanced Input assets are .uasset files)

The assistant authored C++ and documentation; the following Editor asset
settings need a one-time change locally because our GitHub connector cannot
create or edit Unreal binary .uasset assets.

1. After checking out `feature/player-hitscan-combat`, generate VS project
   files (new C++ class) and compile Development Editor / Win64.
2. Open `Content/Input/Actions`. Create two **Input Action** assets:
   - `IA_Fire` — **Value Type: Digital (bool)**, default triggers.
   - `IA_Aim` — **Value Type: Digital (bool)**, default triggers.
3. Open `Content/Input/IMC_Default` and add:
   - `IA_Fire` → **Left Mouse Button**.
   - `IA_Aim` → **Right Mouse Button**.
   Keep existing look/movement/jump mappings unchanged.
4. Open `Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter`, select
   **Class Defaults**, find **Input > Combat** and assign:
   - `Fire Action` = `IA_Fire`.
   - `Aim Action` = `IA_Aim`.
   Compile and save this Blueprint.

## Gameplay verification

1. Open `Lvl_FactionTest` and run Play. Player movement/jump/mouse-look
   should function unchanged.
2. Hold RMB: character should face the camera's yaw and move to an
   over-the-shoulder view. Release to return to normal.
3. Left-click near a soldier. A **cyan tracer** should appear. On successful
   hit, the soldier should lose health (the Output Log already records the
   soldier's current health) and disappear after enough shots.
4. Shoot through solid scenery: projectiles must collide with the wall first.
5. Player fire affects either faction (full friendly fire); the player's
   participation may alter the outcome of the existing contested point.
6. Test with **2 players in PIE** after the initial solo test; verify both
   players can fire, the host resolves damage, and target deaths replicate.
   Further improvements may be needed for perfect shoulder-camera alignment
   under network latency. Do not mark this as network-verified until tested.

## Boundaries and known considerations

- New asset mappings are required; the C++ project may compile successfully
  while fire/aim do nothing if the two actions are not assigned and mapped.
- The current test map and spawners need no modification to validate firing.
- The player is not currently a faction AI target, can't be damaged by the
  squad weapons, and doesn't count as a capturing soldier.
- Temporary cube NPCs remain the targets; weapon mesh, sound, animation,
  reticle, ballistics, recoil, inventory and item-instance stats come later.
- Weapon tuning is exposed on the component's class defaults to support
  iterative balancing; the *final* equipment system will use data assets
  with reusable weapon/ammo definitions, per the GDD.
- Ownership and damage remain server-authoritative. The server does not trust
  target actors or damage amounts supplied by clients.
- A full design of input replication and camera-view reconciliation is a
  follow-up if multiplayer aiming tests uncover discrepancies.
