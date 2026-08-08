// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/Core/Cache.h>
#include <fx/Sim/System.h>

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

            //! \name System
            ///@{

            const sim::System& getSystem() const;

            //! Get the system for editing. Call parameterChanged() afterwards;
            //! the model cannot see the edit for itself.
            sim::System& getSystem();

            //! Throw away the simulation the old parameters produced.
            //!
            //! Everything from the start of the range, not from the playhead: a
            //! constant that changed was never only in effect from here on, and
            //! pretending otherwise would give a cache that no run from the
            //! start could reproduce. Only the frames actually looked at are
            //! re-simulated, so this is cheaper than it sounds.
            void parameterChanged();

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
            std::shared_ptr<ftk::Timer> _playbackTimer;
            int64_t _simTime = 0;
        };
    }
}
