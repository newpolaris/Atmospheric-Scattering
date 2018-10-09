//------------------------------------------------------------------------------

-- Compute

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
layout(rgba16f, binding=0) uniform writeonly imageCube uCube;

const uint sampleCount = 32u;

shared float fInvTotalWeight;
shared vec3 vSampleDirections[sampleCount];
shared float fSampleMipLevels[sampleCount];
shared float fSampleWeights[sampleCount];

uniform samplerCube uEnvMap;
uniform float uRoughness;

const float pi = 3.14159265359;

// ----------------------------------------------------------------------------
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// efficient VanDerCorpus calculation.
float RadicalInverse_VdC(uint bits) 
{
     bits = (bits << 16u) | (bits >> 16u);
     bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
     bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
     bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
     bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
     return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
// ----------------------------------------------------------------------------
vec2 Hammersley(uint i, uint N)
{
	return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}
// ----------------------------------------------------------------------------
vec3 ImportanceSampleGGX(vec2 Xi, float roughness)
{
	float a = roughness*roughness;
	
	float phi = 2.0 * pi * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
	// from spherical coordinates to cartesian coordinates - halfway vector
	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;

    return H;
}
// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = pi * denom * denom;

    return nom / denom;
}

float CalcMipLevel(vec3 H, float Roughness)
{
    // sample from the environment's mip level based on roughness/pdf
    float D = DistributionGGX(vec3(0, 0, 1), H, Roughness);
    float ndoth = max(H.z, 0.0);
    float hdotv = max(H.z, 0.0);
    float pdf = D * ndoth / (4.0 * hdotv) + 0.0001;
    float resolution = 512.0; // resolution of source cubemap (per face)
    // Solid angle covered by 1 pixel with 6 faces that are resolution x resolution
    float omegaP = 4.0 * pi / (6.0 * resolution * resolution);
    // Solid angle represented by this sample
    float omegaS = 1.0 / (float(sampleCount) * pdf + 0.0001);
    // Original paper suggest biasing the mip to improve the results	
    float mipBias = 1.0;
    float mipLevel = 0.0;
    // Remove dot pattern at roughness 0
    if(Roughness > 0.0)
        mipLevel = max(0.5 * log2(omegaS / omegaP) + mipBias, 0.0);
    return mipLevel;
}

vec3 PrefilterEnvMap(float Roughness, vec3 R)
{
	vec3 N = R;
	vec3 V = R;

	// from tangent-space H vector to world-space sample vector
	vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent   = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);
    mat3 tangentToWorld = mat3(tangent, bitangent, N);
	
	vec3 prefilterColor = vec3(0.0);
    float weight = 0.0;
	for (uint s = 0u; s < sampleCount; s++)
	{
        float w = fSampleWeights[s];
		vec3 L = tangentToWorld * vSampleDirections[s];
        prefilterColor += textureLod(uEnvMap, L, fSampleMipLevels[s]).rgb * w;
        weight += w;
	}
	return prefilterColor / weight;
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
        vec3 H = ImportanceSampleGGX(Xi, uRoughness);
    	vec3 V = vec3(0, 0, 1);
	
        // Optimized local coordinate ref. placeholderart [7]
        float mipLevel = CalcMipLevel(H, uRoughness);

        // Compute local reflected vector L from H
        vec3 L = normalize(2.0 * H.z * H - V);
        fSampleMipLevels[si] = mipLevel;
        vSampleDirections[si] = L;
        fSampleWeights[si] = L.z;
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
	vec3 color = PrefilterEnvMap(uRoughness, dir);

	imageStore(uCube, ivec3(x, y, l), vec4(color, 0));
}