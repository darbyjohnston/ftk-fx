// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/CoreTest/SystemTest.h>

#include <fx/Sim/System.h>

#include <ftk/Core/Format.h>

using namespace fx::core;
using namespace fx::sim;

namespace fx
{
    namespace core_test
    {
        namespace
        {
            const double frameRate = 24.0;

            //! Run a system over a frame range, starting from the given frame.
            SystemFrame runFrom(const System& system, SystemFrame frame, int first, int last)
            {
                for (int i = first; i <= last; ++i)
                {
                    frame = system.step(frame, i, frameRate);
                }
                return frame;
            }

            //! Run a system from nothing up to the given frame.
            SystemFrame runTo(const System& system, int last)
            {
                return runFrom(system, SystemFrame(), 1, last);
            }

            //! A system with the variance turned off, for the tests that want
            //! to predict a number rather than a range.
            System plainSystem()
            {
                System out;
                out.getEmitter().rate.setConstant(24.F);
                out.getEmitter().spread.setConstant(0.F);
                out.getEmitter().speed.setConstant(0.F);
                out.getEmitter().speedVariance.setConstant(0.F);
                out.getEmitter().lifespan.setConstant(1000.F);
                out.getEmitter().lifespanVariance.setConstant(0.F);
                out.getForces().drag.setConstant(0.F);
                return out;
            }
        }

        SystemTest::SystemTest(const std::shared_ptr<ftk::Context>& context) :
            ITest(context, "fx::core_test::SystemTest")
        {}

        SystemTest::~SystemTest()
        {}

        std::shared_ptr<SystemTest> SystemTest::create(
            const std::shared_ptr<ftk::Context>& context)
        {
            return std::shared_ptr<SystemTest>(new SystemTest(context));
        }

        void SystemTest::run()
        {
            _emission();
            _gravity();
            _lifespan();
            _determinism();
            _resume();
        }

        bool SystemTest::_equal(const SystemFrame& a, const SystemFrame& b)
        {
            if (a.emitted != b.emitted)
                return false;
            const Pool& pa = a.pool;
            const Pool& pb = b.pool;
            if (pa.size() != pb.size())
                return false;
            for (size_t i = 0; i < pa.size(); ++i)
            {
                if (pa.id[i] != pb.id[i] ||
                    pa.position[i] != pb.position[i] ||
                    pa.velocity[i] != pb.velocity[i] ||
                    pa.age[i] != pb.age[i] ||
                    pa.lifespan[i] != pb.lifespan[i])
                {
                    return false;
                }
            }
            return true;
        }

        void SystemTest::_emission()
        {
            // One particle per frame at 24 per second and 24 frames per second.
            System system = plainSystem();
            const SystemFrame frame = runTo(system, 10);
            if (10 != frame.pool.size())
            {
                _fail(ftk::Format("Emission: expected 10 particles, got {0}").
                    arg(frame.pool.size()));
            }

            // A rate that is not a whole number of particles per frame still
            // comes out at that rate over time rather than rounding down to
            // nothing every frame.
            System fractional = plainSystem();
            fractional.getEmitter().rate.setConstant(12.F);
            FTK_CHECK(50 == runTo(fractional, 100).pool.size());

            // The substep count changes how the frame is solved, not how much
            // is emitted.
            System substepped = plainSystem();
            substepped.setSubsteps(4);
            FTK_CHECK(10 == runTo(substepped, 10).pool.size());

            // A disabled system does nothing at all.
            System disabled = plainSystem();
            disabled.setEnabled(false);
            FTK_CHECK(0 == runTo(disabled, 10).pool.size());
        }

        void SystemTest::_gravity()
        {
            System system = plainSystem();
            system.getEmitter().rate.setConstant(24.F);
            const SystemFrame frame = runTo(system, 24);

            // A second of falling from rest under 9.8 puts the oldest particle
            // about five units down and moving at about that speed. Euler is
            // not exact, so this checks the sign and the order of magnitude,
            // which is what would break if the integration were wrong.
            const ftk::V3F& position = frame.pool.position[0];
            const ftk::V3F& velocity = frame.pool.velocity[0];
            FTK_CHECK(position.y < -4.F && position.y > -6.F);
            FTK_CHECK(velocity.y < -9.F && velocity.y > -11.F);
            FTK_CHECK(0.F == position.x);
            FTK_CHECK(0.F == position.z);

            // Drag pulls the speed back.
            System dragged = plainSystem();
            dragged.getForces().drag.setConstant(2.F);
            const SystemFrame draggedFrame = runTo(dragged, 24);
            FTK_CHECK(draggedFrame.pool.velocity[0].y > velocity.y);

            // Gravity is a parameter like anything else, so it animates.
            System animated = plainSystem();
            Curve curve;
            Key a;
            a.frame = 1.0;
            a.value = 0.F;
            a.interp = Interp::Linear;
            Key b;
            b.frame = 24.0;
            b.value = -20.F;
            b.interp = Interp::Linear;
            curve.addKey(a);
            curve.addKey(b);
            animated.getForces().gravity.y.setCurve(curve);
            FTK_CHECK(runTo(animated, 24).pool.position[0].y > position.y);
        }

        void SystemTest::_lifespan()
        {
            System system = plainSystem();
            system.getEmitter().lifespan.setConstant(1.F);

            // One particle a frame for a second, each living a second: the
            // count settles rather than climbing.
            FTK_CHECK(12 == runTo(system, 12).pool.size());
            const size_t settled = runTo(system, 48).pool.size();
            if (settled > 25)
            {
                _fail(ftk::Format("Lifespan: {0} particles still alive").
                    arg(settled));
            }

            // Nothing outlives its lifespan.
            const SystemFrame frame = runTo(system, 48);
            for (size_t i = 0; i < frame.pool.size(); ++i)
            {
                FTK_CHECK(frame.pool.age[i] < frame.pool.lifespan[i]);
                FTK_CHECK(frame.pool.alive[i]);
            }
        }

        void SystemTest::_determinism()
        {
            System system;
            system.getEmitter().seed = 7;
            const SystemFrame a = runTo(system, 30);
            const SystemFrame b = runTo(system, 30);
            if (!_equal(a, b))
            {
                _fail("Determinism: the same system ran twice gave two answers");
            }
            FTK_CHECK(a.pool.size() > 0);

            // The seed is the handle an artist dials, so it has to change the
            // result.
            System reseeded;
            reseeded.getEmitter().seed = 8;
            FTK_CHECK(!_equal(a, runTo(reseeded, 30)));
        }

        void SystemTest::_resume()
        {
            // Running straight through and resuming from a frame part way
            // through must give the same answer, or the cache is lying: every
            // scrub and every re-simulation depends on this.
            System system;
            system.getEmitter().seed = 3;
            system.setSubsteps(2);

            const SystemFrame whole = runTo(system, 40);

            const SystemFrame partial = runFrom(system, runTo(system, 17), 18, 40);

            if (!_equal(whole, partial))
            {
                _fail(ftk::Format(
                    "Resume: running 1-40 gave {0} particles, "
                    "1-17 then 18-40 gave {1}").
                    arg(whole.pool.size()).
                    arg(partial.pool.size()));
            }
        }
    }
}
