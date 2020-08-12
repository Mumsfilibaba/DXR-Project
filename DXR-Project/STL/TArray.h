#pragma once
#include "Defines.h"
#include "Types.h"

#include "Utilities/TUtilities.h"

/*
* Dynamic Array similar to std::vector
*/
template<typename T>
class TArray
{
public:
	typedef T		ValueType;
	typedef Uint32	SizeType;

	/*
	* Constant Iterator
	*/
	class ConstIterator
	{
		friend class TArray;

	public:
		~ConstIterator() = default;

		FORCEINLINE ConstIterator(ValueType* InPtr)
			: Ptr(InPtr)
		{
		}

		FORCEINLINE const ValueType* operator->() const
		{
			return Ptr;
		}

		FORCEINLINE const ValueType& operator*() const
		{
			return *Ptr;
		}

		FORCEINLINE ConstIterator operator++()
		{
			Ptr++;
			return *this;
		}

		FORCEINLINE ConstIterator operator++(Int32)
		{
			ConstIterator Temp = *this;
			Ptr++;
			return Temp;
		}

		FORCEINLINE ConstIterator operator--()
		{
			Ptr--;
			return *this;
		}

		FORCEINLINE ConstIterator operator--(Int32)
		{
			ConstIterator Temp = *this;
			Ptr--;
			return Temp;
		}

		FORCEINLINE ConstIterator operator+(Int32 Offset) const
		{
			ConstIterator Temp = *this;
			return Temp += Offset;
		}

		FORCEINLINE ConstIterator operator-(Int32 Offset) const
		{
			ConstIterator Temp = *this;
			return Temp -= Offset;
		}

		FORCEINLINE ConstIterator& operator+=(Int32 Offset)
		{
			Ptr += Offset;
			return *this;
		}

		FORCEINLINE ConstIterator& operator-=(Int32 Offset)
		{
			Ptr -= Offset;
			return *this;
		}

		FORCEINLINE bool operator==(const ConstIterator& Other) const
		{
			return (Ptr == Other.Ptr);
		}

		FORCEINLINE bool operator!=(const ConstIterator& Other) const
		{
			return (Ptr != Other.Ptr);
		}

		FORCEINLINE bool operator<(const ConstIterator& Other) const
		{
			return Ptr < Other.Ptr;
		}

		FORCEINLINE bool operator<=(const ConstIterator& Other) const
		{
			return Ptr <= Other.Ptr;
		}

		FORCEINLINE bool operator>(const ConstIterator& Other) const
		{
			return Ptr > Other.Ptr;
		}

		FORCEINLINE bool operator>=(const ConstIterator& Other) const
		{
			return Ptr >= Other.Ptr;
		}

	protected:
		ValueType* Ptr;
	};

	/*
	* Standard Iterator
	*/
	class Iterator : public ConstIterator
	{
		friend class TArray;

	public:
		~Iterator() = default;

		FORCEINLINE Iterator(ValueType* InPtr)
			: ConstIterator(InPtr)
		{
		}

		FORCEINLINE ValueType* operator->() const
		{
			return const_cast<ValueType*>(ConstIterator::operator->());
		}

		FORCEINLINE ValueType& operator*() const
		{
			return const_cast<ValueType&>(ConstIterator::operator*());
		}

		FORCEINLINE Iterator operator++()
		{
			ConstIterator::operator++();
			return *this;
		}

		FORCEINLINE Iterator operator++(Int32)
		{
			Iterator Temp = *this;
			ConstIterator::operator++();
			return Temp;
		}

		FORCEINLINE Iterator operator--()
		{
			ConstIterator::operator--();
			return *this;
		}

		FORCEINLINE Iterator operator--(Int32)
		{
			Iterator Temp = *this;
			ConstIterator::operator--();
			return Temp;
		}

		FORCEINLINE Iterator operator+(Int32 Offset) const
		{
			Iterator Temp = *this;
			return Temp += Offset;
		}

		FORCEINLINE Iterator operator-(Int32 Offset) const
		{
			Iterator Temp = *this;
			return Temp -= Offset;
		}

		FORCEINLINE Iterator& operator+=(Int32 Offset)
		{
			ConstIterator::operator+=(Offset);
			return *this;
		}

		FORCEINLINE Iterator& operator-=(Int32 Offset)
		{
			ConstIterator::operator-=(Offset);
			return *this;
		}
	};

	/*
	* TArray API
	*/
public:
	FORCEINLINE TArray() noexcept
		: Data(nullptr)
		, Size(0)
		, Capacity(0)
	{
	}

	FORCEINLINE TArray(SizeType InSize, const ValueType& Value = ValueType()) noexcept
		: Data(nullptr)
		, Size(0)
		, Capacity(0)
	{
		Reserve(InSize);

		Size = InSize;
		InternalMemset(begin(), end(), Value);
	}

	FORCEINLINE TArray(Iterator Begin, Iterator End) noexcept
		: Data(nullptr)
		, Size(0)
		, Capacity(0)
	{
		VALIDATE(Begin < End);

		const SizeType Count = static_cast<SizeType>(End.Ptr - Begin.Ptr);
		Reserve(Count);

		// TODO: Maybe check if these iterators are from this TArray, then maybe we can simply do a memmove?
		InternalMemcpy(Begin, End, begin());
		Size = Count;
	}

	FORCEINLINE TArray(std::initializer_list<ValueType> IList) noexcept
		: Data(nullptr)
		, Size(0)
		, Capacity(0)
	{
		Assign(IList);
	}

	FORCEINLINE TArray(const TArray& Other) noexcept
		: Data(nullptr)
		, Size(0)
		, Capacity(0)
	{
		Assign(Other.begin(), Other.end());
	}

	FORCEINLINE TArray(TArray&& Other) noexcept
		: Data(Other.Data)
		, Size(Other.Size)
		, Capacity(Other.Capacity)
	{
		Other.Data = nullptr;
		Other.Size = 0;
		Other.Capacity = 0;
	}

	FORCEINLINE ~TArray()
	{
		Clear();
		Capacity = 0;

		if (Data)
		{
			free(Data);
			Data = nullptr;
		}
	}

	FORCEINLINE void Clear() noexcept
	{
		InternalDestructRange(begin(), end());
		Size = 0;
	}

	FORCEINLINE void Assign(SizeType InSize, const ValueType& Value = ValueType()) noexcept
	{
		Clear();
		Reserve(InSize);

		Size = InSize;
		InternalMemset(begin(), end(), Value);
	}

	FORCEINLINE void Assign(Iterator Begin, Iterator End) noexcept
	{
		VALIDATE(Begin < End);

		Clear();

		const SizeType Count = static_cast<SizeType>(End.Ptr - Begin.Ptr);
		Reserve(Count);

		// TODO: Maybe check if these iterators are from this TArray, then maybe we can simply do a memmove?
		InternalMemcpy(Begin, End, begin());
		Size = Count;
	}

	FORCEINLINE void Assign(std::initializer_list<ValueType> IList) noexcept
	{
		Clear();
		Reserve(static_cast<SizeType>(IList.size()));

		SizeType Index = 0;
		for (const ValueType& Object : IList)
		{
			new(Data + Index) ValueType(Move(Object));
			Index++;
		}

		Size = static_cast<SizeType>(IList.size());
	}

	FORCEINLINE void Resize(SizeType InSize, const ValueType& Value = ValueType()) noexcept
	{
		if (InSize == Size)
		{
			return;
		}

		if (InSize == 0)
		{
			Clear();
		}
		else if (InSize > Size)
		{
			if (InSize >= Capacity)
			{
				Reserve(InSize + (Capacity / 2));
			}

			InternalMemset(begin() + Size, end() + (InSize - Size), Value);
		}
		else if (InSize < Size)
		{
			InternalDestructRange(begin() + InSize, end());
		}

		Size = InSize;
	}

	FORCEINLINE void Reserve(SizeType InCapacity) noexcept
	{
		if (InCapacity == Capacity)
		{
			return;
		}

		if (InCapacity < Size)
		{
			InternalDestructRange(begin() + InCapacity, end());
			Size = InCapacity;
		}

		constexpr SizeType ElementSize = sizeof(ValueType);
		ValueType* TempData = reinterpret_cast<ValueType*>(malloc(InCapacity * ElementSize));

		for (SizeType Index = 0; Index < Size; Index++)
		{
			new(TempData + Index) ValueType(Move(Data[Index]));
		}

		if (Data)
		{
			free(Data);
		}

		Data = TempData;
		Capacity = InCapacity;
	}

	FORCEINLINE Iterator PushBack(const ValueType& Element) noexcept
	{
		if (Size >= Capacity)
		{
			Reserve(Capacity + (Capacity / 2));
		}

		Data[Size++] = Element;
		return end();
	}

	FORCEINLINE Iterator PushBack(ValueType&& Element) noexcept
	{
		if (Size >= Capacity)
		{
			Reserve(Capacity + (Capacity / 2));
		}

		Data[Size++] = Move(Element);
		return end();
	}

	template<typename... TArgs>
	FORCEINLINE Iterator EmplaceBack(TArgs&&... Args) noexcept
	{
		if (Size >= Capacity)
		{
			Reserve(Capacity + (Capacity / 2));
		}

		new(Data + (Size++)) T(Forward<TArgs>(Args)...);
		return end();
	}

	template<typename... TArgs>
	FORCEINLINE Iterator Emplace(ConstIterator Pos, TArgs&&... Args) noexcept
	{
		if (Pos == end())
		{
			return EmplaceBack(Forward<TArgs>(Args)...);
		}

		const SizeType Index = static_cast<SizeType>(Pos.Ptr - begin().Ptr);
		if (Size >= Capacity)
		{
			// TODO: Remove unneccessary moves?
			Reserve(Size + (Capacity / 2));
		}

		Iterator Begin = begin() + Index;
		InternalMemmoveReverse(Begin, end(), end());

		new (Begin.Ptr) ValueType(Forward<TArgs>(Args)...);
		Size++;

		return Begin;
	}

	FORCEINLINE Iterator Insert(ConstIterator Pos, const ValueType& Value) noexcept
	{
		if (Pos == end())
		{
			return PushBack(Value);
		}

		const SizeType Index = static_cast<SizeType>(Pos.Ptr - begin().Ptr);
		if (Size >= Capacity)
		{
			// TODO: Remove unneccessary moves?
			Reserve(Size + (Capacity / 2));
		}

		Iterator Begin = begin() + Index;
		InternalMemmoveReverse(Begin, end(), end());

		(*Begin) = Value;
		Size++;

		return Begin;
	}

	FORCEINLINE Iterator Insert(ConstIterator Pos, ValueType&& Value) noexcept
	{
		if (Pos == end())
		{
			return PushBack(Value);
		}

		const SizeType Index = static_cast<SizeType>(Pos.Ptr - begin().Ptr);
		if (Size >= Capacity)
		{
			// TODO: Remove unneccessary moves?
			Reserve(Size + (Capacity / 2));
		}

		Iterator Begin = begin() + Index;
		InternalMemmoveReverse(Begin, end(), end());

		(*Begin) = Move(Value);
		Size++;

		return Begin;
	}

	FORCEINLINE Iterator Insert(ConstIterator Pos, std::initializer_list<ValueType> IList) noexcept
	{
		const SizeType Count = static_cast<SizeType>(IList.size());
		const SizeType NewSize = Size + Count;
		const SizeType Offset = Count - 1;
		const SizeType Index = static_cast<SizeType>(Pos.Ptr - begin().Ptr);
		if (NewSize >= Capacity)
		{
			// TODO: Remove unneccessary moves?
			Reserve(NewSize + (Capacity / 2));
		}

		Iterator Begin = begin() + Index;
		InternalMemmoveReverse(Begin, end(), end() + Offset);

		Iterator It = Begin;
		for (const ValueType& Value : IList)
		{
			new (It.Ptr) ValueType(Move(Value));
			It++;
		}

		Size += Count;
		return Begin;
	}

	FORCEINLINE Iterator PopBack() noexcept
	{
		Data[--Size].~ValueType();
		return end();
	}

	FORCEINLINE Iterator Erase(Iterator Pos) noexcept
	{
		return Erase(static_cast<ConstIterator>(Pos));
	}

	FORCEINLINE Iterator Erase(ConstIterator Pos) noexcept
	{
		if (Pos == end())
		{
			return PopBack();
		}

		(*Pos).~ValueType();

		const SizeType Index = static_cast<SizeType>(Pos.Ptr - begin().Ptr);
		Iterator Begin = begin() + Index;
		InternalMemmove(Begin + 1, end(), Begin);

		Size--;
		return Begin;
	}

	FORCEINLINE Iterator Erase(Iterator First, Iterator Last) noexcept
	{
		return Erase(static_cast<ConstIterator>(First), static_cast<ConstIterator>(Last));
	}

	FORCEINLINE Iterator Erase(ConstIterator First, ConstIterator Last) noexcept
	{
		VALIDATE(First < Last);

		for (ConstIterator It = First; (It != Last); It++)
		{
			(*It).~ValueType();
		}

		const SizeType Index = static_cast<SizeType>(First.Ptr - begin().Ptr);
		const SizeType Offset = static_cast<SizeType>(Last.Ptr - First.Ptr);

		Iterator Begin = begin() + Index;
		InternalMemmove(Begin + Offset, end(), Begin);

		Size -= Offset;
		return Begin;
	}

	FORCEINLINE void Swap(TArray& Other) noexcept
	{
		if (this != std::addressof(Other))
		{
			ValueType* TempPtr = Data;
			SizeType	TempSize = Size;
			SizeType	TempCapacity = Capacity;

			Data = Other.Data;
			Size = Other.Size;
			Capacity = Other.Capacity;

			Other.Data = TempPtr;
			Other.Size = TempSize;
			Other.Capacity = TempCapacity;
		}
	}

	FORCEINLINE void ShrinkToFit() noexcept
	{
		if (Capacity > Size)
		{
			ValueType* TempData = new ValueType[Size];
			InternalMemmove(begin(), end(), Iterator(TempData));

			VALIDATE(Data != nullptr);
			free(Data);

			Data = TempData;
			Capacity = Size;
		}
	}

	FORCEINLINE bool IsEmpty() const noexcept
	{
		return (Size == 0);
	}

	FORCEINLINE ValueType& GetFront() noexcept
	{
		VALIDATE(Size > 0);
		return Data[0];
	}

	FORCEINLINE const ValueType& GetFront() const noexcept
	{
		VALIDATE(Size > 0);
		return Data[0];
	}

	FORCEINLINE ValueType& GetBack() noexcept
	{
		VALIDATE(Size > 0);
		return Data[Size - 1];
	}

	FORCEINLINE const ValueType& GetBack() const noexcept
	{
		VALIDATE(Size > 0);
		return Data[Size - 1];
	}

	FORCEINLINE ValueType* GetData() noexcept
	{
		return Data;
	}

	FORCEINLINE const ValueType* GetData() const noexcept
	{
		return Data;
	}

	FORCEINLINE SizeType GetSize() const noexcept
	{
		return Size;
	}

	FORCEINLINE SizeType GetCapacity() const noexcept
	{
		return Capacity;
	}

	FORCEINLINE ValueType& GetElementAt(SizeType Index) noexcept
	{
		VALIDATE(Index < Size);
		return Data[Index];
	}

	FORCEINLINE const ValueType& GetElementAt(SizeType Index) const noexcept
	{
		VALIDATE(Index < Size);
		return Data[Index];
	}

	FORCEINLINE TArray& operator=(const TArray& Other) noexcept
	{
		if (this != std::addressof(Other))
		{
			Clear();
			Reserve(Other.Capacity);

			InternalMemcpy(Other.begin(), Other.end(), begin());
			Size = Other.Size;
		}

		return *this;
	}

	FORCEINLINE TArray& operator=(TArray&& Other) noexcept
	{
		if (this != std::addressof(Other))
		{
			Data = Other.Data;
			Size = Other.Size;
			Capacity = Other.Capacity;

			Other.Data = nullptr;
			Other.Size = 0;
			Other.Capacity = 0;
		}

		return *this;
	}

	FORCEINLINE TArray& operator=(std::initializer_list<ValueType> IList) noexcept
	{
		Assign(IList);
		return *this;
	}

	FORCEINLINE ValueType& operator[](SizeType Index) noexcept
	{
		return GetElementAt(Index);
	}

	FORCEINLINE const ValueType& operator[](SizeType Index) const noexcept
	{
		return GetElementAt(Index);
	}

	/*
	* STL iterator functions
	*/
public:
	FORCEINLINE Iterator begin() noexcept
	{
		return Iterator(Data);
	}

	FORCEINLINE Iterator end() noexcept
	{
		return Iterator(Data + Size);
	}

	FORCEINLINE ConstIterator begin() const noexcept
	{
		return ConstIterator(Data);
	}

	FORCEINLINE ConstIterator end() const noexcept
	{
		return ConstIterator(Data + Size);
	}

private:
	FORCEINLINE void InternalMemset(Iterator First, Iterator Last, const ValueType& Value)
	{
		VALIDATE(Last >= First);

		// Sets each object in the range to value
		for (; First != Last; First++)
		{
			(*First) = Value;
		}
	}

	FORCEINLINE void InternalMemcpy(Iterator First, Iterator Last, Iterator Dest)
	{
		VALIDATE(Last >= First);

		// Copy each object in the range to the destination
		for (; First != Last; First++)
		{
			(*Dest) = (*First);
			Dest++;
		}
	}

	FORCEINLINE void InternalMemmove(Iterator First, Iterator Last, Iterator Dest)
	{
		VALIDATE(Last >= First);

		// Move each object in the range to the destination
		for (; First != Last; First++)
		{
			(*Dest) = Move(*First);
			Dest++;
		}
	}

	FORCEINLINE void InternalMemcpyReverse(Iterator First, Iterator Last, Iterator Dest)
	{
		VALIDATE(Last >= First);

		// Copy each object in the range to the destination, starts in the "end" and moves forward
		for (; Last != First; Dest--)
		{
			Last--;
			(*Dest) = (*Last);
		}
	}

	FORCEINLINE void InternalMemmoveReverse(Iterator First, Iterator Last, Iterator Dest)
	{
		VALIDATE(Last >= First);

		// Move each object in the range to the destination, starts in the "end" and moves forward
		for (; Last != First; Dest--)
		{
			Last--;
			(*Dest) = Move(*Last);
		}
	}

	FORCEINLINE void InternalDestructRange(Iterator First, Iterator Last)
	{
		VALIDATE(Last >= First);

		// Calls the destructor for every object in the range
		for (; First != Last; First++)
		{
			(*First).~ValueType();
		}
	}

	ValueType* Data;
	SizeType Size;
	SizeType Capacity;
};