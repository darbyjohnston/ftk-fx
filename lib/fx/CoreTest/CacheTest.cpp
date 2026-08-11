// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/CoreTest/CacheTest.h>

#include <fx/Core/Cache.h>

using namespace fx::core;

namespace fx
{
    namespace core_test
    {
        namespace
        {
            //! A frame holding the given number of particles, so that a test
            //! can tell one frame from another and the byte count means
            //! something.
            std::shared_ptr<const Frame> makeFrame(size_t count)
            {
                auto out = std::make_shared<Frame>();
                out->systems.resize(1);
                out->systems[0].pool.birth(count);
                out->systems[0].emitted = static_cast<double>(count);
                return out;
            }
        }

        CacheTest::CacheTest(const std::shared_ptr<ftk::Context>& context) :
            ITest(context, "fx::core_test::CacheTest")
        {}

        CacheTest::~CacheTest()
        {}

        std::shared_ptr<CacheTest> CacheTest::create(
            const std::shared_ptr<ftk::Context>& context)
        {
            return std::shared_ptr<CacheTest>(new CacheTest(context));
        }

        void CacheTest::run()
        {
            _frames();
            _invalidate();
            _lock();
            _range();
            _budget();
        }

        void CacheTest::_frames()
        {
            Cache cache;
            cache.setRange(ftk::RangeI(1, 10));
            FTK_CHECK(ftk::RangeI(1, 10) == cache.getRange());
            FTK_CHECK(10 == cache.getStates().size());
            FTK_CHECK(FrameState::Empty == cache.getState(1));
            FTK_CHECK(!cache.get(1));
            FTK_CHECK(!cache.getLastValid(10));

            cache.set(3, makeFrame(7));
            FTK_CHECK(FrameState::Simulated == cache.getState(3));
            FTK_CHECK(cache.get(3));
            FTK_CHECK(7 == cache.get(3)->getParticleCount());
            FTK_CHECK(cache.getByteCount() > 0);

            // The last valid frame is where a re-simulation starts.
            FTK_CHECK(3 == cache.getLastValid(10));
            FTK_CHECK(3 == cache.getLastValid(3));
            FTK_CHECK(!cache.getLastValid(2));

            // Frames outside the range are ignored rather than crashing.
            cache.set(100, makeFrame(1));
            FTK_CHECK(FrameState::Empty == cache.getState(100));
            FTK_CHECK(!cache.get(0));
        }

        void CacheTest::_invalidate()
        {
            Cache cache;
            cache.setRange(ftk::RangeI(1, 10));
            for (int frame = 1; frame <= 10; ++frame)
            {
                cache.set(frame, makeFrame(1));
            }

            // Forward only: an edit at frame 5 cannot change what frames 1 to 4
            // already are.
            cache.invalidateFrom(5);
            for (int frame = 1; frame <= 4; ++frame)
            {
                FTK_CHECK(FrameState::Simulated == cache.getState(frame));
            }
            for (int frame = 5; frame <= 10; ++frame)
            {
                FTK_CHECK(FrameState::Empty == cache.getState(frame));
            }
            FTK_CHECK(4 == cache.getLastValid(10));
        }

        void CacheTest::_lock()
        {
            Cache cache;
            cache.setRange(ftk::RangeI(1, 10));

            // There is nothing to freeze about a frame that has not been
            // simulated, so locking an empty frame does nothing.
            cache.setLocked(1, true);
            FTK_CHECK(FrameState::Empty == cache.getState(1));

            for (int frame = 1; frame <= 10; ++frame)
            {
                cache.set(frame, makeFrame(1));
            }
            cache.setLocked(ftk::RangeI(4, 6), true);
            FTK_CHECK(FrameState::Locked == cache.getState(4));
            FTK_CHECK(FrameState::Locked == cache.getState(6));
            FTK_CHECK(FrameState::Simulated == cache.getState(7));

            // A locked frame survives invalidation, and a re-simulation running
            // over it cannot overwrite it.
            cache.invalidateFrom(1);
            FTK_CHECK(FrameState::Empty == cache.getState(3));
            FTK_CHECK(FrameState::Locked == cache.getState(5));
            cache.set(5, makeFrame(99));
            FTK_CHECK(1 == cache.get(5)->getParticleCount());

            cache.setLocked(5, false);
            cache.set(5, makeFrame(99));
            FTK_CHECK(99 == cache.get(5)->getParticleCount());

            cache.clear();
            FTK_CHECK(FrameState::Empty == cache.getState(4));
            FTK_CHECK(0 == cache.getByteCount());
        }

        void CacheTest::_range()
        {
            Cache cache;
            cache.setRange(ftk::RangeI(1, 10));
            for (int frame = 1; frame <= 10; ++frame)
            {
                cache.set(frame, makeFrame(frame));
            }
            const size_t byteCount = cache.getByteCount();

            // Frames the two ranges share are kept: changing the range should
            // not throw away work that is still good.
            cache.setRange(ftk::RangeI(5, 20));
            FTK_CHECK(16 == cache.getStates().size());
            FTK_CHECK(FrameState::Simulated == cache.getState(5));
            FTK_CHECK(5 == cache.get(5)->getParticleCount());
            FTK_CHECK(10 == cache.get(10)->getParticleCount());
            FTK_CHECK(FrameState::Empty == cache.getState(11));
            FTK_CHECK(cache.getByteCount() < byteCount);
            FTK_CHECK(cache.getByteCount() > 0);
        }

        void CacheTest::_budget()
        {
            Cache cache;
            cache.setRange(ftk::RangeI(1, 10));
            for (int frame = 1; frame <= 10; ++frame)
            {
                cache.set(frame, makeFrame(100));
            }
            cache.setLocked(1, true);

            // Half the frames fit. The ones nearest the playhead stay, and the
            // locked frame stays whether it is near or not.
            cache.setMemoryBudget(cache.getByteCount() / 2);
            cache.evict(8);
            FTK_CHECK(cache.getByteCount() <= cache.getMemoryBudget());
            FTK_CHECK(FrameState::Locked == cache.getState(1));
            FTK_CHECK(FrameState::Simulated == cache.getState(8));
            FTK_CHECK(FrameState::Empty == cache.getState(2));

            // A budget smaller than the locked frames is exceeded rather than
            // enforced: locked means locked.
            cache.setMemoryBudget(0);
            cache.evict(8);
            FTK_CHECK(FrameState::Locked == cache.getState(1));
            FTK_CHECK(cache.getByteCount() > 0);
        }
    }
}
