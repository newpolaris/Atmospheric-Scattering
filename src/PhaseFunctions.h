#include <glm/glm.hpp>

glm::vec3 ComputeCoefficientRayleigh(const glm::vec3& lambda);
glm::vec3 ComputeCoefficientMie(const glm::vec3& lambda, const glm::vec3& K, float turbidity);
