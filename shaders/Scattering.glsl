-- Vertex

// IN
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexcoords;

// Out
out vec2 vTexcoords;
out vec3 vNormalW;

uniform mat4 uModelToProj;

void main()
{
    vec4 position = uModelToProj*vec4(inPosition, 1.0);
	vTexcoords = inTexcoords;
    vNormalW = -inNormal;
	gl_Position = position.xyww;
}

-- Fragment

#include "PhaseFunctions.glsli"

// IN
in vec2 vTexcoords;
in vec3 vNormalW;

// OUT
out vec3 fragColor;

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
const float humanHeight = 1.0;
const float pi = 3.1415926535897932384626433832795;

uniform bool uChapman;
uniform float uEarthRadius;
uniform float uAtmosphereRadius;
uniform float uAspect;
uniform float uAngle;
uniform float uAltitude;
uniform vec2 uInvResolution;
uniform vec3 uEarthCenter;
uniform vec3 uSunDir;
uniform vec3 uSunIntensity;
uniform vec3 betaR0; // vec3(5.8e-6, 13.5e-6, 33.1e-6);
uniform vec3 betaM0; // vec3(21e-6);


// Ref. [Schuler12]
//
// this is the approximate Chapman function,
// corrected for transitive consistency
float chapman(float X, float h, float coschi)
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

vec2 raySphereIntersect(vec3 pos, vec3 dir, vec3 c, float r)
{
    vec3 tc = c - pos;

    float l = dot(tc, dir);
    float d = l*l - dot(tc, tc) + r*r;
    if (d < 0) return vec2(-1.0, -1.0);
    float sl = sqrt(d);
    return vec2(l - sl, l + sl);
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
		rayleigh = Hr * chapman(Xr, hr, coschi);
		mie = Hm * chapman(Xm, hm, coschi);
		return true;
	}
}

//
// https://www.scratchapixel.com/lessons/procedural-generation-virtual-worlds/simulating-sky
//
vec3 computeIncidentLight(vec3 pos, vec3 dir, vec3 intensity, float tmin, float tmax)
{
    vec2 t = raySphereIntersect(pos, dir, uEarthCenter, uAtmosphereRadius);

    tmin = max(t.x, tmin);
    tmax = min(t.y, tmax);

    if (tmax < 0)
        discard;

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
        vec2 tl = raySphereIntersect(x, uSunDir, uEarthCenter, uAtmosphereRadius);

        // light delta segment 
        float opticalDepthLightR = 0.0;
        float opticalDepthLightM = 0.0;

        if (!opticalDepthLight(x, tl, opticalDepthLightR, opticalDepthLightM))
            continue;
        
        vec3 tauR = betaR0 * (opticalDepthR + opticalDepthLightR);
        vec3 tauM = mieScale * betaM0 * (opticalDepthM + opticalDepthLightM);
        vec3 attenuation = exp(-(tauR + tauM));
        sumR += attenuation * betaR;
        sumM += attenuation * betaM;
    }

    float mu = dot(uSunDir, dir);
    float phaseR = CompuatePhaseRayleigh(mu);
    float phaseM = ComputePhaseMie(mu, g);
    return intensity * (sumR*phaseR*betaR0 + sumM*phaseM*betaM0);
}

// ----------------------------------------------------------------------------
void main() 
{
    vec3 color = vec3(0, 0, 0);
    vec3 dir = normalize(-vNormalW);

    vec3 cameraPos = vec3(0.0, humanHeight + uEarthRadius, 0.0) + vec3(0.0, uAltitude, 0.0);
    vec2 t = raySphereIntersect(cameraPos, dir, uEarthCenter, uEarthRadius);
    // handle ray toward ground
    float tmax = inf;
    if (t.y > 0) tmax = max(0.0, t.x);

    color += computeIncidentLight(cameraPos, dir, uSunIntensity, 0.0, tmax);

    fragColor = color;
}
