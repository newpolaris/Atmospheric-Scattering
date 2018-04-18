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
		vec3 x = pa + ds * i;
		coeff += RayleighCoeff(x.y);
	}
	return exp(-coeff);
}

// ----------------------------------------------------------------------------
void main() 
{
	vec2 xz = 2.0*(gl_FragCoord.xy)*uInvResolution - vec2(1.0);
	float y = sqrt(1 - dot(xz, xz));

	vec3 dir = normalize(vec3(xz, y).xzy);
	vec3 pa = vec3(0, 0, 0);
	float Eb = 6420; // earth atmosphere radius in km
	float Ea = 6360; // earth radius in km

	// atmosphere start to end point
	vec3 pb = pa + dir * (Eb - Ea) * 1000;

	vec3 depth = OpticalDepth(pa, pb, 15);

	fragColor = depth;
}
