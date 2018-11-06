-- Vertex

#extension GL_ARB_gpu_shader5 : enable

// Water effect shader that uses reflection and refraction maps projected onto
// the water These maps are distorted based on the two scrolling normal maps.

// IN
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexcoord;

out vec4 vPositionWS;
out vec4 vPositionCS;
out vec3 vToEyeWS;
out vec2 vTexcoord;

uniform vec3 uCameraPositionWS;
uniform mat4 uMatWorld;
uniform mat4 uMatViewProject;

void main()
{
    vec4 positionWS = uMatWorld*vec4(inPosition, 1.0);
    vPositionWS = positionWS;

	// Compute the vector from the vertex to the eye.
    vToEyeWS = vec3(positionWS) - uCameraPositionWS;

	// Scroll texture coordinates.
    vTexcoord = vec2(inTexcoord.x, 1.0 - inTexcoord.y);
    gl_Position = vPositionCS = uMatViewProject*positionWS;
}

-- Fragment

#include "../Common.glsli"
#include "../Math.glsli"

in vec4 vPositionWS;
in vec4 vPositionCS;
in vec3 vToEyeWS;
in vec2 vTexcoord;

out vec4 fragColor;

uniform vec4 uWaterColor;
uniform vec3 uSunDirectionWS;
uniform vec3 uCameraPositionWS;
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
uniform sampler2D uNoiseSmoothMapSamp;
uniform sampler2D uWaveMap0Samp;
uniform sampler2D uWaveMap1Samp;
uniform sampler2D uRefractMapSamp;
uniform sampler2D uReflectMapSamp;

const float R0 = 0.02037f;

// noise function
// From: https://briansharpe.wordpress.com/2011/11/15/a-fast-and-simple-32bit-floating-point-hash-function/
//
//    FAST_32_hash
//    A very fast 2D hashing function.  Requires 32bit support.
//
//    The hash formula takes the form....
//    hash = mod( coord.x * coord.x * coord.y * coord.y, SOMELARGEFLOAT ) / SOMELARGEFLOAT
//    We truncate and offset the domain to the most interesting part of the noise.
//
vec4 FAST_32_hash(vec2 gridcell)
{
    // gridcell is assumed to be an integer coordinate
    const vec2 OFFSET = vec2(26.0, 161.0);
    const float DOMAIN = 71.0;
    const float SOMELARGEFLOAT = 951.135664;
    vec4 P = vec4(gridcell.xy, gridcell.xy + vec2(1, 1));
    P = P - floor(P * (1.0 / DOMAIN)) * DOMAIN;    //    truncate the domain
    P += OFFSET.xyxy;                              //    offset to interesting part of the noise
    P *= P;                                        //    calculate and return the hash
    return fract(P.xzxz * P.yyww * (1.0 / SOMELARGEFLOAT));
}

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
	float cycleOffset = texture( uNoiseSmoothMapSamp, vTexcoord ).r;

	float phase0 = cycleOffset * .5f + uFlowMapOffset0;
	float phase1 = cycleOffset * .5f + uFlowMapOffset1;

	// Sample normal map.
	vec3 normalT0 = texture(uNoiseMapSamp, ( vTexcoord*uTexScale ) + flowmap * phase0 ).xyz;
	vec3 normalT1 = texture(uNoiseMapSamp, ( vTexcoord*uTexScale ) + flowmap * phase1 ).xyz;

	float flowLerp = ( abs( uHalfCycle - uFlowMapOffset0 ) / uHalfCycle );

	vec3 normalT2 = lerp( normalT0, normalT1, flowLerp );
    fragColor = vec4( normalT2.xxx, 1.0 );
    return;

	 //unroll the normals retrieved from the normalmaps
    normalT0.yz = normalT0.zy;	
	normalT1.yz = normalT1.zy;
	
	normalT0 = 2.0f*normalT0 - 1.0f;
    normalT1 = 2.0f*normalT1 - 1.0f;
    
	vec3 normalT = lerp( normalT0, normalT1, flowLerp );
	
	//get the reflection vector from the eye
	vec3 R = normalize(reflect(toEyeWS, normalT));
	
	vec4 finalColor;
	finalColor.a = 1;

	//compute the fresnel term to blend reflection and refraction maps
	float ang = saturate(dot(-toEyeWS,normalT));
	float f = R0 + (1.0f-R0) * pow(1.0f-ang,5.0);	
	
	//also blend based on distance
	f = min(1.0f, f + 0.007f * uCameraPositionWS.y);	
		
	//compute the reflection from sunlight
	float sunFactor = uSunFactor;
	float sunPower = uSunPower;
	
	if(uCameraPositionWS.y < vPositionWS.y)
	{
		sunFactor = 7.0f; //these could also be sent to the shader
		sunPower = 55.0f;
	}
    vec3 temp = vec3(pow(saturate(dot(R, lightVecWS)), sunPower));
	vec3 sunlight = sunFactor * temp * vec3(uSunColor);

	vec4 refl = texture(uReflectMapSamp, projTexTS.xy + projTexTS.z * normalT.xz);
	vec4 refr = texture(uRefractMapSamp, projTexTS.xy - projTexTS.z * normalT.xz);
	
	//only use the refraction map if we're under water
	if (uCameraPositionWS.y < vPositionWS.y)
		f = 0.0f;
	
	//interpolate the reflection and refraction maps based on the fresnel term and add the sunlight
	// finalColor.rgb = uWaterColor * lerp( refr, refl, f) + sunlight;
	finalColor.rgb = vec3(uWaterColor) + sunlight;
	
    fragColor = finalColor;
}