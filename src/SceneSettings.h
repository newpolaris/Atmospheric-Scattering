#pragma once

#include <glm/glm.hpp>

struct SceneSettings
{
    static const uint32_t NumCascades = 4;

    float debugType = 0.f;
    bool bDebugDepth = false;
    bool bUiChanged = false;
    bool bResized = false;
    bool bUpdated = true;
    bool bBoundSphere = true;
    bool bClipSplitLogUniform = false;
    float depthIndex = 0.f;
	float lambda = 1.f;
    float angle = 76.f;
    float fov = 45.f;
    float Slice1 = 25.f;
    float Slice2 = 90.f;
    float Slice3 = 150.f;
    glm::vec3 position = glm::vec3(0.7f, 9.5f, -1.0f);
};
