// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/CoreTest/PoolTest.h>

#include <fx/Core/Pool.h>

using namespace fx::core;

namespace fx
{
    namespace core_test
    {
        PoolTest::PoolTest(const std::shared_ptr<ftk::Context>& context) :
            ITest(context, "fx::core_test::PoolTest")
        {}

        PoolTest::~PoolTest()
        {}

        std::shared_ptr<PoolTest> PoolTest::create(
            const std::shared_ptr<ftk::Context>& context)
        {
            return std::shared_ptr<PoolTest>(new PoolTest(context));
        }

        void PoolTest::run()
        {
            _birthAndDeath();
            _compact();
            _ids();
        }

        void PoolTest::_birthAndDeath()
        {
            Pool pool;
            FTK_CHECK(0 == pool.size());
            FTK_CHECK(0 == pool.getAliveCount());

            const size_t index = pool.birth(4);
            FTK_CHECK(0 == index);
            FTK_CHECK(4 == pool.size());
            FTK_CHECK(4 == pool.getAliveCount());
            FTK_CHECK(4 == pool.position.size());
            FTK_CHECK(4 == pool.velocity.size());
            FTK_CHECK(4 == pool.age.size());
            FTK_CHECK(4 == pool.lifespan.size());
            FTK_CHECK(4 == pool.alive.size());

            // Killing tombstones without moving anything, so an index taken
            // before the kill still points at the same particle.
            pool.position[3] = ftk::V3F(1.F, 2.F, 3.F);
            pool.kill(1);
            FTK_CHECK(4 == pool.size());
            FTK_CHECK(3 == pool.getAliveCount());
            FTK_CHECK(!pool.alive[1]);
            FTK_CHECK(ftk::V3F(1.F, 2.F, 3.F) == pool.position[3]);

            // Killing twice must not count twice.
            pool.kill(1);
            FTK_CHECK(3 == pool.getAliveCount());

            pool.kill(100);
            FTK_CHECK(3 == pool.getAliveCount());

            pool.clear();
            FTK_CHECK(0 == pool.size());
            FTK_CHECK(0 == pool.getAliveCount());
            FTK_CHECK(0 == pool.getEmittedCount());
        }

        void PoolTest::_compact()
        {
            Pool pool;
            pool.birth(5);
            for (size_t i = 0; i < 5; ++i)
            {
                pool.position[i] = ftk::V3F(static_cast<float>(i), 0.F, 0.F);
            }
            pool.kill(0);
            pool.kill(2);
            pool.kill(4);
            pool.compact();

            FTK_CHECK(2 == pool.size());
            FTK_CHECK(2 == pool.getAliveCount());
            // The survivors keep their order, so ids stay sorted and a frame
            // can be compared against the one before it index by index.
            FTK_CHECK(2 == pool.id[0]);
            FTK_CHECK(4 == pool.id[1]);
            FTK_CHECK(1.F == pool.position[0].x);
            FTK_CHECK(3.F == pool.position[1].x);

            // Compacting a pool with nothing to drop leaves it alone.
            pool.compact();
            FTK_CHECK(2 == pool.size());
        }

        void PoolTest::_ids()
        {
            Pool pool;
            pool.birth(3);
            FTK_CHECK(1 == pool.id[0]);
            FTK_CHECK(3 == pool.id[2]);
            FTK_CHECK(3 == pool.getEmittedCount());

            // An id is never reused, including across a compaction: it is what
            // a re-simulation and a manual edit both use to mean "this
            // particle", and reuse would silently point them at another one.
            pool.kill(0);
            pool.kill(1);
            pool.kill(2);
            pool.compact();
            pool.birth(2);
            FTK_CHECK(4 == pool.id[0]);
            FTK_CHECK(5 == pool.id[1]);
            FTK_CHECK(5 == pool.getEmittedCount());
        }
    }
}
