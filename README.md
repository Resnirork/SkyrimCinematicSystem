# Skyrim Cinematic System SE/AE

New approach to the idea of original [Skyrim Legendary Edition CameraScripter](https://www.nexusmods.com/skyrim/mods/79041) to Skyrim SE/AE using the official [libxse CommonLibSSE-NG template](https://github.com/libxse/commonlibsse-ng-template) structure.
It's *no port*, but completely created from scratch, with backwards compatibility towards old `.scs` files in mind.

## Important

The repository deliberately keeps `lib/commonlibsse-ng` as the CommonLibSSE-NG git submodule rather than copying a vendored snapshot into this archive. Clone/update with:

    git clone --recurse-submodules <repository>
    git submodule update --init --recursive

The upstream template currently requires XMake 3.0+ and a C++23 compiler, and builds into `build/windows/`. See the upstream template for environment variables such as `XSE_TES5_GAME_PATH` and `XSE_TES5_MODS_PATH`.

### Disclaimer

This code is completely AI coded. Errors may occur.

## Current implementation

* Real CommonLibSSE-NG template layout and submodule declaration.
* Complete `.scs` file discovery and parser for `mov`, `spl`, `rot`, `fov`, `tim`, and `rst`.
* Relative `[]` and absolute `{}` values.
* Full Papyrus native surface from the original `CameraScripter.psc`.
* Papyrus event registration via CommonLibSSE-NG `SKSE::RegistrationSet`.
* FreeCameraState activation/restoration.
* Camera position/rotation/FOV/time-scale playback logic.
* `spl` Catmull-Rom spline movement with 2–255 points; `spl` shares the movement channel with `mov` and remains concurrent with `rot`, `fov`, and `tim`.
* Recording API with `recording.scs` output.

## Camera update hook

Playback hooks the active `FreeCameraState` instance by replacing slot 3 of that instance's primary vtable. The hook calls the original `FreeCameraState::Update()` first, then invokes the runtime to apply the script on the same state-update cycle. Runtime processing verifies that the updated camera is Skyrim's player camera and that playback is active. The hook is installed when playback starts and removed when playback ends or is cancelled; it does not use a `PlayerCamera::Update()` call-site trampoline or relocation address.

The active `FreeCameraState` is also temporarily removed from `PlayerControls` handlers and player input is blocked during playback. The hook path and camera-write behavior still require in-game verification on each supported runtime. The rotation mapping is treated as pitch=`rotation.x`, yaw/Z=`rotation.y`, with radians internally and degrees in `.scs`/Papyrus.

## `.scs` script syntax

Scripts are plain-text, comma-separated files. Each non-empty line contains one action; command names are lowercase and case-sensitive. Whitespace around fields is ignored. A `#` starts a comment that continues to the end of the line, including when it follows an action. Blank lines and comment-only lines are ignored.

Time values are seconds from the start of playback. Numeric values must be finite; action start times and durations must be zero or greater. Use a decimal point for fractional values.

### Actions

```text
mov,<start>,<duration>,[x,y,z]   # relative movement
mov,<start>,<duration>,{x,y,z}   # absolute camera position
spl,<start>,<duration>,<point1>,<point2>,...  # spline movement
rot,<start>,<duration>,[pitch,yawZ]  # relative rotation
rot,<start>,<duration>,{pitch,yawZ}  # absolute rotation
fov,<start>,<duration>,[degrees]  # relative FOV change
fov,<start>,<duration>,{degrees}  # absolute FOV
tim,<start>,<duration>,[scale]   # relative timescale change
tim,<start>,<duration>,{scale}   # absolute timescale
rst,<start>                      # reset camera position, rotation, and FOV
```

`mov` requires three coordinates in game units. `rot` uses degrees: pitch is the first value and yaw/Z is the second. The parser also accepts a three-value `rot` vector for compatibility; its middle value is ignored and the third value supplies yaw/Z. Prefer the two-value form shown above. `fov` uses degrees, and `tim` uses Skyrim timescale units.

Square brackets (`[]`) make a value relative to the camera or setting baseline captured when that action starts. Curly braces (`{}`) specify an absolute target. A zero duration applies the target immediately. Actions of the same type replace an overlapping action of that type; different types can run concurrently.

`spl` has the same `<start>,<duration>` prefix as `mov`, followed by 2–255 points. Each point has three coordinates in `[x,y,z]` or `{x,y,z}` form, and all points in one spline must use the same form. The spline begins at the camera position captured when it starts, then follows the supplied points using Catmull-Rom interpolation. Relative points are offsets from that captured position and are not accumulated frame by frame. A spline uses the movement channel, so it can replace an overlapping `mov` or another `spl` while running alongside rotation, FOV, and timescale actions.

`rst` resets the camera position, rotation, and FOV to the values saved when playback began. It does not reset the timescale. Its start time is required and must be non-negative.

### Example

```text
# Move 100 units on X over 5.2 seconds, starting at 0.3 seconds.
mov,0.3,5.2,[100,0,0]

# Set timescale to 40 over 5 seconds, starting at 1 second.
tim,1,5,{40}

# Turn to pitch -30 degrees and yaw/Z -30 degrees.
rot,11,3,[-30,-30]

# Move along an absolute spline with four points.
spl,28,10,{0,0,0},{100,0,50},{200,100,100},{300,100,0}

# Restore the original camera position, rotation, and FOV at 30 seconds.
rst,30
```