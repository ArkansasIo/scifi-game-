# Implementation Roadmap

This roadmap distinguishes working code from planned work. A feature is not
considered complete merely because a header, design document, or stub exists.

## Baseline
- [x] C++17 terminal game and modular source tree
- [x] Data-driven combat, loot, progression and turn scheduling
- [x] Procedural universe and biome generation
- [x] Gameplay layer: exploration, trading, colonies, skills and contracts
- [x] Design documents and Lua content examples
- [ ] Reproducible CMake build verified on Linux, Windows and macOS
- [ ] CI build and protocol tests

## Runtime and multiplayer
- [ ] Complete protocol parser and strict request validation
- [ ] Implement authoritative session dispatcher
- [ ] Add transport abstraction with bounded message sizes and timeouts
- [ ] Add integration tests for login, disconnect, malformed requests and capacity
- [ ] Add rate limiting, audit logging and secure credential storage
- [ ] Persist server state transactionally

## Strategy simulation
- [ ] Planet building queues, costs, upkeep and production
- [ ] Fleet orders, travel, fuel, supply and interception
- [ ] Research prerequisites and progression
- [ ] Diplomacy, treaties, war states and faction AI
- [ ] Combat resolution and after-action reports
- [ ] Save/load migration and deterministic simulation seeds

## Release quality
- [ ] Unit and property-based tests for formulas
- [ ] Sanitizer jobs and static analysis
- [ ] Configuration validation and sample deployment
- [ ] Versioned protocol and save formats
- [ ] License and third-party dependency audit

## Definition of done
Each system needs an implementation, tests for success and failure paths,
documentation, and a build that passes on CI. External services and optional
dependencies must be explicitly documented; no credentials belong in Git.
