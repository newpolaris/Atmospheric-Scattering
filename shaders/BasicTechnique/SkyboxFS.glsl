#version 430

#include "../System.conf"
#include "../Common.glsli"
#include "../Math.glsli"

#define SKYBOX_HDR_RGBT_ENABLE 1
#define SKYBOX_HDR_ENABLE 1

// IN
in vec3 vNormal;

// OUT
out vec4 fragColor;

uniform sampler2D uSkyboxMapSamp;

void main()
{
    vec3 N = normalize(vNormal);
    vec2 uv = ComputeSphereCoord(N);
#if SKYBOX_HDR_RGBT_ENABLE
    vec3 color = DecodeRGBT(textureLod(uSkyboxMapSamp, uv, 0));
#else
    vec3 color = vec3(textureLod(uSkyboxMapSamp, uv, 0));
#endif

#if SKYBOX_HDR_ENABLE
	fragColor = vec4(color, 1.0);
#else
	fragColor = vec4(srgb2linear(color), 1.0);
#endif
}
