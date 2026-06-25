#pragma once
#include "TestHarness.h"

#include <Core/Misc/OutputDeviceLogger.h>

/** @brief Fail the current test (which must return bool) if Condition is false. */
#define TEST_CHECK(Condition) \
    do \
    { \
        if (!(Condition)) \
        { \
            LOG_ERROR("[TEST FAILED] Condition='%s' (%s:%d)", #Condition, __FILE__, __LINE__); \
            return false; \
        } \
    } while (false)

/** @brief Fail the current test (which must return bool) if Lhs != Rhs. */
#define TEST_CHECK_EQ(Lhs, Rhs) \
    do \
    { \
        if (!((Lhs) == (Rhs))) \
        { \
            LOG_ERROR("[TEST FAILED] '%s' != '%s' (%s:%d)", #Lhs, #Rhs, __FILE__, __LINE__); \
            return false; \
        } \
    } while (false)

/** @brief Unconditionally fail the current test (which must return bool). */
#define TEST_FAILED() \
    do \
    { \
        LOG_ERROR("[TEST FAILED] (%s:%d)", __FILE__, __LINE__); \
        return false; \
    } while (false)

#define TEST_BEGIN()       bool _bTestPassed = true; const CHAR* _TestSection = ""
#define TEST_SECTION(Name) _TestSection = (Name)
#define TEST_END()         return _bTestPassed

#define TEST_EXPECT(Condition) \
    do \
    { \
        if (!(Condition)) \
        { \
            LOG_ERROR("[FAIL] %s : '%s' (%s:%d)", _TestSection, #Condition, __FILE__, __LINE__); \
            _bTestPassed = false; \
        } \
    } while (false)

#define TEST_EXPECT_EQ(Lhs, Rhs) \
    do \
    { \
        if (!((Lhs) == (Rhs))) \
        { \
            LOG_ERROR("[FAIL] %s : '%s' != '%s' (%s:%d)", _TestSection, #Lhs, #Rhs, __FILE__, __LINE__); \
            _bTestPassed = false; \
        } \
    } while (false)

/**
 * @brief Run a test expression, record the result with the harness and log it.
 * @param Name A string literal naming the test.
 * @param Expr An expression evaluating to bool (true == passed).
 */
#define RUN_TEST(Name, Expr) \
    do \
    { \
        if (Expr) \
        { \
            TestHarness::AddPass(); \
            LOG_INFO("[ OK ] %s", Name); \
        } \
        else \
        { \
            TestHarness::AddFail(); \
            LOG_ERROR("[FAIL] %s", Name); \
        } \
    } while (false)
