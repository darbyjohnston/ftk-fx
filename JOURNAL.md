# Design journal

What was decided and why. The reasoning is what evaporates first, so it is
written down here rather than left in the code, where only the conclusion fits.

Newest entries at the top.

---

## 2026-08-12 — The rest of the Maya picture, and a drag plane that is fine

The two remaining things the reference named, both on translate.

**Arrows instead of dots on the ends of the arms.** A dot says "this end"; an
arrow says which way. The manipulator that moves things is the one where that is
worth saying, and the three modes now differ at the ends in three ways -- arrow,
box, and the rings having no ends at all.

**A handle in the middle that slides in the plane of the screen.** This is the
one used most for rough placement, because it is the only one that does not make
you decide which axis you meant first.

### A drag plane, after taking one out

The axis drags used to solve against a plane and it was removed for being
edge-on at the worst moment: an axis pointing near the camera made the plane
nearly parallel to the ray, the intersection ran away, and the manipulator flew
across the viewport. The fix was to stop asking the scene where a pixel is.

This handle asks the scene where a pixel is, and that is right. Its plane is
square to the camera *by construction* -- it is the screen -- so every ray
through the viewport meets it near head on and the intersection is
well-conditioned everywhere. Holding the grabbed point under the pointer, the
thing that was unusable along an axis, is exactly what is wanted here.

Worth writing down because the shape of the code is the thing that was rejected,
and the reason it was rejected does not apply. The lesson was never "planes are
bad"; it was "a plane you cannot choose the angle of is bad".

### Reading the wrong number off my own sidecar

The check for this was authored from the arm tip in the sidecar, on the
assumption that an arm a hundred pixels long said something about how many
pixels a unit covers. It does not: the arm is drawn a fixed length whatever the
zoom, so the tip gives a direction and nothing else. The drag came out at 2.99
where 1.00 was expected, and the first guess was that the maths was wrong.

The maths was right -- z came back exactly 0.00, which is the property being
tested -- and the expectation was built on a number that was never there. The
sidecar now writes `pixelsPerUnit` beside each tip: 32.067 here, and 96 pixels
divided by it is 2.99, which is what the panel had been saying all along.

Second time in two days that the sidecar has been the fix for a guess. The
pattern is worth naming: when a check needs a number, the program should be made
to write that number down, not the nearest-looking one that is already there.

---

## 2026-08-12 — Three things from a picture of Maya, and a bug underneath them

A reference image of Maya's three manipulators, offered as "just for reference",
which turned out to name three gaps this one had:

**A handle in the middle of the scale manipulator**, which takes all three axes
at once. Three arms and no centre meant uniform scale had to be typed. It grows
by the same doubling the arms use, so the gesture is the one already learnt, and
it multiplies rather than assigns -- a scale that has been shaped into 3, 1, 1
stays that shape. Assigning one number to all three would be a different
operation wearing the same handle.

There is no arm for it to run along and no direction the scene can offer, so it
takes a fixed one: up and to the right grows. That is the one pair of directions
an artist will guess right without being told.

**A ring lying flat against the screen**, turning about the axis the viewer is
looking along -- which is the one turn the three axis rings cannot do, and
exactly the turn wanted when the emitter is being lined up against what is on
screen. Drawn a third again wider than the others so it reads as going round
them rather than as a fourth of the same kind.

**The far half of each ring faded.** A ring is a loop of one colour, and there
is nothing in it saying which way round it goes; a drag begun on the back of one
turns the opposite way to the hand and looks like a bug. Faded to a third rather
than dropped, because the back of a ring is still worth grabbing and a gap reads
as the ring being broken. It moves 3357 pixels, which is how it was checked.

Both new handles are drawn in a colour none of the three axes uses. Neither
belongs to an axis, and giving either one of the three colours would say it did.

### Which axis the view ring uses, since there are two candidates

The camera's forward axis, not the direction from the camera to the emitter.
They are the same thing only while the gizmo is in the middle of the viewport,
and the difference is what the ring is: forward keeps its plane parallel to the
image plane, so it is a circle lying *on the screen*; camera-to-emitter would
tilt it as the gizmo moved off-axis, making it a billboard facing the camera and
drawing it as an ellipse.

Forward is the right one because it is what the gesture is. Turning something in
the plane you are looking at means the ring should be the circle your hand
traces, and that circle does not care where in the window the thing is.

Measured with the emitter at 7, 3, 4, which puts the gizmo near the right edge:
the ring comes out 316 by 315 pixels, a ratio of 1.003. It stays exactly
circular anywhere, because a circle in a plane parallel to the image plane
projects through a perspective camera by a similarity -- position changes its
size and nothing else. The 0.3% is rasterisation.

### The dashes that were not dashes

The view ring first drew as alternating bright and faint dashes. The fade asks
whether a point is nearer the camera than the middle of its ring is -- and every
point of this ring is exactly as far away as the middle, so the measurement is
zero all the way round and its sign was whatever the last bit of arithmetic
happened to be.

It is now *said* to be near rather than measured. Worth writing down because the
first instinct was to widen the tolerance, which would have been a number chosen
to hide a question that has an exact answer.

### What it found on the way

Adding a fourth handle meant reading the press, and the press was opening the
undo edit at each handle that needed one -- so two of the four did not open one
at all. A rotate or scale drag left an undo entry per mouse move. Measured
before touching it: drag the scale arm to 2.0, press undo once, and the panel
reads **1.82**. It should read 1.00, and the translate drag next to it did.

This is the third bug of one shape on this manipulator. The undo entry said
"Move System" in every mode because the name was written at each place that
edits; the emitter grew along the wrong axis because the axis was decided at
each place that drew. Every one of them is the same sentence: something true of
the whole gesture was written down once per branch, and a branch was missed.

So the press is now `_gizmoPress`, which only answers whether it took the drag,
and the caller opens the edit. One place, and a fifth handle cannot forget.

`Arm` has two members that are not axes now. X, Y and Z stay first and in order
so that subtracting one still indexes the arms and their colours, and the places
that index say so instead of assuming it.

feather-tk gained rotation about an arbitrary axis, which the entry below said
would be wanted when this ring arrived. It was.

---

## 2026-08-12 — A ring that says world has to turn about world

The rings were drawn on the world's axes and each one wrote a single Euler
angle. Those are the same thing only while the other two angles are zero,
which is exactly the state every shot of the rotate manipulator had been taken
in. The entry below already called this out as a thing to fix; here it is.

Each angle is applied after the ones inside it, so `rotate.x` is a turn about
an axis the other two have already moved. Adding a drag to one of them is
therefore a turn about a local axis, whatever the ring it came from was drawn
on. The measurement is flat: start the emitter at a quarter turn about z, grab
the ring drawn about world y, and drag a quarter turn.

| | rotate x | rotate y | rotate z |
|---|---|---|---|
| writing one angle | 0.00 | -90.05 | 90.00 |
| composing | -90.05 | 0.00 | 90.00 |

Both are a quarter turn. Only the second is a quarter turn about world y --
the first turned it about the emitter's own y, which after that first quarter
turn is pointing along world *minus x*. The hand was on a ring at the top of
the screen and the emitter went round an axis at ninety degrees to it.

So the drag now builds the turn its ring stands for, puts it in front of the
orientation the press started from, and reads three angles back out of the
result. All three get written, because one turn about one world axis generally
is a change in all of them.

(The 0.05 is the press point rounding to whole pixels, the same 0.1% the scale
drag has.)

### Reading angles back out is where the sharp edges are

Two sets of angles describe any rotation, and each has more versions of itself
whole turns apart, so which one to return is a question with no derivable
answer -- it has to be asked. `eulerZYX` takes the angles to stay near, and a
drag passes what its last move wrote. That is what keeps a gesture round and
round winding to 360 instead of starting again at zero, and what stops a drag
crossing the fold at ninety degrees from taking the long way round.

It went next to `getRotation`, which is the thing it inverts, and it has a test
rather than a screenshot: round trips through nine poses, a full turn one
degree at a time and back, a sweep straight through the fold, and the two
straight-up poses where only the sum of the outer and inner turns is decided.

That test compares rotations through their matrices, never angles against
angles. Angles that differ are not rotations that differ, and a test that
misses this fails on correct answers -- which would have been the more
expensive mistake, because it would have been "fixed" by making the code wrong.

Both halves earned their place by being taken out: dropping the choice between
the two sets fails three checks, and dropping the winding fails two. Different
checks, which is the useful part -- they are guarding different things.

### What feather-tk did not have

`rotateX/Y/Z` and nothing else: no rotation about an arbitrary axis, and no way
back from a matrix to angles. The first did not bite here because the rings are
on world axes, so the three that exist are exactly the three needed -- it will
bite when the view-aligned ring arrives.

The second went to feather-tk as `getRotateXYZ`, with `rotateXYZ` alongside it
so that the order the angles are applied in is stated once rather than assumed
at both ends. The `nearAngles` argument is what makes it worth sharing: reading
angles back is not a derivation, and an interface that hides that behind a
single return value is one everybody has to work around. `Transform` keeps only
the two lines that are actually about a transform, and its test keeps only the
check that is about *this* project -- that scale is applied inside the rotation.

Every screenshot came back byte identical afterwards except the diagnostics
panel, which prints timings and has never matched itself between two runs. That
is the whole point of moving a thing that already worked: nothing should look
different, and if something does, the move was not a move.

Naming, which cost nothing here but would have cost a Windows build: the
argument is `nearAngles`, not `near`. `windows.h` still defines `near` and `far`
as macros, which is why `ortho()` next door says `nearClip`. There was a local
called `near` in the viewport's own distance-to-segment, from before anyone was
thinking about this; it is `nearest` now.

---

## 2026-08-12 — The scale arms were pointing at the wrong axes

Asked whether the manipulator ought to turn with the emitter -- if it is
rotated, should dragging move it along the world's axes or its own? The answer
turned out to be different for each of the three modes, and one of them was not
a preference at all.

The transform is `translate * rotate * scale`. The scale is applied *inside*
the rotation, so `scale.x` stretches the emitter along an axis it carries with
it. The arms were drawn along the world's. With no rotation the two coincide,
which is why this survived being built, measured and shipped: every shot of it
had the emitter square to the world. Turn the emitter and the arm pointed one
way while the thing grew another.

The translate arms stay world. Not an inconsistency -- the same rule read the
other way: the translation is added *outside* the rotation, so `translate.x`
really is a world distance, and world arms are what it moves along.

Measured by taking the fix back out, which is the only way a screenshot proves
anything: with the emitter turned 45 degrees, the scale gizmo moved 1646
viewport pixels with the fix and **zero** without. The translate gizmo moved
zero either way -- a control that came free, and the thing that says the change
is confined to the mode it was meant for.

### Arms that say where they are

The sharper measurement came from a new line in the capture sidecar: the
manipulator's origin and its three arm ends, in the same pixel space as the
widget boxes. Turned a quarter turn about z, the x arm's end lands on
(590.0, 715.06) -- to the last digit, where the y arm's end was. The z arm does
not move, which is what a rotation about z should do to it.

This exists because authoring a drag shot by reading coordinates off a picture
is guessing, and a guess that misses the arm still writes a perfectly good
screenshot. That is the same failure as the shot that had stopped asking, and
it has now cost enough afternoons to be worth a public accessor and eight lines
of JSON. Guessing at coordinates is the single most repeated mistake on this
project; this is the first change that removes the need to.

`manipulator-scale-local` is the regression: a quarter turn about z, then a
drag straight *up* the screen, which now runs along the x arm. It asserts
`Scale X` reads 2.00 and `Scale Y` reads 1.00. Before the fix that same gesture
was a Y drag.

### Still on paper

Rotate is untouched and still owes the fix described below: rings on world axes
writing Euler angles. Doing it properly means giving up writing one Euler
component per drag -- compose a turn about the grabbed axis onto the current
orientation and decompose back to ZYX -- and that is what makes any ring
orientation, world or local, tell the truth.

---

## 2026-08-12 — Rotate and scale, and a sign nobody should reason about

§15 said "rotate and scale next", and the journal added a condition: once the
drag had been used in anger. It has been, three bugs' worth, so here they are.

One manipulator with three modes rather than three manipulators. They share an
origin, a pick, a drag and a command, and differ only in what the drag means --
which is also why the mode lives on the viewport and is switched with W, E and
R rather than from a menu: it belongs to the viewport the pointer is over, and
a menu item would have to say which one it meant.

### Scale is the translate drag with a different meaning

The same three arms, the same run along the arm's line in pixels. What changes
is the last step: a doubling per arm's length rather than a distance. Growing
and shrinking become the same gesture in opposite directions, and a factor
built by doubling never reaches zero however far the drag goes back, which a
linear one does at exactly one arm's length in.

Measured: one arm's length out gives 1.9973, the same back gives 0.4995, and
out and back returns to exactly 1. The 0.13% is the press point being rounded
to whole pixels.

Square ends rather than round. Two modes draw the same three arms and do
different things with them, so the ends are the only place the difference can
show.

### Rings, sampled rather than solved

A circle seen at an angle is an ellipse, and a circle crossing behind the
camera is neither. Rather than work out which, the ring is sampled in the scene
at sixty-four points and each is projected: perspective comes out of the
projection, and the pieces behind the camera drop out by leaving their segments
unjoined. Picking is then the same distance-to-segment the arms already use, so
three great circles crossing four times over resolve the way two arms crossing
do -- nearest wins.

One radius for all three, taken from the arms' own pixels-per-unit. That is
what makes them read as one ball rather than three unrelated ovals.

### The sign, which is the whole entry

A turn on screen has to become a turn in the scene, and the direction depends
on the projection, the handedness, and which way the screen's y axis points.
Three chances to be wrong, and the failure is not subtle: the manipulator turns
the opposite way to the hand.

So it is not reasoned about. At the press, a point is walked a tenth of a turn
around the ring, projected, and the direction its bearing moved is the sign.
The projection is the thing that decides it, so the projection is what gets
asked. Edge on, where a tenth of a turn barely moves and its direction is
noise, the drag is refused rather than guessed.

That still left one link done on paper: that the ring's parameter is the same
angle as the Euler value it writes. So that got measured too, against
feather-tk's actual matrices:

| press | swept | result | what it does | agrees? |
|---|---|---|---|---|
| six o'clock, X ring | clockwise | `rotate.x` +24.8 | `Rx` tips +Y toward +Z, and +Z is down-right on screen, so the top goes right | yes |
| three o'clock, Z ring | clockwise | `rotate.z` -24.8 | `Rz` at a negative angle tips +X toward -Y, and +X sits at one o'clock, so it falls to three | yes |

Both follow the hand. Worth the twenty minutes: this is the third time on this
manipulator that something I could have argued for turned out to need checking,
and the first time it got checked before shipping rather than after a report.

The angle is added up move by move rather than taken from the press, each step
folded back into half a turn either side of the last. A drag past the far side
of the ring carries on turning instead of coming back as most of a turn the
other way, and a gesture round and round keeps winding: a full circle reads
360.00, and out and back returns to exactly zero.

### What these do not do yet

The rings are world axes, and they write Euler angles. Dragging the Y ring
writes `rotate.y`, which is a rotation about world Y only while the other two
are zero. That is what three Euler sliders in a panel already do, so the
manipulator is not lying about anything the rest of the interface is not -- but
a ring labelled with a world axis that turns about a local one once its
neighbours are non-zero is a thing to fix, either with local-axis rings or with
a real orientation. *(Fixed the same day: see the entry above.)*

No uniform scale: three axes, no centre handle. No screen-space ring either,
the outer one that turns about the view. *(Both added the same day, along with
fading the far half of each ring: see the entry above.)*

And still no gizmo toggle, which the four-up wants more than ever now that
there are rings in it.

---

## 2026-08-12 — Icons that can be opened in something that draws

They were string literals assembled at run time out of a wrapper function, a
handful of named rects and a concatenation per icon. Legible as code, and no
way to edit one as a drawing.

They are files now, compiled in by ftk-resource the way feather-tk and tlRender
carry theirs, so an installed ftk-fx is still one file. The reasoning that was
in the C++ comments went into the SVGs as XML comments, because it is what
somebody changing one of them wants to know and the file is where they will be.

The check worth keeping is the one the change invited: a faithful extraction
changes nothing on screen, so all thirty-nine shots were compared against the
run from before it. Thirty-eight identical to the byte. The thirty-ninth was
panel-diagnostics, and rather than wave that away it was captured twice in a
row -- two different hashes, because it draws live timing graphs. It differs
from itself, which is worth knowing about a shot that has been in the manifest
for a week.

---

## 2026-08-11 — Catching the branch up before it got expensive

`widget-api` had been running alongside feather-tk's and tlRender's mains for
long enough to be worth checking on: nine commits had landed on one and
twenty-five on the other, and the branch had never seen any of them.

Merged main *into* the branch rather than the branch into main, which is the
order that matters. The conflicts get resolved where the whole thing can be
built and tested together, rather than landing half-verified on main -- and
once it is done, the merge to main is a fast-forward, so the part that touches
everyone else's branch is the part with nothing in it.

feather-tk had exactly one conflict. Main had added a `getLineEdit()` accessor
to `FileEdit`; this branch had moved `FileEdit` onto `IContainer`, which meant
deleting the `getSizeHint` and `setGeometry` overrides main was still adding
around. Kept the accessor, dropped the overrides. tlRender had none at all --
its `deps/ftk` pointer looked like a conflict and was not, because this branch's
feather-tk now contains main's, so one side was an ancestor of the other and
git resolved it without being asked.

Verified after each merge rather than before: ninety-five feather-tk tests, ten
ctest, thirty-nine shots. That is the whole point of doing it on the branch --
the twenty-five commits that arrived from tlRender's main include FFmpeg pipe
changes, new comparison modes and timeline fixes, and "it merged cleanly" says
nothing about whether any of that still works here.

The reason to do this now rather than when it becomes urgent: the one conflict
was five lines. A month of the same drift and the same conflict is a rebase
nobody wants to start.

## 2026-08-11 — Test runs stay off the screen

The feather-tk suite makes dozens of applications and shows a window from most
of them. On a developer's machine they flicker past over whatever is being
worked on, and any one of them can be clicked into while a test is driving it.
That last part is the real problem: a stray click does not land in an idle
window, it lands in a test that is halfway through a gesture.

Everything needed was already there. `App` has an offscreen mode, applied to
each window as it is added, and the GL windows are created hidden -- `show()`
is what reveals them. Nothing had to change about how a window is made, only
about whether `show()` is honoured.

What was missing was a way to decide it *before any application exists*, which
is where the decision belongs: the process is a test runner, and it makes
dozens of applications rather than one. So `App::setOffscreenDefault()`, static
for that reason, and `ftk-test` sets it.

`-exit` implies it now as well. A run whose whole purpose is to start the
interface and stop has nobody to show a window to, and that flag is what the
examples registered as tests are run with -- seven more windows per `ctest`.

Verified by asking a window in a test whether it was offscreen rather than by
watching the screen and hoping: it says yes, ninety-five tests pass, and the
screenshot harness -- which depends on the same mechanism for the opposite
reason -- still captures all thirty-nine shots.

## 2026-08-11 — Driving a window without a person at it

Asked what feather-tk could add to make testing easier. The answer came out of
counting what had gone wrong that day rather than from taste, and it is four
things, of which one matters much more than the rest.

**A window that has not been laid out is a trap.** Until `_setSize` runs, every
widget's geometry is `0 0 -1 -1` and nothing is under the cursor. A test that
reads a geometry then reads nothing, and a test that aims at one hits nothing --
and neither *fails*. They pass, having checked nothing. That is exactly how a
splitter test of mine passed against code I had deliberately broken, and the
reason it is worth a named public method rather than a comment is that the
failure is silent. `layout(size)`.

**Everyone who needed to click grew their own window.** `ContextMenuTest`
subclassed `Window` to reach `_cursorPos` and `_mouseButton`; the shuttle test
did the same; ftk-fx's `MainWindow` did the same for its screenshot harness.
Three copies, slightly different. Now `click()`, `drag()` and `keyPress()` on
`IWindow`, and all three copies are gone.

Public rather than test-only, which was the right call for a reason I did not
anticipate: the third caller is not a test. An application that captures its own
screenshots needs to work its own interface, and hiding this behind a test
library would have left ftk-fx subclassing to get at it forever.

**Reaching inside a compound widget.** Aiming at a numeric editor is useless;
what has to be aimed at is the shuttle inside it. Both the test and the harness
had written their own recursive search. `findChild<T>()`.

**A box in a sidecar is not a thing on screen.** This is the one that cost the
most time: a widget scrolled past the bottom of a panel has a perfectly good
geometry, and reads exactly like a visible one. Four wrong guesses at drag
coordinates came from that.

It needed nothing new in feather-tk -- `IWidget::isClipped()` has been there all
along. The gap was that nothing *surfaced* it, so the sidecar now records
`"clipped": true`. The lesson is worth separating from the fix: the missing
thing was not a capability, it was the capability being visible at the moment
somebody needed it.

## 2026-08-11 — A dot instead of a border, and a shuttle that listens to the keyboard

**Third go at the current-editor mark.** A border round the whole editor, then
a bar along its top, now a filled dot at the start of the header. Each was
quieter than the last, and the reason the first two were wrong is the same: the
information is not "here is a region", it is "this one" -- and a region-sized
mark says the first thing however faint it is drawn. The header is where the
editor already says what it is showing. The dot is one more word in that
sentence, and it sits where the eye goes to read the rest of it.

**The shuttle takes modifiers.** A shuttle asks how fast, and how fast deserves
more than one answer. Control is the fine step, Shift the large one.

Shift rather than Alt, which is what was suggested: `DoubleEdit::scrollEvent`
and the sliders already use Shift for the large step, so this is one convention
in the toolkit instead of two. Alt is also the viewport's pan modifier, which
is a weaker argument -- different widget -- but it pointed the same way.

`ShuttleWidget` now keeps the modifiers of the drag and hands them out, read
from the move rather than the press so a key taken up part way through takes
effect from there. Additive; nothing that used the widget had to change.

Tested by dragging the same distance three ways and checking the three
distances differ. Two of those checks fail without the scaling -- which is
worth saying, because the last test written against a feather-tk widget passed
against broken code and the feature turned out not to be broken at all.

## 2026-08-11 — Shuttles for the transform, and a shot that had stopped asking

A slider maps a made-up span of scene units onto a track. For a rate that is
fine: it runs from none to a lot, and where in that it sits is information. For
a translation there is no span. The ends are invented, the artist drags past
them, and then the panel has to be taught not to lie about it -- which it was,
yesterday, and that should have been the clue.

So the transform rows shuttle instead. A shuttle asks how fast, not how far
along: drag it and the value moves at a rate, let go and it stops. Nothing has
to be decided about where the ends are, because there are none. The ranges stay
as clamps, wide enough not to be met by accident.

Measured: a hundred pixels of drag at a step of a tenth gives 0.80, one undo
takes it back, and the scene file has 0.8. Rotate steps by a degree, scale by a
hundredth.

The rows are a little sparse -- a shuttle is a fixed-size knob, so where a
slider filled the row there is now a gap before the key column. The key
diamonds still line up across every group, which is the alignment worth having.

### The part I got wrong first

Before writing any of it I decided `FloatEditShuttle` had a bug: its callback
is wired from the edit box, and the shuttle writes to the model directly, so
plainly a shuttle drag would never be reported. I fixed all three shuttle
widgets in feather-tk and wrote a test.

The test passed with the fix removed.

`FloatEdit` observes the model and fires its callback on *any* change,
including one the shuttle made. There was no bug. My fix would have double-fired
every callback in three widgets, into a library, on a premise I never checked --
and the only reason it did not is that I A/B'd the test out of habit rather than
suspicion.

Reverted. The test stayed: it drives a real shuttle drag through injected mouse
events, which nothing did before, and it is now the thing that would catch this
if the wiring ever did break. Ninety-five tests.

### A shot that had quietly stopped asking anything

`undo-slider-drag` drags two sliders and undoes one. Adding the System group and
the shuttles moved the panel's contents down, and the shot's coordinates --
written months of layout ago -- landed on nothing. It captured a perfectly good
picture of a scene nobody had edited, and passed, with a caption describing two
drags that never happened. Second time: `panes-four-dragged` did the same thing
when a splitter moved.

Twice is a pattern, so the harness now takes an `expect` block:

    "expect": { "Parameters.Rate": "1451", "Parameters.Speed": "6.00" }

Each entry names a tagged widget and a string its text must contain, checked
before the picture is written. Those two together cannot pass unless all three
steps landed: the rate moved and stayed, the speed moved and was undone -- and
had the second drag missed, the undo would have taken the rate back instead.
Verified by pointing the drag at empty space and watching the shot fail.

Finding the right coordinates took four wrong guesses, all of the same kind:
reading a position off a magnified crop instead of asking the program. The last
one was the good one -- the rows were at y=1206 in a window 1200 pixels tall,
scrolled below the bottom edge, and the sidecar records geometry for widgets
that are clipped just as happily as for widgets you can see. A box in the
sidecar does not mean a thing on screen.

## 2026-08-11 — What makes a list a list

The systems panel was a stack of widgets that happened to be arranged in rows.
It is now a list, and the difference is almost entirely in where the highlight
stops.

feather-tk already knows how: `ListItemButton` sets its button role to None and
fills its *whole geometry* with the checked colour -- nothing inset, nothing
rounded -- and `ListItemsWidget` stacks them with no spacing at all. A rounded
band with a margin around it says "this is a control". A band that runs corner
to corner says "this is a row of something". Same information, entirely
different object.

So `SystemRow` borrows the drawing and not the class. `ListWidget`'s items are a
string and a tooltip, and a system row has to carry controls -- one now, more
later -- so it needs to be a widget rather than a value. What is copied is the
order: the row's own colour, then whatever the pointer is doing to it, then the
contents on top.

The name is padded sideways and not vertically -- `Label` takes a role per
axis -- which is the same split feather-tk's list items make: the row's margin
is the gap to the panel edge, and the pad is the gap from the highlight's edge
to the text. One role for both would have made the rows taller to fix a
horizontal problem.

The check box moved to the right, which is where a second and third column will
go. It is a child of the row, so a press that lands on it never reaches the
row: ticking a system does not also select it. Verified both ways -- clicking a
name selected it and left the ticks alone, clicking a tick changed it and left
the selection alone.

### Two things that surfaced by moving it

**The panel divider was already there.** IPanel has drawn one under its header
since it was written. It was invisible: `Divider` draws in `Border`, and it sits
directly beneath the header's own lighter block, between two tones it is already
between. Proved by colouring it loudly for one build rather than by squinting --
the line appeared exactly where it should have been all along. It draws in
`Well` now, so it reads as the bottom edge of the title.

**The panel column can be narrower than its panels.** The rows are as wide as
the widest panel in the column, and the widest is Parameters. Put the check box
on the right and it lands in whatever the column cannot show; on the left,
nobody had ever noticed. `ScrollType::Vertical` looked like the fix and is the
opposite of one -- it clamps the content to *at least* the viewport and then
clips the overflow silently, where `Both` at least lets it be scrolled to.
Left on `Both`, which is where it started.

The underlying thing is a minimum width the panel column has and does not
declare. Worth fixing when something else needs it; today it means a column
dragged narrow hides the right-hand column of the list.

## 2026-08-11 — Three small ones, and what the first was really about

**The transform sliders stopped short of the emitter's.** They were not set to
expand, so each row was as wide as the widest thing in its own group -- and the
groups are separate `FormLayout`s, so Transform sized itself against Transform
and Emitter against Emitter. Two columns of controls that almost line up read
as a bug in the layout, which is what it was. Expanding, they all reach the
same edge.

Worth noting the shape of it: nothing was wrong inside either group. The defect
only existed *between* them, and only became visible once the window was wide
enough for the two answers to differ. A narrow window hides it completely.

**The tool bar groups needed air.** A divider with buttons hard against it on
both sides is one more thing in the row rather than a gap between groups. A
spacer either side, and the eye does the grouping without being told.

**The current editor was shouting.** A whole border in the accent colour, times
four editors, is a great deal of colour spent on one bit of information -- and
the information is not "here is a region", it is "this one". A bar along the
top edge says the same thing at a quarter of the volume, and it lands where the
header already is, which is where the eye goes to read what the editor is
showing.

## 2026-08-11 — The manipulator answers to the mouse, not to the scene

Reported again, in a different dress: from an emitter at X = 118, dragging the
X arm down and to the left sends it off the viewport by the time the mouse has
gone a third of the way. Accelerating away, again.

And the tracking was exact. Both are true, and the gap between them is the
whole entry.

### Keeping a point still is not the same as keeping the manipulator still

What the drag guaranteed was that *the point the pointer grabbed* stayed under
the pointer. What nobody asked for is the point the pointer grabbed. They asked
for the manipulator.

Those are different points. The grab lands wherever the pointer's ray meets the
drag plane, and when the axis is near edge-on -- which is what an emitter at
X = 118 looks like from a camera aimed at the origin -- a press twenty pixels
along the arm can be hundreds of units down the axis. That far point tracked
the mouse perfectly. The emitter, at a different place and a different depth,
translated by the same world vector, and crossed the viewport doing it.

So the model was wrong rather than the arithmetic. The manipulator is a control
and the mouse is what drives it, so the thing to hold fixed is the relationship
on screen:

> the emitter's own origin moves as many pixels along the arm's line as the
> pointer did.

A line in the scene projects to a line on screen, and a point sliding along one
slides along the other -- unevenly, but by a ratio of linear terms. So the
distance that lands the origin on a given pixel is a division, not a search:

    t = (ndc·w₀ − c₀) / (cₐ − ndc·wₐ)

Measured, dragging along the arm: pointer 49.65 pixels along the line, origin
49.65; pointer 249.633, origin 249.633. Locked, while the world distance behind
it runs 1.86, 5.12, 7.91, 10.28 -- perspective is still perspective, it just no
longer leaks into how fast the control moves.

The plane is gone with it. Nothing left to be edge-on to.

### The pole moved somewhere you can see

Every version of this has had a singularity. The nearest-point solve collapsed
where the ray aimed along the axis; the plane solve collapsed where the ray
swung parallel to the plane; this one collapses at the axis's *vanishing point*,
where every remaining point of the axis lands on one pixel. Dragging past it
flipped the sign, which is how it was found -- two shots that had read -69 and
-131 came back +348 and +597.

Same fix as before, and it fits better here: the side of the pole the drag
started on is recorded, and crossing it is refused. This time the pole is a
place on screen the artist can watch the arm converge towards, rather than an
angle to a plane nobody drew.

Orthographic views have no vanishing point at all -- `cₐ.w` is zero, the
denominator is constant, and none of this can happen. That is a good sign about
the formulation: the degeneracy is exactly where the geometry says it should
be, and nowhere else.

### A postscript on the rail

The rail vanished when the pointer went far outside the viewport. It was being
drawn from the arms as recomputed each frame, and an emitter carried far enough
down its axis projects a world unit to under a pixel -- at which point an arm
stops being drawable, and took the line with it.

Right for the arm, wrong for the line, and the reason is sitting in the solve:
the axis does not move while it is dragged along, the emitter slides down it.
That is exactly why the press records the line once and keeps it. Drawing the
rail from the arms was drawing a second thing that usually coincided with it.

Now it comes from the recorded line, so it survives the arms it belongs to.
Which turns out to be the most useful frame of all: the emitter gone off
screen, and the one line that says where it went and which way to drag it back.

### Three solves for one drag

Nearest-point between two lines, then a drag plane, now the projection. Not a
proud sequence, but each one was replaced for a reason found by measuring, and
the third is the first to be driven by the thing the artist is actually holding.
The lesson worth keeping is the one from the middle of it: *exact* and *right*
are different claims, and the first was true all three times.

## 2026-08-11 — A guard set too tight, and a constraint nobody could see

Two reports on the manipulator, and only one of them was a defect.

### The guard was mine

Yesterday's fix refused the drag once the cursor's ray swung within about
seventy degrees of the drag plane. Too eager: on a default scene the Z arm
stopped responding a hundred-odd pixels short of the viewport edge, which is
not an edge case, it is the middle of using the thing.

Seventy degrees was picked to also suppress a large jump near the limit. That
was the wrong thing to buy with it -- the jump is geometry, the wall is not.
Loosened to about eighty-five degrees, which still cannot reach the sign flip
that caused the reversal. Measured, on a new scene, from the true values rather
than the panel:

| drag ends | Z |
|---|---|
| short of the edge | -22.57 |
| at the edge | -55.53 |
| well outside the window | -131.60 |

Still responding all the way out, and still no reversal.

### The lag was not a defect, it was invisible

Reported alongside: dragging Z down and to the right works, but the emitter
"lags farther and farther behind the mouse". Measured, printing the cursor
against where the grabbed point lands on screen:

    along the arm    cursor 918,1035  ->  grabbed point 917.7,1035.6
    off the arm      cursor 918,718   ->  grabbed point 801.8,956.0

Along the arm it tracks to under a pixel. Off the arm it does not, and cannot:
the emitter is constrained to one line in the scene, so whatever the pointer
does at a right angle to that line has nowhere to go. Every manipulator in
every package behaves this way.

But *nothing said so*. The arm is ninety pixels long, and once the pointer is
past the end of it there is no line on screen to be off. So the correct
behaviour reads as a broken one -- and the reasonable conclusion, from the
outside, is that the manipulator is falling behind.

So the fix was not to the arithmetic, which was already exact. While an arm is
held, its line now carries on across the viewport, faint, in the arm's colour.
The rail is the explanation.

That is worth remembering as a category: a report of "this is wrong" can be
correct behaviour plus missing feedback, and the fix belongs in the feedback.
Checking which it was cost two print statements; assuming it was the maths
would have cost another rewrite of a solve that turned out to be right.

### Two things that had to exist to see any of this

**`dragHold`.** The harness released the button at the end of every drag, so no
shot could show what a gesture looks like *during* it. Anything drawn only
while dragging -- this rail, a future rotate ring -- was uncapturable. It now
takes a path and leaves the button down.

**A panel that stops lying.** Verifying the threshold, the panel read -50.00
for two different drags, because a slider clamps its display to its range and
the range is a guess about what is useful. The value was -55.53 and -131.60. I
only noticed because I went to the saved file for the real numbers. A range is
now opened up to hold whatever the value is and closed again when it comes
back, so the number on screen is the number in the scene. Flagged in the entry
below as still open; it took one afternoon to become the thing standing between
me and a measurement.

## 2026-08-11 — Dragging past the edge, and two wrong diagnoses first

Reported: drag the Z arm up and to the left to the edge of the window, keep
going outside it in another direction, and the emitter starts going backwards.

The instrumented drag says it plainly:

    pos=730,745    denom=0.744   t=3.20
    pos=173,259    denom=0.251   t=-64.88
    pos=-385,-227  denom=-0.129  t=+199.11

`t` is where the cursor's ray meets the plane the axis is dragged against.
`denom` is the ray against that plane's normal, and it *crosses zero*. Past
zero the ray meets the plane behind the camera, so `t` comes back with its sign
flipped -- the arm reverses while the cursor keeps going the same way.

### Two things I was sure of that were wrong

**First** I thought the nearest-point-between-two-lines solve was to blame --
its denominator is `1 - (axis·ray)²`, which collapses wherever the cursor
happens to aim along the axis. That is a real defect and I replaced it with the
plane solve. The numbers did not move. Both formulations were failing for the
same reason, one layer down.

**Second** I decided my ray was wrong: unprojecting a far-plane point should
misbehave once the cursor leaves the frustum, because `w` changes sign. So I
rebuilt the ray from the camera as a tangent, which is bounded at a right angle
and cannot flip. The new numbers matched the old ones to six figures. The
unprojection had been right all along.

Both rewrites were reasoning from a mechanism to a symptom without checking the
mechanism was running. The measurement -- printing `denom` alongside `t` --
would have named the cause before either rewrite, and it was three lines.

The tangent ray is kept: it is correct in a way the unprojection was only
accidentally correct, and it drops the matrix inverse.

### What the cause actually is

An axis that is nearly along the view has no good plane to drag against. Every
plane containing it is edge-on to the camera, so the cursor ray is nearly
parallel to the plane and a small movement means an enormous `t` -- and then
crosses it. This is not an off-window condition. In the reported view the
cursor reached it *inside* the viewport, which is why clamping the cursor to
the window would have fixed nothing.

So: the plane's normal is turned at the press to face the ray that grabbed the
arm, and the drag is refused once the ray swings within about seventy degrees
of the plane. The manipulator holds its last value rather than guessing. Held
and not clamped, deliberately -- a manipulator that stops when the cursor asks
for something it cannot answer is one the artist recovers from by moving back;
one that guesses is not.

Verified across every drag case: out-and-back returns to the value it started
on, the half-to-full ratio is unchanged at 2.34, the ortho views survive a drag
2260 pixels outside the window and back, and the reported gesture now stops
instead of reversing. `manipulator-edge` is the shot.

### Still open

- A near-view-parallel axis is a bad thing to drag along however it is solved,
  and the arm is still drawn at full length in that case, because "is this arm
  worth drawing" is a projected-length test and the projection of a unit is not
  small merely because the axis points away. Rotate and scale will meet this
  too.
- The parameters panel shows a slider's range, not the value: dragging past
  fifty units reads as fifty while the scene holds more. The panel is lying,
  quietly, and it should either widen or say so. *(Fixed the next day, after it
  got in the way of a measurement -- see above.)*

## 2026-08-11 — The manipulator was measuring against a scene it had moved

Reported: drag X or Z and the emitter speeds up, leaves the viewport, and does
not come back when the pointer turns round. The entry below claims the drag is
"measured from the press". It was -- the *pointer* was. The arm it was measured
against was recomputed every move, from wherever the emitter had got to.

That is a loop. Solve, move the emitter, and the arm the next solve uses is the
one the last solve moved. In perspective it compounds: pushing the emitter away
from the camera shrinks its pixels-per-unit, so the next equal nudge of the
mouse buys more world distance than the last. Hence *speeds up*.

Measured, because "it feels wrong" is not a number. Same direction, one drag
twice the length of the other:

| drag | old | new |
|---|---|---|
| half | -12.42 | -9.16 |
| full | -45.33 | -21.40 |

Old: twice the input, **3.65×** the output. New: **2.34×**, and that is not a
bug -- a receding axis really does cover more ground per pixel the further down
it you go. One is geometry; the other was the loop.

### What replaced it

Not "freeze the arm at the press", which would have stopped the runaway and
still not followed the cursor. The axis is a line in the scene, the cursor is a
line in the scene, and where they come nearest is a closed-form answer:

    t = (b·e - d) / (1 - b²)   where b = axis·ray, d = axis·w0, e = ray·w0

Take `t` at the press and `t` at each move, and the difference is the distance
to travel. The point that was grabbed stays under the pointer, which is what
was asked for, and it holds in perspective and orthographic alike because a
ray is a ray either way -- ortho just makes them all parallel. The determinant
goes to zero as the axis lines up with the view, which is the case where every
point on the axis is under the cursor at once and the honest answer is to
refuse.

`invert()` was already in feather-tk. I went looking for `inverse`, did not
find it, and nearly wrote a second one.

### The test that was not a test

The first thing I reached for was drag out, drag back, check it returned. Both
versions passed it. Of course they did: the old code took the pixel delta from
the press, so at the end of the return leg the delta is zero and the answer is
zero however wrong the scale in between was. A test that only samples the
endpoints cannot see a path.

What found it was picking a gesture whose *shape* the bug depended on -- along
the view rather than across it -- and then measuring the ratio rather than the
value. Two data points beat one, because the bug was in the slope.

Getting there needed the harness to drag through more than two points, which
it now does; `manipulator-back` is the shot that would have caught this.

### And File/New

Also reported, also small: New kept the playhead where it was. Now both New and
Open go to the start of the new range. They both throw the old scene away
entirely, and frame 60 of a scene that no longer exists says nothing about the
one that just arrived -- a fresh scene opening on an empty frame 60 reads as a
fresh scene that is broken.

## 2026-08-11 — A manipulator that lives in two dimensions

Translate arms on the current system's emitter, grabbed and dragged in the
viewport. §15 has wanted this since Phase 2 was written and gated it on there
being more than one thing to move, which there now is.

### The projection does the work twice

The arms are drawn in pixels, not in the scene. Project the emitter origin,
project the origin plus one unit along each axis, and the difference is both
things a manipulator needs at once:

- **normalised**, it is the direction to draw the arm in;
- **its length**, it is how many pixels a world unit covers along that axis.

So the drag is `dot(mouseDelta, dir) / pixelsPerUnit` and there is no second
derivation of the camera to disagree with the first. A gizmo whose arm points
one way and whose drag goes another is the classic version of this bug, and it
cannot happen when both come out of the same subtraction.

Constant screen size falls out for free, which is what a manipulator wants: it
is a control, and controls do not get smaller as you back away from them.

Two things the same maths gives without asking:

- An arm pointing at the camera projects to almost nothing. Guarded at one
  pixel, which drops it from both the drawing and the picking -- in a front
  view the Z arm simply is not there, rather than being a dot you can grab and
  then send the emitter to infinity with.
- Perspective needs `w > 0` checked, or a point behind the camera projects to
  somewhere perfectly plausible on screen, mirrored. Orthographic never does
  this, which is exactly why it would have been found late.

### Measured from the press, not the last move

The drag sets the value from the *whole gesture* -- press position to now --
rather than adding each move's delta. Accumulating would drift, and worse, it
would have each move re-measuring the arm against a scene the previous move had
already moved. Reading the start once and treating the rest as an offset is
what makes the gesture idempotent.

### What it cost elsewhere

Nothing, which is the point. `beginEdit`/`endEdit` and `systemChanged` were
already there for the sliders, so a whole 8-step drag arrives as one undo step
without the manipulator knowing how undo works. The one thing it did need was
the rule for setting a value at a frame -- key it when animated, set the
constant when not -- which the parameters panel had inline. The manipulator
would have been the third copy, so it moved to `setValue()` in ParameterList
next to the list everything else already shares.

Verified by dragging each arm in turn and reading the panel out of the sidecar:
X moved X and left Y and Z alone, Y moved Y, a drag in empty space orbited the
camera and moved nothing. One undo took a dragged value all the way back.
Dragging an animated transform left the keys at 1 and 80 where they were and
put the dragged value at 40.

### Still open

- **No toggle.** The arms are always drawn, in every viewport, on top of the
  particles -- which in the four-up shot is four of them at the origin, inside
  the plume. The corner tripod has wanted a toggle for a while and now there
  are two overlays wanting the same switch.
- **Translate only.** Rotate and scale are the same picking with different
  maths, and worth doing once the drag has been used in anger.
- **No object picking.** Which system the manipulator is on comes from the
  systems list, not from clicking a particle. That is the piece Phase 2 still
  owes, and it is a different problem: the arms are three known segments, and a
  system is a cloud.

## 2026-08-11 — Panes became editors, and systems stayed systems

Two naming questions, three days after the words started mattering.

### A mnemonic is not a fix

`Pane` and `Panel` were one letter apart, and the entry below from 2026-08-08
already knew it -- it wrote them down side by side and settled the matter with
a mnemonic: *panes are arranged, panels are stacked*. The mnemonic was correct
and it did not work. Three days later the screenshot tags were
`MainWindow.Pane0` and `MainWindow.Panel.Systems`, one character apart in a
file a shot is read out of, and it still took someone else looking at it to
say so.

Which is the lesson worth keeping: when the answer to two confusable names is a
rule for telling them apart, the rule is the smell. Rename one.

So the main region holds **editors** and the right hand column holds
**panels**. Both words were individually right -- split panes and docked panels
are what everyone calls them -- and that is exactly why neither looked wrong
when it was written. The problem was never either name, it was the pair.

"Editor" also happens to describe the thing better than "pane" did. A pane is
a hole in a window; an editor is what the artist is doing in it. It matches
Blender's areas-hold-editors model, which is what this already was: a header
menu that picks what the region shows. And `Editor` holding a `CurveEditor` is
not a collision, it is a specialisation -- a curve editor is a kind of editor,
the way a Graph Editor is.

`Views` became `Panes` became `Editors`. Two renames of the same thing is not
a good record, but the first one was widening the concept and this one was
separating a collision, and both were cheaper the day they happened than the
day after.

The old entries below still say "pane". They are a record of what was decided
when, and rewriting them would make the entry that chose the word "Panes"
describe a choice nobody made.

### Systems stayed systems

The other candidate was renaming `System`, on the grounds that feather-tk has
systems too. It does, and they are `ftk::ISystem` in another namespace and
never reach the artist; the only real friction is `context->getSystem<T>()`
sitting a few lines from `model->getSystem()`.

Kept, because every alternative collides with something this design has
already spent:

- **sim** is the namespace, and "the sim" is the whole scene's simulation --
  the cache and the transport are about that, not about one system.
- **fx** is the project and the outer namespace.
- **layer** belongs to the compositor. §16 has an open question literally
  called "layer caching granularity" which is *about* systems; the two have to
  stay different words for that question to be askable.
- **emitter** is a part of a system, and §6 wants several per system.

And "particle system" is what Maya, Houdini, Blender and every game engine call
it. A name an artist already has is worth more than a name that is merely
unambiguous inside this repository.

## 2026-08-11 — More than one system, and what a pointer into one costs

Manipulators are what Phase 2 wants next, and §15 gates them on "more than one
thing to move". So: a scene holds a list of systems, and the systems panel is
the selection -- every other panel edits the current one, the viewport draws
all of them.

Three decisions worth the words.

**One pool per system, one frame for the lot.** `core::Frame` split into
`SystemFrame` (a pool and its emitted count) and a `Frame` holding one per
system. `System::step` takes and returns a `SystemFrame`, so the solver did not
learn that systems exist. The cache still holds one entry per frame, which
means editing one system re-simulates all of them. That is the coarse answer
and it is on purpose: §16 still has "layer caching granularity" open, and a
cache per system would have answered it by accident, in the dark, while doing
something else.

**The file needed nothing.** It already wrote `"systems"` as an array with one
entry in it -- a note in the old code says a file that has to grow an array
later is a file every reader has to learn twice. Reading all of the entries
instead of the first was the change. Two years of that habit is worth one
afternoon of it.

**Order is part of the scene.** Two systems the other way round is a different
scene, and the test says so. It has to be: the list is the order they are
solved in, and it will be the order they composite in.

### The pointer problem, which was the real work

`ParameterInfo` holds a `core::Parameter*` into a system, and the panels hold
those between rebuilds. The old comment said they were safe because the
model's system "is a member rather than something that can be replaced". Make
it a `std::vector<System>` and that sentence stops being true twice over: a
resize moves the elements, and `applyState` -- which runs on every step of a
slider drag -- assigned a whole new vector over the old one.

The fix is not shared pointers by themselves; it is *assigning into the systems
that are already there*. `_systems` is a `vector<shared_ptr<System>>`, and
`_setSystems` copies each value into the existing object rather than replacing
it. The list can grow and shrink without moving the systems that stay, and a
parameter edit leaves every address alone.

That in turn is what let `applyState` keep the cheap path: it raises
`sceneChanged` only when the *count* changed. A drag still raises nothing but
`parameterChanged`, and the panels still refresh rather than rebuild -- which
is the split those two observables were separated for in the first place.

The undo state became the whole list plus the current index. Adding and
removing a system are then not new commands, they are ordinary state changes,
which is the second time whole-state undo has paid for the copying it does.
Selection is in the state deliberately: undoing a removal brings the system
back *and* selects it. The cost is that undoing a parameter edit also jumps the
selection to where that edit was made, which is a fair description of where
undo has just taken you.

### A panel that never expected to be rebuilt

The parameters panel builds its rows from the current system, so choosing
another one has to rebuild it. It did -- and the screen went on showing the
first build, with a name and a rate belonging to no system at all.

`IPanel::_setContent()` was one line: `value->setParent(_p->panelLayout)`. It
never took away what was there. Nothing had ever called it twice. So the panel
held every build it had ever made, stacked, and each refresh wrote to the
newest one while the oldest was the one on screen.

Worth noticing how it presented: the *model* was right, the systems list was
right, and the debug print said `name=sparks` five times while the screenshot
said `particles`. Everything that reads state agreed; only the pixels
disagreed. That is the same shape as the shader bug -- and it is the second
time the answer has been that a sidecar reads state, so a shot has to be looked
at.

### Still open

- **Columns in the list.** A row is a checkbox and a name today because those
  are the only two things a system has that belong in a list. As more of them
  arrive -- a solo, a lock, a particle count, a colour swatch -- the row wants
  to be a grid with headings rather than a hand-packed horizontal layout, and
  the widget it wants is closer to feather-tk's `ListItemsWidget` than to what
  is here. Worth doing when there is a third column, not before: the shape of
  the columns is a guess until something has to fit in one.
- **Copy and paste of systems.** Deliberately not what the duplicate button
  does, and the icon says so: feather-tk's `Copy` is a clipboard, so duplicate
  got its own two-sheets glyph and `Copy` stays free for the day a system can
  be put on a clipboard and pasted into another scene. That day the clipboard
  needs a format, which is very nearly the scene file's `systems` array
  already.
- Renaming happens in the parameters panel rather than in the list. Double
  click to rename in place is what an artist would reach for; it needs a widget
  swap in a row, and this did not.
- Every system's particles go into one buffer and one draw call. Fine while
  they share a draw type; per-system display settings would end that.
- A disabled system keeps whatever particles it had at the moment it was
  disabled -- except that any edit invalidates from the head of the range, so
  in practice it re-simulates to empty. That is luck rather than design, and it
  will stop being true when invalidation gets finer.

## 2026-08-09 — Half a handle outside the splitter, and a test suite nobody was running

Drag a pane divider all the way to the top and its border line is drawn
across the bottom of the tool bar. Pre-existing, in feather-tk rather than
here, and the arithmetic says why: the handle straddles the split, so at a
split of zero half of it sits above the splitter's own geometry. Nothing
clips it, so it lands on the neighbour. The collapsing child was handed a
negative height on the way.

Clamped in pixels, in `Splitter::_split()`, rather than clamping the split
fraction. Two reasons. A fraction's legal range would be `half / size`,
which changes with the window, so `setSplit(0)` would mean different things
at different sizes and a saved layout would not restore. And clamping the
pixel offset still collapses a child to nothing -- at the limit the handle
is flush against the edge with zero left beyond it -- so nothing is taken
away. `Splitter2D` had the same expression twice, once per axis.

### The part worth writing down

The first three times I ran the new test it passed against deliberately
broken code.

The first two were the same mistake as the `TLRENDER_PROGRAMS` one: this
build tree had `ftk_TESTS=OFF` in its cache from before `local.cmake` set
it on, and a `-C` default does not overwrite a cache value that is already
there. So there was no `ftk-test` target, `cmake --build --target ftk-test`
failed, and the stale binary from some earlier configure ran happily. The
tell was there and I filtered it out: I piped make through `grep -i error`,
and what make actually said was "No rule to make target".

The third was mine. In a test app the window never lays anything out, so
every widget's geometry is `0 0 -1 -1` and the assertions were reading
nothing. `FlowLayoutTest` had already solved this -- call `setGeometry()`
on the widget under test and read the children back -- which is the pattern
now used here.

So: a test is not evidence until it has failed once on purpose. All three
of these looked exactly like a passing test.

`ftk_TESTS` is on in this tree now. The suite is 94 tests and passes; the
GL and PNG tests log errors under a headless run, which they did before any
of this.

## 2026-08-09 — A scroll bar past the bottom of its pane

Reported from a four-pane layout: the curve editor's channel list ended part
way down its column, and its scroll bar carried on past the bottom of the pane
and over the transport.

One symptom, two causes, and they need separate fixes.

The scroll widget was not expanding, so the vertical layout gave it the height
it asked for -- the height of the twenty-odd channels it holds. In a tall pane
that is more than the column has, so it overflowed; in a short one the layout
squeezed it and the list stopped early. `setVStretch(Stretch::Expanding)` makes
it take what is there and scroll the rest, which is what a scroll widget is for.

That alone is enough in every layout I could build, which is the trap. The pane
had no reason to believe it: nothing stopped a child from drawing outside it,
so the first arrangement that overfilled a pane put pixels over its neighbour.
`setClipChildren(true)` on the pane is the guarantee. A pane is a bounded
region of the window and its contents belong inside it, whatever the content is
and whoever writes it next.

Verified by taking the same shot four ways -- neither fix, each alone, both.
Neither: the scroll bar runs 75 pixels past the pane. Clip alone: it stops at
the edge, and the list is still wrong. Stretch alone: correct here, and only
here. Both: the list fills its column and the horizontal scroll bar it needs
appears inside the pane rather than under the transport.

Worth remembering that the manifest's existing two-up curves shot showed
neither symptom. It took a deliberately short pane to make the bug appear at
all, which is the same lesson as the shader: the shot has to be built to fail.

## 2026-08-09 — Two ways to read a vertical axis

The curve editor drew every channel against one range, which is right when the
channels are comparable and useless when they are not: a gravity curve between
-30 and -2 is a flat line along the bottom of a plot that goes to 1800. Both
curves are there and only one of them can be read.

§4a already had the answer -- "normalized and absolute value views" -- and both
are worth having, for different questions. Absolute answers "which of these is
bigger". Normalized answers "what shape is this one", which is the question
being asked when a curve is being edited.

So the plot keeps a range per channel as well as the shared one, and the mode
picks which is used. Two consequences worth writing down:

The zero line is only drawn in absolute mode. Normalized, each curve has its
own zero at its own height, and a single line across the plot would be a
statement about all of them that is true of none.

Dragging goes back through the same range it came from. That falls out of
passing the channel to the mapping rather than reading a member, and it is the
part that would have been wrong if the range had stayed global -- a key would
have jumped to wherever the shared range put it.

Verified by dragging a gravity key in normalized mode and reading the panel:
-30 became -11.03, in gravity's units rather than the rate curve's.

Still missing from §4a's list: axis labels, framing, box selection, tangent
handles. All additions.
---

## 2026-08-09 — Transforms, and a shot that captured nothing

### The emitter has a transform

Translate, rotate and scale, built from Parameters like everything else, so a
transform is animatable by construction rather than by a later retrofit. That is
§4a's argument for the Parameter type, applied to the thing every object will
need.

It replaces a bare position and a bare direction. The spray goes along the
emitter's own up axis, so turning the emitter turns what comes out of it --
which is what turning an emitter means, and which the two separate fields could
not express. Nothing was lost: the direction was the only orientation there
was.

It exists now, with one emitter, because everything after this needs it.
Multiple objects each need one, and a thing with no transform has nowhere to
put a manipulator.

### Renaming into a shader

"Point size" now sizes a sphere as much as a disc, so it became "particle
size" -- in the label, the model, the panes and the capture step, but not in
the shader, where the uniform feeds `gl_PointSize` and really is a point size.

The rename was a search and replace over the file, then a revert of the three
strings that belong to the shader. It missed a fourth: the vertex shader
declared `uniform float pointSize` and assigned `gl_PointSize = particleSize`.
The shader failed to compile, the exception was caught and logged, and the
viewport drew nothing at all -- no particles, no grid.

Twenty-eight shots captured. All twenty-eight succeeded. One of them is the
screenshot at the top of the README, which I regenerated and committed
completely black.

### The harness now listens to the log

This is the third time a silent visual failure has got past the sidecar, and
the second time the reason was that the sidecar reads state and the breakage
was in pixels. There is no general fix for that. There is a specific one that
would have caught all three in this case and costs nothing: the application
*told* us. It logged the shader error, every frame, and nothing was listening.

A shot now fails if the application logs an error while it is being set up or
drawn. Verified by breaking the shader deliberately: the shot fails instead of
capturing a black picture of a working-looking window.

That does not replace looking at the pictures. It does mean the failures that
announce themselves stop being silent.
---

## 2026-08-09 — The end of phase 1

Two things left on §15's list, and reading it again changed what one of them
was.

### An emitter with a shape, not two emitters

I had said the volume emitter would make the emitter an interface with two
implementations, which is the boundary nothing had tested. §6 does not say
that. The volumetric primitives -- sphere, box, cone, disc, cylinder -- are one
bullet, and they differ only in where inside them a point is. A point is a
sphere of no size. So it is one emitter with a shape, and the interface waits
for the kinds that genuinely differ: curve, geometry, texture driven,
secondary.

Radii rather than a radius, so a sphere is an ellipsoid when they differ. That
costs nothing and a flattened emitter is immediately useful.

Two distribution details that are easy to get wrong and hard to see:

A sphere's radius is the cube root of a uniform. A plain uniform radius crowds
the centre, because the shells further out have more room in them.

A box's surface picks a face weighted by area. The first version picked an axis
uniformly, which puts a third of the particles on each pair of faces however
big they are -- so a flat box came out with a bright rim, its thin sides
getting as many particles as its broad faces. Visible from directly above as a
dense border, and not visible at all from anywhere else.

Both are keyed on the particle's id like every other draw, so a re-simulation
puts each particle back where it was.

### Spheres, which are the first step of the distance fields

§10 describes blobs as raymarched distance fields, and a shaded sphere as the
same trick one step earlier. That is what this is: the disc the viewport
already cuts out of a point is the silhouette of a unit sphere, so the surface
normal falls out of where in the disc the pixel is. Lit from the camera with a
little wrap. No geometry, no light in the scene, no depth pass.

It is the difference between a plume that reads as volume and one that reads as
confetti, for about ten lines of shader.

The `round` uniform became `drawType`, because there are three answers now and
two of them are not booleans -- flat for the grid, a disc for a point, a shaded
disc for a sphere.

---

## 2026-08-09 — ActionGroup

The layout menu offering to un-tick the current arrangement turned out to be
three instances in this application, not one: the pane type and view type menus
had it too, created the same way and compensated for the same way. tlRender has
it twice, in two files, for the same three playback actions. feather-tk's own
objview example has it. Darby wrote the toolkit and was caught out by it while
looking for an example to point me at.

That is the signature of a missing concept rather than a set of mistakes.

An action could say it was a command or a switch. One of many had no way to be
said, so everyone said it the same way by accident: create the actions with a
plain callback, so picking one cannot un-pick it, and write an observer that
walks the set calling `setChecked(this one == the current one)`. It works, it
leaves `isCheckable()` false while `isChecked()` is true, and the mutual
exclusion lives in every application that wants it.

`ActionCheckType` -- None, Check, Radio -- lets the action say what it is.
`ActionGroup` owns the rest, deliberately shaped like `ButtonGroup` so there is
nothing new to learn.

### Why it could not be ButtonGroup

Two reasons, and the second is the interesting one. A menu makes its own
buttons, so the application never sees them and cannot put them in a group. And
one action drives several buttons -- the layout actions are menu items and tool
bar buttons at the same time. Relatedness belongs where identity is, and
identity is the action.

### Watching rather than intercepting

The first cut had the group install its own callbacks on each action, which is
wrong: an action's callbacks are given when it is made and belong to whoever
made it. The group would have thrown away the application's own lambda.

It observes `checked` instead, which needs nothing from `Action` that was not
already there. Picking the action that is already current arrives as a request
to turn it off -- the button toggled it on the way in -- and the group puts it
straight back. That is the one line the whole thing exists for, and the test
fails without it.

### What it removed

Three loops here, two in tlRender, one in objview. Each becomes one call
saying which index is current.

### The tool bar was not talking to the actions

Then the layout buttons on the tool bar could be un-checked, and pressing one
switched the layout without checking it, while the same actions in the menu
behaved. The difference was in feather-tk and older than any of this: a menu
button writes a click back to the action and then reports it, and a tool button
only reported it. So an action shown in both had a checked state that only one
of them could change, and anything watching the action heard nothing from the
tool bar.

Why it looked like "activates but does not check": `IButton::click()` runs the
clicked callback before it toggles. The callback set the layout, which came
back through the model and checked the action, which checked the button -- and
then the button flipped itself the other way and told nobody. The menu path
survived it because writing back gave the group a chance to put it right.

This is the second bug in two days that only exists where two widgets are
driven by one object, and the first was the same shape: `MenuBar` renaming a
menu whose button had cached its glyphs. Shared state needs both sides to agree
about who writes it, and "one of them forgot" does not announce itself.

### Built is not the same as compiled

The sweep reached into feather-tk's examples and tlRender's play application,
neither of which this configuration builds -- `ftk_EXAMPLES` and
`TLRENDER_PROGRAMS` are off, because nothing here needs them. So the edits
compiled in the sense that the build went green, and had never been near a
compiler. One of them was missing an include and would not have built at all.

Turning the options on is a line in `etc/Config/local.cmake`, which is exactly
what that file is for. The rule is the boring one: switch on whatever the
change touches before making the change, not after.

---

## 2026-08-09 — Icons, a tool bar, and a shot that had stopped testing

The menu actions carry feather-tk's icons where it has one -- FileNew, FileOpen,
FileSave, Undo, Redo, and the ViewFrame and ViewZoom pair the camera actions
already used. Four are new and belong to this application rather than to the
toolkit: the pane arrangements, drawn as the arrangements themselves, so the
button shows what it does rather than naming it.

The tool bar holds the same `Action` objects the menus do. `ToolButton` takes an
action's icon, tooltip, checkable, checked and enabled state -- but not its text
-- so a tool bar built from actions is icon-only for free and cannot drift out
of step with the menu. Undo greys out in both places because there is only one
place. The layout buttons show which arrangement is current for the same reason.

Two things worth remembering. A `ToolBar` is an `IContainer`, so a divider
between groups has to go through `addWidget` rather than being parented to the
tool bar; parented directly it is never given a geometry and simply does not
appear. And the tool bar is built after the menus, because that is where the
actions come from, then moved to the top of the window with `moveToBack`.

### One of many, without a checkbox

The layout actions were created with a checked callback, which makes them
checkable -- so the menu offered to un-tick the current arrangement, which is
not a thing that can happen. The code compensated by ignoring `false` and
letting an observer put the tick straight back, which works and reads as though
un-ticking were meaningful.

Darby pointed at tlRender's playback actions, which have the same shape --
stop, forward and reverse are one-of-three -- and solve it by not making the
actions checkable at all. A plain callback means picking one always means "use
this" and can never mean "stop using this"; the tick is still drawn, driven from
what the model actually is. The layout actions do that now.

It is a workaround rather than an answer, and feather-tk already has the word
for what is wanted: `ButtonGroupType::Radio`. Actions have `checkable` and
nothing else, so every application that wants a radio group in a menu has to
know that "not checkable, but call setChecked on it" is the way. That is worth
fixing in the toolkit rather than in each application that trips over it.

### The shot that had stopped testing

Adding a tool bar pushed everything below the menu bar down by fifty-eight
pixels, which invalidated every click and drag coordinate in the manifest.
Shifting them was mechanical. Checking them afterwards was not, and it turned up
that `panes-four-dragged` had not been dragging anything for several commits:
its pane boxes were identical to the shot that does no drag at all.

It broke when the panel column was widened from .78 to .66. That moved the
splitter crossing from x=780 to x=656, and the drag had been starting at 776 --
which was the crossing when the coordinates were chosen. Nothing failed. The
shot captured a picture of nothing having happened, with the caption "Both
divisions moved at once" still underneath it.

That is the failure mode of a coordinate in a manifest: it does not go wrong, it
goes quiet. The harness has caught several real bugs precisely because it
asserts on what the sidecar says rather than on how the picture looks, and this
is the one place where that discipline was not applied to the harness itself.
The coordinates are re-derived from a sidecar now, and the notes say to do that
after anything moves. Making the steps relative to a tagged widget would remove
the class entirely and is the obvious next thing.

---

## 2026-08-09 — Frame was not framing

"Frame" set the camera to a fixed position -- centre at (0, 5, 0), thirty units
back -- and never looked at the particles. It was a reset dressed as a fit, and
it happened to look right in the first week because the default emitter puts its
plume roughly where that camera points.

It measures the alive particles now, and the two projections want different
answers:

**Orthographic views get the exact fit.** The eight corners of the box go
through the same rotation the camera uses and the extent is read off the result.
A plume seen from above is wide and shallow, and a fit that allowed for its
height would zoom out to make room for something not on screen.

**The perspective view gets a sphere around the box**, which is looser and
should be. That view orbits, and a box that just fits seen face on does not
when seen corner on -- a fit that changes as the camera moves is worse than one
that is slightly generous.

Framing no longer resets the orientation. That was the old behaviour by
accident rather than by intent, and every other application treats framing as
"fit what is there from where I am standing". Getting back to the default
orientation is a separate thing and does not exist yet.

The harness grew a `frameView` step, which is the only way to reach it: it is a
menu action, and the shots that need it need it applied to every pane.

### Where Frame and Zoom live

They were in the Layout menu, which is a leftover: that menu was called View
until it collided with the panes' own View menus, and the camera actions came
along with the rename. Layout is how the panes are arranged; framing moves a
camera in one of them.

They belong in the pane's own menu, which is where a 3D application would put
them and where "which pane?" would answer itself. That is not available:
`ftk::MainWindow` dispatches shortcuts through its own menu bar only, so an
action in a pane's menu bar would have no key attached. Frame would lose
Backspace for the sake of being in the tidier place, which is a bad trade.

So a Camera menu at window level. Not "View" -- that word is taken by the pane
menus, and reintroducing it is the collision that started this. The menus are
inserted rather than appended while here, so the framework's Window menu sits
at the end instead of in the middle of the application's own.

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

### What setPressedCallback actually is

The bracketing above was wrong, and wrong in a way that only a slider drag
showed: after dragging one, undo went grey and stayed grey for the rest of the
session. Typing the same value in the number edit beside it worked perfectly,
which is the clue -- typing never touches the press callback at all.

`setPressedCallback` is not a press and a release. It is fired from the value
observer on every change, carrying `_isMousePressed()` alongside the new value,
and once more on release. So treating it as a pair called `beginEdit()` once per
mouse move and `endEdit()` once, and the depth counter never came back to zero.
Everything after that was applied and nothing was recorded.

Two changes. The count became a flag, because the callers are widget callbacks
that make no promise to balance and an unmatched open under a count kills undo
silently. And the edit is opened from the *value* callback rather than the press
one: the observer reports the value before it reports the press state, so a
drag's opening change has already happened by the time anything says a drag has
begun -- which put the first undo in the middle of the gesture rather than
before it. Opening on the value change and closing when a change arrives with
the mouse up covers a typed value and a drag with the same two lines.

Sliders are tagged for the sidecar now, so a shot can drag the real widget
instead of guessing where it is. Two drags, then one undo: Speed returns and
Rate stays moved.

### Point size

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
