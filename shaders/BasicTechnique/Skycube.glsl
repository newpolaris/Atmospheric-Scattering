-- Vertex

#define CUBE_MAP_METHOD 1

// IN
layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;

// OUT
out vec3 vDirection;

// UNIFORM
uniform mat4 uMatView;
uniform mat4 uMatProject;

void main()
{
#if CUBE_MAP_METHOD > 0
	float scale = 1000.f;
	gl_Position = uMatProject * uMatView * (inPosition*vec4(scale, scale, scale, 1.0));
#else
	mat4 rotView = mat4(mat3(uMatView));
	vec4 clipPos = uMatProject * rotView * inPosition;
	gl_Position = clipPos.xyww;
#endif

	vDirection = inPosition.xyz;
}

-- Fragment

// IN
in vec3 vDirection;

// OUT
layout(location = 0) out vec4 fragColor;

uniform samplerCube uEnvmapSamp;

void main()
{  
	vec3 dir = normalize(vDirection);
    fragColor  = texture(uEnvmapSamp, dir);
}
