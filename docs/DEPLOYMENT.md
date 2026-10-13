# Deployment Notes

## Current runtime
The current primary deliverable is a terminal application. The server and
client headers describe an intended transport-agnostic interface; do not treat
those declarations alone as a production multiplayer service.

## Local run
Build using `build.ps1` on the documented Windows toolchain or CMake on a
supported C++17 platform. Start the executable from the repository root so
relative config and content paths resolve predictably.

## Production checklist
- Use a dedicated OS account and least-privilege file permissions.
- Keep database credentials outside version control.
- Restrict network access to the required ports.
- Configure log rotation, backups and disk limits.
- Define a save backup/restore test and recovery-point objective.
- Monitor process health, tick duration, session count and database failures.
- Pin dependency versions and review license obligations before distribution.
- Never expose a raw debug/admin protocol to the public internet.
