#include "MathTest.h"

#include <Core/Math/FormatStructs.h>

bool TestFormatStructs()
{
    TEST_BEGIN();

    TEST_SECTION("FRGBA16Snorm::FRGBA16Snorm (size and layout)");
    TEST_EXPECT(sizeof(FRGBA16Snorm) == 8);
    TEST_EXPECT(FRGBA16Snorm() == FRGBA16Snorm(int16(0), int16(0), int16(0), int16(0)));

    TEST_SECTION("FRGBA16Snorm::EncodeChannel / DecodeChannel (exact unit values)");
    TEST_EXPECT(FRGBA16Snorm::EncodeChannel(1.0f) == int16(32767));
    TEST_EXPECT(FRGBA16Snorm::EncodeChannel(-1.0f) == int16(-32767));
    TEST_EXPECT(FRGBA16Snorm::EncodeChannel(0.0f) == int16(0));
    TEST_EXPECT(FRGBA16Snorm::DecodeChannel(int16(32767)) == 1.0f);
    TEST_EXPECT(FRGBA16Snorm::DecodeChannel(int16(-32767)) == -1.0f);
    TEST_EXPECT(FRGBA16Snorm::DecodeChannel(int16(0)) == 0.0f);

    TEST_SECTION("FRGBA16Snorm::EncodeChannel (clamping)");
    TEST_EXPECT(FRGBA16Snorm::EncodeChannel(2.0f) == int16(32767));
    TEST_EXPECT(FRGBA16Snorm::EncodeChannel(-2.0f) == int16(-32767));

    TEST_SECTION("FRGBA16Snorm::DecodeChannel (lowest channel clamps at -1)");
    TEST_EXPECT(FRGBA16Snorm::DecodeChannel(TNumericLimits<int16>::Min()) == -1.0f);

    TEST_SECTION("FRGBA16Snorm::ToVector3 / GetAlpha (axis round trip)");
    {
        const FRGBA16Snorm AxisX(Vector3(1.0f, 0.0f, 0.0f), 1.0f);
        const FRGBA16Snorm AxisY(Vector3(0.0f, -1.0f, 0.0f), -1.0f);

        TEST_EXPECT(AxisX.ToVector3() == Vector3(1.0f, 0.0f, 0.0f));
        TEST_EXPECT(AxisX.GetAlpha() == 1.0f);
        TEST_EXPECT(AxisY.ToVector3() == Vector3(0.0f, -1.0f, 0.0f));
        TEST_EXPECT(AxisY.GetAlpha() == -1.0f);
    }

    TEST_SECTION("FRGBA16Snorm::ToVector3 (arbitrary direction stays within a quantization step)");
    {
        const Vector3      Direction = Vector3(0.267261f, 0.534522f, 0.801784f);
        const FRGBA16Snorm Encoded(Direction);
        const Vector3      Decoded = Encoded.ToVector3();

        const float Tolerance = 1.0f / FRGBA16SNORM_MAX;
        TEST_EXPECT(Math::Abs(Decoded.X - Direction.X) <= Tolerance);
        TEST_EXPECT(Math::Abs(Decoded.Y - Direction.Y) <= Tolerance);
        TEST_EXPECT(Math::Abs(Decoded.Z - Direction.Z) <= Tolerance);
    }

    TEST_SECTION("FRGBA16Snorm::operator== / operator!=");
    TEST_EXPECT(FRGBA16Snorm(0.5f, 0.5f, 0.5f, 0.5f) == FRGBA16Snorm(0.5f, 0.5f, 0.5f, 0.5f));
    TEST_EXPECT(FRGBA16Snorm(0.5f, 0.5f, 0.5f, 0.5f) != FRGBA16Snorm(0.5f, 0.5f, 0.5f, -0.5f));

    TEST_END();
}
