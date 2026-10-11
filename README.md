# Colonial Mercenaries

Working title for a single-player and 1–4 player cooperative open-world sci-fi mercenary game built in **Unreal Engine 5.8**.

## Development status
**Prototype foundation.** Initial goals are a playable third-person soldier, two autonomously hostile AI factions, and a small persistent contested battlefield.

## Project structure
The Unreal project will be committed from the existing local C++ Third Person template. The repository tracks source code, configuration, the `.uproject`, Unreal assets, and design documents. Generated build products and caches are excluded through `.gitignore`.

## Development workflow
Use the main branch as the stable baseline. Work on short-lived feature branches and merge tested changes via pull requests.

## Design principles
- Server-authoritative game systems, with solo and cooperative play.
- C++ for core simulation and networking; Blueprints for content assembly and presentation.
- Data-driven equipment and support configuration; avoid hard-coded balancing values.
- Incremental implementation, with a playable acceptance test for each milestone.

Project documentation and the full GDD will be added under `Docs/` as the prototype foundation is established.
