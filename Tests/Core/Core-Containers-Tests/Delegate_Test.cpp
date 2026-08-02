#include "Delegate_Test.h"

#if RUN_TDELEGATE_TEST
#include "TestUtils.h"

#include <Core/Containers/Tuple.h>
#include <Core/Containers/Pair.h>
#include <Core/Delegates/Delegate.h>
#include <Core/Delegates/MulticastDelegate.h>
#include <Core/Delegates/Event.h>
#include <Core/Containers/String.h>

namespace
{
    static int32 AddOne(int32 Num)
    {
        return Num + 1;
    }

    static int32 AddPayload(int32 Num, int32 Payload)
    {
        return Num + Payload;
    }

    static int32 GVoidCounter = 0;
    static void VoidStatic(int32)
    {
        ++GVoidCounter;
    }

    static int32 GTupleSum = 0;
    static void TupleFunc(int32 N0, int32 N1, int32 N2, int32 N3)
    {
        GTupleSum = N0 + N1 + N2 + N3;
    }

    struct FReceiver
    {
        int32 MemberAdd(int32 Num)
        {
            LastValue = Num;
            ++CallCount;
            return Num + 2;
        }

        int32 ConstAdd(int32 Num) const
        {
            ++CallCount;
            return Num + 3;
        }

        void MemberVoid(int32 Num)
        {
            LastValue = Num;
            ++CallCount;
        }

        void ConstVoid(int32) const
        {
            ++CallCount;
        }

        int32 LastValue = 0;
        mutable int32 CallCount = 0;
    };

    struct FVBase
    {
        virtual ~FVBase() = default;
        int32 Func(int32 Num)
        {
            return Num + 3;
        }

        int32 ConstFunc(int32 Num) const
        {
            return Num + 4;
        }

        virtual int32 VirtualFunc(int32 Num) = 0;
    };

    struct FVDerived : public FVBase
    {
        virtual int32 VirtualFunc(int32 Num) override final
        {
            return Num + 5;
        }
    };
}

DECLARE_EVENT(FSomeEvent, FEventDispatcher, int32);

class FEventDispatcher
{
public:
    void Dispatch()
    {
        SomeEvent.Broadcast(42);
    }

    FSomeEvent SomeEvent;
};

bool TDelegate_Test()
{
    TEST_BEGIN();

    TEST_SECTION("TDelegate CreateStatic / IsBound / Execute");
    {
        TDelegate<int32(int32)> Delegate = TDelegate<int32(int32)>::CreateStatic(&AddOne);
        TEST_EXPECT(Delegate.IsBound());
        TEST_EXPECT_EQ(Delegate.Execute(10), 11);
    }

    TEST_SECTION("TDelegate CreateStatic with payload");
    {
        TDelegate<int32(int32)> Delegate = TDelegate<int32(int32)>::CreateStatic(&AddPayload, 100);
        TEST_EXPECT(Delegate.IsBound());
        TEST_EXPECT_EQ(Delegate.Execute(5), 105);
    }

    TEST_SECTION("TDelegate CreateLambda");
    {
        TDelegate<int32(int32)> Delegate = TDelegate<int32(int32)>::CreateLambda([](int32 Num)
        {
            return Num * 2;
        });

        TEST_EXPECT_EQ(Delegate.Execute(21), 42);
    }

    TEST_SECTION("TDelegate CreateRaw member / const member");
    {
        FReceiver Receiver;
        TDelegate<int32(int32)> Delegate = TDelegate<int32(int32)>::CreateRaw(&Receiver, &FReceiver::MemberAdd);
        TEST_EXPECT_EQ(Delegate.Execute(8), 10);
        TEST_EXPECT_EQ(Receiver.LastValue, 8);

        TDelegate<int32(int32)> ConstDelegate = TDelegate<int32(int32)>::CreateRaw(&Receiver, &FReceiver::ConstAdd);
        TEST_EXPECT_EQ(ConstDelegate.Execute(8), 11);
    }

    TEST_SECTION("TDelegate Bind* / Unbind / ExecuteIfBound");
    {
        TDelegate<int32(int32)> Delegate;
        TEST_EXPECT(!Delegate.IsBound());
        TEST_EXPECT(!Delegate.ExecuteIfBound(1));

        Delegate.BindStatic(&AddOne);
        TEST_EXPECT(Delegate.IsBound());
        TEST_EXPECT(Delegate.ExecuteIfBound(1));

        Delegate.Unbind();
        TEST_EXPECT(!Delegate.IsBound());

        Delegate.BindLambda([](int32 Num)
        {
            return Num;
        });

        TEST_EXPECT(Delegate.IsBound());
    }

    TEST_SECTION("TDelegate UnbindIfBound by object");
    {
        FReceiver Receiver;
        TDelegate<int32(int32)> Delegate = TDelegate<int32(int32)>::CreateRaw(&Receiver, &FReceiver::MemberAdd);
        TEST_EXPECT(Delegate.IsBound());
        TEST_EXPECT(Delegate.UnbindIfBound(&Receiver));
        TEST_EXPECT(!Delegate.IsBound());
        TEST_EXPECT(!Delegate.UnbindIfBound(&Receiver));
    }

    TEST_SECTION("TMulticastDelegate Add* / Broadcast / GetCount / IsBound");
    {
        TMulticastDelegate<int32> Event;
        TEST_EXPECT(!Event.IsBound());
        TEST_EXPECT_EQ(Event.GetCount(), 0u);

        int32 LambdaSum = 0;
        Event.AddLambda([&LambdaSum](int32 Num)
        {
            LambdaSum += Num;
        });

        FReceiver Receiver;
        Event.AddRaw(&Receiver, &FReceiver::MemberVoid);

        TEST_EXPECT(Event.IsBound());
        TEST_EXPECT_EQ(Event.GetCount(), 2u);

        Event.Broadcast(7);
        TEST_EXPECT_EQ(LambdaSum, 7);
        TEST_EXPECT_EQ(Receiver.LastValue, 7);
        TEST_EXPECT_EQ(Receiver.CallCount, 1);
    }

    TEST_SECTION("TMulticastDelegate Unbind by handle");
    {
        TMulticastDelegate<int32> Event;
        
        int32 CounterA = 0;
        int32 CounterB = 0;

        FDelegateHandle HandleA = Event.AddLambda([&CounterA](int32)
        {
            ++CounterA;
        });
        
        Event.AddLambda([&CounterB](int32)
        {
            ++CounterB;
        });

        TEST_EXPECT_EQ(Event.GetCount(), 2u);
        TEST_EXPECT(Event.Unbind(HandleA));
        TEST_EXPECT_EQ(Event.GetCount(), 1u);

        Event.Broadcast(0);
        TEST_EXPECT_EQ(CounterA, 0);
        TEST_EXPECT_EQ(CounterB, 1);
    }

    TEST_SECTION("TMulticastDelegate UnbindIfBound / UnbindAll");
    {
        TMulticastDelegate<int32> Event;
        FReceiver Receiver;
        Event.AddRaw(&Receiver, &FReceiver::MemberVoid);
        Event.AddRaw(&Receiver, &FReceiver::ConstVoid);
        Event.AddStatic(&VoidStatic);

        TEST_EXPECT_EQ(Event.GetCount(), 3u);

        TEST_EXPECT(Event.UnbindIfBound(&Receiver));
        TEST_EXPECT_EQ(Event.GetCount(), 1u);

        Event.UnbindAll();
        TEST_EXPECT_EQ(Event.GetCount(), 0u);
        TEST_EXPECT(!Event.IsBound());
    }

    TEST_SECTION("TTuple: size / GetByIndex / comparison / Swap / Apply");
    {
        TTuple<int32, float, double, String> Tuple(5, 0.9f, 5.0, "A string");
        TEST_EXPECT_EQ(Tuple.Size(), 4);
        TEST_EXPECT_EQ((TTuple<int, float, double>::StaticSize()), 3);
        TEST_EXPECT_EQ(Tuple.GetByIndex<0>(), 5);
        TEST_EXPECT(Tuple.GetByIndex<3>() == "A string");

        TTuple<int32, float, double, String> Copy;
        Copy = Tuple;

        TTuple<int32, float, double, String> Moved = ::Move(Copy);
        TEST_EXPECT(Tuple == Moved);
        TEST_EXPECT(!(Tuple != Moved));
        TEST_EXPECT(Tuple <= Moved);
        TEST_EXPECT(Tuple >= Moved);

        TTuple<int32, float, double> FirstTuple(5, 32.0f, 500.0);
        TTuple<int32, float, double> SecondTuple(2, 22.0f, 100.0);
        FirstTuple.Swap(SecondTuple);
        
        TEST_EXPECT(FirstTuple.GetByIndex<0>() == 2);
        TEST_EXPECT(SecondTuple.GetByIndex<0>() == 5);

        GTupleSum = 0;
        TTuple<int32, int32> Args(80, 900);
        Args.ApplyAfter(TupleFunc, 30, 99);
        TEST_EXPECT(GTupleSum == (80 + 900 + 30 + 99));

        GTupleSum = 0;
        Args.ApplyBefore(TupleFunc, 30, 99);
        TEST_EXPECT(GTupleSum == (80 + 900 + 30 + 99));
    }

    TEST_SECTION("TPair: construct / comparison / Swap");
    {
        TPair<String, int32> P0("Pair0", 32);
        TPair<String, int32> P1("Pair1", 42);
        TEST_EXPECT(!(P0 == P1));
        TEST_EXPECT(P0 != P1);

        TPair<String, int32> P0Copy = P0;
        P0.Swap(P1);
        
        TEST_EXPECT(P1 == P0Copy);
    }

    TEST_SECTION("TDelegate BindRaw / virtual member / copy / move / Swap / payload");
    {
        FVDerived Derived;
        FVBase* Base = &Derived;

        TDelegate<int32(int32)> Delegate;
        Delegate.BindRaw(Base, &FVBase::Func);
        TEST_EXPECT_EQ(Delegate.Execute(100), 103);
        
        Delegate.BindRaw(Base, &FVBase::ConstFunc);
        TEST_EXPECT_EQ(Delegate.Execute(200), 204);

        Delegate.BindRaw(Base, &FVBase::VirtualFunc);
        TEST_EXPECT_EQ(Delegate.Execute(300), 305);
        TEST_EXPECT(Delegate.IsObjectBound(Base));

        TDelegate<int32(int32)> Copy = Delegate;
        TEST_EXPECT_EQ(Copy.Execute(300), 305);

        TDelegate<int32(int32)> Moved = ::Move(Copy);
        TEST_EXPECT_EQ(Moved.Execute(300), 305);
        TEST_EXPECT(!Copy.IsBound());

        TDelegate<int32(int32)> Other = TDelegate<int32(int32)>::CreateStatic(&AddOne);
        Moved.Swap(Other);

        TEST_EXPECT_EQ(Moved.Execute(10), 11);
        TEST_EXPECT_EQ(Other.Execute(300), 305);

        TDelegate<int32()> WithPayload = TDelegate<int32()>::CreateRaw(Base, &FVBase::Func, 200);
        TEST_EXPECT_EQ(WithPayload.Execute(), 203);
    }

    TEST_SECTION("TMulticastDelegate void signature");
    {
        FMulticastDelegate VoidEvent;

        int32 Counter = 0;
        VoidEvent.AddLambda([&Counter]()
        {
            ++Counter;
        });

        VoidEvent.Broadcast();
        TEST_EXPECT_EQ(Counter, 1);
    }

    TEST_SECTION("Delegate declaration macros / Event dispatch");
    {
        DECLARE_DELEGATE(FSomeDelegate, int32);
        FSomeDelegate SomeDelegate;
        TEST_EXPECT(!SomeDelegate.IsBound());

        DECLARE_RETURN_DELEGATE(FSomeReturnDelegate, bool, int32);
        FSomeReturnDelegate SomeReturnDelegate;
        TEST_EXPECT(!SomeReturnDelegate.IsBound());

        DECLARE_MULTICAST_DELEGATE(FSomeMulticast, int32);
        FSomeMulticast SomeMulticast;
        TEST_EXPECT(!SomeMulticast.IsBound());

        GVoidCounter = 0;

        FEventDispatcher Dispatcher;
        Dispatcher.SomeEvent.AddStatic(&VoidStatic);
        Dispatcher.Dispatch();

        TEST_EXPECT_EQ(GVoidCounter, 1);
    }

    TEST_END();
}
#endif
