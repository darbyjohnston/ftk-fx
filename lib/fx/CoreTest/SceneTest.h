// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <ftk/TestLib/ITest.h>

namespace fx
{
    namespace core_test
    {
        class SceneTest : public ftk::test::ITest
        {
        protected:
            SceneTest(const std::shared_ptr<ftk::Context>&);

        public:
            virtual ~SceneTest();

            static std::shared_ptr<SceneTest> create(
                const std::shared_ptr<ftk::Context>&);

            void run() override;

        private:
            void _parameters();
            void _roundTrip();
            void _file();
            void _errors();
            void _defaults();
        };
    }
}
