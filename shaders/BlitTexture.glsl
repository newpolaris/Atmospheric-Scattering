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
uniform sampler2D uTexSource;

// OUT
out vec3 fragColor;

mat3 mat3_from_rows(vec3 c0, vec3 c1, vec3 c2)
{
    mat3 m = mat3(c0, c1, c2);
    m = transpose(m);

    return m;
}

vec3 mul(mat3 m, vec3 v)
{
    return m * v;
}

mat3 mul(mat3 m1, mat3 m2)
{
    return m1 * m2;
}

vec3 rrt_odt_fit(vec3 v)
{
    vec3 a = v*(         v + 0.0245786) - 0.000090537;
    vec3 b = v*(0.983729*v + 0.4329510) + 0.238081;
    return a/b;
}

vec3 aces_fitted(vec3 color)
{
	mat3 ACES_INPUT_MAT = mat3_from_rows(
	    vec3( 0.59719, 0.35458, 0.04823),
	    vec3( 0.07600, 0.90834, 0.01566),
	    vec3( 0.02840, 0.13383, 0.83777));

	mat3 ACES_OUTPUT_MAT = mat3_from_rows(
	    vec3( 1.60475,-0.53108,-0.07367),
	    vec3(-0.10208, 1.10813,-0.00605),
	    vec3(-0.00327,-0.07276, 1.07602));

    color = mul(ACES_INPUT_MAT, color);

    // Apply RRT and ODT
    color = rrt_odt_fit(color);

    color = mul(ACES_OUTPUT_MAT, color);

    // Clamp to [0, 1]
    color = clamp(color, 0.0, 1.0);

    return color;
}

vec3 toSRGB(vec3 v)
{ 
    return pow(v, vec3(1.0/2.2)); 
}

vec3 toneMapAndtoSRGB(vec3 L)
{
    L.r = L.r < 1.413 ? pow(L.r * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.r);
    L.g = L.g < 1.413 ? pow(L.g * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.g);
    L.b = L.b < 1.413 ? pow(L.b * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.b); 
    return L;
}

// ----------------------------------------------------------------------------
void main() 
{
    vec3 samples = texture(uTexSource, vTexcoords).rgb;

	// vec3 col = aces_fitted(samples);
	// col = toSRGB(col);
    vec3 col = toneMapAndtoSRGB(samples);

	fragColor = col;
}
