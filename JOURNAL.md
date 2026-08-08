# Design journal

What was decided and why. The reasoning is what evaporates first, so it is
written down here rather than left in the code, where only the conclusion fits.

Newest entries at the top.

---

## 2026-08-08 — Multiple viewports, and a manifest instead of options

One, two, three or four viewports, each looking through a perspective or an
orthographic axis view, with pan and zoom.

### The viewports are made once and kept

`Views` builds all four up front and re-parents them into a new tree of
splitters when the arrangement changes, rather than making and destroying them
to match. Two things fall out of that: a camera is where it was left when an
arrangement comes back, and the shaders, vertex buffers and offscreen buffer
behind a viewport are not thrown away and rebuilt every time somebody changes
their mind. Four is a fixed limit, which §2a explicitly allows, and it is a real
one rather than a hint -- `viewCountMax` is what sizes the array.

Rebuilding means detaching the viewports *before* dropping the splitters, or the
splitters take them down with them.

### Ortho and perspective share one camera

One `_center`, one `_orbit`, and then `_distance` for perspective and
`_orthoHeight` for orthographic. Both are kept while the other is in use, so
switching a viewport from top to perspective and back does not lose the framing.
The axis views are the same code path with the orbit pinned and the projection
swapped; they do not orbit, because orbiting the front view would leave it
something that is no longer the front.

Panning turns a screen direction back into a world one by undoing the view
rotation, and scales it by how much world one pixel covers -- `_orthoHeight` over
the height in orthographic, and the frustum height at the centre in perspective.
That is what makes the scene stay under the cursor rather than merely move the
right way.

### Screenshots come from a manifest, not from options

There was a `-frame` and then a `-layout`, and it was obvious where that road
goes: an option per thing worth putting in a picture, each one describing a
state somebody happened to need once, all of them a second way of setting
something the application can already set.

`Capture` (ported down from DJV's) reads `etc/Screenshots/screenshots.json`,
applies a shot's setup, lets the window settle, and writes a PNG plus a JSON
sidecar. One process per shot, so nothing a shot leaves behind can reach the
next one.

The sidecar is the part worth having. It carries the bounding box and the
visible text of every widget tagged with `ftk::setScreenshotTag`, so a shot can
be checked without anyone looking at the image: the four-up sidecar says the
viewports read "Persp", "Top", "Front", "Side" and tile the window, and the lock
shot says the checkbox reads `Lock [checked]` at frame 40. That is an assertion.
A picture is not.

DJV's `make_svg.py` step, which turns a sidecar into an annotated SVG for the
documentation, is not ported -- there are no documentation pages yet. The
`annotate` entries pass through to the sidecar ready for it.

---

## 2026-08-08 — Leaving GL state changed draws every glyph as a box

The viewport's offscreen pass turned blending off after drawing its points and
never put it back. `gl::Render::begin()` enables `GL_BLEND` once for the whole
frame and its primitives only ever set the blend *function*, so from then on
every glyph drew as an opaque quad. The tell in the screenshot was that the menu
bar was fine and everything below it was boxes: the menu bar is drawn before the
viewport.

Two things worth keeping from it.

**It was invisible to `-screenshot`.** The viewport only re-renders when
`_doRender` is set, so only frames where it actually ran are affected, and the
buffer that got captured was not one of them. Reproducing it took forcing
`_doRender = true` on every draw. A widget that renders conditionally can hide a
state bug from exactly the check meant to catch it, and the A/B — force the
condition, capture with and without the fix — is the way to be sure.

**The rule this establishes:** a widget that touches raw GL puts back what
`Render::begin()` set up. ftk has state helpers for the render's own state
(`ViewportState`, `ClipRectState` and the rest) but nothing for raw GL, so this
is a convention rather than something the compiler will hold us to. If a second
widget ends up needing it, that is the point to write the scoped guard rather
than repeat the comment.

---

## 2026-08-08 — The first vertical slice

A pool that emits from a point, falls under gravity, draws as points, and
scrubs. Built as one slice rather than as layers, so that every architectural
decision in it got tested against something running before anything was
committed to.

### The frame is the unit, not the pool

`core::Frame` is what the cache holds and what the solver takes and returns:

```
core::Frame System::step(const core::Frame& prev, int frame, double frameRate) const
```

`step()` is `const` and the system holds no state. Everything carried between
frames is in the `Frame`, which today is the pool plus one double.

This came out of the emission-rate problem. A rate of 200 particles a second at
24 frames a second is 8.33 particles a frame, and the third of a particle has to
go somewhere. The obvious place is an accumulator on the emitter — and that is
exactly the hidden state that breaks re-simulation, because resuming from a
cached frame would resume with the accumulator at whatever the last full run
left it at. Putting it in the `Frame` makes it cached along with everything
else, and makes the rule explicit: if it survives a frame boundary, it is in the
`Frame` or it does not exist.

`SystemTest::_resume()` is the test that holds this. Running 1–40 and running
1–17 then 18–40 have to produce bit-identical frames. It is the cheapest
possible test and it will catch the next piece of state somebody puts on a
solver.

### Randomness is keyed, not sequential

`core::randF(seed, id, channel)` hashes rather than advancing a generator.

A sequential generator gives a different answer depending on where the run
resumed, which is the one thing the cache cannot tolerate. Keying on the
particle's id — which is stable, never reused, and does not move when the pool
is compacted — means a particle draws the same numbers however it was arrived
at.

The channel argument matters more than it looks. Each draw a particle makes
(cone angle, azimuth, speed, lifespan) has its own channel, so adding a fifth
draw later does not shift the values the first four get. Without that, adding a
feature would silently change every shot that had already been approved.

### The cache holds `shared_ptr<const Frame>`

Showing a frame costs nothing, a frame still being drawn survives being evicted
under the memory budget, and nothing downstream can edit history. All three fell
out of one decision; the alternative, handing out a `const Frame*` into the
cache's storage, was one eviction away from a dangling pointer in the viewport.

### Re-simulation is lazy

`SceneModel::_simulate(frame)` walks back to the last frame the cache still
holds and steps forward from there. Nothing else drives the solver — playback,
scrubbing, and a parameter edit are all "make sure frame N exists".

That gives the answer to the open question in §16 about interactive curve edits
for free: `parameterChanged()` invalidates from the start of the range but only
simulates as far as the viewport frame, so dragging a slider re-simulates the
frames being looked at and nothing else. Filling in the rest of the range on
mouse-up is an addition to this, not a redesign of it.

Invalidating from the start of the range rather than from the playhead is
deliberate, and is a departure from §5. §5's rule is right for a curve edit at
frame 40. It is wrong for a constant, which was never only in effect from frame
40 on — invalidating forward from the playhead there would leave a cache that no
run from the start could reproduce, and a locked frame in it would then be
lying. When curve editing arrives, `Cache::invalidateFrom()` already takes the
frame; it is the caller that will choose a different one.

### What was deliberately not built

- **The declared attribute schema (§4).** The pool has named members instead.
  There is one set of attributes, and the schema's shape is a guess until there
  is a second. A wrong guess here would have to be re-understood every time
  anything touches the pool.
- **The generation counter on the pool (§4).** It exists to invalidate NumPy
  views, and there is no Python yet. Adding it now would be three lines of
  machinery with no consumer to keep it honest.
- **Profile curves, expressions, connections (§4a).** `Parameter::Type` lists
  only `Constant` and `Curve`. Enumerating cases that do nothing makes the type
  look finished when it is not.
- **Free Bezier handle timing.** `Interp::Bezier` evaluates as a Hermite
  segment, which is a cubic Bezier with its handles a third of the way along in
  time. Free timing needs a solve for t given the frame, and nothing can author
  it yet.
- **Sub-frame birth offsets.** A particle born mid-substep is integrated for the
  whole substep. At one substep this is invisible; it will show up as banding at
  high substep counts and high speeds, and that is when to fix it.

### Smaller things worth remembering

- **The emission epsilon.** `static_cast<uint64_t>(emitted + 1e-6)`. A rate of
  24 a second at 24 fps divides to a hair under 1.0, and without the slack the
  floor drops a particle every so often. Found by a test asserting 10 particles
  after 10 frames.
- **Points draw additively with depth writes off.** Sparks and embers read as
  light rather than as surfaces, and it means the points never have to be
  sorted. Sorting becomes a real cost with sprites; it is free to avoid now.
- **Colour by age is a display choice.** The pool has no colour attribute. Being
  able to *see* age was worth more at this stage than being able to author
  colour, and the viewport is where display-only modes belong anyway (§10a).
- **`-frame N`.** Added so `-screenshot` is worth taking: a screenshot of frame
  one shows eight particles and proves nothing.
- **The cache bar is the timeline.** Clicking it scrubs. Keeping the cache
  read-out and the scrub control as one widget means the artist cannot be
  looking at one and interacting with the other.
