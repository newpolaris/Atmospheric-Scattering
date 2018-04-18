--Vertex

// IN
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 uWorld;
uniform mat4 uViewProj;

void main()
{
    gl_Position = (uViewProj * uWorld) * vec4(aPosition, 1.0);
}

-- Fragment

// OUT
out vec3 fragColor;

void main()
{
	fragColor = vec3(1, 1, 1);
}
