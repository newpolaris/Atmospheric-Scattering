#include <chrono>
#include <string>

#define ProfileOnce(name) ProfileBusyWait __profile(name)

namespace profiler
{
    enum QueryID
    {
        QueryStart = 0,
        QueryStop,
        QueryCount
    };

    void initialize();
    void shutdown();

    enum { MaxQuery = 100 };

    void start(uint32_t idx);
    void stop(uint32_t idx);
    void tick(uint32_t idx);
    void tick(uint32_t idx, float& cpuTime, float& gpuTime);

    class ProfileBusyWait final
    {
    public:
        ProfileBusyWait(const std::string& name);
        ~ProfileBusyWait();

        std::string m_name;
        std::chrono::high_resolution_clock::time_point m_queryCPU[2];
        unsigned int m_queryID[2];
    };
}