// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/Pane.h>

#include <fx/App/CurveEditor.h>
#include <fx/App/PanePlaceholder.h>
#include <fx/App/Viewport.h>

#include <ftk/UI/ActionGroup.h>
#include <ftk/UI/DrawUtil.h>
#include <ftk/UI/Menu.h>
#include <ftk/UI/MenuBar.h>
#include <ftk/UI/RowLayout.h>

using namespace ftk;

namespace fx
{
    namespace app
    {
        void Pane::_init(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            ViewType viewType,
            const std::shared_ptr<IWidget>& parent)
        {
            IContainer::_init(context, "fx::app::Pane", parent);
            setHStretch(Stretch::Expanding);
            setVStretch(Stretch::Expanding);

            _model = model;
            _viewType = viewType;

            _layout = VerticalLayout::create(context);
            _layout->setSpacingRole(SizeRole::None);

            _setWidget(_layout);

            _menuBar = MenuBar::create(context, _layout);

            // The actions outlive the menus they are put into, so the checked
            // state survives the rebuild that a content change causes.
            // Both sets are one of many: a pane shows one thing and a
            // viewport looks from one place, and neither has a "none of them".
            // The groups keep them exclusive and draw the ticks.
            _paneTypeGroup = ActionGroup::create(ActionGroupType::Radio);
            const auto paneLabels = getPaneTypeLabels();
            for (size_t i = 0; i < paneLabels.size(); ++i)
            {
                const PaneType type = static_cast<PaneType>(i);
                _paneTypeActions[type] = Action::create(
                    paneLabels[i],
                    [this, type] { setPaneType(type); });
                _paneTypeGroup->addAction(_paneTypeActions[type]);
            }
            _viewTypeGroup = ActionGroup::create(ActionGroupType::Radio);
            const auto viewLabels = getViewTypeLabels();
            for (size_t i = 0; i < viewLabels.size(); ++i)
            {
                const ViewType type = static_cast<ViewType>(i);
                _viewTypeActions[type] = Action::create(
                    viewLabels[i],
                    [this, type] { setViewType(type); });
                _viewTypeGroup->addAction(_viewTypeActions[type]);
            }

            // Built once here; only the view menu comes and goes. Both titles
            // are the current selection rather than a fixed word, so the header
            // says what the pane is showing without being opened -- which is
            // what the combo boxes these replaced did for free.
            _paneMenu = _menuBar->addMenu(getLabel(_paneType));
            for (const auto& i : _paneTypeActions)
            {
                _paneMenu->addAction(i.second);
            }

            _contentUpdate();
            // Built here rather than left to the tick: this is not inside a
            // menu callback, and the header should have its menus before it is
            // first laid out.
            _menuDirty = false;
            _menuUpdate();
        }

        Pane::~Pane()
        {}

        std::shared_ptr<Pane> Pane::create(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            ViewType viewType,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<Pane>(new Pane);
            out->_init(context, model, viewType, parent);
            return out;
        }

        PaneType Pane::getPaneType() const
        {
            return _paneType;
        }

        void Pane::setPaneType(PaneType value)
        {
            if (value == _paneType)
                return;
            _paneType = value;
            _contentUpdate();
        }

        ViewType Pane::getViewType() const
        {
            return _viewType;
        }

        void Pane::setViewType(ViewType value)
        {
            if (value == _viewType)
                return;
            _viewType = value;
            _menuDirty = true;
            if (_viewport)
            {
                _viewport->setViewType(value);
            }
        }

        std::shared_ptr<Viewport> Pane::getViewport() const
        {
            // Null unless a viewport is what is on screen. A viewport that has
            // been made but switched away from is not what the view actions
            // mean by "the current one".
            return PaneType::View == _paneType ? _viewport : nullptr;
        }

        std::shared_ptr<IWidget> Pane::_getContent(PaneType paneType)
        {
            auto i = _content.find(paneType);
            if (i != _content.end())
                return i->second;

            auto context = getContext();
            std::shared_ptr<IWidget> content;
            if (PaneType::View == paneType)
            {
                auto model = _model.lock();
                _viewport = Viewport::create(context, model, _viewType);
                // The content accepts the click, so the pane never sees it.
                // The viewport passes it back up rather than the pane trying to
                // intercept what its own content is handling.
                _viewport->setPressCallback(
                    [this]
                    {
                        if (_pressCallback)
                        {
                            _pressCallback();
                        }
                    });
                _viewport->setParticleSize(_particleSize);
                _viewport->setDrawType(_drawType);
                content = _viewport;
            }
            else if (PaneType::Curves == paneType)
            {
                content = CurveEditor::create(context, _model.lock());
            }
            else
            {
                content = PanePlaceholder::create(context, paneType);
            }
            return _content[paneType] = content;
        }

        void Pane::_contentUpdate()
        {
            auto content = _getContent(_paneType);
            for (const auto& i : _content)
            {
                // Detached rather than hidden, so a pane holds one content in
                // its layout at a time and the rest cost nothing to lay out.
                i.second->setParent(i.second == content ? _layout : nullptr);
            }
            _menuDirty = true;
        }

        void Pane::tickEvent(
            bool parentsVisible,
            bool parentsEnabled,
            const TickEvent& event)
        {
            IWidget::tickEvent(parentsVisible, parentsEnabled, event);
            if (_menuDirty)
            {
                _menuDirty = false;
                _menuUpdate();
            }
        }

        void Pane::_menuUpdate()
        {
            _paneTypeGroup->setChecked(static_cast<int>(_paneType));
            _viewTypeGroup->setChecked(static_cast<int>(_viewType));

            // Only the View menu comes and goes, so only it is added and
            // removed. The Pane menu is built once and left alone, which is
            // also the menu an action is usually being picked from when this
            // runs.
            _menuBar->setMenuText(_paneMenu, getLabel(_paneType));

            const bool view = PaneType::View == _paneType;
            if (view && !_viewMenu)
            {
                _viewMenu = _menuBar->addMenu(getLabel(_viewType));
                for (const auto& i : _viewTypeActions)
                {
                    _viewMenu->addAction(i.second);
                }
            }
            else if (view)
            {
                _menuBar->setMenuText(_viewMenu, getLabel(_viewType));
            }
            else if (_viewMenu)
            {
                _menuBar->removeMenu(getLabel(_viewType));
                _viewMenu.reset();
            }
        }

        void Pane::setDrawType(DrawType value)
        {
            _drawType = value;
            if (_viewport)
            {
                _viewport->setDrawType(value);
            }
        }

        void Pane::setParticleSize(float value)
        {
            _particleSize = value;
            if (_viewport)
            {
                _viewport->setParticleSize(value);
            }
        }

        void Pane::setCurrent(bool value)
        {
            if (value == _current)
                return;
            _current = value;
            setDrawUpdate();
        }

        void Pane::setPressCallback(const std::function<void(void)>& value)
        {
            _pressCallback = value;
        }


        void Pane::sizeHintEvent(const SizeHintEvent& event)
        {
            IWidget::sizeHintEvent(event);
            _border = event.style->getSizeRole(
                SizeRole::Border,
                event.displayScale);
        }


        void Pane::drawOverlayEvent(const Box2I& drawRect, const DrawEvent& event)
        {
            // Mark the pane the menu actions and the keyboard apply to. With
            // four of them on screen this is the difference between an
            // arrangement and a guess.
            //
            // Drawn in the overlay pass rather than the ordinary one, which
            // runs before the children: the content fills the pane, so a border
            // drawn there survived only where the header did not reach.
            if (_current)
            {
                event.render->drawMesh(
                    border(getGeometry(), _border),
                    event.style->getColorRole(ColorRole::KeyFocus));
            }
        }

        void Pane::mousePressEvent(MouseClickEvent& event)
        {
            // Not accepted: the content below wants the click too, and this
            // only needs to know that the pane was the one clicked in.
            if (_pressCallback)
            {
                _pressCallback();
            }
        }
    }
}
