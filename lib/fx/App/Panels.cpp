// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/Panels.h>

#include <fx/App/DiagPanel.h>
#include <fx/App/IPanel.h>
#include <fx/App/ParametersPanel.h>
#include <fx/App/SystemsPanel.h>

#include <ftk/UI/Divider.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/ScrollWidget.h>
#include <ftk/UI/TabWidget.h>

#include <ftk/Core/Format.h>

#include <algorithm>

using namespace ftk;

namespace fx
{
    namespace app
    {
        std::vector<std::string> getPanelStyleLabels()
        {
            return { "Column", "Tabs" };
        }

        std::string getLabel(PanelStyle value)
        {
            const auto labels = getPanelStyleLabels();
            const size_t i = static_cast<size_t>(value);
            return i < labels.size() ? labels[i] : std::string();
        }

        void Panels::_init(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            const std::shared_ptr<Editors>& editors,
            const std::shared_ptr<IWidget>& parent)
        {
            IContainer::_init(context, "fx::app::Panels", parent);

            _open = ObservableList<std::string>::create();
            _style = Observable<PanelStyle>::create(PanelStyle::Column);

            _columnLayout = VerticalLayout::create(context);
            _columnLayout->setMarginRole(SizeRole::None);
            _columnLayout->setSpacingRole(SizeRole::None);

            // Both, so that a column narrower than the widest panel in it can
            // be scrolled across rather than quietly cutting off whatever is
            // at the right edge -- which is where a list keeps its controls.
            _columnScroll = ScrollWidget::create(context, ScrollType::Both);
            _columnScroll->setBorder(false);
            _columnScroll->setWidget(_columnLayout);

            _tabWidget = TabWidget::create(context);
            _tabWidget->setClosable(true);
            std::weak_ptr<Panels> weak(
                std::dynamic_pointer_cast<Panels>(shared_from_this()));
            _tabWidget->setCloseCallback(
                [weak](int index)
                {
                    if (auto panels = weak.lock())
                    {
                        const auto& open = panels->_open->get();
                        if (index >= 0 && index < static_cast<int>(open.size()))
                        {
                            panels->setOpen(open[index], false);
                        }
                    }
                });

            _add(SystemsPanel::create(context, model));
            _add(ParametersPanel::create(context, model, editors));
            _add(DiagPanel::create(context));

            // Systems above Parameters, because it says what the parameters
            // below it belong to. Diagnostics starts closed: it is for the
            // moments when something is slow, and a graph nobody is reading is
            // a graph taking up the column.
            _open->setIfChanged({ "Systems", "Parameters" });
            _panelsUpdate();
        }

        Panels::~Panels()
        {}

        std::shared_ptr<Panels> Panels::create(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            const std::shared_ptr<Editors>& editors,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<Panels>(new Panels);
            out->_init(context, model, editors, parent);
            return out;
        }

        void Panels::_add(const std::shared_ptr<IPanel>& panel)
        {
            const std::string name = panel->getPanelName();
            _names.push_back(name);
            _panels[name] = panel;
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
            _panelsDirty = true;
        }

        PanelStyle Panels::getStyle() const
        {
            return _style->get();
        }

        std::shared_ptr<IObservable<PanelStyle> > Panels::observeStyle() const
        {
            return _style;
        }

        void Panels::setStyle(PanelStyle value)
        {
            if (!_style->setIfChanged(value))
                return;
            _panelsDirty = true;
        }

        const std::shared_ptr<ScrollWidget>& Panels::_getScroll(
            const std::string& name)
        {
            auto i = _scrolls.find(name);
            if (i != _scrolls.end())
                return i->second;
            auto scroll = ScrollWidget::create(getContext(), ScrollType::Both);
            scroll->setBorder(false);
            return _scrolls[name] = scroll;
        }

        void Panels::_panelsUpdate()
        {
            const bool tabs = PanelStyle::Tabs == _style->get();

            // Everything comes out of both containers first. A parent holds its
            // children by shared pointer, so a panel left in the container it
            // is moving out of stays in it.
            _tabWidget->clear();
            for (const auto& i : _panels)
            {
                i.second->setParent(nullptr);
            }
            for (const auto& i : _scrolls)
            {
                i.second->setParent(nullptr);
            }
            // Between the panels rather than on top of each one. Whether
            // anything is above it is the column's business, not the panel's:
            // a panel asked to draw its own upper edge would draw one at the
            // top of the column too, where there is nothing to be separated
            // from.
            for (const auto& divider : _columnDividers)
            {
                divider->setParent(nullptr);
            }
            _columnDividers.clear();

            for (const auto& name : _names)
            {
                if (!isOpen(name))
                    continue;
                const auto& panel = _panels[name];
                // In tabs the tab already names the panel and carries the close
                // button, so the panel's own header would say it twice.
                panel->setHeaderVisible(!tabs);
                if (tabs)
                {
                    // A scroll area per tab, since only one is on screen at a
                    // time. Stacked, they share one, so that a panel takes the
                    // height it needs rather than an equal share.
                    const auto& scroll = _getScroll(name);
                    scroll->setWidget(panel);
                    _tabWidget->addTab(name, scroll);
                }
                else
                {
                    if (!_columnLayout->getChildren().empty())
                    {
                        auto divider = Divider::create(
                            getContext(), Orientation::Vertical, _columnLayout);
                        // Recessed, like the one under a panel's title. A
                        // collapsed bellows and a panel title are the same
                        // height, colour and shape, so without this the seam
                        // between two panels read as one more row of the
                        // list above it.
                        divider->setBackgroundRole(ColorRole::Well);
                        _columnDividers.push_back(divider);
                    }
                    panel->setParent(_columnLayout);
                }
            }

            // One or the other fills the column. IContainer detaches whichever
            // is on the way out, so there is no hiding to keep track of.
            _setWidget(tabs ?
                std::static_pointer_cast<IWidget>(_tabWidget) :
                std::static_pointer_cast<IWidget>(_columnScroll));

            // With nothing open the column has nothing to say, so it gets out
            // of the way and the splitter hands the whole width to the editors.
            // The Panels menu is how it comes back.
            setVisible(!_open->get().empty());
        }

        void Panels::tickEvent(
            bool parentsVisible,
            bool parentsEnabled,
            const TickEvent& event)
        {
            IWidget::tickEvent(parentsVisible, parentsEnabled, event);
            if (_panelsDirty)
            {
                _panelsDirty = false;
                _panelsUpdate();
            }
        }


    }
}
