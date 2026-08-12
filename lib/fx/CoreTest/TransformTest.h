// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <ftk/TestLib/ITest.h>

namespace fx
{
    namespace core_test
    {
        class TransformTest : public ftk::test::ITest
        {
        protected:
            TransformTest(const std::shared_ptr<ftk::Context>&);

        public:
            virtual ~TransformTest();

            static std::shared_ptr<TransformTest> create(
                const std::shared_ptr<ftk::Context>&);

            void run() override;

        private:
            void _matrix();
            void _euler();
            void _eulerWinding();
            void _eulerLock();
        };
    }
}
