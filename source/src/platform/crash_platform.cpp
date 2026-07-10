/**
 * @file crash_platform.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-07-10
 * @version 0.2.0
 * @brief 崩溃堆栈平台原语实现
 * @details Windows：动态加载 dbghelp.dll 进行符号解析。
 *          POSIX：使用 backtrace() + dladdr()。
 *          符号解析采用三级降级：完整符号 → 导出函数名 → 模块名+偏移。
 */

#include "platform/crash_platform.hpp"

#include <cstdio>
#include <cstring>

// ============================================
// Windows 实现
// ============================================
#ifdef _WIN32

namespace {

/// dbghelp.dll 函数指针类型
typedef BOOL(WINAPI* PFN_SymInitialize)(HANDLE, PCSTR, BOOL);
typedef BOOL(WINAPI* PFN_SymCleanup)(HANDLE);
typedef BOOL(WINAPI* PFN_SymFromAddr)(HANDLE, DWORD64, PDWORD64, PSYMBOL_INFO);
typedef BOOL(WINAPI* PFN_SymGetLineFromAddr64)(HANDLE, DWORD64, PDWORD, PIMAGEHLP_LINE64);
typedef DWORD(WINAPI* PFN_SymSetOptions)(DWORD);
typedef DWORD(WINAPI* PFN_SymGetOptions)(void);

HMODULE        g_dbghelp_dll = nullptr;
PFN_SymInitialize       pSymInitialize = nullptr;
PFN_SymCleanup          pSymCleanup = nullptr;
PFN_SymFromAddr         pSymFromAddr = nullptr;
PFN_SymGetLineFromAddr64 pSymGetLineFromAddr64 = nullptr;
PFN_SymSetOptions       pSymSetOptions = nullptr;
PFN_SymGetOptions       pSymGetOptions = nullptr;

/// 用于 SymFromAddr 的缓冲区（含 MAX_SYM_NAME 空间）
struct SymInfoBuf {
    SYMBOL_INFO info;
    char        name_buf[512];
};

}  // namespace

bool zbase::platform::CrashSymInit()
{
    if (g_dbghelp_dll) return true;  // 已初始化

    g_dbghelp_dll = LoadLibraryA("dbghelp.dll");
    if (!g_dbghelp_dll) return false;

    // 加载所需函数（部分函数可选）
    pSymInitialize = (PFN_SymInitialize)GetProcAddress(g_dbghelp_dll, "SymInitialize");
    pSymCleanup    = (PFN_SymCleanup)GetProcAddress(g_dbghelp_dll, "SymCleanup");
    pSymFromAddr   = (PFN_SymFromAddr)GetProcAddress(g_dbghelp_dll, "SymFromAddr");
    pSymGetLineFromAddr64 = (PFN_SymGetLineFromAddr64)GetProcAddress(g_dbghelp_dll, "SymGetLineFromAddr64");
    pSymSetOptions = (PFN_SymSetOptions)GetProcAddress(g_dbghelp_dll, "SymSetOptions");
    pSymGetOptions = (PFN_SymGetOptions)GetProcAddress(g_dbghelp_dll, "SymGetOptions");

    if (!pSymInitialize || !pSymFromAddr) {
        FreeLibrary(g_dbghelp_dll);
        g_dbghelp_dll = nullptr;
        pSymInitialize = nullptr;
        pSymFromAddr = nullptr;
        return false;
    }

    // 初始化符号系统：搜索路径设为 NULL，使用默认搜索策略
    HANDLE hProcess = GetCurrentProcess();
    if (!pSymInitialize(hProcess, nullptr, TRUE)) {
        FreeLibrary(g_dbghelp_dll);
        g_dbghelp_dll = nullptr;
        pSymInitialize = nullptr;
        pSymFromAddr = nullptr;
        return false;
    }

    // 启用行号解析 + 延迟加载（提升性能）
    if (pSymSetOptions && pSymGetOptions) {
        DWORD opts = pSymGetOptions();
        opts |= SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME;
        pSymSetOptions(opts);
    }

    return true;
}

void zbase::platform::CrashSymCleanup()
{
    if (g_dbghelp_dll) {
        if (pSymCleanup) {
            pSymCleanup(GetCurrentProcess());
        }
        FreeLibrary(g_dbghelp_dll);
        g_dbghelp_dll = nullptr;
    }
    pSymInitialize = nullptr;
    pSymCleanup = nullptr;
    pSymFromAddr = nullptr;
    pSymGetLineFromAddr64 = nullptr;
    pSymSetOptions = nullptr;
    pSymGetOptions = nullptr;
}

int zbase::platform::CrashCaptureBacktrace(void** frames, int max_frames, void* /*context*/)
{
    // CaptureStackBackTrace 从 kernel32.dll 提供，无需符号系统
    USHORT count = CaptureStackBackTrace(0, (ULONG)max_frames, frames, nullptr);
    return static_cast<int>(count);
}

void zbase::platform::CrashResolveFrame(void* address, CrashFrame& out)
{
    std::memset(&out, 0, sizeof(out));
    out.address = address;

    // 获取模块名
    MEMORY_BASIC_INFORMATION mbi = {};
    if (VirtualQuery(address, &mbi, sizeof(mbi))) {
        HMODULE hModule = (HMODULE)mbi.AllocationBase;
        char    mod_path[MAX_PATH] = {0};
        DWORD   len = GetModuleFileNameA(hModule, mod_path, sizeof(mod_path));
        if (len > 0) {
            // 提取文件名部分（不含路径）
            const char* base = mod_path;
            for (const char* p = mod_path; *p; ++p) {
                if (*p == '\\' || *p == '/') base = p + 1;
            }
            std::snprintf(out.module_name, sizeof(out.module_name), "%s", base);
        }

        // 计算偏移
        DWORD64 module_base = (DWORD64)mbi.AllocationBase;
        out.offset = (DWORD64)address - module_base;
    }

    // 尝试符号解析（三级降级）
    if (g_dbghelp_dll && pSymFromAddr) {
        SymInfoBuf sym_buf;
        std::memset(&sym_buf, 0, sizeof(sym_buf));
        sym_buf.info.SizeOfStruct = sizeof(SYMBOL_INFO);
        sym_buf.info.MaxNameLen = sizeof(sym_buf.name_buf) - 1;

        DWORD64 disp = 0;
        HANDLE  hProcess = GetCurrentProcess();

        // 第一级：完整符号（函数名 + 偏移）
        if (pSymFromAddr(hProcess, (DWORD64)address, &disp, &sym_buf.info)) {
            std::snprintf(out.function_name, sizeof(out.function_name),
                          "%s", sym_buf.info.Name);
            out.offset = static_cast<uint64_t>(disp);

            // 第二级：源文件 + 行号
            if (pSymGetLineFromAddr64) {
                IMAGEHLP_LINE64 line = {};
                line.SizeOfStruct = sizeof(line);
                DWORD line_disp = 0;
                if (pSymGetLineFromAddr64(hProcess, (DWORD64)address,
                                          &line_disp, &line)) {
                    const char* file_base = line.FileName;
                    // 提取文件名
                    for (const char* p = line.FileName; *p; ++p) {
                        if (*p == '\\' || *p == '/') file_base = p + 1;
                    }
                    std::snprintf(out.source_file, sizeof(out.source_file),
                                  "%s", file_base);
                    out.source_line = static_cast<int>(line.LineNumber);
                }
            }
            return;  // 已成功解析到函数名级别
        }
    }

    // 第三级：仅有模块名 + 偏移（已通过 VirtualQuery 获取）
    if (out.module_name[0] == '\0') {
        std::snprintf(out.module_name, sizeof(out.module_name), "<unknown>");
    }
}

// ============================================
// POSIX 实现
// ============================================
#else  // !_WIN32

#include <cxxabi.h>  // __cxa_demangle

bool zbase::platform::CrashSymInit()
{
    // POSIX 下 backtrace() + dladdr() 无需额外初始化
    return true;
}

void zbase::platform::CrashSymCleanup()
{
    // POSIX 下无需清理
}

int zbase::platform::CrashCaptureBacktrace(void** frames, int max_frames, void* /*context*/)
{
    return backtrace(frames, max_frames);
}

void zbase::platform::CrashResolveFrame(void* address, CrashFrame& out)
{
    std::memset(&out, 0, sizeof(out));
    out.address = address;

    // 第一级：使用 dladdr 获取符号信息
    Dl_info dl_info = {};
    if (dladdr(address, &dl_info) && dl_info.dli_fname) {
        // 提取模块文件名
        const char* base = dl_info.dli_fname;
        for (const char* p = dl_info.dli_fname; *p; ++p) {
            if (*p == '/') base = p + 1;
        }
        std::snprintf(out.module_name, sizeof(out.module_name), "%s", base);

        // 计算偏移
        if (dl_info.dli_fbase) {
            out.offset = (uint64_t)((char*)address - (char*)dl_info.dli_fbase);
        }

        // 第二级：函数名（含 C++ name demangle）
        // 注意：__cxa_demangle 内部可能调用 malloc()，非严格信号安全。
        // 但这是业界广泛采用的做法（如 backward-cpp、Google Breakpad），
        // 实际崩溃场景中极少因此引发二次崩溃。
        if (dl_info.dli_sname) {
            int   status = 0;
            char* demangled = abi::__cxa_demangle(dl_info.dli_sname, nullptr,
                                                  nullptr, &status);
            if (status == 0 && demangled) {
                std::snprintf(out.function_name, sizeof(out.function_name),
                              "%s", demangled);
                std::free(demangled);
            } else {
                std::snprintf(out.function_name, sizeof(out.function_name),
                              "%s", dl_info.dli_sname);
            }
            // 计算函数内偏移
            if (dl_info.dli_saddr) {
                out.offset = (uint64_t)((char*)address - (char*)dl_info.dli_saddr);
            }
        }
        return;  // dladdr 已提供足够信息
    }

    // 第三级：无法解析时仅保留地址
    out.module_name[0] = '\0';
    std::snprintf(out.module_name, sizeof(out.module_name), "<unknown>");
}

#endif  // _WIN32
