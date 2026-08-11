// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/EditorPlaceholder.h>

#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>

using namespace ftk;

namespace fx
{
    namespace app
    {
        void EditorPlaceholder::_init(
            const std::shared_ptr<Context>& context,
            EditorType editorType,
            const std::shared_ptr<IWidget>& parent)
        {
            IContainer::_init(context, "fx::app::EditorPlaceholder", parent);
            setHStretch(Stretch::Expanding);
            setVStretch(Stretch::Expanding);
            setBackgroundRole(ColorRole::Well);

            auto layout = VerticalLayout::create(context);
            layout->setSpacingRole(SizeRole::SpacingSmall);
            layout->addSpacer(Stretch::Expanding);

            auto label = Label::create(context, getLabel(editorType), layout);
            label->setFont(FontType::Bold);
            label->setTextRole(ColorRole::TextDisabled);
            label->setHAlign(HAlign::Center);

            auto description = Label::create(
                context,
                getEditorTypeDescription(editorType),
                layout);
            description->setTextRole(ColorRole::TextDisabled);
            description->setHAlign(HAlign::Center);

            layout->addSpacer(Stretch::Expanding);
            _setWidget(layout);
        }

        EditorPlaceholder::~EditorPlaceholder()
        {}

        std::shared_ptr<EditorPlaceholder> EditorPlaceholder::create(
            const std::shared_ptr<Context>& context,
            EditorType editorType,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<EditorPlaceholder>(new EditorPlaceholder);
            out->_init(context, editorType, parent);
            return out;
        }

    }
}
