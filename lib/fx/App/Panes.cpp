// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/Panes.h>

#include <fx/App/SceneModel.h>
#include <fx/App/Pane.h>

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
        void Panes::_init(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            const std::shared_ptr<IWidget>& parent)
        {
            IContainer::_init(context, "fx::app::Panes", parent);
            setHStretch(Stretch::Expanding);
            setVStretch(Stretch::Expanding);

            _layout = Observable<PaneLayout>::create(PaneLayout::Single);

            for (int i = 0; i < paneCountMax; ++i)
            {
                _panes[i] = Pane::create(
                    context,
                    model,
                    getDefaultViewType(i));
                setScreenshotTag(
                    _panes[i],
                    Format("MainWindow.Pane{0}").arg(i));
                _panes[i]->setPressCallback(
                    [this, i] { setCurrentIndex(i); });
            }
            _panes[0]->setCurrent(true);

            _layoutUpdate();

            _pointSizeObserver = Observer<float>::create(
                model->observePointSize(),
                [this](float value) { setPointSize(value); });
        }

        Panes::~Panes()
        {}

        std::shared_ptr<Panes> Panes::create(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<Panes>(new Panes);
            out->_init(context, model, parent);
            return out;
        }

        PaneLayout Panes::getLayout() const
        {
            return _layout->get();
        }

        std::shared_ptr<IObservable<PaneLayout> > Panes::observeLayout() const
        {
            return _layout;
        }

        void Panes::setLayout(PaneLayout value)
        {
            if (!_layout->setIfChanged(value))
                return;
            _layoutUpdate();
            // The current viewport may have just been hidden, and actions
            // aimed at a viewport nobody can see would be a mystery.
            setCurrentIndex(std::min(_currentIndex, getPaneCount(value) - 1));
        }

        const std::shared_ptr<Pane>& Panes::getCurrent() const
        {
            return _panes[_currentIndex];
        }

        const std::shared_ptr<Pane>& Panes::getPane(int index) const
        {
            return _panes[clamp(index, 0, paneCountMax - 1)];
        }

        int Panes::getCurrentIndex() const
        {
            return _currentIndex;
        }

        void Panes::setCurrentIndex(int value)
        {
            const int index = clamp(
                value,
                0,
                getPaneCount(_layout->get()) - 1);
            if (index == _currentIndex)
                return;
            _panes[_currentIndex]->setCurrent(false);
            _currentIndex = index;
            _panes[_currentIndex]->setCurrent(true);
        }

        void Panes::setPointSize(float value)
        {
            for (const auto& pane : _panes)
            {
                pane->setPointSize(value);
            }
        }



        void Panes::_layoutUpdate()
        {
            auto context = getContext();
            if (!context)
                return;

            // Detach the panes before dropping the splitters, or the splitters
            // take them down with them. Splitter::setWidgets() does the same
            // for its own children, which is what makes the rebuild below a
            // statement rather than an ordering to get right.
            for (const auto& pane : _panes)
            {
                pane->setParent(nullptr);
            }

            switch (_layout->get())
            {
            case PaneLayout::Two:
            {
                // Stacked rather than side by side. A viewport is wider than
                // it is tall, and halving the width of an already wide window
                // gives two panes that are the wrong shape for what goes in
                // them.
                auto splitter = Splitter::create(context, Orientation::Vertical);
                splitter->setWidgets({ _panes[0], _panes[1] });
                _root = splitter;
                break;
            }
            case PaneLayout::Three:
            {
                // Two across the top and one along the bottom, which is the
                // four-up with a row missing rather than the two-up with a
                // column added -- and it keeps the same reading order.
                auto top = Splitter::create(context, Orientation::Horizontal);
                top->setWidgets({ _panes[0], _panes[1] });
                auto splitter = Splitter::create(context, Orientation::Vertical);
                splitter->setWidgets({ top, _panes[2] });
                _root = splitter;
                break;
            }
            case PaneLayout::Four:
            {
                // One splitter that divides both ways, rather than two ganged
                // together: the crossing in the middle belongs to it, so
                // dragging there moves both divisions at once.
                auto splitter = Splitter2D::create(context);
                splitter->setWidgets(
                    { _panes[0], _panes[1], _panes[2], _panes[3] });
                _root = splitter;
                break;
            }
            default:
                _root = _panes[0];
                break;
            }

            // The old tree goes when this replaces it: IContainer detaches what
            // it is handed over, and a widget only dies when its parent lets go
            // of it.
            _setWidget(_root);
        }
    }
}
