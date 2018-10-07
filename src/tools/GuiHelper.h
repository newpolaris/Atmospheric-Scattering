#pragma once

#include <glm/glm.hpp>

struct FloatSetting
{
    bool updateGUI();
    float value() const;
    float ratio() const;

    const char* name;
    glm::vec3 _value;
};
