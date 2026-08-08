// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/App/IPanel.h>

namespace fx
{
    namespace app
    {
        //! Diagnostics panel.
        //!
        //! A graph per sampler registered with ftk::DiagSystem: feather-tk's
        //! own frame time, triangle and glyph counts, and the samplers this
        //! application adds for the simulation and the cache.
        //!
        //! It is here early and deliberately. The viewport is the product, and
        //! the way that gets lost is one widget at a time, each of them cheap
        //! on its own. A graph of frame time on screen while the panels are
        //! being built is what makes that visible on the day it happens rather
        //! than in a profile six months later.
        class DiagPanel : public IPanel
        {
            FTK_NON_COPYABLE(DiagPanel);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<IWidget>& parent);

            DiagPanel();

        public:
            virtual ~DiagPanel();

            static std::shared_ptr<DiagPanel> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            FTK_PRIVATE();
        };
    }
}
