# Security and Threat Model

## Trust boundaries
- Terminal input and save files are untrusted.
- If a network transport is added, every message is untrusted regardless of
  client version or authentication state.
- Database rows and Lua content can be malformed or stale.
- The game simulation is authoritative; clients must not decide combat outcomes,
  resource balances, movement validity, or loot awards.

## Required controls
1. Bound every line, token, collection and persisted field before copying.
2. Reject unknown protocol verbs, excess arguments and invalid numeric ranges.
3. Require authentication for character/world mutations.
4. Apply rate limits to login, combat actions and expensive world queries.
5. Store password hashes using a modern password hashing function in a vetted
   library; never store plaintext passwords or reversible password encryption.
6. Use parameterized SQL and least-privilege database credentials.
7. Treat Lua as trusted game content unless a sandbox with resource limits is
   deliberately implemented.
8. Use deterministic server-side validation for movement, inventory and combat.
9. Avoid logging credentials, session tokens or private player data.
10. Keep secrets in environment variables or a secret manager, not config files.

## Abuse cases to test
- Oversized and unterminated messages
- Repeated login attempts and duplicate names
- Negative, overflowing and non-numeric IDs
- Replay of stale commands
- Forged loot, travel or resource requests
- Save corruption and partial database failure
- Extremely expensive scripts or procedural-generation inputs

This document is an engineering checklist, not a claim that all controls are
already implemented.
