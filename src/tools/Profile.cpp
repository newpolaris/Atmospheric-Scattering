#include <tools/Profile.h>
#include <string>
#include <cassert>
#include <vector>

#include <GL/glew.h>
#include <glfw3.h>
#include <memory>

using std::chrono::high_resolution_clock;

namespace profiler
{
    // Use code 'dj_opengl'
    struct ProfileQuery final
    {
        ProfileQuery();
        ~ProfileQuery();

        void start();
        void stop();
        void tick();

        double cpuTicks() const;
        double gpuTicks() const;

        GLdouble cpu_ticks = 0.0, gpu_ticks = 0.0;
        GLint64 cpu_start_ticks = 0;
        GLuint queries[QueryCount];
        GLint is_gpu_ticking = GL_FALSE;
        GLint is_cpu_ticking = GL_FALSE;
        GLint is_gpu_ready = GL_TRUE;
    };

    std::vector<ProfileQuery> m_Queries;
}

void profiler::initialize()
{
    m_Queries.resize(MaxQuery);
}

void profiler::shutdown()
{
    m_Queries.clear();
}

void profiler::start(uint32_t idx)
{
    assert(idx < MaxQuery);
    m_Queries[idx].start();
}

void profiler::stop(uint32_t idx)
{
    assert(idx < MaxQuery);
    m_Queries[idx].stop();
}

void profiler::tick(uint32_t idx)
{
    m_Queries[idx].tick();
    auto tcpu = m_Queries[idx].cpuTicks();
    auto tgpu = m_Queries[idx].gpuTicks();

    printf("CPU : %10.5f ms\n", tcpu);
    printf("GPU : %10.5f ms\n", tgpu);
}

void profiler::tick(uint32_t idx, float& cpuTime, float& gpuTime)
{
    m_Queries[idx].tick();
    cpuTime = (float)m_Queries[idx].cpuTicks();
    gpuTime = (float)m_Queries[idx].gpuTicks();
}

using namespace profiler;

ProfileQuery::ProfileQuery()
{
    glGenQueries(QueryCount, queries);
    glQueryCounter(queries[QueryStart], GL_TIMESTAMP);
    glQueryCounter(queries[QueryStop], GL_TIMESTAMP);
}

ProfileQuery::~ProfileQuery()
{
    glDeleteQueries(QueryCount, queries);
    queries[QueryStart] = GL_ZERO;
    queries[QueryStop] = GL_ZERO;
}

void ProfileQuery::start()
{
    if (!is_cpu_ticking)
    {
        is_cpu_ticking = GL_TRUE;
        glGetInteger64v(GL_TIMESTAMP, &cpu_start_ticks);
    }
    if (!is_gpu_ticking && is_gpu_ready)
    {
        glQueryCounter(queries[QueryStart], GL_TIMESTAMP);
        is_gpu_ticking = GL_TRUE;
    }
}

void ProfileQuery::stop()
{
    if (is_cpu_ticking)
    {
        GLint64 now = 0;

        glGetInteger64v(GL_TIMESTAMP, &now);
        cpu_ticks = (now - cpu_start_ticks) / 1e6;
        is_cpu_ticking = GL_FALSE;
    }
    if (is_gpu_ticking && is_gpu_ready)
    {
        glQueryCounter(queries[QueryStop], GL_TIMESTAMP);
        is_gpu_ticking = GL_FALSE;
    }
}

void ProfileQuery::tick()
{
    if (!is_gpu_ticking)
    {
        glGetQueryObjectiv(queries[QueryStop],
            GL_QUERY_RESULT_AVAILABLE,
            &is_gpu_ready);
        if (is_gpu_ready)
        {
            GLuint64 start, stop;

            glGetQueryObjectui64v(queries[QueryStart],
                GL_QUERY_RESULT,
                &start);
            glGetQueryObjectui64v(queries[QueryStop],
                GL_QUERY_RESULT,
                &stop);
            gpu_ticks = (stop - start) / 1e6;
        }
    }
}

double profiler::ProfileQuery::cpuTicks() const
{
    return cpu_ticks;
}

double profiler::ProfileQuery::gpuTicks() const
{
    return gpu_ticks;
}

ProfileBusyWait::ProfileBusyWait(const std::string& name) : 
	m_name(name)
{
	m_queryCPU[QueryStart] = high_resolution_clock::now();
	// generate two queries
	glGenQueries(QueryCount, m_queryID);

	// issue the first query
	// Records the time only after all previous 
	// commands have been completed
	glQueryCounter(m_queryID[QueryStart], GL_TIMESTAMP);
}

ProfileBusyWait::~ProfileBusyWait()
{
	using namespace std::chrono;
	using millisec = duration<float, std::milli>;

	m_queryCPU[QueryStop] = high_resolution_clock::now();
	auto cpuTime = duration_cast<millisec>(m_queryCPU[QueryStop] - m_queryCPU[QueryStart]);

	// issue the second query
	// records the time when the sequence of OpenGL 
	// commands has been fully executed
	glQueryCounter(m_queryID[QueryStop], GL_TIMESTAMP);

	// wait until the results are available
	GLint stopTimerAvailable = 0;
	while(!stopTimerAvailable) {
		glGetQueryObjectiv(m_queryID[QueryStop],
				GL_QUERY_RESULT_AVAILABLE,
				&stopTimerAvailable);
	}

	// get query results
	GLuint64 startTime, stopTime;
	glGetQueryObjectui64v(m_queryID[QueryStart], GL_QUERY_RESULT, &startTime);
	glGetQueryObjectui64v(m_queryID[QueryStop], GL_QUERY_RESULT, &stopTime);

	printf("Time spent on the CPU %s: %f ms\n", m_name.c_str(), cpuTime.count());
	printf("Time spent on the GPU %s: %f ms\n", m_name.c_str(), (stopTime - startTime) / 1000000.0);

	fflush(stdout);
	glDeleteQueries(QueryCount, m_queryID);
}

