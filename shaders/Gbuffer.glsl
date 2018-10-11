--Vertex

// IN
layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexcoords;

// OUT
out vec3 vNormalWS;
out vec3 vViewDirWS;
out vec3 vWorldPosWS;
out vec2 vTexcoords;

// UNIFORM
uniform mat4 uMtxSrt;
uniform mat4 uModelViewProjMatrix;
uniform vec3 uEyePosWS;

void main()
{
    // Clip Space position
    gl_Position = uModelViewProjMatrix * uMtxSrt * inPosition;

    // World Space normal
    vec3 normal = mat3(uMtxSrt) * inNormal;
    vNormalWS = normalize(normal);

    vTexcoords = inTexcoords;

    // World Space view direction from world space position
    vec3 posWS = vec3((uMtxSrt * inPosition).xyz);
    vViewDirWS = normalize(uEyePosWS - posWS);
    vWorldPosWS = posWS;
}


--

//------------------------------------------------------------------------------


--Fragment

// IN
in vec3 vNormalWS;
in vec3 vViewDirWS;
in vec3 vWorldPosWS;
in vec2 vTexcoords;

// OUT
layout(location = 0) out vec4 buffer1;
layout(location = 1) out vec4 buffer2;
layout(location = 2) out vec4 buffer3;
layout(location = 3) out vec4 buffer4;

uniform float ubMetalOrSpec;
uniform float ubDiffuse;
uniform float ubSpecular;
uniform float ubDiffuseIbl;
uniform float ubSpecularIbl;
uniform float uGlossiness;
uniform float uReflectivity;
uniform float uExposure;
uniform vec3 uLightDir;
uniform vec3 uLightCol;
uniform vec3 uRgbDiff;
uniform vec3 uLightPositions[4];
uniform vec3 uLightColors[4];

void main()
{
    // Material params. uMetallicMap
    vec3  inAlbedo = uRgbDiff;
    float inMetallic = uReflectivity;
    float inRoughness = uGlossiness;

    buffer1 = vec4(inAlbedo, inMetallic);
    buffer2 = vec4(vViewDirWS, inRoughness);
    buffer3 = vec4(vWorldPosWS, 0.0);
    buffer4 = vec4(vNormalWS, 0.0);
}

