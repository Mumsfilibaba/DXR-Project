#include <Core/Containers/String.h>
#include <Core/Containers/UniquePtr.h>
#include <Core/Templates/TypeTraits.h>

#include <iostream>
#include <type_traits>
#include <utility>

/* Helper classes etc. */

struct FClass
{
    void Func(int32 Num)
    {
        std::cout << "FClass: " << Num << std::endl;
    }

    int32 Member = 6;
};

struct FAnotherClass : public FNonCopyAndNonMovable 
{
public:
    operator FClass()
    {
        return Instance;
    }

private:
    FClass Instance;
};

struct FPolyClassBase
{
    virtual void Func(int Num)
    {
        std::cout << "FPolyClassBase: " << Num << std::endl;
    }
};

class FPolyClass final : public FPolyClassBase
{
};

struct FNotTrivialCopy
{
    FNotTrivialCopy(const FNotTrivialCopy&) { }
};

struct FVirtualDestructor
{
    virtual ~FVirtualDestructor() = default;
};

struct FStruct
{
    FStruct(int InX)
        : x(InX)
    { }

    FStruct(const FStruct&) = default;
    FStruct& operator=(const FStruct&) = default;

    int x;
};

struct FEmpty
{
};

struct FBitField
{
    int32 : 0;
};

struct FStatic
{
    static int32 StaticInt;
};

struct FInvokable
{
    void operator()(int32 Num)
    {
        std::cout << "SInvokable: " << Num << std::endl;
    }
};

struct FPOD
{
    int x;
};

union FUnion
{
};

enum class EEnumClass
{
};

enum EEnum
{
};

void Func(int32 Num)
{
    std::cout << Num << std::endl;
}

auto Func2(CHAR) -> int(*)()
{
    return nullptr;
}

/* Tests */

int main()
{
    /* Is Same */
    static_assert(TIsSame<int, int>::Value  == true);
    static_assert(TIsSame<int, CHAR>::Value == false);

    /* Is Array */
    static_assert(TIsArray<int>::Value    == false);
    static_assert(TIsArray<int[]>::Value  == true);
    static_assert(TIsArray<int[5]>::Value == true);

    /* Is Unbounded Array */
    static_assert(TIsUnboundedArray<int[]>::Value == true);
    static_assert(TIsUnboundedArray<int>::Value   == false);

    /* Is Bounded Array */
    static_assert(TIsBoundedArray<int>::Value    == false);
    static_assert(TIsBoundedArray<int[5]>::Value == true);

    /* Is Const */
    static_assert(TIsConst<int>::Value       == false);
    static_assert(TIsConst<const int>::Value == true);

    /* Is Union */
    static_assert(TIsUnion<FUnion>::Value     == true);
    static_assert(TIsUnion<FClass>::Value     == false);
    static_assert(TIsUnion<FStruct>::Value    == false);
    static_assert(TIsUnion<EEnum>::Value      == false);
    static_assert(TIsUnion<EEnumClass>::Value == false);
    static_assert(TIsUnion<int>::Value        == false);

    /* Remove CV */
    static_assert(TIsSame<int, typename TRemoveCV<int>::Type>::Value                == true);
    static_assert(TIsSame<int, typename TRemoveCV<const int>::Type>::Value          == true);
    static_assert(TIsSame<int, typename TRemoveCV<const volatile int>::Type>::Value == true);
    static_assert(TIsSame<int, typename TRemoveCV<volatile int>::Type>::Value       == true);

    /* Remove Const */
    static_assert(TIsSame<int, typename TRemoveConst<int>::Type>::Value          == true);
    static_assert(TIsSame<int, typename TRemoveConst<volatile int>::Type>::Value == false);
    static_assert(TIsSame<int, typename TRemoveConst<const int>::Type>::Value    == true);

    /* Remove Volatile*/
    static_assert(TIsSame<int, typename TRemoveVolatile<int>::Type>::Value          == true);
    static_assert(TIsSame<int, typename TRemoveVolatile<const int>::Type>::Value    == false);
    static_assert(TIsSame<int, typename TRemoveVolatile<volatile int>::Type>::Value == true);

    /* Remove extent */
    static_assert(TIsSame<int, typename TRemoveExtent<int>::Type>::Value       == true);
    static_assert(TIsSame<int, typename TRemoveExtent<const int>::Type>::Value == false);
    static_assert(TIsSame<int, typename TRemoveExtent<int[]>::Type>::Value     == true);
    static_assert(TIsSame<int, typename TRemoveExtent<int[5]>::Type>::Value    == true);

    /* Remove pointer */
    static_assert(TIsSame<int, typename TRemovePointer<int>::Type>::Value                 == true);
    static_assert(TIsSame<int, typename TRemovePointer<const int>::Type>::Value           == false);
    static_assert(TIsSame<int, typename TRemovePointer<int*>::Type>::Value                == true);
    static_assert(TIsSame<int, typename TRemovePointer<int* const>::Type>::Value          == true);
    static_assert(TIsSame<int, typename TRemovePointer<int* volatile>::Type>::Value       == true);
    static_assert(TIsSame<int, typename TRemovePointer<int* const volatile>::Type>::Value == true);

    /* Remove reference */
    static_assert(TIsSame<int, typename TRemoveReference<int>::Type>::Value       == true);
    static_assert(TIsSame<int, typename TRemoveReference<const int>::Type>::Value == false);
    static_assert(TIsSame<int, typename TRemoveReference<int&>::Type>::Value      == true);
    static_assert(TIsSame<int, typename TRemoveReference<int&&>::Type>::Value     == true);

    /* Identity */
    static_assert(TIsSame<int, typename TIdentity<int>::Type>::Value == true);

    /* Add CV */
    static_assert(TIsSame<const volatile int, typename TAddCV<int>::Type>::Value == true);
    static_assert(TIsSame<int,                typename TAddCV<int>::Type>::Value == false);

    /* Add Const */
    static_assert(TIsSame<const int, typename TAddConst<int>::Type>::Value == true);
    static_assert(TIsSame<int,       typename TAddConst<int>::Type>::Value == false);

    /* Add Const */
    static_assert(TIsSame<volatile int, typename TAddVolatile<int>::Type>::Value == true);
    static_assert(TIsSame<int,          typename TAddVolatile<int>::Type>::Value == false);

    /* Add Pointer */
    static_assert(TIsSame<int*, typename TAddPointer<int>::Type>::Value == true);
    static_assert(TIsSame<int,  typename TAddPointer<int>::Type>::Value == false);

    /* Add LValue Reference */
    static_assert(TIsSame<int&,       typename TAddLValueReference<int>::Type>::Value == true);
    static_assert(TIsSame<int,        typename TAddLValueReference<int>::Type>::Value == false);
	static_assert(TIsSame<const int&, typename TAddLValueReference<typename TAddConst<int>::Type>::Type>::Value == true);

    /* Add RValue Reference */
    static_assert(TIsSame<int&&, typename TAddRValueReference<int>::Type>::Value == true);
    static_assert(TIsSame<int,   typename TAddRValueReference<int>::Type>::Value == false);

    /* Add Reference */
    static_assert(TIsSame<int&,  typename TAddReference<int>::LValue>::Value == true);
    static_assert(TIsSame<int,   typename TAddReference<int>::LValue>::Value == false);
    static_assert(TIsSame<int&&, typename TAddReference<int>::RValue>::Value == true);
    static_assert(TIsSame<int,   typename TAddReference<int>::RValue>::Value == false);

    /* Void */
    static_assert(TIsSame<void, typename TVoid<void>::Type>::Value      == true);
    static_assert(TIsSame<void, typename TVoid<int>::Type>::Value       == true);
    static_assert(TIsSame<void, typename TVoid<const int>::Type>::Value == true);

    /* Or */
    static_assert(TOr<TIsSame<float, int>, TIsSame<bool, int>, TIsSame<int, int>>::Value  == true);
    static_assert(TOr<TIsSame<float, int>, TIsSame<bool, int>, TIsSame<void, int>>::Value == false);

    /* And */
    static_assert(TAnd<TIsSame<float, int>,   TIsSame<bool, int>, TIsSame<int, int>>::Value   == false);
    static_assert(TAnd<TIsSame<float, float>, TIsSame<int, int>,  TIsSame<void, void>>::Value == true);

    /* Not */
    static_assert(TNot<TIsSame<float, int>>::Value   == true);
    static_assert(TNot<TIsSame<float, float>>::Value == false);

    /* Is Volatile*/
    static_assert(TIsVolatile<int>::Value == false);
    static_assert(TIsVolatile<volatile int>::Value == true);

    /* Is Void*/
    static_assert(TIsVoid<int>::Value                == false);
    static_assert(TIsVoid<const int>::Value          == false);
    static_assert(TIsVoid<const volatile int>::Value == false);
    static_assert(TIsVoid<void>::Value               == true);

    /* Is LValue Reference */
    static_assert(TIsLValueReference<int>::Value        == false);
    static_assert(TIsLValueReference<const int&>::Value == true);
    static_assert(TIsLValueReference<int&>::Value       == true);

    /* Is RValue Reference */
    static_assert(TIsRValueReference<int>::Value         == false);
    static_assert(TIsRValueReference<const int&&>::Value == true);
    static_assert(TIsRValueReference<int&&>::Value       == true);

    /* Is Reference */
    static_assert(TIsReference<int>::Value   == false);
    static_assert(TIsReference<int&>::Value  == true);
    static_assert(TIsReference<int&&>::Value == true);

    /* Is Polymorphic */
    static_assert(TIsPolymorphic<FClass>::Value             == false);
    static_assert(TIsPolymorphic<FPolyClassBase>::Value     == true);
    static_assert(TIsPolymorphic<FPolyClass>::Value         == true);
    static_assert(TIsPolymorphic<FVirtualDestructor>::Value == true);

    /* Is Pointer */
    static_assert(TIsPointer<int>::Value  == false);
    static_assert(TIsPointer<int*>::Value == true);

    /* Conditional */
    static_assert(TIsSame<float, typename TConditional<false, int, float>::Type>::Value == true);
    static_assert(TIsSame<float, typename TConditional<true, int, float>::Type>::Value  == false);

    /* Decay */
    static_assert(TIsSame<int*, typename TDecay<int[10]>::Type>::Value == true);
    static_assert(TIsSame<int, typename TDecay<int&>::Type>::Value     == true);

    /* Is Class */
    static_assert(TIsClass<FClass>::Value     == true);
    static_assert(TIsClass<FStruct>::Value    == true);
    static_assert(TIsClass<FUnion>::Value     == false);
    static_assert(TIsClass<EEnum>::Value      == false);
    static_assert(TIsClass<EEnumClass>::Value == false);
    static_assert(TIsClass<int>::Value        == false);

    /* Is Integer */
    static_assert(TIsInteger<FClass>::Value       == false);
    static_assert(TIsInteger<FStruct>::Value      == false);
    static_assert(TIsInteger<FUnion>::Value       == false);
    static_assert(TIsInteger<float>::Value        == false);
    static_assert(TIsInteger<double>::Value       == false);
    static_assert(TIsInteger<long double>::Value  == false);
    static_assert(TIsInteger<CHAR>::Value         == true);
    static_assert(TIsInteger<int>::Value          == true);
    static_assert(TIsInteger<const int>::Value    == true);
    static_assert(TIsInteger<unsigned int>::Value == true);

    /* Is Floating point */
    static_assert(TIsFloatingPoint<FClass>::Value       == false);
    static_assert(TIsFloatingPoint<FStruct>::Value      == false);
    static_assert(TIsFloatingPoint<FUnion>::Value       == false);
    static_assert(TIsFloatingPoint<float>::Value        == true);
    static_assert(TIsFloatingPoint<const float>::Value  == true);
    static_assert(TIsFloatingPoint<double>::Value       == true);
    static_assert(TIsFloatingPoint<const double>::Value == true);
    static_assert(TIsFloatingPoint<long double>::Value  == true);
    static_assert(TIsFloatingPoint<CHAR>::Value         == false);
    static_assert(TIsFloatingPoint<int>::Value          == false);
    static_assert(TIsFloatingPoint<const int>::Value    == false);
    static_assert(TIsFloatingPoint<long long>::Value    == false);

    /* Invoke */
    Invoke(Func, 5);

    FClass Instance;
    Invoke(&FClass::Func, Instance, 5);
    std::cout << "CClass::Member=" << Invoke(&FClass::Member, Instance) << std::endl;

    FPolyClassBase BasePolyInstance;
    FPolyClass     PolyInstance;
    Invoke(&FPolyClassBase::Func, BasePolyInstance, 5);
    Invoke(&FPolyClassBase::Func, PolyInstance, 6);

    auto SomeLambda = [](int Num)
    {
        std::cout << "SomeLambda=" << Num << std::endl;
    };

    Invoke(SomeLambda, 20);

    FInvokable InvokableInstance;
    Invoke(InvokableInstance, 15);

    /* Is Assignable */
    static_assert(TIsAssignable<int, int>::Value                 == false);
    static_assert(TIsAssignable<int&, int>::Value                == true);
    static_assert(TIsAssignable<int, double>::Value              == false);
    static_assert(TIsAssignable<int&, double>::Value             == true);
    static_assert(TIsAssignable<std::string, double>::Value      == true);
    static_assert(TIsAssignable<FStruct&, const FStruct&>::Value == true);

    /* Is Base Of */
    static_assert(TIsBaseOf<FClass, FPolyClass>::Value         == false);
    static_assert(TIsBaseOf<FPolyClass, FPolyClass>::Value     == true);
    static_assert(TIsBaseOf<FPolyClassBase, FPolyClass>::Value == true);
    static_assert(TIsBaseOf<FPolyClass, FPolyClassBase>::Value == false);

    /* Is constructible */
    static_assert(TIsConstructible<FStruct, const FStruct&>::Value == true);
    static_assert(TIsConstructible<FStruct, int>::Value            == true);

    /* Is convertible */
    static_assert(TIsConvertible<FPolyClass*, FPolyClassBase*>::Value == true);
    static_assert(TIsConvertible<FPolyClassBase*, FPolyClass*>::Value == false);
    static_assert(TIsConvertible<FPolyClassBase*, FClass*>::Value     == false);
    static_assert(TIsConvertible<FAnotherClass, FClass>::Value        == true);

    /* Is Copyable Constructible */
    static_assert(TIsCopyConstructable<FClass>::Value        == true);
    static_assert(TIsCopyConstructable<FAnotherClass>::Value == false);

    /* Is Copyable Assignable */
    static_assert(TIsCopyAssignable<FClass>::Value        == true);
    static_assert(TIsCopyAssignable<FAnotherClass>::Value == false);

    /* Is Empty */
    static_assert(TIsEmpty<FEmpty>::Value             == true);
    static_assert(TIsEmpty<FStruct>::Value            == false);
    static_assert(TIsEmpty<FStatic>::Value            == true);
    static_assert(TIsEmpty<FVirtualDestructor>::Value == false);
    static_assert(TIsEmpty<FUnion>::Value             == false);
    // static_assert(TIsEmpty<FBitField>::Value          == true); // triggers the assert, but should be true according to: https://en.cppreference.com/w/cpp/types/is_empty
    // static_assert(std::is_empty<FBitField>::value     == true);

    /* Is Enum */
    static_assert(TIsEnum<FClass>::Value     == false);
    static_assert(TIsEnum<EEnum>::Value      == true);
    static_assert(TIsEnum<int>::Value        == false);
    static_assert(TIsEnum<EEnumClass>::Value == true);

    /* Is Final */
    static_assert(TIsFinal<FClass>::Value     == false);
    static_assert(TIsFinal<FPolyClass>::Value == true);

    /* Is Function */
    static_assert(TIsFunction<FClass>::Value         == false);
    static_assert(TIsFunction<int(int)>::Value       == true);
    static_assert(TIsFunction<decltype(Func)>::Value == true);
    static_assert(TIsFunction<int>::Value            == false);
    static_assert(TIsFunction<typename TMemberPointerTraits<decltype(&FClass::Func)>::Type>::Value == true);

    /* Is Fundamental */
    static_assert(TIsFundamental<FClass>::Value == false);
    static_assert(TIsFundamental<int>::Value    == true);
    static_assert(TIsFundamental<int&>::Value   == false);
    static_assert(TIsFundamental<int*>::Value   == false);
    static_assert(TIsFundamental<float>::Value  == true);
    static_assert(TIsFundamental<float&>::Value == false);
    static_assert(TIsFundamental<float*>::Value == false);

    /* Is Compound */
    static_assert(TIsCompound<FClass>::Value == true);
    static_assert(TIsCompound<int>::Value    == false);

    /* Is Invokable */
    static_assert(TIsInvokable<int()>::Value                            == true);
    static_assert(TIsInvokable<int(), int>::Value                       == false);
    static_assert(TIsInvokableR<int(), int>::Value                      == true);
    static_assert(TIsInvokableR<int(), int*>::Value                     == false);
    static_assert(TIsInvokableR<void(int), void, int>::Value            == true);
    static_assert(TIsInvokableR<void(int), void, void>::Value           == false);
    static_assert(TIsInvokableR<decltype(Func2), int(*)(), CHAR>::Value == true);
    static_assert(TIsInvokableR<decltype(Func2), int(*)(), void>::Value == false);

    /* Is Member-pointer */
    static_assert(TIsMemberPointer<int FClass::*>::Value == true);
    static_assert(TIsMemberPointer<int>::Value           == false);

    /* Is Movable Constructible */
    static_assert(TIsMoveConstructable<FClass>::Value        == true);
    static_assert(TIsMoveConstructable<FAnotherClass>::Value == false);

    /* Is Movable Assignable */
    static_assert(TIsMoveAssignable<FClass>::Value        == true);
    static_assert(TIsMoveAssignable<FAnotherClass>::Value == false);

    /* Is Plain Old Data */
    static_assert(TIsPOD<FPOD>::Value          == true);
    static_assert(TIsPOD<FAnotherClass>::Value == false);
    static_assert(TIsPOD<FPolyClass>::Value    == false);

    /* Is Scalar */
    static_assert(TIsScalar<int>::Value    == true);
    static_assert(TIsScalar<FClass>::Value == false);

    /* Is Object */
    static_assert(TIsObject<int>::Value     == true);
    static_assert(TIsObject<int&>::Value    == false);
    static_assert(TIsObject<FClass>::Value  == true);
    static_assert(TIsObject<FClass&>::Value == false);

    /* Is Nullptr */
    static_assert(TIsNullptr<decltype(nullptr)>::Value == true);
    static_assert(TIsNullptr<int*>::Value              == false);

    /* Is Signed */
    static_assert(TIsSigned<FClass>::Value       == false);
    static_assert(TIsSigned<float>::Value        == true);
    static_assert(TIsSigned<signed int>::Value   == true);
    static_assert(TIsSigned<unsigned int>::Value == false);
    static_assert(TIsSigned<EEnum>::Value        == false);
    static_assert(TIsSigned<EEnumClass>::Value   == false);

    /* Is Unsigned */
    static_assert(TIsUnsigned<FClass>::Value       == false);
    static_assert(TIsUnsigned<float>::Value        == false);
    static_assert(TIsUnsigned<signed int>::Value   == false);
    static_assert(TIsUnsigned<unsigned int>::Value == true);
    static_assert(TIsUnsigned<EEnum>::Value        == false);
    static_assert(TIsUnsigned<EEnumClass>::Value   == false);

    /* Is Trivially Copyable */
    static_assert(TIsTriviallyCopyable<FClass>::Value          == true);
    static_assert(TIsTriviallyCopyable<FNotTrivialCopy>::Value == false);
    static_assert(TIsTriviallyCopyable<FPolyClass>::Value      == false);
    static_assert(TIsTriviallyCopyable<FStruct>::Value         == true);

    /* Is Trivially Constructible */
    static_assert(TIsTriviallyConstructable<FClass, const FClass&>::Value == true);
    static_assert(TIsTriviallyConstructable<FClass, int>::Value           == true);

    /* Is Trivially Destructible */
    static_assert(TIsTriviallyDestructable<FVirtualDestructor>::Value == false);
    static_assert(TIsTriviallyDestructable<FPOD>::Value               == true);

    /* Is Trivial */
    static_assert(TIsTrivial<FPOD>::Value   == true);
    static_assert(TIsTrivial<FClass>::Value == false);

    /* Integer Sequence */
    TIntegerSequence Sequence = TMakeIntegerSequence<uint32, 5>();
    static_assert(Sequence.Size == 5);

    return 0;
}