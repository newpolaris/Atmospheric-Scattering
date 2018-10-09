

//------------------------------------------------------------------------------


-- Fragment

#define RGBT_TEXTURE 1
#if !RGBT_TEXTURE

out vec4 FragColor;
in vec3 WorldPos;

uniform sampler2D equirectangularMap;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{		
    vec2 uv = SampleSphericalMap(normalize(WorldPos));
    vec3 color = texture(equirectangularMap, uv).rgb;
    
    FragColor = vec4(color, 1.0);
}

#else

#include "Common.glsli"
#include "Math.glsli"

out vec4 FragColor;
in vec3 WorldPos;

uniform sampler2D equirectangularMap;

void main()
{		
    vec3 N = normalize(WorldPos);
    N.y = -N.y; // DX TO OGL
    vec2 uv = ComputeSphereCoord(N);
    vec3 color = DecodeRGBT(textureLod(equirectangularMap, uv, 0));
    FragColor = vec4(color, 1.0);
}

#endif

--