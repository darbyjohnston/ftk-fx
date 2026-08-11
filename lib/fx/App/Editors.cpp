// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/Editors.h>

#include <fx/App/SceneModel.h>
#include <fx/App/Editor.h>

#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/Splitter.h>
#include <ftk/UI/Splitter2D.h>

#include <ftk/Core/Format.h>
#include <ftk/Core/Math.h>

using namespace ftk;

namespace fx
{
    namespace app
    {
        void Editors::_init(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            const std::shared_ptr<IWidget>& parent)
        {
            IContainer::_init(context, "fx::app::Editors", parent);
            setHStretch(Stretch::Expanding);
            setVStretch(Stretch::Expanding);

            _layout = Observable<EditorLayout>::create(EditorLayout::Single);

            for (int i = 0; i < editorCountMax; ++i)
            {
                _editors[i] = Editor::create(
                    context,
                    model,
                    getDefaultViewType(i));
                setScreenshotTag(
                    _editors[i],
                    Format("MainWindow.Editor{0}").arg(i));
                _editors[i]->setPressCallback(
                    [this, i] { setCurrentIndex(i); });
            }
            _editors[0]->setCurrent(true);

            _layoutUpdate();

            _particleSizeObserver = Observer<float>::create(
                model->observeParticleSize(),
                [this](float value) { setParticleSize(value); });
            _drawTypeObserver = Observer<DrawType>::create(
                model->observeDrawType(),
                [this](DrawType value) { setDrawType(value); });
        }

        Editors::~Editors()
        {}

        std::shared_ptr<Editors> Editors::create(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<Editors>(new Editors);
            out->_init(context, model, parent);
            return out;
        }

        EditorLayout Editors::getLayout() const
        {
            return _layout->get();
        }

        std::shared_ptr<IObservable<EditorLayout> > Editors::observeLayout() const
        {
            return _layout;
        }

        void Editors::setLayout(EditorLayout value)
        {
            if (!_layout->setIfChanged(value))
                return;
            _layoutUpdate();
            // The current viewport may have just been hidden, and actions
            // aimed at a viewport nobody can see would be a mystery.
            setCurrentIndex(std::min(_currentIndex, getEditorCount(value) - 1));
        }

        const std::shared_ptr<Editor>& Editors::getCurrent() const
        {
            return _editors[_currentIndex];
        }

        const std::shared_ptr<Editor>& Editors::getEditor(int index) const
        {
            return _editors[clamp(index, 0, editorCountMax - 1)];
        }

        int Editors::getCurrentIndex() const
        {
            return _currentIndex;
        }

        void Editors::setCurrentIndex(int value)
        {
            const int index = clamp(
                value,
                0,
                getEditorCount(_layout->get()) - 1);
            if (index == _currentIndex)
                return;
            _editors[_currentIndex]->setCurrent(false);
            _currentIndex = index;
            _editors[_currentIndex]->setCurrent(true);
        }

        void Editors::setDrawType(DrawType value)
        {
            for (const auto& editor : _editors)
            {
                editor->setDrawType(value);
            }
        }

        void Editors::setParticleSize(float value)
        {
            for (const auto& editor : _editors)
            {
                editor->setParticleSize(value);
            }
        }



        void Editors::_layoutUpdate()
        {
            auto context = getContext();
            if (!context)
                return;

            // Detach the editors before dropping the splitters, or the
            // splitters take them down with them. Splitter::setWidgets() does
            // the same for its own children, which is what makes the rebuild
            // below a statement rather than an ordering to get right.
            for (const auto& editor : _editors)
            {
                editor->setParent(nullptr);
            }

            switch (_layout->get())
            {
            case EditorLayout::Two:
            {
                // Stacked rather than side by side. A viewport is wider than
                // it is tall, and halving the width of an already wide window
                // gives two editors that are the wrong shape for what goes in
                // them.
                auto splitter = Splitter::create(context, Orientation::Vertical);
                splitter->setWidgets({ _editors[0], _editors[1] });
                _root = splitter;
                break;
            }
            case EditorLayout::Three:
            {
                // Two across the top and one along the bottom, which is the
                // four-up with a row missing rather than the two-up with a
                // column added -- and it keeps the same reading order.
                auto top = Splitter::create(context, Orientation::Horizontal);
                top->setWidgets({ _editors[0], _editors[1] });
                auto splitter = Splitter::create(context, Orientation::Vertical);
                splitter->setWidgets({ top, _editors[2] });
                _root = splitter;
                break;
            }
            case EditorLayout::Four:
            {
                // One splitter that divides both ways, rather than two ganged
                // together: the crossing in the middle belongs to it, so
                // dragging there moves both divisions at once.
                auto splitter = Splitter2D::create(context);
                splitter->setWidgets(
                    { _editors[0], _editors[1], _editors[2], _editors[3] });
                _root = splitter;
                break;
            }
            default:
                _root = _editors[0];
                break;
            }

            // The old tree goes when this replaces it: IContainer detaches what
            // it is handed over, and a widget only dies when its parent lets go
            // of it.
            _setWidget(_root);
        }
    }
}
