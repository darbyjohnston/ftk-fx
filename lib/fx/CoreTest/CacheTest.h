// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <ftk/TestLib/ITest.h>

namespace fx
{
    namespace core_test
    {
        class CacheTest : public ftk::test::ITest
        {
        protected:
            CacheTest(const std::shared_ptr<ftk::Context>&);

        public:
            virtual ~CacheTest();

            static std::shared_ptr<CacheTest> create(
                const std::shared_ptr<ftk::Context>&);

            void run() override;

        private:
            void _frames();
            void _invalidate();
            void _lock();
            void _range();
            void _budget();
        };
    }
}
