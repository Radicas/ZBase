/**
 * @file time.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-29
 * @version 0.1.0
 * @brief 时间工具实现
 */

#include "zbase/time.h"
#include "zbase/error.h"
#include "zbase/internal/c_api_guard.hpp"
#include "platform/time_platform.hpp"

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#include <time.h>
#endif

#include <cstdio>
#include <ctime>

extern "C" {

int64_t z_time_now_ms(void) {
    return zbase::platform::WallNowMs();
}

int64_t z_time_now_us(void) {
#ifdef _WIN32
    // GetSystemTimePreciseAsFileTime 精度 0.1us（Win8+）
    FILETIME ft;
    GetSystemTimePreciseAsFileTime(&ft);
    uint64_t v = (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    // FILETIME 100ns 间隔 → 微秒；起点 1601-01-01 → 减去到 1970 的偏移
    return static_cast<int64_t>(v / 10ULL - 11644473600000000ULL);
#else
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return static_cast<int64_t>(tv.tv_sec) * 1000000LL + tv.tv_usec;
#endif
}

int z_time_format_iso(int64_t ms, char* buf, size_t buf_size) {
    ZBASE_C_API_BEGIN
    if (buf == nullptr || buf_size == 0) return Z_ERR_INVALID_ARG;
    // 输出格式 "YYYY-MM-DDTHH:MM:SS.mmm" 共 23 字符 + '\0' = 24 字节
    if (buf_size < 24) return Z_ERR_OVERFLOW;
    time_t seconds = static_cast<time_t>(ms / 1000);
    int millis = static_cast<int>(ms % 1000);
    if (millis < 0) { millis += 1000; seconds -= 1; }
    struct tm tmv;
#ifdef _WIN32
    if (localtime_s(&tmv, &seconds) != 0) return Z_ERR_UNKNOWN;
#else
    if (localtime_r(&seconds, &tmv) == nullptr) return Z_ERR_UNKNOWN;
#endif
    int n = std::snprintf(buf, buf_size,
                          "%04d-%02d-%02dT%02d:%02d:%02d.%03d",
                          tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
                          tmv.tm_hour, tmv.tm_min, tmv.tm_sec, millis);
    if (n < 0 || static_cast<size_t>(n) >= buf_size) return Z_ERR_OVERFLOW;
    return Z_OK;
    ZBASE_C_API_END(Z_ERR_UNKNOWN)
}

void z_time_sleep_ms(uint32_t ms) {
    zbase::platform::SleepMs(ms);
}

void z_time_sleep_us(uint64_t us) {
#ifdef _WIN32
    // Windows 最小 sleep 精度约 1ms（15.6ms 默认 timer granularity）
    // 超过 1ms 用 Sleep，否则用 QPC 忙等以获得更高精度
    if (us >= 1000) {
        ::Sleep(static_cast<DWORD>(us / 1000));
    } else {
        uint64_t start = zbase::platform::MonoNowNs();
        while (zbase::platform::MonoNowNs() - start < us * 1000ULL) {
            // 忙等
        }
    }
#else
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(us / 1000000ULL);
    ts.tv_nsec = static_cast<long>(us % 1000000ULL) * 1000ULL;
    nanosleep(&ts, nullptr);
#endif
}

}  // extern "C"
