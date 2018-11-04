#pragma once

#include <GraphicsTypes.h>
#include <GraphicsContext.h>

namespace skybox
{
	void setDevice(const GraphicsDevicePtr& device);
    void initialize();
    void shutdown();
    void render(GraphicsContext& gfxContext, const TCamera& camera);
}