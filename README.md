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
| `lib/fx/App` | Scene model, panes, timeline, panel column |
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
ends. Clicking or dragging on the cache bar scrubs.

The Panels menu opens the panels in the right hand column: the emitter, force
and display parameters, and a diagnostics panel graphing feather-tk's frame time
and object counts alongside this application's particle count, solve time and
cache size.

The View menu arranges one, two, three or four panes. Each pane's own menu picks
what it shows -- a viewport, or a stand-in for the curve editor, the particle
spreadsheet, the expression editor or the compositor -- and a viewport pane gets
a second menu for what it looks through: perspective, front, side or top, the
axis views orthographic. Dragging orbits a perspective view; the middle button, or
alt and the left button, pans any of them; the wheel zooms. Backspace reframes
the viewport with the highlighted border, which is whichever one was last
clicked in.

## Screenshots

`etc/Screenshots/screenshots.json` describes the shots and the application
captures them itself, one process per shot:

```bash
python3 etc/Screenshots/build_screenshots.py etc/Screenshots/screenshots.json --ftk-fx ../build-Debug/bin/ftk-fx/ftk-fx --out /tmp/shots
```

Each shot writes a PNG and a JSON sidecar holding the bounding box and the
visible text of every tagged widget, so what was on screen can be checked
without anyone looking at the picture. See `etc/Screenshots/notes.txt`.

## Tests

```bash
./build-Debug/tests/fx-test/fx-test
```

A name argument runs the tests whose names contain it, e.g. `fx-test System`.
The checks use `FTK_CHECK`, so they are compiled into a Release build as well as
a Debug one.
