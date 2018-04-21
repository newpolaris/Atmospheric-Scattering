#pragma once

#include <vector>
#include <glm/glm.hpp>

struct Atmosphere
{
public:
	Atmosphere(glm::vec3 sunDir);
	glm::vec4 computeIncidentLight(const glm::vec3& orig, const glm::vec3& dir, float tmin, float tmax) const; 
	void renderSkyDome(std::vector<glm::vec4>& image, int width, int height) const;

	float m_Hr = 7994; // Rayleigh scale height
    float m_Hm = 1200; // Mie scale height
	float m_Ar = 6420e3; // atmosphere radius
	float m_Er = 6360e3; // earth radius

	glm::vec3 m_SunDir;
	glm::vec3 m_Ec = glm::vec3(0.f); // earth center
	glm::vec3 m_BetaR0 = glm::vec3(3.8e-6f, 13.5e-6f, 33.1e-6f); 
    glm::vec3 m_BetaM0 = glm::vec3(210e-5f);
};
