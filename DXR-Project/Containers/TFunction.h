#include "Utilities/TUtilities.h"

/*
 * TFunction - Saves function pointers and is callable
 */
template<typename TCallable>
class TFunction;

template<typename TReturn, typename... TArgs>
class TFunction<TReturn(TArgs...)>
{
private:
	// Base
	class IFunction
	{
	public:
		virtual ~IFunction() = default;

		virtual TReturn Invoke(TArgs... Args) noexcept = 0;

		virtual IFunction* Clone(VoidPtr Memory) noexcept	= 0;
		virtual IFunction* Move(VoidPtr Memory) noexcept	= 0;
	};

	// Member functions
	template<typename T>
	class MemberFunction : public IFunction
	{
	public:
		typedef T* TPtr;
		typedef TReturn(T::* FunctionType)(TArgs...);

		inline MemberFunction(TPtr InThis, FunctionType InFunc) noexcept
			: IFunction()
			, Func(InFunc)
			, This(InThis)
		{
		}

		inline MemberFunction(const MemberFunction& Other)
			: IFunction()
			, Func(Other.Func)
			, This(Other.This)
		{
		}

		inline MemberFunction(MemberFunction&& Other) noexcept
			: IFunction()
			, Func(::Move(Other.Func))
			, This(::Move(Other.This))
		{
			Other.Func = nullptr;
			Other.This = nullptr;
		}

		inline virtual TReturn Invoke(TArgs... Args) noexcept override final
		{
			return ((*This).*Func)(Forward<TArgs>(Args)...);
		}

		inline virtual IFunction* Clone(VoidPtr Memory) noexcept override final
		{
			return new(Memory) MemberFunction(*this);
		}

		inline virtual IFunction* Move(VoidPtr Memory) noexcept override final
		{
			return new(Memory) MemberFunction(::Move(*this));
		}

	private:
		FunctionType Func;
		TPtr This;
	};

	// Generic functors
	template<typename F>
	class GenericFunctor : public IFunction
	{
	public:
		inline GenericFunctor(const F& InFunctor) noexcept
			: IFunction()
			, Functor(InFunctor)
		{
		}

		inline GenericFunctor(const GenericFunctor& Other) noexcept
			: IFunction()
			, Functor(Other.Functor)
		{
		}

		inline GenericFunctor(GenericFunctor&& Other) noexcept
			: IFunction()
			, Functor(::Move(Other.Functor))
		{
			if constexpr (std::is_pointer<F>())
			{
				Other.Functor = nullptr;
			}
		}

		inline virtual TReturn Invoke(TArgs... Args) noexcept override final
		{
			return Functor(Forward<TArgs>(Args)...);
		}

		inline virtual IFunction* Clone(VoidPtr Memory) noexcept override final
		{
			return new(Memory) GenericFunctor(*this);
		}

		inline virtual IFunction* Move(VoidPtr Memory) noexcept override final
		{
			return new(Memory) GenericFunctor(::Move(*this));
		}

	private:
		F Functor;
	};

public:
	inline TFunction() noexcept
		: StackBuffer()
		, Func(nullptr)
	{
	}

	inline TFunction(std::nullptr_t) noexcept
		: StackBuffer()
		, Func(nullptr)
	{
	}

	template<typename F>
	inline TFunction(F Functor) noexcept
		: StackBuffer()
		, Func(nullptr)
	{
		constexpr Uint32 StackSize = sizeof(StackBuffer);
		constexpr Uint32 FunctorSize = sizeof(GenericFunctor<F>);
		static_assert(FunctorSize <= StackSize, "Functor is too big for TFunction");
		static_assert(std::is_invocable<F, TArgs...>());

		new(reinterpret_cast<VoidPtr>(StackBuffer)) GenericFunctor<F>(Functor);
		Func = reinterpret_cast<IFunction*>(StackBuffer);
	}

	template<typename T>
	inline TFunction(T* This, TReturn(T::* MemberFunc)(TArgs...)) noexcept
		: StackBuffer()
		, Func(nullptr)
	{
		constexpr Uint32 StackSize = sizeof(StackBuffer);
		constexpr Uint32 FuncSize = sizeof(MemberFunction<T>);
		static_assert(FuncSize <= StackSize, "Functor is too big for TFunction");

		new(reinterpret_cast<VoidPtr>(StackBuffer)) MemberFunction<T>(This, MemberFunc);
		Func = reinterpret_cast<IFunction*>(StackBuffer);
	}

	inline TFunction(const TFunction& Other) noexcept
		: StackBuffer()
		, Func(nullptr)
	{
		if (Other.Func)
		{
			Func = Other.Func->Clone(reinterpret_cast<VoidPtr>(StackBuffer));
		}
	}

	inline TFunction(TFunction&& Other) noexcept
		: StackBuffer()
		, Func(nullptr)
	{
		if (Other.Func)
		{
			Func = Other.Func->Move(reinterpret_cast<VoidPtr>(StackBuffer));
			Other.Func = nullptr;
		}
	}

	inline ~TFunction()
	{
		InternalRelease();
	}

	void Swap(TFunction& Other)
	{
		TFunction TempFunc(*this);
		*this = Other;
		Other = TempFunc;
	}

	template<typename F>
	void Assign(F&& Functor)
	{
		constexpr Uint32 StackSize = sizeof(StackBuffer);
		constexpr Uint32 FunctorSize = sizeof(GenericFunctor<F>);
		static_assert(FunctorSize <= StackSize, "Functor is too big for TFunction");
		static_assert(std::is_invocable<F, TArgs...>());

		InternalRelease();

		new(reinterpret_cast<VoidPtr>(StackBuffer)) GenericFunctor<F>(Functor);
		Func = reinterpret_cast<IFunction*>(StackBuffer);
	}

	template<typename T>
	void Assign(T* This, TReturn(T::* MemberFunc)(TArgs...))
	{
		constexpr Uint32 StackSize = sizeof(StackBuffer);
		constexpr Uint32 FuncSize = sizeof(MemberFunction<T>);
		static_assert(FuncSize <= StackSize, "Functor is too big for TFunction");

		InternalRelease();

		new(reinterpret_cast<VoidPtr>(StackBuffer)) MemberFunction<T>(This, MemberFunc);
		Func = reinterpret_cast<IFunction*>(StackBuffer);
	}

	TReturn Invoke(TArgs... Args) noexcept
	{
		VALIDATE(Func != nullptr);
		return Func->Invoke(Forward<TArgs>(Args)...);
	}

	TReturn operator()(TArgs... Args) noexcept
	{
		return Invoke(Forward<TArgs>(Args)...);
	}

	explicit operator bool() const noexcept
	{
		return (Func != nullptr);
	}

	TFunction& operator=(const TFunction& Other) noexcept
	{
		if (this != &Other)
		{
			InternalRelease();
			if (Other.Func)
			{
				Func = Other.Func->Clone(reinterpret_cast<VoidPtr>(StackBuffer));
			}
		}

		return *this;
	}

	TFunction& operator=(TFunction&& Other) noexcept
	{
		if (this != &Other)
		{
			InternalRelease();
			if (Other.Func)
			{
				Func		= Other.Func->Move(reinterpret_cast<VoidPtr>(StackBuffer));
				Other.Func	= nullptr;
			}
		}

		return *this;
	}

	TFunction& operator=(std::nullptr_t) noexcept
	{
		InternalRelease();
		return *this;
	}

private:
	void InternalRelease()
	{
		if (Func)
		{
			Func->~IFunction();
			Func = nullptr;
		}
	}

	Byte StackBuffer[56];
	IFunction* Func;
};
