#pragma once

namespace common
{
  /**
   * @brief Terminates execution immediately.
   *
   * This function halts execution without stack unwinding.
   * On GCC/Clang it emits a CPU trap instruction, allowing debuggers
   * to break precisely at the failure site.
   *
   * @note This function does not return.
   */
  [[noreturn]] static inline void fatal_trap(void)
  {
  #if defined(__GNUC__) || defined(__clang__)
    __builtin_trap();
  #else
    std::abort();
  #endif
  }

  /**
   * @brief Issues a CPU hint for spin-wait loops.
   *
   * On x86 architectures this emits the PAUSE instruction, reducing
   * power consumption and contention while spinning.
   *
   * On unsupported architectures, this function is a no-op.
   *
   * @note Intended for use inside tight polling loops.
   */
  static inline void cpu_relax(void)
  {
  #if defined(__x86_64__) || \
      defined(_M_X64)     || \
      defined(__i386__)   || \
      defined(_M_IX86)
    #if defined(_MSC_VER)
      _mm_pause();
    #else
      __builtin_ia32_pause();
    #endif
  #else
    // no-op on unsupported architectures
  #endif
  }
} // namespace common
