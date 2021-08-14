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

/*
 * Test
 */

void TFunction_Test()
{
    std::cout << std::endl << "-------TMemberFunction and TConstMemberFunction-------" << std::endl << std::endl;
    std::cout << "Testing constructor and Invoke" << std::endl;

    A a;
    TMemberFunction A_Func = TMemberFunction( &a, &A::Func );
    A_Func( 32 );

    TConstMemberFunction A_ConstFunc = TConstMemberFunction( &a, &A::ConstFunc );
    A_ConstFunc( 32 );

    std::cout << "Testing virtual functions" << std::endl;
    CSecond Second;

    TMemberFunction SecondFunc = TMemberFunction( &Second, &CSecond::Func );
    SecondFunc( 42 );

    CFourth Fourth;

    TMemberFunction FourthFunc = TMemberFunction( &Fourth, &CFourth::Func );
    FourthFunc( 42 );

    TFunction<void(int32)> FourthFuncWrapper = Bind( &Fourth, &CFourth::Func );
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

    TFunction<bool( int32 )> MemberFunc = Bind( &a, &A::Func );
    MemberFunc( 10 );

    TFunction<bool( int32 )> MemberFunc1 = Bind( &a, &A::ConstFunc );
    MemberFunc1( 666 );

    TFunction<bool( int32 )> FunctorFunc = Fun;
    FunctorFunc( 15 );

    TFunction<bool( int32 )> LambdaFunc = []( int32 Input ) -> bool
    {
        std::cout << "Lambda " << Input << std::endl;
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
    MemberFunc.Assign( Bind( &a, &A::Func2 ) );

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
}

#endif
