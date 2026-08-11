// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/CoreTest/SceneTest.h>

#include <fx/Core/Serialize.h>
#include <fx/Sim/Scene.h>

#include <ftk/Core/Assert.h>
#include <ftk/Core/Format.h>

#include <filesystem>
#include <fstream>

using namespace fx::core;
using namespace fx::sim;

namespace fx
{
    namespace core_test
    {
        SceneTest::SceneTest(const std::shared_ptr<ftk::Context>& context) :
            ITest(context, "fx::core_test::SceneTest")
        {}

        SceneTest::~SceneTest()
        {}

        std::shared_ptr<SceneTest> SceneTest::create(
            const std::shared_ptr<ftk::Context>& context)
        {
            return std::shared_ptr<SceneTest>(new SceneTest(context));
        }

        void SceneTest::run()
        {
            _parameters();
            _roundTrip();
            _file();
            _errors();
            _defaults();
        }

        namespace
        {
            //! A curve with something of everything in it, so a round trip has
            //! more to lose than a single default key.
            Curve testCurve()
            {
                Curve out;
                out.addKey(Key{ 1.0, 0.F, Interp::Step });
                out.addKey(Key{ 12.0, 4.5F, Interp::Linear });
                out.addKey(Key{ 30.0, -2.25F, Interp::Bezier, .5F, -1.5F });
                out.addKey(Key{ 48.0, 7.F, Interp::Smooth });
                out.setPreInfinity(Infinity::Cycle);
                out.setPostInfinity(Infinity::Oscillate);
                return out;
            }

            Scene testScene()
            {
                Scene out;
                out.range = ftk::RangeI(4, 96);
                out.frameRate = 30.0;
                out.system.setName("sparks");
                out.system.setSubsteps(3);
                auto& emitter = out.system.getEmitter();
                emitter.seed = 17;
                emitter.shape = EmitterShape::Box;
                emitter.surface = true;
                emitter.size.x.setConstant(3.5F);
                emitter.size.z.setConstant(.25F);
                emitter.rate.setConstant(1234.5F);
                emitter.rate.setCurve(testCurve());
                emitter.position.y.setConstant(2.5F);
                emitter.spread.setConstant(45.F);
                out.system.getForces().gravity.y.setConstant(-3.5F);
                out.system.getForces().drag.setConstant(.75F);
                return out;
            }
        }

        void SceneTest::_parameters()
        {
            // A constant writes as a bare number rather than an object, which
            // is the whole reason the file is readable.
            const nlohmann::json constant = Parameter(2.5F);
            FTK_CHECK(constant.is_number());
            FTK_CHECK(2.5F == constant.get<Parameter>().getValue(0.0));

            Parameter animated(9.F);
            animated.setCurve(testCurve());
            const nlohmann::json json = animated;
            FTK_CHECK(json.is_object());
            const auto back = json.get<Parameter>();
            FTK_CHECK(back == animated);
            // The constant survives the trip, so switching animation off comes
            // back to where it was rather than to zero.
            FTK_CHECK(9.F == back.getConstant());

            // Every interpolation and infinity name round trips. This is what
            // stops a reordered enum from quietly changing old files.
            for (size_t i = 0; i < static_cast<size_t>(Interp::Count); ++i)
            {
                const Interp interp = static_cast<Interp>(i);
                Curve curve;
                curve.addKey(Key{ 1.0, 1.F, interp, .25F, .75F });
                const auto j = nlohmann::json(curve).get<Curve>();
                FTK_CHECK(j.getKeys()[0].interp == interp);
            }
            for (size_t i = 0; i < static_cast<size_t>(Infinity::Count); ++i)
            {
                const Infinity inf = static_cast<Infinity>(i);
                Curve curve;
                curve.setPostInfinity(inf);
                FTK_CHECK(nlohmann::json(curve).get<Curve>().getPostInfinity() == inf);
            }
        }

        void SceneTest::_roundTrip()
        {
            const Scene scene = testScene();
            const auto back = nlohmann::json(scene).get<Scene>();
            FTK_CHECK(back == scene);

            // Equality is the test for unsaved changes, so it has to notice a
            // change anywhere in the recipe rather than only near the top.
            Scene changed = scene;
            changed.system.getEmitter().lifespanVariance.setConstant(.9F);
            FTK_CHECK(changed != scene);
            changed = scene;
            changed.system.getForces().gravity.z.setConstant(1.F);
            FTK_CHECK(changed != scene);
            changed = scene;
            changed.system.getEmitter().shape = EmitterShape::Sphere;
            FTK_CHECK(changed != scene);
            changed = scene;
            changed.system.getEmitter().surface = false;
            FTK_CHECK(changed != scene);
            changed = scene;
            changed.system.getEmitter().size.y.setConstant(9.F);
            FTK_CHECK(changed != scene);
            changed = scene;
            changed.range = ftk::RangeI(1, 10);
            FTK_CHECK(changed != scene);
        }

        void SceneTest::_file()
        {
            const auto path = std::filesystem::temp_directory_path() /
                "SceneTest.fx";
            const Scene scene = testScene();
            write(path, scene);
            FTK_CHECK(read(path) == scene);

            // The file says which version wrote it, as promised by the design.
            std::ifstream is(path);
            nlohmann::json json;
            is >> json;
            FTK_CHECK(json.contains("version"));
            std::filesystem::remove(path);
        }

        void SceneTest::_errors()
        {
            const auto dir = std::filesystem::temp_directory_path();
            const auto missing = dir / "SceneTest-does-not-exist.fx";
            std::filesystem::remove(missing);
            try
            {
                read(missing);
                FTK_CHECK(false);
            }
            catch (const std::exception&)
            {}

            // A file that is not a scene, and a scene that is not sensible,
            // both throw rather than loading something half true.
            const auto bad = dir / "SceneTest-bad.fx";
            {
                std::ofstream os(bad);
                os << "{ not json";
            }
            try
            {
                read(bad);
                FTK_CHECK(false);
            }
            catch (const std::exception&)
            {}
            {
                std::ofstream os(bad);
                os << R"({ "range": { "min": 90, "max": 10 } })";
            }
            try
            {
                read(bad);
                FTK_CHECK(false);
            }
            catch (const std::exception&)
            {}
            {
                std::ofstream os(bad);
                os << R"({ "systems": [ { "emitter": { "shape": "Blob" } } ] })";
            }
            try
            {
                read(bad);
                FTK_CHECK(false);
            }
            catch (const std::exception&)
            {}
            {
                std::ofstream os(bad);
                os << R"({ "systems": [ { "emitter": { "rate":
                    { "curve": { "keys": [ { "frame": 1, "value": 0,
                    "interp": "Wobble" } ] } } } } ] })";
            }
            try
            {
                read(bad);
                FTK_CHECK(false);
            }
            catch (const std::exception&)
            {}
            std::filesystem::remove(bad);
        }

        void SceneTest::_defaults()
        {
            // An empty object is a default scene rather than an error: every
            // field is optional so that a file written before a field existed
            // still opens.
            const auto scene = nlohmann::json::parse("{}").get<Scene>();
            FTK_CHECK(scene == Scene());
        }
    }
}
