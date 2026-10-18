# Player casualties and revival prototype

UE 5.8 / multiplayer server-authoritative. Source branch: `feature/player-casualties`.

## GDD alignment
The GDD says players become incapacitated and can be revived, otherwise return
to headquarters with medical expenses and potentially recoverable/lost gear.
This milestone implements **damage -> downed -> teammate revive or respawn**.
No actual headquarters, financial account, medical charge, inventory drop or
persistent recovery has been implemented yet. **PlayerStart** temporarily
represents headquarters for the respawn test.

## Implementation
- Character now owns the same replicated `UCMHealthComponent` and
  `UCMFactionComponent` as NPC soldiers. No level Blueprint logic.
- NPC targeting detects living hostile soldiers **and players** using
  health/faction components, with the same LOS, movement and server-only fire.
  Friendly-fire damage still applies when an actor blocks an actual bullet,
  regardless of whether the NPC intentionally targeted that actor.
- **Neutral/unassigned player faction does not draw faction aggression.**
  The player company starts independent in the GDD. For this particular
  firefight test, **explicitly set the player faction to Blue** in the BP;
  Red soldiers will then regard players as hostile. Later allegiance and
  faction disposition will replace the temporary different-ID rule.
- At 0 HP, the character is incapacitated: stops walking, cannot shoot/aim,
  stays in the world for a teammate, and starts a 20 s server bleed-out timer.
- Another living co-op player can press **E** within 230 cm and unobstructed
  visibility to immediately revive the nearest downed teammate to 45 HP.
  A server RPC validates state, distance and line of sight. No client
  health edits are trusted.
- If time runs out, authority unpossesses the downed pawn, uses the active
  GameMode's `RestartPlayer`, and destroys the old pawn. Existing pawn
  Blueprint and firing/aim reticle components should be recreated normally.
- New minimal local C++ HUD displays `HP 100 / 100` at top-left and
  `INCAPACITATED | Teammate: press E to revive` when downed.
  It listens to replicated events instead of polling health on Tick.

## Unreal Editor setup (required after compiling)
1. In GitHub Desktop fetch + switch to `feature/player-casualties`,
   regenerate Visual Studio files if prompted, build **Development Editor,
   Win64** and open UE 5.8.
2. In Content Browser create an **Input Action** named `IA_Revive`,
   Value Type **Boolean** (e.g. `Content/ColonialMercenaries/Input`).
3. Open **IMC_Default** (the existing input mapping context) and add
   `IA_Revive` bound to the **E** key (no modifiers or triggers necessary).
4. Open `BP_ThirdPersonCharacter`. In **Class Defaults** (search
   `Revive Action`) set `Revive Action = IA_Revive`.
5. Select the new **Faction** component in the Components hierarchy.
   In Details set **Faction Id = Faction.Prototype.Blue**.
   This is a *temporary test allegiance*, not the intended long-term
   independent-company disposition.
6. Compile/save `BP_ThirdPersonCharacter`, `IA_Revive`, `IMC_Default`.

## Suggested acceptance tests
1. Single-player `Lvl_FactionTest`: find a Red soldier; as Blue player,
   stand in range/LOS and verify incoming shots decrement your top-left HP.
2. After five typical 20-damage hits from full HP, confirm
   `INCAPACITATED` status, no movement/fire, and no instantaneous
   disappearance. Other actors may still shoot around the downed body.
3. Wait 20 seconds. Verify new player pawn spawns at a **PlayerStart** with
   full health, functional gun/aim and fresh HUD.
4. Two-player PIE: make one player go down. Move the other player within
   about 2 meters, face the downed player (no wall in the way), press E.
   The downed player should recover to 45 HP and walk/shoot again; their
   scheduled respawn should not fire later.
5. Player 2 can independently go down/revive/respawn and should have their
   own health HUD. Confirm the Red/Blue NPC-versus-NPC battle still works.
6. Try pressing E from more than 230 cm or through a wall: no revive.
   Check that a downed player cannot deal damage with a delayed shot RPC.
7. If NPCs do not attack, check **Faction Id** on your player and that
   Red soldiers are close enough to detect you (default detection 6500 cm).
   They still choose visible, closer hostile soldiers when those exist.

## Deferred
- Bleed-out timer visible in UI, reviver progress/animations, incapacitated
  animation/ragdoll, healing/medkits, armor/protection zones and damage types.
- Actual HQ respawn selection, medical expenses, equipment drops/insurance,
  capture/wounded persistent state, recovery operations and medical coverage.
- Independent mercenary hostility based on reputation, contracts and
  aggression (current player Blue tag is testing scaffolding).
- Proper visual HUD art, ally revive indicator and accessible UI.
- Server-authoritative inventory persistence after respawning.
