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

		FORCEINLINE ConstIterator(ValueType* InPtr = nullptr)
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

		FORCEINLINE Iterator(ValueType* InPtr = nullptr)
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

	FORCEINLINE explicit TArray(SizeType InSize, const ValueType& Value = ValueType()) noexcept
		: Data(nullptr)
		, Size(0)
		, Capacity(0)
	{
		Reserve(InSize);

		Size = InSize;
		InternalMemset(Begin(), End(), Value);
	}

	FORCEINLINE explicit TArray(Iterator InBegin, Iterator InEnd) noexcept
		: Data(nullptr)
		, Size(0)
		, Capacity(0)
	{
		VALIDATE(InBegin < InEnd);

		const SizeType Count = static_cast<SizeType>(InEnd.Ptr - InBegin.Ptr);
		Iterator NewRange = InternalAllocateElements(Count);
		InternalCopyEmplace(InBegin, InEnd, NewRange);

		Data = NewRange.Ptr;
		Size = Count;
		Capacity = Count;
	}

	FORCEINLINE TArray(std::initializer_list<ValueType> IList) noexcept
		: Data(nullptr)
		, Size(0)
		, Capacity(0)
	{
		const SizeType Count = static_cast<SizeType>(IList.size());
		Iterator NewRange = InternalAllocateElements(Count);
		InternalMoveEmplace(IList, NewRange);

		Data = NewRange.Ptr;
		Size = Count;
		Capacity = Count;
	}

	FORCEINLINE TArray(const TArray& Other) noexcept
		: Data(nullptr)
		, Size(0)
		, Capacity(0)
	{
		ConstIterator ItBegin = Other.Begin();
		ConstIterator ItEnd = Other.End();

		const SizeType Count = static_cast<SizeType>(ItEnd.Ptr - ItBegin.Ptr);
		Iterator NewRange = InternalAllocateElements(Count);
		InternalCopyEmplace(ItBegin, ItEnd, NewRange);

		Data = NewRange.Ptr;
		Size = Count;
		Capacity = Count;
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

		InternalReleaseData();

		Capacity = 0;
		Data = nullptr;
	}

	FORCEINLINE void Clear() noexcept
	{
		InternalDestructRange(Begin(), InternalRealEnd());
		Size = 0;
	}

	FORCEINLINE void Assign(SizeType InSize, const ValueType& Value = ValueType()) noexcept
	{
		Clear();
		Reserve(InSize);

		Size = InSize;
		InternalMemset(Begin(), End(), Value);
	}

	FORCEINLINE void Assign(Iterator InBegin, Iterator InEnd) noexcept
	{
		// Does not make sense to assign a range from this TArray, since they could overlap
		VALIDATE(InternalIsRangeOwner(InBegin, InEnd) == false);
		VALIDATE(InBegin < InEnd);

		Clear();

		const SizeType Count = static_cast<SizeType>(InEnd.Ptr - InBegin.Ptr);
		if (Count > Capacity)
		{
			Iterator NewRange = InternalAllocateElements(Count);
			InternalReleaseData();
			Data = NewRange.Ptr;
			Capacity = Count;
		}

		InternalCopyEmplace(InBegin, InEnd, Begin());
		Size = Count;
		if (Size < Capacity)
		{
			InternalConstructRange(End(), InternalRealEnd());
		}
	}

	FORCEINLINE void Assign(std::initializer_list<ValueType> IList) noexcept
	{
		Clear();

		const SizeType Count = static_cast<SizeType>(IList.size());
		if (Count > Capacity)
		{
			Iterator NewRange = InternalAllocateElements(Count);
			InternalReleaseData();
			Data = NewRange.Ptr;
			Capacity = Count;
		}

		InternalMoveEmplace(IList, Begin());
		Size = Count;
		if (Size < Capacity)
		{
			InternalConstructRange(End(), InternalRealEnd());
		}
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
				Reserve(InSize + (Capacity / 2) + 1);
			}

			InternalMemset(Begin() + Size, End() + (InSize - Size), Value);
		}
		else if (InSize < Size)
		{
			InternalDestructRange(Begin() + InSize, End());
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
			InternalDestructRange(Begin() + InCapacity, End());
			Size = InCapacity;
		}

		Iterator NewRange = InternalAllocateElements(InCapacity);
		InternalMoveEmplace(Begin(), End(), NewRange);
		InternalConstructRange(NewRange + Size, NewRange + InCapacity);

		InternalDestructRange(Begin(), InternalRealEnd());
		InternalReleaseData();

		Data = NewRange.Ptr;
		Capacity = InCapacity;
	}

	FORCEINLINE ValueType& PushBack(const ValueType& Element) noexcept
	{
		Iterator ItEnd = End();
		if (Size >= Capacity)
		{
			const SizeType NewCapacity = InternalGetResizeFactor();
			Iterator NewRange = InternalAllocateElements(NewCapacity);
			InternalMoveEmplace(Begin(), ItEnd, NewRange);
			InternalConstructRange(NewRange + (Size + 1), NewRange + NewCapacity);

			InternalDestructRange(Begin(), InternalRealEnd());
			InternalReleaseData();

			Data = NewRange.Ptr;
			Capacity = NewCapacity;

			ItEnd = End();
		}
		else
		{
			InternalDestruct(ItEnd);
		}

		new(ItEnd.Ptr) ValueType(Element);
		Size++;
		return (*ItEnd);
	}

	FORCEINLINE ValueType& PushBack(ValueType&& Element) noexcept
	{
		Iterator ItEnd = End();
		if (Size >= Capacity)
		{
			const SizeType NewCapacity = InternalGetResizeFactor();
			Iterator NewRange = InternalAllocateElements(NewCapacity);
			InternalMoveEmplace(Begin(), ItEnd, NewRange);
			InternalConstructRange(NewRange + (Size + 1), NewRange + NewCapacity);

			InternalDestructRange(Begin(), InternalRealEnd());
			InternalReleaseData();

			Data = NewRange.Ptr;
			Capacity = NewCapacity;

			ItEnd = End();
		}
		else
		{
			InternalDestruct(ItEnd);
		}

		new(ItEnd.Ptr) ValueType(Move(Element));
		Size++;
		return (*ItEnd);
	}

	template<typename... TArgs>
	FORCEINLINE ValueType& EmplaceBack(TArgs&&... Args) noexcept
	{
		Iterator ItEnd = End();
		if (Size >= Capacity)
		{
			const SizeType NewCapacity = InternalGetResizeFactor();
			Iterator NewRange = InternalAllocateElements(NewCapacity);
			InternalMoveEmplace(Begin(), ItEnd, NewRange);
			InternalConstructRange(NewRange + (Size + 1), NewRange + NewCapacity);

			InternalDestructRange(Begin(), InternalRealEnd());
			InternalReleaseData();

			Data = NewRange.Ptr;
			Capacity = NewCapacity;

			ItEnd = End();
		}
		else
		{
			InternalDestruct(ItEnd);
		}

		new(ItEnd.Ptr) ValueType(Forward<TArgs>(Args)...);
		Size++;
		return (*ItEnd);
	}

	template<typename... TArgs>
	FORCEINLINE Iterator Emplace(ConstIterator Pos, TArgs&&... Args) noexcept
	{
		if (Pos == End())
		{
			EmplaceBack(Forward<TArgs>(Args)...);
			return (End() - 1);
		}

		const SizeType Index = static_cast<SizeType>(Pos.Ptr - Begin().Ptr);

		Iterator ItBegin = Begin() + Index;
		if (Size >= Capacity)
		{
			const SizeType NewCapacity = InternalGetResizeFactor();
			Iterator NewRange = InternalAllocateElements(NewCapacity);
			InternalMoveEmplace(Begin(), ItBegin, NewRange);
			InternalMoveEmplace(ItBegin, End(), NewRange + Index + 1);
			InternalConstructRange(NewRange + (Size + 1), NewRange + NewCapacity);

			InternalDestructRange(Begin(), InternalRealEnd());
			InternalReleaseData();

			Capacity = NewCapacity;
			Data = NewRange.Ptr;

			ItBegin = Begin() + Index;
		}
		else
		{
			InternalMemmoveForward(ItBegin, End(), End());
			InternalDestruct(ItBegin);
		}

		new (ItBegin.Ptr) ValueType(Forward<TArgs>(Args)...);
		Size++;
		return ItBegin;
	}

	FORCEINLINE Iterator Insert(ConstIterator Pos, const ValueType& Value) noexcept
	{
		if (Pos == End())
		{
			PushBack(Value);
			return (End() - 1);
		}

		const SizeType Index = static_cast<SizeType>(Pos.Ptr - Begin().Ptr);

		Iterator ItBegin = Begin() + Index;
		if (Size >= Capacity)
		{
			const SizeType NewCapacity = InternalGetResizeFactor();
			Iterator NewRange = InternalAllocateElements(NewCapacity);
			InternalMoveEmplace(Begin(), ItBegin, NewRange);
			InternalMoveEmplace(ItBegin, End(), NewRange + Index + 1);
			InternalConstructRange(NewRange + (Size + 1), NewRange + NewCapacity);

			InternalDestructRange(Begin(), InternalRealEnd());
			InternalReleaseData();

			Capacity = NewCapacity;
			Data = NewRange.Ptr;

			ItBegin = Begin() + Index;
		}
		else
		{
			InternalMemmoveForward(ItBegin, End(), End());
			InternalDestruct(ItBegin);
		}

		new(ItBegin.Ptr) ValueType(Value);
		Size++;
		return ItBegin;
	}

	FORCEINLINE Iterator Insert(ConstIterator Pos, ValueType&& Value) noexcept
	{
		if (Pos == End())
		{
			PushBack(Value);
			return (End() - 1);
		}

		const SizeType Index = static_cast<SizeType>(Pos.Ptr - Begin().Ptr);

		Iterator ItBegin = Begin() + Index;
		if (Size >= Capacity)
		{
			const SizeType NewCapacity = InternalGetResizeFactor();
			Iterator NewRange = InternalAllocateElements(NewCapacity);
			InternalMoveEmplace(Begin(), ItBegin, NewRange);
			InternalMoveEmplace(ItBegin, End(), NewRange + Index + 1);
			InternalConstructRange(NewRange + Size + 1, NewRange + NewCapacity);

			InternalDestructRange(Begin(), InternalRealEnd());
			InternalReleaseData();

			Capacity = NewCapacity;
			Data = NewRange.Ptr;

			ItBegin = Begin() + Index;
		}
		else
		{
			InternalMemmoveForward(ItBegin, End(), End());
			InternalDestruct(ItBegin);
		}

		new(ItBegin.Ptr) ValueType(Move(Value));
		Size++;
		return ItBegin;
	}

	FORCEINLINE Iterator Insert(ConstIterator Pos, std::initializer_list<ValueType> IList) noexcept
	{
		const SizeType Count = static_cast<SizeType>(IList.size());
		const SizeType NewSize = Size + Count;
		const SizeType Index = static_cast<SizeType>(Pos.Ptr - Begin().Ptr);

		Iterator ItBegin = Begin() + Index;
		if (NewSize >= Capacity)
		{
			const SizeType NewCapacity = NewSize + (Capacity / 2) + 1;
			Iterator NewRange = InternalAllocateElements(NewCapacity);
			InternalMoveEmplace(Begin(), ItBegin, NewRange);
			InternalMoveEmplace(ItBegin, End(), NewRange + Index + Count);
			InternalConstructRange(NewRange + Size + Count, NewRange + NewCapacity);

			InternalDestructRange(Begin(), InternalRealEnd());
			InternalReleaseData();

			Capacity = NewCapacity;
			Data = NewRange.Ptr;

			ItBegin = Begin() + Index;
		}
		else
		{
			InternalMemmoveForward(ItBegin, End(), End() + (Count - 1));
			InternalDestructRange(ItBegin, ItBegin + Count);
		}

		InternalMoveEmplace(IList, ItBegin);

		Size = NewSize;
		return ItBegin;
	}

	FORCEINLINE Iterator PopBack() noexcept
	{
		VALIDATE(Size > 0);

		InternalDestruct(Begin() + (--Size));
		return End();
	}

	FORCEINLINE Iterator Erase(Iterator Pos) noexcept
	{
		return Erase(static_cast<ConstIterator>(Pos));
	}

	FORCEINLINE Iterator Erase(ConstIterator Pos) noexcept
	{
		VALIDATE(InternalIsIteratorOwner(Pos));

		if (Pos == End())
		{
			return PopBack();
		}

		InternalDestruct(Pos);

		const SizeType Index = static_cast<SizeType>(Pos.Ptr - Begin().Ptr);
		Iterator ItBegin = Begin() + Index;
		InternalMemmoveBackwards(ItBegin + 1, End(), ItBegin);

		Size--;
		return ItBegin;
	}

	FORCEINLINE Iterator Erase(Iterator InBegin, Iterator InEnd) noexcept
	{
		return Erase(static_cast<ConstIterator>(InBegin), static_cast<ConstIterator>(InEnd));
	}

	FORCEINLINE Iterator Erase(ConstIterator InBegin, ConstIterator InEnd) noexcept
	{
		VALIDATE(InBegin < InEnd);
		VALIDATE(InternalIsRangeOwner(InBegin, InEnd));

		InternalDestructRange(InBegin, InEnd);

		const SizeType Index = static_cast<SizeType>(InBegin.Ptr - Begin().Ptr);
		const SizeType Offset = static_cast<SizeType>(InEnd.Ptr - InBegin.Ptr);

		Iterator ItBegin = Begin() + Index;
		Iterator ItOffset = ItBegin + Offset;
		if (ItOffset < End())
		{
			InternalMemmoveBackwards(ItOffset, End(), ItBegin);
		}

		Size -= Offset;
		return ItBegin;
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
			Iterator NewRange = InternalAllocateElements(Size);
			InternalMoveEmplace(Begin(), End(), NewRange);

			InternalDestructRange(Begin(), InternalRealEnd());
			InternalReleaseData();

			Data = NewRange.Ptr;
			Capacity = Size;
		}
	}

	FORCEINLINE bool IsEmpty() const noexcept
	{
		return (Size == 0);
	}

	FORCEINLINE Iterator Begin() noexcept
	{
		return Iterator(Data);
	}

	FORCEINLINE Iterator End() noexcept
	{
		return Iterator(Data + Size);
	}

	FORCEINLINE ConstIterator Begin() const noexcept
	{
		return ConstIterator(Data);
	}

	FORCEINLINE ConstIterator End() const noexcept
	{
		return ConstIterator(Data + Size);
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

			if (Capacity < Other.Size)
			{
				Iterator NewRange = InternalAllocateElements(Other.Size);
				InternalReleaseData();
				Data = NewRange.Ptr;
				Capacity = Other.Size;
			}

			InternalCopyEmplace(Other.Begin(), Other.End(), Begin());
			Size = Other.Size;
			if (Size < Capacity)
			{
				InternalConstructRange(End(), InternalRealEnd());
			}
		}

		return *this;
	}

	FORCEINLINE TArray& operator=(TArray&& Other) noexcept
	{
		if (this != std::addressof(Other))
		{
			Clear();

			InternalReleaseData();

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
	* STL iterator functions, Only here so that you can use Range for-loops
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
	FORCEINLINE Iterator InternalRealEnd() const noexcept
	{
		return Iterator(Data + Capacity);
	}

	FORCEINLINE bool InternalIsRangeOwner(ConstIterator InBegin, ConstIterator InEnd)
	{
		return (InBegin < InEnd) && (InBegin >= Begin()) && (InEnd <= End());
	}

	FORCEINLINE bool InternalIsIteratorOwner(ConstIterator It)
	{
		return (It >= Begin()) && (It <= End());
	}

	FORCEINLINE SizeType InternalGetResizeFactor() const
	{
		return Size + (Capacity)+1;
	}

	FORCEINLINE Iterator InternalAllocateElements(SizeType InCapacity)
	{
		constexpr SizeType ElementByteSize = sizeof(ValueType);
		return Iterator(reinterpret_cast<ValueType*>(malloc(static_cast<size_t>(ElementByteSize) * InCapacity)));
	}

	FORCEINLINE void InternalReleaseData()
	{
		if (Data)
		{
			free(Data);
		}
	}

	FORCEINLINE void InternalCopyEmplace(ConstIterator InBegin, ConstIterator InEnd, Iterator Dest)
	{
		// This function assumes that there is no overlap
		if constexpr (std::is_trivially_move_constructible<ValueType>())
		{
			const SizeType Count = static_cast<SizeType>(InEnd.Ptr - InBegin.Ptr);
			const SizeType CpySize = Count * sizeof(ValueType);
			memcpy(Dest.Ptr, InBegin.Ptr, CpySize);
		}
		else
		{
			while (InBegin != InEnd)
			{
				new(Dest.Ptr) ValueType(*InBegin);
				InBegin++;
				Dest++;
			}
		}
	}

	FORCEINLINE void InternalMoveEmplace(Iterator InBegin, Iterator InEnd, Iterator Dest)
	{
		// This function assumes that there is no overlap
		if constexpr (std::is_trivially_move_constructible<ValueType>())
		{
			const SizeType Count = static_cast<SizeType>(InEnd.Ptr - InBegin.Ptr);
			const SizeType CpySize = Count * sizeof(ValueType);
			memcpy(Dest.Ptr, InBegin.Ptr, CpySize);
		}
		else
		{
			while (InBegin != InEnd)
			{
				new(Dest.Ptr) ValueType(Move(*InBegin));
				InBegin++;
				Dest++;
			}
		}
	}

	FORCEINLINE void InternalMoveEmplace(std::initializer_list<ValueType> IList, Iterator Dest)
	{
		// This function assumes that there is no overlap
		for (const ValueType& Value : IList)
		{
			new(Dest.Ptr) ValueType(Move(Value));
			Dest++;
		}
	}

	FORCEINLINE void InternalMemset(Iterator InBegin, Iterator InEnd, const ValueType& Value)
	{
		// Sets each object in the range to value
		if constexpr (std::is_trivially_copyable<ValueType>() && (sizeof(ValueType) == sizeof(Char)))
		{
			// We can use normal memset if this object is trivially copyable and has the size of maximumum of a char (basically only chars)
			const SizeType Count = static_cast<SizeType>(InEnd.Ptr - InBegin.Ptr);
			const SizeType CpySize = Count * sizeof(ValueType);
			memset(InBegin.Ptr, Value, CpySize);
		}
		else if constexpr (std::is_nothrow_copy_assignable<ValueType>())
		{
			while (InBegin != InEnd)
			{
				(*InBegin) = Value;
				InBegin++;
			}
		}
		else if constexpr (std::is_nothrow_copy_constructible<ValueType>())
		{
			while (InBegin != InEnd)
			{
				InternalDestruct(InBegin);
				new(InBegin.Ptr) ValueType(Value);
				InBegin++;
			}
		}
		else if constexpr (std::is_nothrow_move_assignable<ValueType>())
		{
			while (InBegin != InEnd)
			{
				(*InBegin) = Move(const_cast<ValueType&>(Value));
				InBegin++;
			}
		}
		else if constexpr (std::is_nothrow_move_constructible<ValueType>())
		{
			while (InBegin != InEnd)
			{
				InternalDestruct(InBegin);
				new(InBegin.Ptr) ValueType(Move(const_cast<ValueType&>(Value)));
				InBegin++;
			}
		}
	}

	FORCEINLINE void InternalMemcpy(Iterator InBegin, Iterator InEnd, Iterator Dest)
	{
		// Copy each object in the range to the destination
		// This functions assumes that the range is not overlapping

		if constexpr (std::is_trivially_copyable<ValueType>())
		{
			const SizeType Count = static_cast<SizeType>(InEnd.Ptr - InBegin.Ptr);
			const SizeType CpySize = Count * sizeof(ValueType);
			memcpy(Dest.Ptr, InBegin.Ptr, CpySize);
		}
		else
		{
			while (InBegin != InEnd)
			{
				(*Dest) = (*InBegin);
				Dest++;
				InBegin++;
			}
		}
	}

	FORCEINLINE void InternalMemmoveBackwards(Iterator InBegin, Iterator InEnd, Iterator Dest)
	{
		VALIDATE(InBegin <= InEnd);
		if (InBegin == InEnd)
		{
			return;
		}

		VALIDATE(InEnd <= InternalRealEnd());
		VALIDATE(InternalIsIteratorOwner(Dest));

		// Move each object in the range to the destination
		// This functions assumes that the range is from this TArray (I.e It can overlap)

		const SizeType Count = static_cast<SizeType>(InEnd.Ptr - InBegin.Ptr);
		if constexpr (std::is_trivially_move_assignable<ValueType>())
		{
			const SizeType CpySize = Count * sizeof(ValueType);
			memmove(Dest.Ptr, InBegin.Ptr, CpySize);
		}
		else
		{
			while (InBegin != InEnd)
			{
				(*Dest) = Move(*InBegin);
				Dest++;
				InBegin++;
			}
		}
	}

	FORCEINLINE void InternalMemmoveForward(Iterator InBegin, Iterator InEnd, Iterator Dest)
	{
		// Move each object in the range to the destination, starts in the "End" and moves forward
		const SizeType Count = static_cast<SizeType>(InEnd.Ptr - InBegin.Ptr);
		if constexpr (std::is_trivially_move_assignable<ValueType>())
		{
			if (Count > 0)
			{
				const SizeType CpySize = Count * sizeof(ValueType);
				const SizeType OffsetSize = (Count - 1) * sizeof(ValueType);
				memmove(reinterpret_cast<Char*>(Dest.Ptr) - OffsetSize, InBegin.Ptr, CpySize);
			}
		}
		else
		{
			while (InEnd != InBegin)
			{
				InEnd--;
				(*Dest) = Move(*InEnd);
				Dest--;
			}
		}
	}

	FORCEINLINE void InternalDestruct(ConstIterator Pos)
	{
		// Calls the destructor (If it needs to be called)
		if constexpr (std::is_trivially_destructible<ValueType>() == false)
		{
			(*Pos).~ValueType();
		}
	}

	FORCEINLINE void InternalDestructRange(ConstIterator InBegin, ConstIterator InEnd)
	{
		VALIDATE(InBegin <= InEnd);

		// Calls the destructor for every object in the range (If it needs to be called)
		if constexpr (std::is_trivially_destructible<ValueType>() == false)
		{
			while (InBegin != InEnd)
			{
				InternalDestruct(InBegin);
				InBegin++;
			}
		}
	}

	FORCEINLINE void InternalConstructRange(ConstIterator InBegin, ConstIterator InEnd)
	{
		VALIDATE(InBegin <= InEnd);

		// Calls the default constructor for every object in the range (If it can be called)
		if constexpr (std::is_default_constructible<ValueType>())
		{
			while (InBegin != InEnd)
			{
				new(InBegin.Ptr) ValueType();
				InBegin++;
			}
		}
	}

	ValueType* Data;
	SizeType Size;
	SizeType Capacity;
};