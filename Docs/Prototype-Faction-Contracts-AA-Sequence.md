# Colonial Mercenaries — Faction commissions, AA denial and strike sequence

**Design planning note — October 27, 2026. No gameplay code in this change.**
**Status:** Proposed playtest sequence to refine with the designer; not a committed narrative/scripted mission design.

## Why this sequence

The first Relay Intelligence Recovery job now demonstrates server-owned
contract progress, physically carried/dropped cargo, successful delivery
and one company payout in two-player PIE. It is currently assigned to
an **independent frontier broker** with no faction consequences.
Red and Blue fight each other while the mercenary players begin neutral
(unless they directly provoke soldiers). This is a useful technical
test, but its mission stakes are not yet representative of the intended
world and economy.

GDD references:
- §4: choose contracts, prepare, deploy, influence a persistent war, return for payment.
- §9: tactical autonomy and territorial control within strategic constraints.
- §10: contracts have employers, requirements, consequences and world-state dependencies.
- §11 and §19: company may be neutral/independent, work for different factions or formally align; effects consider evidence and attribution.
- §15: air defenses and route risk restrict support; explain failure reasons.
- §20: persistent company/faction state, shared multiplayer authority.
- §22–23: one contested region with a strategic AA site, procurement,
  support availability, and a destroy-AA -> support availability test
  are explicit Prototype 0.1 goals.

## Proposed mission chain (first pass)

### Commission 1 — Contested Relay Intelligence (adapt existing job)

- The existing cache remains in the contested relay compound, accessible
  while the Red and Blue factions are engaged with one another.
- Begin with **both factions neutral toward the independent player company**.
  Combat remains dangerous because of battlefield crossfire; either faction
  still retaliates when attacked.
- Offer mutually exclusive versions of this job from **Red and/or Blue**.
  Player/company chooses *one* employer; the same cache must not be sold
  to two conflicting employers through duplicated turn-ins.
- A physical carrier must deliver to the commissioned destination.
  Contract completion pays once and updates **company-level faction
  relationships**. The other side may become hostile or untrusting.
- **Preferred initial demonstration:** expose consequences on confirmed
  delivery rather than silently making distant troops hostile at the moment
  of a private acceptance. Certain *overt* contracts may instead publish
  the employer relationship at acceptance. Being witnessed attacking or
  stealing can cause an immediate reaction independently of delivery.
- Display employer, expected payout and possible diplomatic consequences
  **before acceptance**, with confirmation after completion.
- Neutral contracts should remain available in the wider design: accepting
  a job does not automatically force permanent faction allegiance.

### Commission 2 — Air support denial (teach the AA constraint)

- Once the player has earned money and gained access to a support supplier,
  offer a test strike against an enemy-controlled installation or outpost.
- AA installation is **active** and covers the target. The authoritative
  support-availability query rejects the request with an informative reason:
  e.g. `STRIKE DENIED — ENEMY AA COVERAGE`.
- No charge if a request is rejected before dispatch. The UI should
  communicate whether any order has cost, risk, coverage, cooldown or
  provider restrictions.
- The denial motivates a follow-up sabotage contract; it is not simply a
  scripted cutscene or an invisible quest flag.
- If the AA has already been disabled through sandbox play, the strike
  must be allowed if other restrictions pass; don't force impossible
  blocking/denial stages after the fact.

### Commission 3 — Disable the AA battery

- Add a functional AA installation at the north strategic site planned
  for `Lvl_ContestedFrontier` (map plan: about `(0, +9500)`).
  Its owner/defenders should reflect the employer/opponent chosen earlier.
- The AA has a damageable/sabotageable functional component and
  explicit **Active / Disabled** replicated world state.
- Infantry infiltration, sabotage/planting charges, or destructible
  critical equipment are candidate methods. Avoid requiring hundreds of
  ordinary rifle rounds to grind down a heavy military installation.
- Objective and world state should agree. Physical destruction or
  disabling of the actual site immediately changes support eligibility
  for affected areas and remains so after save/load.
- The attack must provoke the defending faction and have company
  relationship consequences even without completing the job.

### Commission 4 — Call in the strike

- With AA coverage removed, purchase/request the same support service.
- Show provider, target validity, availability, cost and cooldown.
- Server validates credits, relationship/access, AA suppression, target
  restrictions and support resources before dispatch.
- Strike affects the **actual target base**, its occupants, damageable
  infrastructure and/or available garrison strength, not just a HUD label.
- Apply relevant friendly fire, reputation and strategic consequences.
- The new world state influences continuing autonomous faction warfare.

## Critical system boundaries

1. **Company reputation/relationships, not a global forced player faction.**
   Keep employer affiliation separate from character faction tags and
   immediate personal aggression. Faction-to-faction hostility continues.
   Persistent standings should ultimately live in authoritative company
   state / a relationship service, not the contract terminal.
2. **Contracts express data-driven world conditions and consequences.**
   Employer faction, rival faction, objective, reward, public/covert
   nature and on-accept / on-complete effects should be explicit.
3. **Existing physical cargo stays intact.** Contract status is shared;
   individual possession is not. Do not reintroduce payment by a
   non-carrier.
4. **Support restrictions query world state.** AA ownership, operational
   status, range and provider capabilities determine availability.
   A quest should observe this system, never override it.
5. **Avoid fragile mission dependencies.** Autonomous factions might
   weaken/capture/destroy a site first; contracts should adapt, cancel
   cleanly or present alternate objectives without impossible goals.
6. **Listen-server authority and later persistence:** only the host
   changes credits, reputation, contract stages, AA condition, strikes
   and base damage; replicate observable state to all players.
7. **One coherent region.** Reuse Red/Blue FOBs, relay, company HQ and
   north AA terrain; no need to build a second map first.

## Placement within the revised prototype roadmap

- **Now:** complete current map/contract/HUD checkpoint. Prioritize
  player weapon/movement feel, then soldier movement/cover.
- **After core infantry feel:** implement a *thin faction consequence
  slice* (one employer choice, a shared company relationship variable,
  opposing-faction reaction). Refactor the first contract to use it.
  Test both Red and Blue paths in separate PIE runs.
- **After or alongside basic item/equipment/procurement:** company money
  moves out of the temporary terminal; purchasing becomes meaningful.
- **Prototype 0.1 completion:** functional AA state, one support order
  type and provider, honest denial reason, a physical sabotage path,
  supported strike after AA goes down, persistence and multiplayer checks.
- **Vertical slice:** expand mission chains, variant employers,
  dedicated contract-board/PDA UI, aircraft presentation, more dynamic
  reputation/evidence logic, richer effects on bases and the war.

## Acceptance checks for the future sequence

1. Player company neutral to both military factions at start.
2. Can commission the same relay retrieval objective from only one
   selected employer; cannot complete both contracts with one package.
3. Completion changes shared company relationship; reactions are
   intelligible and affect relevant soldiers/privileges.
4. Other faction conflict and independent soldier decisions continue.
5. With AA operational, a paid strike request into its coverage is
   rejected clearly **before charging money**.
6. Disabling AA through actual gameplay changes both replicated state
   and strike availability for both connected players.
7. Once allowed and paid for, strike physically affects a real target
   and the continuing regional war.
8. Reloading a campaign preserves company standings, AA condition,
   finances and relevant damage without duplication exploits.
9. Alternate sequencing (AA already disabled or base captured) does
   not leave a contract impossible to finish.

## Open design choices — do not hardcode prematurely

- Is the initial intel contract actually commissionable by **either**
  Red or Blue, or only by one with a competing counteroffer?
- How overt is the job? Does merely accepting it alert the opposition,
  or is betrayal attributed only when witnessed/confirmed/delivered?
- How steep is the first relationship penalty, and does the other
  faction become immediately hostile on confirmed delivery or just
  increasingly suspicious?
- Who controls the north AA battery at mission start? Does it change
  hands dynamically, or is it a campaign-locked defensive installation?
- Is strike denial free at ordering time, or do some unsafe dispatched
  flights incur cancellation costs later? Start with free preflight
  denial for the prototype.

These are design questions, not implementation blockers for combat
improvements.
