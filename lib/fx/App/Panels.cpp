// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/Panels.h>

#include <fx/App/DiagPanel.h>
#include <fx/App/IPanel.h>
#include <fx/App/ParametersPanel.h>

#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/ScrollWidget.h>

#include <ftk/Core/Format.h>

#include <algorithm>

using namespace ftk;

namespace fx
{
    namespace app
    {
        void Panels::_init(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            const std::shared_ptr<Panes>& panes,
            const std::shared_ptr<IWidget>& parent)
        {
            IWidget::_init(context, "fx::app::Panels", parent);

            _open = ObservableList<std::string>::create();

            _layout = VerticalLayout::create(context);
            _layout->setMarginRole(SizeRole::None);
            _layout->setSpacingRole(SizeRole::None);

            _scrollWidget = ScrollWidget::create(
                context,
                ScrollType::Both,
                shared_from_this());
            _scrollWidget->setBorder(false);
            _scrollWidget->setWidget(_layout);

            _add(ParametersPanel::create(context, model, panes));
            _add(DiagPanel::create(context));

            // Diagnostics starts closed. It is for the moments when something
            // is slow, and a graph nobody is reading is a graph taking up the
            // column.
            _open->setIfChanged({ "Parameters" });
            _openUpdate();
        }

        Panels::~Panels()
        {}

        std::shared_ptr<Panels> Panels::create(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            const std::shared_ptr<Panes>& panes,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<Panels>(new Panels);
            out->_init(context, model, panes, parent);
            return out;
        }

        void Panels::_add(const std::shared_ptr<IPanel>& panel)
        {
            const std::string name = panel->getPanelName();
            _names.push_back(name);
            _panels[name] = panel;
            panel->setParent(_layout);
            setScreenshotTag(panel, Format("MainWindow.Panel.{0}").arg(name));
            std::weak_ptr<Panels> weak(
                std::dynamic_pointer_cast<Panels>(shared_from_this()));
            panel->setCloseCallback(
                [weak, name]
                {
                    if (auto panels = weak.lock())
                    {
                        panels->setOpen(name, false);
                    }
                });
        }

        const std::vector<std::string>& Panels::getPanelNames() const
        {
            return _names;
        }

        bool Panels::isOpen(const std::string& name) const
        {
            const auto& open = _open->get();
            return std::find(open.begin(), open.end(), name) != open.end();
        }

        std::shared_ptr<IObservableList<std::string> > Panels::observeOpen() const
        {
            return _open;
        }

        void Panels::setOpen(const std::string& name, bool value)
        {
            if (value == isOpen(name))
                return;
            // Rebuilt from the panel order rather than appended to, so the
            // stack does not reshuffle itself depending on what was opened
            // when.
            std::vector<std::string> open;
            for (const auto& i : _names)
            {
                const bool isThis = i == name;
                if (isThis ? value : isOpen(i))
                {
                    open.push_back(i);
                }
            }
            _open->setIfChanged(open);
            _openUpdate();
        }

        void Panels::_openUpdate()
        {
            for (const auto& i : _panels)
            {
                i.second->setVisible(isOpen(i.first));
            }
        }

        Size2I Panels::getSizeHint() const
        {
            return _scrollWidget->getSizeHint();
        }

        void Panels::setGeometry(const Box2I& value)
        {
            IWidget::setGeometry(value);
            _scrollWidget->setGeometry(value);
        }
    }
}
