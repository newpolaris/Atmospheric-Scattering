#include <LightingTechnique.h>
#include <tools/string.h>
#include <tools/misc.hpp>

LightingTechnique::LightingTechnique()
{
}

void LightingTechnique::initialize()
{
    m_shader.setDevice(m_Device.lock());
    m_shader.initialize();
    m_shader.addShader(GL_VERTEX_SHADER, "LightingVS.glsl");
    m_shader.addShader(GL_FRAGMENT_SHADER, "LightingPS.glsl");
    m_shader.link();
    m_shader.bind();

    auto shaderid = m_shader.getShaderID();
    auto GetUniformLocation = [shaderid](const std::string& uniform) {
        return glGetUniformLocation(shaderid, uniform.c_str());
    };
    auto GetUniformLocationL = [shaderid](const std::string& uniform, uint32_t index) {
        return glGetUniformLocation(shaderid, util::format(uniform, index).c_str());
    };

    #define INVALID_UNIFORM_LOCATION 0xffffffff

    for (uint32_t i = 0; i < numCascade; i++)
    {
        m_texShadowLoc[i] = GetUniformLocationL("uTexShadowmap[{0}]", i);
        m_matLightLoc[i] = GetUniformLocationL("uMatLight[{0}]", i);
        m_cascadeEndClipSpaceLoc[i] = GetUniformLocationL("uCascadeEndClipSpace[{0}]", i);
    }
    if (util::any_of(m_texShadowLoc, m_texShadowLoc + numCascade, INVALID_UNIFORM_LOCATION)
     || util::any_of(m_matLightLoc, m_matLightLoc + numCascade, INVALID_UNIFORM_LOCATION)
     || util::any_of(m_cascadeEndClipSpaceLoc, m_cascadeEndClipSpaceLoc + numCascade, INVALID_UNIFORM_LOCATION))
        assert(false);

    m_texWoodLoc = GetUniformLocation("uTexWood");
    m_matModelLoc = GetUniformLocation("uMatModel");
    m_matViewLoc = GetUniformLocation("uMatView");
    m_matProjectLoc = GetUniformLocation("uMatProject");
    m_eyePositionWSLoc = GetUniformLocation("uEyePositionWS");

    if (m_texWoodLoc == INVALID_UNIFORM_LOCATION)
        assert(false);

    m_numPointLightsLocation = GetUniformLocation("uNumPointLights");
    m_numSpotLightsLocation = GetUniformLocation("uNumSpotLights");

    #define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))

    m_dirLightLocation.Color = GetUniformLocation("uDirectionalLight.Base.Color");
    m_dirLightLocation.AmbientIntensity = GetUniformLocation("uDirectionalLight.Base.AmbientIntensity");
    m_dirLightLocation.Direction = GetUniformLocation("uDirectionalLight.Direction");
    m_dirLightLocation.DiffuseIntensity = GetUniformLocation("uDirectionalLight.Base.DiffuseIntensity");

    for (uint32_t i = 0 ; i < ARRAY_SIZE_IN_ELEMENTS(m_pointLightsLocation); i++) {
        m_pointLightsLocation[i].Color = GetUniformLocationL("uPointLights[{0}].Base.Color", i);
        m_pointLightsLocation[i].AmbientIntensity = GetUniformLocationL("uPointLights[{0}].Base.AmbientIntensity", i);
        m_pointLightsLocation[i].Position = GetUniformLocationL("uPointLights[{0}].Position", i);
        m_pointLightsLocation[i].DiffuseIntensity = GetUniformLocationL("uPointLights[{0}].Base.DiffuseIntensity", i);
        m_pointLightsLocation[i].Attenuation = GetUniformLocationL("uPointLights[{0}].Attenuation", i);

        if (m_pointLightsLocation[i].Color == INVALID_UNIFORM_LOCATION ||
            m_pointLightsLocation[i].AmbientIntensity == INVALID_UNIFORM_LOCATION ||
            m_pointLightsLocation[i].Position == INVALID_UNIFORM_LOCATION ||
            m_pointLightsLocation[i].DiffuseIntensity == INVALID_UNIFORM_LOCATION ||
            m_pointLightsLocation[i].Attenuation == INVALID_UNIFORM_LOCATION)
            assert(false);
    }

    for (uint32_t i = 0 ; i < ARRAY_SIZE_IN_ELEMENTS(m_spotLightsLocation) ; i++) {
        m_spotLightsLocation[i].Color = GetUniformLocationL("uSpotLights[{0}].Base.Base.Color", i);
        m_spotLightsLocation[i].AmbientIntensity = GetUniformLocationL("uSpotLights[{0}].Base.Base.AmbientIntensity", i);
        m_spotLightsLocation[i].Position = GetUniformLocationL("uSpotLights[{0}].Base.Position", i);
        m_spotLightsLocation[i].Direction = GetUniformLocationL("uSpotLights[{0}].Direction", i);
        m_spotLightsLocation[i].Cutoff = GetUniformLocationL("uSpotLights[{0}].Cutoff", i);
        m_spotLightsLocation[i].DiffuseIntensity = GetUniformLocationL("uSpotLights[{0}].Base.Base.DiffuseIntensity", i);
        m_spotLightsLocation[i].Attenuation = GetUniformLocationL("uSpotLights[{0}].Base.Attenuation", i);

        if (m_spotLightsLocation[i].Color == INVALID_UNIFORM_LOCATION ||
            m_spotLightsLocation[i].AmbientIntensity == INVALID_UNIFORM_LOCATION ||
            m_spotLightsLocation[i].Position == INVALID_UNIFORM_LOCATION ||
            m_spotLightsLocation[i].Direction == INVALID_UNIFORM_LOCATION ||
            m_spotLightsLocation[i].Cutoff == INVALID_UNIFORM_LOCATION ||
            m_spotLightsLocation[i].DiffuseIntensity == INVALID_UNIFORM_LOCATION ||
            m_spotLightsLocation[i].Attenuation == INVALID_UNIFORM_LOCATION)
            assert(false);
    }
}

void LightingTechnique::bind()
{
    m_shader.bind();
}

void LightingTechnique::setDevice(const GraphicsDevicePtr& device)
{
    m_Device = device;
}

void LightingTechnique::setMatLightSpace(uint32_t i, const glm::mat4& mat)
{
    m_shader.setUniform(m_matLightLoc[i], mat);
}

void LightingTechnique::setMatModel(const glm::mat4& mat)
{
    m_shader.setUniform(m_matModelLoc, mat);
}

void LightingTechnique::setMatView(const glm::mat4 & mat)
{
    m_shader.setUniform(m_matViewLoc, mat);
}

void LightingTechnique::setMatProject(const glm::mat4 & mat)
{
    m_shader.setUniform(m_matProjectLoc, mat);
}

void LightingTechnique::setEyePositionWS(const glm::vec3& position)
{
    m_shader.setUniform(m_eyePositionWSLoc, position);
}

void LightingTechnique::setCascadeEndClipSpace(uint32_t i, float vClipZ)
{
    m_shader.setUniform(m_cascadeEndClipSpaceLoc[i], vClipZ);
}

void LightingTechnique::setShadowMap(uint32_t numCascade, const GraphicsTexturePtr* texture)
{
    m_shader.bindTexture(m_texShadowLoc[0], texture[0], 1);
    m_shader.bindTexture(m_texShadowLoc[1], texture[1], 2);
    m_shader.bindTexture(m_texShadowLoc[2], texture[2], 3);
}

void LightingTechnique::setTexWood(const GraphicsTexturePtr& texture)
{
    m_shader.bindTexture(m_texWoodLoc, texture, 0);
}

void LightingTechnique::setDebugType(int32_t type)
{
    m_shader.setUniform("uDebugType", type);
}

void LightingTechnique::setDirectionalLight(const DirectionalLight& Light)
{
    m_shader.setUniform(m_dirLightLocation.Color, Light.Color);
    m_shader.setUniform(m_dirLightLocation.AmbientIntensity, Light.AmbientIntensity);
    m_shader.setUniform(m_dirLightLocation.Direction, glm::normalize(Light.Direction));
    m_shader.setUniform(m_dirLightLocation.DiffuseIntensity, Light.DiffuseIntensity);
}

void LightingTechnique::setSpotLights(uint32_t NumLights, const SpotLight* pLights)
{
    m_shader.setUniform(m_numSpotLightsLocation, GLint(NumLights));
    for (uint32_t i = 0 ; i < NumLights; i++) {
        m_shader.setUniform(m_spotLightsLocation[i].Color, pLights[i].Color);
        m_shader.setUniform(m_spotLightsLocation[i].AmbientIntensity, pLights[i].AmbientIntensity);
        m_shader.setUniform(m_spotLightsLocation[i].DiffuseIntensity, pLights[i].DiffuseIntensity);
        m_shader.setUniform(m_spotLightsLocation[i].Position, pLights[i].Position);
        m_shader.setUniform(m_spotLightsLocation[i].Attenuation, pLights[i].Attenuation); 
        m_shader.setUniform(m_spotLightsLocation[i].Cutoff, glm::cos(glm::radians(pLights[i].Cutoff))); 
        m_shader.setUniform(m_spotLightsLocation[i].Direction, glm::normalize(pLights[i].Direction)); 
    }
}

void LightingTechnique::setPointLights(uint32_t NumLights, const PointLight * pLights)
{
    m_shader.setUniform(m_numPointLightsLocation, GLint(NumLights));
    for (uint32_t i = 0 ; i < NumLights ; i++) {
        m_shader.setUniform(m_pointLightsLocation[i].Color, pLights[i].Color);
        m_shader.setUniform(m_pointLightsLocation[i].AmbientIntensity, pLights[i].AmbientIntensity);
        m_shader.setUniform(m_pointLightsLocation[i].DiffuseIntensity, pLights[i].DiffuseIntensity);
        m_shader.setUniform(m_pointLightsLocation[i].Position, pLights[i].Position);
        m_shader.setUniform(m_pointLightsLocation[i].Attenuation, pLights[i].Attenuation);
    }
}

