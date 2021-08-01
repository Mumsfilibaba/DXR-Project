#include "TFunction_Test.h"

#include <Core/Containers/Function.h>

#include <iostream>

/*
 * A
 */

struct A
{
    bool Func(int32 In)
    {
        std::cout << "MemberCall " << In << std::endl;
        return true;
    }
    
    bool ConstFunc(int32 In) const
    {
        std::cout << "Const MemberCall " << In << std::endl;
        return true;
    }
    
    bool Func2(int32 In)
    {
        std::cout << "MemberCall2 " << In << std::endl;
        return true;
    }
};

static bool Func(int32 In)
{
    std::cout << "FunctionCall " << In << std::endl;
    return true;
}

static bool Func2(int32 In)
{
    std::cout << "FunctionCall2 " << In << std::endl;
    return true;
}

/*
 * Test
 */

void TFunction_Test()
{
    std::cout << std::endl << "-------TMemberFunction-------" << std::endl << std::endl;
    std::cout << "Testing constructor and Invoke" << std::endl;
    
    A a;
    TMemberFunction A_Func = TMemberFunction<A, bool(int32)>(&a, &A::Func);
    A_Func(32);
    
    TConstMemberFunction A_ConstFunc = TConstMemberFunction<A, bool(int32)>(&a, &A::ConstFunc);
    A_ConstFunc(32);
    
    std::cout << std::endl << "----------TFunction----------" << std::endl << std::endl;
    std::cout << "Testing constructors" << std::endl;
    
    struct Functor
    {
        bool operator()(int32 In)
        {
            std::cout << "Functor " << In << std::endl;
            return true;
        }
    } Fun;
    
    TFunction<bool(int32)> NormalFunc = Func;
    NormalFunc(5);
    
    TFunction<bool(int32)> MemberFunc = BindFunction(&a, &A::Func);
    MemberFunc(10);
    
    TFunction<bool(int32)> MemberFunc1 = BindFunction(&a, &A::ConstFunc);
    MemberFunc1(666);
    
    TFunction<bool(int32)> FunctorFunc = Fun;
    FunctorFunc(15);
    
    TFunction<bool(int32)> LambdaFunc = [](int32 Input) -> bool
    {
        std::cout << "Lambda " << Input << std::endl;
        return true;
    };
    LambdaFunc(20);
    
    A a1;
    TFunction<bool(int32)> LambdaMemberFunc = [&](int32 Input) -> bool
    {
        std::cout << "--Lambda Begin--" << std::endl;
        a1.Func(Input);
        a1.Func2(Input);
        std::cout << "--Lambda End--";
        return true;
    };
    LambdaMemberFunc(20);
    
    std::cout << "Test copy constructor" << std::endl;
    
    TFunction<bool(int32)> CopyFunc(MemberFunc);
    CopyFunc(30);
    MemberFunc(40);

    std::cout << "Test move constructor" << std::endl;
    TFunction<bool(int32)> MoveFunc(::Move(LambdaFunc));
    MoveFunc(50);
    if (LambdaFunc)
    {
        LambdaFunc(60);
    }

    std::cout << "Testing Assign" << std::endl;
    NormalFunc.Assign(Func2);
    MemberFunc.Assign(BindFunction(&a, &A::Func2));

    NormalFunc(70);
    MemberFunc(80);

    std::cout << "Testing Swap" << std::endl;
    NormalFunc.Swap(MemberFunc);

    NormalFunc(90);
    MemberFunc(100);
}
