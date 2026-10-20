# Prototype casualty presentation and revive interaction (UE 5.8)

## Goal and scope

The Colonial Mercenaries GDD calls for an incapacitated state and teammate
revival, with headquarters respawn and medical expenses when not revived.
PR #11 established the server-authoritative health / down / instant-E-revive /
20-second respawn loop. This pass improves **feedback** and replaces instant
revival with a deliberate, server-validated **hold E for 1.6 seconds**.

This is **not** a new injury, loot, insurance, inventory, or faction system.

## Included

- Native health bar with numeric HP (top-left) and a short, muted full-screen
  red overlay when **replicated local HP decreases**. No extra network RPC
  or client-authored damage.
- Overhead **DOWNED** screen-space widget on incapacitated player pawns.
  Every peer sees it via the replicated health events. Hidden on revival.
- Contextual **HOLD [E] – REVIVE TEAMMATE** hint when a downed player is
  within the configured **230cm** range and reachable by a Visibility trace.
- Visible percentage and progress bar during the hold. The server owns the
  start time and target; this cosmetic widget projects the synchronized
  server time received from GameState for presentation.
- E `Started` -> request hold, `Completed`/`Canceled` -> cancel.
  On the authority, a short-lived 0.05s timer validates that the original
  downed target stays within reach / LOS and that the reviver is alive.
  It restores health (45 HP) only after the required 1.6 seconds elapsed.
- The local HUD refreshes the contextual hint at 10Hz via a UUserWidget
  timer. There is no added Actor Tick, no persistent server scan, and no
  new input assets. Revive actions are still bound to existing IA_Revive.
- Tuning fields on `BP_ThirdPersonCharacter` class defaults:
  `Revive Range`, `Revive Health`, `Revive Hold Seconds` and
  `Bleed Out Seconds`.

## Ownership and authority

Health damage, bleedout, and successful revival remain **server-only**.
Clients can request beginning or canceling a hold; the server validates
every part and is the only side that changes HP. The reviver's target and
start timestamp replicate to **the owning client only** for HUD progress.
The downed marker uses each client's health replication events.

## Test steps

1. GitHub Desktop: fetch, switch to `feature/casualty-feedback`; build
   **Development Editor / Win64** in Visual Studio. Open Unreal 5.8.
   There are no new asset creation or Blueprint wiring steps.
2. Play `Lvl_FactionTest` with **2 players** using the existing
   IA_Revive bound to E.
3. Take a hit: numeric HP / bar decreases, and the damaged player's screen
   briefly flashes red. The other player's screen should **not** flash.
4. Down player 2. On player 1's screen, a **DOWNED** marker should be
   visible above the incapacitated player's body.
5. Player 1 approaches within about **2 meters** and clear LOS.
   Confirm contextual `HOLD [E] – REVIVE TEAMMATE` appears.
6. **Tap** E: player 2 should **stay down**. Hold E for at least
   **1.6 seconds**; expect an increasing progress bar and % and then
   revival to 45 HP with restored movement/shooting.
7. Repeat: release E halfway through. Revive should **not** complete
   afterward. Holding E again must start progress from zero.
8. Move beyond 230cm or behind a wall while holding E: server should
   cancel, preventing revival (local HUD may reflect cancellation after
   a brief replication delay).
9. Let an incapacitated player bleed out without revival. After 20s,
   GameMode respawns a new pawn with full HP, reticle and health HUD.
10. Confirm existing aiming, shooting, Red-vs-Blue NPC battle,
    faction aggression and friendly-fire behavior are unchanged.

## Known prototype limitations

- Native widgets use basic fonts, a bar, and a solid flash overlay; visual
  art and accessibility polish are intentionally postponed.
- Screen-space DOWNED labels may appear through cover. The actual revive
  interaction still checks range and line of sight server-side.
- Progress is a client-side presentation of a replicated server start time;
  server validation, timer granularity and network delay determine the
  actual completion moment.
- The revive candidate scan uses a local timer and TActorIterator to inspect
  the very small number of co-op pawns; it is **not** run every frame.
- Final revive animation, sound, third-person body pose, wounded-state
  constraints, medkits, insurance, medical charges, lost equipment and HQ
  selection are deferred.

**This branch needs local compilation and testing; do not merge until verified.**
