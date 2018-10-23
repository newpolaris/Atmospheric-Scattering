-- Vertex

// IN
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexcoords;

// Out
out vec2 vTexcoords;

void main()
{
	vTexcoords = inTexcoords;
	gl_Position = vec4(inPosition, 1.0);
}

-- Fragment

#define HDR_TONEMAP_OPERATOR 0

#include "Common.glsli"
#include "Math.glsli"
#include "ToneMappingUtility.glsli"

// IN
in vec2 vTexcoords;
uniform sampler2D uTexSource;

// OUT
out vec3 fragColor;

// ----------------------------------------------------------------------------
void main() 
{
    vec3 color = texture(uTexSource, vTexcoords).rgb;
    // color = ColorToneMapping(color);
    // color = linear2srgb(color);

    fragColor = color;
}
