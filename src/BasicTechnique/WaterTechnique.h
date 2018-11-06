#pragma once

#include <functional>
#include <string>
#include <glm/glm.hpp>
#include <GraphicsTypes.h>
#include <GraphicsContext.h>
#include <GLType/ProgramShader.h>
#include <Mesh.h>

using Renderfunctiondelegator = std::function<void(GraphicsContext& context, const glm::mat4& reflection)>;

struct WaterOptions
{
    // how large to scale the wave map texture in the shader
    // higher than 1 and the texture will repeat providing finer detail normals
    float WaveMapScale = 1.0f;

    //asset names for the normal/wave maps
    std::string FlowMapAsset;
    std::string NoiseMapAsset;
    std::string WaveMapAsset0;
    std::string WaveMapAsset1;

    //water color and sun light properties
    glm::vec4 WaterColor;
    glm::vec4 SunColor;
    glm::vec3 SunDirection;
    float SunFactor; //the intensity of the sun specular term.
    float SunPower;  //how shiny we want the sun specular term on the water to be.
};

class WaterTechnique
{
public:

	void setDevice(const GraphicsDevicePtr& device);
    void setDrawfunction(Renderfunctiondelegator function);
    void create(float waterSize, float cellSpacing);
    void update(float detla, const WaterOptions& options);
    void destroy();
    void beforerender(GraphicsContext& gfxContext);
    void render(GraphicsContext& gfxContext, const TCamera& camera);

private:

    // offsets for the flowmap updated every frame. These offsets are used to 
    // simulate the water flowing through the flow map.
    float FlowMapOffset0 = 0.f;
    float FlowMapOffset1 = 0.f;

    WaterOptions m_Options;
    GraphicsDeviceWeakPtr m_Device; 
    GraphicsTexturePtr m_FlowMapTex;
    GraphicsTexturePtr m_NoiseMapTex;
    GraphicsTexturePtr m_NoiseSmoothMapTex;
    GraphicsTexturePtr m_Wave0Tex;
    GraphicsTexturePtr m_Wave1Tex;
    GraphicsTexturePtr m_RefractTex;
    GraphicsFramebufferPtr m_RefractFramebuffer;
    ProgramShader m_WaterShader;
    PlaneMesh m_WaterPlane;
    Renderfunctiondelegator m_renderfunction;
};
