-- Vertex
// IN
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexcoords;

// Out
out vec4 vPositionW;
out vec3 vNormalW;
out vec2 vTexcoords;

uniform mat4 uWorld;
uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
    mat4 worldViewProj = uProjection*uView*uWorld;

    vPositionW = uWorld * vec4(inPosition, 1.0);
    vNormalW = mat3(uWorld) * inNormal;
	vTexcoords = inTexcoords; 
	gl_Position = worldViewProj * vec4(inPosition, 1.0);
}

-- Fragment

// #define USE_SPHERE_INTEGRAL 1
// #define USE_TEXTURE_DIR 1

const float LUT_SIZE = 64.0;
const float LUT_SCALE = (LUT_SIZE - 1.0) / LUT_SIZE;
const float LUT_BIAS = 0.5 / LUT_SIZE;

// bind roughness   {label:"Roughness", default:0.25, min:0.01, max:1, step:0.001}
// bind dcolor      {label:"Diffuse Color",  r:1.0, g:1.0, b:1.0}
// bind intensity   {label:"Light Intensity", default:4, min:0, max:10}
// bind width       {label:"Width",  default: 8, min:0.1, max:15, step:0.1}
// bind height      {label:"Height", default: 8, min:0.1, max:15, step:0.1}
// bind roty        {label:"Rotation Y", default: 0, min:0, max:1, step:0.001}
// bind rotz        {label:"Rotation Z", default: 0, min:0, max:1, step:0.001}
// bind twoSided    {label:"Two-sided", default:false}

// IN
in vec4 vPositionW;
in vec3 vNormalW;
in vec2 vTexcoords;

// OUT
out vec3 FragColor;

uniform vec4 uQuadPoints[4]; // Area light quad
uniform vec4 uStarPoints[10]; // Area light star
uniform vec3 uViewPositionW;

uniform float uF0; // frenel
uniform vec4 uAlbedo2; // additional albedo
uniform float uIntensity;
uniform float uWidth;
uniform float uHeight;
uniform float uRotY;
uniform float uRotZ;
uniform bool ubTwoSided;
uniform bool ubClipless;
uniform bool ubTexturedLight;
uniform bool ubDebug;

uniform sampler2D uLtc1;
uniform sampler2D uLtc2;
uniform sampler2DArray uFilteredMap;
uniform sampler2D uAlbedo;
uniform sampler2D uNormal;
uniform sampler2D uRoughness;
uniform sampler2D uMetalness;

uniform mat4 uView;
uniform vec2 uResolution;
uniform int uSampleCount;

// Tracing and intersection
///////////////////////////

struct Ray
{
	vec3 origin;
	vec3 dir;
};

struct Rect
{
	vec3 center;
	vec3 dirx;
	vec3 diry;

	float halfx;
	float halfy;

	vec4 plane;
};


bool RayPlaneIntersect(Ray ray, vec4 plane, out float t)
{
	t = -dot(plane, vec4(ray.origin, 1.0))/dot(plane.xyz, ray.dir);
	return t > 0.0;
}

bool RayRectIntersect(Ray ray, Rect rect, out float t)
{
	bool intersect = RayPlaneIntersect(ray, rect.plane, t);
	if (intersect)
	{
		vec3 pos = ray.origin + ray.dir*t;
		vec3 lpos = pos - rect.center;

		float x = dot(lpos, rect.dirx);
		float y = dot(lpos, rect.diry);

		if (abs(x) > rect.halfx || abs(y) > rect.halfy)
			intersect = false;
	}
	return intersect;
}

vec2 RectUVs(vec3 pos, Rect rect)
{
    vec3 lpos = pos - rect.center;

    float x = dot(lpos, rect.dirx);
    float y = dot(lpos, rect.diry);

    return vec2(
        0.5*x/rect.halfx + 0.5,
        0.5*y/rect.halfy + 0.5);
}

// Camera functions
///////////////////

Ray GenerateCameraRay(vec3 position, vec3 target)
{
	Ray ray;

	// Random jitter within pixel for AA
	ray.origin = position;
	ray.dir = normalize(target - position);

	return ray;
}

vec3 rotation_y(vec3 v, float a)
{
    vec3 r;
    r.x =  v.x*cos(a) + v.z*sin(a);
    r.y =  v.y;
    r.z = -v.x*sin(a) + v.z*cos(a);
    return r;
}

vec3 rotation_z(vec3 v, float a)
{
    vec3 r;
    r.x =  v.x*cos(a) - v.y*sin(a);
    r.y =  v.x*sin(a) + v.y*cos(a);
    r.z =  v.z;
    return r;
}

vec3 rotation_yz(vec3 v, float ay, float az)
{
    return rotation_z(rotation_y(v, ay), az);
}

// Linearly Transformed Cosines
///////////////////////////////

// Real-Time Area Lighting: a Journey from Research to Production
vec3 IntegrateEdgeVec(vec3 v1, vec3 v2)
{
    float x = dot(v1, v2);
    float y = abs(x);

    float a = 0.8543985 + (0.4965155 + 0.0145206*y)*y;
    float b = 3.4175940 + (4.1616724 + y)*y;
    float v = a / b;

    float theta_sintheta = (x > 0.0) ? v : 0.5*inversesqrt(max(1.0 - x*x, 1e-7)) - v;

    return cross(v1, v2)*theta_sintheta;
}

float IntegrateEdge(vec3 v1, vec3 v2)
{
    return IntegrateEdgeVec(v1, v2).z;
}

void ClipQuadToHorizon(inout vec3 L[5], out int n)
{
    // detect clipping config
    int config = 0;
    if (L[0].z > 0.0) config += 1;
    if (L[1].z > 0.0) config += 2;
    if (L[2].z > 0.0) config += 4;
    if (L[3].z > 0.0) config += 8;

    // clip
    n = 0;

    if (config == 0)
    {
        // clip all
    }
    else if (config == 1) // V1 clip V2 V3 V4
    {
        n = 3;
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
        L[2] = -L[3].z * L[0] + L[0].z * L[3];
    }
    else if (config == 2) // V2 clip V1 V3 V4
    {
        n = 3;
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
    }
    else if (config == 3) // V1 V2 clip V3 V4
    {
        n = 4;
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
        L[3] = -L[3].z * L[0] + L[0].z * L[3];
    }
    else if (config == 4) // V3 clip V1 V2 V4
    {
        n = 3;
        L[0] = -L[3].z * L[2] + L[2].z * L[3];
        L[1] = -L[1].z * L[2] + L[2].z * L[1];
    }
    else if (config == 5) // V1 V3 clip V2 V4) impossible
    {
        n = 0;
    }
    else if (config == 6) // V2 V3 clip V1 V4
    {
        n = 4;
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
        L[3] = -L[3].z * L[2] + L[2].z * L[3];
    }
    else if (config == 7) // V1 V2 V3 clip V4
    {
        n = 5;
        L[4] = -L[3].z * L[0] + L[0].z * L[3];
        L[3] = -L[3].z * L[2] + L[2].z * L[3];
    }
    else if (config == 8) // V4 clip V1 V2 V3
    {
        n = 3;
        L[0] = -L[0].z * L[3] + L[3].z * L[0];
        L[1] = -L[2].z * L[3] + L[3].z * L[2];
        L[2] =  L[3];
    }
    else if (config == 9) // V1 V4 clip V2 V3
    {
        n = 4;
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
        L[2] = -L[2].z * L[3] + L[3].z * L[2];
    }
    else if (config == 10) // V2 V4 clip V1 V3) impossible
    {
        n = 0;
    }
    else if (config == 11) // V1 V2 V4 clip V3
    {
        n = 5;
        L[4] = L[3];
        L[3] = -L[2].z * L[3] + L[3].z * L[2];
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
    }
    else if (config == 12) // V3 V4 clip V1 V2
    {
        n = 4;
        L[1] = -L[1].z * L[2] + L[2].z * L[1];
        L[0] = -L[0].z * L[3] + L[3].z * L[0];
    }
    else if (config == 13) // V1 V3 V4 clip V2
    {
        n = 5;
        L[4] = L[3];
        L[3] = L[2];
        L[2] = -L[1].z * L[2] + L[2].z * L[1];
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
    }
    else if (config == 14) // V2 V3 V4 clip V1
    {
        n = 5;
        L[4] = -L[0].z * L[3] + L[3].z * L[0];
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
    }
    else if (config == 15) // V1 V2 V3 V4
    {
        n = 4;
    }
    
    if (n == 3)
        L[3] = L[0];
    if (n == 4)
        L[4] = L[0];
}

vec3 mul(mat3 m, vec3 v)
{
    return m * v;
}

mat3 mul(mat3 m1, mat3 m2)
{
    return m1 * m2;
}

vec3 FetchColorTexture(vec2 uv, float lod)
{
    if (!ubTexturedLight)
        return vec3(1, 1, 1);
    return texture(uFilteredMap, vec3(uv, lod)).rgb;
}

// Use code in 'LTC demo sample'
vec3 FetchDiffuseFilteredTexture(vec3 p1, vec3 p2, vec3 p3, vec3 p4)
{
    if (ubTexturedLight == false)
        return vec3(1, 1, 1);
	
    // area light plane basis
    vec3 V1 = p2 - p1;
    vec3 V2 = p4 - p1;
    vec3 planeOrtho = cross(V1, V2);
    float planeAreaSquared = dot(planeOrtho, planeOrtho);
    float planeDistxPlaneArea = dot(planeOrtho, p1);
    // orthonormal projection of (0,0,0) in area light space
    vec3 P = planeDistxPlaneArea * planeOrtho / planeAreaSquared - p1;

    // find tex coords of P
    float dot_V1_V2 = dot(V1, V2);
    float inv_dot_V1_V1 = 1.0 / dot(V1, V1);
    vec3 V2_ = V2 - V1 * dot_V1_V2 * inv_dot_V1_V1;
    vec2 Puv;
    Puv.y = dot(V2_, P) / dot(V2_, V2_);
    Puv.x = dot(V1, P)*inv_dot_V1_V1 - dot_V1_V2*inv_dot_V1_V1*Puv.y;

    // LOD
    float d = abs(planeDistxPlaneArea) / pow(planeAreaSquared, 0.75);
    
    // Flip texture to match OpenGL conventions
    Puv = Puv*vec2(1, -1) + vec2(0, 1);
    
    // in source file(prefilterAreaLight.cpp)
    // const float dist = powf(3.0f, level) / powf(2.0f, Nlevels - 1.0f);
    float lod = log(2048.0*d)/log(3.0);
    lod = min(lod, 7.0);
    
    float lodA = floor(lod);
    float lodB = ceil(lod);
    float t = lod - lodA;
    
    vec3 a = FetchColorTexture(Puv, lodA);
    vec3 b = FetchColorTexture(Puv, lodB);

    return mix(a, b, t);
}

vec3 FetchDiffuseFilteredTexture(vec3 p1, vec3 p2, vec3 p3, vec3 p4, vec3 dir)
{
    if (ubTexturedLight == false)
        return vec3(1, 1, 1);
    
    // area light plane basis
    vec3 V1 = p2 - p1;
    vec3 V2 = p4 - p1;
    vec3 planeOrtho = cross(V1, V2);
    float planeAreaSquared = dot(planeOrtho, planeOrtho);

    Ray ray;
    ray.origin = vec3(0, 0, 0);
    ray.dir = dir;
    vec4 plane = vec4(planeOrtho, -dot(planeOrtho, p1));
    float planeDist;
    RayPlaneIntersect(ray, plane, planeDist);
 
    vec3 P = planeDist*ray.dir - p1;
 
    // find tex coords of P
    float dot_V1_V2 = dot(V1, V2);
    float inv_dot_V1_V1 = 1.0 / dot(V1, V1);
    vec3 V2_ = V2 - V1 * dot_V1_V2 * inv_dot_V1_V1;
    vec2 Puv;
    Puv.y = dot(V2_, P) / dot(V2_, V2_);
    Puv.x = dot(V1, P)*inv_dot_V1_V1 - dot_V1_V2*inv_dot_V1_V1*Puv.y;

    // LOD
    float d = abs(planeDist) / pow(planeAreaSquared, 0.25);
    
    // Flip texture to match OpenGL conventions
    Puv = Puv*vec2(1, -1) + vec2(0, 1);
    
    float lod = log(2048.0*d)/log(3.0);
    lod = min(lod, 7.0);
    
    float lodA = floor(lod);
    float lodB = ceil(lod);
    float t = lod - lodA;
    
    vec3 a = FetchColorTexture(Puv, lodA);
    vec3 b = FetchColorTexture(Puv, lodB);

    return mix(a, b, t);
}

// Use code in 'LTC webgl sample'
vec3 LTC_Evaluate(vec3 N, vec3 V, vec3 P, mat3 Minv, vec4 points[4], bool twoSided)
{
    // construct orthonormal basis around N
    vec3 T1, T2;
    T1 = normalize(V - N*dot(V, N));
    T2 = cross(N, T1);

    // rotate area light in (T1, T2, N) basis
    Minv = mul(Minv, transpose(mat3(T1, T2, N)));
    
    mat3 MM = mat3(1);

    // polygon (allocate 5 vertices for clipping)
    vec3 L[5];
    L[0] = mul(Minv, points[0].xyz - P);
    L[1] = mul(Minv, points[1].xyz - P);
    L[2] = mul(Minv, points[2].xyz - P);
    L[3] = mul(Minv, points[3].xyz - P);
    L[4] = L[3]; // avoid warning

    vec3 LL[4];
    LL[0] = L[0];
    LL[1] = L[1];
    LL[2] = L[2];
    LL[3] = L[3];

    // integrate
    float sum = 0.0;
    vec3 colorMap = vec3(1);

    if (ubClipless)
    {
        vec3 dir = points[0].xyz - P;
        vec3 lightNormal = cross(points[1].xyz - points[0].xyz, points[3].xyz - points[0].xyz);
        bool behind = (dot(dir, lightNormal) < 0.0);

        L[0] = normalize(L[0]);
        L[1] = normalize(L[1]);
        L[2] = normalize(L[2]);
        L[3] = normalize(L[3]);

        vec3 vsum = vec3(0.0);

        vsum += IntegrateEdgeVec(L[0], L[1]);
        vsum += IntegrateEdgeVec(L[1], L[2]);
        vsum += IntegrateEdgeVec(L[2], L[3]);
        vsum += IntegrateEdgeVec(L[3], L[0]);

        float len = length(vsum);
    #ifndef USE_SPHERE_INTEGRAL
        float z = vsum.z/len;

        if (behind)
            z = -z;

        vec2 uv = vec2(z*0.5 + 0.5, len);
        // if mtx data is loaded from image need to be flip
        uv.y = 1 - uv.y;
        uv = uv*LUT_SCALE + LUT_BIAS;

        float scale = texture(uLtc2, uv).w;
        sum = len*scale;
    #else
        // SphereIntegral
        sum = max((len*len + vsum.z)/(len + 1), 0);
    #endif
        if (behind && !twoSided)
            sum = 0.0;

        vec3 fetchDir = vsum/len;
    #ifdef USE_TEXTURE_DIR
        colorMap = FetchDiffuseFilteredTexture(LL[0], LL[1], LL[2], LL[3], fetchDir);
    #else
        colorMap = FetchDiffuseFilteredTexture(LL[0], LL[1], LL[2], LL[3]);
    #endif
    }
    else
    {
        int n;
        ClipQuadToHorizon(L, n);

        if (n == 0)
            return vec3(0, 0, 0);

        // project onto sphere
        L[0] = normalize(L[0]);
        L[1] = normalize(L[1]);
        L[2] = normalize(L[2]);
        L[3] = normalize(L[3]);
        L[4] = normalize(L[4]);
        
        vec3 vsum;

        // integrate
        vsum  = IntegrateEdgeVec(L[0], L[1]);
        vsum += IntegrateEdgeVec(L[1], L[2]);
        vsum += IntegrateEdgeVec(L[2], L[3]);
        if (n >= 4)
            vsum += IntegrateEdgeVec(L[3], L[4]);
        if (n == 5)
            vsum += IntegrateEdgeVec(L[4], L[0]);

        sum = twoSided ? abs(vsum.z) : max(0.0, vsum.z);

        vec3 fetchDir = normalize(vsum);
    #ifdef USE_TEXTURE_DIR
        colorMap = FetchDiffuseFilteredTexture(LL[0], LL[1], LL[2], LL[3], fetchDir);
    #else
        colorMap = FetchDiffuseFilteredTexture(LL[0], LL[1], LL[2], LL[3]);
    #endif
    }

    // scale by filtered light color
    return sum * colorMap;
}

// Scene helpers
////////////////

void InitRect(out Rect rect, in vec4 points[4])
{
    vec3 right = vec3(points[3] - points[0]);
    vec3 up = vec3(points[0] - points[1]);
    rect.dirx = normalize(right);
    rect.diry = normalize(up);
	rect.center = vec3((points[0] + points[2]))*0.5;
	rect.halfx = 0.5*length(right);
	rect.halfy = 0.5*length(up);

	vec3 rectNormal = cross(rect.dirx, rect.diry);
	rect.plane = vec4(rectNormal, -dot(rectNormal, rect.center));
}

vec3 toLinear(vec3 _rgb)
{
	return pow(abs(_rgb), vec3(2.2));
}

mat3 calcTbn(vec3 _normal, vec3 _worldPos, vec2 _texCoords)
{
    vec3 Q1  = dFdx(_worldPos);
    vec3 Q2  = dFdy(_worldPos);
    vec2 st1 = dFdx(_texCoords);
    vec2 st2 = dFdy(_texCoords);

    vec3 N  = _normal;
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    return mat3(T, B, N);
}

void main()
{
    const float minRoughness = 0.03;
    float metallic = texture(uMetalness, vTexcoords).x;
    float roughness = texture(uRoughness, vTexcoords).x;
    roughness = max(roughness*roughness, minRoughness);
	vec3 lcol = vec3(uIntensity);
    vec3 albedo = toLinear(vec3(uAlbedo2));
    vec3 baseColor = toLinear(texture(uAlbedo, vTexcoords).xyz);
    vec3 dcol = baseColor*(1.0 - metallic);
    vec3 scol = mix(vec3(uF0), baseColor, metallic);

	vec3 normal = normalize(vec3(vNormalW));
	mat3 tbn = calcTbn(normal, vPositionW.xyz, vTexcoords);
	vec3 tangentNormal = texture(uNormal, vTexcoords).xyz * 2.0 - 1.0;
	normal = normalize(tbn * tangentNormal);

	Ray ray = GenerateCameraRay(uViewPositionW, vPositionW.xyz);

	vec3 col = vec3(0);
    vec3 pos = vPositionW.xyz;
    vec3 V = -ray.dir;
    vec3 N = normal;

    float ndotv = clamp(dot(N, V), 0, 1);
    vec2 uv = vec2(roughness, sqrt(1.0 - ndotv));
    // if mtx data is loaded from image need to be flip
    uv.y = 1 - uv.y;
    // scale and bias coordinates, for correct filtered lookup
    uv = uv*LUT_SCALE + LUT_BIAS;

    vec4 t1 = texture(uLtc1, uv);
    vec4 t2 = texture(uLtc2, uv);
    mat3 Minv = mat3(
        vec3(t1.x, 0, t1.y),
        vec3(   0, 1, 0),
        vec3(t1.z, 0, t1.w)
    );

    vec3 spec = LTC_Evaluate(N, V, pos, Minv, uQuadPoints, ubTwoSided);

    // apply BRDF scale terms (BRDF magnitude and Schlick Fresnel)
    spec *= scol*t2.x + (1.0 - scol)*t2.y;

    vec3 diff = LTC_Evaluate(N, V, pos, mat3(1), uQuadPoints, ubTwoSided);

    col = lcol*(spec + dcol*diff*albedo);

	FragColor = col;
}
