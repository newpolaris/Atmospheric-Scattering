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
};

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

    glm::vec3 GetSunDirection() const;

    std::vector<glm::vec2> m_Samples;
    SceneSettings m_Settings;
	TCamera m_Camera;
    SimpleTimer m_Timer;
    CubeMesh m_Cube;
    SphereMesh m_Sphere;
    SphereMesh m_SphereMini;
    FullscreenTriangleMesh m_ScreenTraingle;
    ProgramShader m_PostProcessHDRShader;
    GraphicsTexturePtr m_ScreenColorTex;
    GraphicsDevicePtr m_Device;
};

CREATE_APPLICATION(LightScattering);

LightScattering::LightScattering() noexcept :
    m_Sphere(32, 1.0e2f),
    m_SphereMini(48, 1.0f)
{
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

	m_PostProcessHDRShader.setDevice(m_Device);
	m_PostProcessHDRShader.initialize();
	m_PostProcessHDRShader.addShader(GL_VERTEX_SHADER, "PostProcessHDR.Vertex");
	m_PostProcessHDRShader.addShader(GL_FRAGMENT_SHADER, "PostProcessHDR.Fragment");
	m_PostProcessHDRShader.link();

    m_ScreenTraingle.create();
    m_Cube.create();
    m_Sphere.create();
    m_SphereMini.create();
}

void LightScattering::closeup() noexcept
{
    m_Cube.destroy();
    m_Sphere.destroy();
    m_SphereMini.destroy();
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

    const auto matViewInverse = glm::inverse(m_Camera.getViewMatrix());
    const auto matProjectInverse = glm::inverse(m_Camera.getProjectionMatrix());
    const auto matViewProjectInverse = glm::inverse(m_Camera.getViewProjMatrix());

    context.SetFramebuffer(Graphics::g_MainFramebuffer);
    context.SetViewport(0, 0, Graphics::g_NativeWidth, Graphics::g_NativeHeight); 
    
    context.ClearColor(glm::vec4(1, 0, 0, 0));
    context.ClearDepth(1.0f);
    context.Clear(kColorBufferBit | kDepthBufferBit);

    // Tone mapping
    {
        context.SetFramebuffer(nullptr);

        context.SetViewport(0, 0, getFrameWidth(), getFrameHeight());
        context.SetDepthTest(false);
        m_PostProcessHDRShader.bind();
        m_PostProcessHDRShader.bindTexture("uTexSource", Graphics::g_SceneMap, 0);
        m_ScreenTraingle.draw();
        context.SetDepthTest(true);
    }
    profiler::stop(kProfilerTypeRender);
    profiler::tick(kProfilerTypeRender, s_CpuTick, s_GpuTick);
}

glm::vec3 LightScattering::GetSunDirection() const
{
    const float angle = glm::radians(m_Settings.angle);
    glm::vec3 sunDir = glm::vec3(0.0f, glm::cos(angle), -glm::sin(angle));
    return glm::normalize(sunDir);
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
	m_Camera.setProjectionParams(45.0f, aspectRatio, 1.0f, 10000.f);

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
