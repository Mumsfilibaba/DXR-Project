#include "ShaderPermutationTests.h"

#include <RendererCore/Shaders/ShaderPermutation.h>

#include "TestCommon/TestMacros.h"

namespace
{
    class FTestBoolA : SHADER_PERMUTATION_BOOL("TEST_BOOL_A");
    class FTestBoolB : SHADER_PERMUTATION_BOOL("TEST_BOOL_B");
    class FTestCount : SHADER_PERMUTATION_INT("TEST_COUNT", 3);
    class FTestRange : SHADER_PERMUTATION_RANGE_INT("TEST_RANGE", 2, 4);

    enum class ETestMode : uint8
    {
        First  = 0,
        Second = 1,
        Third  = 2,
        Count,
    };

    class FTestMode : SHADER_PERMUTATION_ENUM("TEST_MODE", ETestMode);

    /** @brief A dimension that raises the shader model itself, the FBindless shape. */
    class FTestRaisesModel : public FShaderPermutationBool
    {
        SHADER_PERMUTATION_DEFINE("TEST_RAISES_MODEL");

    public:
        static void ModifyCompilationEnvironment(Type Value, FShaderCompilationEnvironment& Environment)
        {
            if (Value)
            {
                Environment.RequireShaderModel(EShaderModel::SM_6_6);
            }
        }
    };

    using FInnerPermutation = TShaderPermutation<FTestBoolA, FTestBoolB>;
    using FOuterPermutation = TShaderPermutation<FInnerPermutation, FTestCount, FTestRaisesModel>;

    const FShaderDefine* FindDefine(const FShaderCompilationEnvironment& Environment, const CHAR* Name)
    {
        for (const FShaderDefine& Define : Environment.Defines)
        {
            if (Define.Define == Name)
            {
                return &Define;
            }
        }

        return nullptr;
    }
}

bool ShaderPermutation_Test()
{
    TEST_BEGIN();

    TEST_SECTION("PermutationCount (product of the dimensions)");
    static_assert(TShaderPermutation<>::PermutationCount == 1);
    static_assert(TShaderPermutation<FTestBoolA>::PermutationCount == 2);
    static_assert(TShaderPermutation<FTestBoolA, FTestBoolB>::PermutationCount == 4);
    static_assert(TShaderPermutation<FTestBoolA, FTestCount>::PermutationCount == 6);
    static_assert(TShaderPermutation<FTestRange>::PermutationCount == 4);
    static_assert(TShaderPermutation<FTestMode>::PermutationCount == 3);
    static_assert(FOuterPermutation::PermutationCount == 4 * 3 * 2);
    TEST_EXPECT(FOuterPermutation::PermutationCount == 24);

    TEST_SECTION("Encode / decode round-trip is bijective");
    {
        bool bAllIDsRoundTripped = true;
        for (int32 ID = 0; ID < FOuterPermutation::PermutationCount; ++ID)
        {
            const FOuterPermutation Permutation = FOuterPermutation(ID);
            if (Permutation.GetPermutationID() != ID)
            {
                bAllIDsRoundTripped = false;
            }
        }

        TEST_EXPECT(bAllIDsRoundTripped);
    }

    TEST_SECTION("A default permutation is ID zero");
    TEST_EXPECT(FOuterPermutation().GetPermutationID() == 0);

    TEST_SECTION("Set / Get on a flat permutation");
    {
        TShaderPermutation<FTestBoolA, FTestCount, FTestRange, FTestMode> Permutation;
        Permutation.Set<FTestBoolA>(true);
        Permutation.Set<FTestCount>(2);
        Permutation.Set<FTestRange>(4);
        Permutation.Set<FTestMode>(ETestMode::Third);

        TEST_EXPECT(Permutation.Get<FTestBoolA>() == true);
        TEST_EXPECT(Permutation.Get<FTestCount>() == 2);
        TEST_EXPECT(Permutation.Get<FTestRange>() == 4);
        TEST_EXPECT(Permutation.Get<FTestMode>() == ETestMode::Third);

        // A range dimension's first value maps onto value ID zero rather than onto the raw integer
        TShaderPermutation<FTestRange> RangeOnly;
        RangeOnly.Set<FTestRange>(2);
        TEST_EXPECT(RangeOnly.GetPermutationID() == 0);
        RangeOnly.Set<FTestRange>(5);
        TEST_EXPECT(RangeOnly.GetPermutationID() == 3);
    }

    TEST_SECTION("Set / Get search nested permutations recursively");
    {
        FOuterPermutation Permutation;
        Permutation.Set<FTestBoolB>(true);
        Permutation.Set<FTestCount>(1);

        TEST_EXPECT(Permutation.Get<FTestBoolA>() == false);
        TEST_EXPECT(Permutation.Get<FTestBoolB>() == true);
        TEST_EXPECT(Permutation.Get<FTestCount>() == 1);

        // The nested permutation is reachable as a whole as well as through its dimensions
        TEST_EXPECT(Permutation.Get<FInnerPermutation>().Get<FTestBoolB>() == true);

        FInnerPermutation Inner;
        Inner.Set<FTestBoolA>(true);
        Inner.Set<FTestBoolB>(false);
        Permutation.Set<FInnerPermutation>(Inner);

        TEST_EXPECT(Permutation.Get<FTestBoolA>() == true);
        TEST_EXPECT(Permutation.Get<FTestBoolB>() == false);
    }

    TEST_SECTION("A nested permutation round-trips through its outer ID");
    {
        FOuterPermutation Permutation;
        Permutation.Set<FTestBoolA>(true);
        Permutation.Set<FTestBoolB>(true);
        Permutation.Set<FTestCount>(2);
        Permutation.Set<FTestRaisesModel>(true);

        const FOuterPermutation Decoded = FOuterPermutation(Permutation.GetPermutationID());
        TEST_EXPECT(Decoded == Permutation);
        TEST_EXPECT(Decoded.Get<FTestBoolA>() == true);
        TEST_EXPECT(Decoded.Get<FTestBoolB>() == true);
        TEST_EXPECT(Decoded.Get<FTestCount>() == 2);
        TEST_EXPECT(Decoded.Get<FTestRaisesModel>() == true);
    }

    TEST_SECTION("ModifyCompilationEnvironment emits one define per leaf dimension");
    {
        FOuterPermutation Permutation;
        Permutation.Set<FTestBoolA>(true);
        Permutation.Set<FTestCount>(2);

        FShaderCompilationEnvironment Environment;
        Permutation.ModifyCompilationEnvironment(Environment);

        // A nested permutation contributes its children's defines rather than one of its own
        TEST_EXPECT(Environment.Defines.Size() == 4);
        TEST_EXPECT(FindDefine(Environment, "TEST_BOOL_A") != nullptr);
        TEST_EXPECT(FindDefine(Environment, "TEST_BOOL_B") != nullptr);
        TEST_EXPECT(FindDefine(Environment, "TEST_COUNT") != nullptr);
        TEST_EXPECT(FindDefine(Environment, "TEST_RAISES_MODEL") != nullptr);

        TEST_EXPECT(FindDefine(Environment, "TEST_BOOL_A")->Value == "(1)");
        TEST_EXPECT(FindDefine(Environment, "TEST_BOOL_B")->Value == "(0)");
        TEST_EXPECT(FindDefine(Environment, "TEST_COUNT")->Value == "2");
    }

    TEST_SECTION("A dimension may raise the shader model, and RequireShaderModel only raises");
    {
        FShaderCompilationEnvironment Environment;
        Environment.ShaderModel = EShaderModel::SM_6_2;

        FOuterPermutation Permutation;
        Permutation.Set<FTestRaisesModel>(false);
        Permutation.ModifyCompilationEnvironment(Environment);
        TEST_EXPECT(Environment.ShaderModel == EShaderModel::SM_6_2);

        Permutation.Set<FTestRaisesModel>(true);
        Permutation.ModifyCompilationEnvironment(Environment);
        TEST_EXPECT(Environment.ShaderModel == EShaderModel::SM_6_6);

        // Lowering is refused, so hooks compose regardless of the order they run in
        Environment.RequireShaderModel(EShaderModel::SM_6_0);
        TEST_EXPECT(Environment.ShaderModel == EShaderModel::SM_6_6);

        Environment.RequireShaderModel(EShaderModel::SM_6_9);
        TEST_EXPECT(Environment.ShaderModel == EShaderModel::SM_6_9);
    }

    TEST_SECTION("Dimension counting underpins the unambiguous-access assert");
    static_assert(TShaderPermutationDimensionCount<FTestBoolA, FOuterPermutation>::Value == 1);
    static_assert(TShaderPermutationDimensionCount<FInnerPermutation, FOuterPermutation>::Value == 1);
    static_assert(TShaderPermutationDimensionCount<FTestRange, FOuterPermutation>::Value == 0);
    static_assert(TIsShaderPermutation<FInnerPermutation>::Value);
    static_assert(!TIsShaderPermutation<FTestBoolA>::Value);
    TEST_EXPECT(true);

    TEST_END();
}
