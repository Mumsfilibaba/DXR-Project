#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"
#include "LaunchProgram/ProgramEntry.h"

bool TestIntPoint2();
bool TestIntPoint3();

bool TestInt16Vector2();
bool TestInt16Vector3();

bool TestVector2();
bool TestVector3();
bool TestVector4();

bool TestMatrix2();
bool TestMatrix3();
bool TestMatrix4();

bool TestQuaternion();

bool TestFormatStructs();

bool TestCameraBasis();

static int32 RunMathTests()
{
    TestHarness::Initialize(TEXT("TestResults_Core.log"));
    LOG_INFO("=== Math Library Tests ===");

    RUN_TEST(TEXT("IntVector2"), TestIntPoint2());
    RUN_TEST(TEXT("IntVector3"), TestIntPoint3());

    RUN_TEST(TEXT("Int16Vector2"), TestInt16Vector2());
    RUN_TEST(TEXT("Int16Vector3"), TestInt16Vector3());

    RUN_TEST(TEXT("Vector2"), TestVector2());
    RUN_TEST(TEXT("Vector3"), TestVector3());
    RUN_TEST(TEXT("Vector4"), TestVector4());

    RUN_TEST(TEXT("Matrix2"), TestMatrix2());
    RUN_TEST(TEXT("Matrix3"), TestMatrix3());
    RUN_TEST(TEXT("Matrix4"), TestMatrix4());

    RUN_TEST(TEXT("Quaternion"), TestQuaternion());

    RUN_TEST(TEXT("FormatStructs"), TestFormatStructs());

    RUN_TEST(TEXT("CameraBasis"), TestCameraBasis());

    const int32 ExitCode = TestHarness::Report();
    TestHarness::Shutdown();
    return ExitCode;
}

IMPLEMENT_PROGRAM_MAIN(TEXT("Core-Math-Tests"), RunMathTests);
