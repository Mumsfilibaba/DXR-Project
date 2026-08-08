#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/Templates/TypeTraits.h"
#include "RHI/RHICore.h"
#include "RHI/ShaderCompiler.h"

struct FShaderCompilationEnvironment
{
    void RequireShaderModel(EShaderModel InShaderModel)
    {
        if (InShaderModel > ShaderModel)
        {
            ShaderModel = InShaderModel;
        }
    }

    void SetDefine(const CHAR* Define, const CHAR* Value)
    {
        Defines.Emplace(Define, Value);
    }

    TArray<FShaderDefine> Defines;
    EShaderModel          ShaderModel = EShaderModel::SM_6_0;
};

struct FShaderPermutationDesc
{
    int32 PermutationID                      = 0;
    bool  bSupportsBindless                  = false;
    bool  bSupportsRayTracing                = false;
    bool  bSupportsShaderExecutionReordering = false;
};

class FShaderPermutationBool
{
public:
    using Type = bool;

    static constexpr int32 PermutationCount = 2;

    NODISCARD static constexpr int32 ToDimensionValueID(Type Value)
    {
        return Value ? 1 : 0;
    }

    NODISCARD static constexpr Type FromDimensionValueID(int32 ValueID)
    {
        return ValueID != 0;
    }

    NODISCARD static String ToDefineValue(Type Value)
    {
        return Value ? String("(1)") : String("(0)");
    }
};

template<typename InType, int32 InCount, int32 InFirstValue = 0>
class TShaderPermutationInt
{
public:
    using Type = InType;

    static constexpr int32 PermutationCount = InCount;
    static constexpr int32 FirstValue       = InFirstValue;

    static_assert(InCount > 0, "A permutation dimension needs at least one value");

    NODISCARD static constexpr int32 ToDimensionValueID(Type Value)
    {
        return static_cast<int32>(Value) - FirstValue;
    }

    NODISCARD static constexpr Type FromDimensionValueID(int32 ValueID)
    {
        return static_cast<Type>(ValueID + FirstValue);
    }

    NODISCARD static String ToDefineValue(Type Value)
    {
        return String::CreateFormatted("%d", static_cast<int32>(Value));
    }
};

template<typename InEnumType>
class TShaderPermutationEnum
{
public:
    using Type = InEnumType;

    static constexpr int32 PermutationCount = static_cast<int32>(InEnumType::Count);

    static_assert(TIsEnum<InEnumType>::Value, "SHADER_PERMUTATION_ENUM needs an enum type");
    static_assert(PermutationCount > 0, "SHADER_PERMUTATION_ENUM needs the enum to declare a Count enumerator");

    NODISCARD static constexpr int32 ToDimensionValueID(Type Value)
    {
        return static_cast<int32>(Value);
    }

    NODISCARD static constexpr Type FromDimensionValueID(int32 ValueID)
    {
        return static_cast<Type>(ValueID);
    }

    NODISCARD static String ToDefineValue(Type Value)
    {
        return String::CreateFormatted("%d", static_cast<int32>(Value));
    }
};

#define SHADER_PERMUTATION_BOOL(InDefineName) \
    public FShaderPermutationBool             \
    { public: static constexpr const CHAR* DefineName = InDefineName; }

#define SHADER_PERMUTATION_INT(InDefineName, InCount)  \
    public TShaderPermutationInt<int32, InCount>       \
    { public: static constexpr const CHAR* DefineName = InDefineName; }

#define SHADER_PERMUTATION_RANGE_INT(InDefineName, InFirstValue, InCount)  \
    public TShaderPermutationInt<int32, InCount, InFirstValue>             \
    { public: static constexpr const CHAR* DefineName = InDefineName; }

#define SHADER_PERMUTATION_ENUM(InDefineName, InEnumType)  \
    public TShaderPermutationEnum<InEnumType>               \
    { public: static constexpr const CHAR* DefineName = InDefineName; }

#define SHADER_PERMUTATION_DEFINE(InDefineName) \
    public: static constexpr const CHAR* DefineName = InDefineName

template<typename... DimensionTypes>
class TShaderPermutation;

template<typename T>
struct TIsShaderPermutation : TFalseType { };

template<typename... DimensionTypes>
struct TIsShaderPermutation<TShaderPermutation<DimensionTypes...>> : TTrueType { };

template<typename Dimension, typename PermutationType>
struct TShaderPermutationDimensionCount
{
    static constexpr int32 Value = 0;
};

template<typename Dimension>
struct TShaderPermutationDimensionCount<Dimension, TShaderPermutation<>>
{
    static constexpr int32 Value = 0;
};

template<typename Dimension, typename HeadType, typename... TailTypes>
struct TShaderPermutationDimensionCount<Dimension, TShaderPermutation<HeadType, TailTypes...>>
{
private:
    static constexpr int32 HeadCount =
        TIsSame<Dimension, HeadType>::Value ? 1 :
        (TIsShaderPermutation<HeadType>::Value ? TShaderPermutationDimensionCount<Dimension, HeadType>::Value : 0);

public:
    static constexpr int32 Value = HeadCount + TShaderPermutationDimensionCount<Dimension, TShaderPermutation<TailTypes...>>::Value;
};

template<typename DimensionType>
concept CDimensionModifiesEnvironment = requires(typename DimensionType::Type Value, FShaderCompilationEnvironment& Environment)
{
    DimensionType::ModifyCompilationEnvironment(Value, Environment);
};

template<>
class TShaderPermutation<>
{
public:
    using Type = TShaderPermutation;

    static constexpr int32 PermutationCount = 1;

    TShaderPermutation() = default;

    explicit TShaderPermutation(int32 InPermutationID)
    {
        UNREFERENCED_VARIABLE(InPermutationID);
        CHECK(InPermutationID == 0);
    }

    NODISCARD static constexpr int32 ToDimensionValueID(const Type&)
    {
        return 0;
    }

    NODISCARD static constexpr Type FromDimensionValueID(int32)
    {
        return Type();
    }

    NODISCARD constexpr int32 GetPermutationID() const
    {
        return 0;
    }

    void ModifyCompilationEnvironment(FShaderCompilationEnvironment&) const
    {
    }

    NODISCARD bool operator==(const TShaderPermutation&) const
    {
        return true;
    }
};

template<typename DimensionType, typename... TailTypes>
class TShaderPermutation<DimensionType, TailTypes...>
{
public:
    using FTail = TShaderPermutation<TailTypes...>;
    using Type  = TShaderPermutation;

    static constexpr int32 PermutationCount = FTail::PermutationCount * DimensionType::PermutationCount;

    NODISCARD static constexpr int32 ToDimensionValueID(const Type& Value)
    {
        return Value.GetPermutationID();
    }

    NODISCARD static constexpr Type FromDimensionValueID(int32 ValueID)
    {
        return Type(ValueID);
    }

public:
    TShaderPermutation() = default;

    explicit TShaderPermutation(int32 InPermutationID)
        : DimensionValue(DimensionType::FromDimensionValueID(InPermutationID % DimensionType::PermutationCount))
        , Tail(InPermutationID / DimensionType::PermutationCount)
    {
        CHECK(InPermutationID >= 0 && InPermutationID < PermutationCount);
    }

    NODISCARD int32 GetPermutationID() const
    {
        return DimensionType::ToDimensionValueID(DimensionValue)
             + DimensionType::PermutationCount * Tail.GetPermutationID();
    }

    template<typename Dimension>
    void Set(typename Dimension::Type Value)
    {
        static_assert(TShaderPermutationDimensionCount<Dimension, TShaderPermutation>::Value == 1,
            "The dimension must occur exactly once in the permutation for Set to be unambiguous");

        if constexpr (TIsSame<Dimension, DimensionType>::Value)
        {
            DimensionValue = Value;
        }
        else if constexpr (TShaderPermutationDimensionCount<Dimension, DimensionType>::Value == 1)
        {
            DimensionValue.template Set<Dimension>(Value);
        }
        else
        {
            Tail.template Set<Dimension>(Value);
        }
    }

    template<typename Dimension>
    NODISCARD typename Dimension::Type Get() const
    {
        static_assert(TShaderPermutationDimensionCount<Dimension, TShaderPermutation>::Value == 1,
            "The dimension must occur exactly once in the permutation for Get to be unambiguous");

        if constexpr (TIsSame<Dimension, DimensionType>::Value)
        {
            return DimensionValue;
        }
        else if constexpr (TShaderPermutationDimensionCount<Dimension, DimensionType>::Value == 1)
        {
            return DimensionValue.template Get<Dimension>();
        }
        else
        {
            return Tail.template Get<Dimension>();
        }
    }

    void ModifyCompilationEnvironment(FShaderCompilationEnvironment& OutEnvironment) const
    {
        if constexpr (TIsShaderPermutation<DimensionType>::Value)
        {
            DimensionValue.ModifyCompilationEnvironment(OutEnvironment);
        }
        else
        {
            OutEnvironment.Defines.Emplace(DimensionType::DefineName, DimensionType::ToDefineValue(DimensionValue));

            if constexpr (CDimensionModifiesEnvironment<DimensionType>)
            {
                DimensionType::ModifyCompilationEnvironment(DimensionValue, OutEnvironment);
            }
        }

        Tail.ModifyCompilationEnvironment(OutEnvironment);
    }

    NODISCARD bool operator==(const TShaderPermutation& Other) const
    {
        return DimensionValue == Other.DimensionValue && Tail == Other.Tail;
    }

private:
    typename DimensionType::Type DimensionValue = { };
    FTail                        Tail;
};
