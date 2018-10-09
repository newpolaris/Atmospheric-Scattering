//------------------------------------------------------------------------------

-- Compute

#include "Constants.glsli"
#include "Sampling.glsli"

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
layout(rgba16f, binding=0) uniform writeonly imageCube uCube;

const uint sampleCount = 96u;

shared vec3 vSampleDirections[sampleCount];

uniform samplerCube uEnvMap;

// ----------------------------------------------------------------------------
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html	
vec3 ImportanceSampleSphereUniform(vec2 Xi);
vec4 ImportanceSampleHemisphereUniform(vec2 Xi);
vec3 ImportanceSampleHemisphereCosine(vec2 Xi)
{
	float phi = 2 * pi * Xi.x;
	float cosTheta = sqrt(Xi.y);
	float sinTheta = sqrt( 1 - cosTheta * cosTheta );
	float pdf = cosTheta / pi;

	vec3 H;
	H.x = sinTheta * cos( phi );
	H.y = sinTheta * sin( phi );
	H.z = cosTheta;
	
    return H;
}

vec3 DiffuseIBL(vec3 N)
{
	// from tangent-space H vector to world-space sample vector
	vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent   = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);
    mat3 tangentToWorld = mat3(tangent, bitangent, N);

    float weight = 0.0;
	vec3 color = vec3(0.0);
	for (uint s = 0u; s < sampleCount; s++)
	{
        vec3 L = vSampleDirections[s];
		L = tangentToWorld * L;
        float ndotl = clamp(dot(L, N), 0, 1);
        color += textureLod(uEnvMap, L, 6).rgb * ndotl;
        weight += ndotl;
	}
	return color / weight;
}

// Use code glow-extras's
vec3 Direction(float x, float y, uint l)
{
	// see ogl spec 8.13. CUBE MAP TEXTURE SELECTION	
	switch(l) {
		case 0: return vec3(+1, -y, -x); // +x
		case 1: return vec3(-1, -y, +x); // -x
		case 2: return vec3(+x, +1, +y); // +y
		case 3: return vec3(+x, -1, -y); // -y
		case 4: return vec3(+x, -y, +1); // +z
		case 5: return vec3(-x, -y, -1); // -z
	}
	return vec3(0, 1, 0);
}

void main()
{
    uint si = gl_LocalInvocationIndex;
    if (si < sampleCount)
    {
        vec2 Xi = Hammersley(si, sampleCount);
		vec3 L = ImportanceSampleHemisphereCosine(Xi);
        vSampleDirections[si] = L;
    }
    // Ensure shared memory writes are visible to work group
    memoryBarrierShared();

    // Ensure all threads in work group   
    // have executed statements above
    barrier();

	uint x = gl_GlobalInvocationID.x;	
	uint y = gl_GlobalInvocationID.y;
	uint l = gl_GlobalInvocationID.z;
	ivec2 s = imageSize(uCube);

	// check out of bounds
	if (x >= s.x || y >= s.y)
		return;

	float fx = (float(x) + 0.5) / float(s.x);
	float fy = (float(y) + 0.5) / float(s.y);

	vec3 dir = normalize(Direction(fx * 2 - 1, fy * 2 - 1, l));	
	vec3 color = DiffuseIBL(dir);

	imageStore(uCube, ivec3(x, y, l), vec4(color, 0));
}
