// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/Core/Cache.h>
#include <fx/Sim/Scene.h>

#include <filesystem>

#include <ftk/Core/Command.h>
#include <ftk/Core/Observable.h>
#include <ftk/Core/ObservableList.h>
#include <ftk/Core/Range.h>

namespace ftk
{
    class Context;
    class Timer;
}

namespace fx
{
    namespace app
    {
        //! The scene: a system, the cache of its simulated frames, and the
        //! playhead.
        //!
        //! Everything the user interface does goes through here. The widgets
        //! observe it and never talk to the solver or the cache themselves,
        //! which keeps "what happens when a parameter changes" in one place
        //! rather than spread across the panels that change them.
        class SceneModel : public std::enable_shared_from_this<SceneModel>
        {
        protected:
            void _init(const std::shared_ptr<ftk::Context>&);

            SceneModel() = default;

        public:
            ~SceneModel();

            static std::shared_ptr<SceneModel> create(
                const std::shared_ptr<ftk::Context>&);

            //! \name File
            ///@{

            //! Get the scene: the recipe and the time it runs over, without
            //! the playhead or the cache.
            sim::Scene getScene() const;

            //! Replace the scene, throwing away everything simulated from the
            //! old one.
            void setScene(const sim::Scene&);

            //! Start again from a default scene, with no path.
            void newScene();

            //! Read a scene. Throws, leaving the current scene untouched:
            //! failing to open a file should not cost the artist the one they
            //! already had.
            void open(const std::filesystem::path&);

            //! Write the scene, and take the path as the current one.
            void save(const std::filesystem::path&);

            //! Get the path the scene was last opened from or saved to. Empty
            //! for a scene that has never been saved.
            const std::filesystem::path& getPath() const;
            std::shared_ptr<ftk::IObservable<std::filesystem::path> > observePath() const;

            //! Observe wholesale replacement of the scene -- New, Open, or
            //! anything else that changes every value at once.
            //!
            //! Separate from parameterChanged(), which the panels raise
            //! themselves: a panel refreshing on its own edits would fight the
            //! drag that caused them.
            std::shared_ptr<ftk::IObservable<int> > observeSceneChanged() const;

            //! Observe edits to the parameters, as a counter that goes up.
            //!
            //! Raised by parameterChanged(), so it fires on every step of a
            //! drag: anything watching it has to be cheap or has to compare
            //! before it acts.
            std::shared_ptr<ftk::IObservable<int> > observeParameterChanged() const;

            //! Get whether the scene differs from what is on disk. Compared
            //! rather than flagged, so putting a value back where it was is
            //! not a change.
            bool isModified() const;
            std::shared_ptr<ftk::IObservable<bool> > observeModified() const;

            ///@}

            //! \name System
            ///@{

            const sim::System& getSystem() const;

            //! Get the system for editing. Call parameterChanged() afterwards;
            //! the model cannot see the edit for itself.
            sim::System& getSystem();

            //! Record an edit that has already been made to the system, and
            //! throw away the simulation the old values produced.
            //!
            //! The caller mutates the system through getSystem() and then hands
            //! back what it looked like beforehand; the two together are the
            //! undo step. Editing in place and reporting afterwards, rather
            //! than describing the edit and asking for it, is what lets one
            //! command cover every kind of change -- a constant, a key, a seed
            //! -- without a command class per operation.
            //!
            //! Inside beginEdit()/endEdit() the change is applied but not
            //! recorded, so a drag is one undo step rather than one per mouse
            //! move.
            //!
            //! The cache is invalidated from the start of the range rather than
            //! from the playhead: a constant that changed was never only in
            //! effect from here on, and pretending otherwise would give a cache
            //! that no run from the start could reproduce. Only the frames
            //! actually looked at are re-simulated.
            void systemChanged(
                const std::string& name,
                const sim::System& before);

            ///@}

            //! \name Undo
            ///@{

            //! Open an edit that several changes will contribute to.
            //!
            //! A drag is one thing the artist did, however many events it took
            //! to do it. Bracketed explicitly rather than guessed at from what
            //! the values look like: two edits that happen to leave the system
            //! in the same state are not necessarily the same edit.
            //!
            //! Opening one that is already open does nothing, and closing one
            //! that is not open does nothing. A flag rather than a count, and
            //! deliberately: the callers are widget callbacks that do not
            //! promise to balance, and an unmatched open under a count leaves
            //! the edit never closing and undo silently dead from then on.
            void beginEdit();

            //! Close the edit and record it, unless nothing actually changed.
            void endEdit(const std::string& name);

            std::shared_ptr<ftk::IObservable<bool> > observeHasUndo() const;
            std::shared_ptr<ftk::IObservable<bool> > observeHasRedo() const;
            void undo();
            void redo();

            //! Replace everything an edit can touch and re-simulate, without
            //! recording anything. For the command stack; everything else
            //! wants systemChanged() or setPointSize().
            void applyState(const sim::System&, float pointSize);

            ///@}

            //! \name Display
            ///@{

            //! How big the particles draw, in points.
            //!
            //! Not part of the scene file yet, though §10's render presets say
            //! it eventually should be. It is on the undo stack regardless: it
            //! sits in the same panel as the parameters, with the same slider,
            //! and nobody is going to remember which sliders undo.
            float getPointSize() const;
            std::shared_ptr<ftk::IObservable<float> > observePointSize() const;
            void setPointSize(float);

            ///@}

            //! \name Time
            ///@{

            const ftk::RangeI& getRange() const;
            std::shared_ptr<ftk::IObservable<ftk::RangeI> > observeRange() const;
            void setRange(const ftk::RangeI&);

            double getFrameRate() const;

            int getCurrentFrame() const;
            std::shared_ptr<ftk::IObservable<int> > observeCurrentFrame() const;
            void setCurrentFrame(int);

            void frameStart();
            void frameEnd();
            void framePrev();
            void frameNext();

            bool isPlaying() const;
            std::shared_ptr<ftk::IObservable<bool> > observePlaying() const;
            void setPlaying(bool);

            ///@}

            //! \name Frames
            ///@{

            //! Observe the simulated frame at the playhead.
            std::shared_ptr<ftk::IObservable<std::shared_ptr<const core::Frame> > >
                observeFrame() const;

            //! Observe the state of every frame in the range.
            std::shared_ptr<ftk::IObservableList<core::FrameState> >
                observeCacheStates() const;

            //! Observe the cache size in bytes.
            std::shared_ptr<ftk::IObservable<size_t> > observeCacheByteCount() const;

            //! Freeze or release the frame at the playhead.
            void setCurrentLocked(bool);

            //! Get how long the last call to simulate took, in milliseconds.
            //! Zero when the playhead landed on a frame the cache already had,
            //! which is the number worth seeing: it says the scrub was free.
            //! How long the last step through _frameUpdate() took, in
            //! microseconds.
            int64_t getSimTime() const;

            //! Get the number of particles at the playhead.
            size_t getParticleCount() const;

            //! Get the number of frames the cache is holding.
            size_t getCachedFrameCount() const;

            ///@}

        private:
            //! Simulate whatever is needed to have the given frame, starting
            //! from the last frame the cache still holds.
            void _simulate(int frame);

            //! Publish the cache state and size.
            void _cacheUpdate();

            //! Compare the scene against what was last read or written.
            void _modifiedUpdate();

            //! Apply what has already been changed, and put it on the stack
            //! unless an edit is open around it.
            void _record(
                const std::string& name,
                const sim::System& beforeSystem,
                float beforePointSize);

            std::shared_ptr<ftk::Context> _context;
            sim::System _system;
            core::Cache _cache;
            double _frameRate = 24.0;

            std::shared_ptr<ftk::Observable<ftk::RangeI> > _range;
            std::shared_ptr<ftk::Observable<int> > _currentFrame;
            std::shared_ptr<ftk::Observable<bool> > _playing;
            std::shared_ptr<ftk::Observable<std::shared_ptr<const core::Frame> > > _frame;
            std::shared_ptr<ftk::ObservableList<core::FrameState> > _cacheStates;
            std::shared_ptr<ftk::Observable<size_t> > _cacheByteCount;
            std::shared_ptr<ftk::Observable<std::filesystem::path> > _path;
            std::shared_ptr<ftk::Observable<bool> > _modified;
            std::shared_ptr<ftk::Observable<int> > _sceneChanged;
            std::shared_ptr<ftk::Observable<int> > _parameterChanged;
            std::shared_ptr<ftk::Observable<float> > _pointSize;
            std::shared_ptr<ftk::CommandStack> _commands;

            //! The command at the top of the stack, when it is still the one
            //! this model pushed. Held so that a drag can extend it instead of
            //! stacking a command per mouse move; cleared by undo and redo,
            //! after which it is no longer the top.
            std::shared_ptr<ftk::ICommand> _lastCommand;

            //! The system as it was when the outermost beginEdit() ran, and
            //! how many are open. Counted rather than flagged so that a widget
            //! bracketing an edit that another widget already bracketed does
            //! not close it early.
            sim::System _editBefore;
            float _editBeforePointSize = 3.F;
            bool _editOpen = false;

            //! The scene as it is on disk, which is what "modified" is
            //! measured against.
            sim::Scene _saved;
            std::shared_ptr<ftk::Timer> _playbackTimer;
            int64_t _simTime = 0;
        };
    }
}
