// trace.hpp
// Cross-thread stack capture for Windows + Cygwin (GCC-friendly, -Werror friendly).
//
// - Windows: uses DbgHelp for symbolization (PDB exports etc.).
// - Cygwin: uses DbgHelp for module/function exports AND addr2line for DWARF
//          (tries both relative and absolute addresses to handle ASLR/PIE-ish cases).
//
// Build (Cygwin GCC):
//   g++ -std=c++20 -O0 -ggdb3 -fno-omit-frame-pointer -fno-optimize-sibling-calls -Wall -Wextra -Werror main.cc -o a.exe -ldbghelp -limagehlp
//
// Build (Windows MSVC):
//   cl /std:c++20 /Zi main.cc dbghelp.lib
//
// Notes:
// - Do NOT use #pragma comment on GCC/Cygwin (unknown-pragmas under -Werror).
// - For cross-thread capture you must pass a REAL thread HANDLE with:
//     THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION
// - Suspending a thread is intrusive; use only for debugging/profiling.

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#if !defined(_WIN32) && !defined(__CYGWIN__)
  #error "trace.hpp supports only Windows or Cygwin."
#endif

#ifndef NOMINMAX
  #define NOMINMAX
#endif
#include <windows.h>
#include <dbghelp.h>

#if defined(__CYGWIN__)
  #include <pthread.h>
#endif

namespace stacktrace
{
  struct Frame
  {
    std::uintptr_t pc{};
    std::string    function;
    std::string    module;
    std::string    file;
    std::uint32_t  line{};
    std::uintptr_t offset{};
  };

  enum CaptureFlags : std::uint32_t
  {
    None              = 0u,
    FilterSTL         = 1u << 0,
    KeepExeOnly       = 1u << 1,
    FilterConventions = 1u << 2,
  };

  static inline std::string strip_trailing_newlines(std::string s) noexcept
  {
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
    return s;
  }

#if defined(__CYGWIN__)
  extern "C" {
    FILE* popen(const char* command, const char* type);
    int   pclose(FILE* stream);
  }

  static inline std::string escape_for_bash_single_quotes(const std::string& s) noexcept
  {
    std::string out;
    out.reserve(s.size() + 2);
    out.push_back('\'');
    for (char ch : s)
    {
      if (ch == '\'') out += "'\"'\"'";
      else out.push_back(ch);
    }
    out.push_back('\'');
    return out;
  }

  static inline std::string run_pipe_read_all(const std::string& cmd) noexcept
  {
    FILE* fp = popen(cmd.c_str(), "r");
    if (!fp) return {};
    std::string out;
    char buf[512];
    while (std::fgets(buf, sizeof(buf), fp)) out += buf;
    (void)pclose(fp);
    return out;
  }

  static inline std::string winpath_to_cygpath_u(const std::string& winpath) noexcept
  {
    std::string cmd = "bash -c \"cygpath -u ";
    cmd += escape_for_bash_single_quotes(winpath);
    cmd += " 2>/dev/null | tail -n 1\"";
    return strip_trailing_newlines(run_pipe_read_all(cmd));
  }
#endif

  // Returns the module path for the main executable (best-effort).
  static inline const std::string& main_exe_module_path() noexcept
  {
    static std::string cached;
    if (!cached.empty()) return cached;

    char path[MAX_PATH];
    DWORD n = ::GetModuleFileNameA(nullptr, path, MAX_PATH);
    if (n == 0 || n >= MAX_PATH)
    {
      cached = "<unknown-exe>";
      return cached;
    }

    std::string win(path, path + n);

#if defined(__CYGWIN__)
    std::string posix = winpath_to_cygpath_u(win);
    cached = posix.empty() ? win : posix;
#else
    cached = win;
#endif

    return cached;
  }

  static inline bool string_ends_with(const std::string& s, const std::string& suffix) noexcept
  {
    if (suffix.size() > s.size()) return false;
    return std::memcmp(s.data() + (s.size() - suffix.size()), suffix.data(), suffix.size()) == 0;
  }

  // Keep frame if it came from the main executable.
  static inline bool is_exe_frame(const Frame& f) noexcept
  {
    const std::string& exe = main_exe_module_path();
    if (exe.empty() || exe == "<unknown-exe>") return false;
    if (f.module.empty()) return false;

    // Exact match is best (Cygwin tends to give posix paths).
    if (f.module == exe) return true;

    // Sometimes module is a slightly different representation; match by basename.
    // Compare “.../a.exe” endings.
    const std::size_t slash = exe.find_last_of("/\\");
    const std::string base = (slash == std::string::npos) ? exe : exe.substr(slash + 1);

    return string_ends_with(f.module, base);
  }

  static inline bool contains_substr(const std::string& s, const char* sub) noexcept
  {
    return (!s.empty() && sub && std::strstr(s.c_str(), sub) != nullptr);
  }

  static inline bool starts_with(const std::string& s, const char* pre) noexcept
  {
    if (!pre) return false;
    const std::size_t n = std::strlen(pre);
    return s.size() >= n && std::memcmp(s.data(), pre, n) == 0;
  }

  // Heuristic STL filter:
  // - Function begins with std:: / __gnu_cxx:: / std::__ / __cxxabiv1::
  // - OR debug location is in libstdc++ headers (/usr/include/c++ or .../include/c++)
  // - OR module path looks like libstdc++ / libgcc / libc++ (rare on Cygwin, but harmless)
  static inline bool is_stl_frame(const Frame& f) noexcept
  {
    // Function-based filtering (works even when file/line is missing).
    if (starts_with(f.function, "std::") ||
        starts_with(f.function, "std::__") ||
        starts_with(f.function, "__gnu_cxx::") ||
        starts_with(f.function, "__cxxabiv1::"))
    {
      return true;
    }

    // These are very common “STL plumbing” names (optional but useful).
    if (contains_substr(f.function, "std::__invoke") ||
        contains_substr(f.function, "__invoke_impl") ||
        contains_substr(f.function, "__invoke_r") ||
        contains_substr(f.function, "std::call_once") ||
        contains_substr(f.function, "std::once_flag") ||
        contains_substr(f.function, "std::__future_base") ||
        contains_substr(f.function, "std::function<"))
    {
      return true;
    }

    // File path based filtering (libstdc++ headers).
    if (contains_substr(f.file, "/usr/include/c++") ||
        contains_substr(f.file, "\\usr\\include\\c++") ||
        contains_substr(f.file, "/include/c++") ||
        contains_substr(f.file, "\\include\\c++") ||
        contains_substr(f.file, "/usr/lib/gcc/") ||
        contains_substr(f.file, "\\usr\\lib\\gcc\\"))
    {
      return true;
    }

    // Module path based filtering (best-effort; often empty or your exe).
    if (contains_substr(f.module, "libstdc++") ||
        contains_substr(f.module, "libgcc") ||
        contains_substr(f.module, "libc++"))
    {
      return true;
    }

    return false;
  }

  static inline bool is_cpp_convention(const Frame& f) noexcept
  {
    if (starts_with(f.function, "operator"))
    {
      return true;
    }

    return false;
  }

  static inline std::string stable_function_name(const Frame& f)
  {
    if (!f.function.empty() && !f.module.empty())
    {
      return f.module + "!" + f.function;
    }

    return f.function.empty() ? "<unknown>" : f.function;
  }

  // ----------------------------
  // DbgHelp init (process-wide)
  // ----------------------------
  static inline void ensure_symbols_initialized() noexcept
  {
    static bool ready = false;
    if (ready) return;
    ready = true;

    HANDLE proc = ::GetCurrentProcess();
    ::SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES);
    (void)::SymInitialize(proc, nullptr, TRUE);
  }

  // ----------------------------------------------
  // Utility: duplicate pseudo-handle -> real handle
  // (useful if you want to capture *current* thread)
  // ----------------------------------------------
  static inline HANDLE duplicate_current_thread_handle() noexcept
  {
    HANDLE dup = nullptr;
    (void)::DuplicateHandle(::GetCurrentProcess(),
                            ::GetCurrentThread(),     // pseudo handle
                            ::GetCurrentProcess(),
                            &dup,
                            THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION,
                            FALSE,
                            0);
    return dup; // caller must CloseHandle()
  }

  // ----------------------------------------------
  // DbgHelp symbolization: function/module/file/line
  // ----------------------------------------------
  static inline void symbolize_dbghelp(HANDLE proc, DWORD64 pc, Frame& f) noexcept
  {
    ensure_symbols_initialized();

    // Function name + offset
    {
      constexpr std::size_t kMaxName = 1024;
      std::vector<std::uint8_t> buf(sizeof(SYMBOL_INFO) + kMaxName);

      SYMBOL_INFO* si = reinterpret_cast<SYMBOL_INFO*>(buf.data());
      si->SizeOfStruct = sizeof(SYMBOL_INFO);
      si->MaxNameLen   = static_cast<ULONG>(kMaxName);

      DWORD64 disp = 0;
      if (::SymFromAddr(proc, pc, &disp, si))
      {
        // In w32api headers, Name is a fixed array (never null).
        f.function = (si->Name[0] != '\0') ? si->Name : "<unknown>";
        f.offset   = static_cast<std::uintptr_t>(disp);
      }
      else
      {
        if (f.function.empty()) f.function = "<unknown>";
      }
    }

    // Module image
    {
      IMAGEHLP_MODULE64 mod{};
      mod.SizeOfStruct = sizeof(mod);

      if (::SymGetModuleInfo64(proc, pc, &mod))
      {
        if (mod.ImageName[0] != '\0')
          f.module = mod.ImageName;
      }
    }

    // File:line (often unavailable without PDBs)
    {
      IMAGEHLP_LINE64 line{};
      line.SizeOfStruct = sizeof(line);

      DWORD disp32 = 0;
      if (::SymGetLineFromAddr64(proc, pc, &disp32, &line))
      {
        if (line.FileName) f.file = line.FileName;
        f.line = static_cast<std::uint32_t>(line.LineNumber);
      }
    }
  }

#if defined(__CYGWIN__)
  // ----------------------------
  // Cygwin helpers: popen/addr2line/cygpath
  // ----------------------------
  extern "C" {
    FILE* popen(const char* command, const char* type);
    int   pclose(FILE* stream);
  }

  static inline bool module_from_address(void* addr,
                                         std::uintptr_t& base_out,
                                         std::string& module_posix_out) noexcept
  {
    HMODULE mod = nullptr;
    if (!::GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                              GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                              reinterpret_cast<LPCSTR>(addr),
                              &mod))
      return false;

    base_out = reinterpret_cast<std::uintptr_t>(mod);

    char path[MAX_PATH];
    DWORD n = ::GetModuleFileNameA(mod, path, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return false;

    module_posix_out = winpath_to_cygpath_u(std::string(path, path + n));
    return !module_posix_out.empty();
  }

  static inline bool looks_unknown_addr2line(const std::string& s) noexcept
  {
    // Common unknown formats: "?? ??:0", "?? at ??:0"
    return (s.find("??") != std::string::npos) && (s.find("?:0") != std::string::npos);
  }

  static inline std::string addr2line_query(const std::string& module_posix,
                                            std::uintptr_t addr) noexcept
  {
    char addr_buf[32];
    std::snprintf(addr_buf, sizeof(addr_buf), "0x%llx",
                  static_cast<unsigned long long>(addr));

    // -f function, -C demangle, -p pretty, -i inline frames (we keep last line)
    std::string cmd = "bash -c \"addr2line -f -C -p -i -e ";
    cmd += escape_for_bash_single_quotes(module_posix);
    cmd += " ";
    cmd += addr_buf;
    cmd += " 2>/dev/null | tail -n 1\"";

    std::string out = run_pipe_read_all(cmd);
    out = strip_trailing_newlines(out);
    return out;
  }
#endif // __CYGWIN__

// --- NEW: trim helpers ---
  static inline std::string ltrim(std::string s) {
    std::size_t i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) ++i;
    s.erase(0, i);
    return s;
  }
  static inline std::string rtrim(std::string s) {
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) s.pop_back();
    return s;
  }
  static inline std::string trim(std::string s) { return rtrim(ltrim(std::move(s))); }

  // --- NEW: extract "func" and "file:line" from addr2line -p output ---
  // Expected forms:
  //   "func at /path/file.cc:123"
  //   "func at ??:0"
  //   "?? ??:0"
  static inline void parse_addr2line_pretty(const std::string& pretty,
                                            std::string& func_out,
                                            std::string& file_out,
                                            std::uint32_t& line_out) noexcept
  {
    func_out.clear();
    file_out.clear();
    line_out = 0;

    std::string s = trim(pretty);
    if (s.empty()) return;

    // If it's the unknown pattern, just leave func_out empty.
    if (looks_unknown_addr2line(s)) return;

    // Split on " at " (addr2line -p uses this delimiter)
    const std::string kAt = " at ";
    const std::size_t pos = s.find(kAt);

    if (pos == std::string::npos)
    {
      // No " at " => treat whole line as function name
      func_out = trim(s);
      return;
    }

    func_out = trim(s.substr(0, pos));
    std::string loc = trim(s.substr(pos + kAt.size())); // file:line (or ??:0)

    // loc might include " (discriminator N)" etc. Keep it simple: cut at first space.
    const std::size_t sp = loc.find(' ');
    if (sp != std::string::npos) loc = loc.substr(0, sp);

    // Split file:line
    const std::size_t colon = loc.rfind(':');
    if (colon == std::string::npos)
    {
      file_out = loc;
      return;
    }

    file_out = loc.substr(0, colon);

    // parse line number
    const char* p = loc.c_str() + colon + 1;
    char* endp = nullptr;
    unsigned long v = std::strtoul(p, &endp, 10);
    if (endp && endp != p)
      line_out = static_cast<std::uint32_t>(v);
  }

  // --- UPDATED: stable names ---
  // Clean function name only (no module, no file/line).
  static inline std::string stable_function_name_only(const Frame& f)
  {
    if (!f.function.empty() && f.function != "<unknown>")
      return f.function;
    return "<unknown>";
  }

  // If you want module!function (but still no file/line)
  static inline std::string stable_function_name_module_func(const Frame& f)
  {
    const std::string fn = stable_function_name_only(f);
    if (!f.module.empty() && fn != "<unknown>")
      return f.module + "!" + fn;
    return fn;
  }

#if defined(__CYGWIN__)
  // --- UPDATED: symbolize_addr2line now fills function/file/line cleanly ---
  static inline void symbolize_addr2line(Frame& f) noexcept
  {
    std::uintptr_t base = 0;
    std::string module_posix;

    if (!module_from_address(reinterpret_cast<void*>(f.pc), base, module_posix))
      return;

    f.module = module_posix;

    const std::uintptr_t abs = f.pc;
    const std::uintptr_t rel = (base && abs >= base) ? (abs - base) : abs;

    std::string out = addr2line_query(module_posix, rel);
    if (out.empty() || looks_unknown_addr2line(out))
    {
      std::string out2 = addr2line_query(module_posix, abs);
      if (!out2.empty() && !looks_unknown_addr2line(out2))
        out = std::move(out2);
    }

    if (out.empty() || looks_unknown_addr2line(out))
      return;

    std::string func, file;
    std::uint32_t line = 0;
    parse_addr2line_pretty(out, func, file, line);

    if (!func.empty())
      f.function = std::move(func);

    if (!file.empty() && file != "??")
      f.file = std::move(file);

    if (line != 0)
      f.line = line;
  }
#endif

  // ----------------------------------------------
  // Capture stack from another thread (suspend + context + StackWalk64)
  //
  // th must be a REAL thread handle with:
  //   THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION
  //
  // skip: number of walked frames to skip AFTER StackWalk has begun (not including capture()).
  // ----------------------------------------------
  static inline std::vector<Frame> capture(HANDLE th,
                                           std::size_t skip = 0,
                                           std::size_t max_frames = 64,
                                           std::uint32_t flags = (stacktrace::CaptureFlags::KeepExeOnly |
                                                                  stacktrace::CaptureFlags::FilterSTL   |
                                                                  stacktrace::CaptureFlags::FilterConventions)) noexcept
  {
    std::vector<Frame> out;
    out.reserve(max_frames);

    if (!th || th == INVALID_HANDLE_VALUE)
    {
      std::cerr << "[stacktrace] invalid thread handle\n";
      return out;
    }

    ensure_symbols_initialized();

    ::SetLastError(0);
    DWORD prev = ::SuspendThread(th);
    if (prev == static_cast<DWORD>(-1))
    {
      std::cerr << "[stacktrace] SuspendThread failed: " << ::GetLastError() << "\n";
      return out;
    }

    struct ResumeGuard
    {
      HANDLE t{};
      ~ResumeGuard() { (void)::ResumeThread(t); }
    } guard{th};

    CONTEXT ctx{};
    ctx.ContextFlags = CONTEXT_FULL;
    if (!::GetThreadContext(th, &ctx))
    {
      std::cerr << "[stacktrace] GetThreadContext failed: " << ::GetLastError() << "\n";
      return out;
    }

    HANDLE proc = ::GetCurrentProcess();

    STACKFRAME64 frame{};
    DWORD machine = 0;

#if defined(_M_X64) || defined(__x86_64__)
    machine = IMAGE_FILE_MACHINE_AMD64;
    frame.AddrPC.Offset    = static_cast<DWORD64>(ctx.Rip);
    frame.AddrStack.Offset = static_cast<DWORD64>(ctx.Rsp);
    frame.AddrFrame.Offset = static_cast<DWORD64>(ctx.Rbp);
#elif defined(_M_IX86) || defined(__i386__)
    machine = IMAGE_FILE_MACHINE_I386;
    frame.AddrPC.Offset    = static_cast<DWORD64>(ctx.Eip);
    frame.AddrStack.Offset = static_cast<DWORD64>(ctx.Esp);
    frame.AddrFrame.Offset = static_cast<DWORD64>(ctx.Ebp);
#else
  #error "Unsupported architecture for StackWalk64."
#endif

    frame.AddrPC.Mode    = AddrModeFlat;
    frame.AddrStack.Mode = AddrModeFlat;
    frame.AddrFrame.Mode = AddrModeFlat;

    std::size_t walked = 0;

    // Walk frames
    while (out.size() < max_frames)
    {
      BOOL ok = ::StackWalk64(machine,
                              proc,
                              th,
                              &frame,
                              &ctx,
                              nullptr,
                              ::SymFunctionTableAccess64,
                              ::SymGetModuleBase64,
                              nullptr);

      if (!ok || frame.AddrPC.Offset == 0)
        break;

      if (walked++ < skip)
        continue;

      Frame f{};
      f.pc = static_cast<std::uintptr_t>(frame.AddrPC.Offset);

      // 1) DbgHelp first: good for OS dll exports
      symbolize_dbghelp(proc, static_cast<DWORD64>(frame.AddrPC.Offset), f);

#if defined(__CYGWIN__)
      // 2) addr2line: fixes your EXE / DWARF modules (try rel+abs)
      // Only override if still unknown-ish.
      if (f.function.empty() || f.function == "<unknown>")
        symbolize_addr2line(f);
#endif

      if ((flags & CaptureFlags::KeepExeOnly) != 0u)
      {
        if (!is_exe_frame(f))
          continue; // drop this frame
      }

      if ((flags & CaptureFlags::FilterSTL) != 0u)
      {
        if (is_stl_frame(f))
          continue; // drop this frame
      }

      if ((flags & CaptureFlags::FilterConventions) != 0u)
      {
        if (is_cpp_convention(f))
          continue; // drop this frame
      }

      out.push_back(std::move(f));
    }

    if (out.empty())
      std::cerr << "[stacktrace] StackWalk64 produced 0 frames\n";

    return out; // guard resumes
  }

} // namespace stacktrace
