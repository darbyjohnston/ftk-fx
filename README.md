# ftk-fx

An interactive particle FX application for film and television visual effects,
built on [feather-tk](https://github.com/grizzlypeak3d/feather-tk). See
[particle-fx-design.md](particle-fx-design.md) for the design, and
[JOURNAL.md](JOURNAL.md) for why the code looks the way it does.

Where it is now: a vertical slice. A point emitter fills a pool, gravity and
drag move it, particles die at their lifespan, the frames are cached, and the
whole thing scrubs and plays back. Everything the artist can reach is already a
`Parameter`, so it is already animatable.

## Layout

| Directory | Contents |
|---|---|
| `lib/fx/Core` | Pool, frame, cache, curve, parameter, keyed randomness |
| `lib/fx/Sim` | Emitters, forces, rules, the solver |
| `lib/fx/App` | Scene model, viewport, timeline, panels |
| `lib/fx/CoreTest` | Tests for the above |
| `bin/ftk-fx` | The application |
| `tests/fx-test` | The test runner |
| `deps/tlRender` | Submodule; brings feather-tk in at `deps/tlRender/deps/ftk` |
| `etc/Config` | What to build, one file per configuration |

## Building

tlRender is a submodule and brings feather-tk with it, so the whole stack
builds from source and a debugger can step all the way down it.

The checkout lives inside a working directory that also holds the build, so
that nothing built ends up in the source tree:

```
ftk-fx/            the working directory
    ftk-fx/        this checkout
    ftk-Debug/     feather-tk's dependencies
    tl-Debug/      tlRender's dependencies
    build-Debug/   this
    install-Debug/ everything installed
```

The superbuild builds the dependencies, then feather-tk and tlRender, then
this. Run it from the working directory, naming the checkout:

```bash
cd ~/Dev/ftk-fx && sh ftk-fx/sbuild-macos.sh ftk-fx Debug
```

`sbuild-linux.sh` and `sbuild-win.bat` take the same arguments. The first run
takes a while; after that only `build-<type>` is rebuilt:

```bash
cmake --build build-Debug
```

What gets built is in `etc/Config/default.cmake`, not in the scripts. Personal
settings go in `etc/Config/local.cmake`, which is not tracked and wins over
everything else.

Movie support is off in the default configuration — FFmpeg and the AV1 encoders
are most of the superbuild's time, and this application renders passes rather
than playing them back. Turning `TLRENDER_FFMPEG` on is a one-line change the
day a movie plate has to load behind the sim.

## Running

```bash
./build-Debug/bin/ftk-fx/ftk-fx
```

Space plays and stops, the arrow keys step a frame, Home and End jump to the
ends, and Backspace reframes the view. Dragging in the viewport orbits and the
scroll wheel zooms. Clicking or dragging on the cache bar scrubs.

`-frame N` starts on a frame, which is what makes `-screenshot` worth taking:

```bash
./build-Debug/bin/ftk-fx/ftk-fx -frame 70 -screenshot fx.png
```

## Tests

```bash
./build-Debug/tests/fx-test/fx-test
```

A name argument runs the tests whose names contain it, e.g. `fx-test System`.
The checks use `FTK_CHECK`, so they are compiled into a Release build as well as
a Debug one.
