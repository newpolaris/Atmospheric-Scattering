#include <tools/SimpleProfile.h>
#include <string>

#include <GL/glew.h>
#include <glfw3.h>

using std::chrono::high_resolution_clock;

SimpleProfile::SimpleProfile(const std::string& name) : 
	m_name(name)
{
	m_queryCPU[0] = high_resolution_clock::now();
	// generate two queries
	glGenQueries(2, m_queryID);

	// issue the first query
	// Records the time only after all previous 
	// commands have been completed
	glQueryCounter(m_queryID[0], GL_TIMESTAMP);

}

SimpleProfile::~SimpleProfile()
{
	using namespace std::chrono;
	using millisec = duration<float, std::milli>;

	m_queryCPU[1] = high_resolution_clock::now();
	auto cpuTime = duration_cast<millisec>(m_queryCPU[1] - m_queryCPU[0]);

	// issue the second query
	// records the time when the sequence of OpenGL 
	// commands has been fully executed
	glQueryCounter(m_queryID[1], GL_TIMESTAMP);

	// wait until the results are available
	GLint stopTimerAvailable = 0;
	while(!stopTimerAvailable) {
		glGetQueryObjectiv(m_queryID[1],
				GL_QUERY_RESULT_AVAILABLE,
				&stopTimerAvailable);
	}

	// get query results
	GLuint64 startTime, stopTime;
	glGetQueryObjectui64v(m_queryID[0], GL_QUERY_RESULT, &startTime);
	glGetQueryObjectui64v(m_queryID[1], GL_QUERY_RESULT, &stopTime);

	printf("Time spent on the CPU %s: %f ms\n", m_name.c_str(), cpuTime.count());
	printf("Time spent on the GPU %s: %f ms\n", m_name.c_str(), (stopTime - startTime) / 1000000.0);

	fflush(stdout);
	glDeleteQueries(2, m_queryID);
}
