-- Vertex

// IN
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexcoords;

// Out
out vec2 vTexcoords;
out vec3 vNormalW;

uniform vec3 uCameraPosition;
uniform mat4 uModelToProj;

void main()
{
    vec4 position = uModelToProj*vec4(inPosition + uCameraPosition, 1.0);
    gl_Position = position;
    vTexcoords = inTexcoords;
    vNormalW = -inPosition;
}

-- Fragment

#include "Math.glsli"
#include "PhaseFunctions.glsli"

// IN
in vec2 vTexcoords;
in vec3 vNormalW;

// OUT
out vec4 fragColor;

const int numScatteringSamples = 16;
const int numLightSamples = 8;
const int numSamples = 4;

// [Hillaire16] use 1.11 factor 
// [Preetham99] Mie coefficient ratio scattering / (absorption+scattering) is about to 0.9
const float mieScale = 1.11;

// big number
const float inf = 9.0e8;
const float Hr = 7994.0;
const float Hm = 1220.0;
const float g = 0.760;
// 1 m
const float humanHeight = 1.0;

// https://ozonewatch.gsfc.nasa.gov/facts/ozone.html
// https://en.wikipedia.org/wiki/Number_density
// Ozone scattering with its mass up to 0.00006%, 0.00006 is standard
// Ozone scattering with its number density up to 2.5040, 2.5040 is standard
const vec3 mOzoneMassParams = vec3(0.6e-6, 0.0, 0.9e-6) * 2.504;

// http://www.iup.physik.uni-bremen.de/gruppen/molspec/databases/referencespectra/o3spectra2011/index.html
// Version 22.07.2013: Fast Fourier Transform Filter applied to the initial data in the region 213.33 -317 nm 
// Ozone scattering with wavelength (680nm, 550nm, 440nm) and 293K
const vec3 mOzoneScatteringCoeff = vec3(1.36820899679147, 3.31405330400124, 0.13601728252538);

uniform bool uChapman;
uniform float uEarthRadius; 
uniform float uAtmosphereRadius;
uniform float uAspect;
uniform float uAngle;
uniform float uAltitude;
uniform float uTurbidity;
uniform vec2 uInvResolution;
uniform vec3 uEarthCenter;
uniform vec3 uSunDir;
uniform vec3 uSunIntensity;
uniform vec3 uCameraPosition;
uniform vec3 betaR0; // vec3(5.8e-6, 13.5e-6, 33.1e-6);
uniform vec3 betaM0; // vec3(21e-6);
// [Hillaire16]
uniform vec3 betaO0 = vec3(3.426, 8.298, 0.356) * 6e-7;


// Ref. [Schuler12]
//
// this is the approximate Chapman function,
// corrected for transitive consistency
float ChapmanApproximation(float X, float h, float coschi)
{
	float c = sqrt(X + h);
	if (coschi >= 0.0)
	{
		return	c / (c*coschi + 1.0) * exp(-h);
	}
	else
	{
		float x0 = sqrt(1.0 - coschi*coschi)*(X + h);
		float c0 = sqrt(x0);
		return 2.0*c0*exp(X - x0) - c/(1.0 - c*coschi)*exp(-h);
	}
}

bool opticalDepthLight(vec3 s, vec2 t, out float rayleigh, out float mie)
{
	if (!uChapman)
	{
		// start from position 's'
		float lmin = 0.0;
		float lmax = t.y;
		float ds = (lmax - lmin) / numLightSamples;
		float r = 0.f;
		float m = 0.f;
		for (int i = 0; i < numLightSamples; i++)
		{
			vec3 x = s + ds*(0.5 + i)*uSunDir;
			float h = length(x) - uEarthRadius;
			if (h < 0) return false;
			r += exp(-h/Hr)*ds;
			m += exp(-h/Hm)*ds;
		}
		rayleigh = r;
		mie = m;
		return true;
	}
	else
	{
		// approximate optical depth with chapman function  
		float x = length(s);
		float Xr = uEarthRadius / Hr; 
		float Xm = uEarthRadius / Hm;
		float coschi = dot(s/x, uSunDir);
		float xr = x / Hr;
		float xm = x / Hm;
		float hr = xr - Xr;
		float hm = xm - Xm;
		rayleigh = Hr * ChapmanApproximation(Xr, hr, coschi);
		mie = Hm * ChapmanApproximation(Xm, hm, coschi);
		return true;
	}
}

//
// https://www.scratchapixel.com/lessons/procedural-generation-virtual-worlds/simulating-sky
//
vec3 computeIncidentLight(vec3 pos, vec3 dir, vec3 intensity, float tmin, float tmax)
{
    vec2 t = ComputeRaySphereIntersection(pos, dir, uEarthCenter, uAtmosphereRadius);

    tmin = max(t.x, tmin);
    tmax = min(t.y, tmax);

    if (tmax < 0)
        discard;

    // see pig.8 in scratchapixel
    // tc: camera position
    // pb: tc (tmin is 0)
    // pa: intersection point with atmosphere
    vec3 tc = pos;
    vec3 pa = tc + tmax*dir;
    vec3 pb = tc + tmin*dir;

    float opticalDepthR = 0.0;
    float opticalDepthM = 0.0;
    float ds = (tmax - tmin) / numScatteringSamples; // delta segment

    vec3 sumR = vec3(0, 0, 0);
    vec3 sumM = vec3(0, 0, 0);

    for (int s = 0; s < numScatteringSamples; s++)
    {
        vec3 x = pb + ds*(0.5 + s)*dir;
        float h = length(x) - uEarthRadius;
        float betaR = exp(-h/Hr)*ds;
        float betaM = exp(-h/Hm)*ds;
        opticalDepthR += betaR;
        opticalDepthM += betaM;
        
        // find intersect sun lit with atmosphere
        vec2 tl = ComputeRaySphereIntersection(x, uSunDir, uEarthCenter, uAtmosphereRadius);

        // light delta segment 
        float opticalDepthLightR = 0.0;
        float opticalDepthLightM = 0.0;

        if (!opticalDepthLight(x, tl, opticalDepthLightR, opticalDepthLightM))
            continue;
        
        vec3 tauR = betaR0 * (opticalDepthR + opticalDepthLightR);
        vec3 tauM = mieScale * betaM0 * (opticalDepthM + opticalDepthLightM);
        // [KJH17][Hillaire16] Ozone has 0 scattering(absorption only) and similar betaR (distribution)
        // so, reuse optical depth for rayleigh
        vec3 tauO = betaO0 * (opticalDepthR + opticalDepthLightR);
        vec3 attenuation = exp(-(tauR + tauM + tauO));
        sumR += attenuation * betaR;
        sumM += attenuation * betaM;
    }

    float mu = dot(uSunDir, dir);
    float phaseR = ComputePhaseRayleigh(mu);
    float phaseM = ComputePhaseMie(mu, g);
    return intensity * (sumR*phaseR*betaR0 + sumM*phaseM*betaM0);
}

// ----------------------------------------------------------------------------
void main() 
{
    vec3 dir = normalize(-vNormalW);

    vec3 cameraPos = vec3(0.0, humanHeight + uAltitude + uEarthRadius, 0.0);
    vec2 t = ComputeRaySphereIntersection(cameraPos, dir, uEarthCenter, uEarthRadius);
    // handle ray toward ground
    float tmax = inf;
    if (t.y > 0) tmax = max(0.0, t.x);

    vec3 color = computeIncidentLight(cameraPos, dir, uSunIntensity, 0.0, tmax);

	float intersectionTest = float(t.x < 0.0 && t.y < 0.0);
    float angle = dot(dir, uSunDir);
    float edge = ((angle >= 0.9) ? smoothstep(0.9, 1.0, angle) : 0.0);
    if (edge > cos(radians(10)))
        color += vec3(1.0) * intersectionTest;

    fragColor = vec4(color, 1.0);
}

