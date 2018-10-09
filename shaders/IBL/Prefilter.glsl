//------------------------------------------------------------------------------

-- Fragment

out vec4 FragColor;
in vec3 WorldPos;

uniform float uRoughness;
uniform samplerCube environmentCube;

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
vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
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
	
	// from tangent-space H vector to world-space sample vector
	vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent   = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);
	
	vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
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

void main()
{
	vec3 N = normalize(WorldPos);	
	vec3 R = N;
	vec3 V = R;

	uint sampleCount = 32u;
	vec3 prefilterColor = vec3(0.0);
	float totalWeight = 0.0;
	for (uint s = 0u; s < sampleCount; s++)
	{
		// low discrepancy sequence
		vec2 Xi = Hammersley(s, sampleCount);
		vec3 H = ImportanceSampleGGX(Xi, N, uRoughness);
		vec3 L = normalize(2.0 * dot(V, H) * H - V);
		float ndotl = max(dot(N, L), 0.0);
		if (ndotl > 0)
		{
			// sample from the environment's mip level based on roughness/pdf
			float D = DistributionGGX(N, H, uRoughness);
			float ndoth = max(dot(N, H), 0.0);
			float hdotv = max(dot(H, V), 0.0);
			float pdf = D * ndoth / (4.0 * hdotv) + 0.0001;
			float resolution = 512.0; // resolution of source cubemap (per face)
			// Solid angle covered by 1 pixel with 6 faces that are resolution x resolution
			float omegaP = 4.0 * pi / (6.0 * resolution * resolution);
			// Solid angle represented by this sample
			float omegaS = 1.0 / (float(sampleCount) * pdf + 0.0001);
			// Original paper suggest biasing the mip to improve the results	
			float mipBias = 1.0;
			float mipLevel = max(0.5 * log2(omegaS / omegaP) + mipBias, 0.0);
			prefilterColor += textureLod(environmentCube, L, mipLevel).rgb * ndotl;
			totalWeight += ndotl;
		}
	}

	prefilterColor /= totalWeight;
	FragColor = vec4(prefilterColor, 1.0);
}
