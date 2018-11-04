-- Vertex

// IN
layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;

// OUT
out vec3 vDirection;

// UNIFORM
uniform mat4 uViewMatrix;
uniform mat4 uProjMatrix;

void main()
{
	float scale = 100.0;
	mat4 rotView = mat4((uViewMatrix));
	vec4 clipPos = uProjMatrix * rotView * (inPosition*vec4(scale, scale, scale, 1.0));
	gl_Position = clipPos.xyzw;

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
