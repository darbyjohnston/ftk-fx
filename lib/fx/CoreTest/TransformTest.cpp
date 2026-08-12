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
    }
}
