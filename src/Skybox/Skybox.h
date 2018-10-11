#pragma once

#include <GraphicsTypes.h>
#include <tools/TCamera.h>

namespace skybox
{
    void initialize(const GraphicsDevicePtr& device);
    void shutdown();
    void light(const TCamera& camera);
    void render(const TCamera& camera);
}
