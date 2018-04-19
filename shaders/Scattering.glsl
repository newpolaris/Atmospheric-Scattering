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
uniform vec2 uInvResolution;

// OUT
out vec3 fragColor;

// h is height above the sea level
vec3 RayleighCoeff(float h)
{
	float hR = 8000; // 8 km
	vec3 rC = vec3(33.1, 13.5, 5.8); // r, g, b term
	vec3 rgbTerm = rC * 1e-3;
	return rgbTerm * exp(-(h/hR));
}

vec3 OpticalDepth(vec3 pa, vec3 pb, float N)
{
	vec3 ds = (pb - pa) / N;
	vec3 coeff = vec3(0, 0, 0);
	for (int i = 0; i < N; i++)
	{
		vec3 x = pa + ds * (i + 0.5);
		coeff += RayleighCoeff(x.y);
	}
	return coeff;
}

vec2 RaySphereIntersect(vec3 o, vec3 dir, vec3 so, float sr)
{
    vec3 tc = so - o;
    float l = dot(tc, dir);
    float d = l*l - dot(tc, tc) + sr*sr;
    if (d < 0)
        return vec2(-1.0, -1.0);
    float delta = sqrt(d);
    return vec2(l - delta, l + delta);
}

vec3 Transmittance(vec3 Pa, vec3 Pb)
{
    return exp(-OpticalDepth(Pa, Pb, 15));
}

vec3 Extinction(float h)
{
    vec3 RayleighCoeff = vec3(33.1, 13.5, 5.8)*1e-6;
    return RayleighCoeff.bgr * exp(-h/8000);
}

// ----------------------------------------------------------------------------
void main() 
{
	vec2 xz = 2.0*(gl_FragCoord.xy)*uInvResolution - vec2(1.0);
    float d = 1 - dot(xz, xz);
    if (d < 0.0) discard;
	float y = sqrt(d);

	vec3 dir = normalize(vec3(xz, y).xzy);
	vec3 pa = vec3(0, 0, 0);
	float Eb = 6420e3; // earth atmosphere radius in km
	float Ea = 6360e3; // earth radius in km

	// atmosphere start to end point
	vec3 pb = pa + dir * (Eb - Ea);

	vec3 depth = OpticalDepth(pa, pb, 15);
    vec2 lc = RaySphereIntersect(vec3(0, 0, 0), dir, vec3(0, 0, 0), Ea);

    vec3 Pc = vec3(0, 0, 0); // camera position
    vec3 Ec = vec3(0, 0, 0); // earth center
    vec3 SunIntensity = vec3(20, 20, 20);

    vec2 ipv = RaySphereIntersect(Pc, dir, Ec, Eb);
    vec3 Pa = Pc + ipv.g*dir;
    int NSliceV = 20;
    float delta = ipv.g/NSliceV;
    vec3 Cs = vec3(0, 0, 0);
    vec3 L = normalize(vec3(0, 1, 0));
    for (int i = 0; i < NSliceV; i++)
    {
        vec3 X = Pc + delta * (i + 0.5) * dir;
        vec3 TrV = Transmittance(Pc, X);
        vec2 ips = RaySphereIntersect(X, L, Ec, Eb);
        vec3 Ps = X + L*ips.g;
        vec3 TrS = Transmittance(X, Ps);
        vec3 BetaS = Extinction(X.y);
        Cs += TrV * TrS * BetaS;
    }

	fragColor = SunIntensity * Cs;
}
