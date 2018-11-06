#pragma once

#include <glm/glm.hpp>

struct SceneSettings
{
    float debugType = 0.f;
    bool bDebugDepth = false;
    bool bUiChanged = false;
    bool bResized = false;
    bool bUpdated = true;
    bool bBoundSphere = true;
    bool bReduceShimmer = true;
    bool bClipSplitLogUniform = true;
    float depthIndex = 0.f;
	float lambda = 1.f;
    float angle = 160.f;
    float fov = 45.f;
    float Slice1 = 25.f;
    float Slice2 = 90.f;
    float Slice3 = 150.f;
    float WaterTexScale = 2.5f;
    glm::vec4 WaterColor = glm::vec4( 0.5f, 0.79f, 0.75f, 1.0f );
    glm::vec3 position = glm::vec3(0.7f, 9.5f, -1.0f);
};
