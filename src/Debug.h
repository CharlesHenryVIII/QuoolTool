#pragma once

#include <type_traits>
#include <string>

#define _CAR_CONCAT(a, b) a ## b
#define CAR_CONCAT(a, b) _CAR_CONCAT(a, b)

#define REQUIRE_SEMICOLON enum {}

#define FAIL ASSERT(false)
#define VERIFY(expr)        [](bool valid) -> bool { ASSERT(valid); return valid; }(!!(expr))
#define VALIDATE(expr)        { if (!VERIFY(expr)) return;     } REQUIRE_SEMICOLON
#define VALIDATE_V(expr, __v) { if (!VERIFY(expr)) return __v; } REQUIRE_SEMICOLON
#define arrsize(arr__) (sizeof(arr__) / sizeof(arr__[0]))

#define ENUMOPS(T) \
constexpr T operator++(T& a, int)\
{\
    a = T((std::underlying_type<T>::type)(a) + 1);\
    return a;\
}\
constexpr T operator--(T& a, int)\
{\
    a = T((std::underlying_type<T>::type)(a) - 1);\
    return a;\
}\
constexpr auto operator+(T a)\
{\
    return static_cast<typename std::underlying_type<T>::type>(a);\
}

#define ENUMOPS_PURE(T) \
constexpr auto operator+(T a)\
{\
    return static_cast<typename std::underlying_type<T>::type>(a);\
}\
constexpr T operator++(T& a, int)\
{\
    a = T((std::underlying_type<T>::type)(a) + 1);\
    return a;\
}

#define ENUMOPS_CLASS(T) \
constexpr auto operator+(T a)\
{\
    return static_cast<typename std::underlying_type<T>::type>(a);\
}\
friend constexpr T operator++(T& a, int)\
{\
    a = T((std::underlying_type<T>::type)(a) + 1);\
    return a;\
}

//constexpr T operator++(T& a, int)\
//{\
//    a = T((std::underlying_type<T>::type)(a) + 1);\
//    return a;\
//}
//constexpr GpuBuffer::Type operator++(GpuBuffer::Type& a, int)
//{
//    a = GpuBuffer::Type((std::underlying_type<GpuBuffer::Type>::type)(a) + 1);// = (static_cast<typename std::underlying_type<GpuBuffer::Type>::type>(a) + 1);
//    return a;
//}

//constexpr auto operator++(T a)\
//{\
//return (T)(static_cast<typename std::underlying_type<T>::type>(a)++); \
//}

#define FEATURE_CUSTOM_ASSERT 1

#if FEATURE_CUSTOM_ASSERT

#undef assert

#ifndef NDEBUG // NOTE: NDEBUG is what controls whether asserts exist in the c runtime
void OsAssert(bool expr, const char* message, const char* file, int line);
//#define assert(expr) OsAssert(expr, #expr, __FILE__, __LINE__)
#define ASSERT(expr) OsAssert(expr, #expr, __FILE__, __LINE__)
#define ASSERT_MSG(expr, msg) OsAssert(expr, msg, __FILE__, __LINE__)
#else
//#define assert(expr) ((void)0)
#define ASSERT(expr) ((void)0)
#define ASSERT_MSG(expr, msg) ((void)0)
#endif

#else

#include <assert.h>
#define ASSERT(expr) assert(expr)

#endif


#ifdef _DEBUGPRINT
#define DEBUG_LOG(...) DebugPrint(__VA_ARGS__)
#else
#define DEBUG_LOG(...) ((void)0)
#endif

/**********************************************
 *
 * Defer
 *
 ***************/

template <typename T>
struct ExitScope
{
    T lambda;
    ExitScope(T lambda): lambda(lambda){ }
    ~ExitScope(){ lambda();}
};

struct ExitScopeHelp
{
    template<typename T>
    ExitScope<T> operator+(T t) { return t; }
};

#define Defer auto CAR_CONCAT(defer__, __LINE__) = ExitScopeHelp() + [&]()


extern bool g_running;
extern char* g_ClipboardTextData;
