
#pragma once

#include <proxc/config.hpp>

PROXC_NAMESPACE_BEGIN
namespace detail {

#if defined(PROXC_ARCH_ARM)

#   if defined(PROXC_COMP_MSVC)
#       define cpu_relax() YieldProcessor()
#   else
#       define cpu_relax() asm volatile ("yield" ::: "memory")
#   endif

#elif defined(PROXC_ARCH_MIPS)

#   define cpu_relax()  asm volatile ("pause" ::: "memory")

#elif defined(PROXC_ARCH_PPC)

#   define cpu_relax()  asm volatile ("or 27,27,27" ::: "memory")

#elif defined(PROXC_ARCH_X86)

#   if defined(PROXC_COMP_MSVC)
#       define cpu_relax()  YieldProcessor()
#   else
#       define cpu_relax()  asm volatile ("pause" ::: "memory")
#   endif

#else /* unknown architecture */

#   warning "architecture does not support yield/pause mnemonic"
#   define cpu_relax()  std::this_thread_yield()

#endif

} // namespace detail
PROXC_NAMESPACE_END

