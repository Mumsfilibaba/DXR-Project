#include "Function_Test.h"

#if RUN_TFUNCTION_TEST
#include <Core/Containers/Function.h>

#include <iostream>

/*
 * A
 */

struct A
{
    bool Func( int32 In )
    {
        std::cout << "MemberCall " << In << std::endl;
        return true;
    }

    bool ConstFunc( int32 In ) const
    {
        std::cout << "Const MemberCall " << In << std::endl;
        return true;
    }

    bool Func2( int32 In )
    {
        std::cout << "MemberCall2 " << In << std::endl;
        return true;
    }
};

struct CFirst
{
    virtual void Func( int32 In ) = 0;
};

struct CSecond : public CFirst
{
    virtual void Func( int32 In )
    {
        std::cout << "Virtual MemberCall " << In << std::endl;
    }
};

struct CThird
{
    virtual void SecondFunc( int32 In ) = 0;
};

struct CFourth : public CThird, public CSecond
{
    virtual void SecondFunc( int32 In )
    {
        std::cout << "Second Virtual MemberCall " << In << std::endl;
    }
};

static bool Func( int32 In )
{
    std::cout << "FunctionCall " << In << std::endl;
    return true;
}

static bool Func2( int32 In )
{
    std::cout << "FunctionCall2 " << In << std::endl;
    return true;
}

/* Tuple func */
int32 TupleFunc( int32 Num0, int32 Num1, int32 Num2, int32 Num3 )
{
    std::cout << "Tuple func Num0=" << Num0 << ", Num1=" << Num1 << ", Num2=" << Num2 << ", Num3=" << Num3 << std::endl;
    return Num0 + 1;
}

/*
 * Test
 */

void TFunction_Test()
{
    std::cout << std::endl << "-------TMemberFunction and TConstMemberFunction-------" << std::endl << std::endl;
    std::cout << "Testing constructor and Invoke" << std::endl;

    A a;
    auto A_Func = Bind( &A::Func, &a );
    A_Func( 32 );

    auto A_ConstFunc = Bind( &A::ConstFunc, &a );
    A_ConstFunc( 32 );

    std::cout << "Testing virtual functions" << std::endl;
    CSecond Second;

    auto SecondFunc = Bind( &CSecond::Func, &Second );
    SecondFunc( 42 );

    CFourth Fourth;

    auto FourthFunc = Bind( &CFourth::Func, &Fourth );
    FourthFunc( 42 );

    TFunction<void(int32)> FourthFuncWrapper = Bind( &CFourth::Func, &Fourth );
    FourthFuncWrapper( 555 );

    std::cout << std::endl << "----------TFunction----------" << std::endl << std::endl;
    std::cout << "Testing constructors" << std::endl;

    struct Functor
    {
        bool operator()( int32 In ) const
        {
            std::cout << "Functor " << In << std::endl;
            return true;
        }
    } Fun;

    TFunction<bool( int32 )> NormalFunc = Func;
    NormalFunc( 5 );

    TFunction<bool( int32 )> MemberFunc = Bind( &A::Func, &a );
    MemberFunc( 10 );

    TFunction<bool( int32 )> MemberFunc1 = Bind( &A::ConstFunc, &a );
    MemberFunc1( 666 );

    TFunction<bool( int32 )> FunctorFunc = Fun;
    FunctorFunc( 15 );

    int64 x = 50;
    int64 y = 150;
    int64 z = 250;
    TFunction<bool( int32 )> LambdaFunc = [=]( int32 Input ) -> bool
    {
        std::cout << "Lambda (x=" << x << ", y=" << y << ", z=" << z << ") =" << Input << std::endl;
        return true;
    };
    LambdaFunc( 20 );

    A a1;
    TFunction<bool( int32 )> LambdaMemberFunc = [&]( int32 Input ) -> bool
    {
        std::cout << "--Lambda Begin--" << std::endl;
        a1.Func( Input );
        a1.Func2( Input );
        std::cout << "--Lambda End--";
        return true;
    };
    LambdaMemberFunc( 20 );

    std::cout << std::endl << "-------Test copy constructor-------" << std::endl << std::endl;

    TFunction<bool( int32 )> CopyFunc( MemberFunc );
    CopyFunc( 30 );
    MemberFunc( 40 );

    std::cout << std::endl << "-------Test Move constructor-------" << std::endl << std::endl;
    TFunction<bool( int32 )> MoveFunc( Move( LambdaFunc ) );
    MoveFunc( 50 );
    if ( LambdaFunc )
    {
        LambdaFunc( 60 );
    }

    std::cout << std::endl << "-------Test Assign-------" << std::endl << std::endl;
    NormalFunc.Assign( Func2 );
    MemberFunc.Assign( Bind( &A::Func2, &a ) );

    NormalFunc( 70 );
    MemberFunc( 80 );

    std::cout << std::endl << "-------Test Swap-------" << std::endl << std::endl;
    NormalFunc.Swap( MemberFunc );

    NormalFunc( 90 );
    MemberFunc( 100 );

    std::cout << std::endl << "-------Test IsValid-------" << std::endl << std::endl;
    std::cout << "NormalFunc=" << std::boolalpha << NormalFunc.IsValid() << std::endl;

    TFunction<void(int)> EmptyFunc;
    std::cout << "EmptyFunc=" << std::boolalpha << EmptyFunc.IsValid() << std::endl;

    std::cout << std::endl << "-------Test Bind-------" << std::endl << std::endl;
    int32 Num0 = 50;
    int32 Num1 = 100;

    TFunction<int(int, int )> Payload = Bind( TupleFunc, Num0, Num1 );
    Payload(150, 200);

    auto Payload2 = Bind( Func, 42 );
    Payload2();

    // Lambda
    auto Lambda = [=]( int32 Num ) -> int32
    {
        std::cout << "Lambda (x=" << x << ", y=" << y << ", z=" << z << ") =" << Num << std::endl;
        return Num + 1;
    };

    auto Payload3 = Bind( Lambda, 42 );
    Payload3();

    auto Payload4 = Bind( &CSecond::Func, &Second, 42 );
    Payload4();

    Invoke( &CSecond::Func, &Second, 42 );

    auto Payload5 = Bind( Func );
    Payload5( 42 );

    auto Payload6 = Bind( &CSecond::Func );
    Payload6( &Second, 42 );
}

#endif
