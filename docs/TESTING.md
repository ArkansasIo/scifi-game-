# Testing Guide

## Configure and build
```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

For Windows, use a Visual Studio developer terminal and a generator supported
by the installed CMake toolchain. The existing PowerShell build remains
available as a compatibility path.

## Test layers
- Unit: protocol parsing, stat formulas, effects, item valuation and scaling.
- Simulation: seeded universe generation, turns, colony production and combat.
- Persistence: round-trip saves, schema migrations, CSV escaping and failed writes.
- Integration: client/server lifecycle, authentication, invalid requests and
  disconnected clients.
- Regression: preserve a small set of deterministic scenarios and expected
  state summaries.

## Quality gates
- Compiler warnings enabled; warnings are reviewed, not blindly suppressed.
- Tests use fixed seeds where random generation is involved.
- Test malformed inputs and boundary values, not only happy paths.
- Sanitizer builds should be run where supported.
- Do not describe a build as verified until the actual command and result are
  recorded.
