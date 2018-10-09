//------------------------------------------------------------------------------


-- Vertex

// IN
layout(location = 0) in vec3 aPos;
// layout(location = 0) in vec4 inPosition;

// OUT
out vec3 WorldPos;
// out vec2 vTexCoord;

// UNIFORM
uniform mat4 projection;
uniform mat4 view;
// uniform mat4 uModelViewProjMatrix;

void main()
{
    WorldPos = aPos;
    gl_Position =  projection * view * vec4(WorldPos, 1.0);
}



--