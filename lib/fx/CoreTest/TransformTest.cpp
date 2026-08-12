// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/CoreTest/TransformTest.h>

#include <fx/Sim/Transform.h>

#include <ftk/Core/Format.h>

#include <cmath>

using namespace ftk;
using namespace fx::sim;

namespace fx
{
    namespace core_test
    {
        namespace
        {
            //! Whether two rotations put the same points in the same places.
            //!
            //! Angles are compared through the matrices they build rather
            //! than against each other: the same rotation has more than one
            //! set of angles, so two sets that differ are not necessarily two
            //! rotations that differ, and a test that says otherwise fails on
            //! correct answers.
            bool sameRotation(const M44F& a, const M44F& b, float tolerance = .001F)
            {
                for (int row = 0; row < 3; ++row)
                {
                    for (int column = 0; column < 3; ++column)
                    {
                        if (std::abs(a.get(row, column) - b.get(row, column)) >
                            tolerance)
                        {
                            return false;
                        }
                    }
                }
                return true;
            }

            M44F build(const V3F& degrees)
            {
                return rotateZ(degrees.z) * rotateY(degrees.y) *
                    rotateX(degrees.x);
            }

            bool close(float a, float b, float tolerance = .001F)
            {
                return std::abs(a - b) < tolerance;
            }
        }

        TransformTest::TransformTest(const std::shared_ptr<Context>& context) :
            ITest(context, "fx::core_test::TransformTest")
        {}

        TransformTest::~TransformTest()
        {}

        std::shared_ptr<TransformTest> TransformTest::create(
            const std::shared_ptr<Context>& context)
        {
            return std::shared_ptr<TransformTest>(new TransformTest(context));
        }

        void TransformTest::run()
        {
            _matrix();
            _euler();
            _eulerWinding();
            _eulerLock();
        }

        void TransformTest::_matrix()
        {
            // Scale is applied before the rotation, which is what makes the
            // manipulator's scale arms turn with the emitter while its
            // translate arms do not. Checked here rather than left to the
            // viewport, because the viewport is where it was got wrong.
            Transform transform;
            transform.rotate.z.setConstant(90.F);
            transform.scale.x.setConstant(3.F);
            const V3F p = transform.getMatrix(0.0) * V3F(1.F, 0.F, 0.F);
            // A quarter turn about z takes the stretched x axis to +y.
            FTK_CHECK(close(p.x, 0.F));
            FTK_CHECK(close(p.y, 3.F));
            FTK_CHECK(close(p.z, 0.F));
        }

        void TransformTest::_euler()
        {
            // Round trips. Every one of these builds a rotation, reads the
            // angles back out and builds it again: the second matrix has to
            // be the first, whichever set of angles came back.
            const std::vector<V3F> cases =
            {
                V3F(0.F, 0.F, 0.F),
                V3F(30.F, 0.F, 0.F),
                V3F(0.F, 30.F, 0.F),
                V3F(0.F, 0.F, 30.F),
                V3F(30.F, 40.F, 50.F),
                V3F(-30.F, -40.F, -50.F),
                V3F(170.F, 10.F, -170.F),
                V3F(10.F, 130.F, 20.F),
                V3F(-95.F, 85.F, 175.F)
            };
            for (const auto& degrees : cases)
            {
                const M44F m = build(degrees);
                const V3F back = eulerZYX(m, degrees);
                FTK_CHECK(sameRotation(m, build(back)));
                // Asked from where it already is, the answer is where it is.
                // A manipulator that has not been moved must not rewrite the
                // panel with a different description of the same pose.
                FTK_CHECK(close(back.x, degrees.x));
                FTK_CHECK(close(back.y, degrees.y));
                FTK_CHECK(close(back.z, degrees.z));
            }
        }

        void TransformTest::_eulerWinding()
        {
            // A full turn about z, a degree at a time, the way a drag arrives.
            // The angle has to come out at 360 rather than back at zero: an
            // artist who has been round once should be able to see that they
            // have.
            V3F previous;
            for (int i = 1; i <= 360; ++i)
            {
                previous = eulerZYX(
                    rotateZ(static_cast<float>(i)), previous);
            }
            FTK_CHECK(close(previous.z, 360.F, .01F));

            // And back the other way, to zero rather than to -360.
            for (int i = 359; i >= 0; --i)
            {
                previous = eulerZYX(
                    rotateZ(static_cast<float>(i)), previous);
            }
            FTK_CHECK(close(previous.z, 0.F, .01F));

            // Past where the y angle folds. asin only reaches a quarter turn,
            // so carrying on past it needs the other set of angles; taking
            // the nearer one has to hold the pose steady, whatever the three
            // numbers do.
            previous = V3F(0.F, 0.F, 0.F);
            for (int i = 1; i <= 180; ++i)
            {
                const M44F m = rotateY(static_cast<float>(i));
                previous = eulerZYX(m, previous);
                FTK_CHECK(sameRotation(m, build(previous)));
            }
        }

        void TransformTest::_eulerLock()
        {
            // Straight up and straight down, where the first and last turns
            // are about the same axis and only their sum is decided. Any
            // answer is allowed as long as it builds the rotation asked for.
            for (float y : { 90.F, -90.F })
            {
                for (float z : { 0.F, 45.F, -120.F })
                {
                    const M44F m = build(V3F(0.F, y, z));
                    const V3F back = eulerZYX(m, V3F(0.F, y, z));
                    FTK_CHECK(sameRotation(m, build(back)));
                }
            }
        }
    }
}
