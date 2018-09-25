#include "SimpleTimer.h"

void SimpleTimer::initialize()
{
    _last = _first = std::chrono::steady_clock::now();
}

void SimpleTimer::update()
{
    _last = std::chrono::steady_clock::now();
}

float SimpleTimer::duration() const
{
    return float(std::chrono::duration_cast<std::chrono::milliseconds>(_last - _first).count()) / 1000;
}
