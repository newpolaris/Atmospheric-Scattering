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
#include <ModelAssImp.h>
#include <LightingTechnique.h>

#include <fstream>
#include <memory>
#include <vector>
#include <algorithm>
#include <GameCore.h>
#include <tools/string.h>
#include <GraphicsContext.h>
#include <GLType/GraphicsTexture.h>

enum ProfilerType { kProfilerTypeRender = 0, kProfilerTypeUpdate };

struct OrthographicProjection
{
    float l, r, b, t, n, f;
};

namespace 
{
    float s_CpuTick = 0.f;
    float s_GpuTick = 0.f;
    const int s_NumMeshes = 5;
    float near_plane = 1.0f, far_plane = 200.0f;
    glm::mat4 m_MatMeshModel[s_NumMeshes];
    glm::vec3 lightPosition = glm::vec3(0.0f, 0.0f, 0.0f);
}

struct SceneSettings
{
    float debugType = 0.f;
    bool bDebugDepth = false;
    bool bUiChanged = false;
    bool bResized = false;
    bool bUpdated = true;
    float depthIndex = 0.f;
	float m_exposure = 0.f;
    float angle = 76.f;
    float fov = 45.f;
    float Slice1 = 25.f;
    float Slice2 = 90.f;
    glm::vec3 lightDirection = glm::vec3(1.f, -1.f, 0.f);
    glm::vec3 position = glm::vec3(0.7f, 9.5f, -1.0f);
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

    void CalcOrthoProjections();
    void ShadowMapPass(GraphicsContext& gfxContext);
    void RenderDebugDepth(GraphicsContext& gfxContext);
    void RenderPass(GraphicsContext& gfxContext);
    void TonemapPass(GraphicsContext& gfxContext);
    void RenderScene(const ProgramShader& shader);
    void RenderScene(LightingTechnique& technique);

    glm::vec3 GetSunDirection() const;
    glm::mat4 GetLightViewMatrix() const;
    glm::mat4 GetLightSpaceMatrix(uint32_t i) const;

    static const uint32_t m_NumCascades = 3;
    float m_CascadeEnd[m_NumCascades+1];
    OrthographicProjection m_ShadowOrthoProject[m_NumCascades]; 

    std::vector<glm::vec2> m_Samples;
    SceneSettings m_Settings;
	TCamera m_Camera;
    SimpleTimer m_Timer;
    ModelAssImp m_Dragon;
    DirectionalLight m_DirectionalLight;
    CubeMesh m_Cube;
    SphereMesh m_Sphere;
    SphereMesh m_SphereMini;
    PlaneMesh m_Ground;
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
	m_Camera.setMoveCoefficient(0.35f);
    m_Camera.setViewParams(glm::vec3(8.f, 21.f, -23.f), glm::vec3(8.f, 21.f, -23.f) + glm::vec3(-0.7f, -0.44f, 0.9f));

    glm::vec3 p = { -9.85913467, 25.9683094, -18.1401157 };
    glm::vec3 c = {-9.82689571, 25.1048946, -17.6366520 };
    glm::vec3 u = {0.0551749095, 0.504494607, 0.861650109};
    m_Camera.setViewParams(p, c);

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
    m_Dragon.create();
    m_Dragon.loadFromFile("resources/dragon.obj");

    for (int i = 0; i < s_NumMeshes; i++) {
        glm::mat4 model(1.f);
        m_MatMeshModel[i] = glm::translate(model, glm::vec3(0.0f, 0.0f, 3.f + i * 30.f));
    }
}

void LightScattering::closeup() noexcept
{
    m_Cube.destroy();
    m_Sphere.destroy();
    m_SphereMini.destroy();
    m_Ground.destroy();
    m_ScreenTraingle.destroy();
    m_Dragon.destroy();
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
    lightPosition = m_Settings.position;

    m_CascadeEnd[0] = -near_plane;
    m_CascadeEnd[1] = -m_Settings.Slice1;
    m_CascadeEnd[2] = -m_Settings.Slice2;
    m_CascadeEnd[3] = -far_plane;
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
            ImGui::Separator();
            bUpdated |= ImGui::SliderFloat("Debug Light type", &m_Settings.debugType, 0.f, 3.f);
            ImGui::Separator();
            bUpdated |= ImGui::Checkbox("Debug depth", &m_Settings.bDebugDepth);
            bUpdated |= ImGui::SliderFloat("Debug depth index", &m_Settings.depthIndex, 0.f, 2.f);
            ImGui::Separator();
            bUpdated |= ImGui::SliderFloat("Near", &near_plane, -10.f, m_Settings.Slice1);
            bUpdated |= ImGui::SliderFloat("Slice1", &m_Settings.Slice1, near_plane, m_Settings.Slice2);
            bUpdated |= ImGui::SliderFloat("Slice2", &m_Settings.Slice2, m_Settings.Slice1, far_plane);
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

    CalcOrthoProjections();
    ShadowMapPass(context);
    if (m_Settings.bDebugDepth) RenderDebugDepth(context);
    else RenderPass(context);
    TonemapPass(context);

    profiler::stop(kProfilerTypeRender);
    profiler::tick(kProfilerTypeRender, s_CpuTick, s_GpuTick);
}

void LightScattering::CalcOrthoProjections()
{
    glm::mat4 viewInv = glm::inverse(m_Camera.getViewMatrix());
    glm::mat4 lightView = GetLightViewMatrix();

    float aspect = (float)Graphics::g_NativeWidth/Graphics::g_NativeHeight;
    float halfFovY = glm::radians(m_Settings.fov / 2.f);
    float tanHalfVerticalFOV = glm::tan(halfFovY);
    float tanHalfHorizontalFOV = tanHalfVerticalFOV*aspect;

    for (uint32_t i = 0; i < m_NumCascades; i++)
    {
        float xn = m_CascadeEnd[i] * tanHalfHorizontalFOV;
        float yn = m_CascadeEnd[i] * tanHalfVerticalFOV;
        float xf = m_CascadeEnd[i+1] * tanHalfHorizontalFOV;
        float yf = m_CascadeEnd[i+1] * tanHalfVerticalFOV;

        auto frustumCorners = {
            // near face
            glm::vec4( xn,  yn, m_CascadeEnd[i], 1.f),
            glm::vec4(-xn,  yn, m_CascadeEnd[i], 1.f),
            glm::vec4( xn, -yn, m_CascadeEnd[i], 1.f),
            glm::vec4(-xn, -yn, m_CascadeEnd[i], 1.f),

            // far face
            glm::vec4( xf,  yf, m_CascadeEnd[i+1], 1.f),
            glm::vec4(-xf,  yf, m_CascadeEnd[i+1], 1.f),
            glm::vec4( xf, -yf, m_CascadeEnd[i+1], 1.f),
            glm::vec4(-xf, -yf, m_CascadeEnd[i+1], 1.f),
        };

        glm::vec3 minPoint = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 maxPoint = glm::vec3(-std::numeric_limits<float>::max());

        for (auto it : frustumCorners)
        {
            // Transform the frustum coordinate from view to world space
            glm::vec4 positionWS = viewInv * it;
            // Transform the frustum coordinate from world to light space
            glm::vec3 positionLS = glm::vec3(lightView * positionWS);

            minPoint = glm::min(minPoint, positionLS);
            maxPoint = glm::max(maxPoint, positionLS);
        }
        // glm::orth espect camera n, f which is > 0, so revert it
        m_ShadowOrthoProject[i] = { minPoint.x, maxPoint.x, minPoint.y, maxPoint.y, -maxPoint.z, -minPoint.z };
    }
}

void LightScattering::ShadowMapPass(GraphicsContext& gfxContext)
{
    m_ShadowMapShader.bind();
    // depth clamping so that the shadow maps keep from moving 
    // through objects which causes shadows to disappear.
    gfxContext.SetDepthClamp(true);
    gfxContext.SetCullFace(kCullFront);
    for (int i = 0; i < m_NumCascades; i++)
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
    for (uint32_t i = 0; i < m_NumCascades; i++)
    {
        glm::vec4 vView(0.f, 0.f, m_CascadeEnd[i + 1], 1.0f);
        glm::vec4 vClip = project * vView;
        m_LightShader.setUniform(util::format("uCascadeEndClipSpace[{0}]", i), vClip.z);
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
    shader.setUniform("uMatModel", glm::mat4(1.f));
    m_Ground.draw();

    for (int i = 0; i < s_NumMeshes; i++)
    {
        shader.setUniform("uMatModel", m_MatMeshModel[i]);
        m_Dragon.render();
    }
}

void LightScattering::RenderScene(LightingTechnique& technique)
{
    technique.setMatModel(glm::mat4(1.f));
    m_Ground.draw();
    for (int i = 0; i < s_NumMeshes; i++)
    {
        technique.setMatModel(m_MatMeshModel[i]);
        m_Dragon.render();
    }
}

glm::vec3 LightScattering::GetSunDirection() const
{
    const float angle = glm::radians(m_Settings.angle);
    glm::vec3 sunDir = glm::vec3(0.0f, glm::cos(angle), -glm::sin(angle));
    return glm::normalize(sunDir);
}

glm::mat4 LightScattering::GetLightViewMatrix() const
{
    // poisition zero and direction make similar result;
    return glm::lookAt(-5.f*m_DirectionalLight.Direction, glm::vec3(0.f), glm::vec3(0.0, 1.0, 0.0));
    // From ogldev tutorial49
    // "Since we are dealing with a directional light that has no origin 
    //  we just need to rotate the world so that the light direction becomes 
    //  aligned with the positive Z axis. The origin of light can simply be
    //  the origin of the light space coordinate system (which means we don't 
    //  need any translation)"
    return glm::lookAt(glm::vec3(0.f), m_DirectionalLight.Direction, glm::vec3(0.0, 1.0, 0.0));
}

glm::mat4 LightScattering::GetLightSpaceMatrix(uint32_t i) const
{
    const auto& info = m_ShadowOrthoProject[i];
    glm::mat4 lightProjection = glm::ortho(info.l, info.r, info.b, info.t, info.n, info.f);
    return lightProjection * GetLightViewMatrix();
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
	m_Camera.setProjectionParams(m_Settings.fov, aspectRatio, 1.f, 1000.f);

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
