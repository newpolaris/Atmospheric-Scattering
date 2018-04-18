-- Vertex

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out VS_OUT {
    vec3 FragPos;
    vec3 NormalW;
    vec4 PositionW;
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.NormalW = mat3(model) * aNormal;
    vs_out.PositionW = model * vec4(aPos, 1.0);
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}

-- Fragment

struct lightData
{
    float enabled;
    float type; // 0 = pointlight 1 = directionlight
    float a, b;
    vec4 ambient;
    vec4 position; // where are we
    vec4 diffuse; // how diffuse
    vec4 specular; // what kinda specular stuff we got going on?
    // attenuation
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
    float c;
    // spot and area
    vec3 spotDirection;
    float d;
    // only for area
    float width;
    float height;
    float e, f;
    vec3 right;
    float g;
    vec3 up;
    float h;
};

out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 NormalW;
    vec4 PositionW;
} fs_in;

uniform Light
{
    lightData lights;
};

uniform mat4 view;
uniform vec4 mat_ambient;
uniform vec4 mat_diffuse;
uniform vec4 mat_specular;
uniform vec4 mat_emissive;
uniform float mat_shininess;

uniform vec3 uCameraPos; // world

vec3 projection(in vec3 p, in vec3 p0, in vec3 normal)
{
    return dot(p - p0, normal) * normal;
}

vec3 projectOnPlane(in vec3 pos, in vec3 planeCenter, in vec3 planeNormal)
{
    return pos - projection(pos, planeCenter, planeNormal);
}

vec3 linePlaneIntersect(in vec3 lp, in vec3 lv, in vec3 pc, in vec3 pn)
{
    return lp + lv * dot(pn, pc - lp)/dot(pn, lv);
}

vec2 project2D(vec3 dir, vec3 right, vec3 up)
{
    return vec2(dot(dir, right), dot(dir, up));
}

vec2 nearest2D(vec2 dir, vec2 x, vec2 y)
{
    return vec2(clamp(dir.x, x.r, x.g), clamp(dir.y, y.r, y.g));
}

void areaLight( in lightData light, in vec3 normalW, in vec3 positionW, inout vec3 ambient, inout vec3 diffuse, inout vec3 specular )
{
    vec3 lightPos = light.position.xyz;
    vec3 right = light.right;
    // light plane normal, front direction
    vec3 direction = light.spotDirection;
    vec3 up = light.up;

    float width = light.width * 0.5;
    float height = light.height * 0.5;

    // project onto plane and calculate direction from center to the projection.
    vec3 projection = projectOnPlane(positionW, lightPos, direction);
    vec3 dirDiff = projection - lightPos;

    // project onto 2D areaLight plane to compare with width/height
    vec2 dirDiff2D = project2D(dirDiff, right, up);
    vec2 nearestDiff2D = nearest2D(dirDiff2D, vec2(-width, width), vec2(-height, height));
    vec3 nearestPointInside = lightPos + right * nearestDiff2D.x + up * nearestDiff2D.y;
    float dist = distance(positionW, nearestPointInside); // real distance to area rectangle
    vec3 diffuseDir = normalize(nearestPointInside - positionW);
    float attenuation = 1.0 / (light.constantAttenuation + light.linearAttenuation * dist + light.quadraticAttenuation * dist * dist);

    float ndotl = max(dot(direction, -diffuseDir), 0.0);
    float ndotl2 = max(dot(normalW, diffuseDir), 0.0);
    if (ndotl * ndotl2 > 0.0)
    {
        float diffuseFactor = sqrt(ndotl * ndotl2);

        // find point e that intersect with reflect vector in light area
        vec3 r = reflect(normalize(uCameraPos - positionW), normalW);
        vec3 e = linePlaneIntersect(positionW, r, lightPos, direction);
        float specAngle = dot(r, direction);
        if (specAngle > 0.0)
        {
            vec3 dirSpec = e - lightPos;
            vec2 dirSpec2D = project2D(dirSpec, right, up);
            vec2 nearestSpec2D = nearest2D(dirSpec2D, vec2(-width, width), vec2(-height, height));
            // to get sharp edge line
            float edgeFactor = 5.0;
            float specFactor = 1.0 - clamp(length(nearestSpec2D - dirSpec2D) * edgeFactor * mat_shininess, 0, 1);
            specular += light.specular.rgb * specFactor * specAngle * diffuseFactor * attenuation;
        }
        diffuse += light.diffuse.rgb * diffuseFactor * attenuation;
    }
    ambient += light.ambient.rgb * attenuation;
}

void main()
{           
    vec3 ambient = vec3(0.1, 0.1, 0.1);
    vec3 diffuse = vec3(0.0, 0.0, 0.0);
    vec3 specular = vec3(0.0, 0.0, 0.0);

    vec3 w_transformedNormal = normalize(fs_in.NormalW);
    vec3 w_Position = vec3(fs_in.PositionW);

    areaLight(lights, w_transformedNormal, w_Position, ambient, diffuse, specular);

    vec4 localColor = vec4(ambient, 1.0) * mat_ambient + vec4(diffuse, 1.0) * mat_diffuse + vec4(specular, 1.0) * mat_specular + mat_emissive;
    FragColor = localColor;
}