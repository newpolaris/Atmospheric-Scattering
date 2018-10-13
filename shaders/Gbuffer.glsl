--Vertex

// IN
layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexcoords;

// OUT
out vec3 vNormalWS;
out vec3 vViewDirWS;
out vec3 vWorldPosWS;
out vec3 vPositionVS;
out vec2 vTexcoords;
out vec2 vDepthZW;

// UNIFORM
uniform mat4 uMtxSrt;
uniform mat4 uModelViewMatrix;
uniform mat4 uModelViewProjMatrix;
uniform vec3 uEyePosWS;

void main()
{
    // Clip Space position
    gl_Position = uModelViewProjMatrix * uMtxSrt * inPosition;
    vec4 positionVS = uModelViewMatrix * uMtxSrt * inPosition;

    // World Space normal
    vec3 normal = mat3(uMtxSrt) * inNormal;
    vNormalWS = normalize(normal);

    vTexcoords = inTexcoords;

    // World Space view direction from world space position
    vec3 posWS = vec3((uMtxSrt * inPosition).xyz);
    vViewDirWS = normalize(uEyePosWS - posWS);
    vWorldPosWS = posWS;
    vDepthZW = gl_Position.zw;
    vPositionVS = positionVS.xyz;
}


--

//------------------------------------------------------------------------------


--Fragment

// IN
in vec3 vNormalWS;
in vec3 vViewDirWS;
in vec3 vWorldPosWS;
in vec3 vPositionVS;
in vec2 vTexcoords;
in vec2 vDepthZW;

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
uniform mat4 uProjection;

const float A = uProjection[2].z;
const float B = uProjection[3].z;
const float near = -B / (1.0 - A);
const float far = B / (1.0 + A);

float ViewDepth(float fragZ)
{
    float z = fragZ * 2.0 - 1.0;
    return (2.0 * near * far) / (far + near - z * (far - near));
}

float LinearizeDepth(float fragZ)
{
    return ViewDepth(fragZ) / far;
}

void main()
{
    // Material params. uMetallicMap
    vec3  inAlbedo = uRgbDiff;
    float inMetallic = uReflectivity;
    float inRoughness = uGlossiness;
    float ndcDepth = gl_FragCoord.z * 2 - 1; // = vDepthZW.x / vDepthZW.y = [-1, 1]
    float linearDepth = LinearizeDepth(gl_FragCoord.z);
    float viewDist = length(vPositionVS);
    buffer1 = vec4(inAlbedo, inMetallic);
    buffer2 = vec4(linearDepth, viewDist, 0.0, inRoughness);
    buffer3 = vec4(vWorldPosWS, ndcDepth);
    buffer4 = vec4(vNormalWS, 0.0);
}

