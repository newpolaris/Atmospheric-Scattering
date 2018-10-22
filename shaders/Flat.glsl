-- Vertex

// IN
layout (location = 0) in vec3 inPosition;

uniform mat4 uMatWorldViewProject;

void main()
{
    gl_Position = uMatWorldViewProject*vec4(inPosition, 1.0);
}

-- Fragment

out vec4 fragColor;

void main()
{           
    fragColor = vec4(1.0, 0.0, 0.0, 1.0);
}