# Design journal

What was decided and why. The reasoning is what evaporates first, so it is
written down here rather than left in the code, where only the conclusion fits.

Newest entries at the top.

---

## 2026-08-09 — Undo, and two bugs it found

The command stack, moved into phase 1 for the reason the `Parameter` type is
there: undo is not a feature, it is a claim about where edits happen, and every
edit written before the claim exists has to be found and moved. There were
eight such places. There will be sixty.

**One command for every kind of edit.** It holds the whole system before and
after rather than a description of what changed. That sounds wasteful and is
not: a system is a recipe of a few dozen parameters, copying one is far cheaper
than the re-simulation the edit causes anyway, and it means setting a constant,
moving a key, deleting one and re-rolling a seed are the same command rather
than four classes that each have to get their inverse right.

The shape is edit-then-report rather than describe-then-ask: the widget mutates
the system through `getSystem()` and hands back what it looked like beforehand.
That keeps the existing pointer-based editing and still gives the model both
ends of the change.

**A drag is one step.** First attempt guessed at it -- merge with the previous
command if the name matches and the values follow on. Darby pointed at
`setPressedCallback` on the sliders, which is the honest answer: a slider knows
when it was taken hold of and let go, so the edit is bracketed rather than
inferred. Two edits that happen to leave the system in the same state are not
necessarily the same edit, and now nothing has to assume they are.

### The drag was cancelling itself

Verifying this needed the harness to drag in steps rather than to jump, since a
widget that treats a drag as one gesture and one that treats every move as a
separate edit look identical from a single event. Made it move in eight, and
the eight-step drag landed somewhere the one-step drag did not.

The cause was not in the drag. Every edit raises "the parameters changed", the
curve editor rebuilds its channel list on that, and handing the plot its
channels again resets the selection -- so the first move of any drag ended it.
It had been there since the editor was written and could not be seen without
a drag that was more than one event long. The plot is only handed its channels
when the set actually differs now.

### And the harness was reordering the shot

Then a drag followed by an undo produced a state that made no sense. Clicks and
drags have to wait for the window to be laid out, so they are applied later than
the rest of the setup -- which meant everything written after one ran *before*
it. A shot that clicks and then undoes was undoing the click. The first
deferred step defers the rest now, and a manifest runs in the order it reads.

None of the three showed up in the values the sidecar records. They showed up in
a number that was wrong for a reason nothing on screen would explain.

### Then two more, from looking at it

Darby undid a rate change and the viewport went back while the slider went on
reading 2000. The panel refreshed on frame changes and on a new scene, which is
every way the values could change *except* the one that had just been added.
The plot had the same hole for the same reason and would not have been noticed
as quickly: the editor only hands it channels when the set differs, so undoing
a key move redrew nothing. Both follow every edit now.

Point size had no undo at all, which was a decision rather than an oversight --
it is display state, not part of the recipe, and it deliberately stays out of
the scene file. That distinction is real and it is also invisible: it is a
slider in the panel of sliders, and nobody is going to remember which ones
undo. So the command holds it too. What the stack captures is no longer "the
system" but "everything an edit can touch", which is the honest description and
leaves room for the render presets §10 wants.

---

## 2026-08-09 — Saving a scene, and keying one

Two of the four things §15 still wanted from Phase 1.

### The file

JSON, as §13 asks for, and written to be read rather than only to be parsed.
A constant parameter is a bare number; only an animated one becomes an object,
and then it carries its constant alongside the curve so that switching the
animation off returns the value to where it was rather than to zero.
Interpolation and infinity are written as names, not as the integers the enums
happen to have, so inserting a mode does not silently reinterpret every file
already on disk. A key writes its slopes only when it is a Bezier, because for
every other mode they are either unused or taken from the neighbours, and
storing them would put numbers in the file that editing a neighbour invalidates.

Floats are narrowed before they are written. A float promoted to double prints
as the double nearest the float, so a tenth reads `0.10000000149011612`; the
shortest form that still parses back to the same float is used instead. It is
exact -- the round trip test compares for equality, so a wrong answer here
fails rather than drifting.

Every field is optional on load and defaults from a fresh object, so a file
written before a field existed still opens. What is not optional is a file that
makes no sense: a range that runs backwards or an interpolation nobody has
heard of throws. `open()` reads the whole file before touching the model, so a
bad file costs the artist nothing.

**Modified is compared, not flagged.** `Scene::operator==` walks the recipe, so
nudging a value and putting it back is not a change. That is worth the handful
of comparisons it costs; a dirty flag that lies is worse than no dirty flag.

Two things fell out of the wiring. The framework's File menu holds Exit and
nothing else, and appending to it puts New and Open *below* Exit -- so it is
replaced instead, and `MenuBar` grew `insertMenu` to put the replacement back
where the original was. And opening a scene changes every value at once, which
no panel was watching for: `parameterChanged()` is what a panel raises itself,
and a panel refreshing on its own edits would fight the drag that caused them.
So there is a separate signal for wholesale replacement.

### The keys

`Parameter` has had curves and `SystemTest` has animated gravity through them
since the first week. Nothing in the interface could author one, which is a
strange place for a tool whose design says "can I animate this?" should never
be a question.

A diamond beside every parameter now keys it at the playhead, and the Curves
pane is a real editor: a channel list, a plot sampled a pixel at a time so it
shows what the solver will actually see rather than straight lines between
keys, and keys that can be dragged. Dragging one re-runs the simulation, which
the harness checks -- a shot that drags the frame 40 key of a rate curve down
reports 406 particles where the one that leaves it alone reports 1768.

Dragging a *slider* on an animated parameter moves the key at the playhead
rather than throwing the curve away. Silently discarding animation because a
slider moved is not something anyone would ask for.

The panel and the editor read the same list of what exists, in
`ParameterList.h`. Two lists would be two things to keep in agreement, and the
way to make two things agree is to not have two things.

### Two bugs, one of them old

The plot came up empty with every channel ticked. An ftk observer runs its
callback when it is created, and the scene observer cleared the set of visible
channels -- so the constructor filled the set, built the rows from it, and then
the observer emptied it behind them. The rows were right and the plot was not,
which is exactly the shape of the symptom.

Fixing that turned up a segfault that was already there: the channel list is
rebuilt by detaching its children, and it was iterating the list
`getChildren()` returns a reference to while `setParent(nullptr)` erased from
it. It had never fired because the only rebuild that had ever run was the first
one, when the list is empty. The fix is to copy the list first -- which is what
`Splitter::setWidgets` in ftk already does, for the same reason.

### What the editor does not do yet

One value range shared by every channel, so a gravity curve between -30 and -2
is a flat line at the bottom of a plot that goes to 1800. §4a wants normalised
and absolute views, which is the answer. No axis labels, no box selection, no
tangent handles, no framing. All of those are additions rather than rewrites.

---

## 2026-08-09 — A thinner playhead, and which way is up

Two from Darby, both about reading the screen rather than about behaviour.

**The playhead.** The choice offered was an ftk slider handle, for consistency
with the parameter sliders, or a thin bar like the tlRender timeline. The
slider handle is the wrong idiom here despite being the consistent one: a
slider's trough carries no information, so a handle covering part of it costs
nothing, whereas every pixel of this bar is a frame with a state. A handle wide
enough to take hold of is wide enough to hide the frame it points at -- which is
the same mistake the filled cell made two entries ago, arrived at from the
other direction.

So it is the tlRender form: a two-pixel marker centred on the frame rather than
at its left edge, so it points at a cell instead of at the join between two.
The affordance the slider handle would have given comes from the whole strip
lighting up under the pointer instead, which is also the first hover feedback
the bar has ever had.

**The tripod.** A small three-axis gizmo in each viewport's bottom-left corner,
drawn in pixels with the camera's rotation but not its position or its zoom --
it says which way the scene is facing, nothing else. Each axis has a faint stub
running the other way, because with only the positive half drawn Front and Back
are the same picture. It is sized in points, which a draw event does not carry,
so the display scale is caught in `sizeHintEvent` and kept.

Written first in OpenGL alongside the grid and the points, then moved onto
`event.render` -- the question being whether the viewport's drawing could be
centralised there, since OpenGL is meant to be retired eventually.

For the grid and the particles the answer is no, and forcing it would be a
mistake: the renderer is two dimensional. `TriMesh2F` has no z, mesh primitives
hardcode straight alpha, and there are no point sprites, so the projection
would move to the CPU and the scene would lose its depth, its additive
accumulation -- which is what makes the core of a plume read as light -- and
its round points. Centralising a 3D viewport behind a 2D API costs the three
things that make it a 3D viewport.

The tripod is the exception, and it belongs there. It is a screen space overlay
at a fixed size, so it has no projection to lose, and it comes out better for
the move: `LineOptions` carries a width where `glLineWidth` is one pixel
whatever is asked for, and `circle()` gives real dots on the tips rather than
point sprites. Sorting the three axes by depth before drawing replaces what the
depth test was doing. It draws after the buffer is blitted rather than inside
it, which is what an overlay should have been doing anyway.

---

## 2026-08-09 — Measuring the four-up, and the ruler being wrong

A screenshot of four viewports under load: 5926 particles, 48MB of GL buffers,
14ms a frame. The question was whether any of that is out of proportion.

Two of the three answer themselves. `ftk GL Memory/Buffers` counts offscreen
buffers, not vertex buffers -- the 48MB is the `RGBA_F16` viewport targets, and
`Meshes: 0MB` beside it says the particle geometry is nothing (six thousand
particles across four panes is under 400KB). And `fx Sim/Time` at zero with the
cache full says playback is simulating nothing, so the frame time is all
drawing.

Measured at 1840x1240:

| | GL buffers | offscreen objects |
|---|---|---|
| one pane | 115 MB | 2 |
| four panes | 113 MB | 5 |

Splitting one viewport into four is free. It is the same pixels cut up
differently, which is the answer to "should the four-up worry me" and also the
reason the F16 decision in §10 is the only lever that moves this number.

### A fifth number does not fit

Adding Peak pushed the frame group's legend off the side of the panel. The
legend is a horizontal layout of one entry per sampler, which is right for a
legend only while it has the room -- four entries were already one sampler away
from the same thing, and the diagnostics colour list goes up to six.

ftk had no wrapping layout, so there is one now. `FlowLayout` places children
left to right and starts a new line when the next will not fit. The awkward
part is that its height depends on its width, and `getSizeHint()` is not told
the width; it reports the tallest single child until it has been given a
geometry, and what that width actually needed thereafter. That means it settles
over two passes instead of one, which is the price of the layout protocol not
having height-for-width, and is cheap enough here.

Verified the way the rest of this has been: the test fails with the wrap
disabled and passes with it, and the panel captures at three widths show one
line, two lines and three.

### The frame time could not answer anything

Five captures of the same scene, one pane and then four:

```
one pane    3  6  8  8  9   ms
four panes  5  5  6 11 12   ms
```

Overlapping completely, with the medians the wrong way round. `RenderDiag::time`
was one frame's `begin()` to `end()`, cast to whole milliseconds, read by a
sampler that fires every few seconds -- so both the quantisation and the choice
of which single frame to look at varied by more than the difference being
looked for. Fine for catching a five-fold regression, useless for comparing two
configurations, which is most of what a diagnostics panel is for.

It is the mean over the last sixty frames now, in microseconds, and there is a
peak beside it over the same window: the mean says what a frame costs, the peak
says whether it hitches. The same ten captures:

| | mean | peak |
|---|---|---|
| one pane | 9.8 ms | 12.2 ms |
| four panes | 11.1 ms | 13.3 ms |

Four viewports cost about 13% more than one, and the peak sits a couple of
milliseconds above the mean in both, so the extra panes do not introduce a
hitch. That is a sentence the old readout could not have supported.

`fx Sim/Time` had the same rounding and is microseconds too. It read zero for
every scrub inside the cache; it reads 50387 for a jump of fifty frames at six
thousand particles, which is a millisecond a frame and worth knowing.

The harness needed two things to run this: a `rate` step, because frame time is
only meaningful against a known particle count, and a settle timeout that
scales instead of a fixed six seconds, which was killing an eight second shot
before it captured.

---

## 2026-08-09 — Reading the screenshot back

A screenshot of the four-up, looked at as a stranger would. Five things were
wrong with it, and all five were the kind that only show up in a picture.

**Every pane header said the same thing.** Four panes, four headers reading
"Pane View", and no way to tell the perspective one from the top one without
opening a menu. The combo boxes these menus replaced got this right without
trying: a combo box shows its selection. The menus were named for what they
contained. So `ftk::MenuBar` grew `setMenuText`, and each pane now titles its
two menus with what is picked in them -- "View Persp", "View Top". The rename is
taken by menu pointer rather than by title, since the title is the thing that
moves; `Pane` holds the two menus instead of looking them up.

That leaves the window with a View menu and every pane with one, meaning
different things. The window's is about how the panes are arranged, so it is
"Layout" now.

**The points were square.** Obvious at three pixels once you look, glaring at
fourteen. They are cut back to the disc inside with `gl_PointCoord`, and the
outer pixel faded rather than stepped -- an unsoftened disc looks worse than the
square did. The grid lines share the shader and `gl_PointCoord` means nothing
outside a point, hence the uniform switching it off for them. A `pointSize`
capture step gives this a shot big enough to see.

**Three parameter rows had no reset button** -- the three where nobody called
`setDefault`. Read as a panel where some values can be put back and some cannot,
which is worse than not offering it at all. All of them have a default now.

The button itself was a cross, which beside a value reads as "get rid of this",
and it sat a few rows below a panel close button that was also a cross. ftk's
Reset icon is only ever used by reset buttons, so the artwork just changed to
the undo arrow.

None of which could be seen at the default width: the column opened at 216
points and the rows wanted 327, so the resets were outside it and reachable only
by scrolling. The split starts at .66 now, which fits them.

**The cache bar was a strip of colour along the bottom edge.** One handle tall,
two points from the window edge, and though it has always been clickable it did
not look it. Two handles tall, inside the bar's margin, and bordered like the
other controls.

Widening it exposed a second thing: the playhead was a filled cell, so it hid
the state of the one frame most worth knowing the state of. The `cache-locked`
shot was captioned "the locked frame" and never showed it -- the blue was
underneath the white. The playhead is an outline now.

The `1.4 MB` beside the particle count was unlabelled. It is the cache.

### The rename did nothing, and the sidecar said it worked

Shipped, looked at, and the header still said "Persp" with Side ticked in
the menu below it. `MenuBarButton` caches the glyphs it measured the first
time and only throws them away when the style changes; `IButton::setText`
calls `setSizeUpdate()`, which is not the same thing. Five of ftk's button
subclasses override `setText` to set their own cache flag -- five identical
copies of the same six lines -- and four never did, `MenuBarButton` among
them. `IButton` calls a `_sizeDirty()` hook now and the subclasses override
that instead, which is both shorter and not something the next subclass can
forget.

Worth recording how this got past the harness. The sidecar reads a widget's
text out of the widget, and the widget's text was right the whole time; only
the pixels were wrong. Every earlier bug in here -- the phantom splitters, the
dropped pane, the crash -- moved something the sidecar could see. This one did
not, and no assertion over the sidecar would ever have caught it. The picture
is not a nicety on top of the harness.

The reset icon needed a second pass for a related reason: the first one reused
the Undo artwork, which is drawn for twenty pixels, so at eleven the ring came
out thinner than a pixel. Drawn for its own size now.

### The one that was not in the screenshot

`Frame` frames the current pane. In a four-up that is rarely what is wanted, and
there was no way to ask for the rest, so `Frame All` sits beside it.

---

## 2026-08-09 — A splitter that divides both ways

The four-up was two splitters ganged together with a callback keeping the rows
in step. It worked, and it was still two splitters: the place where the
divisions cross belonged to neither of them, so there was nothing there to grab.
`ftk::Splitter2D` has one division each way and one crossing, and dragging the
crossing moves both.

The hit test asks about the crossing first, because it is part of both
divisions; the upright one moves the split across, the flat one moves it down.

### Dragging, in the harness

Verifying this needed the harness to drag, not just click, so `MainWindow` grew
a `drag()` beside its `click()` and the manifest a `drag` step. Both are three
lines on top of the protected window hooks.

What that buys is an assertion rather than a look. Four shots at the same
window size, reading the pane boxes out of the sidecars:

| | pane 0 |
|---|---|
| untouched | 768 x 560 |
| drag the crossing | 492 x 340 |
| drag the upright division | 492 x 560 |
| drag the flat division | 768 x 340 |

Both dimensions change for the crossing, one each for the single divisions.
That is the whole feature, stated in numbers, and `panes-four-dragged` keeps it
that way.

This is the second time the answer to "how do I test this" was that ftk already
allowed it and the harness had not caught up. Worth remembering that the
harness's limits have mostly been mine rather than the toolkit's.

---

## 2026-08-09 — The rest of the feather-tk list

Four more, all of them things another application would hit.

**`gl::StateSave`.** The white-boxes bug from the first week, fixed at the
source. A widget drawing its own OpenGL now takes one guard at the top of its
draw and the state goes back on the way out. The viewport's manual restores are
gone; it only enables things now. `SetAndRestore` was already there for a single
capability, so this is the blunt end of an existing pair rather than a new idea.

Verified the way the bug was found: force the viewport to re-render every frame,
and check the text below it.

**`MenuBar::removeMenu()`.** The absence of this is why the pane rebuilt its
whole menu bar on every content change, which is what crashed. Now only the View
menu is added and removed and the Pane menu -- the one an action is usually
being picked from -- is built once and left alone.

Making removal safe meant the buttons looking their index up when a callback
runs rather than capturing it when the callback is made. A captured index is
correct exactly until something is removed before it.

**`DiagSystem::setTickTime()`.** Three seconds makes the graphs a trend over
minutes. Nothing here needs finer, but an application chasing a hitch does, and
there was no lever.

**Two doc notes**, for the two things that cost real debugging here: that
`drawEvent` runs before the children and `drawOverlayEvent` after them, and that
a parent holds its children by shared pointer, so a widget merely forgotten
about stays in the tree and goes on drawing.

### What the exercise produced

Six of the original suggestions, plus callback safety, minus the two that did
not survive checking. Every one of them came from a bug or a line count in this
application rather than from an opinion about what a toolkit ought to have, and
every one is paired with its adoption here, because an addition nobody has used
is a guess with a nice comment on it.

---

## 2026-08-09 — The crash, fixed where it belonged

The menu crash got two fixes. The first, here, was to stop rebuilding a menu
from inside its own callback. The second, which is the one that matters, is in
feather-tk: the dispatch sites now hold on to what they are about to touch.

* `Menu` keeps a weak reference to itself and locks it before `_accept()`.
* `IButton::click()` holds itself alive across both of its callbacks, since it
  reads its own checkable flag after them.
* `ButtonGroup` does the same across its checked callback, which walks its
  buttons afterwards.

None of this was the caller's fault. A callback is user code and may do
anything, including closing the thing the widget belongs to. A toolkit that
reads its own members after handing control away has to survive that, and an
application that has to know which callbacks are safe to act in has been handed
a rule it will forget.

Verified by putting the application's bad code back: rebuilding a pane's menu
bar from inside that menu's own callback used to segfault on the first click and
now does what it says. That is a better test than the regression shot, because
it exercises the toolkit fix rather than the workaround.

The workaround stays. Deferring the rebuild to the next tick is still the right
thing for an application to do -- taking a widget apart while it is dispatching
is confusing whether or not it crashes -- but it is no longer load bearing, and
the comments saying so have been corrected.

---

## 2026-08-09 — The harness can click, and two suggestions that did not survive

### Clicking

`IWindow::_cursorEnter`, `_cursorPos` and `_mouseButton` are protected, so a
window can offer a `click()` that goes through the same path a person's mouse
does. DJV has had one for its documentation shots all along. Ported.

Clicks wait until the window has been laid out and then run one per settle,
because a click is a position and until the first layout there is nothing at any
position -- and because a click on what a previous click opened needs the popup
to have reached the screen first.

The `click-pane-menu` shot opens a pane's menu and picks Spreadsheet from it,
which is the path that crashed. Putting the crash back makes the shot fail and
taking it out makes it pass, which is the difference between a screenshot and a
regression test. The coordinates come from the boxes in a sidecar.

This narrows what the previous entry said about the harness. It drives the model
by default, and that is still where most of its steps live, but the interface is
now reachable when a shot needs it.

### Two suggestions withdrawn

Six improvements to feather-tk came out of building this. Three landed. Checking
the other two before writing them killed both, which is the entry.

**Font roles.** The complaint was that `Label::setFontSize()` takes raw pixels
while colours and sizes go through roles that scale. It does not: `Style::getFont(font, size, scale)`
multiplies by the display scale, and `Label` passes `event.displayScale` in. The
premise was simply wrong. What is left -- no *named* size for a heading, so each
application picks its own -- is real and tiny, and nobody has asked to theme
one. Not written.

**A shared header strip.** The claim was three implementations. There are two:
DJV's `IToolWidget` and this application's `IPanel`, which was written by
copying it. The third was `FilesTool` using `ColorRole::Header` as a row
background, which is not the same widget at all. Two is under §2a's bar, the two
already differ in how they close, and there is no way to check a shared version
against DJV from here. Not written.

Two of six claims did not survive being checked, and a third -- that
`IMouseWidget` cannot take any button -- was already wrong. The ones that did
survive were the ones with a bug or a line count behind them rather than a
recollection.

---

## 2026-08-08 — Three changes to feather-tk, on a widget-api branch

Building this application turned up things the toolkit could do rather than
things it did wrong, so three of them went upstream. Each is paired with its
adoption here, which is the only honest test of whether it helped.

**`ftk::IContainer`.** Six widgets here carried the identical pair forwarding
their geometry and size hint to a layout that filled them; `ScrollWidget` and
DJV's `IToolWidget` already had it too. Deriving and calling `_setWidget()` says
it once. Net 63 lines out of this application, and `Panels` gained the most: the
column and the tab widget were two children with their visibility toggled and
are now whichever one the container is handed.

**The button and modifiers on `MouseMoveEvent`.** A dragging widget needs to
know which button is doing it. Without that the viewport recorded the button on
press, cleared it on release, and had a release handler for no other reason.
`IWindow` already knows -- it keeps the click that started the press -- so it
fills them in.

I had this half wrong when I proposed it. `IMouseWidget` *can* accept any button
already: passing `MouseButton::None` matches all of them. What it could not do
was say which one, which is the part that mattered.

**`Splitter::setWidgets()` and a split callback.** The first makes a rebuild one
call rather than an ordering to get right, which is the phantom splitters again
seen from the toolkit's side. The second is what a linked four-up needs, and the
four-up's rows now divide together.

### The one that bit back

Adopting `IContainer` in `Panes` broke the four-up: pane zero vanished. In a
single-pane arrangement the container's child *is* pane zero, and rebuilding
into a four-up parents pane zero under the new tree and then hands the tree to
`_setWidget`, which dutifully detached the old child -- taking pane zero back
out of the tree it had just been put into.

So `_setWidget` releases only what is still its own child. "Let go of what I am
holding" is right; "detach this widget wherever it now lives" is not. A
convenience that owns lifetimes has to be careful about which of those it means.

This one the harness did catch, in the sidecar: `panes-four` came back with
three panes and one of them twice the width.

### Not done

`FTK_ENUM` is welded to `FTK_API`, so a downstream application marks its own
functions with ftk's export attribute. I overstated this as something that bit
me: `FTK_STATIC` makes `FTK_API` empty, and this application is a static build,
so the macro would have worked. It is wrong only for a shared build on Windows.
Left alone.

Font roles and a shared header-strip widget are still worth doing and are not
done.

---

## 2026-08-08 — Never take apart the thing that is calling you

Switching a pane to the spreadsheet crashed. The pane menu's action rebuilt the
pane's menu bar, which destroyed the `ftk::Menu` that was in the middle of
dispatching the click:

```
Menu::addAction(...)::$_2::operator()   // the menu's own button callback
  action->doCheckedCallback(value)      // ours: setPaneType, rebuilds the bar,
                                        // frees this Menu
Menu::_accept(this=...)                 // reads p.parentMenu on freed memory
```

`ftk::Menu` calls the action's callback and *then* closes itself. `IButton` is
the same shape: `click()` reads its own `checkable` after the clicked callback
has returned. So the rule is not about menus:

**A callback must not destroy the widget that is calling it.** Anything that
takes widgets apart -- rebuilding a menu bar, clearing a tab bar, swapping a
container -- has to wait until the event that asked for it has finished.

The fix is a dirty flag acted on in `tickEvent`. `Pane::_menuDirty` and
`Panels::_panelsDirty`, both set by the things that used to rebuild directly,
both cleared on the next tick. `_tickRecursive` ticks every widget whether it is
visible or not, so a panel column that has hidden itself still gets the tick
that brings it back -- which was the thing to check before relying on this.

The same hazard was already latent in the tab bar: closing a tab called
`setOpen`, which called `_tabWidget->clear()`, which destroyed the close button
that was mid-click. It had not been clicked yet.

Both rebuilds still run directly from a constructor, which is not a callback and
is the one place it is safe. The header would otherwise be laid out once with no
menus in it.

### What this says about the harness

The capture harness drives the model: it calls `setPaneType` the same way the
menu does, and it never crashed, because the crash needed a real `Menu` to be
mid-dispatch. Every screenshot was green while the application would fall over
on the first click.

That is worth knowing about what the harness is for. It checks what the
application *shows*, given a state it was put into directly. It says nothing
about what happens when a person puts it into that state, and it never will
without input injection. The last three bugs -- the phantom splitters, the
border under the content, and this -- were all found by looking at the running
application or by clicking in it.

---

## 2026-08-08 — A menu bar per pane, and two bugs it made visible

Three changes, and the two bugs they surfaced are worth more than the changes.

### The pane header is a menu bar

The two combo boxes are gone. Each pane has an `ftk::MenuBar` with a Pane menu
for the content type and a View menu for the camera, the current entry checked.

`MenuBar` adds menus and clears them all, but does not take one away, so the bar
is rebuilt whenever the content changes. That is why the actions are made once
in the constructor and kept: they outlive the menus they are put into, so their
checked state survives the rebuild. A pane showing a spreadsheet has no View
menu at all, rather than a disabled one that does nothing.

### Phantom splitters: the parent owns the children

Faint handles were showing through the pane headers, left over from previous
arrangements. `Panes::_layoutUpdate()` dropped `_root` and built a new tree, and
`_root` was the only other reference -- except it was not, because
`IWidget::_children` is a list of *shared* pointers. Dropping the local
reference left the old splitters parented to `Panes`, keeping their last
geometry and going on drawing. One leaked tree per layout change.

The fix is `_root->setParent(nullptr)` before the reset. The rule this
establishes, which the pane and panel code both now follow: **detaching from the
parent is what destroys a widget; dropping your own reference is not.**

### The current-pane border was drawn under the content

It was drawn in `drawEvent`, which runs before the children, so the content
covered it everywhere except the strip the header did not fill -- which read as
a box around the header rather than a border on the pane. Moved to
`drawOverlayEvent`, which runs after.

Both bugs were spotted by looking at the application, not by anything in the
capture harness. The sidecar can say what a widget's box is; it cannot say that
something was drawn over it.

### The panel column takes itself away

Closing the last panel hides the column, and `ftk::Splitter` does the rest --
with one visible child it gives it the whole area and draws no handle. Opening a
panel from the menu brings it back, and the splitter has kept its position.

### Column or tabs

A stacked column is the default, for the reason the column exists at all: these
panels are wanted at the same time as each other. Tabs are there for when the
column is narrow and one panel at full height beats two squeezed, which is a
judgement only the person looking at it can make.

The scroll area moves with the choice. Stacked, there is one for the whole
column, so a panel takes the height its contents need rather than an equal
share. In tabs each panel gets its own, since only one is on screen. In tabs the
panel's own header is hidden: the tab already names it and carries a close
button, and having both said it twice.

---

## 2026-08-08 — Panes, with stand-ins for the editors

The pane system from two entries ago, built early and on purpose. The journal
said to wait for the curve editor to pull the interface into existence; instead
the four editors that do not exist are stand-ins -- a label naming the editor
and the section of the design it comes from -- so the mechanism can be arranged,
switched and screenshotted before any of them is written.

That is a different thing from building the abstraction on one implementation,
which is what the earlier entry warned against. A stand-in is not a guess at
what a curve editor needs; it is an admission that we do not know yet, and it
still exercises everything the mechanism has to do.

### Two words that nearly mean the same thing

`Panes` is the main region -- viewports and editors, in fixed arrangements.
`Panels` is the right hand column -- parameters and diagnostics, stacked. They
are different mechanisms answering §2's two different shapes, and the names are
one letter apart, so: **panes are arranged, panels are stacked.**

`Views` became `Panes` in the same change. Leaving a spreadsheet inside
something called `Views` is exactly the sort of name that costs a re-read after
a month away.

### What the mechanism turned out to be

Almost nothing, because feather-tk's `setParent` both detaches and attaches. A
pane keeps a map of content by type, hands the layout the one that is current,
and parents the rest to null. Switching content is one line, and the trap is the
same one the arrangements hit: detach before dropping the thing that owns them,
or it takes them with it.

Content is made on first use and kept per slot -- the lazy caching the earlier
entry predicted. It is about ten lines. Switching a pane to Curves and back
finds the same viewport, at the camera it was left at, which is the sidecar
assertion in the `roundtrip` probe: after Curves, Spreadsheet, View and a switch
to top, the pane reads "View Top".

### The header strip, and what it cost

The viewport used to carry its own view menu as an overlay in its corner, which
cost no vertical space. A pane needs a menu for its content type as well, and
overlaying two menus on content that might be a spreadsheet is not a design. So
the pane has a header strip, and the viewport lost its overlay.

The cost is a strip of vertical space per pane -- real, and worse in a four-up.
What it buys is the thing the earlier entry asked for: a place for per-content
controls, so the application's menu bar does not have to change depending on
which pane was last clicked.

### Small things that only appear once content can be swapped

- **Point size moved onto the pane.** It is a display setting, so a viewport
  switched away from and back to should still have it. Held by the pane and
  applied when the viewport is made.
- **The pane never sees the click.** Content accepts the mouse, so a viewport
  passes the press back up rather than the pane trying to intercept what its own
  content is handling. A stand-in accepts nothing, so the pane's own handler
  catches those.
- **`getViewport()` returns null** when the pane is not showing one, and the
  Frame and Zoom actions check it. There is no camera to frame in a spreadsheet.

### The boundary still holds

Fixed arrangements, no drag-to-split, no tear-off, no floating windows, no saved
layouts. Nothing here needed any of them.

---

## 2026-08-08 — The panel column, and what it found on its first day

The right hand column from the previous entry, built: `IPanel` provides a
coloured header with a title and a close button, `Panels` stacks them, and
Parameters and Diagnostics are the first two.

A stack rather than tabs, because the whole reason these are a column and not
one of the panes is that they are wanted at the same time as each other --
watching frame time while dragging a slider is the point, and tabs would make it
one or the other. One scroll area for the whole stack rather than one per panel,
so a panel takes the height its contents need instead of an equal share.

Panels are made up front and shown or hidden. There are two, both cheap. The
lazy creation the previous entry called for is what to write when one of them is
not, and not before.

### Diagnostics earned its place immediately

`ftk::DiagWidget` does the work; the panel is a wrapper. What this side adds is
four samplers on `ftk::DiagSystem` -- particles, solve time, cache memory, cache
frames -- alongside feather-tk's own frame time, triangle, glyph and object
counts.

Three things it said on the first run, none of which anyone was going to notice
by looking at the application:

**The viewport colour buffer was full float.** `gl::offscreenColorDefault` is
`RGBA_F32` on desktop GL, sixteen bytes a pixel, and the viewport buffers are
the largest thing this application allocates. Now `RGBA_F16`: the viewport
blends additively so it does want headroom above one, but not four bytes a
channel to hold it, and §10 had already settled on half float for everything
that leaves the application. A 1600x1000 window on a retina display went from
52MB of viewport buffer to 26MB, with no visible difference.

**Four viewports cost the same as one.** 101MB of offscreen buffers in a
single-view layout, 100MB in a four-up. It scales with total pixel area, not
with viewport count, because four quarter-size buffers are one full-size one.
That is the reassurance the previous entry's design needed and did not have.

**A seventy frame re-simulation takes 3ms** at 549 particles. That is the number
that decides whether dragging a slider feels alive, and it is now on screen
instead of being a thing to wonder about.

### What the graphs do not do

`DiagSystem` samples every three seconds. The graphs are a trend over minutes,
not a meter -- a screenshot of them shows the readouts and empty plots, and
watching one to catch a hitch will not work. If a per-frame meter is ever wanted
that is a different widget, not a faster tick on this one: sampling every frame
would make the diagnostics part of what they are measuring.

---

## 2026-08-08 — Where the panels go (decided, not built)

Nothing was written for this. It is here because the answer shapes what the
curve editor can be, and because the reasoning is worth more now than it will be
after somebody has to re-derive it.

The question was whether panels should be panes that can hold either a view or
an editor, or their own thing in a column on the right.

**Both, split by shape.** Panes in the main region hold a view or an editor. The
parameters column on the right stays its own fixed thing and is not a pane type.

### Why the split

The panels §4a, §9, §10a and §12 call for are two different shapes, and trying
to make one mechanism serve both is what would go wrong.

*Viewport-shaped* — wants the big area, wants to be one of two or four: the
curve editor, the particle spreadsheet, the expression editor, diagnostic plots,
the compositor layer stack.

*Column-shaped* — wants to be visible **at the same time as** whatever else is
on screen: emitter, field and rule parameters, the systems list, render layers.

If panels only exist as a right-hand column, the curve editor has nowhere to go,
and it gets a bottom dock or a window of its own -- two panel systems instead of
one, which is the failure mode worth avoiding.

The other half is just as load bearing. Parameters must not compete for pane
space. Rate gets dialled while watching the sim; a value gets keyed while
looking at its curve. If parameters were a pane type, a curve editor layout
would cost the artist the parameters they were keying.

### It is mostly built already

`Views` holds a fixed number of persistent children, re-parents them into a tree
of splitters per arrangement, tracks a current one, and gives each a type
selector in its corner. That is a pane system. What is view-specific is the
array's type, `setPointSize` fanning out, and the corner menu listing view
types -- not the layout.

So the widening is a panel interface and a longer menu, not a docking framework,
and fixed arrangements with adjustable splitters survive it intact.

Two things do change:

- **Making them all up front stops working.** Four slots times several panel
  types is a lot of widgets, and some are expensive -- a spreadsheet over a
  million particles is not something to build and keep four of on the chance
  somebody looks at it. It becomes lazy creation per slot, cached: a slot builds
  a panel the first time it is asked for and keeps it.
- **Frame and Zoom only mean something to a viewport.** Rather than a menu bar
  that changes with the current pane, each panel gets its own controls in a
  header strip beside its type menu. A menu whose contents depend on which pane
  was last clicked is a menu nobody can learn.

### Scope boundary -- a hard rule

**Fixed arrangements only. No drag-to-split, no tear-off, no floating windows,
no user-saved layouts.**

Written down for the same reason §10a's boundary is: every one of those will
sound reasonable on the day it is asked for, and accepting them in order is how
a quarter goes into writing a window manager instead of a particle tool.

### Not yet

There is one panel type today, and §2a says to write the specific thing before
abstracting it. An interface with one implementation is a guess; the curve
editor is the second implementation and the first one that is genuinely not a
viewport, so it is what should pull the interface into existence.

Build the curve editor as a panel type, and let `Views` become the panel system
at that moment. Deciding the shape now is what keeps that cheap. Building it now
would be guessing at what the curve editor needs.

Until then the only thing worth protecting is that `Views` does not accumulate
viewport assumptions in its layout code.

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
