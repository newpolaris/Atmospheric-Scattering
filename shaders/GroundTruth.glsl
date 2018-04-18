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

// bind roughness   {label:"Roughness", default:0.25, min:0.01, max:1, step:0.001}
// bind dcolor      {label:"Diffuse Color",  r:1.0, g:1.0, b:1.0}
// bind intensity   {label:"Light Intensity", default:4, min:0, max:10}
// bind width       {label:"Width",  default: 8, min:0.1, max:15, step:0.1}
// bind height      {label:"Height", default: 8, min:0.1, max:15, step:0.1}
// bind twoSided    {label:"Two-sided", default:false}

bool bDiffuseLight = true;
bool bDiffuseBRDF = true;
bool bSpecLight = true;
bool bSpecBRDF = true;

// IN
in vec4 vPositionW;
in vec3 vNormalW;
in vec2 vTexcoords;

// OUT
out vec3 FragColor;

const int NumSamples = 4;
const float pi = 3.14159265;

uniform vec4 uQuadPoints[4]; // Area light quad
uniform vec4 uStarPoints[10]; // Area light star
uniform vec4 uSamples[NumSamples];
uniform vec3 uViewPositionW;

uniform float uF0; // frenel
uniform vec4 uAlbedo2; // additional albedo
uniform float uIntensity;
uniform float uWidth;
uniform float uHeight;
uniform float uRotY;
uniform float uRotZ;
uniform bool ubTwoSided;
uniform bool uTexturedLight;
uniform int uSampleCount;

uniform sampler2D uAlbedo;
uniform sampler2D uNormal;
uniform sampler2D uRoughness;
uniform sampler2D uMetalness;
uniform sampler2D uTexColor;

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

vec3 mul(mat3 m, vec3 v)
{
    return m * v;
}

mat3 mul(mat3 m1, mat3 m2)
{
    return m1 * m2;
}

vec3 toLinear(vec3 _rgb)
{
	return pow(abs(_rgb), vec3(2.2));
}

// "Building an Orthonormal Basis from a 3D Unit Vector Without Normalization"
mat3 BasisFrisvad(vec3 n)
{
    vec3 b1, b2;
    if (n.z < -0.999999) // Handle the sigularity
    {
        b1 = vec3(0.0, -1.0, 0.0);
        b2 = vec3(-1.0, 0.0, 0.0);
    }
    else
    {
        float a = 1.0 / (1.0 + n.z);
        float b = -n.x*n.y*a;
        b1 = vec3(1.0 - n.x*n.x*a, b, -n.x);
        b2 = vec3(b, 1.0 - n.y*n.y*a, -n.y);
    }
    return mat3(b1, b2, n);
}

struct SphQuad 
{
    vec3 o, x, y, z; // local reference system 'R'
    float z0, z0sq; 
    float x0, y0, y0sq; // rectangle coords in 'R' 
    float x1, y1, y1sq; 
    float b0, b1, b0sq, k; // misc precomputed constants 
    float S; // solid angle of 'Q' 
};

//
// "An Area-Preserving Parametrization for Spherical Rectangles"
//
// s: one of its vertice on 3D planar rectangle P
// ex, ey: two perpendicular vectors
// o: center of unit radius sphere
//
SphQuad SphQuadInit(vec3 s, vec3 ex, vec3 ey, vec3 o) 
{
    SphQuad squad;

    squad.o = o;
    float exl = length(ex), eyl = length(ey); 

    // compute local reference system 'R' 
    squad.x = ex / exl;
    squad.y = ey / eyl;
    squad.z = cross(squad.x, squad.y);

    // compute rectangle coords in local reference system 
    vec3 d = s - o;
    squad.z0 = dot(d, squad.z); 

    // flip 'z' to make it point against 'Q' 
    if (squad.z0 > 0) 
    {
        squad.z *= -1; 
        squad.z0 *= -1; 
    } 
    squad.z0sq = squad.z0 * squad.z0;
    squad.x0 = dot(d, squad.x);
    squad.y0 = dot(d, squad.y);
    squad.x1 = squad.x0 + exl;
    squad.y1 = squad.y0 + eyl;
    squad.y0sq = squad.y0 * squad.y0;
    squad.y1sq = squad.y1 * squad.y1; 
    
    // create vectors to four vertices 
    vec3 v00 = vec3(squad.x0, squad.y0, squad.z0);
    vec3 v01 = vec3(squad.x0, squad.y1, squad.z0);
    vec3 v10 = vec3(squad.x1, squad.y0, squad.z0);
    vec3 v11 = vec3(squad.x1, squad.y1, squad.z0); 

    // compute normals to edges 
    vec3 n0 = normalize(cross(v00, v10));
    vec3 n1 = normalize(cross(v10, v11));
    vec3 n2 = normalize(cross(v11, v01));
    vec3 n3 = normalize(cross(v01, v00)); 
    
    // compute internal angles (gamma_i) 
    float g0 = acos(-dot(n0,n1));
    float g1 = acos(-dot(n1,n2));
    float g2 = acos(-dot(n2,n3));
    float g3 = acos(-dot(n3,n0)); 
    
    // compute predefined constants 
    squad.b0 = n0.z;
    squad.b1 = n2.z;
    squad.b0sq = squad.b0 * squad.b0;
    squad.k = 2*pi - g2 - g3; 
    
    // compute solid angle from internal angles 
    squad.S = g0 + g1 - squad.k; 

    return squad;
}

vec3 SphQuadSample(SphQuad squad, float u, float v) 
{
    // 1. compute 'cu' 
    float au = u * squad.S + squad.k;
    float fu = (cos(au) * squad.b0 - squad.b1) / sin(au);
    float cu = 1/sqrt(fu*fu + squad.b0sq) * (fu>0 ? +1 : -1);
    cu = clamp(cu, -1, 1); // avoid NaNs 
    // 2. compute 'xu' 
    float xu = -(cu * squad.z0) / sqrt(1 - cu*cu);
    xu = clamp(xu, squad.x0, squad.x1); // avoid Infs 
    // 3. compute 'yv' 
    float d = sqrt(xu*xu + squad.z0sq);
    float h0 = squad.y0 / sqrt(d*d + squad.y0sq);
    float h1 = squad.y1 / sqrt(d*d + squad.y1sq);
    float hv = h0 + v * (h1-h0), hv2 = hv*hv;
    float yv = (hv2 < 1 - 1e-6) ? (hv*d)/sqrt(1-hv2) : squad.y1;
    // 4. transform (xu,yv,z0) to world coords 
    return (squad.o + xu*squad.x + yv*squad.y + squad.z0*squad.z); 
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

bool QuadRayTest(vec4 q[4], vec3 pos, vec3 dir, out vec2 uv, bool twoSided)
{
    // compute plane normal and distance from origin
    // note that in right hand coordinates, zaxis is toward plane backward
    vec3 xaxis = q[1].xyz - q[0].xyz;
    vec3 yaxis = q[3].xyz - q[0].xyz;

    float xlen = length(xaxis);
    float ylen = length(yaxis);
    xaxis = xaxis / xlen;
    yaxis = yaxis / ylen;

    vec3 zaxis = normalize(cross(xaxis, yaxis));

    float d = dot(zaxis, q[0].xyz);

    // zaxis faces backwards in the plane
    float ndotz = dot(dir, zaxis);
    if (twoSided)
        ndotz = abs(ndotz);

    if (ndotz < 0.00001)
        return false;

    // compute intersection point
    float t = (-dot(pos, zaxis) + d) / dot(dir, zaxis);

    if (t < 0.0)
        return false;

    vec3 projpt = pos + dir * t;

    // use intersection point to determine the UV
    uv = vec2(dot(xaxis, projpt - q[0].xyz),
              dot(yaxis, projpt - q[0].xyz)) / vec2(xlen, ylen);

    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)
        return false;

    // swap y in right hand coordinate
    uv.y = 1 - uv.y;

    return true;
}

float GGX(vec3 V, vec3 L, float alpha, out float pdf)
{
    if (V.z <= 0.0 || L.z <= 0.0)
    {
        pdf = 0.0;
        return 0.0;
    }

    float a2 = alpha*alpha;

    // height-correlated Smith masking-shadowing function
    float G1_wi = 2.0*V.z/(V.z + sqrt(a2 + (1.0 - a2)*V.z*V.z));
    float G1_wo = 2.0*L.z/(L.z + sqrt(a2 + (1.0 - a2)*L.z*L.z));
    float G     = G1_wi*G1_wo / (G1_wi + G1_wo - G1_wi*G1_wo);

    // D
    vec3 H = normalize(V + L);
    float d = 1.0 + (a2 - 1.0)*H.z*H.z;
    float D = a2/(pi* d*d);

    float ndoth = H.z;
    float vdoth = dot(V, H);

    if (vdoth <= 0.0)
    {
        pdf = 0.0;
        return 0.0;
    }

    pdf = D * ndoth / (4.0*vdoth);

    float res = D * G / 4.0 / V.z / L.z;
    return res;
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
    const float pi = 3.14159265;
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

    mat3 t2w = BasisFrisvad(normal);
    mat3 w2t = transpose(t2w);
    vec3 position = vPositionW.xyz;
	vec3 toEye = normalize(uViewPositionW - vPositionW.xyz);

    // express receiver dir in tangent space
    vec3 o = mul(w2t, toEye);

    // uQuadPoints[4]: {{ -1.f, 0.f, -1.f, 1.f }, { +1.f, 0.f, -1.f, 1.f }, { +1.f, 0.f, +1.f, 1.f }, { -1.f, 0.f, +1.f, 1.f }}
    // note that in right hand ez is toward invese normal direction
    vec3 ex = uQuadPoints[1].xyz - uQuadPoints[0].xyz;
    vec3 ey = uQuadPoints[3].xyz - uQuadPoints[0].xyz;
    vec2 uvScale = vec2(length(ex), length(ey));

    SphQuad squad = SphQuadInit(uQuadPoints[0].xyz, ex, ey, position);

    float rcpSolidAngle = 1.0/squad.S;
    float alpha = roughness*roughness;

    // since ey is downward,  reverses the direction of ez
    vec3 quadn = -normalize(cross(ex, ey));
    quadn = mul(w2t, quadn);

    vec2 jitter = FAST_32_hash(gl_FragCoord.xy).xy;

    // integrate
    vec3 Lo_d = vec3(0, 0, 0);
    vec3 Lo_s = vec3(0, 0, 0);

    for (int t = 0; t < NumSamples; t++)
    {
        float u1 = fract(jitter.x + uSamples[t].x);
        float u2 = fract(jitter.y + uSamples[t].y);

        // light sample
        vec3 lightPos = SphQuadSample(squad, u1, u2);

        vec3 i = normalize(lightPos - position);
        i = mul(w2t, i);

        // diffuse light sample
        if (bDiffuseLight)
        {
            float cos_theta_i = i.z;

            // Derive UVs from sample point
            vec3 pd = lightPos - uQuadPoints[0].xyz;
            vec2 uv = vec2(dot(pd, squad.x), dot(pd, squad.y)) / uvScale;
            // invert in OpenGL
            uv.y = 1.0 - uv.y;

            vec3 color = textureLod(uTexColor, uv, 0.0).rgb;

            float pdfBRDF = 1.0/(2.0*pi);
            vec3 fr_p = color/pi;

            float pdfLight = rcpSolidAngle;

            if (cos_theta_i > 0.0 && (dot(i, quadn) < 0.0 || ubTwoSided))
                Lo_d += fr_p*cos_theta_i/(pdfBRDF + pdfLight);
        }
        // specular light sample
        if (bSpecLight)
        {
            // Derive UVs from sample point
            vec3 pd = lightPos - uQuadPoints[0].xyz;
            vec2 uv = vec2(dot(pd, squad.x), dot(pd, squad.y)) / uvScale;
            // invert in OpenGL
            uv.y = 1.0 - uv.y;

            // derive UVs from sample point
            vec3 h = normalize(i + o);

            vec3 F = scol + (1.0 - scol)*pow(1.0 - clamp(dot(h, o), 0, 1), 5.0);
            vec3 color = textureLod(uTexColor, uv, 0.0).rgb;

            float pdfBRDF;
            vec3 fr_p = GGX(o, i, alpha, pdfBRDF)*F*color;

            float pdfLight = rcpSolidAngle;

            float cos_theta_i = i.z;

            if (cos_theta_i > 0.0 && (dot(i, quadn) < 0.0 || ubTwoSided))
                Lo_s += fr_p*cos_theta_i/(pdfBRDF + pdfLight);
        }

        // BRDF sample
        float phi = 2.0*pi*u1;
        float cp = cos(phi);
        float sp = sin(phi);

        // diffuse BRDF sample
        if (bDiffuseBRDF)
        {
            float r = sqrt(u2);
            vec3 i = vec3(r*cp, r*sp, sqrt(1.0 - r*r));

            float cos_theta_i = i.z;

            vec2 uv = vec2(0, 0);
            bool hit = QuadRayTest(uQuadPoints, position, mul(t2w, i), uv, ubTwoSided);
            vec3 color = textureLod(uTexColor, uv, 0.0).rgb;
            color = hit ? color : vec3(0, 0, 0);

            float pdfBRDF = cos_theta_i / pi;
            vec3 fr_p = color/pi;

            float pdfLight = hit ? rcpSolidAngle : 0.0;

            if (cos_theta_i > 0.0 && pdfBRDF > 0.0)
                Lo_d += fr_p*cos_theta_i/(pdfBRDF + pdfLight);
        }
        // Specular BRDF sample
        if (bSpecBRDF)
        {
            float r = sqrt(u2/(1.0 - u2));
            vec3 h = vec3(r*alpha*cp, r*alpha*sp, 1.0);
            h = normalize(h);

            // o is normalized and transformed toEye vector
            vec3 i = reflect(-o, h);

            vec2 uv = vec2(0, 0);
            bool hit = QuadRayTest(uQuadPoints, position, mul(t2w, i), uv, ubTwoSided);
            vec3 F = scol + (1.0 - scol)*pow(1.0 - clamp(dot(h, o), 0, 1), 5.0);

            vec3 color = textureLod(uTexColor, uv, 0.0).rgb;
            color = hit ? color : vec3(0, 0, 0);

            float pdfBRDF;
            vec3 fr_p = GGX(o, i, alpha, pdfBRDF)*F*color;

            float pdfLight = hit ? rcpSolidAngle : 0.0;

            float cos_theta_i = i.z;

            if (cos_theta_i > 0.0 && pdfBRDF > 0.0)
                Lo_s += fr_p*cos_theta_i/(pdfBRDF + pdfLight);
        }
    }
    // scale by diffuse albedo
    Lo_d *= dcol*albedo;

    vec3 Lo_i = Lo_d + Lo_s;

    // scale by light intensity
    Lo_i *= lcol;

    // normalize
    Lo_i /= float(NumSamples);

    FragColor = Lo_i;
}
