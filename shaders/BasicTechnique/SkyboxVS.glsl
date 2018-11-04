#version 430

// IN
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;

// Out
out vec3 vNormal;

uniform vec3 uCameraPosition;
uniform mat4 uMatViewProject;

void main()
{
	const float scale = 1000.0;

#if SKYDOME_ENABLE
	vNormal = normalize(inPosition);
	vNormal.y = vNormal.y<-0.05 ? -0.05 : vNormal.y;
	vNormal.y += 0.04999;
	gl_Position = uMatViewProject*vec4(vNormal.xyz * scale, 1);
	vNormal.y -= 0.04999;
#else
	vNormal = normalize(inPosition);
	gl_Position = uMatViewProject*vec4(inPosition*scale, 1.0);
#endif
}
