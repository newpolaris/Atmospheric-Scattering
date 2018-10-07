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

float FloatSetting::ratio() const
{
    assert(_value.z - _value.y != 0.0);
    return (_value.x - _value.y) / (_value.z - _value.y);
}
