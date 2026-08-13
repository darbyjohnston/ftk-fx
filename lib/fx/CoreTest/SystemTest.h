// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <ftk/TestLib/ITest.h>

namespace fx
{
    namespace core
    {
        struct SystemFrame;
    }

    namespace core_test
    {
        class SystemTest : public ftk::test::ITest
        {
        protected:
            SystemTest(const std::shared_ptr<ftk::Context>&);

        public:
            virtual ~SystemTest();

            static std::shared_ptr<SystemTest> create(
                const std::shared_ptr<ftk::Context>&);

            void run() override;

        private:
            void _emission();
            void _gravity();
            void _lifespan();
            void _rotation();
            void _determinism();
            void _resume();

            //! Compare two frames exactly. Nothing here is allowed a tolerance:
            //! the point of the determinism tests is that the numbers are the
            //! same numbers, not nearly the same ones.
            bool _equal(const core::SystemFrame&, const core::SystemFrame&);
        };
    }
}
