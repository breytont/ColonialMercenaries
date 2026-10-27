# Prototype 0.1 — First mercenary contract: Relay Intelligence Recovery

## Purpose and GDD basis

The *Colonial Mercenaries* GDD, sections **4, 10, 20 and 22–23**,
calls for accepting contracts from a company headquarters, traveling
to a physical objective, returning for payment and validating the
financial transaction on the host. The GDD specifies that contracts
have employers, objectives, rewards, conditions and consequences.
Later development will include world-state and procedural contracts,
mission persistence, reputation and true company finance.

This milestone deliberately implements **one hardcoded prototype
contract type**, no contract generation, no UI menus, no equipment
purchase, no reputation changes, and no long-term save. It is
**a development test**, not the finished mission framework.

## Contract premise

**Title:** Relay Intelligence Recovery

**Employer:** Independent frontier broker (placeholder; eventually
choose employers from actual faction/company relationships)

**Brief:** Recover an intelligence package from the contested relay
compound. The autonomous Red–Blue war continues independently; the
player must physically collect the cache and return to HQ to get paid.

**Reward:** 1,500 prototype credits to the **shared company ledger**.
Credits are granted **exactly once**, on the authoritative server, and
never once per connected player. Initial balance is configurable.

**Steps:** Available → Accepted → Intel Recovered → Paid.

- Interaction at the HQ terminal starts the mission, if the
  linked cache is configured.
- Interaction with the cache at the relay advances to recovered **and physically attaches the intel prop to that specific player's character**, with replicated carrier identity.
- **Only the physical carrier** may deliver at HQ: interaction after recovery pays the company and marks the
  contract complete.
- Soldiers capturing the relay point cannot retrieve the cache.
- No automatic payment on relay ownership change.
- If the carrier is incapacitated or its character/controller is destroyed,
  the physical package drops near that position. A surviving teammate must
  retrieve it and physically bring it to HQ; the mission stays active.
- Available/active/completed state and balance are replicated on
  the terminal. The cache replicates its **carrier and delivered state**;
  its actor attachment follows the carrier.

## Architecture and ownership

- `ACMPrototypeContractTerminal` is a placeable replicated
  authoritative **state machine** and temporary shared company-credit
  ledger. C++ `CMPrototypeContractTerminal.h/.cpp`.
- `ACMPrototypeIntelCache` is a linked, replicated physical objective.
  C++ `CMPrototypeIntelCache.h/.cpp`.
- The two objects use editor instance actor references to one another.
  There is no `GetAllActorsOfClass` scan or Level Blueprint logic.
- `AColonialMercenariesCharacter` adds optional Enhanced Input
  `InteractAction`, bound on `Started`. It finds only near-world-dynamic
  actors via an overlap at interaction time and sends **one owned server
  RPC** containing a candidate actor. The server rechecks actor type,
  player alive/controller, full 3D distance (≤300cm) and an unobstructed
  visibility trace, then dispatches to the terminal or cache.
- Ground interactables use query-only WorldDynamic collision and do not
  obstruct player navigation or bullets; their mesh blocks the Visibility
  channel for interaction traces. **Carried intel disables its interaction
  collision** so pressing F at HQ selects the terminal, not the prop.
- No Actor Tick. World-space status labels show the contract step and
  current funds.
- There is **no gameplay money in PlayerState yet**, no save-game,
  server reconnect/restart persistence or active merchant; the terminal
  ledger is a narrow transitional prototype.

## Setup in Lvl_ContestedFrontier (UE 5.8)

Before pulling, **Save All**, close Unreal and commit any map and
External Actor changes on `feature/contested-frontier-blockout`.
Pull branch updates and build `Development Editor | Win64`.

### Enhanced Input asset configuration

In the existing `Content/Input` or `Content/ColonialMercenaries/Input`
asset directory:

1. Create **Input Action** `IA_Interact`, **Value Type Boolean**.
2. Open the already-used `IMC_Default` mapping context,
   add `IA_Interact` assigned to the **F** key.
   Leave `IA_Revive` bound to E.
3. Open the existing playable player character Blueprint
   (`BP_ThirdPersonCharacter` or the actual subclass used by this
   map) and set **Interact Action** = `IA_Interact`.

### World actors

1. Place **CM Prototype Contract Terminal** near the operations
   building's west-facing wall. Initial recommendation
   `HQ_ContractTerminal` at approximately **(350, -9450, 0)**,
   rotation **(0, 0, 0)**. Confirm accessible from the open HQ
   courtyard and not hidden inside the building.
2. Place **CM Prototype Intel Cache** on the relay foundation,
   e.g. `Relay_IntelCache` at **(-350, 0, 120)**.
   Keep it clear of the utility hut and low cover.
3. On `HQ_ContractTerminal`, assign **Intel Cache** =
   `Relay_IntelCache` via eyedropper in Details.
4. On `Relay_IntelCache`, assign **Contract Terminal** =
   `HQ_ContractTerminal`.
5. Leave **Reward Credits** = 1500 and **Starting Company Credits** = 0.
6. Save All, commit `.umap`, relevant External Actor packages and new
   `IA_Interact` / edited `IMC_Default` / edited character Blueprint.

### 1-player PIE acceptance test

1. Spawn at HQ, walk up to the visible contract terminal,
   read **[F] ACCEPT: RELAY INTEL**, press **F** once.
   Expect terminal label **ACTIVE** and no change to balance.
2. Walk up the service road and into the contested relay compound.
   The existing Red/Blue battle should continue unaffected.
3. Approach **Relay_IntelCache** and press **F**.
   Expect the visible intel briefcase to attach to that **player**, while
   HQ contract stage becomes **IntelRecovered**. The ground cache
   disappears because it is physically carried, not because the item
   teleported to a shared inventory.
4. Return to HQ **as the carrier** and interact once.
   Expect **CONTRACT COMPLETE - PAID** and company funds = **1500**.
   First verify that an unladen teammate at HQ **cannot** claim payment.
5. Press F repeatedly. Balance must **stay 1500**.
6. Stop PIE; current design intentionally resets the prototype
   state and balance on a new session. Persistence is a separate
   Prototype 0.1 milestone.

### 2-player multiplayer acceptance test

- Both clients observe the same stage and company funds.
- Either player can accept and either can retrieve the cache, but **only
  the player currently carrying the physical package** can turn it in.
  Exactly **one company payment** of 1500.
- Have **Player 1 remain at HQ** while Player 2 recovers the intel.
  Player 1's attempt to turn it in must produce **no payment**.
  Player 2 must travel back and deliver it.
- Test Player 2 becoming incapacitated before delivery: the cache should
  drop near the body, then Player 1 can retrieve and deliver it.
- An NPC capturing the relay point alone must not advance the
  contract. An unaligned/neutral player remains free to interact
  without automatically becoming hostile to Red or Blue.
- Distant client cannot trigger remote interactions; the server
  checks actual player position and line of sight.
- Existing revive and combat inputs continue functioning.

## Follow-ups, not part of first test

- Move company finance and contract progress out of the physical
  HQ terminal into persistent, replicated company systems.
- Adopt data assets defining contracts, employers, prerequisites,
  rewards, expiry, objectives, and world-state links; build UI.
- Replace the single-purpose visible carried intel actor with proper
  inventory item/equipment and handoff interfaces later; this prototype
  already drops on incapacitation, but **intentional handoff and broader
  inventory management are not implemented**.
- Add actual employer relationship and red/blue war consequences.
- Add procurement with authoritative spending, AA support restriction
  and save/load; satisfy remaining Prototype 0.1 acceptance criteria.

## Important caveat

GitHub commits are source changes, **not an Unreal compile or PIE test**.
Any compile, input-binding, collision or multiplayer defects must be
fixed based on editor/build feedback before merging PR #16.

## Playtest checkpoint — October 27, 2026

**User-tested in single-player PIE:** the full HQ terminal → relay
intelligence cache → HQ payout interaction loop worked as expected.
Input mapping `IA_Interact` bound to **F**, both linked world actors,
contract stages and terminal world-space feedback were exercised.
This is user-confirmed functional behavior, not merely a source-code build.

**Remaining release checks:**
- [ ] 2-player listen-server PIE: either player can accept, recover
      and turn in; the shared contract state/balance is identical in
      both windows.
- [ ] Confirm payment is awarded **once** even when both players
      interact during and after the payout.
- [ ] Confirm unrelated Red/Blue war/capture logic stays independent
      and player-faction neutrality still works.
- [ ] Confirm 2 independent PlayerStarts inside HQ and no spawn overlap.
- [ ] Save/commit/push final modified `Lvl_ContestedFrontier` map,
      relevant `__ExternalActors__`, `IA_Interact`, `IMC_Default`
      and player Blueprint before requesting PR merge.

**Development review:** Next pass should prioritize multiplayer and
feedback on interaction clarity. Do not assume the temporary shared
contract ledger, static world labels, or lack of save persistence
fulfills the final finance/contract design.

## Multiplayer recovery correction — October 27, 2026

**Observed:** The user confirmed 2-player PIE initially allowed Player 2
at the relay to interact with the intel, while Player 1 remained at HQ
and collected payment. All players shared the `IntelRecovered` state,
so the game incorrectly equated mission progress with physical delivery.

**Implemented in code, pending build/PIE test:**
- Recovery now attaches the existing replicated cache actor to the
  retrieving player's character; `Carrier` replicates.
- Only that carrier can submit the same physical cache at HQ and
  receive the one-off company payment. The non-carrier's attempted
  turn-in is rejected on the server.
- On incapacitation or Pawn/Controller destruction, the cache detaches,
  drops to nearby world-static ground, re-enables world interaction,
  and can be picked up by the remaining teammate.
- No Tick, new input mappings, or new level objects required. Existing
  `IA_Interact`, `HQ_ContractTerminal`, `Relay_IntelCache` and
  reciprocal level references are reused.
- Prototype limitation: one package, no explicit intentional handoff,
  no saved inventory, and no persistence across PIE restarts.

**Acceptance case:** Player 1 takes the contract and stays at HQ.
Player 2 retrieves the intel. Player 1 **must not** be paid when
pressing F; Player 2 returns and **must** be paid once. Repeat with
Player 2 incapacitated in the field: a new carrier must pick up the
dropped object and perform delivery. Finish with ordinary faction
warfare and 2-player replication regression.

## Field validation — physical delivery (October 27, 2026)

After correcting a compile error caused by a forward-declared character
type in an inline header function, the user confirmed the updated
physical-intel transport behavior **works as expected** in Unreal.

The tested scenario followed the previous two-player setup in which
Player 1 could remain at HQ while Player 2 retrieved intel. The
physical-carrier fix prevents awarding payment to someone who is not
carrying the package, while preserving the shared company contract.

**Additional user test — October 27: PASSED**:
- Player 2 recovered the intel, became incapacitated, and dropped it.
  Player 1 successfully retrieved the same physical package and
  delivered it to HQ for the reward. The contract stayed usable across
  the carrier change.

**Still explicitly to validate before finalizing the milestone:**
- Disconnect/destruction behavior, including the package remaining
  recoverable without falling through terrain.
- One-off reward against repeated simultaneous inputs from both players.
- Final committed `Lvl_ContestedFrontier` actor/Blueprint changes
  and regression of the existing combat and territory systems.

Avoid treating these untested edge cases as already validated.
