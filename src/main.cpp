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

#include <GLType/OGLTexture.h>
#include <GLType/OGLCoreTexture.h>

#include <GLType/OGLFramebuffer.h>
#include <GLType/OGLCoreFramebuffer.h>

#include <BufferManager.h>
#include <GraphicsTypes.h>
#include <PhaseFunctions.h>
#include <Mesh.h>

#include <fstream>
#include <memory>
#include <vector>
#include <algorithm>
#include <GameCore.h>

enum ProfilerType { kProfilerTypeRender = 0, kProfilerTypeUpdate };
enum SkyTextureFile { kHelipad = 0, kNewport };

namespace 
{
    float s_CpuTick = 0.f;
    float s_GpuTick = 0.f;
}

enum EnumSkyModel { kNishita = 0, kTimeOfDay, kTimeOfNight, };

struct SceneSettings
{
    bool bUiChanged = false;
    bool bResized = false;
    bool bUpdated = true;
	bool bChapman = true;
    bool bUpdateLight = true;
	float m_exposure = 0.f;
	float m_radianceSlider = 2.f;
	float m_bgType = 7.f;
    float angle = 76.f;
    float altitude = 1.f;
    float fov = 45.f;
    glm::vec3 m_rgbDiff = glm::vec3(0.2f, 0.2f, 0.2f);

    EnumSkyModel kModel = kTimeOfNight;

    // Nishita Sky model
    bool bCPU = false;
    FloatSetting sunTurbidityParams {"Sun Turbidity", glm::vec3(-7.f, -9.f, -4.f)};

    // Time of Day
    FloatSetting cloudSpeedParams = {"Cloud Speed", glm::vec3(0.05, 0.0, 1.0)};
    FloatSetting cloudDensityParams = {"Cloud Density", glm::vec3(400, 0.0, 1600.0)};
    // Sun Radius, How much size that simulates the sun size
    FloatSetting sunRaidusParams {"Sun Radius", glm::vec3(5000, 100000, 100)};
    // Sun light power, 10.0 is normal
    FloatSetting sunRadianceParams {"Sun Radiance", glm::vec3(10, 1.0, 20.0)}; 	
    FloatSetting sunTurbidity2Params {"Sun Turbidity", glm::vec3(100.f, 1e-5f, 1000)};

    // Time of night
    FloatSetting moonRadianceParams {"Moon Radiance", glm::vec3(5.0, 1.0, 10.0)}; 	
    FloatSetting moonTurbidityParams {"Moon Turbidity", glm::vec3(200.f, 1e-5f, 500)};
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
    ProgramShader m_FlatShader;
    ProgramShader m_PostProcessHDRShader;
    GraphicsTexturePtr m_ScreenColorTex;
    GraphicsFramebufferPtr m_ColorRenderTarget;
    GraphicsDevicePtr m_Device;
};

CREATE_APPLICATION(LightScattering);

LightScattering::LightScattering() noexcept :
    m_Sphere(32, 1.0e2f),
    m_SphereMini(48, 5.0f)
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

	m_FlatShader.setDevice(m_Device);
	m_FlatShader.initialize();
	m_FlatShader.addShader(GL_VERTEX_SHADER, "Flat.Vertex");
	m_FlatShader.addShader(GL_FRAGMENT_SHADER, "Flat.Fragment");
	m_FlatShader.link();

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
            ImGui::Separator();

            ImGui::Text("Sky Models:");
            int kModel = m_Settings.kModel;
            ImGui::RadioButton("Nishita", &kModel, 0);
            ImGui::RadioButton("Time of Day", &kModel, 1);
            ImGui::RadioButton("Time of Night", &kModel, 2);
            m_Settings.kModel = EnumSkyModel(kModel);
        }
        ImGui::Separator();
        {
            bUpdated |= ImGui::SliderFloat("Sun Angle", &m_Settings.angle, 0.f, 120.f);
            bUpdated |= m_Settings.sunRaidusParams.updateGUI();
            bUpdated |= ImGui::SliderFloat("Altitude (km)", &m_Settings.altitude, 0.f, 100.f);
            bUpdated |= ImGui::SliderFloat("Fov", &m_Settings.fov, 15.f, 120.f);
        }
        ImGui::Separator();
        if (m_Settings.kModel == kNishita)
        {
            bUpdated |= ImGui::Checkbox("Use  approximation", &m_Settings.bChapman);
            bUpdated |= m_Settings.sunRadianceParams.updateGUI();
            bUpdated |= m_Settings.sunTurbidityParams.updateGUI();
        }
        if (m_Settings.kModel == kTimeOfDay)
        {
            bUpdated |= m_Settings.cloudSpeedParams.updateGUI();
            bUpdated |= m_Settings.cloudDensityParams.updateGUI();
            bUpdated |= m_Settings.sunRadianceParams.updateGUI();
            bUpdated |= m_Settings.sunTurbidity2Params.updateGUI();
        }
        if (m_Settings.kModel == kTimeOfNight)
        {
            bUpdated |= m_Settings.moonRadianceParams.updateGUI();
            bUpdated |= m_Settings.moonTurbidityParams.updateGUI();
        }
    }
    ImGui::Unindent();
    ImGui::Separator();
    ImGui::Text("Background:");
    ImGui::Indent();
    {
        ImGui::Indent();
        ImGui::Checkbox("Update Lightcube", &m_Settings.bUpdateLight);
        ImGui::Unindent();
    }
    {
        int32_t selection;
        if (0.0f == m_Settings.m_bgType)
        {
            selection = UINT8_C(0);
        }
        else if (7.0f == m_Settings.m_bgType)
        {
            selection = UINT8_C(2);
        }
        else
        {
            selection = UINT8_C(1);
        }

        float tabWidth = ImGui::GetContentRegionAvailWidth() / 3.0f;
        if (ImGui::TabButton("Skybox", tabWidth, selection == 0))
        {
            selection = 0;
        }

        ImGui::SameLine(0.0f, 0.0f);
        if (ImGui::TabButton("Radiance", tabWidth, selection == 1))
        {
            selection = 1;
        }

        ImGui::SameLine(0.0f, 0.0f);
        if (ImGui::TabButton("Irradiance", tabWidth, selection == 2))
        {
            selection = 2;
        }

        if (0 == selection) m_Settings.m_bgType = 0.0f;
        else if (2 == selection) m_Settings.m_bgType = 7.0f;
        else m_Settings.m_bgType = m_Settings.m_radianceSlider;

        const bool isRadiance = (selection == 1);
        if (isRadiance)
            ImGui::SliderFloat("Mip level", &m_Settings.m_radianceSlider, 1.0f, 6.0f);
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

enum FrontFaceType { kCountClockWise = 0, kClockWise };
enum ClearBitType
{
    kColorBufferBit = 1,
    kDepthBufferBit = 2,
    kStencilBufferBit = 4,
    kAccumBufferBit = 8
};

class GraphicsContext
{
public:

    GraphicsContext(GraphicsDeviceType type);

    void Clear(uint8_t flags);
    void ClearColor(glm::vec4 color);
    void ClearDepth(float depth);

    void SetViewport(int x, int y, size_t width, size_t height);
    void SetFrontFace(FrontFaceType flag);
    void SetDepthTest(bool bFlag);
    void SetCubemapSeamless(bool bFlag);
    void SetFramebuffer(const GraphicsFramebufferPtr& framebuffer) noexcept;

    GraphicsDeviceType m_DeviceType;
};

GraphicsContext::GraphicsContext(GraphicsDeviceType type) 
    :  m_DeviceType(type) 
{
}

void GraphicsContext::Clear(uint8_t flags)
{
    GLbitfield mask = 0;
    if (flags & kColorBufferBit)
        mask |= GL_COLOR_BUFFER_BIT;
    if (flags & kDepthBufferBit)
        mask |= GL_DEPTH_BUFFER_BIT;
    if (flags & kStencilBufferBit)
        mask |= GL_STENCIL_BUFFER_BIT;
    if (flags & kAccumBufferBit)
        mask |= GL_ACCUM_BUFFER_BIT;
    glClear(mask);
}

void GraphicsContext::ClearColor(glm::vec4 color)
{
    glClearColor(color.r, color.g, color.b, color.a);
}

void GraphicsContext::ClearDepth(float depth)
{
    glClearDepthf(depth);
}

void GraphicsContext::SetViewport(int x, int y, size_t width, size_t height)
{
    glViewport(x, y, width, height);
}

void GraphicsContext::SetFrontFace(FrontFaceType flag)
{
    glFrontFace((flag == kCountClockWise) ? GL_CCW : GL_CW);
}

void GraphicsContext::SetDepthTest(bool bFlag)
{
    const auto func = bFlag ? glEnable : glDisable;
    func(GL_DEPTH_TEST);
}

void GraphicsContext::SetCubemapSeamless(bool bFlag)
{
    const auto func = bFlag ? glEnable : glDisable;
    func(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

void GraphicsContext::SetFramebuffer(const GraphicsFramebufferPtr& framebuffer) noexcept
{
    assert(framebuffer);

    if (!framebuffer)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }

    if (m_DeviceType == GraphicsDeviceType::GraphicsDeviceTypeOpenGLCore)
    {
        auto fbo = framebuffer->downcast_pointer<OGLCoreFramebuffer>();
        if (fbo) fbo->bind();
    }
    else if (m_DeviceType == GraphicsDeviceType::GraphicsDeviceTypeOpenGL)
    {
        auto fbo = framebuffer->downcast_pointer<OGLFramebuffer>();
        if (fbo) fbo->bind();
    }
}


void LightScattering::render() noexcept
{
    GraphicsContext context(GraphicsDeviceTypeOpenGLCore);
    profiler::start(kProfilerTypeRender);

    const auto matViewInverse = glm::inverse(m_Camera.getViewMatrix());
    const auto matProjectInverse = glm::inverse(m_Camera.getProjectionMatrix());
    const auto matViewProjectInverse = glm::inverse(m_Camera.getViewProjMatrix());

    auto& desc = m_ScreenColorTex->getGraphicsTextureDesc();

    context.SetFramebuffer(m_ColorRenderTarget);
    context.SetViewport(0, 0, desc.getWidth(), desc.getHeight());
    context.ClearColor(glm::vec4(1, 0, 0, 0));
    context.ClearDepth(1.0f);
    context.Clear(kColorBufferBit | kDepthBufferBit);

    // Tone mapping
    {
        context.SetFramebuffer(nullptr);

        context.SetViewport(0, 0, getFrameWidth(), getFrameHeight());
        context.SetDepthTest(false);
        m_PostProcessHDRShader.bind();
        m_PostProcessHDRShader.bindTexture("uTexSource", m_ScreenColorTex, 0);
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

    GraphicsTextureDesc colorDesc;
    colorDesc.setWidth(width);
    colorDesc.setHeight(height);
    colorDesc.setFormat(gli::FORMAT_RGBA16_SFLOAT_PACK16);
    m_ScreenColorTex = m_Device->createTexture(colorDesc);

    GraphicsTextureDesc depthDesc;
    depthDesc.setWidth(width);
    depthDesc.setHeight(height);
    depthDesc.setFormat(gli::FORMAT_D24_UNORM_S8_UINT_PACK32);
    auto depthTex = m_Device->createTexture(depthDesc);

    GraphicsFramebufferDesc desc;  
    desc.addComponent(GraphicsAttachmentBinding(m_ScreenColorTex, GL_COLOR_ATTACHMENT0));
    desc.addComponent(GraphicsAttachmentBinding(depthTex, GL_DEPTH_ATTACHMENT));
    
    m_ColorRenderTarget = m_Device->createFramebuffer(desc);;
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
