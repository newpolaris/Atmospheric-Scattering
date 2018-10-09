#version 450 core

#include "Common.glsli"
#include "Math.glsli"

// IN
in vec4 vTexcoord;
in vec3 vDirection;

// OUT
layout(location = 0) out vec4 fragColor;

uniform sampler2D uTextureSamp;

void main()
{  
	vec3 V = normalize(vDirection);
    vec2 coord = vTexcoord.xy / vTexcoord.w;
    vec3 color = DecodeRGBT(textureLod(uTextureSamp, coord, 0));
	fragColor = vec4(color, 1.0);
}
