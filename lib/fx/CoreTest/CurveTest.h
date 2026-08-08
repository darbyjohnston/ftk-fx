// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <ftk/TestLib/ITest.h>

namespace fx
{
    namespace core_test
    {
        class CurveTest : public ftk::test::ITest
        {
        protected:
            CurveTest(const std::shared_ptr<ftk::Context>&);

        public:
            virtual ~CurveTest();

            static std::shared_ptr<CurveTest> create(
                const std::shared_ptr<ftk::Context>&);

            void run() override;

        private:
            void _keys();
            void _interp();
            void _infinity();
            void _parameter();
        };
    }
}
