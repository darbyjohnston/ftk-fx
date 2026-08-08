// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/Core/Frame.h>

#include <ftk/Core/Range.h>

#include <memory>
#include <optional>
#include <vector>

namespace fx
{
    namespace core
    {
        //! What the cache holds for a frame.
        enum class FrameState
        {
            //! Nothing; the frame has to be simulated to be shown.
            Empty,

            //! A simulated frame, which an upstream edit may throw away.
            Simulated,

            //! A simulated frame the artist has frozen. Locked frames survive
            //! invalidation and eviction; this is how an approved section of a
            //! shot is kept.
            Locked,

            Count,
            First = Empty
        };

        //! The frame cache.
        //!
        //! One simulated frame per frame of the range. Everything downstream --
        //! scrubbing, playback, the cache bar, re-simulation -- is a question
        //! asked of this class, which is why it is worth getting right before
        //! there is much of a simulation to cache.
        //!
        //! Frames are held by shared pointer and handed out as const, so that
        //! showing a frame costs nothing, a frame still being drawn survives
        //! being evicted, and nothing downstream can edit history.
        class Cache
        {
        public:
            //! Get the cached range.
            const ftk::RangeI& getRange() const;

            //! Set the cached range. Frames outside it are dropped, and the
            //! frames the old and new ranges share are kept.
            void setRange(const ftk::RangeI&);

            //! \name Frames
            ///@{

            FrameState getState(int frame) const;

            //! Get the state of every frame in the range, for the cache bar.
            std::vector<FrameState> getStates() const;

            //! Get a frame, or null when it is empty.
            std::shared_ptr<const Frame> get(int frame) const;

            //! Store a simulated frame. A locked frame is left alone, so a
            //! re-simulation running over one cannot overwrite it.
            void set(int frame, const std::shared_ptr<const Frame>&);

            //! Get the last frame at or before the given frame that holds a
            //! state -- where a re-simulation has to start from.
            std::optional<int> getLastValid(int frame) const;

            ///@}

            //! \name Locking
            ///@{

            void setLocked(int frame, bool);
            void setLocked(const ftk::RangeI&, bool);

            ///@}

            //! Drop the frames from the given frame forward, leaving the locked
            //! ones. Invalidation is forward-only: an edit made at frame 40
            //! cannot change what frames 1 to 39 already are.
            void invalidateFrom(int frame);

            //! Drop everything, including locked frames.
            void clear();

            //! \name Memory
            ///@{

            size_t getByteCount() const;

            size_t getMemoryBudget() const;
            void setMemoryBudget(size_t);

            //! Drop simulated frames until the cache is inside its budget,
            //! furthest from the playhead first. Locked frames are never
            //! dropped, so a budget smaller than the locked frames is simply
            //! exceeded rather than enforced.
            void evict(int playhead);

            ///@}

        private:
            struct Entry
            {
                FrameState state = FrameState::Empty;
                std::shared_ptr<const Frame> frame;
            };

            //! Get the index into _entries, or -1 when the frame is outside the
            //! range.
            int _index(int frame) const;

            void _drop(Entry&);

            ftk::RangeI _range = ftk::RangeI(1, 100);
            std::vector<Entry> _entries = std::vector<Entry>(100);
            size_t _byteCount = 0;
            size_t _memoryBudget = 1024 * 1024 * 1024;
        };
    }
}
