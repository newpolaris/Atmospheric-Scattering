
#include <tools/imgui.h>
#include "GuiHelper.h"

bool FloatSetting::updateGUI()
{
    return ImGui::SliderFloat(name, &_value.r, _value.g, _value.b);
}

float FloatSetting::value() const
{
    return _value.x;
}
