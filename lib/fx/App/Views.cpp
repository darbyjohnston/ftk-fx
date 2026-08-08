// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/Views.h>

#include <fx/App/SceneModel.h>
#include <fx/App/Viewport.h>

#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/Splitter.h>

#include <ftk/Core/Format.h>
#include <ftk/Core/Math.h>

using namespace ftk;

namespace fx
{
    namespace app
    {
        void Views::_init(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            const std::shared_ptr<IWidget>& parent)
        {
            IWidget::_init(context, "fx::app::Views", parent);
            setHStretch(Stretch::Expanding);
            setVStretch(Stretch::Expanding);

            _layout = Observable<ViewLayout>::create(ViewLayout::Single);

            for (int i = 0; i < viewCountMax; ++i)
            {
                _viewports[i] = Viewport::create(
                    context,
                    model,
                    getDefaultViewType(i));
                setScreenshotTag(
                    _viewports[i],
                    Format("MainWindow.Viewport{0}").arg(i));
                _viewports[i]->setPressCallback(
                    [this, i] { setCurrentIndex(i); });
            }
            _viewports[0]->setCurrent(true);

            _layoutUpdate();
        }

        Views::~Views()
        {}

        std::shared_ptr<Views> Views::create(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<Views>(new Views);
            out->_init(context, model, parent);
            return out;
        }

        ViewLayout Views::getLayout() const
        {
            return _layout->get();
        }

        std::shared_ptr<IObservable<ViewLayout> > Views::observeLayout() const
        {
            return _layout;
        }

        void Views::setLayout(ViewLayout value)
        {
            if (!_layout->setIfChanged(value))
                return;
            _layoutUpdate();
            // The current viewport may have just been hidden, and actions
            // aimed at a viewport nobody can see would be a mystery.
            setCurrentIndex(std::min(_currentIndex, getViewCount(value) - 1));
        }

        const std::shared_ptr<Viewport>& Views::getCurrent() const
        {
            return _viewports[_currentIndex];
        }

        const std::shared_ptr<Viewport>& Views::getViewport(int index) const
        {
            return _viewports[clamp(index, 0, viewCountMax - 1)];
        }

        int Views::getCurrentIndex() const
        {
            return _currentIndex;
        }

        void Views::setCurrentIndex(int value)
        {
            const int index = clamp(
                value,
                0,
                getViewCount(_layout->get()) - 1);
            if (index == _currentIndex)
                return;
            _viewports[_currentIndex]->setCurrent(false);
            _currentIndex = index;
            _viewports[_currentIndex]->setCurrent(true);
        }

        void Views::setPointSize(float value)
        {
            for (const auto& viewport : _viewports)
            {
                viewport->setPointSize(value);
            }
        }

        Size2I Views::getSizeHint() const
        {
            return _root ? _root->getSizeHint() : Size2I();
        }

        void Views::setGeometry(const Box2I& value)
        {
            IWidget::setGeometry(value);
            if (_root)
            {
                _root->setGeometry(value);
            }
        }

        void Views::_layoutUpdate()
        {
            auto context = getContext();
            if (!context)
                return;

            // Detach the viewports before dropping the splitters, or the
            // splitters take them down with them.
            for (const auto& viewport : _viewports)
            {
                viewport->setParent(nullptr);
            }
            _root.reset();

            switch (_layout->get())
            {
            case ViewLayout::Two:
            {
                auto splitter = Splitter::create(context, Orientation::Horizontal);
                _viewports[0]->setParent(splitter);
                _viewports[1]->setParent(splitter);
                _root = splitter;
                break;
            }
            case ViewLayout::Three:
            {
                auto splitter = Splitter::create(context, Orientation::Horizontal);
                _viewports[0]->setParent(splitter);
                auto right = Splitter::create(context, Orientation::Vertical, splitter);
                _viewports[1]->setParent(right);
                _viewports[2]->setParent(right);
                _root = splitter;
                break;
            }
            case ViewLayout::Four:
            {
                // Two rows, each split in two. The rows divide independently,
                // which is not what a linked four-up does; it needs the
                // splitters to talk to each other and is not worth the
                // machinery until someone is bothered by it.
                auto splitter = Splitter::create(context, Orientation::Vertical);
                auto top = Splitter::create(context, Orientation::Horizontal, splitter);
                _viewports[0]->setParent(top);
                _viewports[1]->setParent(top);
                auto bottom = Splitter::create(context, Orientation::Horizontal, splitter);
                _viewports[2]->setParent(bottom);
                _viewports[3]->setParent(bottom);
                _root = splitter;
                break;
            }
            default:
                _root = _viewports[0];
                break;
            }

            _root->setParent(shared_from_this());
            _root->setGeometry(getGeometry());
        }
    }
}
