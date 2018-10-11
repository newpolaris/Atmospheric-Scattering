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

#include <BufferManager.h>
#include <Skybox/Skybox.h>
#include <Skybox/LightCube.h>
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
	bool m_doDiffuse = false;
	bool m_doSpecular = false;
	bool m_doDiffuseIbl = true;
	bool m_doSpecularIbl = true;
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

    void renderCloud() noexcept;
    void renderSkybox() noexcept;
    void renderSkycube() noexcept;
    void renderCubeSample() noexcept;

    std::vector<glm::vec2> m_Samples;
    SceneSettings m_Settings;
	TCamera m_Camera;
    SimpleTimer m_Timer;
    CubeMesh m_Cube;
    SphereMesh m_Sphere;
    SphereMesh m_SphereMini;
    FullscreenTriangleMesh m_ScreenTraingle;
    LightCube m_LightCube;
    ProgramShader m_FlatShader;
    ProgramShader m_NishitaSkyShader;
    ProgramShader m_TimeOfDayShader;
    ProgramShader m_TimeOfNightShader;
    ProgramShader m_StarShader;
    ProgramShader m_MoonShader;
    ProgramShader m_BlitShader;
    ProgramShader m_PostProcessHDRShader;
    ProgramShader m_SkyboxShader;
    ProgramShader m_SkycubeShader;
    ProgramShader m_programMesh;
    GraphicsTexturePtr m_SkyboxTex;
    GraphicsTexturePtr m_ScreenColorTex;
	GraphicsTexturePtr m_NoiseMapSamp;
	GraphicsTexturePtr m_MilkywaySamp;
	GraphicsTexturePtr m_MoonMapSamp;
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
	m_Camera.setViewParams(glm::vec3(2.0f, 5.0f, 15.0f), glm::vec3(2.0f, 0.0f, 0.0f));

	GraphicsDeviceDesc deviceDesc;
#if __APPLE__
	deviceDesc.setDeviceType(GraphicsDeviceType::GraphicsDeviceTypeOpenGL);
#else
	deviceDesc.setDeviceType(GraphicsDeviceType::GraphicsDeviceTypeOpenGLCore);
#endif
	m_Device = createDevice(deviceDesc);

    light_cube::initialize(m_Device);
    skybox::initialize(m_Device);

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

	m_SkycubeShader.setDevice(m_Device);
	m_SkycubeShader.initialize();
	m_SkycubeShader.addShader(GL_VERTEX_SHADER, "Skycube.Vertex");
	m_SkycubeShader.addShader(GL_FRAGMENT_SHADER, "Skycube.Fragment");
	m_SkycubeShader.link();

	m_programMesh.setDevice(m_Device);
    m_programMesh.initialize();
    m_programMesh.addShader(GL_VERTEX_SHADER, "IBL/IblMesh.Vertex");
    m_programMesh.addShader(GL_FRAGMENT_SHADER, "IBL/IblMesh.Fragment");
    m_programMesh.link();

    m_ScreenTraingle.create();
    m_Cube.create();
    m_Sphere.create();
    m_SphereMini.create();

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

    // load the HDR environment map
    GraphicsTextureDesc skyDesc;
    skyDesc.setWrapS(GL_REPEAT);
    skyDesc.setWrapT(GL_REPEAT);
    skyDesc.setMinFilter(GL_NEAREST_MIPMAP_LINEAR);
    skyDesc.setMagFilter(GL_LINEAR);
    skyDesc.setFilename("resources/skybox/helipad.dds");
    // skyDesc.setFilename("resources/skybox/newport_loft.hdr");
    m_SkyboxTex = m_Device->createTexture(skyDesc);
    assert(m_SkyboxTex);
    m_LightCube.initialize(m_Device, m_SkyboxTex);
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

    profiler::start(kProfilerTypeUpdate);
    if (m_Settings.bUpdateLight)
        m_LightCube.update(m_Device);
    profiler::stop(kProfilerTypeUpdate);
    profiler::tick(kProfilerTypeUpdate, s_CpuTick, s_GpuTick);
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

void LightScattering::render() noexcept
{
    profiler::start(kProfilerTypeRender);

    skybox::light(m_Camera);

    auto& desc = m_ScreenColorTex->getGraphicsTextureDesc();
    m_Device->setFramebuffer(m_ColorRenderTarget);
    GLenum clearFlag = GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT;
    glViewport(0, 0, desc.getWidth(), desc.getHeight());
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepthf(1.0f);
    glClear(clearFlag);

    // renderCubeSample();
    // renderSkycube();
    // renderSkybox();
    // renderCloud();

    // Tone mapping
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, getFrameWidth(), getFrameHeight());

        glDisable(GL_DEPTH_TEST);
        m_PostProcessHDRShader.bind();
        m_PostProcessHDRShader.bindTexture("uTexSource", m_ScreenColorTex, 0);
        m_ScreenTraingle.draw();
        glEnable(GL_DEPTH_TEST);
    }
    profiler::stop(kProfilerTypeRender);
    // profiler::tick(kProfilerTypeRender, s_CpuTick, s_GpuTick);
}

void LightScattering::renderCloud() noexcept
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

void LightScattering::renderSkycube() noexcept
{
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glFrontFace(GL_CW);

    m_SkycubeShader.bind();
    m_SkycubeShader.setUniform("uViewMatrix", m_Camera.getViewMatrix());
    m_SkycubeShader.setUniform("uProjMatrix", m_Camera.getProjectionMatrix());
    m_SkycubeShader.setUniform("uBgType", m_Settings.m_bgType);
    m_SkycubeShader.setUniform("uExposure", m_Settings.m_exposure);

    // Texture binding
    m_SkycubeShader.bindTexture("uEnvmapSamp", m_LightCube.getEnvCube(), 0);
    m_SkycubeShader.bindTexture("uEnvmapIrrSamp", m_LightCube.getIrradiance(), 1);
    m_SkycubeShader.bindTexture("uEnvmapPrefilterSamp", m_LightCube.getPrefilter(), 2);

    m_Cube.draw();
    glFrontFace(GL_CCW);
    glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

void LightScattering::renderCubeSample() noexcept
{
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    m_programMesh.bind();

    // Uniform binding
    m_programMesh.setUniform("uModelViewProjMatrix", m_Camera.getViewProjMatrix());
    m_programMesh.setUniform("uEyePosWS", m_Camera.getPosition());
    m_programMesh.setUniform("uExposure", m_Settings.m_exposure);
    m_programMesh.setUniform("ubDiffuse", float(m_Settings.m_doDiffuse));
    m_programMesh.setUniform("ubSpecular", float(m_Settings.m_doSpecular));
    m_programMesh.setUniform("ubDiffuseIbl", float(m_Settings.m_doDiffuseIbl));
    m_programMesh.setUniform("ubSpecularIbl", float(m_Settings.m_doSpecularIbl));
    m_programMesh.setUniform("uRgbDiff", m_Settings.m_rgbDiff);
    m_programMesh.setUniform("uMtxSrt", glm::mat4(1));

    // Texture binding
    m_programMesh.bindTexture("uEnvmapIrr", m_LightCube.getIrradiance(), 4);
    m_programMesh.bindTexture("uEnvmapPrefilter", m_LightCube.getPrefilter(), 5);
    m_programMesh.bindTexture("uEnvmapBrdfLUT", m_LightCube.getBrdfLUT(), 6);

    // Submit orbs.
    for (float yy = 0, yend = 5.0f; yy < yend; yy += 1.0f)
    {
        for (float xx = 0, xend = 5.0f; xx < xend; xx += 1.0f)
        {
            const float scale = 1.2f;
            const float spacing = 2.2f * 30;
            const float yAdj = -0.8f;
            glm::vec3 translate(
                0.0f + (xx / xend)*spacing - (1.0f + (scale - 1.0f)*0.5f - 1.0f / xend),
                yAdj / yend + (yy / yend)*spacing - (1.0f + (scale - 1.0f)*0.5f - 1.0f / yend),
                0.0f);
            glm::mat4 mtxS = glm::scale(glm::mat4(1), glm::vec3(scale / xend));
            glm::mat4 mtxST = glm::translate(mtxS, translate);
            m_programMesh.setUniform("uGlossiness", xx*(1.0f / xend));
            m_programMesh.setUniform("uReflectivity", (yend - yy)*(1.0f / yend));
            m_programMesh.setUniform("uMtxSrt", mtxST);
            m_SphereMini.draw();
        }
    }
    m_programMesh.unbind();
    glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
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
