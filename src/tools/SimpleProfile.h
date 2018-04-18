#include <chrono>
#include <string>

#define PROFILEGL(name) SimpleProfile __profile(name)

class SimpleProfile final
{
public:
    SimpleProfile(const std::string& name);
	~SimpleProfile();

    std::string m_name;
	std::chrono::high_resolution_clock::time_point m_queryCPU[2];
    unsigned int m_queryID[2];
};
