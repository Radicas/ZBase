/**
 * @file time_platform.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-29
 * @version 0.1.0
 * @brief 时间原语实现
 */

#include "platform/time_platform.hpp"

#ifdef _WIN32
#include <Windows.h>
#else
#include <time.h>
#include <unistd.h>
#endif

namespace zbase {
namespace platform {

#ifdef _WIN32

namespace {
/// QueryPerformanceFrequency 缓存（避免每次调用）
int64_t g_qpf_freq = 0;
}  // namespace

uint64_t MonoNowNs() {
    if (g_qpf_freq == 0) {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        g_qpf_freq = freq.QuadPart;
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    // counter / freq * 1e9 → counter * 1e9 / freq
    return static_cast<uint64_t>(
        (counter.QuadPart * 1000000000LL) / g_qpf_freq);
}

int64_t WallNowMs() {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    uint64_t v = (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    // FILETIME 100ns 间隔，起点 1601-01-01；减去 11644473600000ms 得到 epoch 1970-01-01
    return static_cast<int64_t>(v / 10000ULL - 11644473600000ULL);
}

void SleepMs(uint32_t ms) {
    ::Sleep(ms);
}

#else  // POSIX

uint64_t MonoNowNs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1000000000ULL + ts.tv_nsec;
}

int64_t WallNowMs() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return static_cast<int64_t>(ts.tv_sec) * 1000LL + ts.tv_nsec / 1000000LL;
}

void SleepMs(uint32_t ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, nullptr);
}

#endif

}  // namespace platform
}  // namespace zbase
