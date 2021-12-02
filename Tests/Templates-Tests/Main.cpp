#include <Core/Templates/TypeTraits.h>

#include <Core/Containers/String.h>
#include <Core/Containers/UniquePtr.h>
#include <Core/Templates/IntegerSequence.h>

#include <iostream>
#include <type_traits>
#include <utility>

/* Helper classes etc. */

class CClass
{
public:
    void Func( int Num )
    {
        std::cout << "CClass: " << Num << std::endl;
    }

    int Member = 6;
};

class CAnotherClass
{
public:
    CAnotherClass( const CAnotherClass& ) = delete;
    CAnotherClass( CAnotherClass&& ) = delete;
    CAnotherClass& operator=( const CAnotherClass& ) = delete;
    CAnotherClass& operator=( CAnotherClass&& ) = delete;

    operator CClass()
    {
        return Instance;
    }

private:
    CClass Instance;
};

class CPolyClassBase
{
public:
    virtual void Func( int Num )
    {
        std::cout << "CPolyClassBase: " << Num << std::endl;
    }
};

class CPolyClass final : public CPolyClassBase
{
};

class CNotTrivialCopy
{
public:
    CNotTrivialCopy( const CNotTrivialCopy& )
    {
    }
};

class CVirtualDestructor
{
public:
    virtual ~CVirtualDestructor() = default;
};

struct SStruct
{
    SStruct( int InX )
        : x( InX )
    {
    }

    SStruct( const SStruct& ) = default;
    SStruct& operator=( const SStruct& ) = default;

    int x;
};

struct SEmpty
{
};

struct SBitField
{
    int : 0;
};

struct SStatic
{
    static int StaticInt;
};

struct SInvokable
{
    void operator()( int Num )
    {
        std::cout << "SInvokable: " << Num << std::endl;
    }
};

struct SPOD
{
    int x;
};

union UUnion
{
};

enum class EEnumClass
{
};

enum EEnum
{
};

void Func( int Num )
{
    std::cout << Num << std::endl;
}

auto Func2( char ) -> int (*)()
{
    return nullptr;
}

/* Tests */

int main()
{
    /* Is Same */
    static_assert(TIsSame<int, int>::Value == true);
    static_assert(TIsSame<int, char>::Value == false);

    /* Is Array */
    static_assert(TIsArray<int>::Value == false);
    static_assert(TIsArray<int[]>::Value == true);
    static_assert(TIsArray<int[5]>::Value == true);

    /* Is Unbounded Array */
    static_assert(TIsUnboundedArray<int[]>::Value == true);
    static_assert(TIsUnboundedArray<int>::Value == false);

    /* Is Bounded Array */
    static_assert(TIsBoundedArray<int>::Value == false);
    static_assert(TIsBoundedArray<int[5]>::Value == true);

    /* Is Const */
    static_assert(TIsConst<int>::Value == false);
    static_assert(TIsConst<const int>::Value == true);

    /* Is Union */
    static_assert(TIsUnion<UUnion>::Value == true);
    static_assert(TIsUnion<CClass>::Value == false);
    static_assert(TIsUnion<SStruct>::Value == false);
    static_assert(TIsUnion<EEnum>::Value == false);
    static_assert(TIsUnion<EEnumClass>::Value == false);
    static_assert(TIsUnion<int>::Value == false);

    /* Remove CV */
    static_assert(TIsSame<int, typename TRemoveCV<int>::Type>::Value == true);
    static_assert(TIsSame<int, typename TRemoveCV<const int>::Type>::Value == true);
    static_assert(TIsSame<int, typename TRemoveCV<const volatile int>::Type>::Value == true);
    static_assert(TIsSame<int, typename TRemoveCV<volatile int>::Type>::Value == true);

    /* Remove Const */
    static_assert(TIsSame<int, typename TRemoveConst<int>::Type>::Value == true);
    static_assert(TIsSame<int, typename TRemoveConst<volatile int>::Type>::Value == false);
    static_assert(TIsSame<int, typename TRemoveConst<const int>::Type>::Value == true);

    /* Remove Volatile*/
    static_assert(TIsSame<int, typename TRemoveVolatile<int>::Type>::Value == true);
    static_assert(TIsSame<int, typename TRemoveVolatile<const int>::Type>::Value == false);
    static_assert(TIsSame<int, typename TRemoveVolatile<volatile int>::Type>::Value == true);

    /* Remove extent */
    static_assert(TIsSame<int, typename TRemoveExtent<int>::Type>::Value == true);
    static_assert(TIsSame<int, typename TRemoveExtent<const int>::Type>::Value == false);
    static_assert(TIsSame<int, typename TRemoveExtent<int[]>::Type>::Value == true);
    static_assert(TIsSame<int, typename TRemoveExtent<int[5]>::Type>::Value == true);

    /* Remove pointer */
    static_assert(TIsSame<int, typename TRemovePointer<int>::Type>::Value == true);
    static_assert(TIsSame<int, typename TRemovePointer<const int>::Type>::Value == false);
    static_assert(TIsSame<int, typename TRemovePointer<int*>::Type>::Value == true);
    static_assert(TIsSame<int, typename TRemovePointer<int* const>::Type>::Value == true);
    static_assert(TIsSame<int, typename TRemovePointer<int* volatile>::Type>::Value == true);
    static_assert(TIsSame<int, typename TRemovePointer<int* const volatile>::Type>::Value == true);

    /* Remove reference */
    static_assert(TIsSame<int, typename TRemoveReference<int>::Type>::Value == true);
    static_assert(TIsSame<int, typename TRemoveReference<const int>::Type>::Value == false);
    static_assert(TIsSame<int, typename TRemoveReference<int&>::Type>::Value == true);
    static_assert(TIsSame<int, typename TRemoveReference<int&&>::Type>::Value == true);

    /* Identity */
    static_assert(TIsSame<int, typename TIdentity<int>::Type>::Value == true);

    /* Add CV */
    static_assert(TIsSame<const volatile int, typename TAddCV<int>::Type>::Value == true);
    static_assert(TIsSame<int, typename TAddCV<int>::Type>::Value == false);

    /* Add Const */
    static_assert(TIsSame<const int, typename TAddConst<int>::Type>::Value == true);
    static_assert(TIsSame<int, typename TAddConst<int>::Type>::Value == false);

    /* Add Const */
    static_assert(TIsSame<volatile int, typename TAddVolatile<int>::Type>::Value == true);
    static_assert(TIsSame<int, typename TAddVolatile<int>::Type>::Value == false);

    /* Add Pointer */
    static_assert(TIsSame<int*, typename TAddPointer<int>::Type>::Value == true);
    static_assert(TIsSame<int, typename TAddPointer<int>::Type>::Value == false);

    /* Add LValue Reference */
    static_assert(TIsSame<int&, typename TAddLValueReference<int>::Type>::Value == true);
    static_assert(TIsSame<int, typename TAddLValueReference<int>::Type>::Value == false);

    /* Add RValue Reference */
    static_assert(TIsSame<int&&, typename TAddRValueReference<int>::Type>::Value == true);
    static_assert(TIsSame<int, typename TAddRValueReference<int>::Type>::Value == false);

    /* Add Reference */
    static_assert(TIsSame<int&, typename TAddReference<int>::LValue>::Value == true);
    static_assert(TIsSame<int, typename TAddReference<int>::LValue>::Value == false);
    static_assert(TIsSame<int&&, typename TAddReference<int>::RValue>::Value == true);
    static_assert(TIsSame<int, typename TAddReference<int>::RValue>::Value == false);

    /* Void */
    static_assert(TIsSame<void, typename TVoid<void>::Type>::Value == true);
    static_assert(TIsSame<void, typename TVoid<int>::Type>::Value == true);
    static_assert(TIsSame<void, typename TVoid<const int>::Type>::Value == true);

    /* Or */
    static_assert(TOr<TIsSame<float, int>, TIsSame<bool, int>, TIsSame<int, int>>::Value == true);
    static_assert(TOr<TIsSame<float, int>, TIsSame<bool, int>, TIsSame<void, int>>::Value == false);

    /* And */
    static_assert(TAnd<TIsSame<float, int>, TIsSame<bool, int>, TIsSame<int, int>>::Value == false);
    static_assert(TAnd<TIsSame<float, float>, TIsSame<int, int>, TIsSame<void, void>>::Value == true);

    /* Not */
    static_assert(TNot<TIsSame<float, int>>::Value == true);
    static_assert(TNot<TIsSame<float, float>>::Value == false);

    /* Is Volatile*/
    static_assert(TIsVolatile<int>::Value == false);
    static_assert(TIsVolatile<volatile int>::Value == true);

    /* Is Void*/
    static_assert(TIsVoid<int>::Value == false);
    static_assert(TIsVoid<const int>::Value == false);
    static_assert(TIsVoid<const volatile int>::Value == false);
    static_assert(TIsVoid<void>::Value == true);

    /* Is LValue Reference */
    static_assert(TIsLValueReference<int>::Value == false);
    static_assert(TIsLValueReference<const int&>::Value == true);
    static_assert(TIsLValueReference<int&>::Value == true);

    /* Is RValue Reference */
    static_assert(TIsRValueReference<int>::Value == false);
    static_assert(TIsRValueReference<const int&&>::Value == true);
    static_assert(TIsRValueReference<int&&>::Value == true);

    /* Is Reference */
    static_assert(TIsReference<int>::Value == false);
    static_assert(TIsReference<int&>::Value == true);
    static_assert(TIsReference<int&&>::Value == true);

    /* Is Polymorphic */
    static_assert(TIsPolymorphic<CClass>::Value == false);
    static_assert(TIsPolymorphic<CPolyClassBase>::Value == true);
    static_assert(TIsPolymorphic<CPolyClass>::Value == true);
    static_assert(TIsPolymorphic<CVirtualDestructor>::Value == true);

    /* Is Pointer */
    static_assert(TIsPointer<int>::Value == false);
    static_assert(TIsPointer<int*>::Value == true);

    /* Conditional */
    static_assert(TIsSame<float, typename TConditional<false, int, float>::Type>::Value == true);
    static_assert(TIsSame<float, typename TConditional<true, int, float>::Type>::Value == false);

    /* Decay */
    static_assert(TIsSame<int*, typename TDecay<int[10]>::Type>::Value == true);
    static_assert(TIsSame<int, typename TDecay<int&>::Type>::Value == true);

    /* Is Class */
    static_assert(TIsClass<CClass>::Value == true);
    static_assert(TIsClass<SStruct>::Value == true);
    static_assert(TIsClass<UUnion>::Value == false);
    static_assert(TIsClass<EEnum>::Value == false);
    static_assert(TIsClass<EEnumClass>::Value == false);
    static_assert(TIsClass<int>::Value == false);

    /* Is Integer */
    static_assert(TIsInteger<CClass>::Value == false);
    static_assert(TIsInteger<SStruct>::Value == false);
    static_assert(TIsInteger<UUnion>::Value == false);
    static_assert(TIsInteger<float>::Value == false);
    static_assert(TIsInteger<double>::Value == false);
    static_assert(TIsInteger<long double>::Value == false);
    static_assert(TIsInteger<char>::Value == true);
    static_assert(TIsInteger<int>::Value == true);
    static_assert(TIsInteger<const int>::Value == true);
    static_assert(TIsInteger<unsigned int>::Value == true);

    /* Is Floating point */
    static_assert(TIsFloatingPoint<CClass>::Value == false);
    static_assert(TIsFloatingPoint<SStruct>::Value == false);
    static_assert(TIsFloatingPoint<UUnion>::Value == false);
    static_assert(TIsFloatingPoint<float>::Value == true);
    static_assert(TIsFloatingPoint<const float>::Value == true);
    static_assert(TIsFloatingPoint<double>::Value == true);
    static_assert(TIsFloatingPoint<const double>::Value == true);
    static_assert(TIsFloatingPoint<long double>::Value == true);
    static_assert(TIsFloatingPoint<char>::Value == false);
    static_assert(TIsFloatingPoint<int>::Value == false);
    static_assert(TIsFloatingPoint<const int>::Value == false);
    static_assert(TIsFloatingPoint<long long>::Value == false);

    /* Invoke */
    Invoke( Func, 5 );

    CClass Instance;
    Invoke( &CClass::Func, Instance, 5 );
    std::cout << "CClass::Member=" << Invoke( &CClass::Member, Instance ) << std::endl;

    CPolyClassBase BasePolyInstance;
    CPolyClass PolyInstance;
    Invoke( &CPolyClassBase::Func, BasePolyInstance, 5 );
    Invoke( &CPolyClassBase::Func, PolyInstance, 6 );

    auto SomeLambda = []( int Num )
    {
        std::cout << "SomeLambda=" << Num << std::endl;
    };

    Invoke( SomeLambda, 20 );

    SInvokable InvokableInstance;
    Invoke( InvokableInstance, 15 );

    /* Is Assignable */
    static_assert(TIsAssignable<int, int>::Value == false);
    static_assert(TIsAssignable<int&, int>::Value == true);
    static_assert(TIsAssignable<int, double>::Value == false);
    static_assert(TIsAssignable<int&, double>::Value == true);
    static_assert(TIsAssignable<std::string, double>::Value == true);
    static_assert(TIsAssignable<SStruct&, const SStruct&>::Value == true);

    /* Is Base Of */
    static_assert(TIsBaseOf<CClass, CPolyClass>::Value == false);
    static_assert(TIsBaseOf<CPolyClass, CPolyClass>::Value == true);
    static_assert(TIsBaseOf<CPolyClassBase, CPolyClass>::Value == true);
    static_assert(TIsBaseOf<CPolyClass, CPolyClassBase>::Value == false);

    /* Is constructable */
    static_assert(TIsConstructible<SStruct, const SStruct&>::Value == true);
    static_assert(TIsConstructible<SStruct, int>::Value == true);

    /* Is convertible */
    static_assert(TIsConvertible<CPolyClass*, CPolyClassBase*>::Value == true);
    static_assert(TIsConvertible<CPolyClassBase*, CPolyClass*>::Value == false);
    static_assert(TIsConvertible<CPolyClassBase*, CClass*>::Value == false);
    static_assert(TIsConvertible<CAnotherClass, CClass>::Value == true);

    /* Is Copyable Constructable */
    static_assert(TIsCopyConstructable<CClass>::Value == true);
    static_assert(TIsCopyConstructable<CAnotherClass>::Value == false);

    /* Is Copyable Assignable */
    static_assert(TIsCopyAssignable<CClass>::Value == true);
    static_assert(TIsCopyAssignable<CAnotherClass>::Value == false);

    /* Is Empty */
    static_assert(TIsEmpty<SEmpty>::Value == true);
    static_assert(TIsEmpty<SStruct>::Value == false);
    static_assert(TIsEmpty<SStatic>::Value == true);
    static_assert(TIsEmpty<CVirtualDestructor>::Value == false);
    static_assert(TIsEmpty<UUnion>::Value == false);
    // static_assert(TIsEmpty<SBitField>::Value          == true); triggers the assert, but should be true according to: https://en.cppreference.com/w/cpp/types/is_empty
    // static_assert(std::is_empty<SBitField>::value     == true);

    /* Is Enum */
    static_assert(TIsEnum<CClass>::Value == false);
    static_assert(TIsEnum<EEnum>::Value == true);
    static_assert(TIsEnum<int>::Value == false);
    static_assert(TIsEnum<EEnumClass>::Value == true);

    /* Is Final */
    static_assert(TIsFinal<CClass>::Value == false);
    static_assert(TIsFinal<CPolyClass>::Value == true);

    /* Is Function */
    static_assert(TIsFunction<CClass>::Value == false);
    static_assert(TIsFunction<int( int )>::Value == true);
    static_assert(TIsFunction<decltype(Func)>::Value == true);
    static_assert(TIsFunction<int>::Value == false);
    static_assert(TIsFunction<typename TMemberPointerTraits<decltype(&CClass::Func)>::Type>::Value == true);

    /* Is Fundamental */
    static_assert(TIsFundamental<CClass>::Value == false);
    static_assert(TIsFundamental<int>::Value == true);
    static_assert(TIsFundamental<int&>::Value == false);
    static_assert(TIsFundamental<int*>::Value == false);
    static_assert(TIsFundamental<float>::Value == true);
    static_assert(TIsFundamental<float&>::Value == false);
    static_assert(TIsFundamental<float*>::Value == false);

    /* Is Compound */
    static_assert(TIsCompound<CClass>::Value == true);
    static_assert(TIsCompound<int>::Value == false);

    /* Is Invokable */
    static_assert(TIsInvokable<int()>::Value == true);
    static_assert(TIsInvokable<int(), int>::Value == false);
    static_assert(TIsInvokableR<int(), int>::Value == true);
    static_assert(TIsInvokableR<int(), int*>::Value == false);
    static_assert(TIsInvokableR<void( int ), void, int>::Value == true);
    static_assert(TIsInvokableR<void( int ), void, void>::Value == false);
    static_assert(TIsInvokableR<decltype(Func2), int(*)(), char>::Value == true);
    static_assert(TIsInvokableR<decltype(Func2), int(*)(), void>::Value == false);

    /* Is Memberpointer */
    static_assert(TIsMemberPointer<int CClass::*>::Value == true);
    static_assert(TIsMemberPointer<int>::Value == false);

    /* Is Movable Constructable */
    static_assert(TIsMoveConstructable<CClass>::Value == true);
    static_assert(TIsMoveConstructable<CAnotherClass>::Value == false);

    /* Is Movable Assignable */
    static_assert(TIsMoveAssignable<CClass>::Value == true);
    static_assert(TIsMoveAssignable<CAnotherClass>::Value == false);

    /* Is Plain Old Data */
    static_assert(TIsPlainOldData<SPOD>::Value == true);
    static_assert(TIsPlainOldData<CAnotherClass>::Value == false);
    static_assert(TIsPlainOldData<CPolyClass>::Value == false);

    /* Is Scalar */
    static_assert(TIsScalar<int>::Value == true);
    static_assert(TIsScalar<CClass>::Value == false);

    /* Is Object */
    static_assert(TIsObject<int>::Value == true);
    static_assert(TIsObject<int&>::Value == false);
    static_assert(TIsObject<CClass>::Value == true);
    static_assert(TIsObject<CClass&>::Value == false);

    /* Is Nullptr */
    static_assert(TIsNullptr<decltype(nullptr)>::Value == true);
    static_assert(TIsNullptr<int*>::Value == false);

    /* Is Signed */
    static_assert(TIsSigned<CClass>::Value == false);
    static_assert(TIsSigned<float>::Value == true);
    static_assert(TIsSigned<signed int>::Value == true);
    static_assert(TIsSigned<unsigned int>::Value == false);
    static_assert(TIsSigned<EEnum>::Value == false);
    static_assert(TIsSigned<EEnumClass>::Value == false);

    /* Is Unsigned */
    static_assert(TIsUnsigned<CClass>::Value == false);
    static_assert(TIsUnsigned<float>::Value == false);
    static_assert(TIsUnsigned<signed int>::Value == false);
    static_assert(TIsUnsigned<unsigned int>::Value == true);
    static_assert(TIsUnsigned<EEnum>::Value == false);
    static_assert(TIsUnsigned<EEnumClass>::Value == false);

    /* Is Trivially Copyable */
    static_assert(TIsTriviallyCopyable<CClass>::Value == true);
    static_assert(TIsTriviallyCopyable<CNotTrivialCopy>::Value == false);
    static_assert(TIsTriviallyCopyable<CPolyClass>::Value == false);
    static_assert(TIsTriviallyCopyable<SStruct>::Value == true);

    /* Is Trivially Constructable */
    static_assert(TIsTriviallyConstructable<CClass, const CClass&>::Value == true);
    static_assert(TIsTriviallyConstructable<CClass, int>::Value == false);

    /* Is Trivially Destructable */
    static_assert(TIsTriviallyDestructable<CVirtualDestructor>::Value == false);
    static_assert(TIsTriviallyDestructable<SPOD>::Value == true);

    /* Is Trivial */
    static_assert(TIsTrivial<SPOD>::Value == true);
    static_assert(TIsTrivial<CClass>::Value == false);

    /* Integer Sequence */
    TIntegerSequence Sequence = TMakeIntegerSequence<uint32, 5>();
    Sequence.Size;

    return 0;
}