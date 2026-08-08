// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/CoreTest/CurveTest.h>

#include <fx/Core/Parameter.h>

#include <ftk/Core/Format.h>

#include <cmath>

using namespace fx::core;

namespace fx
{
    namespace core_test
    {
        namespace
        {
            bool close(float a, float b, float tolerance = .001F)
            {
                return std::abs(a - b) <= tolerance;
            }

            Key key(double frame, float value, Interp interp)
            {
                Key out;
                out.frame = frame;
                out.value = value;
                out.interp = interp;
                return out;
            }
        }

        CurveTest::CurveTest(const std::shared_ptr<ftk::Context>& context) :
            ITest(context, "fx::core_test::CurveTest")
        {}

        CurveTest::~CurveTest()
        {}

        std::shared_ptr<CurveTest> CurveTest::create(
            const std::shared_ptr<ftk::Context>& context)
        {
            return std::shared_ptr<CurveTest>(new CurveTest(context));
        }

        void CurveTest::run()
        {
            _keys();
            _interp();
            _infinity();
            _parameter();
        }

        void CurveTest::_keys()
        {
            Curve curve;
            FTK_CHECK(0.F == curve.getValue(0.0));

            // Keys are kept in frame order however they arrive.
            curve.addKey(key(20.0, 2.F, Interp::Linear));
            curve.addKey(key(10.0, 1.F, Interp::Linear));
            FTK_CHECK(2 == curve.getKeys().size());
            FTK_CHECK(10.0 == curve.getKeys()[0].frame);
            FTK_CHECK(20.0 == curve.getKeys()[1].frame);

            // Adding at a frame that already has a key replaces it rather than
            // leaving two keys on one frame.
            curve.addKey(key(10.0, 5.F, Interp::Linear));
            FTK_CHECK(2 == curve.getKeys().size());
            FTK_CHECK(5.F == curve.getValue(10.0));

            // A single key is that value everywhere.
            curve.removeKey(1);
            FTK_CHECK(5.F == curve.getValue(-100.0));
            FTK_CHECK(5.F == curve.getValue(100.0));

            curve.setKeys({
                key(30.0, 3.F, Interp::Linear),
                key(10.0, 1.F, Interp::Linear),
                key(30.0, 9.F, Interp::Linear) });
            FTK_CHECK(2 == curve.getKeys().size());
            FTK_CHECK(9.F == curve.getValue(30.0));

            curve.clear();
            FTK_CHECK(curve.getKeys().empty());
        }

        void CurveTest::_interp()
        {
            Curve curve;
            curve.addKey(key(10.0, 0.F, Interp::Linear));
            curve.addKey(key(20.0, 10.F, Interp::Linear));

            FTK_CHECK(close(0.F, curve.getValue(10.0)));
            FTK_CHECK(close(10.F, curve.getValue(20.0)));
            if (!close(5.F, curve.getValue(15.0)))
            {
                _fail(ftk::Format("Linear midpoint: expected 5, got {0}").
                    arg(curve.getValue(15.0)));
            }

            // A stepped key holds its value until the next key.
            curve.addKey(key(10.0, 0.F, Interp::Step));
            FTK_CHECK(close(0.F, curve.getValue(19.99F)));
            FTK_CHECK(close(10.F, curve.getValue(20.0)));

            // A smooth key is flat where it is an extreme, which is the whole
            // point of taking the tangent from the neighbours: a curve that
            // rises and falls should not overshoot at the top.
            Curve smooth;
            smooth.addKey(key(0.0, 0.F, Interp::Smooth));
            smooth.addKey(key(10.0, 10.F, Interp::Smooth));
            smooth.addKey(key(20.0, 0.F, Interp::Smooth));
            FTK_CHECK(smooth.getValue(9.0) <= 10.F);
            FTK_CHECK(smooth.getValue(11.0) <= 10.F);
            FTK_CHECK(close(10.F, smooth.getValue(10.0)));
            // Symmetric about the middle key.
            FTK_CHECK(close(smooth.getValue(5.0), smooth.getValue(15.0)));

            // Explicit tangents: flat handles at both ends give an ease that
            // leaves and arrives at zero slope.
            Curve bezier;
            Key a = key(0.0, 0.F, Interp::Bezier);
            Key b = key(10.0, 10.F, Interp::Bezier);
            bezier.addKey(a);
            bezier.addKey(b);
            FTK_CHECK(close(5.F, bezier.getValue(5.0)));
            FTK_CHECK(bezier.getValue(1.0) < 1.F);
            FTK_CHECK(bezier.getValue(9.0) > 9.F);
        }

        void CurveTest::_infinity()
        {
            Curve curve;
            curve.addKey(key(0.0, 0.F, Interp::Linear));
            curve.addKey(key(10.0, 10.F, Interp::Linear));

            // Constant holds the end values.
            FTK_CHECK(close(0.F, curve.getValue(-10.0)));
            FTK_CHECK(close(10.F, curve.getValue(20.0)));

            curve.setPreInfinity(Infinity::Linear);
            curve.setPostInfinity(Infinity::Linear);
            FTK_CHECK(close(-10.F, curve.getValue(-10.0), .05F));
            FTK_CHECK(close(20.F, curve.getValue(20.0), .05F));

            // Cycle repeats: a gust authored over ten frames covers the shot.
            curve.setPostInfinity(Infinity::Cycle);
            FTK_CHECK(close(5.F, curve.getValue(15.0)));
            FTK_CHECK(close(5.F, curve.getValue(25.0)));

            // Cycle with offset stacks each repeat on the last.
            curve.setPostInfinity(Infinity::CycleOffset);
            FTK_CHECK(close(15.F, curve.getValue(15.0)));
            FTK_CHECK(close(25.F, curve.getValue(25.0)));

            // Oscillate turns around, so the value comes back down.
            curve.setPostInfinity(Infinity::Oscillate);
            FTK_CHECK(close(5.F, curve.getValue(15.0)));
            FTK_CHECK(close(0.F, curve.getValue(20.0)));
            FTK_CHECK(close(5.F, curve.getValue(25.0)));

            curve.setPreInfinity(Infinity::Cycle);
            FTK_CHECK(close(5.F, curve.getValue(-5.0)));
        }

        void CurveTest::_parameter()
        {
            Parameter p(2.F);
            FTK_CHECK(Parameter::Type::Constant == p.getType());
            FTK_CHECK(close(2.F, p.getValue(0.0)));
            FTK_CHECK(close(2.F, p.getValue(1000.0)));

            Curve curve;
            curve.addKey(key(0.0, 0.F, Interp::Linear));
            curve.addKey(key(10.0, 10.F, Interp::Linear));
            p.setCurve(curve);
            FTK_CHECK(Parameter::Type::Curve == p.getType());
            FTK_CHECK(close(5.F, p.getValue(5.0)));

            // The constant is remembered, so turning animation off puts the
            // parameter back where the artist left it.
            FTK_CHECK(close(2.F, p.getConstant()));
            p.setConstant(p.getConstant());
            FTK_CHECK(Parameter::Type::Constant == p.getType());
            FTK_CHECK(close(2.F, p.getValue(5.0)));

            V3Parameter v(ftk::V3F(1.F, 2.F, 3.F));
            FTK_CHECK(ftk::V3F(1.F, 2.F, 3.F) == v.getValue(0.0));
            v.y.setCurve(curve);
            FTK_CHECK(close(5.F, v.getValue(5.0).y));
            FTK_CHECK(close(1.F, v.getValue(5.0).x));
        }
    }
}
