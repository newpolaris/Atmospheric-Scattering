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
#include <GLType/OGLCoreFramebuffer.h>

#include <GraphicsTypes.h>
#include <SkyBox.h>
#include <PhaseFunctions.h>
#include <Mesh.h>

#include <fstream>
#include <memory>
#include <vector>
#include <algorithm>
#include <GameCore.h>
#include "Atmosphere.h"

enum ProfilerType { kProfilerTypeRender = 0 };
enum SkyTextureFile { kHelipad = 0, kNewport };

namespace 
{
    float s_CpuTick = 0.f;
    float s_GpuTick = 0.f;
}

enum EnumSkyModel { kNishita = 0, kTimeOfDay, kTimeOfNight, };

struct SceneSettings
{
    bool bProfile = true;
    bool bUiChanged = false;
    bool bResized = false;
    bool bUpdated = true;
	bool bChapman = true;
    float angle = 76.f;
    float altitude = 1.f;
    float fov = 45.f;

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

    void updateImage(SkyTextureFile texture);
    void renderCloud() noexcept;
    void renderSkybox() noexcept;

    std::vector<glm::vec2> m_Samples;
    SceneSettings m_Settings;
	TCamera m_Camera;
    SimpleTimer m_Timer;
    CubeMesh m_Cube;
    SphereMesh m_Sphere;
    FullscreenTriangleMesh m_ScreenTraingle;
    ProgramShader m_FlatShader;
    ProgramShader m_NishitaSkyShader;
    ProgramShader m_TimeOfDayShader;
    ProgramShader m_TimeOfNightShader;
    ProgramShader m_StarShader;
    ProgramShader m_MoonShader;
    ProgramShader m_BlitShader;
    ProgramShader m_PostProcessHDRShader;
    ProgramShader m_SkyboxShader;
    GraphicsTexturePtr m_SkyboxTex;
    GraphicsTexturePtr m_SkyColorTex;
    GraphicsTexturePtr m_ScreenColorTex;
	GraphicsTexturePtr m_NoiseMapSamp;
	GraphicsTexturePtr m_MilkywaySamp;
	GraphicsTexturePtr m_MoonMapSamp;
    GraphicsFramebufferPtr m_ColorRenderTarget;
    GraphicsDevicePtr m_Device;
};

CREATE_APPLICATION(LightScattering);

LightScattering::LightScattering() noexcept :
    m_Sphere(32, 1.0e2f)
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
	m_Camera.setViewParams(glm::vec3(2.0f, 5.0f, 15.0f), glm::vec3(2.0f, 0.0f, 0.0f));

	GraphicsDeviceDesc deviceDesc;
#if __APPLE__
	deviceDesc.setDeviceType(GraphicsDeviceType::GraphicsDeviceTypeOpenGL);
#else
	deviceDesc.setDeviceType(GraphicsDeviceType::GraphicsDeviceTypeOpenGLCore);
#endif
	m_Device = createDevice(deviceDesc);
	assert(m_Device);

	m_FlatShader.setDevice(m_Device);
	m_FlatShader.initialize();
	m_FlatShader.addShader(GL_VERTEX_SHADER, "Flat.Vertex");
	m_FlatShader.addShader(GL_FRAGMENT_SHADER, "Flat.Fragment");
	m_FlatShader.link();

	m_NishitaSkyShader.setDevice(m_Device);
	m_NishitaSkyShader.initialize();
	m_NishitaSkyShader.addShader(GL_VERTEX_SHADER, "Nishita.Vertex");
	m_NishitaSkyShader.addShader(GL_FRAGMENT_SHADER, "Nishita.Fragment");
	m_NishitaSkyShader.link();

	m_TimeOfDayShader.setDevice(m_Device);
	m_TimeOfDayShader.initialize();
	m_TimeOfDayShader.addShader(GL_VERTEX_SHADER, "Time of day/Time of day.Vertex");
	m_TimeOfDayShader.addShader(GL_FRAGMENT_SHADER, "Time of day/Time of day.Fragment");
	m_TimeOfDayShader.link();

	m_TimeOfNightShader.setDevice(m_Device);
	m_TimeOfNightShader.initialize();
	m_TimeOfNightShader.addShader(GL_VERTEX_SHADER, "Time of night/Time of night.Vertex");
	m_TimeOfNightShader.addShader(GL_FRAGMENT_SHADER, "Time of night/Time of night.Fragment");
	m_TimeOfNightShader.link();

	m_StarShader.setDevice(m_Device);
	m_StarShader.initialize();
	m_StarShader.addShader(GL_VERTEX_SHADER, "Time of night/Stars.Vertex");
	m_StarShader.addShader(GL_FRAGMENT_SHADER, "Time of night/Stars.Fragment");
	m_StarShader.link();

	m_MoonShader.setDevice(m_Device);
	m_MoonShader.initialize();
	m_MoonShader.addShader(GL_VERTEX_SHADER, "Time of night/Moon.Vertex");
	m_MoonShader.addShader(GL_FRAGMENT_SHADER, "Time of night/Moon.Fragment");
	m_MoonShader.link();

	m_BlitShader.setDevice(m_Device);
	m_BlitShader.initialize();
	m_BlitShader.addShader(GL_VERTEX_SHADER, "BlitTexture.Vertex");
	m_BlitShader.addShader(GL_FRAGMENT_SHADER, "BlitTexture.Fragment");
	m_BlitShader.link();

	m_PostProcessHDRShader.setDevice(m_Device);
	m_PostProcessHDRShader.initialize();
	m_PostProcessHDRShader.addShader(GL_VERTEX_SHADER, "PostProcessHDR.Vertex");
	m_PostProcessHDRShader.addShader(GL_FRAGMENT_SHADER, "PostProcessHDR.Fragment");
	m_PostProcessHDRShader.link();

	m_SkyboxShader.setDevice(m_Device);
	m_SkyboxShader.initialize();
	m_SkyboxShader.addShader(GL_VERTEX_SHADER, "Helipad GoldenHour/Sky with box.Vertex");
	m_SkyboxShader.addShader(GL_FRAGMENT_SHADER, "Helipad GoldenHour/Sky with box.Fragment");
	m_SkyboxShader.link();

    m_ScreenTraingle.create();
    m_Cube.create();
    m_Sphere.create();

    GraphicsTextureDesc noise;
    noise.setWrapS(GL_REPEAT);
    noise.setWrapT(GL_REPEAT);
    noise.setMinFilter(GL_LINEAR);
    noise.setMagFilter(GL_LINEAR);
    noise.setFilename("resources/Skybox/cloud.tga");
    m_NoiseMapSamp = m_Device->createTexture(noise);

    GraphicsTextureDesc milkyWay;
    milkyWay.setWrapS(GL_REPEAT);
    milkyWay.setWrapT(GL_REPEAT);
    milkyWay.setMinFilter(GL_NEAREST);
    milkyWay.setMagFilter(GL_NEAREST);
    milkyWay.setFilename("resources/Skybox/milky way.jpg");
    m_MilkywaySamp = m_Device->createTexture(milkyWay);

    GraphicsTextureDesc moon;
    moon.setWrapS(GL_REPEAT);
    moon.setWrapT(GL_REPEAT);
    moon.setMinFilter(GL_LINEAR);
    moon.setMagFilter(GL_LINEAR);
    moon.setFilename("resources/Skybox/moon.jpg");
    m_MoonMapSamp = m_Device->createTexture(moon);

    updateImage(kHelipad);
}

void LightScattering::closeup() noexcept
{
    m_Cube.destroy();
    m_Sphere.destroy();
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
    if (m_Settings.bUpdated && m_Settings.bCPU)
    {
        float angle = glm::radians(m_Settings.angle);
        std::vector<glm::vec4> image(width*height, glm::vec4(0.f));
        glm::vec3 sunDir = glm::vec3(0.0f, glm::cos(angle), -glm::sin(angle));

        Atmosphere atmosphere(sunDir);
        atmosphere.renderSkyDome(image, width, height);

        GraphicsTextureDesc colorDesc;
        colorDesc.setWidth(width);
        colorDesc.setHeight(height);
        colorDesc.setFormat(gli::FORMAT_RGBA32_SFLOAT_PACK32);
        colorDesc.setStream((uint8_t*)image.data());
        colorDesc.setStreamSize(width*height*sizeof(glm::vec4));
        m_SkyColorTex = m_Device->createTexture(colorDesc);
    }
}

void LightScattering::updateImage(SkyTextureFile texture)
{
    const std::vector<std::string> list {
        "resources/skybox/helipad.dds",
    };

    // load the HDR environment map
    GraphicsTextureDesc skyDesc;
    skyDesc.setWrapS(GL_REPEAT);
    skyDesc.setWrapT(GL_REPEAT);
    skyDesc.setMinFilter(GL_LINEAR);
    skyDesc.setMagFilter(GL_LINEAR);
    skyDesc.setFilename(list[texture]);
    m_SkyboxTex = m_Device->createTexture(skyDesc);
    assert(m_SkyboxTex);
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
            bUpdated |= ImGui::Checkbox("Mode CPU", &m_Settings.bCPU);
            bUpdated |= ImGui::Checkbox("Always redraw", &m_Settings.bProfile);
            bUpdated |= ImGui::Checkbox("Use chapman approximation", &m_Settings.bChapman);
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
    ImGui::End();

    m_Settings.bUiChanged = bUpdated;
}

void LightScattering::render() noexcept
{
    profiler::start(kProfilerTypeRender);

    auto& desc = m_ScreenColorTex->getGraphicsTextureDesc();
    m_Device->setFramebuffer(m_ColorRenderTarget);
    GLenum clearFlag = GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT;
    glViewport(0, 0, desc.getWidth(), desc.getHeight());
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepthf(1.0f);
    glClear(clearFlag);

    renderSkybox();
    // renderCloud();

    // Tone mapping
    {
        GraphicsTexturePtr target = m_ScreenColorTex;
        if (m_Settings.bCPU && m_SkyColorTex) 
            target = m_SkyColorTex;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, getFrameWidth(), getFrameHeight());

        glDisable(GL_DEPTH_TEST);
        m_PostProcessHDRShader.bind();
        m_PostProcessHDRShader.bindTexture("uTexSource", target, 0);
        m_ScreenTraingle.draw();
        glEnable(GL_DEPTH_TEST);
    }
    profiler::stop(kProfilerTypeRender);
    profiler::tick(kProfilerTypeRender, s_CpuTick, s_GpuTick);

}

void LightScattering::renderCloud() noexcept
{
    bool bUpdate = m_Settings.bProfile || m_Settings.bUpdated;
    if (!m_Settings.bCPU && bUpdate)
    {
        // [Preetham99]
        const glm::vec3 K = glm::vec3(0.686282f, 0.677739f, 0.663365f); // spectrum
        const glm::vec3 lambda = glm::vec3(680e-9f, 550e-9f, 440e-9f);

        // sky box
        glDisable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);

        const float time = m_Timer.duration();
        const float angle = glm::radians(m_Settings.angle);
        glm::vec3 sunDir = glm::vec3(0.0f, glm::cos(angle), -glm::sin(angle));
        if (m_Settings.kModel == kNishita)
        {
            float turbidity = glm::exp(m_Settings.sunTurbidityParams.value());
            glm::vec3 mie = ComputeCoefficientMie(lambda, K, turbidity);
            glm::vec3 rayleigh = ComputeCoefficientRayleigh(lambda);

            m_NishitaSkyShader.bind();
            m_NishitaSkyShader.setUniform("uModelToProj", m_Camera.getViewProjMatrix());
            m_NishitaSkyShader.setUniform("uChapman", m_Settings.bChapman);
            m_NishitaSkyShader.setUniform("uEarthRadius", 6360e3f);
            m_NishitaSkyShader.setUniform("uAtmosphereRadius", 6420e3f);
            m_NishitaSkyShader.setUniform("uEarthCenter", glm::vec3(0.f));
            m_NishitaSkyShader.setUniform("uSunDir", glm::normalize(sunDir));
            m_NishitaSkyShader.setUniform("uAltitude", m_Settings.altitude*1e3f);
            m_NishitaSkyShader.setUniform("uSunRadius", m_Settings.sunRaidusParams.value());
            m_NishitaSkyShader.setUniform("uSunRadiance", m_Settings.sunRadianceParams.value());
            m_NishitaSkyShader.setUniform("betaR0", rayleigh);
            m_NishitaSkyShader.setUniform("betaM0", mie);
            m_Sphere.draw();
        }
        if (m_Settings.kModel == kTimeOfDay)
        {
            m_TimeOfDayShader.bind();
            m_TimeOfDayShader.setUniform("uCameraPosition", m_Camera.getPosition());
            m_TimeOfDayShader.setUniform("uModelToProj", m_Camera.getViewProjMatrix());
            m_TimeOfDayShader.setUniform("uSunDir", glm::normalize(sunDir));
            m_TimeOfDayShader.setUniform("uAltitude", m_Settings.altitude*1e3f);
            m_TimeOfDayShader.setUniform("uCloudSpeed", m_Settings.cloudSpeedParams.value() * time);
            m_TimeOfDayShader.setUniform("uCloudDensity", m_Settings.cloudDensityParams.value());
            m_TimeOfDayShader.setUniform("uSunRadius", m_Settings.sunRaidusParams.value());
			m_TimeOfDayShader.setUniform("uSunRadiance", m_Settings.sunRadianceParams.value());
            m_TimeOfDayShader.setUniform("uTurbidity", m_Settings.sunTurbidity2Params.value());
            m_TimeOfDayShader.bindTexture("uNoiseMapSamp", m_NoiseMapSamp, 0);
            m_Sphere.draw();
        }
        if (m_Settings.kModel == kTimeOfNight)
        {
            m_StarShader.bind();
            m_StarShader.setUniform("uTime", time);
            m_StarShader.setUniform("uCameraPosition", m_Camera.getPosition());
            m_StarShader.setUniform("uModelToProj", m_Camera.getViewProjMatrix());
            m_StarShader.setUniform("uSunDir", glm::normalize(sunDir));
            m_StarShader.bindTexture("uMilkyWayMapSamp", m_MilkywaySamp, 0);
            m_Sphere.draw();

            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

            m_MoonShader.bind();
            m_MoonShader.setUniform("uTime", time);
            m_MoonShader.setUniform("uCameraPosition", m_Camera.getPosition());
            m_MoonShader.setUniform("uModelToProj", m_Camera.getViewProjMatrix());
            m_MoonShader.setUniform("uSunDirection", -glm::normalize(sunDir));
            m_MoonShader.setUniform("uMoonBrightness", m_Settings.moonRadianceParams.ratio());
            m_MoonShader.bindTexture("uMoonMapSamp", m_MoonMapSamp, 0);
            m_Sphere.draw();

            glBlendFunc(GL_ONE, GL_SRC_ALPHA);
            m_TimeOfNightShader.bind();
            m_TimeOfNightShader.setUniform("uCameraPosition", m_Camera.getPosition());
            m_TimeOfNightShader.setUniform("uModelToProj", m_Camera.getViewProjMatrix());
            m_TimeOfNightShader.setUniform("uSunDir", glm::normalize(sunDir));
            m_TimeOfNightShader.setUniform("uTurbidity", m_Settings.moonTurbidityParams.value());
            m_Sphere.draw();

            glDisable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
        }
		glEnable(GL_CULL_FACE);
    }
}

void LightScattering::renderSkybox() noexcept
{
    glFrontFace(GL_CW);
    m_SkyboxShader.bind();
    m_SkyboxShader.setUniform("uModelToProj", m_Camera.getViewProjMatrix());
    m_SkyboxShader.bindTexture("uSkyboxMapSamp", m_SkyboxTex, 0);
    m_Cube.draw();
    glFrontFace(GL_CCW);
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
	m_Camera.setProjectionParams(45.0f, aspectRatio, 0.1f, 10000.f);

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
