-- Vertex

// IN
layout (location = 0) in vec3 inPosition;

uniform mat4 uModelToProj;

void main()
{
    vec4 position = uModelToProj*vec4(inPosition, 1.0);
	gl_Position = position.xyww;
}

-- Fragment

out vec4 fragColor;
void main()
{           
    fragColor = vec4(1.0, 0.0, 0.0, 1.0);
}