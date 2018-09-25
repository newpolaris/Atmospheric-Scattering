#include <PhaseFunctions.h>
#include <glm/gtc/type_ptr.hpp> 

glm::vec3 ComputeCoefficientRayleigh(const glm::vec3& lambda)
{
    const float n = 1.0003f; // refractive index
    const float N = 2.545e25f; // molecules per unit
    const float p = 0.035f; // depolarization factor for standard air
    const float pi = glm::pi<float>();

    const glm::vec3 l4 = lambda*lambda*lambda*lambda;
    return 8*pi*pi*pi*glm::pow(n*n - 1, 2) / (3*N*l4) * ((6 + 3*p)/(6 - 7*p));
}

glm::vec3 ComputeCoefficientMie(const glm::vec3& lambda, const glm::vec3& K, float turbidity)
{
    const int jungeexp = 4;
    const float pi = glm::pi<float>();
    const float c = glm::max(0.f, 0.6544f*turbidity - 0.6510f)*1e-16f; // concentration factor
    const float mie =  0.434f * c * pi * glm::pow(2*pi, jungeexp - 2);
    return mie * K / lambda;
}
