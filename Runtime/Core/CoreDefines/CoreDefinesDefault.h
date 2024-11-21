#pragma once

#ifndef FORCEINLINE
    #define FORCEINLINE inline
#endif

#ifndef ALIGN_AS
    #define ALIGN_AS(Alignment) alignas(Alignment)
#endif

#ifndef NOINLINE
    #define NOINLINE
#endif

#ifndef VECTORCALL
    #define VECTORCALL
#endif

#ifndef RESTRICT
    #define RESTRICT
#endif

#ifndef FUNCTION_SIGNATURE
    #define FUNCTION_SIGNATURE __func__
#endif

#ifndef MODULE_EXPORT
    #define MODULE_EXPORT
#endif

#ifndef MODULE_IMPORT
    #define MODULE_IMPORT
#endif

#ifndef DEBUG_BREAK
    #define DEBUG_BREAK() ((void)0)
#endif

// Warning Control Macros

// Disable unreferenced variable warning
#if !defined(DISABLE_UNREFERENCED_VARIABLE_WARNING)
    #define DISABLE_UNREFERENCED_VARIABLE_WARNING
    #define ENABLE_UNREFERENCED_VARIABLE_WARNING
#endif

// Disable unreachable code warning
#if !defined(DISABLE_UNREACHABLE_CODE_WARNING)
    #define DISABLE_UNREACHABLE_CODE_WARNING
    #define ENABLE_UNREACHABLE_CODE_WARNING
#endif

// Disable hides previous local declaration warning
#if !defined(DISABLE_HIDES_PREVIOUS_LOCAL_DEFINITION_WARNING)
    #define DISABLE_HIDES_PREVIOUS_LOCAL_DEFINITION_WARNING
    #define ENABLE_HIDES_PREVIOUS_LOCAL_DEFINITION_WARNING
#endif
