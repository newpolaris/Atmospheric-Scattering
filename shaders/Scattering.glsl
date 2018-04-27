-- Vertex

// IN
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexcoords;

// Out
out vec2 vTexcoords;

void main()
{
	vTexcoords = inTexcoords;
	gl_Position = vec4(inPosition, 1.0);
}

-- Fragment

// IN
in vec2 vTexcoords;

// OUT
out vec3 fragColor;

const int numScatteringSamples = 16;
const int numLightSamples = 8;
const int numSamples = 4;
// simple scaling factor, without specific reason
const float mieScale = 1.1;
// big number
const float inf = 9.0e8;
const float Hr = 7994.0;
const float Hm = 1220.0;
const float g = 0.76f;
const float pi = 3.1415926535897932384626433832795;
const vec3 betaR0 = vec3(3.8e-6, 13.5e-6, 33.1e-6);
const vec3 betaM0 = vec3(21e-6);

uniform bool uChapman;
uniform float uEarthRadius;
uniform float uAtmosphereRadius;
uniform float uAspect;
uniform float uAngle;
uniform vec2 uInvResolution;
uniform vec3 uEarthCenter;
uniform vec3 uSunDir;
uniform vec3 uSunIntensity;
uniform vec2 uSamples[numSamples];

// Ref. [Schuler12](GPU PRO 2)
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
// for detailed algorithm and description
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
    float phaseR = 3.0 / (16.0*pi) * (1.0 + mu*mu);
    float phaseM = 3.0 / (8.0*pi) * ((1 - g*g)*(1 + mu*mu))/((2 + g*g)*pow(1 + g*g - 2*g*mu, 1.5));
    return intensity * (sumR*phaseR*betaR0 + sumM*phaseM*betaM0);
}

// From: https://briansharpe.wordpress.com/2011/11/15/a-fast-and-simple-32bit-floating-point-hash-function/
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

// ----------------------------------------------------------------------------
void main() 
{
    vec2 jitter = FAST_32_hash(gl_FragCoord.xy).xy;
    vec3 color = vec3(0, 0, 0);
    for (int k = 0; k < numSamples; k++)
    {
        vec2 u = fract(jitter + uSamples[k]);
        vec2 xy = 2.0*(gl_FragCoord.xy + u)*uInvResolution - vec2(1.0);
        xy *= uAngle;
        xy.x *= uAspect;
        vec3 dir = normalize(vec3(xy, -1.0));

        vec3 cameraPos = vec3(0.0, uEarthRadius + 1.0e3, 3.0e4);
        vec2 t = raySphereIntersect(cameraPos, dir, uEarthCenter, uEarthRadius);
        // handle ray toward ground
        float tmax = inf;
        if (t.y > 0) tmax = max(0.0, t.x);

        color += computeIncidentLight(cameraPos, dir, uSunIntensity, 0.0, tmax);
    }

    fragColor = color / numSamples;
}
