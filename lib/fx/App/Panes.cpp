// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/Panes.h>

#include <fx/App/SceneModel.h>
#include <fx/App/Pane.h>

#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/Splitter.h>

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
                auto splitter = Splitter::create(context, Orientation::Horizontal);
                splitter->setWidgets({ _panes[0], _panes[1] });
                _root = splitter;
                break;
            }
            case PaneLayout::Three:
            {
                auto right = Splitter::create(context, Orientation::Vertical);
                right->setWidgets({ _panes[1], _panes[2] });
                auto splitter = Splitter::create(context, Orientation::Horizontal);
                splitter->setWidgets({ _panes[0], right });
                _root = splitter;
                break;
            }
            case PaneLayout::Four:
            {
                // Two rows, each split in two, with the two divisions tied
                // together so the four panes stay a grid rather than drifting
                // into a ragged pair of rows.
                auto top = Splitter::create(context, Orientation::Horizontal);
                top->setWidgets({ _panes[0], _panes[1] });
                auto bottom = Splitter::create(context, Orientation::Horizontal);
                bottom->setWidgets({ _panes[2], _panes[3] });
                std::weak_ptr<Splitter> topWeak(top);
                std::weak_ptr<Splitter> bottomWeak(bottom);
                top->setSplitCallback(
                    [bottomWeak](float value)
                    {
                        if (auto bottom = bottomWeak.lock())
                        {
                            bottom->setSplit(value);
                        }
                    });
                bottom->setSplitCallback(
                    [topWeak](float value)
                    {
                        if (auto top = topWeak.lock())
                        {
                            top->setSplit(value);
                        }
                    });
                auto splitter = Splitter::create(context, Orientation::Vertical);
                splitter->setWidgets({ top, bottom });
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
