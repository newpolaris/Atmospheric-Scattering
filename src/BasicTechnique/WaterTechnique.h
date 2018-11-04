#pragma once

#include <string>
#include <glm/glm.hpp>
#include <GraphicsTypes.h>
#include <GraphicsContext.h>
#include <GLType/ProgramShader.h>
#include <Mesh.h>

struct WaterOptions
{
    // the number of vertices in the x and z plane (must be of the form 2^n + 1)
    // and the amount of spacing between vertices
    int Width = 257;
    int Height = 257;
    float CellSpacing = .5f;

    // how large to scale the wave map texture in the shader
    // higher than 1 and the texture will repeat providing finer detail normals
    float WaveMapScale = 1.0f;

    // size of the reflection and refraction render targets' width and height
    int RenderTargetSize = 512;

    // offsets for the flowmap updated every frame. These offsets are used to 
    // simulate the water flowing through the flow map.
    float FlowMapOffset0 = 0.f;
    float FlowMapOffset1 = 0.f;

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
    void create(const WaterOptions& options);
    void update(float detla);
    void destroy();
    void render(GraphicsContext& gfxContext, const TCamera& camera);

private:

    WaterOptions m_Options;
    GraphicsDeviceWeakPtr m_Device; 
    GraphicsTexturePtr m_FlowMapTex;
    GraphicsTexturePtr m_NoiseMapTex;
    GraphicsTexturePtr m_Wave0Tex;
    GraphicsTexturePtr m_Wave1Tex;
    ProgramShader m_WaterShader;
    PlaneMesh m_WaterPlane;
};
