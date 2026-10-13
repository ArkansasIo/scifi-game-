# Contributing

## Workflow
1. Create a focused branch from `master`.
2. Keep changes modular and preserve existing game behavior unless a change is
   explicitly intended.
3. Add tests for bug fixes and new formulas.
4. Update the relevant document when behavior or configuration changes.
5. Run the build and tests locally; include exact commands in the PR.
6. Do not commit credentials, generated executables, save files or editor caches.

## C++ conventions
- Target C++17 unless a separately reviewed change updates the baseline.
- Prefer explicit bounds checks and small functions with clear ownership.
- Keep game rules deterministic where practical; pass seeds into random systems.
- Avoid silently changing save/protocol formats; version migrations explicitly.
- Keep optional dependencies disabled by default and document enablement.

## Pull requests
Describe motivation, design, compatibility impact, tests run and known gaps.
A header or stub is not considered a completed feature without implementation
and meaningful tests.
