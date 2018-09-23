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
#include <tools/Timer.hpp>

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
#include <Mesh.h>

#include <fstream>
#include <memory>
#include <vector>
#include <algorithm>
#include <GameCore.h>
#include "Atmosphere.h"

enum ProfilerType { ProfilerTypeRender = 0 };

namespace 
{
    float s_CpuTick = 0.f;
    float s_GpuTick = 0.f;
}

struct FloatSetting
{
    const char* name;

    bool updateGUI()
    {
        return ImGui::SliderFloat(name, &_value.r, _value.g, _value.b);
    }

    float value()
    {
        return _value.x;
    }

    glm::vec3 _value;
};

struct Duration
{
    void initialize()
    {
        _last = _first = std::chrono::steady_clock::now();
    }

    void update()
    {
        _last = std::chrono::steady_clock::now();
    }

    float duration()
    {
        return float(std::chrono::duration_cast<std::chrono::milliseconds>(_last - _first).count()) / 1000;
    }

    std::chrono::time_point<std::chrono::steady_clock> _first, _last;
};

struct SceneSettings
{
    bool bCPU = false;
    bool bProfile = true;
    bool bUiChanged = false;
    bool bResized = false;
    bool bUpdated = true;
	bool bChapman = true;
    float angle = 76.f;
    float intensity = 20.f;
    float altitude = 1.f;
    float turbidity = 10.f;
    float fov = 45.f;
    FloatSetting cloudSpeedParams = {"Cloud Speed", glm::vec3(0.05, 0.0, 1.0)};
    FloatSetting cloudDensityParams = {"Cloud Density", glm::vec3(400, 0.0, 1600.0)};
};

glm::vec3 ComputeCoefficientRayleigh(const glm::vec3& lambda)
{
    const float n = 1.0003f; // refractive index
    const float N = 2.545e25f; // molecules per unit
    const float p = 0.035f; // depolarization factor for standard air
    const float pi = glm::pi<float>();

    const glm::vec3 l4 = lambda*lambda*lambda*lambda;
    return 8*pi*pi*pi*glm::pow(n*n - 1, 2) / (3*N*l4) * ((6 + 3*p)/(6 - 7*p));
}

glm::vec3 ComputeCoefficientMie(const glm::vec3& lambda, const glm::vec3& K, float turbidity)
{
    const int jungeexp = 4;
    const float pi = glm::pi<float>();
    const float c = glm::max(0.f, 0.6544f*turbidity - 0.6510f)*1e-16f; // concentration factor
    const float mie =  0.434f * c * pi * glm::pow(2*pi, jungeexp - 2);
    return mie * K / lambda;
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

    std::vector<glm::vec2> m_Samples;
    SphereMesh m_Sphere;
    SceneSettings m_Settings;
	TCamera m_Camera;
    Duration m_Duration;
    FullscreenTriangleMesh m_ScreenTraingle;
    ProgramShader m_FlatShader;
    ProgramShader m_SkyShader;
    ProgramShader m_BlitShader;
    GraphicsTexturePtr m_SkyColorTex;
    GraphicsTexturePtr m_ScreenColorTex;
	GraphicsTexturePtr m_NoiseMapSamp;
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
    m_Duration.initialize();
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

	m_SkyShader.setDevice(m_Device);
	m_SkyShader.initialize();
	m_SkyShader.addShader(GL_VERTEX_SHADER, "Time of day/Time of day.Vertex");
	m_SkyShader.addShader(GL_FRAGMENT_SHADER, "Time of day/Time of day.Fragment");
	m_SkyShader.link();

	m_BlitShader.setDevice(m_Device);
	m_BlitShader.initialize();
	m_BlitShader.addShader(GL_VERTEX_SHADER, "BlitTexture.Vertex");
	m_BlitShader.addShader(GL_FRAGMENT_SHADER, "BlitTexture.Fragment");
	m_BlitShader.link();

    m_ScreenTraingle.create();
    m_Sphere.create();

    GraphicsTextureDesc noise;
    noise.setWrapS(GL_REPEAT);
    noise.setWrapT(GL_REPEAT);
    noise.setMinFilter(GL_LINEAR);
    noise.setMagFilter(GL_LINEAR);
    noise.setFilename("resources/Skybox/cloud.tga");
    m_NoiseMapSamp = m_Device->createTexture(noise);
}

void LightScattering::closeup() noexcept
{
    m_Sphere.destroy();
    m_ScreenTraingle.destroy();
	profiler::shutdown();
}

void LightScattering::update() noexcept
{
    m_Duration.update();
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
    bUpdated |= ImGui::Checkbox("Mode CPU", &m_Settings.bCPU);
    bUpdated |= ImGui::Checkbox("Always redraw", &m_Settings.bProfile);
	bUpdated |= ImGui::Checkbox("Use chapman approximation", &m_Settings.bChapman);
    bUpdated |= ImGui::SliderFloat("Sun Angle", &m_Settings.angle, 0.f, 120.f);
    bUpdated |= ImGui::SliderFloat("Sun Intensity", &m_Settings.intensity, 10.f, 50.f);
    bUpdated |= ImGui::SliderFloat("Altitude (km)", &m_Settings.altitude, 0.f, 100.f);
    bUpdated |= ImGui::SliderFloat("Turbidity", &m_Settings.turbidity, 1e-5f, 10000.f);
    bUpdated |= ImGui::SliderFloat("Fov", &m_Settings.fov, 15.f, 120.f);
    bUpdated |= m_Settings.cloudSpeedParams.updateGUI();
    bUpdated |= m_Settings.cloudDensityParams.updateGUI();
    ImGui::Text("CPU %s: %10.5f ms\n", "main", s_CpuTick);
    ImGui::Text("GPU %s: %10.5f ms\n", "main", s_GpuTick);
    ImGui::PushItemWidth(180.0f);
    ImGui::Indent();
    ImGui::Unindent();
    ImGui::End();

    m_Settings.bUiChanged = bUpdated;
}

void LightScattering::render() noexcept
{
    bool bUpdate = m_Settings.bProfile || m_Settings.bUpdated;

    profiler::start(ProfilerTypeRender);
    if (!m_Settings.bCPU && bUpdate)
    {
        // [Preetham99]
        const glm::vec3 K = glm::vec3(0.686282f, 0.677739f, 0.663365f); // spectrum
        const glm::vec3 lambda = glm::vec3(680e-9f, 550e-9f, 440e-9f);

        glm::vec3 mie = ComputeCoefficientMie(lambda, K, m_Settings.turbidity);
        glm::vec3 rayleigh = ComputeCoefficientRayleigh(lambda);

        auto& desc = m_ScreenColorTex->getGraphicsTextureDesc();
        m_Device->setFramebuffer(m_ColorRenderTarget);
        GLenum clearFlag = GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT;
        glViewport(0, 0, desc.getWidth(), desc.getHeight());
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClearDepthf(1.0f);
        glClear(clearFlag);

        // sky box
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);

        const float time = m_Duration.duration();
        const float angle = glm::radians(m_Settings.angle);
		glm::vec2 resolution(desc.getWidth(), desc.getHeight());
        glm::vec3 sunDir = glm::vec3(0.0f, glm::cos(angle), -glm::sin(angle));
        m_SkyShader.bind();
        m_SkyShader.setUniform("uCameraPosition", m_Camera.getPosition());
        m_SkyShader.setUniform("uModelToProj", m_Camera.getViewProjMatrix());
        m_SkyShader.setUniform("uChapman", m_Settings.bChapman);
        m_SkyShader.setUniform("uEarthRadius", 6360e3f);
        m_SkyShader.setUniform("uAtmosphereRadius", 6420e3f);
        m_SkyShader.setUniform("uEarthCenter", glm::vec3(0.f));
        m_SkyShader.setUniform("uSunDir", glm::normalize(sunDir));
        m_SkyShader.setUniform("uSunIntensity", glm::vec3(m_Settings.intensity));
        m_SkyShader.setUniform("uAltitude", m_Settings.altitude*1e3f);
        m_SkyShader.setUniform("uTurbidity", m_Settings.turbidity);
        m_SkyShader.setUniform("uCloudSpeed", m_Settings.cloudSpeedParams.value() * time);
        m_SkyShader.setUniform("uCloudDensity", m_Settings.cloudDensityParams.value());
        m_SkyShader.setUniform("uTime", time);
        m_SkyShader.bindTexture("uNoiseMapSamp", m_NoiseMapSamp, 0);
        m_Sphere.draw();
		glEnable(GL_CULL_FACE);
        glDepthMask(GL_TRUE);
    }
    // Tone mapping
    {
        GraphicsTexturePtr target = m_ScreenColorTex;
        if (m_Settings.bCPU && m_SkyColorTex) 
            target = m_SkyColorTex;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, getFrameWidth(), getFrameHeight());

        glDisable(GL_DEPTH_TEST);
        m_BlitShader.bind();
        m_BlitShader.bindTexture("uTexSource", target, 0);
        m_ScreenTraingle.draw();
        glEnable(GL_DEPTH_TEST);
    }
    profiler::stop(ProfilerTypeRender);
    profiler::tick(ProfilerTypeRender, s_CpuTick, s_GpuTick);

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
