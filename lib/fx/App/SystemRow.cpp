// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/SystemsPanelPrivate.h>

#include <ftk/UI/CheckBox.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>

using namespace ftk;

namespace fx
{
    namespace app
    {
        void SystemRow::_init(
            const std::shared_ptr<Context>& context,
            const std::string& text,
            const std::shared_ptr<IWidget>& parent)
        {
            IMouseWidget::_init(context, "fx::app::SystemRow", parent);
            _setMouseHoverEnabled(true);
            _setMousePressEnabled(true);

            _label = Label::create(context, text);
            _label->setHStretch(Stretch::Expanding);

            // On the right, where the columns will be. There is one control
            // today; a solo, a lock and a particle count are the obvious next
            // ones, and they line up beside this rather than pushing the name
            // further in each time one arrives.
            _checkBox = CheckBox::create(context);
            _checkBox->setTooltip("Solve this system");

            _layout = HorizontalLayout::create(context, shared_from_this());
            _layout->setMarginRole(SizeRole::MarginInside);
            _layout->setSpacingRole(SizeRole::SpacingSmall);
            _label->setParent(_layout);
            _checkBox->setParent(_layout);
        }

        SystemRow::~SystemRow()
        {}

        std::shared_ptr<SystemRow> SystemRow::create(
            const std::shared_ptr<Context>& context,
            const std::string& text,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<SystemRow>(new SystemRow);
            out->_init(context, text, parent);
            return out;
        }

        void SystemRow::setCurrent(bool value)
        {
            if (value == _current)
                return;
            _current = value;
            setDrawUpdate();
        }

        void SystemRow::setSolved(bool value)
        {
            _checkBox->setChecked(value);
        }

        void SystemRow::setCurrentCallback(const std::function<void(void)>& value)
        {
            _currentCallback = value;
        }

        void SystemRow::setSolvedCallback(const std::function<void(bool)>& value)
        {
            _checkBox->setCheckedCallback(value);
        }

        Size2I SystemRow::getSizeHint() const
        {
            return _layout->getSizeHint();
        }

        void SystemRow::setGeometry(const Box2I& value)
        {
            IMouseWidget::setGeometry(value);
            _layout->setGeometry(value);
        }

        void SystemRow::drawEvent(const Box2I& drawRect, const DrawEvent& event)
        {
            IMouseWidget::drawEvent(drawRect, event);

            // The whole geometry, in the order feather-tk's list items use it:
            // what the row is, then what the pointer is doing to it. Drawn
            // here rather than in the overlay pass so the label and the check
            // box land on top of it.
            const Box2I& g = getGeometry();
            if (_current)
            {
                event.render->drawRect(
                    g, event.style->getColorRole(ColorRole::Checked));
            }
            if (_isMousePressed())
            {
                event.render->drawRect(
                    g, event.style->getColorRole(ColorRole::Pressed));
            }
            else if (_isMouseInside())
            {
                event.render->drawRect(
                    g, event.style->getColorRole(ColorRole::Hover));
            }
        }

        void SystemRow::mousePressEvent(MouseClickEvent& event)
        {
            IMouseWidget::mousePressEvent(event);
            // The check box is a child, so a press that lands on it never
            // reaches here: ticking a system does not also select it.
            if (_currentCallback)
            {
                _currentCallback();
            }
        }
    }
}
