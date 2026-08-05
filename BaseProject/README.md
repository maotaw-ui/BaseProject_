# BaseProject

A clean Visual Studio 2022 x64 base.

## Files users edit

### `User/Draw.hpp`
Contains only reusable drawing data and drawing functions.

### `User/Features.cpp`
Contains the single cached-entity loop and every feature call.
Add box, health bar, snapline, skeleton, labels, or future per-entity logic in this one file.

## Runtime flow

- Entity addresses are rebuilt by the cache every few seconds.
- Live values such as position, health, armor, team, and the view matrix are updated every frame.
- `Features::Run()` loops over the current cached list once per frame.

Press Delete to close the program.

Offsets are version-dependent and may need updating.
