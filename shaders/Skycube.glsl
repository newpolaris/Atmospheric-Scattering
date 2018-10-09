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
	mat4 rotView = mat4(mat3(uViewMatrix));
	vec4 clipPos = uProjMatrix * rotView * inPosition;
	gl_Position = clipPos.xyww;

	vDirection = inPosition.xyz;
}

-- Fragment

// IN
in vec3 vDirection;

// OUT
layout(location = 0) out vec4 fragColor;

uniform samplerCube uEnvmapSamp;
uniform samplerCube uEnvmapIrrSamp;
uniform samplerCube uEnvmapPrefilterSamp;
uniform float uBgType;
uniform float uExposure;

void main()
{  
	vec3 dir = normalize(vDirection);

    vec4 color;
	if (uBgType == 7.0)
	{
		color = texture(uEnvmapIrrSamp, dir);
	}
	else if (uBgType == 0.0)
	{
		color = texture(uEnvmapSamp, dir);
	}
	else
	{
		// Omit 0th mip which roughness is 0.0. it is similar to the result of non filtered
		color = textureLod(uEnvmapPrefilterSamp, dir, uBgType-1);
	}
	color.rgb *= exp2(uExposure);

	fragColor = color;
}
