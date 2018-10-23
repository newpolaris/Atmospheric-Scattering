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

#include <BufferManager.h>
#include <GraphicsTypes.h>
#include <PhaseFunctions.h>
#include <Mesh.h>
#include <LightingTechnique.h>

#include <fstream>
#include <memory>
#include <vector>
#include <algorithm>
#include <GameCore.h>
#include <GraphicsContext.h>

enum ProfilerType { kProfilerTypeRender = 0, kProfilerTypeUpdate };

namespace 
{
    float s_CpuTick = 0.f;
    float s_GpuTick = 0.f;
}

struct SceneSettings
{
    bool bUiChanged = false;
    bool bResized = false;
    bool bUpdated = true;
	float m_exposure = 0.f;
    float angle = 76.f;
    float fov = 45.f;
    glm::vec3 lightDirection = glm::vec3(1.f, -1.f, 0.f);
    glm::vec3 position = glm::vec3(-20.0f, 20.0f, 5.0f);
};


glm::mat4 OrthoProjectTransform(float l, float r, float t, float b, float n, float f)
{
    float X = 2.f / (r - l);
    float Y = 2.f / (t - b);
    float Z = 2.f / (f - n);
    float Tx = -(r + l)/(r - l);
    float Ty = -(t + b)/(t - b);
    float Tz = -(f + n)/(f - n);

    return glm::mat4(
        X,   0.f,  0.f,  Tx,
        0.f,   Y,  0.f,  Ty,
        0.f, 0.f,    Z,  Tz,
        0.f, 0.f,  0.f, 1.f);
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
    void RenderPass(GraphicsContext& gfxContext);
    void TonemapPass(GraphicsContext& gfxContext);

    glm::vec3 GetSunDirection() const;
    glm::mat4 GetShadowMatrix() const;

    std::vector<glm::vec2> m_Samples;
    SceneSettings m_Settings;
	TCamera m_Camera;
    SimpleTimer m_Timer;
    DirectionalLight m_DirectionalLight;
    SpotLight m_SpotLight;
    CubeMesh m_Cube;
    SphereMesh m_Sphere;
    SphereMesh m_SphereMini;
    PlaneMesh m_Ground;
    FullscreenTriangleMesh m_ScreenTraingle;
    LightingTechnique m_LightTechnique;
    ProgramShader m_FlatShader;
    ProgramShader m_ShadowMapShader;
    ProgramShader m_PostProcessHDRShader;
    GraphicsTexturePtr m_ScreenColorTex;
    GraphicsDevicePtr m_Device;
};

CREATE_APPLICATION(LightScattering);

LightScattering::LightScattering() noexcept :
    m_Sphere(32, 1.0e2f),
    m_SphereMini(48, 2.0f)
{
    m_SpotLight.AmbientIntensity = 0.0f;
    m_SpotLight.DiffuseIntensity = 0.9f;
    m_SpotLight.Color = glm::vec3(1.0f);
    m_SpotLight.Attenuation.x = 0.5f;
    m_SpotLight.Attenuation.y = 0.0f;
    m_SpotLight.Attenuation.z = 0.0f;
    m_SpotLight.Position = glm::vec3(-20.0f, 20.0f, 5.0f);
    m_SpotLight.Direction = glm::vec3(1.f, -1.f, 0.f);
    m_SpotLight.Cutoff = 20.0f;

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
	m_Camera.setMoveCoefficient(0.35f);
	m_Camera.setViewParams(glm::vec3(8.0f, 8.0f, 15.0f), glm::vec3(8.0f, 8.0f, 0.0f));

	GraphicsDeviceDesc deviceDesc;
#if __APPLE__
	deviceDesc.setDeviceType(GraphicsDeviceType::GraphicsDeviceTypeOpenGL);
#else
	deviceDesc.setDeviceType(GraphicsDeviceType::GraphicsDeviceTypeOpenGLCore);
#endif
	m_Device = createDevice(deviceDesc);

    m_ShadowMapShader.setDevice(m_Device);
    m_ShadowMapShader.initialize();
    m_ShadowMapShader.addShader(GL_VERTEX_SHADER, "ShadowMapVS.glsl");
    m_ShadowMapShader.link();

    m_FlatShader.setDevice(m_Device);
    m_FlatShader.initialize();
    m_FlatShader.addShader(GL_VERTEX_SHADER, "Flat.Vertex");
    m_FlatShader.addShader(GL_FRAGMENT_SHADER, "Flat.Fragment");
    m_FlatShader.link();

    m_LightTechnique.setDevice(m_Device);
    m_LightTechnique.initialize();

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
}

void LightScattering::closeup() noexcept
{
    m_Cube.destroy();
    m_Sphere.destroy();
    m_SphereMini.destroy();
    m_Ground.destroy();
    m_ScreenTraingle.destroy();
	profiler::shutdown();
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
            bUpdated |= ImGui::SliderFloat("Sun Angle", &m_Settings.angle, 0.f, 120.f);
            bUpdated |= ImGui::SliderFloat("Fov", &m_Settings.fov, 15.f, 120.f);
            bUpdated |= ImGui::SliderFloat("Fov", &m_Settings.fov, 15.f, 120.f);
            ImGui::Separator();
            bUpdated |= ImGui::SliderFloat("Light X", &m_Settings.lightDirection.x, -1.f, 1.f);
            bUpdated |= ImGui::SliderFloat("Light Y", &m_Settings.lightDirection.y, -1.f, 1.f);
            bUpdated |= ImGui::SliderFloat("Light Z", &m_Settings.lightDirection.z, -1.f, 1.f);
            ImGui::Separator();
            bUpdated |= ImGui::SliderFloat("Position X", &m_Settings.position.x, -20.f, 20.f);
            bUpdated |= ImGui::SliderFloat("Position Y", &m_Settings.position.y, -20.f, 20.f);
            bUpdated |= ImGui::SliderFloat("Position Z", &m_Settings.position.z, -20.f, 20.f);
        }
    }
    ImGui::Unindent();
    ImGui::Separator();
    ImGui::Text("Post processing:");
    ImGui::Indent();
    ImGui::SliderFloat("Exposure", &m_Settings.m_exposure, -4.0f, 4.0f);
    ImGui::Unindent();
    ImGui::End();

    m_Settings.bUiChanged = bUpdated;
}

void LightScattering::render() noexcept
{
    GraphicsContext context(GraphicsDeviceTypeOpenGLCore);
    profiler::start(kProfilerTypeRender);

    m_DirectionalLight.Direction = m_Settings.lightDirection;
    m_SpotLight.Direction = m_Settings.lightDirection;
    m_SpotLight.Position = m_Settings.position;

    ShadowMapPass(context);
    RenderPass(context);
    TonemapPass(context);

    profiler::stop(kProfilerTypeRender);
    profiler::tick(kProfilerTypeRender, s_CpuTick, s_GpuTick);
}

void LightScattering::ShadowMapPass(GraphicsContext& gfxContext)
{
    gfxContext.SetFramebuffer(Graphics::g_ShadowMapFramebuffer);
    gfxContext.SetViewport(0, 0, Graphics::g_ShadowMapSize, Graphics::g_ShadowMapSize); 
    gfxContext.ClearDepth(1.0f);
    gfxContext.Clear(kDepthBufferBit);

    m_ShadowMapShader.bind();
    m_ShadowMapShader.setUniform("uMatShadow", GetShadowMatrix());
    m_Cube.draw();
    m_SphereMini.draw();
    m_ShadowMapShader.unbind();
}

void LightScattering::RenderPass(GraphicsContext& gfxContext)
{
    gfxContext.SetFramebuffer(Graphics::g_MainFramebuffer);
    gfxContext.SetViewport(0, 0, Graphics::g_NativeWidth, Graphics::g_NativeHeight); 
    gfxContext.ClearColor(glm::vec4(0, 0, 0, 0));
    gfxContext.ClearDepth(1.0f);
    gfxContext.Clear(kColorBufferBit | kDepthBufferBit);

    glm::mat4 matWorld(1.0);
    glm::mat4 matWorldViewProject = m_Camera.getViewProjMatrix()*matWorld;

    m_LightTechnique.bind();
    m_LightTechnique.setDirectionalLight(m_DirectionalLight);
    m_LightTechnique.setMatWorld(matWorld);
    m_LightTechnique.setMatWorldViewProject(matWorldViewProject);
    m_LightTechnique.setMatLight(GetShadowMatrix());
    m_LightTechnique.setEyePositionWS(m_Camera.getPosition());
    m_LightTechnique.setShadowMap(Graphics::g_ShadowMap);
    m_Ground.draw();
    m_Cube.draw();
    m_SphereMini.draw();
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

glm::vec3 LightScattering::GetSunDirection() const
{
    const float angle = glm::radians(m_Settings.angle);
    glm::vec3 sunDir = glm::vec3(0.0f, glm::cos(angle), -glm::sin(angle));
    return glm::normalize(sunDir);
}

glm::mat4 LightScattering::GetShadowMatrix() const
{
#if 0
    glm::mat4 view = glm::lookAtRH(m_SpotLight.Position, m_SpotLight.Position + m_SpotLight.Direction, glm::vec3(0, 1, 0));
    glm::mat4 project = glm::perspectiveFovRH_NO(glm::radians(60.f), float(Graphics::g_NativeWidth), float(Graphics::g_NativeHeight), 0.1f, 100.f);
    glm::mat4 matShadow = project * view;
#else
    glm::mat4 view = glm::lookAtRH(glm::vec3(0.f), m_DirectionalLight.Direction, glm::vec3(0, 1, 0));
    glm::mat4 project = glm::orthoRH_NO(-20.f, 20.f, -20.f, 20.f, -10.f, 50.f);
    glm::mat4 matShadow = project * view;
#endif
    return matShadow;
}

void LightScattering::keyboardCallback(uint32_t key, bool isPressed) noexcept
{
	switch (key)
	{
	case GLFW_KEY_UP:
		m_Camera.keyboardHandler(MOVE_FORWARD, isPressed);
		break;

	case GLFW_KEY_DOWN:
		m_Camera.keyboardHandler(MOVE_BACKWARD, isPressed);
		break;

	case GLFW_KEY_LEFT:
		m_Camera.keyboardHandler(MOVE_LEFT, isPressed);
		break;

	case GLFW_KEY_RIGHT:
		m_Camera.keyboardHandler(MOVE_RIGHT, isPressed);
		break;
	}
}

void LightScattering::framesizeCallback(int32_t width, int32_t height) noexcept
{
	float aspectRatio = (float)width/height;
	m_Camera.setProjectionParams(45.0f, aspectRatio, 0.1f, 100.f);

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
