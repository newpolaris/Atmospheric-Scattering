#pragma once

#include <chrono>

struct SimpleTimer
{
    void initialize();
    void update();
    float duration() const;

    std::chrono::time_point<std::chrono::steady_clock> _first, _last;
};

