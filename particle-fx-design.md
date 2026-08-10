# ftk-fx — Design Document

*Project name is a placeholder. An interactive particle FX application for film and television visual effects, built on [feather-tk](https://github.com/grizzlypeak3d/feather-tk).*

---

## 1. Premise

A focused, standalone particle animation tool in the spirit of the single-purpose particle systems of the early nineties: one application an artist can learn in a week, live in all day, and use to hand-craft effects shots. Not a general-purpose 3D package, not a physically-accurate solver, not a node-graph environment.

The target work is bread-and-butter episodic and feature effects — debris, sparks, embers, dust, smoke wisps, swarms, impacts, atmospherics — where the sim is judged by how it reads on camera, not by how well it matches physics.

### Goals

- Interactive authoring loop: change something, see it immediately, without a bake step.
- Manual, art-directable control as the default path; procedural and algorithmic control as an available path.
- Scripting that is continuous from per-particle expressions up to custom tool UIs, in one language.
- First-class integration into an existing pipeline via imported geometry/cameras and exported caches and render passes.
- Runs on Linux, macOS, and Windows.

### Non-goals

- Fluid, cloth, rigid-body, or destruction solving. (Import the results of those; don't compute them.)
- Modeling, rigging, character animation, or UV/texture authoring.
- Final-quality beauty rendering. Output is comp-ready passes, not a finished frame.
- Node graphs as the primary authoring metaphor.
- A general-purpose plugin SDK in v1.

---

## 2. Design principles

1. **Manual beats automatic.** Every automatic behavior must have a manual override. If the artist can't reach in and change one particle by hand, the feature is incomplete.
2. **Nothing Python touches runs per-particle.** Python owns anything evaluated once per frame or once per edit. Per-particle math runs array-at-a-time (NumPy over the C++ pools) or in C++.
3. **A frame is a pure function of the frame before it.** Expressions read the previous frame's state and write the current one. This is what makes scrubbing, caching, and re-simulation predictable, and it cannot be retrofitted.
4. **Determinism by default.** Same scene, same seed, same result — on any machine, in the GUI or headless.
5. **The viewport is the product.** Interactivity is the feature. Anything that stalls the viewport is a bug, not a performance characteristic.
6. **Comp-first.** The tool's output is intermediate. Every rendering decision assumes the image goes into Nuke.

---

## 2a. Project constraints

**This is a part-time project worked on in small bursts, by a small number of people.** This is not a footnote — it is a design constraint with the same weight as the technical ones, and it should override architectural elegance whenever the two conflict.

### Scope reality check

A standalone particle tool of the early nineties was likely 100–150k lines of C, of which perhaps half was GUI code and language implementation. Most of that is now free: feather-tk provides the widget toolkit, Python provides the scripting language, NumPy provides vectorized math, and OpenEXR/Alembic/OCIO/tlRender provide I/O. The application-specific code here — pools, solver, emitters, fields, events, cache, curves, render types, compositor — is plausibly **25–40k lines**. That is achievable at a few thousand lines a quarter, and the phase boundaries mean it is useful long before it is finished.

### What "small bursts" implies

The binding constraint is not total hours; it is how much context has to be rebuilt after weeks away. That shapes the code:

- **Every phase ends at a usable state.** No refactor that leaves the app broken across a gap in work. If a change can't be finished in one sitting, it needs an intermediate state that runs.
- **Boring over clever.** Code that can be re-read cold beats code that is elegant. Nothing that requires re-deriving an insight to modify.
- **Explicit over generic.** Write the specific thing three times before abstracting. Premature generalization is the most expensive mistake available here, because the abstraction has to be re-understood every time.
- **Vertical slices over horizontal layers.** Build one emitter type all the way through to pixels before building six emitters that render nothing.
- **Conventions carry the memory.** Consistent file layout, naming, and structure so any subsystem is navigable without recall. A short running design journal alongside the code, recording *why* — the reasoning is what evaporates first.
- **Tests as documentation.** Enough coverage on the core (pool, cache, curve evaluation, determinism) that a change after a long gap fails loudly rather than subtly.

### Permission to be dated

Running 1990s design on 2020s hardware is an acceptable and often correct starting point. Explicitly allowed:

- Fixed limits (maximum systems, maximum attributes) rather than dynamic everything
- Modal dialogs rather than non-blocking panels
- Synchronous operations with a progress bar rather than async and cancellation everywhere
- Single-threaded solve
- A fixed UI layout rather than fully dockable panels
- No plugin SDK, no node graph, no multi-user, no GPU compute

Each of these is a decision that doesn't have to be made, a subsystem that doesn't have to be built, and a source of bugs that doesn't exist. Revisit them only when a real shot demands it.

---

## 3. Architecture

Four layers, strictly ordered — each may depend on those above it, never below.

| Layer | Language | Responsibility |
|---|---|---|
| **Core** | C++ | Particle pools, attributes, time/cache, evaluation |
| **Sim** | C++ | Emitters, fields, rules, collisions, events, solver |
| **App** | C++ (feather-tk) | Viewport, widgets, actions, undo, settings, render |
| **Script** | Python | Scene assembly, expressions, tool UIs, I/O orchestration |

Python and C++ share one process and one address space via pybind11. Particle attribute arrays are exposed to Python as zero-copy NumPy views over the C++ storage.

**Interpreter ownership:** C++ owns `main()` and embeds the interpreter (`py::scoped_interpreter`). The application controls the event loop and frame clock, and remains runnable if a user script throws. A Python-hosted mode (`import ftkfx`) should also work for prototyping and batch scripts.

**Threading:** start simple. The solve is single-threaded per frame, with data laid out for SIMD and vectorized evaluation. Parallelism comes later and comes first *across* systems, which is safe because systems own their pools and interact only through explicit events. Splitting a single pool across threads is deferred indefinitely — it complicates collision ordering, event ordering, and determinism, and the vectorized path should be fast enough for a long time. I/O, cache writes, and image loading are threaded from the start, since they don't touch the solve.

---

## 4. Core data model

### Particle pools

Structure-of-arrays. Each attribute is a contiguous typed array; particles are indices, not objects.

- Attribute types: `float`, `int`, `bool`, `vec2`, `vec3`, `vec4`, `quat`, `mat4`, `string` (sparse/rare)
- Built-in attributes: `id`, `position`, `velocity`, `acceleration`, `age`, `lifespan`, `mass`, `radius`, `color`, `opacity`, `orientation`, `angularVelocity`, `active`
- Custom attributes are declared per-system with a name, type, default, and whether they persist to cache

**Attributes are a declared schema, not free-form.** Scripts and the UI may add or remove attributes at any time the system is not simulating; while a sim is running the schema is frozen. This keeps every cached frame the same shape, so the cache format, the export schema, and re-simulation from a mid-range frame all stay simple. Storage allocates lazily on first write — declaring an attribute you never use costs nothing.

**IDs are stable and never reused.** They are the handle for manual intervention, for matching particles across a re-sim, and for ID passes in comp.

### Birth and death

Particles are added at the end of the pool; dead particles are tombstoned and compacted at a controlled point in the frame, never mid-expression. Any NumPy view handed to Python is invalidated by a generation counter on the pool, so a resize can't leave Python holding freed memory.

### Systems

A **particle system** is one pool plus the emitters, fields, rules, and events bound to it. A scene contains many. **Each system owns its own pool** — sparks and smoke are separate arrays, not one array with a group tag.

This costs some performance and some vectorization opportunity, and buys: a mental model that matches how artists think, a direct mapping to render layers, free solo/isolate, per-system caching and export, and disabling a system becoming a no-op rather than a compaction pass.

Systems may reference each other — system B emits from system A's collision events — forming a small dependency DAG evaluated in topological order. All cross-system interaction is explicit, routed through the event system rather than happening implicitly inside a shared pool. Cycles are rejected at scene-load time.

---

## 4a. Parameters, curves, and animation

The curve editor is the primary expression of the "manual beats automatic" principle. Most art direction happens by dragging a key, not by writing an expression. Its foundations must be in place from the first phase, because retrofitting animation into a parameter system means touching every subsystem.

### The Parameter type

Every scalar and vector value in the application — emitter rate, field strength, collision bounce, sprite scale, layer opacity — is a uniform `Parameter`, not a plain float. A parameter holds one of:

- a **constant**
- an **animation curve** (keyframed, time-domain)
- a **profile curve** (a ramp, indexed by an attribute)
- an **expression**
- a **connection** to another parameter

Anything reachable in the UI is therefore animatable, drivable, and scriptable by construction. "Can I animate this?" should never be a question anyone has to ask.

### Animation curves

Time-domain, keyframed, evaluated once per frame — so evaluation cost is irrelevant and full Bézier is fine.

- Interpolation per key: step, linear, Bézier with tangent handles, auto/smooth
- Tangent modes: broken, unified, flat, locked
- Pre- and post-infinity behavior: constant, linear, cycle, cycle-with-offset, oscillate. Cycling matters — a looping wind gust authored over 20 frames should cover a 200-frame shot.
- Keys snap to whole frames by default, with sub-frame allowed

### Profile curves (ramps)

Normalized-domain, indexed by an attribute rather than by time: opacity over normalized age, field strength over distance from centre, sprite frame over speed, emission weight over surface curvature.

**These are evaluated per particle, so they are never evaluated as Béziers at runtime.** A profile curve is baked to a sampled lookup table (256–1024 samples) whenever it is edited; the solver does a table lookup with linear interpolation. This keeps ramps effectively free and lets them be used liberally.

**Per-particle variance.** A ramp may carry a randomness band — a second curve defining the spread around the first — so each particle gets its own slightly different profile, seeded from its ID. This is what stops a thousand particles from fading out in perfect unison, and it is one of the highest-value-per-line-of-code features in the whole tool.

### Driven curves

A curve's input may be time, a particle attribute, or **another parameter's value** — the set-driven-key idea. Wind strength driven by the hero geometry's speed, emission rate driven by distance to the collision surface. This is where a lot of art direction lives without anyone writing code.

### Curve editor UI

- Multiple curves visible and editable simultaneously, with a channel list and solo/isolate
- Box, lasso, and marquee key selection; move, scale, and retime selected keys as a group
- Normalized and absolute value views; frame-range and value-range framing
- Copy, paste, mirror, and reverse curves between parameters
- Import curves from Alembic/USD animation, so an imported camera or prop can drive a parameter directly
- Full undo/redo integration via feather-tk's command stack

### Diagnostic plotting

Once the widget exists, point it at the simulation's own data: particle count over time, a selected particle's speed or age or custom attribute over its life, collisions per frame, cache memory over the range. Near-free to build and disproportionately useful when a sim misbehaves.

---

## 5. Time and caching

The spine of the application. Decide this first; everything else is downstream.

- **Frame range, sim start frame, and playback range are separate.** A sim may need to start 30 frames before the shot to settle.
- **Substeps** are per-system, an integer division of the frame. Needed for fast-moving particles and reliable collisions.
- **Cache states per frame:** `empty` → `simulated` → `locked`. Locked frames never re-simulate, even if upstream parameters change. This is how an artist freezes an approved section of a shot.
- **Invalidation is forward-only.** Editing a parameter at frame 40 invalidates 40 onward; frames 1–39 stay valid.
- **Scrubbing backward into cached frames is instant.** Scrubbing into un-cached frames either re-simulates from the last valid frame (with progress and a cancel) or shows the last valid frame, per user preference. The UI must always make it obvious which frames are cached, which are locked, and which are stale — a cache bar under the timeline, color-coded.
- **Memory budget** is a user setting. When the budget is hit, evict the frames furthest from the playhead, never locked frames.
- **Disk cache** for long sims: the same format as the export cache, so a cached sim is also a deliverable.

---

## 6. Sources: emitters

All emitters share: rate (per second or per frame), a burst mode, an enable/disable toggle, a random seed, and an initial-state block (velocity, spread, lifespan, plus any custom attributes) that can be a constant, a range, or an expression.

- **Point** — single origin
- **Volumetric primitives** — sphere, box, cone, disc, cylinder; emit from surface or interior
- **Curve** — along an imported or authored curve
- **Geometry** — from imported mesh points, faces (area-weighted), or volume; supports animated/deforming geometry, with velocity inheritance from the surface
- **Texture-driven** — emission masked and weighted by a map on the emitting geometry's UVs. The most direct art-direction lever for "make it come from *here*."
- **Particle** (secondary emission) — one system emits from another's particles, either continuously (trails) or on an event (sparks on impact)

Emitters are transformable and animatable, and can be parented to imported geometry or camera.

---

## 7. Forces, rules, and events

### Force fields

Each field has a strength, a falloff, an optional shaped region of influence, and an optional per-particle attribute weight (so scripting can make some particles heavier than others without a separate system).

Gravity · wind (with turbulence and gustiness) · drag · turbulence (curl noise) · vortex · radial (attract/repel) · uniform directional · axial · newton (inverse-square) · volumetric field from an imported vector grid

### Rules (non-scripted behaviors)

Presented as toggleable behavior blocks with a handful of parameters each — the reason an artist can do 80% of shots without opening the script editor.

- **Lifespan** — constant, random range, or attribute-driven
- **Collide** — against imported geometry, with bounce, friction, stickiness, and thickness
- **Kill** — on age, on collision, on leaving a bounding region, on falling below a speed threshold
- **Trail** — leave history behind each particle
- **Goal** — attract toward a target position, geometry, or another particle system, with weight
- **Speed limit / clamp** — min/max speed, min/max attribute value
- **Orient** — align to velocity, to camera, to a fixed axis, or free tumble
- **Randomize** — jitter an attribute within a range, with per-particle-constant or per-frame variation
- **Ramp over life** — drive any attribute from a profile curve keyed against normalized age (see §4a). Should be available everywhere; it is the single most-used control in this kind of work.

### Events

The bridge between rules and scripting. An event has a trigger and one or more actions.

- Triggers: on birth · on death · on collision · at age · on entering/leaving a region · on attribute threshold · on frame
- Actions: set attribute · emit into another system · kill · apply impulse · run a script callback

---

## 8. Collisions

Deserves its own treatment because it's the most common source of both artifacts and artist frustration.

- Collide against imported static and animated geometry, and against simple analytic primitives (plane, sphere, box) which are much faster and often sufficient.
- Continuous collision detection within substeps — no tunneling at speed.
- Per-collision response: bounce coefficient, friction, stickiness, randomization of the reflected vector (essential for making debris look natural rather than billiard-ball).
- Collisions raise events and record `collisionCount`, `lastCollisionNormal`, and `lastCollisionTime` as per-particle attributes.
- Velocity inheritance from moving collision geometry.

---

## 9. Scripting

One language, Python, at every level. The mental model must be continuous from "make this particle turn red on impact" up to "build me a control panel for this swarm."

### Expressions

Written per-particle, evaluated array-at-a-time over NumPy views. Two hooks per system, the split particle systems have always made:

- **Creation** — runs once when a particle is born
- **Runtime** — runs every substep

Expressions read the previous frame's state and write the current one. They may read scene-level context (`frame`, `time`, `camera`, imported geometry) and any user-defined parameter.

Because the expression surface is a restricted, vectorized subset by construction, the evaluation backend can later move to Numba or a custom JIT without changing a line of user-facing syntax. Design for that; don't build it in v1.

### Editing

Built-in editors first, using feather-tk's text editing widgets — an expression panel per system with syntax highlighting and inline error reporting. Errors must be non-fatal: a broken expression pauses that system and reports, it does not take the app down or lose the cache.

**Bring-your-own editor** comes later, and the mechanism should be designed in now: an expression may live either inline in the scene file or in an external `.py` file referenced by path. External files are watched for changes and hot-reloaded on save. That single mechanism gives the artist any editor they want, plus version control on expressions for free.

### Tool scripting

The same Python API builds custom UIs against feather-tk widgets — sliders, buttons, and layouts wired to emitter and field parameters, declared in a few lines in the scene's script. This is the capability that made Sophia worth using and that most modern tools have lost.

### Scene API

Full scriptable access to create, query, and modify every scene object; drive batch operations; and implement import/export. Anything reachable from the GUI must be reachable from Python.

---

## 10. Rendering

Fast, previewable, comp-oriented. Not a beauty renderer.

### Particle render types

point · sphere · streak (velocity-aligned) · sprite · disc · instanced geometry (with per-particle orientation and scale) · trail/ribbon · blobby/metaball *(later)*

### Sprites

Treated as a first-class subsystem rather than a render option. Sprites are the cheapest way to buy visual complexity in this kind of work, and the difference between a passable sprite implementation and a good one is most of the difference between a shot that reads and one that doesn't.

- **Sources:** single image, image sequence, or a sprite sheet / atlas with a grid or named regions
- **Per-particle variation:** random selection from a set, random frame offset, random playback rate, random rotation, random flip. Variation is what stops a sprite cloud from looking like a stamped repeat.
- **Frame driven by attribute:** sprite frame indexed by normalized age, speed, or any custom attribute — so an explosion sprite plays out over each particle's life rather than on a global clock
- **Orientation:** camera-facing, velocity-aligned, fixed-axis, or free rotation about the view vector
- **Color and alpha:** per-particle tint multiply, opacity, premultiplied and straight alpha handling, and additive / over / screen blend modes
- **Aspect and pivot:** preserve source aspect, with an adjustable pivot for sprites that should hang or trail from a point rather than centre on it
- **Sorting:** depth-sorted per camera for correct `over` compositing, with sorting skippable for purely additive sprites (a meaningful performance lever, since sorting millions of sprites per frame is not free)
- **Texture caching:** sprite sequences share the image cache with the plate reader, with a memory budget and eviction

### Shading

Deliberately simple: unlit constant, per-particle color and opacity, simple diffuse from scene lights, sprite texture with an alpha, and additive/screen/over blend modes. Glow as a post-process on the render pass.

### Lights

Point, directional, spot, and ambient. Enough to give sprites and instanced geometry a sense of direction and to key off the plate's lighting. Lights can be animated and imported.

### Motion blur

Non-optional. Velocity-based, with a shutter angle and shutter offset, and the ability to fall back to multi-sample accumulation for instanced geometry. Substep count drives the sample count.

### Render presets

**One renderer, one code path, different settings.** Everything renders in the viewport — there is no separate offline renderer with different behavior, and therefore no class of surprise where the final render doesn't match what was approved.

A **render preset** is a named bundle of quality settings: resolution scale, motion blur sample count, sprite texture resolution, particle decimation percentage, depth sorting on/off, and which AOVs to produce. Ship a few defaults — `interactive`, `review`, `final` — and let users save their own.

The viewport uses a preset like everything else. Switching the viewport from `interactive` to `final` renders the real thing in place, slowly. This makes the preview/final distinction a setting the artist controls and can see, rather than a hidden discrepancy they have to learn about. When the viewport is on a reduced preset, say so in the HUD.

### Passes / AOVs

RGBA beauty · depth · velocity (for vector blur in comp) · particle ID · age · per-system mattes · custom attribute passes

Output to EXR, multi-layer, with OCIO-managed color throughout. Half-float by default.

---

## 10a. Render layers and the built-in compositor

Not something the older particle tools had, and one of the higher-value additions here.

### Render layers

A **render layer** is a named set of particle systems plus the render settings applied to them: render type, blend mode, motion blur settings, and which AOVs to produce. Because each system owns its own pool, a layer is just a list of references — no masking, no per-particle bookkeeping.

Layers are the unit of iteration. An artist renders the spark layer at full quality while leaving the smoke layer at preview resolution, or re-renders one layer without touching the others. Layer state is cached independently.

### Compositor

A simple, ordered layer stack — deliberately not a node graph. Each layer has:

- blend mode (over, add, screen, multiply), opacity, and an on/off toggle
- 2D transform (translate, scale, rotate) for cheating an element into place
- basic grade (exposure, gamma, saturation, tint)
- a small set of effects: blur, glow/bloom, and a defocus approximation driven by the depth AOV
- reorder by drag

**Every compositor parameter is a `Parameter` (§4a), so all of it is animatable and curve-drivable for free.** Layer opacity ramping over the shot, a glow blooming on the impact frame, a blur easing off — none of this needs new machinery, because the parameter and curve system was built first. This is the main payoff of that phase-1 decision.

**Start with layers and blend modes only.** Effects come after the stack works. Each effect is independently small, which is exactly what makes them dangerous — see the scope boundary below.

**The plate is layer zero.** This is the point of the whole feature. The imported plate isn't a special-case viewport background — it's the bottom of the same stack the artist is grading and reordering. What they see while art-directing is the comp, not an approximation of it, which is the difference between judging an effect and guessing at it.

The compositor is for *look development and review*, not for final comp. It exists so the artist can make confident decisions before the shot leaves the building. Output is both a flattened preview render (for dailies and review) and the unflattened multi-layer EXR (for the real comp downstream). Both must come from the same evaluation, so the preview never lies about what the comp will receive.

### Scope boundary — a hard rule

**The compositor exists to make the plate the bottom of the stack and to judge the look before handoff. Anything beyond that goes downstream.**

This rule is written down because every individual violation of it will sound reasonable. Layer masks are reasonable. Letting one layer's depth AOV drive another layer's defocus is reasonable. Roto shapes, tracking, a second stack, keyers — each is a small, sensible addition. Accept all the reasonable additions and the result is a year spent building a worse Nuke instead of a particle tool.

When a feature request arrives, the test is: *does the artist need this to decide whether the effect is working?* If they need it to deliver a finished frame, it belongs in Nuke.

feather-tk's existing render layer and tlRender's image pipeline cover most of the underlying machinery; this is largely assembly rather than new infrastructure.

### Viewport rendering

Shares the render types with the offline path so what the artist sees is what they get. Adds display-only modes: color by attribute (speed, age, ID, custom), point-size scaling, particle count decimation for interactivity, ghosting and motion trails, velocity vectors, emitter and field gizmos.

**Background plate.** The imported camera's plate loads behind the sim in the viewport, with correct aspect, gate, and OCIO display transform. Without this, comp-first work is guesswork.

---

## 11. Import / export

### Import

- Geometry (static and animated/deforming): **Alembic**, **USD**, OBJ
- Cameras with animation, plus film-back and lens data: **Alembic**, **USD**
- Image sequences for plates and sprite textures: EXR, DPX, PNG, JPEG, TIFF (reuse tlRender/DJV's I/O)
- Vector fields for volumetric forces: OpenVDB *(later)*

### Export

- **Particle caches:** Alembic points, USD point instancer, and a native binary format. Caches carry all persisting custom attributes, plus velocity for downstream motion blur.
- **Render passes:** multi-layer EXR sequences
- **Baked geometry** for instanced particles *(later)*

Import and export should be implemented against the Python scene API where practical, so that adding a format doesn't require a C++ change.

---

## 12. Manual control affordances

Where the "manual over algorithmic" principle stops being a slogan. Treat these as core features, not conveniences.

- **Select particles in the viewport** — click, box, lasso, by attribute range, by ID, by system.
- **Inspect** — a spreadsheet view of the selected particles' attributes at the current frame.
- **Edit by hand** — set attributes on a selection, delete particles, move them, kill them.
- **Isolate/solo** — view one system, or one selection, with everything else hidden or ghosted.
- **Lock frames** — freeze a range of the cache so approved work can't be lost to an upstream edit.
- **Seed control** — every stochastic element has an exposed, re-rollable seed. Artists dial seeds; make it one click.
- **Snapshot/version** — save the current parameter state as a named variant and A/B against it.

---

## 13. Scene persistence

- Human-readable **JSON** (feather-tk already depends on nlohmann), diffable and merge-friendly under version control.
- The scene file is **data, not code**. Scripts are referenced by path or stored as embedded strings, but the scene is not itself a Python program. This keeps loading safe, fast, and inspectable, and lets tooling read a scene without executing it.
- Paths stored relative to the project root, with environment-variable expansion for pipeline roots.
- Every scene records the app version, the seed state, and the full parameter set needed to reproduce the sim exactly.
- Undo/redo via feather-tk's command stack, covering script-driven edits as well as GUI edits.

---

## 14. Headless / batch

A `--headless` mode that loads a scene, runs a frame range, and writes caches and/or passes, with no window and no GL context required. Frame ranges must be splittable across machines, which is why forward-only determinism and an explicit sim start frame matter. Progress and errors go to stdout in a farm-parseable form.

---

## 15. Phasing

**Phase 1 — Skeleton**
Particle pool and attribute system. **The uniform `Parameter` type**, with constants and animation curves. Time/cache model with a working cache bar. Point and volume emitters, gravity and drag, lifespan and kill rules. Point and sphere viewport rendering. Playback and scrubbing. Scene save/load. *Goal: particles fall, scrubbing feels right, and every value is already animatable.*

**Phase 2 — Usable**
Curve editor UI with animation and profile curves, including per-particle variance bands. Geometry import and geometry emitters. Camera import and plate display. Full field set. Collisions against geometry. Ramp-over-life on every attribute. Streak and basic sprite rendering. Native cache export. *Goal: an artist can do a simple shot end-to-end.*

**Phase 3 — Scriptable**
Python bindings with zero-copy attribute views. Creation and runtime expressions, edited in built-in panels. Event system. Secondary emission. Script-built tool UIs. *Goal: Sophia's authoring loop, in Python.*

**Phase 4 — Production**
Alembic/USD import and export. Full sprite subsystem. Render layers and the compositor stack. Multi-layer EXR passes with AOVs and OCIO. Motion blur. Instanced geometry rendering. Lights. Manual selection and editing tools. Headless mode. *Goal: shippable to a facility.*

**Phase 5 — Depth**
Trails and ribbons. Goal/flocking rules. Volumetric fields from VDB. Blobby rendering. External expression files with hot-reload. Performance work: cross-system threading, SIMD, optional JIT expression backend.

---

## 16. Open questions

*Resolved: attribute schema (declared, frozen during sim), systems vs. pool (per-system pools), threading (single-threaded solve first), expression editing (built-in first, external files later), where the plate stops (it's layer zero in the compositor), preview vs. final (one renderer, named quality presets, viewport included), compositor scope (layers and blend modes first, effects after, hard boundary written into §10a).*

**Cache format — start simple.** A cached frame stores exactly the schema it was written with. If the schema changes, the cache is invalidated rather than defaulted. Invalidating is one line and never silently changes a sim; defaulting is friendlier and can be added later once the format has a version field. Add the version field now, use it later.

Still open:

- **Layer caching granularity.** Cached per layer, per system, or both? Per-system is finer and matches the solve; per-layer matches how artists iterate.
- **Sub-frame cache resolution.** Substeps are needed for collisions and motion blur. Are intermediate substeps cached, or only frame boundaries with velocity retained for blur reconstruction?
- **Instanced geometry memory.** Instancing implies loaded source geometry per system. Shared asset cache across systems, or per-system copies?
- **Curve edits and cache invalidation.** Dragging a key at frame 60 invalidates 60 onward — but during an interactive drag that means continuous re-simulation. Likely answer: re-simulate live through the current viewport frame only, and fill in the rest of the range on mouse-up. Worth prototyping early, as it shapes how the cache and UI talk to each other.
- **Curve LUT resolution.** Fixed sample count for all profile curves, or adaptive based on curvature? Fixed is simpler and probably sufficient.
