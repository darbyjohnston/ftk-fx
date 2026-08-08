// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <ftk/TestLib/ITest.h>

namespace fx
{
    namespace core_test
    {
        class PoolTest : public ftk::test::ITest
        {
        protected:
            PoolTest(const std::shared_ptr<ftk::Context>&);

        public:
            virtual ~PoolTest();

            static std::shared_ptr<PoolTest> create(
                const std::shared_ptr<ftk::Context>&);

            void run() override;

        private:
            void _birthAndDeath();
            void _compact();
            void _ids();
        };
    }
}
