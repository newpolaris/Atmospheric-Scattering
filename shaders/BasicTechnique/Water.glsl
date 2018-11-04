-- Vertex

#extension GL_ARB_gpu_shader5 : enable

// Water effect shader that uses reflection and refraction maps projected onto
// the water These maps are distorted based on the two scrolling normal maps.

// IN
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexcoord;

out vec4 vPositionCS;
out vec3 vToEyeWS;
out vec2 vTexcoord;

uniform vec3 uCameraPositionWS;
uniform mat4 uMatWorld;
uniform mat4 uMatViewProject;

void main()
{
    vec4 positionWS = uMatWorld*vec4(inPosition, 1.0);

	// Compute the vector from the vertex to the eye.
    vToEyeWS = vec3(positionWS) - uCameraPositionWS;

	// Scroll texture coordinates.
    vTexcoord = inTexcoord;
    gl_Position = vPositionCS = uMatViewProject*positionWS;
}

-- Fragment

#include "../Common.glsli"
#include "../Math.glsli"

in vec4 vPositionCS;
in vec3 vToEyeWS;
in vec2 vTexcoord;

out vec4 fragColor;

uniform vec4 uWaterColor;
uniform vec3 uSunDirectionWS;
uniform vec4 uSunColor; 

//Flow map offsets used to scroll the wave maps
uniform float uFlowMapOffset0;
uniform	float uFlowMapOffset1;

// scale used on the wave maps
uniform float uTexScale;
uniform float uHalfCycle;

uniform float uSunFactor; //the intensity of the sun specular term.
uniform float uSunPower; //how shiny we want the sun specular term on the water to be.

uniform sampler2D uFlowMapSamp;
uniform sampler2D uNoiseMapSamp;
uniform sampler2D uWaveMap0Samp;
uniform sampler2D uWaveMap1Samp;

const float R0 = 0.02037f;

#if 1
void main()
{
	// transform the projective texcoords to NDC space
	// and scale and offset xy to correctly sample a DX texture
	vec3 projTexTS = vPositionCS.xyz / vPositionCS.w;            
	projTexTS.xy =  0.5*projTexTS.xy + 0.5; 
	projTexTS.z = 0.1 / projTexTS.z; //refract more based on distance from the camera

    vec3 toEyeWS = normalize(vToEyeWS);

	// Light vector is opposite the direction of the light.
	vec3 lightVecWS = -uSunDirectionWS;
	
	//get and uncompress the flow vector for this pixel
	vec2 flowmap = texture( uFlowMapSamp, vTexcoord ).rg * 2.0f - 1.0f;
	float cycleOffset = texture( uNoiseMapSamp, vTexcoord ).r;

	float phase0 = cycleOffset * .5f + uFlowMapOffset0;
	float phase1 = cycleOffset * .5f + uFlowMapOffset1;

	// Sample normal map.
	vec3 normalT0 = texture(uWaveMap0Samp, ( vTexcoord*uTexScale ) + flowmap * phase0 ).xyz;
	vec3 normalT1 = texture(uWaveMap1Samp, ( vTexcoord*uTexScale ) + flowmap * phase1 ).xyz;

	float f = ( abs( uHalfCycle - uFlowMapOffset0 ) / uHalfCycle );
    
	vec3 normalT = lerp( normalT0, normalT1, f );
    fragColor = vec4( normalT, 1.0f );
}
#else
void main()
{
    fragColor = texture(uWaveMap0Samp, vTexcoord);
}
#endif