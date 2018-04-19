#include <GL/glew.h>
#include <glfw3.h>

// GLM for matrix transformation
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> 

#include <tools/gltools.hpp>
#include <tools/SimpleProfile.h>
#include <tools/imgui.h>
#include <tools/TCamera.h>

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

struct SceneSettings
{
    bool bUiChanged = false;
    bool bResized = false;
    bool bSampleReset = false;
};

static float Halton(int index, float base)
{
    float result = 0.0f;
    float f = 1.0f/base;
    float i = float(index);
    for (;;)
    {
        if (i <= 0.0f)
            break;

        result += f*fmodf(i, base);
        i = floorf(i/base);
        f = f/base;
    }
    return result;
}

static std::vector<glm::vec4> Halton4D(int size, int offset)
{
    std::vector<glm::vec4> s(size);
    for (int i = 0; i < size; i++)
    {
        s[i][0] = Halton(i + offset, 2.0f);
        s[i][1] = Halton(i + offset, 3.0f);
        s[i][2] = Halton(i + offset, 5.0f);
        s[i][3] = Halton(i + offset, 7.0f);
    }
    return s;
}

glm::mat4 jitterProjMatrix(const glm::mat4& proj, int sampleCount, float jitterAASigma, float width, float height)
{
    // Per-frame jitter to camera for AA
    const int frameNum = sampleCount + 1; // Add 1 since otherwise first sample is an outlier

    float u1 = Halton(frameNum, 2.0f);
    float u2 = Halton(frameNum, 3.0f);

    // Gaussian sample
    float phi = 2.0f*glm::pi<float>()*u2;
    float r = jitterAASigma*sqrtf(-2.0f*log(std::max(u1, 1e-7f)));
    float x = r*cos(phi);
    float y = r*sin(phi);

    glm::mat4 ret = proj;
    ret[0].w += x*2.0f/width;
    ret[1].w += y*2.0f/height;

    return ret;
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

    SceneSettings m_Settings;
	TCamera m_Camera;
    FullscreenTriangleMesh m_ScreenTraingle;
    ProgramShader m_SkyShader;
    ProgramShader m_BlitShader;
    GraphicsTexturePtr m_ScreenColorTex;
    GraphicsFramebufferPtr m_ColorRenderTarget;
    GraphicsDevicePtr m_Device;
};

CREATE_APPLICATION(LightScattering);

LightScattering::LightScattering() noexcept
{
}

LightScattering::~LightScattering() noexcept
{
}

void LightScattering::startup() noexcept
{
	m_Camera.setViewParams(glm::vec3(2.0f, 5.0f, 15.0f), glm::vec3(2.0f, 0.0f, 0.0f));
	m_Camera.setMoveCoefficient(0.35f);

	GraphicsDeviceDesc deviceDesc;
#if __APPLE__
	deviceDesc.setDeviceType(GraphicsDeviceType::GraphicsDeviceTypeOpenGL);
#else
	deviceDesc.setDeviceType(GraphicsDeviceType::GraphicsDeviceTypeOpenGLCore);
#endif
	m_Device = createDevice(deviceDesc);
	assert(m_Device);

	m_SkyShader.setDevice(m_Device);
	m_SkyShader.initialize();
	m_SkyShader.addShader(GL_VERTEX_SHADER, "Scattering.Vertex");
	m_SkyShader.addShader(GL_FRAGMENT_SHADER, "Scattering.Fragment");
	m_SkyShader.link();

	m_BlitShader.setDevice(m_Device);
	m_BlitShader.initialize();
	m_BlitShader.addShader(GL_VERTEX_SHADER, "BlitTexture.Vertex");
	m_BlitShader.addShader(GL_FRAGMENT_SHADER, "BlitTexture.Fragment");
	m_BlitShader.link();

    m_ScreenTraingle.create();
}

void LightScattering::closeup() noexcept
{
    m_ScreenTraingle.destroy();
}

void LightScattering::update() noexcept
{
    bool bCameraUpdated = m_Camera.update();

    static float preWidth = 0.f;
    static float preHeight = 0.f;

    float width = (float)getFrameWidth();
    float height = (float)getFrameHeight();
    bool bResized = false;
    if (preWidth != width || preHeight != height)
    {
        preWidth = width, preHeight = height;
        bResized = true;
    }
    m_Settings.bSampleReset = (m_Settings.bUiChanged || bCameraUpdated || bResized);
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
    ImGui::Unindent();
    ImGui::End();

    m_Settings.bUiChanged = bUpdated;
}

void LightScattering::render() noexcept
{
	auto& desc = m_ScreenColorTex->getGraphicsTextureDesc();
    m_Device->setFramebuffer(m_ColorRenderTarget);
    GLenum clearFlag = GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT;
	glViewport(0, 0, desc.getWidth(), desc.getHeight());
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepthf(1.0f);
	// glClear(clearFlag);

    // color pass
    {
		glm::vec2 resolution(desc.getWidth(), desc.getHeight());
        m_SkyShader.bind();
		m_SkyShader.setUniform("uInvResolution", 1.f/resolution);
        // m_ScreenTraingle.draw();
    }
    // Tone mapping
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, getFrameWidth(), getFrameHeight());

        glDisable(GL_DEPTH_TEST);
        m_BlitShader.bind();
        m_BlitShader.bindTexture("uTexSource", m_ScreenColorTex, 0);
        m_ScreenTraingle.draw();
        glEnable(GL_DEPTH_TEST);
    }
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

struct Atmosphere
{
public:
	Atmosphere(glm::vec3 sunDir);
	glm::vec4 computeIncidentLight(const glm::vec3& orig, const glm::vec3& dir, float tmin, float tmax) const; 
	void renderSkyDome(std::vector<glm::vec4>& image, int width, int height) const;

	float m_Hr = 7994;
	float m_Ar = 6420e3; // atmosphere radius
	float m_Er = 6360e3; // earth radius

	glm::vec3 m_SunDir;
	glm::vec3 m_Ec = glm::vec3(0.f); // earth center
	glm::vec3 m_BetaR0 = glm::vec3(3.8e-6f, 13.5e-6f, 33.1e-6f); 
};

Atmosphere::Atmosphere(glm::vec3 sunDir) : 
	m_SunDir(sunDir)
{
}

glm::vec2 RaySphereIntersect(glm::vec3 pos, glm::vec3 dir, glm::vec3 c, float r)
{
	assert(r > 0.f);

	glm::vec3 tc = c - pos;

	float l = glm::dot(tc, dir);
	float d = l*l - glm::dot(tc, tc) + r*r;
	if (d < 0) return glm::vec2(-1.0);
	float sl = glm::sqrt(d);

	return glm::vec2(l - sl, l + sl);
}

glm::vec4 Atmosphere::computeIncidentLight(const glm::vec3& pos, const glm::vec3& dir, float tmin, float tmax) const
{
	const glm::vec3 SunIntensity = glm::vec3(20.f);
	const int numSamples = 16;
	const int numLightSamples = 8;

	auto t = RaySphereIntersect(pos, dir, m_Ec, m_Ar);
	tmin = std::min(t.x, 0.f);
	tmax = t.y;
	if (tmax < 0) return glm::vec4(0.f);
	auto tc = pos;
	auto pa = tc + tmax*dir, pb = tc + tmin*dir;

	float opticalDepthR = 0.f, opticalDepthM = 0.f;
	float ds = (tmax - tmin) / numSamples; // delta segment
	glm::vec3 sumR(0.f);
	for (int s = 0; s < numSamples; s++)
	{
		glm::vec3 x = tb + ds*(0.5 + s)*dir;
		float h = glm::lenth(x) - m_Er;
		float betaR = exp(-h/m_Hr)*ds;
		opticalDepthR += betaR;
		auto tl = RaySphereIntersect(x, m_SunDir, m_Ec, m_Ar);
		float lmax = tl.y, lmin = 0.f;
		float dsl = (lmax - lmin)/numLightSamples;
		int l = 0;
		float opticalDepthLightR = 0.f;
		for (; l < numLightSamples; l++)
		{
			glm::vec3 xl = x + dsl*(0.5 + l)*m_SunDir;
			float hl = glm::length(xl) - m_Er;
			if (hl < 0) break;
			opticalDepthLightR += exp(-hl/m_Hr)*dsl;
		}
		if (l < numLightSamples) continue;
		glm::vec3 tau = m_BetaR0 * (opticalDepthR + opticalDepthLightR);
		glm::vec3 attenuation = glm::exp(-tau);
		sumR += attenuation * betaR;
	}

	float mu = glm::dot(m_SunDir, dir);
	float phaseR = 3.f / (16.f*glm::pi()) * (1.f + mu*mu);
	return SunIntensity * sumR * phaseR * m_BetaR0;
}

void Atmosphere::renderSkyDome(std::vector<glm::vec4>& image, int width, int height) const
{
	const float inf = 9e8;
	for (int y = 0; y < height; y++)
	for (int x = 0; x < height; x++)
	{
		float fy = 2.f * ((float)y + 0.5f)/(height-1) - 1.f;
		float fx = 2.f * ((float)x + 0.5f)/(height-1) - 1.f;
		float z2 = 1.f - (fy*fy+fx*fx);
		if (z2 < 0) continue;
		glm::vec3 dir = glm::normalize(glm::vec3(fx, glm::sqrt(z2), fy));
		image[y*width + x] = computeIncidentLight(glm::vec3(0.f), dir, 0.f, inf);
	}
}

void LightScattering::framesizeCallback(int32_t width, int32_t height) noexcept
{
	float aspectRatio = (float)width/height;
	m_Camera.setProjectionParams(45.0f, aspectRatio, 0.1f, 100.0f);

	glm::vec3 sunDir(0, 1, 0);
	std::vector<glm::vec4> image(width*height, glm::vec4(0.f));
	Atmosphere atmosphere(sunDir);
	atmosphere.renderSkyDome(image, width, height);

    GraphicsTextureDesc colorDesc;
    colorDesc.setWidth(width);
    colorDesc.setHeight(height);
    colorDesc.setFormat(gli::FORMAT_RGBA32_SFLOAT_PACK32);
	colorDesc.setStream((uint8_t*)image.data());
	colorDesc.setStreamSize(width*height*sizeof(glm::vec4));
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
