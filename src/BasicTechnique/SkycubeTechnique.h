#pragma once

#include <GraphicsTypes.h>
#include <GraphicsContext.h>

namespace skycube
{
	void setDevice(const GraphicsDevicePtr& device);
    void initialize();
    void shutdown();
    void render(GraphicsContext& gfxContext, const TCamera& camera);
}
