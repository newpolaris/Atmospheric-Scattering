#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <tools/TCamera.h>
#include <SceneSettings.h>

namespace Graphics
{
    void CalcOrthoProjections(const SceneSettings& settings, const TCamera& camera, const glm::vec3& direction, std::vector<float>& m_ClipspaceCascadeEnd, std::vector<glm::mat4>& lightSpace);
}
