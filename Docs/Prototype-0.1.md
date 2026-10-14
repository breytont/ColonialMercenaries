# Colonial Mercenaries: Prototype 0.1

## First increment: faction and health foundations

This branch introduces reusable components; it does **not yet change** the
default Third Person gameplay. A subsequent increment will add a test AI
combatant and connect these components to autonomous engagements.

### Components

- `UCMFactionComponent`: replicated faction ID using a Gameplay Tag. The
  first prototype temporarily regards two *different valid* faction tags
  as hostile; a future relationship system will model alliances and neutrality.
- `UCMHealthComponent`: health managed on the authoritative server, with
  replicated health and Blueprint-accessible health/death events. Uses Unreal's
  standard `ApplyDamage`/ `OnTakeAnyDamage` pipeline. Health defaults are
  editable per Blueprint.

**Component replication requires an actor with replication enabled.** The
upcoming AI test actor will enable actor replication by default.

### Local acceptance test

1. Pull this branch after it is available in GitHub Desktop.
2. With Unreal Editor closed, build **ColonialMercenariesEditor** (Development
   Editor, Win64) in Visual Studio, or open the `.uproject` and allow Unreal
   to build its missing C++ modules.
3. Open the project and confirm that the existing Third Person level plays
   with no behavior changes.
4. In **Tools > New C++ Class**, or the Content Browser C++ Classes view,
   confirm the `CMFactionComponent` and `CMHealthComponent` classes are
   visible; no Blueprint setup is required yet.
5. Report build errors if any, including the *first* compiler error rather
   than just the final 'build failed' summary.

### Deferred until components compile

- AI combatant with patrol/target acquisition and attacks
- AI factions fighting independently of the player
- Player firearm and faction accountability
- Persistent territorial objective and war simulation
- Contracts, economy, salvage, and support

Core gameplay remains unchanged deliberately until this increment is verified.
