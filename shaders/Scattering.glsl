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

#define TEST_RAY_MMD 1
#define ATM_SAMPLES_NUMS 16
#define ATM_CLOUD_ENABLE 1
#define ATM_LIMADARKENING_ENABLE 0

const float pi = 3.1415926535897932384626433832795;

#include "Atmospheric.glsli"
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
// mSunPhaseParams = vec3(0.76, 0.65, 1.0); 		// Sun concentration with its phase functions up to 0.76, 0.76 is standard
const float g = 0.760;
// 1 m
const float humanHeight = 1.0;

// for parameter image
// https://github.com/gaj-cg/ray-mmd-docs-ja/wiki/

// scale for km to m
const float mUnitDistance = 1000.0;
// const vec3 mSunRadianceParams = vec3(10, 1.0, 20.0); 	// Sun light power, 10.0 is normal
const float mSunRadiance = 10.0;
const float mEarthRadius = 6360.0; // Earth radius up to 6360 km
const float mEarthAtmoRadius = 6420.0; // Earth radius with its atmospheric height up to 6420km

// [Preetham99]
const vec3 mWaveLength = vec3(680e-9, 550e-9, 440e-9); // standard earth lambda of 680nm, 550nm, 450nm
const vec3 mMieColor = vec3(0.686282f, 0.677739f, 0.663365f); // spectrum, note that ray-mmd use SunColor
const vec3 mRayleighColor = vec3(1.0, 1.0, 1.0); // Unknown
const vec3 mCloudColor = vec3(1.0, 1.0, 1.0); // ray-mmd use SunColor instead
// sky turbidity: (1.0 pure air to 64.0 thin fog)[Preetham99]
const vec3 mSunTurbidityParams = vec3(100, 1e-5, 500); 	// Sun turbidity  

// unknown values: vec3(default, min, max)
const vec3 mFogRangeParams = vec3(1, 1e-5, 10.0);
const vec3 mFogIntensityParams = vec3(1, 0.0, 200.0);
const vec3 mFogDensityParams = vec3(100, 0.1, 5000.0);
const vec3 mFogDensityFarParams = vec3(1e-2, 1e-5, 1e-1);

const vec3 mCloudSpeedParams = vec3(0.05, 0.0, 1.0);
const vec3 mCloudTurbidityParams = vec3(80, 1e-5, 200.0);
const vec3 mCloudDensityParams = vec3(400, 0.0, 1600.0);

const vec3 mMiePhaseParams = vec3(0.76, 0.65, 1.0);		// Mie scattering with its phase functions up to 0.76, 0.76 is standard
const vec3 mMieHeightParams = vec3(1.2, 1e-5, 2.4);		// Mie scattering with its water particles up to 1.2km, 1.2km is standard
const vec3 mMieTurbidityParams = vec3(200, 1e-5, 500); 	// Mie scattering with its wave length param

// Rayleigh scattering with its atmosphereic up to 8.0km, 8.0km is standard
const vec3 mRayleighHeightParams = vec3(8.0, 1e-5, 24.0);

// Precomputed Rayleigh scattering coefficients for wavelength lambda using the following formula
// F(lambda) = (8.0*PI/3.0) * (n^2.0 - 1.0)^2.0 * ((6.0+3.0*p) / (6.0-7.0*p)) / (lambda^4.0 * N)
// n : refractive index of the air (1.0003) https://en.wikipedia.org/wiki/Refractive_index
// p : air depolarization factor (0.035)
// N : air number density under NTP : (2.545e25 molecule * m^-3) 
// lambda : wavelength for which scattering coefficient is computed, standard earth lambda of (680nm, 550nm, 440nm)
const vec3 mRayleighScatteringCoeff = vec3(5.8e-6, 13.6e-6, 33.1e-6);

// https://ozonewatch.gsfc.nasa.gov/facts/ozone.html
// https://en.wikipedia.org/wiki/Number_density
// Ozone scattering with its mass up to 0.00006%, 0.00006 is standard
// Ozone scattering with its number density up to 2.5040, 2.5040 is standard
const vec3 mOzoneMassParams = vec3(0.6e-6, 0.0, 0.9e-6) * 2.504;

// http://www.iup.physik.uni-bremen.de/gruppen/molspec/databases/referencespectra/o3spectra2011/index.html
// Version 22.07.2013: Fast Fourier Transform Filter applied to the initial data in the region 213.33 -317 nm 
// Ozone scattering with wavelength (680nm, 550nm, 440nm) and 293K
const vec3 mOzoneScatteringCoeff = vec3(1.36820899679147, 3.31405330400124, 0.13601728252538);

const float mSunTurbidity = mSunTurbidityParams.x;
const float mOzoneMass = mOzoneMassParams.x;
const float mCloudSpeed = mCloudSpeedParams.x;
const float mCloudTurbidity = mCloudTurbidityParams.x;
const float mCloudDensity = mCloudDensityParams.x;
const float mMieHeight = mMieHeightParams.x;
const float mMieTurbidity = mMieTurbidityParams.x;
const float mRayleighHeight = mRayleighHeightParams.x;

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

vec2 ComputeRaySphereIntersection(vec3 pos, vec3 dir, vec3 c, float r)
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
        vec3 attenuation = exp(-(tauR + tauM));
        sumR += attenuation * betaR;
        sumM += attenuation * betaM;
    }

    float mu = dot(uSunDir, dir);
    float phaseR = ComputePhaseRayleigh(mu);
    float phaseM = ComputePhaseMie(mu, g);
    return intensity * (sumR*phaseR*betaR0 + sumM*phaseM*betaM0);
}

struct ScatteringParams
{
#if ATM_LIMADARKENING_ENABLE
	float sunRadius;
#endif
	float sunRadiance;

	float mieG;
	float mieHeight;
    float rayleighHeight;

	vec3 waveLambdaMie;
	vec3 waveLambdaOzone;
	vec3 waveLambdaRayleigh;

	float earthRadius;
	float earthAtmTopRadius;
	vec3 earthCenter;
};

void clip(float b)
{
    if (b < 0)
        discard;
}

float GetOpticalDepthSchueler(float h, float H, float earthRadius, float cosZenith)
{
	return H * ChapmanApproximation(earthRadius / H, h / H, cosZenith);
}

vec3 GetTransmittance(ScatteringParams setting, vec3 L, vec3 V)
{
	float ch = GetOpticalDepthSchueler(L.y, setting.rayleighHeight, setting.earthRadius, V.y);
	return exp(-(setting.waveLambdaMie + setting.waveLambdaRayleigh) * ch);
}

vec2 ComputeOpticalDepth(ScatteringParams setting, vec3 samplePoint, vec3 V, vec3 L, float neg)
{
    float rl = length(samplePoint);
    float h = rl - setting.earthRadius;
    vec3 r = samplePoint / rl;

    float cos_chi_sun = dot(r, -L);
    float cos_chi_ray = dot(r, V * neg);

    float opticalDepthSun = GetOpticalDepthSchueler(h, setting.rayleighHeight, setting.earthRadius, cos_chi_sun);
    float opticalDepthCamera = GetOpticalDepthSchueler(h, setting.rayleighHeight, setting.earthRadius, cos_chi_ray) * neg;

    return vec2(opticalDepthSun, opticalDepthCamera);
}

void AerialPerspective(ScatteringParams setting, vec3 start, vec3 end, vec3 V, vec3 L, bool infinite, inout vec3 transmittance, out vec3 insctrMie, out vec3 insctrRayleigh)
{
    float inf_neg = infinite ? 1.0 : -1.0;

    vec3 sampleStep = (end - start) / ATM_SAMPLES_NUMS;
    vec3 samplePoint = end - sampleStep;
    vec3 sampleLambda = setting.waveLambdaMie + setting.waveLambdaRayleigh + setting.waveLambdaOzone;

    float sampleLength = length(sampleStep);

    vec3 scattering = vec3(0.0, 0.0, 0.0);
    vec2 lastOpticalDepth = ComputeOpticalDepth(setting, end, V, L, inf_neg);   

    for (int i = 1; i < ATM_SAMPLES_NUMS; i++, samplePoint -= sampleStep)
	{
		vec2 opticalDepth = ComputeOpticalDepth(setting, samplePoint, V, L, inf_neg);

		vec3 segment_s = exp(-sampleLambda * (opticalDepth.x + lastOpticalDepth.x));
		vec3 segment_t = exp(-sampleLambda * (opticalDepth.y - lastOpticalDepth.y));
		
		transmittance *= segment_t;
		
		scattering = scattering * segment_t;
		scattering += exp(-(length(samplePoint) - setting.earthRadius) / setting.rayleighHeight) * segment_s;

		lastOpticalDepth = opticalDepth;
	}

	insctrMie = scattering * setting.waveLambdaMie * sampleLength;
	insctrRayleigh = scattering * setting.waveLambdaRayleigh * sampleLength;
}

vec2 ComputeRaySphereIntersection2(vec3 position, vec3 dir, vec3 center, float radius)
{
	vec3 origin = position - center;
	float B = dot(origin, dir);
	float C = dot(origin, origin) - radius * radius;
	float D = B * B - C;

	vec2 minimaxIntersections;
	if (D < 0.0)
	{
		minimaxIntersections = vec2(-1.0, -1.0);
	}
	else
	{
		D = sqrt(D);
		minimaxIntersections = vec2(-B - D, -B + D);
	}

	return minimaxIntersections;
}

bool ComputeSkyboxChapman(ScatteringParams setting, vec3 eye, vec3 V, vec3 L, inout vec3 transmittance, out vec3 insctrMie, out vec3 insctrRayleigh)
{
    bool neg = true;

    vec2 outerIntersections = ComputeRaySphereIntersection2(eye, V, setting.earthCenter, setting.earthAtmTopRadius);
    clip(outerIntersections.y);

    vec2 innerIntersections = ComputeRaySphereIntersection2(eye, V, setting.earthCenter, setting.earthRadius);
    if (innerIntersections.x > 0.0) // if forward to the ground (not if under ground)
    {
        neg = false;
        outerIntersections.y = innerIntersections.x;
    }

    // earth center have nagative value (like relative cordinate)
    eye -= setting.earthCenter;

    vec3 start = eye + V * max(0.0, outerIntersections.x);
    vec3 end = eye + V * outerIntersections.y;

	AerialPerspective(setting, start, end, V, L, neg, transmittance, insctrMie, insctrRayleigh);

    // ground case
	bool intersectionTest = innerIntersections.x < 0.0 && innerIntersections.y < 0.0;
	return intersectionTest;
}

vec4 ComputeSkyInscattering(ScatteringParams setting, vec3 eye, vec3 V, vec3 L)
{
    vec3 insctrMie = vec3(0.0);
    vec3 insctrRayleigh = vec3(0.0);
    vec3 insctrOpticalLength = vec3(1.0);
    bool intersectionTest = ComputeSkyboxChapman(setting, eye, V, L, insctrOpticalLength, insctrMie, insctrRayleigh);

	float phaseTheta = dot(V, -L);
	float phaseMie = ComputePhaseMie(phaseTheta, setting.mieG);
	float phaseRayleigh = ComputePhaseRayleigh(phaseTheta);

	vec3 insctrTotalMie = insctrMie * phaseMie;
	vec3 insctrTotalRayleigh = insctrRayleigh * phaseRayleigh;

	vec3 sky = (insctrTotalMie + insctrTotalRayleigh) * setting.sunRadiance;

    return vec4(sky, 1.0);
}

vec3 ComputeWaveLengthMie(vec3 density)
{
	return 2e-5f * density;
}

const float PI = pi;
const float PI_2 = 2*pi;

float pow2(float x)
{
    return x * x;
}

vec3 ComputeWaveLengthMie(vec3 lambda, vec3 K, float T, float U = 4)
{
	float c_pi = (0.6544 * T - 0.6510) * 1e-16 * PI;
	float mieConst = 0.434 * c_pi * pow(PI_2, U - 2.0);
	return mieConst * K / pow(lambda, vec3(U - 2.0));
}

vec3 ComputeWaveLengthRayleigh(vec3 lambda)
{
	const float n = 1.0003;
	const float N = 2.545e25;
	const float pn = 0.035;
	const float n2 = n * n;
	const float pi3 = PI * PI * PI;
	const float rayleighConst = (8.0 * pi3 * pow2(n2 - 1.0)) / (3.0 * N) * ((6.0 + 3.0 * pn) / (6.0 - 7.0 * pn));
	return rayleighConst / (lambda * lambda * lambda * lambda);
}

// ----------------------------------------------------------------------------
void main() 
{
#if TEST_RAY_MMD
    vec3 mieLambda = ComputeCoefficientMie(mWaveLength, mMieColor, mSunTurbidity);
    vec3 rayleight = ComputeWaveLengthRayleigh(mWaveLength) * mRayleighColor;
	vec3 cloud = ComputeCoefficientMie(mWaveLength, mCloudColor, mCloudTurbidity);

    rayleight = vec3(5.80703318e-06, 1.35687360e-05, 3.31267875e-05);
    mieLambda = vec3(2.71618246e-05, 4.10025968e-05, 6.27077752e-05);
    rayleight = betaR0;
    mieLambda = betaM0;

    ScatteringParams setting;
    setting.mieG = g;
    setting.sunRadiance = mSunRadiance;
    setting.earthRadius = mEarthRadius * mUnitDistance;
    setting.earthCenter = vec3(0, -setting.earthRadius, 0);
    setting.earthAtmTopRadius = mEarthAtmoRadius * mUnitDistance;
	setting.waveLambdaMie = mieLambda;
	setting.waveLambdaOzone = mOzoneScatteringCoeff * mOzoneMass;
	setting.waveLambdaRayleigh = rayleight;
	setting.mieHeight = mMieHeight * mUnitDistance;
	setting.rayleighHeight = mRayleighHeight * mUnitDistance;

    vec3 L = -uSunDir;
    vec3 V = normalize(-vNormalW);
    vec3 CameraPos = vec3(0.0, humanHeight + uAltitude, 0.0);
    fragColor = ComputeSkyInscattering(setting, CameraPos, V, L);

#else
    vec3 dir = normalize(-vNormalW);

    vec3 cameraPos = vec3(0.0, humanHeight + uAltitude + uEarthRadius, 0.0);
    vec2 t = ComputeRaySphereIntersection(cameraPos, dir, uEarthCenter, uEarthRadius);
    // handle ray toward ground
    float tmax = inf;
    if (t.y > 0) tmax = max(0.0, t.x);

    vec3 color = computeIncidentLight(cameraPos, dir, uSunIntensity, 0.0, tmax);
    fragColor = vec4(color, 1.0);
#endif
}
