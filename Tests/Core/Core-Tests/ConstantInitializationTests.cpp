#include "ConstantInitializationTests.h"

#include <Core/Containers/Optional.h>
#include <Core/Containers/StringView.h>
#include <Core/Math/AABB.h>
#include <Core/Math/Color.h>
#include <Core/Math/IntVector2.h>
#include <Core/Math/IntVector3.h>
#include <Core/Math/IntVector4.h>
#include <Core/Math/Matrix2.h>
#include <Core/Math/Matrix3.h>
#include <Core/Math/Matrix3x4.h>
#include <Core/Math/Matrix4.h>
#include <Core/Math/Plane.h>
#include <Core/Math/Quaternion.h>
#include <Core/Math/Vector2.h>
#include <Core/Math/Vector3.h>
#include <Core/Math/Vector4.h>
#include <Core/Stats/Stats.h>
#include <Core/Threading/Atomic.h>
#include <Core/Threading/Spinlock.h>
#include <Core/Threading/ThreadLocal.h>
#include <Core/Time/Timespan.h>

#include "TestCommon/TestMacros.h"

static CONST_INIT AtomicInt32       GConstInitInt(42);
static CONST_INIT AtomicBool        GConstInitBool(true);
static CONST_INIT FSpinLock         GConstInitLock;
static CONST_INIT FThreadLocalSlot  GConstInitSlot;
static CONST_INIT Vector2           GConstInitVector2(1.0f, 2.0f);
static CONST_INIT Vector3           GConstInitVector3(1.0f, 2.0f, 3.0f);
static CONST_INIT Vector4           GConstInitVector4(Vector3(1.0f, 2.0f, 3.0f), 4.0f);
static CONST_INIT IntVector2        GConstInitIntVector2(1, 2);
static CONST_INIT IntVector3        GConstInitIntVector3(1, 2, 3);
static CONST_INIT IntVector4        GConstInitIntVector4(1, 2, 3, 4);
static CONST_INIT Matrix2           GConstInitMatrix2(1.0f);
static CONST_INIT Matrix3           GConstInitMatrix3(1.0f);
static CONST_INIT Matrix3x4         GConstInitMatrix3x4;
static CONST_INIT Matrix4           GConstInitMatrix4(1.0f);
static CONST_INIT Quaternion        GConstInitQuaternion(0.0f, 0.0f, 0.0f, 1.0f);
static CONST_INIT Plane             GConstInitPlane(Vector3(0.0f, 1.0f, 0.0f), 2.0f);
static CONST_INIT FColor            GConstInitColor(1, 2, 3, 4);
static CONST_INIT FFloatColor       GConstInitFloatColor(Vector3(0.25f, 0.5f, 0.75f));
static CONST_INIT FAABB             GConstInitAABB(Vector3(1.0f, 2.0f, 3.0f), Vector3(-1.0f, -2.0f, -3.0f));
static CONST_INIT FTimespan         GConstInitTimespan(1000);
static CONST_INIT TOptional<int32>  GConstInitOptional;
static CONST_INIT StringView        GConstInitView("Constant", 8);

#if STATS_ENABLED
STAT_DEFINE_COUNTER(STAT_ConstInitTestCounter, "Constant Initialization Test", "Tests");
#endif

bool ConstantInitialization_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Constant-initialized globals hold their values");
    {
        TEST_EXPECT_EQ(GConstInitInt.Load(), 42);
        TEST_EXPECT(GConstInitBool.Load());
        TEST_EXPECT(GConstInitLock.TryLock());
        GConstInitLock.Unlock();
        TEST_EXPECT(GConstInitSlot.GetValue() == nullptr);

        TEST_EXPECT_EQ(GConstInitVector2.Y, 2.0f);
        TEST_EXPECT_EQ(GConstInitVector3.Z, 3.0f);
        TEST_EXPECT_EQ(GConstInitVector4.W, 4.0f);
        TEST_EXPECT_EQ(GConstInitIntVector4.W, 4);
        TEST_EXPECT_EQ(GConstInitMatrix4.M[3][3], 1.0f);
        TEST_EXPECT_EQ(GConstInitMatrix4.M[0][1], 0.0f);
        TEST_EXPECT_EQ(GConstInitQuaternion.W, 1.0f);
        TEST_EXPECT_EQ(GConstInitColor.A, 4);
        TEST_EXPECT_EQ(GConstInitFloatColor.B, 0.75f);
        TEST_EXPECT_EQ(GConstInitFloatColor.A, 1.0f);
        TEST_EXPECT_EQ(GConstInitAABB.Min.X, -1.0f);
        TEST_EXPECT_EQ(GConstInitAABB.Max.Z, 3.0f);
        TEST_EXPECT(!GConstInitOptional.HasValue());
        TEST_EXPECT_EQ(GConstInitView.Length(), 8);
    }

    TEST_SECTION("Math constants are set before any code runs");
    {
        TEST_EXPECT_EQ(Vector3::Up.Y, 1.0f);
        TEST_EXPECT_EQ(Quaternion::Identity.W, 1.0f);
        TEST_EXPECT_EQ(FFloatColor::White.R, 1.0f);
    }

#if STATS_ENABLED
    TEST_SECTION("Stat storage is constant-initialized and still registered");
    {
        STAT_ADD(STAT_ConstInitTestCounter, 3);
        TEST_EXPECT_EQ(STAT_GET(STAT_ConstInitTestCounter), 3);

        bool bRegistered = false;
        for (FStatData* Stat : FStatRegistry::Get().GetAllStats())
        {
            bRegistered = bRegistered || (Stat == &STAT_ConstInitTestCounter);
        }

        TEST_EXPECT(bRegistered);
    }
#endif

    TEST_END();
}
