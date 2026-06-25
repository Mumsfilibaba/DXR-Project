#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

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

int main()
{
    TestHarness::Initialize();
    LOG_INFO("=== Math Library Tests ===");

    RUN_TEST("IntPoint2", TestIntPoint2());
    RUN_TEST("IntPoint3", TestIntPoint3());

    RUN_TEST("Int16Vector2", TestInt16Vector2());
    RUN_TEST("Int16Vector3", TestInt16Vector3());

    RUN_TEST("Vector2", TestVector2());
    RUN_TEST("Vector3", TestVector3());
    RUN_TEST("Vector4", TestVector4());

    RUN_TEST("Matrix2", TestMatrix2());
    RUN_TEST("Matrix3", TestMatrix3());
    RUN_TEST("Matrix4", TestMatrix4());

    RUN_TEST("Quaternion", TestQuaternion());

    const int32 ExitCode = TestHarness::Report();
    TestHarness::Shutdown();
    return ExitCode;
}
