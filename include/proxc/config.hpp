
#pragma once

#include <proxc/version.hpp>

////////////////////////////////////////////////////////////////////////////////
// Detect the compiler
////////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER) && !defined(__clang__) // MSVC

#   pragma message("Warning: the native Microsoft compiler is not supported due to lack of proper C++14 support.")

#elif defined(__clang__) && defined(_MSC_VER) // Clang-cl (Clang for Windows)

#   define PROXC_CONFIG_CLANG PROXC_CONFIG_VERSION( \
        __clang_major__, __clang_minor__, __clang_patchlevel__)

#   if PROXC_CONFIG_CLANG < PROXC_CONFIG_VERSION(3, 5, 0)
#       warning "Versions of Clang prior to 3.5.0 are not supported for lack of proper C++14 support."
#   endif

#elif defined(__clang__) && defined(__apple_build_version__) // Apple's Clang

#   if __apple_build_version__ >= 6000051
#       define PROXC_CONFIG_CLANG PROXC_CONFIG_VERSION(3, 6, 0)
#   else
#       warning "Versions of Apple's Clang prior to the one shipped with XCode is not supported."
#   endif

#elif defined(__clang__) // Genuine Clang

#   define PROXC_CONFIG_CLANG PROXC_CONFIG_VERSION( \
        __clang_major__, __clang_minor__, __clang_patchlevel__)

#   if PROXC_CONFIG_CLANG < PROXC_CONFIG_VERSION(3, 5, 0)
#       warning "Versions of Clang prior to 3.5.0 are not supported for lack of proper C++14 support."
#   endif

#elif defined(__GNUC__) // GCC

#   define PROXC_CONFIC_GCC PROXC_CONFIG_VERSION(\
        __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)

#   if PROXC_CONFIC_GCC < PROXC_CONFIG_VERSION(5, 0, 0)
#       warning "Versions of GCC prior to 5.0.0 are not supported for lack of proper C++14 support."
#   endif

#else

#   warning "Your compiler was not detected properly and might not work for ProXC."

#endif


////////////////////////////////////////////////////////////////////////////////
// Detect the compiler for general C++14 capabilities
////////////////////////////////////////////////////////////////////////////////

#if (__cplusplus < 201400)
#   if defined(_MSC_VER)
#       pragma message("Warning: Your compiler doesn't provide C++14 or higher capabilities. Try adding the compiler flag 'std=c++14' or '-std=c++1y'.")
#   else
#       warning "Your compiler doesn't provde C++14 or higher capabilities. Try adding the compiler flag 'std=c++14' or 'std=c++1y'"
#   endif
#endif


////////////////////////////////////////////////////////////////////////////////
// Detect the standard library
////////////////////////////////////////////////////////////////////////////////

#include <cstddef>

#if defined(_LIBCPP_VERSION)

#   define PROXC_CONFIG_LIBCPP PROXC_CONFIG_VERSION( \
        ((_LIBCPP_VERSION) / 1000) % 10, 0, (_LIBCPP_VERSION) % 1000)

#   if PROXC_CONFIG_LIBCPP < PROXC_CONFIG_VERSION(1, 0, 101)
#       warning "Versions of libc++ prior to the one shipped with Clang 3.5.0 are not supported for lack of full C++14 support."
#   endif

#elif defined(__GLIBCXX__)

#   if __GLIBCXX__ < 20150422 // --> the libstdc++ shipped with GCC 5.1.0
#       warning "Versions of libstdc++ prior to the one shipped with GCC 5.1.0 are not supported for lack of full C++14 support."
#   endif

#   define PROXC_CONFIG_LIBSTDCXX

#elif defined(_MSC_VER)

#   define PROXC_CONFIG_LIBMSVCCXX

#else

#   warning "Your standard library was not detected properly and might not work for ProXC."

#endif


////////////////////////////////////////////////////////////////////////////////
// Namespace macros
////////////////////////////////////////////////////////////////////////////////

#define PROXC_NAMESPACE_BEGIN namespace proxc {

#define PROXC_NAMESPACE_END }

