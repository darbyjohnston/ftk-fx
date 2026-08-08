// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include "fx-test.h"

#include <fx/CoreTest/CacheTest.h>
#include <fx/CoreTest/CurveTest.h>
#include <fx/CoreTest/PoolTest.h>
#include <fx/CoreTest/SystemTest.h>

#include <ftk/Core/CmdLine.h>
#include <ftk/Core/Context.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/String.h>

#include <algorithm>
#include <iostream>

namespace fx
{
    namespace tests
    {
        struct App::Private
        {
            std::shared_ptr<ftk::CmdLineListArg<std::string> > testNames;
            std::vector<std::shared_ptr<ftk::test::ITest> > tests;
            std::chrono::steady_clock::time_point startTime;
        };

        void App::_init(
            const std::shared_ptr<ftk::Context>& context,
            std::vector<std::string>& argv)
        {
            FTK_P();
            p.testNames = ftk::CmdLineListArg<std::string>::create(
                "Test",
                "Names of the tests to run.",
                true);
            IApp::_init(
                context,
                argv,
                "fx-test",
                "Test application",
                { p.testNames });
            p.startTime = std::chrono::steady_clock::now();

            p.tests.push_back(core_test::PoolTest::create(context));
            p.tests.push_back(core_test::CurveTest::create(context));
            p.tests.push_back(core_test::CacheTest::create(context));
            p.tests.push_back(core_test::SystemTest::create(context));
        }

        App::App() :
            _p(new Private)
        {}

        App::~App()
        {}

        std::shared_ptr<App> App::create(
            const std::shared_ptr<ftk::Context>& context,
            std::vector<std::string>& argv)
        {
            auto out = std::shared_ptr<App>(new App);
            out->_init(context, argv);
            return out;
        }

        int App::run()
        {
            FTK_P();

            std::vector<std::shared_ptr<ftk::test::ITest> > runTests;
            std::vector<std::string> unmatched;
            const auto& cmdLineTests = p.testNames->getList();
            if (!cmdLineTests.empty())
            {
                for (const auto& test : cmdLineTests)
                {
                    size_t matched = 0;
                    for (const auto& other : p.tests)
                    {
                        if (ftk::contains(
                            other->getName(),
                            test,
                            ftk::CaseCompare::Insensitive))
                        {
                            ++matched;
                            if (std::find(runTests.begin(), runTests.end(), other) ==
                                runTests.end())
                            {
                                runTests.push_back(other);
                            }
                        }
                    }
                    if (0 == matched)
                    {
                        unmatched.push_back(test);
                    }
                }
            }
            else
            {
                runTests = p.tests;
            }

            // A filter that matched nothing would run zero tests and exit
            // successfully, which reads exactly like a suite that passed.
            if (!unmatched.empty())
            {
                for (const auto& name : unmatched)
                {
                    _print(ftk::Format("ERROR: no tests match: {0}").arg(name));
                }
                return 1;
            }

            size_t failureCount = 0;
            for (const auto& test : runTests)
            {
                _context->tick();
                _print(ftk::Format("Running test: {0}").arg(test->getName()));
                test->run();
                failureCount += test->getFailureCount();
            }

            const auto now = std::chrono::steady_clock::now();
            const std::chrono::duration<float> diff = now - p.startTime;
            _print(ftk::Format("Seconds elapsed: {0}").arg(diff.count(), 2));
            _print(ftk::Format("Tests run: {0}").arg(runTests.size()));
            _print(ftk::Format("Failures: {0}").arg(failureCount));

            // The count is printed rather than returned: exit codes are
            // truncated to eight bits, so a run with a multiple of 256 failures
            // would report success.
            return failureCount > 0 ? 1 : 0;
        }
    }
}

FTK_MAIN()
{
    try
    {
        auto context = ftk::Context::create();
        auto args = ftk::convert(argc, argv);
        auto app = fx::tests::App::create(context, args);
        if (app->hasCmdLineHelp())
            return 0;
        return app->run();
    }
    catch (const std::exception& e)
    {
        std::cout << "ERROR: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
