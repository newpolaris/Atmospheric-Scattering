#include <GL/glew.h>
#include <glfw3.h>

// GLM for matrix transformation
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> 

#include <tools/gltools.hpp>
#include <tools/Profile.h>
#include <tools/imgui.h>
#include <tools/TCamera.h>
#include <tools/SimpleTimer.h>
#include <tools/GuiHelper.h>

#include <GLType/GraphicsDevice.h>
#include <GLType/GraphicsData.h>
#include <GLType/OGLDevice.h>
#include <GLType/ProgramShader.h>
#include <GLType/GraphicsFramebuffer.h>
#include <GLType/GraphicsTexture.h>

#include <BufferManager.h>
#include <GraphicsTypes.h>
#include <PhaseFunctions.h>
#include <Mesh.h>
#include <ModelAssImp.h>
#include <LightingTechnique.h>
#include <ShadowTechnique.h>
#include <BasicTechnique/SkyboxTechnique.h>
#include <BasicTechnique/WaterTechnique.h>

#include <fstream>
#include <memory>
#include <vector>
#include <algorithm>
#include <GameCore.h>
#include <SceneSettings.h>
#include <tools/string.h>
#include <GraphicsContext.h>

enum ProfilerType { kProfilerTypeRender = 0, kProfilerTypeUpdate };

namespace 
{
    float s_CpuTick = 0.f;
    float s_GpuTick = 0.f;
    const int s_NumMeshes = 5;
    glm::mat4 m_MatMeshModel[s_NumMeshes];
}

class LightScattering final : public gamecore::IGameApp
{
public:
	LightScattering() noexcept;
	virtual ~LightScattering() noexcept;

	virtual void startup() noexcept override;
	virtual void closeup() noexcept override;
	virtual void update() noexcept override;
    virtual void updateHUD() noexcept override;
	virtual void render() noexcept override;

	virtual void keyboardCallback(uint32_t c, bool bPressed) noexcept override;
	virtual void framesizeCallback(int32_t width, int32_t height) noexcept override;
	virtual void motionCallback(float xpos, float ypos, bool bPressed) noexcept override;
	virtual void mouseCallback(float xpos, float ypos, bool bPressed) noexcept override;

	GraphicsDevicePtr createDevice(const GraphicsDeviceDesc& desc) noexcept;

private:

    void ShadowMapPass(GraphicsContext& gfxContext);
    void RenderDebugDepth(GraphicsContext& gfxContext);
    void RenderPass(GraphicsContext& gfxContext);
    void TonemapPass(GraphicsContext& gfxContext);
    void RenderScene(const ProgramShader& shader);

    glm::vec3 GetSunDirection() const;
    glm::mat4 GetLightSpaceMatrix(uint32_t i) const;

    std::vector<float> m_CascadeEnd;
    std::vector<glm::mat4> m_lightSpace;

    std::vector<glm::vec2> m_Samples;
    SceneSettings m_Settings;
	TCamera m_Camera;
    SimpleTimer m_Timer;
    DirectionalLight m_DirectionalLight;
    CubeMesh m_Cube;
    SphereMesh m_Sphere;
    SphereMesh m_SphereMini;
    PlaneMesh m_Ground;
    ModelAssImp m_Column;
    ModelAssImp m_Dragon;
    WaterTechnique m_Water;
    FullscreenTriangleMesh m_ScreenTraingle;
    ProgramShader m_LightShader;
    ProgramShader m_FlatShader;
    ProgramShader m_ShadowMapShader;
    ProgramShader m_DebugDepthShader;
    ProgramShader m_PostProcessHDRShader;
    GraphicsDevicePtr m_Device;
    GraphicsTexturePtr m_TexWood;
};

CREATE_APPLICATION(LightScattering);

LightScattering::LightScattering() noexcept :
    m_Sphere(32, 1.0e2f),
    m_SphereMini(48, 2.0f),
    m_Ground(1000.f, 32.f, 10.f)
{
    m_DirectionalLight.AmbientIntensity = 0.5f;
    m_DirectionalLight.DiffuseIntensity = 0.9f;
    m_DirectionalLight.Color = glm::vec3(1.f, 1.f, 1.f);
    m_DirectionalLight.Direction = glm::vec3(1.0f, -1.f, 0.0f);
}

LightScattering::~LightScattering() noexcept
{
}

void LightScattering::startup() noexcept
{
	profiler::initialize();

    m_Timer.initialize();
	m_Camera.setMoveCoefficient(1.f);
    m_Camera.setViewParams(glm::vec3(8.f, 21.f, -23.f), glm::vec3(8.f, 21.f, -23.f) + glm::vec3(-0.7f, -0.44f, 0.9f));

	GraphicsDeviceDesc deviceDesc;
#if __APPLE__
	deviceDesc.setDeviceType(GraphicsDeviceType::GraphicsDeviceTypeOpenGL);
#else
	deviceDesc.setDeviceType(GraphicsDeviceType::GraphicsDeviceTypeOpenGLCore);
#endif
	m_Device = createDevice(deviceDesc);

    GraphicsTextureDesc woodDesc;
    woodDesc.setFilename("resources/wood.png");
    woodDesc.setWrapS(GL_REPEAT);
    woodDesc.setWrapT(GL_REPEAT);
    m_TexWood = m_Device->createTexture(woodDesc);

    m_ShadowMapShader.setDevice(m_Device);
    m_ShadowMapShader.initialize();
    m_ShadowMapShader.addShader(GL_VERTEX_SHADER, "ShadowMapVS.glsl");
    m_ShadowMapShader.link();

    m_FlatShader.setDevice(m_Device);
    m_FlatShader.initialize();
    m_FlatShader.addShader(GL_VERTEX_SHADER, "Flat.Vertex");
    m_FlatShader.addShader(GL_FRAGMENT_SHADER, "Flat.Fragment");
    m_FlatShader.link();

    m_DebugDepthShader.setDevice(m_Device);
    m_DebugDepthShader.initialize();
    m_DebugDepthShader.addShader(GL_VERTEX_SHADER, "DebugDepth.Vertex");
    m_DebugDepthShader.addShader(GL_FRAGMENT_SHADER, "DebugDepth.Fragment");
    m_DebugDepthShader.link();

    m_LightShader.setDevice(m_Device);
    m_LightShader.initialize();
    m_LightShader.addShader(GL_VERTEX_SHADER, "LightingVS.glsl");
    m_LightShader.addShader(GL_FRAGMENT_SHADER, "LightingPS.glsl");
    m_LightShader.link();

	m_PostProcessHDRShader.setDevice(m_Device);
	m_PostProcessHDRShader.initialize();
	m_PostProcessHDRShader.addShader(GL_VERTEX_SHADER, "PostProcessHDR.Vertex");
	m_PostProcessHDRShader.addShader(GL_FRAGMENT_SHADER, "PostProcessHDR.Fragment");
	m_PostProcessHDRShader.link();

    m_ScreenTraingle.create();
    m_Cube.create();
    m_Sphere.create();
    m_SphereMini.create();
    m_Ground.create();
    m_Column.create();
    m_Column.loadFromFile("resources/WaterFlow/BasicColumnScene.x");
    m_Dragon.create();
    // m_Dragon.loadFromFile("resources/WaterFlow/dragon.x");

    for (int i = 0; i < s_NumMeshes; i++) {
        glm::mat4 model(1.f);
        m_MatMeshModel[i] = glm::translate(model, glm::vec3(0.0f, 0.0f, 3.f + i * 30.f));
    }

    skybox::setDevice(m_Device);
    skybox::initialize();

    const float waterSize = 65.f;
    const float cellSpacing = 1.75f;
    m_Water.setDevice(m_Device);
    m_Water.create(waterSize, cellSpacing);
}

void LightScattering::closeup() noexcept
{
    m_Water.destroy();
    m_Cube.destroy();
    m_Sphere.destroy();
    m_SphereMini.destroy();
    m_Ground.destroy();
    m_Column.destroy();
    m_Dragon.destroy();
    m_ScreenTraingle.destroy();
	profiler::shutdown();
    skybox::shutdown();
}

void LightScattering::update() noexcept
{
    m_Timer.update();
    m_Camera.setFov(m_Settings.fov);
    bool bCameraUpdated = m_Camera.update();

    static int32_t preWidth = 0;
    static int32_t preHeight = 0;

    int32_t width = getFrameWidth();
    int32_t height = getFrameHeight();
    bool bResized = false;
    if (preWidth != width || preHeight != height)
    {
        preWidth = width, preHeight = height;
        bResized = true;
    }
    m_Settings.bUpdated = (m_Settings.bUiChanged || bCameraUpdated || bResized);

    WaterOptions waterOpts;
    waterOpts.WaveMapScale = 2.5f;
    waterOpts.WaterColor = m_Settings.m_WaterColor;
    waterOpts.SunColor = glm::vec4( 1.0f, 0.8f, 0.4f, 1.0f );
    waterOpts.SunDirection = GetSunDirection();
    waterOpts.SunFactor = 1.5f;
    waterOpts.SunPower = 100.0f;

    static float elapsed = 0.f;
    float delta = float(glfwGetTime()) - elapsed;
    m_Water.update(delta, waterOpts);
    elapsed += delta;

    Graphics::CalcOrthoProjections(m_Settings, m_Camera, m_DirectionalLight.Direction, m_CascadeEnd, m_lightSpace);
}

void LightScattering::updateHUD() noexcept
{
    bool bUpdated = false;
    float width = (float)getWindowWidth(), height = (float)getWindowHeight();

    ImGui::SetNextWindowPos(
        ImVec2(width - width / 4.f - 10.f, 10.f),
        ImGuiSetCond_FirstUseEver);
    ImGui::Begin("Settings",
        NULL,
        ImVec2(width / 4.0f, height - 20.0f),
        ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::PushItemWidth(180.0f);
    ImGui::Indent();
    {
        // global
        {
            ImGui::Text("CPU %s: %10.5f ms\n", "Main", s_CpuTick);
            ImGui::Text("GPU %s: %10.5f ms\n", "Main", s_GpuTick);
        }
        ImGui::Separator();
        {
            bUpdated |= ImGui::SliderFloat("Sun Angle", &m_Settings.angle, -180.f, 180.f);
            bUpdated |= ImGui::SliderFloat("Fov", &m_Settings.fov, 15.f, 120.f);
            ImGui::ColorWheel("Color:", glm::value_ptr(m_Settings.m_WaterColor), 0.6f);
            bUpdated |= ImGui::Checkbox("Bound Sphere", &m_Settings.bBoundSphere);
            bUpdated |= ImGui::Checkbox("Reduce Shimmer", &m_Settings.bReduceShimmer);
            ImGui::Separator();
            bUpdated |= ImGui::SliderFloat("Debug Light type", &m_Settings.debugType, 0.f, 3.f);
            ImGui::Separator();
            bUpdated |= ImGui::Checkbox("Debug depth", &m_Settings.bDebugDepth);
            bUpdated |= ImGui::SliderFloat("Debug depth index", &m_Settings.depthIndex, 0.f, 2.f);
            ImGui::Separator();
            bUpdated |= ImGui::Checkbox("Automatic Split using LogUniform", &m_Settings.bClipSplitLogUniform);
            if (!m_Settings.bClipSplitLogUniform)
            {
                bUpdated |= ImGui::SliderFloat("Slice1", &m_Settings.Slice1, m_Camera.getNear(), m_Settings.Slice2);
                bUpdated |= ImGui::SliderFloat("Slice2", &m_Settings.Slice2, m_Settings.Slice1, m_Settings.Slice3);
                bUpdated |= ImGui::SliderFloat("Slice3", &m_Settings.Slice3, m_Settings.Slice2, m_Camera.getFar());
            }
        }
    }
    ImGui::Unindent();
    ImGui::Separator();
    ImGui::Text("Post processing:");
    ImGui::Indent();
    ImGui::SliderFloat("Lambda", &m_Settings.lambda, 0.0f, 1.0f);
    ImGui::Unindent();
    ImGui::End();

    m_Settings.bUiChanged = bUpdated;
}

void LightScattering::render() noexcept
{
    GraphicsContext context(GraphicsDeviceTypeOpenGLCore);
    profiler::start(kProfilerTypeRender);

    ShadowMapPass(context);
    if (m_Settings.bDebugDepth) RenderDebugDepth(context);
    else RenderPass(context);
    m_Water.render(context, m_Camera);
    TonemapPass(context);

    profiler::stop(kProfilerTypeRender);
    profiler::tick(kProfilerTypeRender, s_CpuTick, s_GpuTick);
}

void LightScattering::ShadowMapPass(GraphicsContext& gfxContext)
{
    m_ShadowMapShader.bind();
    // depth clamping so that the shadow maps keep from moving 
    // through objects which causes shadows to disappear.
    gfxContext.SetDepthClamp(true);
    gfxContext.SetCullFace(kCullFront);
    for (uint32_t i = 0; i < Graphics::g_NumShadowCascade; i++)
    {
        gfxContext.SetFramebuffer(Graphics::g_ShadowMapFramebuffer[i]);
        gfxContext.SetViewport(0, 0, Graphics::g_ShadowMapSize, Graphics::g_ShadowMapSize);
        gfxContext.ClearDepth(1.0f);
        gfxContext.Clear(kDepthBufferBit);
        m_ShadowMapShader.setUniform("uMatLightSpace", GetLightSpaceMatrix(i));
        RenderScene(m_ShadowMapShader);
    }
    gfxContext.SetDepthClamp(false);
    gfxContext.SetCullFace(kCullBack);
}

void LightScattering::RenderDebugDepth(GraphicsContext& gfxContext)
{
    gfxContext.SetFramebuffer(Graphics::g_MainFramebuffer);
    gfxContext.SetViewport(0, 0, Graphics::g_NativeWidth, Graphics::g_NativeHeight); 
    gfxContext.ClearColor(glm::vec4(0, 0, 0, 0));
    gfxContext.ClearDepth(1.0f);
    gfxContext.Clear(kColorBufferBit | kDepthBufferBit);

    m_DebugDepthShader.bind();
    m_DebugDepthShader.setUniform("ubOrthographic", true);
    m_DebugDepthShader.bindTexture("uTexShadowmap", Graphics::g_ShadowMap[int(m_Settings.depthIndex)], 0);
    m_ScreenTraingle.draw();
}

void LightScattering::RenderPass(GraphicsContext& gfxContext)
{
    gfxContext.SetFramebuffer(Graphics::g_MainFramebuffer);
    gfxContext.SetViewport(0, 0, Graphics::g_NativeWidth, Graphics::g_NativeHeight); 
    gfxContext.ClearColor(glm::vec4(0, 0, 0, 0));
    gfxContext.ClearDepth(1.0f);
    gfxContext.Clear(kColorBufferBit | kDepthBufferBit);

    skybox::render(gfxContext, m_Camera);

    glm::mat4 view = m_Camera.getViewMatrix();
    glm::mat4 project = m_Camera.getProjectionMatrix();

    m_LightShader.bind();
    m_LightShader.setUniform("uDebugType", int32_t(m_Settings.debugType));
    m_LightShader.setUniform("uDirectionalLight.Base.Color", m_DirectionalLight.Color);
    m_LightShader.setUniform("uDirectionalLight.Base.AmbientIntensity", m_DirectionalLight.AmbientIntensity);
    m_LightShader.setUniform("uDirectionalLight.Direction", glm::normalize(m_DirectionalLight.Direction));
    m_LightShader.setUniform("uDirectionalLight.Base.DiffuseIntensity", m_DirectionalLight.DiffuseIntensity);
    m_LightShader.setUniform("uMatView", view);
    m_LightShader.setUniform("uMatProject", project);
    for (uint32_t i = 0; i < Graphics::g_NumShadowCascade; i++)
    {
        m_LightShader.setUniform(util::format("uCascadeEndClipSpace[{0}]", i), m_CascadeEnd[i]);
        m_LightShader.setUniform(util::format("uMatLight[{0}]", i), GetLightSpaceMatrix(i));
        m_LightShader.bindTexture(util::format("uTexShadowmap[{0}]", i), Graphics::g_ShadowMap[i], i+1);
    }
    m_LightShader.setUniform("uEyePositionWS", m_Camera.getPosition());
    m_LightShader.bindTexture("uTexWood", m_TexWood, 0);
    RenderScene(m_LightShader);
}

void LightScattering::TonemapPass(GraphicsContext& gfxContext)
{
    gfxContext.SetFramebuffer(nullptr);
    gfxContext.SetViewport(0, 0, getFrameWidth(), getFrameHeight());
    gfxContext.SetDepthTest(false);
    m_PostProcessHDRShader.bind();
    m_PostProcessHDRShader.bindTexture("uTexSource", Graphics::g_SceneMap, 0);
    m_ScreenTraingle.draw();
    gfxContext.SetDepthTest(true);
}

void LightScattering::RenderScene(const ProgramShader& shader)
{
    shader.setUniform("uMatModel", glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f)));
    m_Column.render();
    shader.setUniform("uMatModel", glm::scale(glm::mat4(1.f), glm::vec3(100.f)));
    // m_Dragon.render();
}

glm::vec3 LightScattering::GetSunDirection() const
{
    const float angle = glm::radians(m_Settings.angle);
    glm::vec3 sunDir = glm::vec3(glm::cos(angle), -1.0f, glm::sin(angle));
    return glm::normalize(sunDir);
}

glm::mat4 LightScattering::GetLightSpaceMatrix(uint32_t i) const
{
    return m_lightSpace[i];
}

void LightScattering::keyboardCallback(uint32_t key, bool isPressed) noexcept
{
	switch (key)
	{
	case GLFW_KEY_W:
		m_Camera.keyboardHandler(MOVE_FORWARD, isPressed);
		break;

	case GLFW_KEY_S:
		m_Camera.keyboardHandler(MOVE_BACKWARD, isPressed);
		break;

	case GLFW_KEY_A:
		m_Camera.keyboardHandler(MOVE_LEFT, isPressed);
		break;

	case GLFW_KEY_D:
		m_Camera.keyboardHandler(MOVE_RIGHT, isPressed);
		break;
	}
}

void LightScattering::framesizeCallback(int32_t width, int32_t height) noexcept
{
	float aspectRatio = (float)width/height;
	m_Camera.setProjectionParams(m_Settings.fov, aspectRatio, 1.f, 4000.f);

    Graphics::initializeRenderingBuffers(m_Device, width, height); 
    Graphics::resizeDisplayDependentBuffers(width, height); 
}

void LightScattering::motionCallback(float xpos, float ypos, bool bPressed) noexcept
{
	const bool mouseOverGui = ImGui::MouseOverArea();
	if (!mouseOverGui && bPressed) m_Camera.motionHandler(int(xpos), int(ypos), false);    
}

void LightScattering::mouseCallback(float xpos, float ypos, bool bPressed) noexcept
{
	const bool mouseOverGui = ImGui::MouseOverArea();
	if (!mouseOverGui && bPressed) m_Camera.motionHandler(int(xpos), int(ypos), true); 
}

GraphicsDevicePtr LightScattering::createDevice(const GraphicsDeviceDesc& desc) noexcept
{
	GraphicsDeviceType deviceType = desc.getDeviceType();

#if __APPLE__
	assert(deviceType != GraphicsDeviceType::GraphicsDeviceTypeOpenGLCore);
#endif

	if (deviceType == GraphicsDeviceType::GraphicsDeviceTypeOpenGL ||
		deviceType == GraphicsDeviceType::GraphicsDeviceTypeOpenGLCore)
    {
        auto device = std::make_shared<OGLDevice>();
        if (device->create(desc))
            return device;
        return nullptr;
    }
    return nullptr;
}
