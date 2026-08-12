// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/Editor.h>

#include <fx/App/CurveEditor.h>
#include <fx/App/EditorPlaceholder.h>
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
        namespace
        {
            //! The mark that says which editor the menus and the keyboard are
            //! aimed at: a filled dot at the start of the header, beside the
            //! menus naming what the editor is showing.
            //!
            //! It used to be a bar along the top edge, and before that a
            //! border round the whole editor. Both were a lot of accent colour
            //! for one bit of information, and both put it somewhere the eye
            //! was not already looking. The header is where the editor says
            //! what it is; this is one more word in that sentence.
            class CurrentDot : public IWidget
            {
            protected:
                CurrentDot() = default;

            public:
                static std::shared_ptr<CurrentDot> create(
                    const std::shared_ptr<Context>& context,
                    const std::shared_ptr<IWidget>& parent = nullptr)
                {
                    auto out = std::shared_ptr<CurrentDot>(new CurrentDot);
                    out->_init(context, "fx::app::CurrentDot", parent);
                    return out;
                }

                void setCurrent(bool value)
                {
                    if (value == _current)
                        return;
                    _current = value;
                    setDrawUpdate();
                }

                Size2I getSizeHint() const override
                {
                    const int s = _dot + _margin * 2;
                    return Size2I(s, s);
                }

                void styleEvent(const StyleEvent& event) override
                {
                    IWidget::styleEvent(event);
                    if (event.hasChanges())
                    {
                        _init2 = true;
                    }
                }

                void sizeHintEvent(const SizeHintEvent& event) override
                {
                    IWidget::sizeHintEvent(event);
                    if (_init2)
                    {
                        _init2 = false;
                        _dot = event.style->getSizeRole(
                            SizeRole::Border, event.displayScale) * 4;
                        _margin = event.style->getSizeRole(
                            SizeRole::MarginInside, event.displayScale);
                    }
                }

                void drawEvent(
                    const Box2I& drawRect,
                    const DrawEvent& event) override
                {
                    IWidget::drawEvent(drawRect, event);
                    if (!_current)
                        return;
                    const Box2I& g = getGeometry();
                    event.render->drawMesh(
                        circle(
                            V2I(
                                g.min.x + g.w() / 2,
                                g.min.y + g.h() / 2),
                            _dot / 2),
                        event.style->getColorRole(ColorRole::KeyFocus));
                }

            private:
                bool _current = false;
                bool _init2 = true;
                int _dot = 0;
                int _margin = 0;
            };
        }

        void Editor::_init(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            ViewType viewType,
            const std::shared_ptr<IWidget>& parent)
        {
            IContainer::_init(context, "fx::app::Editor", parent);
            setHStretch(Stretch::Expanding);
            setVStretch(Stretch::Expanding);
            // An editor's contents stay inside it. A scroll bar is drawn the
            // full height of the list it scrolls, and without this it carries
            // on past the bottom of the editor and over whatever is below.
            setClipChildren(true);

            _model = model;
            _viewType = viewType;

            _layout = VerticalLayout::create(context);
            _layout->setSpacingRole(SizeRole::None);

            _setWidget(_layout);

            auto headerLayout = HorizontalLayout::create(context, _layout);
            headerLayout->setSpacingRole(SizeRole::None);
            headerLayout->setBackgroundRole(ColorRole::Button);
            _currentDot = CurrentDot::create(context, headerLayout);
            _menuBar = MenuBar::create(context, headerLayout);
            _menuBar->setHStretch(Stretch::Expanding);

            // The actions outlive the menus they are put into, so the checked
            // state survives the rebuild that a content change causes.
            // Both sets are one of many: an editor shows one thing and a
            // viewport looks from one place, and neither has a "none of them".
            // The groups keep them exclusive and draw the ticks.
            _editorTypeGroup = ActionGroup::create(ActionGroupType::Radio);
            const auto editorLabels = getEditorTypeLabels();
            for (size_t i = 0; i < editorLabels.size(); ++i)
            {
                const EditorType type = static_cast<EditorType>(i);
                _editorTypeActions[type] = Action::create(
                    editorLabels[i],
                    [this, type] { setEditorType(type); });
                _editorTypeGroup->addAction(_editorTypeActions[type]);
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
            // says what the editor is showing without being opened -- which is
            // what the combo boxes these replaced did for free.
            _editorMenu = _menuBar->addMenu(getLabel(_editorType));
            for (const auto& i : _editorTypeActions)
            {
                _editorMenu->addAction(i.second);
            }

            _contentUpdate();
            // Built here rather than left to the tick: this is not inside a
            // menu callback, and the header should have its menus before it is
            // first laid out.
            _menuDirty = false;
            _menuUpdate();
        }

        Editor::~Editor()
        {}

        std::shared_ptr<Editor> Editor::create(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            ViewType viewType,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<Editor>(new Editor);
            out->_init(context, model, viewType, parent);
            return out;
        }

        EditorType Editor::getEditorType() const
        {
            return _editorType;
        }

        void Editor::setEditorType(EditorType value)
        {
            if (value == _editorType)
                return;
            _editorType = value;
            _contentUpdate();
        }

        ViewType Editor::getViewType() const
        {
            return _viewType;
        }

        void Editor::setViewType(ViewType value)
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

        std::shared_ptr<Viewport> Editor::getViewport() const
        {
            // Null unless a viewport is what is on screen. A viewport that has
            // been made but switched away from is not what the view actions
            // mean by "the current one".
            return EditorType::View == _editorType ? _viewport : nullptr;
        }

        std::shared_ptr<IWidget> Editor::_getContent(EditorType editorType)
        {
            auto i = _content.find(editorType);
            if (i != _content.end())
                return i->second;

            auto context = getContext();
            std::shared_ptr<IWidget> content;
            if (EditorType::View == editorType)
            {
                auto model = _model.lock();
                _viewport = Viewport::create(context, model, _viewType);
                // The content accepts the click, so the editor never sees it.
                // The viewport passes it back up rather than the editor trying
                // to intercept what its own content is handling.
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
            else if (EditorType::Curves == editorType)
            {
                _curveEditor = CurveEditor::create(context, _model.lock());
                content = _curveEditor;
            }
            else
            {
                content = EditorPlaceholder::create(context, editorType);
            }
            return _content[editorType] = content;
        }

        void Editor::_contentUpdate()
        {
            auto content = _getContent(_editorType);
            for (const auto& i : _content)
            {
                // Detached rather than hidden, so an editor holds one content
                // in its layout at a time and the rest cost nothing to lay out.
                i.second->setParent(i.second == content ? _layout : nullptr);
            }
            _menuDirty = true;
        }

        void Editor::tickEvent(
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

        void Editor::_menuUpdate()
        {
            _editorTypeGroup->setChecked(static_cast<int>(_editorType));
            _viewTypeGroup->setChecked(static_cast<int>(_viewType));

            // Only the View menu comes and goes, so only it is added and
            // removed. The Editor menu is built once and left alone, which is
            // also the menu an action is usually being picked from when this
            // runs.
            _menuBar->setMenuText(_editorMenu, getLabel(_editorType));

            const bool view = EditorType::View == _editorType;
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

        const std::shared_ptr<CurveEditor>& Editor::getCurveEditor() const
        {
            return _curveEditor;
        }

        void Editor::setDrawType(DrawType value)
        {
            _drawType = value;
            if (_viewport)
            {
                _viewport->setDrawType(value);
            }
        }

        void Editor::setParticleSize(float value)
        {
            _particleSize = value;
            if (_viewport)
            {
                _viewport->setParticleSize(value);
            }
        }

        void Editor::setCurrent(bool value)
        {
            if (value == _current)
                return;
            _current = value;
            std::dynamic_pointer_cast<CurrentDot>(_currentDot)->setCurrent(value);
        }

        void Editor::setPressCallback(const std::function<void(void)>& value)
        {
            _pressCallback = value;
        }



        void Editor::mousePressEvent(MouseClickEvent& event)
        {
            // Not accepted: the content below wants the click too, and this
            // only needs to know that the editor was the one clicked in.
            if (_pressCallback)
            {
                _pressCallback();
            }
        }
    }
}
