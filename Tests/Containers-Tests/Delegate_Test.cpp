#include "Delegate_Test.h"

#if RUN_TDELEGATE_TEST
#include "TestUtils.h"

#include <Core/Containers/Tuple.h>
#include <Core/Containers/Pair.h>
#include <Core/Delegates/Delegate.h>
#include <Core/Delegates/MulticastDelegate.h>
#include <Core/Delegates/Event.h>

#include <iostream>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Functions

static int32 StaticFunc(int32 Num) 
{
    std::cout << "StaticFunc=" << Num << std::endl;
    return Num + 1;
}

static void StaticFunc2(int32 Num) 
{
    std::cout << "StaticFunc2=" << Num << std::endl;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FBase

struct FBase
{
    virtual ~FBase() = default;

    int32 Func(int32 Num) 
    {
        std::cout << "MemberFunc=" << Num << std::endl;
        return Num + 3;
    }

    int32 ConstFunc(int32 Num) const
    {
        std::cout << "ConstMemberFunc=" << Num << std::endl;
        return Num + 4;
    }

    virtual int32 VirtualFunc(int32 Num) = 0;

    virtual int32 VirtualConstFunc(int32 Num) const = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FDerived

struct FDerived : public FBase
{
    virtual int32 VirtualFunc(int32 Num) override final
    {
        std::cout << "Virtual MemberFunc=" << Num << std::endl;
        return Num + 5;
    }

    virtual int32 VirtualConstFunc(int32 Num) const override final
    {
        std::cout << "Virtual ConstMemberFunc=" << Num << std::endl;
        return Num + 6;
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FBase2

struct FBase2
{
    virtual ~FBase2() = default;

    void Func(int32 Num)
    {
        std::cout << "MemberFunc2=" << Num << std::endl;
    }

    void ConstFunc(int32 Num) const
    {
        std::cout << "ConstMemberFunc2=" << Num << std::endl;
    }

    virtual void VirtualFunc(int32 Num) = 0;

    virtual void VirtualConstFunc(int32 Num) const = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FDerived2

struct FDerived2 : public FBase2
{
    virtual void VirtualFunc(int32 Num) override final
    {
        std::cout << "Virtual MemberFunc2=" << Num << std::endl;
    }

    virtual void VirtualConstFunc(int32 Num) const override final
    {
        std::cout << "Virtual ConstMemberFunc2=" << Num << std::endl;
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TupleFunc

static void TupleFunc(int32 Num0, int32 Num1, int32 Num2, int32 Num3) 
{
    std::cout << "Tuple func Num0=" << Num0 << ", Num1=" << Num1 << ", Num2=" << Num2 << ", Num3=" << Num3 << std::endl;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FEventDispacher

DECLARE_EVENT(FSomeEvent, FEventDispacher, int32);
FSomeEvent GSomeEvent;

class FEventDispacher
{
public:
    void Func()
    {
        GSomeEvent.Broadcast(42);
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TDelegate_Test

bool TDelegate_Test()
{
    std::cout << std::endl << "----Testing Tuple----" << std::endl << std::endl;
    TTuple<int32, float, double, std::string> Tuple(5, 0.9f, 5.0, "A string");

    TEST_CHECK(Tuple.Size() == 4);
    TEST_CHECK(TTuple<int, float, double>::NumElements == 3);

    TEST_CHECK(Tuple.GetByIndex<0>() == 5);
    TEST_CHECK(Tuple.GetByIndex<1>() == 0.9f);
    TEST_CHECK(Tuple.GetByIndex<2>() == 5.0);
    TEST_CHECK(Tuple.GetByIndex<3>() == "A string");

    TEST_CHECK(Tuple.Get<int32>()       == 5);
    TEST_CHECK(Tuple.Get<float>()       == 0.9f);
    TEST_CHECK(Tuple.Get<double>()      == 5.0);
    TEST_CHECK(Tuple.Get<std::string>() == "A string");

    TEST_CHECK(TupleGetByIndex<0>(Tuple) == 5);
    TEST_CHECK(TupleGetByIndex<1>(Tuple) == 0.9f);
    TEST_CHECK(TupleGetByIndex<2>(Tuple) == 5.0);
    TEST_CHECK(TupleGetByIndex<3>(Tuple) == "A string");

    TEST_CHECK(TupleGet<int>(Tuple)         == 5);
    TEST_CHECK(TupleGet<float>(Tuple)       == 0.9f);
    TEST_CHECK(TupleGet<double>(Tuple)      == 5.0);
    TEST_CHECK(TupleGet<std::string>(Tuple) == "A string");

    TTuple<int32, float, double, std::string> Tuple2;
    Tuple2 = Tuple;

    TTuple<int32, float, double, std::string> Tuple3 = ::Move(Tuple2);
    TEST_CHECK((Tuple == Tuple3) == true);
    TEST_CHECK((Tuple != Tuple3) == false);
    TEST_CHECK((Tuple <= Tuple3) == true);
    TEST_CHECK((Tuple <  Tuple3) == false);
    TEST_CHECK((Tuple >  Tuple3) == false);
    TEST_CHECK((Tuple >= Tuple3) == true);
    
    TTuple<int32, float, double> Tuple4(5, 32.0f, 500.0);
    TTuple<int32, float, double> Tuple5(2, 22.0f, 100.0);
    Tuple4.Swap(Tuple5);
    
    TEST_CHECK((Tuple4 == Tuple5) == false);
    TEST_CHECK((Tuple4 != Tuple5) == true);
    TEST_CHECK((Tuple4 <= Tuple5) == true);
    TEST_CHECK((Tuple4 <  Tuple5) == true);
    TEST_CHECK((Tuple4 >  Tuple5) == false);
    TEST_CHECK((Tuple4 >= Tuple5) == false);

    TTuple<float, float> PairTuple0(80.0f, 900.0f);
    TTuple<float, float> PairTuple1(50.0f, 100.0f);
    PairTuple1.First  = 30.0f;
    PairTuple1.Second = 200.0f;

    TEST_CHECK(PairTuple0.First           == 80.0f);
    TEST_CHECK(PairTuple0.Second          == 900.0f);
    TEST_CHECK(PairTuple0.Get<float>()    == 80.0f);
    TEST_CHECK(PairTuple0.Get<float>()    == 80.0f);
    TEST_CHECK(PairTuple0.GetByIndex<0>() == 80.0f);
    TEST_CHECK(PairTuple0.GetByIndex<1>() == 900.0f);

    PairTuple0.Swap(PairTuple1);

    TEST_CHECK(PairTuple0.First           == 30.0f);
    TEST_CHECK(PairTuple0.Second          == 200.0f);
    TEST_CHECK(PairTuple0.Get<float>()    == 30.0f);
    TEST_CHECK(PairTuple0.Get<float>()    == 30.0f);
    TEST_CHECK(PairTuple0.GetByIndex<0>() == 30.0f);
    TEST_CHECK(PairTuple0.GetByIndex<1>() == 200.0f);

    TTuple<int32, int32> PairTuple2(80, 900);
    PairTuple2.ApplyAfter(TupleFunc, 30, 99);
    PairTuple2.ApplyBefore(TupleFunc, 30, 99);

    TTuple<int32, int32, int32> Args(10, 20, 30);
    Args.ApplyAfter(TupleFunc, 99);
    Args.ApplyBefore(TupleFunc, 99);

    TPair<std::string, int32> Pair0("Pair0", 32);
    TPair<std::string, int32> Pair1("Pair1", 42);
    TEST_CHECK((Pair0 == Pair1) == false);
    TEST_CHECK((Pair0 != Pair1) == true);

    Pair0.Swap(Pair1);

    std::cout << std::endl << "----Testing Delegate----" << std::endl << std::endl;

    {
        TDelegate<int32(int32) > Delegate;
        TEST_CHECK(Delegate.IsBound()          == false);
        TEST_CHECK(Delegate.ExecuteIfBound(32) == false);

        // Static
        Delegate.BindStatic(StaticFunc);
        TEST_CHECK(Delegate.GetBoundObject() != nullptr);
        TEST_CHECK(Delegate.IsBound()        == true);
        TEST_CHECK(Delegate.Execute(32)      == 33);

        // Lambda
        int64 x = 50;
        int64 y = 150;
        int64 z = 250;
        auto Lambda = [=](int32 Num)  -> int32
        {
            std::cout << "Lambda (x=" << x << ", y=" << y << ", z=" << z << ") =" << Num << std::endl;
            return Num + 2;
        };

        Delegate.BindLambda(Lambda);
        TEST_CHECK(Delegate.Execute(500) == 502);

        // Members
        FBase* Base = new FDerived();
        Delegate.BindRaw(Base, &FBase::Func);
        TEST_CHECK(Delegate.Execute(100) == 103);
    
        Delegate.BindRaw(Base, &FBase::ConstFunc);
        TEST_CHECK(Delegate.Execute(200) == 204);

        Delegate.BindRaw(Base, &FDerived::Func);
        TEST_CHECK(Delegate.Execute(300) == 303);

        Delegate.BindRaw(Base, &FDerived::ConstFunc);
        TEST_CHECK(Delegate.Execute(400) == 404);

        TEST_CHECK(Delegate.IsObjectBound(Base) == true);
        TEST_CHECK(Delegate.UnbindIfBound(Base) == true);

        Delegate.Unbind();

        TEST_CHECK(Delegate.IsObjectBound(Base) == false);
        TEST_CHECK(Delegate.UnbindIfBound(Base) == false);

        // Copy
        Delegate.BindLambda(Lambda);
        Delegate.Execute(0);

        TDelegate<int32(int32) > Delegate2 = Delegate;
        Delegate2.Execute(100);

        Delegate = Delegate2;
        Delegate.Execute(200);

        // Move
        TDelegate<int32(int32) > Delegate3 = Move(Delegate2);
        Delegate3.Execute(300);
        Delegate2.ExecuteIfBound(400);

        Delegate = Move(Delegate3);
        Delegate.Execute(500);

        // Swap
        TDelegate<int32(int32) > Static;
        Static.BindLambda(StaticFunc);
        Static.Execute(1000);

        TDelegate<int32(int32) > NotStatic;
        NotStatic.BindLambda(Lambda);
        NotStatic.Execute(2000);

        NotStatic.Swap(Static);
        NotStatic.Execute(3000);
        Static.Execute(4000);

        // Static Create functions with and without payload
        std::cout << std::endl << "Static Create functions with and without payload" << std::endl << std::endl;
        TDelegate<int32(int32) > Lambda2 = TDelegate<int32(int32) >::CreateLambda(Lambda);
        Lambda2.Execute(100);
        TDelegate<int32()> Lambda3 = TDelegate<int32()>::CreateLambda(Lambda, 200);
        Lambda3.Execute();

        TDelegate<int32(int32) > Static2 = TDelegate<int32(int32) >::CreateStatic(StaticFunc);
        Static2.Execute(100);
        TDelegate<int32()> Static3 = TDelegate<int32()>::CreateStatic(StaticFunc, 200);
        Static3.Execute();

        TDelegate<int32(int32) > Member0 = TDelegate<int32(int32) >::CreateRaw(Base, &FBase::Func);
        Member0.Execute(100);
        TDelegate<int32()> Member01 = TDelegate<int32()>::CreateRaw(Base, &FBase::Func, 200);
        Member01.Execute();

        TDelegate<int32(int32) > Member1 = TDelegate<int32(int32) >::CreateRaw(Base, &FBase::ConstFunc);
        Member1.Execute(100);
        TDelegate<int32()> Member11 = TDelegate<int32()>::CreateRaw(Base, &FBase::ConstFunc, 200);
        Member11.Execute();

        TDelegate<int32(int32) > Member2 = TDelegate<int32(int32) >::CreateRaw(Base, &FDerived::Func);
        Member2.Execute(100);
        TDelegate<int32()> Member21 = TDelegate<int32()>::CreateRaw(Base, &FDerived::Func, 200);
        Member21.Execute();

        TDelegate<int32(int32) > Member3 = TDelegate<int32(int32) >::CreateRaw(Base, &FDerived::ConstFunc);
        Member3.Execute(100);

        TDelegate<int32()> Member31 = TDelegate<int32()>::CreateRaw(Base, &FDerived::ConstFunc, 200);
        Member31.Execute();

        delete Base;
    }

    std::cout << std::endl << "----Testing MultiCastDelegate----" << std::endl << std::endl;

    {
        TMulticastDelegate<> VoidMultiDelegates;
        VoidMultiDelegates.AddLambda([]()
        {
            std::cout << "Lambda Void" << std::endl;
        });
        
        VoidMultiDelegates.Broadcast();
        
        TMulticastDelegate<int32> MultiDelegates;
        TEST_CHECK(MultiDelegates.IsBound() == false);

        // Static
        MultiDelegates.AddStatic(StaticFunc2);
        TEST_CHECK(MultiDelegates.IsBound() == true);

        // Lambda
        int64 x = 50;
        int64 y = 150;
        int64 z = 250;
        auto Lambda = [=](int32 Num) 
        {
            std::cout << "Lambda2 (x=" << x << ", y=" << y << ", z=" << z << ") =" << Num << std::endl;
        };

        MultiDelegates.AddLambda(Lambda);

        // Members
        FBase2* Base = new FDerived2();
        MultiDelegates.AddRaw(Base, &FBase2::Func);
        MultiDelegates.AddRaw(Base, &FBase2::ConstFunc);
        MultiDelegates.AddRaw(Base, &FDerived2::Func);
        MultiDelegates.AddRaw(Base, &FDerived2::ConstFunc);

        MultiDelegates.Broadcast(5000);

        TEST_CHECK(MultiDelegates.GetCount()          == 6);
        TEST_CHECK(MultiDelegates.UnbindIfBound(Base) == true);
        TEST_CHECK(MultiDelegates.GetCount()          == 2);
        TEST_CHECK(MultiDelegates.UnbindIfBound(Base) == false);

        delete Base;
    }

    std::cout << std::endl << "----Testing Delegate Macros----" << std::endl << std::endl;
    {
        DECLARE_DELEGATE(FSomeDelegate, int32);
        FSomeDelegate SomeDelegate;

        DECLARE_RETURN_DELEGATE(FSomeReturnDelegate, bool, int32);
        FSomeReturnDelegate SomeReturnDelegate;

        DECLARE_MULTICAST_DELEGATE(FSomeMulticastDelegate, int32);
        FSomeMulticastDelegate SomeMulticastDelegate;

        GSomeEvent.AddStatic(StaticFunc2);

        FEventDispacher EventDispacher;
        EventDispacher.Func();
    }
}

#endif
