// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/App/PaneOptions.h>

#include <ftk/UI/IContainer.h>

namespace fx
{
    namespace app
    {
        //! A stand-in for an editor that has not been written.
        //!
        //! It names the editor and the section of the design it comes from, and
        //! does nothing else. The point is to have something real to arrange
        //! and switch between while the pane mechanism is being built, so that
        //! the mechanism is exercised by more than one kind of content before
        //! any of that content exists.
        //!
        //! Each of these is meant to be deleted, one at a time, as the editor
        //! it stands for arrives.
        class PanePlaceholder : public ftk::IContainer
        {
        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                PaneType,
                const std::shared_ptr<ftk::IWidget>& parent);

            PanePlaceholder() = default;

        public:
            virtual ~PanePlaceholder();

            static std::shared_ptr<PanePlaceholder> create(
                const std::shared_ptr<ftk::Context>&,
                PaneType,
                const std::shared_ptr<ftk::IWidget>& parent = nullptr);
        };
    }
}
