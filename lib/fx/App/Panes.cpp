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
            IWidget::_init(context, "fx::app::Panes", parent);
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

        Size2I Panes::getSizeHint() const
        {
            return _root ? _root->getSizeHint() : Size2I();
        }

        void Panes::setGeometry(const Box2I& value)
        {
            IWidget::setGeometry(value);
            if (_root)
            {
                _root->setGeometry(value);
            }
        }

        void Panes::_layoutUpdate()
        {
            auto context = getContext();
            if (!context)
                return;

            // Detach the panes before dropping the splitters, or the splitters
            // take them down with them.
            for (const auto& pane : _panes)
            {
                pane->setParent(nullptr);
            }
            // And detach the old tree from this widget. A parent holds its
            // children by shared pointer, so dropping the only other reference
            // does not free them: the old splitters stayed in the tree, kept
            // their last geometry and went on drawing their handles over the
            // new arrangement. One leaked tree per layout change.
            if (_root)
            {
                _root->setParent(nullptr);
            }
            _root.reset();

            switch (_layout->get())
            {
            case PaneLayout::Two:
            {
                auto splitter = Splitter::create(context, Orientation::Horizontal);
                _panes[0]->setParent(splitter);
                _panes[1]->setParent(splitter);
                _root = splitter;
                break;
            }
            case PaneLayout::Three:
            {
                auto splitter = Splitter::create(context, Orientation::Horizontal);
                _panes[0]->setParent(splitter);
                auto right = Splitter::create(context, Orientation::Vertical, splitter);
                _panes[1]->setParent(right);
                _panes[2]->setParent(right);
                _root = splitter;
                break;
            }
            case PaneLayout::Four:
            {
                // Two rows, each split in two. The rows divide independently,
                // which is not what a linked four-up does; it needs the
                // splitters to talk to each other and is not worth the
                // machinery until someone is bothered by it.
                auto splitter = Splitter::create(context, Orientation::Vertical);
                auto top = Splitter::create(context, Orientation::Horizontal, splitter);
                _panes[0]->setParent(top);
                _panes[1]->setParent(top);
                auto bottom = Splitter::create(context, Orientation::Horizontal, splitter);
                _panes[2]->setParent(bottom);
                _panes[3]->setParent(bottom);
                _root = splitter;
                break;
            }
            default:
                _root = _panes[0];
                break;
            }

            _root->setParent(shared_from_this());
            _root->setGeometry(getGeometry());
        }
    }
}
