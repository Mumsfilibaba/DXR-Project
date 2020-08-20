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
	typedef Uint32 SizeType;

	/*
	* Constant Iterator
	*/
	class ConstIterator
	{
		friend class TArray;

	public:
		~ConstIterator() = default;

		FORCEINLINE explicit ConstIterator(T* InPtr = nullptr)
			: Ptr(InPtr)
		{
		}

		FORCEINLINE const T* operator->() const
		{
			return Ptr;
		}

		FORCEINLINE const T& operator*() const
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
		T* Ptr;
	};

	/*
	* Standard Iterator
	*/
	class Iterator : public ConstIterator
	{
		friend class TArray;

	public:
		~Iterator() = default;

		FORCEINLINE explicit Iterator(T* InPtr = nullptr)
			: ConstIterator(InPtr)
		{
		}

		FORCEINLINE T* operator->() const
		{
			return const_cast<T*>(ConstIterator::operator->());
		}

		FORCEINLINE T& operator*() const
		{
			return const_cast<T&>(ConstIterator::operator*());
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
	* Reverse Constant Iterator
	* Stores for example End(), but will reference End() - 1
	*/
	class ReverseConstIterator
	{
		friend class TArray;

	public:
		~ReverseConstIterator() = default;

		FORCEINLINE explicit ReverseConstIterator(T* InPtr = nullptr)
			: Ptr(InPtr)
		{
		}

		FORCEINLINE const T* operator->() const
		{
			return (Ptr - 1);
		}

		FORCEINLINE const T& operator*() const
		{
			return *(Ptr - 1);
		}

		FORCEINLINE ReverseConstIterator operator++()
		{
			Ptr--;
			return *this;
		}

		FORCEINLINE ReverseConstIterator operator++(Int32)
		{
			ConstIterator Temp = *this;
			Ptr--;
			return Temp;
		}

		FORCEINLINE ReverseConstIterator operator--()
		{
			Ptr++;
			return *this;
		}

		FORCEINLINE ReverseConstIterator operator--(Int32)
		{
			ReverseConstIterator Temp = *this;
			Ptr++;
			return Temp;
		}

		FORCEINLINE ReverseConstIterator operator+(Int32 Offset) const
		{
			ReverseConstIterator Temp = *this;
			return Temp += Offset;
		}

		FORCEINLINE ReverseConstIterator operator-(Int32 Offset) const
		{
			ReverseConstIterator Temp = *this;
			return Temp -= Offset;
		}

		FORCEINLINE ReverseConstIterator& operator+=(Int32 Offset)
		{
			Ptr -= Offset;
			return *this;
		}

		FORCEINLINE ReverseConstIterator& operator-=(Int32 Offset)
		{
			Ptr += Offset;
			return *this;
		}

		FORCEINLINE bool operator==(const ReverseConstIterator& Other) const
		{
			return (Ptr == Other.Ptr);
		}

		FORCEINLINE bool operator!=(const ReverseConstIterator& Other) const
		{
			return (Ptr != Other.Ptr);
		}

	protected:
		T* Ptr;
	};

	/*
	* Standard Reverse Iterator
	*/
	class ReverseIterator : public ReverseConstIterator
	{
		friend class TArray;

	public:
		~ReverseIterator() = default;

		FORCEINLINE explicit ReverseIterator(T* InPtr = nullptr)
			: ReverseConstIterator(InPtr)
		{
		}

		FORCEINLINE T* operator->() const
		{
			return const_cast<T*>(ReverseConstIterator::operator->());
		}

		FORCEINLINE T& operator*() const
		{
			return const_cast<T&>(ReverseConstIterator::operator*());
		}

		FORCEINLINE ReverseIterator operator++()
		{
			ReverseConstIterator::operator++();
			return *this;
		}

		FORCEINLINE ReverseIterator operator++(Int32)
		{
			ReverseIterator Temp = *this;
			ReverseConstIterator::operator++();
			return Temp;
		}

		FORCEINLINE ReverseIterator operator--()
		{
			ReverseConstIterator::operator--();
			return *this;
		}

		FORCEINLINE ReverseIterator operator--(Int32)
		{
			ReverseIterator Temp = *this;
			ReverseConstIterator::operator--();
			return Temp;
		}

		FORCEINLINE ReverseIterator operator+(Int32 Offset) const
		{
			ReverseIterator Temp = *this;
			return Temp += Offset;
		}

		FORCEINLINE ReverseIterator operator-(Int32 Offset) const
		{
			ReverseIterator Temp = *this;
			return Temp -= Offset;
		}

		FORCEINLINE ReverseIterator& operator+=(Int32 Offset)
		{
			ReverseConstIterator::operator+=(Offset);
			return *this;
		}

		FORCEINLINE ReverseIterator& operator-=(Int32 Offset)
		{
			ReverseConstIterator::operator-=(Offset);
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

	FORCEINLINE explicit TArray(SizeType InSize) noexcept
		: Data(nullptr)
		, Size(0)
		, Capacity(0)
	{
		InternalConstruct(InSize);
	}

	FORCEINLINE explicit TArray(SizeType InSize, const T& Value) noexcept
		: Data(nullptr)
		, Size(0)
		, Capacity(0)
	{
		InternalConstruct(InSize, Value);
	}

	FORCEINLINE explicit TArray(ConstIterator InBegin, ConstIterator InEnd) noexcept
		: Data(nullptr)
		, Size(0)
		, Capacity(0)
	{
		InternalConstruct(InBegin, InEnd);
	}

	FORCEINLINE TArray(std::initializer_list<T> IList) noexcept
		: Data(nullptr)
		, Size(0)
		, Capacity(0)
	{
		// TODO: Get rid of const_cast
		InternalConstruct(const_cast<T*>(IList.begin()), const_cast<T*>(IList.end()));
	}

	FORCEINLINE TArray(const TArray& Other) noexcept
		: Data(nullptr)
		, Size(0)
		, Capacity(0)
	{
		InternalConstruct(Other.Begin(), Other.End());
	}

	FORCEINLINE TArray(TArray&& Other) noexcept
		: Data(nullptr)
		, Size(0)
		, Capacity(0)
	{
		InternalMove(Move(Other));
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
		InternalDestructRange(Data, Data + Size);
		Size = 0;
	}

	FORCEINLINE void Assign(SizeType InSize) noexcept
	{
		Clear();
		InternalConstruct(InSize);
	}

	FORCEINLINE void Assign(SizeType InSize, const T& Value) noexcept
	{
		Clear();
		InternalConstruct(InSize, Value);
	}

	FORCEINLINE void Assign(ConstIterator InBegin, ConstIterator InEnd) noexcept
	{
		Clear();
		InternalConstruct(InBegin, InEnd);
	}

	FORCEINLINE void Assign(std::initializer_list<T> IList) noexcept
	{
		Clear();

		// TODO: Get rid of const_cast
		InternalConstruct(const_cast<T*>(IList.begin()), const_cast<T*>(IList.end()));
	}

	FORCEINLINE void Resize(SizeType InSize) noexcept
	{
		if (InSize > Size)
		{
			if (InSize >= Capacity)
			{
				const SizeType NewCapacity = InternalGetResizeFactor(InSize);
				InternalRealloc(NewCapacity);
			}

			InternalDefaultConstructRange(Data + Size, Data + InSize);
		}
		else if (InSize < Size)
		{
			InternalDestructRange(Data + InSize, Data + Size);
		}

		Size = InSize;
	}

	FORCEINLINE void Resize(SizeType InSize, const T& Value) noexcept
	{
		if (InSize > Size)
		{
			if (InSize >= Capacity)
			{
				const SizeType NewCapacity = InternalGetResizeFactor(InSize);
				InternalRealloc(NewCapacity);
			}

			InternalCopyEmplace(InSize - Size, Value, Data + Size);
		}
		else if (InSize < Size)
		{
			InternalDestructRange(Data + InSize, Data + Size);
		}

		Size = InSize;
	}

	FORCEINLINE void Reserve(SizeType InCapacity) noexcept
	{
		if (InCapacity != Capacity)
		{
			SizeType OldSize = Size;
			if (InCapacity < Size)
			{
				Size = InCapacity;
			}

			T* TempData = InternalAllocateElements(InCapacity);
			InternalMoveEmplace(Data, Data + Size, TempData);
			InternalDestructRange(Data, Data + OldSize);
			InternalReleaseData();

			Data = TempData;
			Capacity = InCapacity;
		}
	}

	template<typename... TArgs>
	FORCEINLINE T& EmplaceBack(TArgs&&... Args) noexcept
	{
		if (Size >= Capacity)
		{
			const SizeType NewCapacity = InternalGetResizeFactor();
			InternalRealloc(NewCapacity);
		}

		T* DataEnd = Data + Size;
		new(reinterpret_cast<void*>(DataEnd)) T(Forward<TArgs>(Args)...);
		Size++;
		return (*DataEnd);
	}

	FORCEINLINE T& PushBack(const T& Element) noexcept
	{
		return EmplaceBack(Element);
	}

	FORCEINLINE T& PushBack(T&& Element) noexcept
	{
		return EmplaceBack(Move(Element));
	}

	template<typename... TArgs>
	FORCEINLINE Iterator Emplace(ConstIterator Pos, TArgs&&... Args) noexcept
	{
		// Emplace back
		if (Pos == End())
		{
			const SizeType OldSize = Size;
			EmplaceBack(Forward<TArgs>(Args)...);
			return Iterator(Data + OldSize);
		}

		// Emplace
		const SizeType Index = InternalIndex(Pos);
		T* DataBegin = Data + Index;
		if (Size >= Capacity)
		{
			const SizeType NewCapacity = InternalGetResizeFactor();
			InternalEmplaceRealloc(NewCapacity, DataBegin, 1);
			DataBegin = Data + Index;
		}
		else
		{
			// Construct the range so that we can move to it
			T* DataEnd = Data + Size;
			InternalDefaultConstructRange(DataEnd, DataEnd + 1);
			InternalMemmoveForward(DataBegin, DataEnd, DataEnd);
			InternalDestruct(DataBegin);
		}

		new (reinterpret_cast<void*>(DataBegin)) T(Forward<TArgs>(Args)...);
		Size++;
		return Iterator(DataBegin);
	}

	FORCEINLINE Iterator Insert(ConstIterator Pos, const T& Value) noexcept
	{
		return Emplace(Pos, Value);
	}

	FORCEINLINE Iterator Insert(ConstIterator Pos, T&& Value) noexcept
	{
		return Emplace(Pos, Move(Value));
	}

	FORCEINLINE Iterator Insert(ConstIterator Pos, std::initializer_list<T> IList) noexcept
	{
		// Insert at end
		if (Pos == End())
		{
			const SizeType OldSize = Size;
			for (const T& Value : IList)
			{
				EmplaceBack(Move(Value));
			}

			return Iterator(Data + OldSize);
		}

		// Insert
		const SizeType ListSize = static_cast<SizeType>(IList.size());
		const SizeType NewSize = Size + ListSize;
		const SizeType Index = InternalIndex(Pos);

		T* RangeBegin = Data + Index;
		if (NewSize >= Capacity)
		{
			const SizeType NewCapacity = InternalGetResizeFactor(NewSize);
			InternalEmplaceRealloc(NewCapacity, RangeBegin, ListSize);
			RangeBegin = Data + Index;
		}
		else
		{
			// Construct the range so that we can move to it
			T* DataEnd = Data + Size;
			T* NewDataEnd = Data + Size + ListSize;
			T* RangeEnd = RangeBegin + ListSize;
			InternalDefaultConstructRange(DataEnd, NewDataEnd);
			InternalMemmoveForward(RangeBegin, DataEnd, NewDataEnd - 1);
			InternalDestructRange(RangeBegin, RangeEnd);
		}

		// TODO: Get rid of const_cast
		InternalMoveEmplace(const_cast<T*>(IList.begin()), const_cast<T*>(IList.end()), RangeBegin);
		Size = NewSize;
		return Iterator(RangeBegin);
	}

	FORCEINLINE void PopBack() noexcept
	{
		if (Size > 0)
		{
			InternalDestruct(Data + (--Size));
		}
	}

	FORCEINLINE Iterator Erase(Iterator Pos) noexcept
	{
		return Erase(static_cast<ConstIterator>(Pos));
	}

	FORCEINLINE Iterator Erase(ConstIterator Pos) noexcept
	{
		VALIDATE(InternalIsIteratorOwner(Pos));

		// Erase at end
		if (Pos == End())
		{
			PopBack();
			return End();
		}

		// Erase
		T* DataBegin	= Pos.Ptr;
		T* DataEnd		= Data + Size;
		InternalMemmoveBackwards(DataBegin + 1, DataEnd, DataBegin);
		InternalDestruct(DataEnd - 1);

		Size--;
		return Iterator(DataBegin);
	}

	FORCEINLINE Iterator Erase(Iterator InBegin, Iterator InEnd) noexcept
	{
		return Erase(static_cast<ConstIterator>(InBegin), static_cast<ConstIterator>(InEnd));
	}

	FORCEINLINE Iterator Erase(ConstIterator InBegin, ConstIterator InEnd) noexcept
	{
		VALIDATE(InBegin < InEnd);
		VALIDATE(InternalIsRangeOwner(InBegin, InEnd));

		T* DataBegin = InBegin.Ptr;
		T* DataEnd = InEnd.Ptr;
		const SizeType ElementCount = InternalDistance(DataBegin, DataEnd);
		if (InEnd >= End())
		{
			InternalDestructRange(DataBegin, DataEnd);
		}
		else
		{
			T* RealEnd = Data + Size;
			InternalMemmoveBackwards(DataEnd, RealEnd, DataBegin);
			InternalDestructRange(RealEnd - ElementCount, RealEnd);
		}

		Size -= ElementCount;
		return Iterator(DataBegin);
	}

	FORCEINLINE void Swap(TArray& Other) noexcept
	{
		if (this != std::addressof(Other))
		{
			T* TempPtr = Data;
			SizeType TempSize = Size;
			SizeType TempCapacity = Capacity;

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
			InternalRealloc(Size);
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

	FORCEINLINE ConstIterator ConstBegin() const noexcept
	{
		return ConstIterator(Data);
	}

	FORCEINLINE ConstIterator ConstEnd() const noexcept
	{
		return ConstIterator(Data + Size);
	}

	FORCEINLINE ReverseIterator ReverseBegin() noexcept
	{
		return ReverseIterator(Data + Size);
	}

	FORCEINLINE ReverseIterator ReverseEnd() noexcept
	{
		return ReverseIterator(Data);
	}

	FORCEINLINE ReverseConstIterator ReverseBegin() const noexcept
	{
		return ReverseConstIterator(Data + Size);
	}

	FORCEINLINE ReverseConstIterator ReverseEnd() const noexcept
	{
		return ReverseConstIterator(Data);
	}

	FORCEINLINE ReverseConstIterator ConstReverseBegin() const noexcept
	{
		return ReverseConstIterator(Data + Size);
	}

	FORCEINLINE ReverseConstIterator ConstReverseEnd() const noexcept
	{
		return ReverseConstIterator(Data);
	}

	FORCEINLINE T& GetFront() noexcept
	{
		VALIDATE(Size > 0);
		return Data[0];
	}

	FORCEINLINE const T& GetFront() const noexcept
	{
		VALIDATE(Size > 0);
		return Data[0];
	}

	FORCEINLINE T& GetBack() noexcept
	{
		VALIDATE(Size > 0);
		return Data[Size - 1];
	}

	FORCEINLINE const T& GetBack() const noexcept
	{
		VALIDATE(Size > 0);
		return Data[Size - 1];
	}

	FORCEINLINE T* GetData() noexcept
	{
		return Data;
	}

	FORCEINLINE const T* GetData() const noexcept
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

	FORCEINLINE T& GetElementAt(SizeType Index) noexcept
	{
		VALIDATE(Index < Size);
		return Data[Index];
	}

	FORCEINLINE const T& GetElementAt(SizeType Index) const noexcept
	{
		VALIDATE(Index < Size);
		return Data[Index];
	}

	FORCEINLINE TArray& operator=(const TArray& Other) noexcept
	{
		if (this != std::addressof(Other))
		{
			Clear();
			InternalConstruct(Other.Begin(), Other.End());
		}

		return *this;
	}

	FORCEINLINE TArray& operator=(TArray&& Other) noexcept
	{
		if (this != std::addressof(Other))
		{
			Clear();
			InternalReleaseData();
			InternalMove(Move(Other));
		}

		return *this;
	}

	FORCEINLINE TArray& operator=(std::initializer_list<T> IList) noexcept
	{
		Assign(IList);
		return *this;
	}

	FORCEINLINE T& operator[](SizeType Index) noexcept
	{
		return GetElementAt(Index);
	}

	FORCEINLINE const T& operator[](SizeType Index) const noexcept
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

	FORCEINLINE ConstIterator cbegin() const noexcept
	{
		return ConstIterator(Data);
	}

	FORCEINLINE ConstIterator cend() const noexcept
	{
		return ConstIterator(Data + Size);
	}

	FORCEINLINE ReverseIterator rbegin() noexcept
	{
		return ReverseIterator(Data);
	}

	FORCEINLINE ReverseIterator rend() noexcept
	{
		return ReverseIterator(Data + Size);
	}

	FORCEINLINE ReverseConstIterator rbegin() const noexcept
	{
		return ReverseConstIterator(Data);
	}

	FORCEINLINE ReverseConstIterator rend() const noexcept
	{
		return ReverseConstIterator(Data + Size);
	}

	FORCEINLINE ReverseConstIterator crbegin() const noexcept
	{
		return ReverseConstIterator(Data);
	}

	FORCEINLINE ReverseConstIterator crend() const noexcept
	{
		return ReverseConstIterator(Data + Size);
	}

private:
	// Check is the iterator belongs to this TArray
	FORCEINLINE bool InternalIsRangeOwner(ConstIterator InBegin, ConstIterator InEnd)
	{
		return (InBegin < InEnd) && (InBegin >= Begin()) && (InEnd <= End());
	}

	FORCEINLINE bool InternalIsIteratorOwner(ConstIterator It)
	{
		return (It >= Begin()) && (It <= End());
	}

	// Helpers
	FORCEINLINE SizeType InternalDistance(ConstIterator InBegin, ConstIterator InEnd)
	{
		return InternalDistance(InBegin.Ptr, InEnd.Ptr);
	}

	FORCEINLINE SizeType InternalDistance(const T* InBegin, const T* InEnd)
	{
		return static_cast<SizeType>(InEnd - InBegin);
	}

	FORCEINLINE SizeType InternalIndex(ConstIterator Pos)
	{
		return InternalIndex(Pos.Ptr);
	}

	FORCEINLINE SizeType InternalIndex(const T* Pos)
	{
		return static_cast<SizeType>(Pos - Data);
	}

	FORCEINLINE SizeType InternalGetResizeFactor() const
	{
		return InternalGetResizeFactor(Size);
	}

	FORCEINLINE SizeType InternalGetResizeFactor(SizeType BaseSize) const
	{
		return BaseSize + (Capacity)+1;
	}

	FORCEINLINE T* InternalAllocateElements(SizeType InCapacity)
	{
		constexpr SizeType ElementByteSize = sizeof(T);
		return reinterpret_cast<T*>(malloc(static_cast<size_t>(ElementByteSize) * InCapacity));
	}

	FORCEINLINE void InternalReleaseData()
	{
		if (Data)
		{
			free(Data);
		}
	}

	FORCEINLINE void InternalAllocData(SizeType InCapacity)
	{
		if (InCapacity > Capacity)
		{
			InternalReleaseData();

			Data = InternalAllocateElements(InCapacity);
			Capacity = InCapacity;
		}
	}

	FORCEINLINE void InternalRealloc(SizeType InCapacity)
	{
		T* TempData = InternalAllocateElements(InCapacity);
		InternalMoveEmplace(Data, Data + Size, TempData);
		InternalDestructRange(Data, Data + Size);
		InternalReleaseData();

		Data = TempData;
		Capacity = InCapacity;
	}

	FORCEINLINE void InternalEmplaceRealloc(SizeType InCapacity, T* EmplacePos, SizeType Count)
	{
		VALIDATE(InCapacity >= Size + Count);

		const SizeType Index = InternalIndex(EmplacePos);
		T* TempData = InternalAllocateElements(InCapacity);
		InternalMoveEmplace(Data, EmplacePos, TempData);
		if (EmplacePos != Data + Size)
		{
			InternalMoveEmplace(EmplacePos, Data + Size, TempData + Index + Count);
		}

		InternalDestructRange(Data, Data + Size);
		InternalReleaseData();

		Data = TempData;
		Capacity = InCapacity;
	}

	// Construct
	FORCEINLINE void InternalConstruct(SizeType InSize)
	{
		if (InSize > 0)
		{
			InternalAllocData(InSize);
			Size = InSize;
			InternalDefaultConstructRange(Data, Data + Size);
		}
	}

	FORCEINLINE void InternalConstruct(SizeType InSize, const T& Value)
	{
		if (InSize > 0)
		{
			InternalAllocData(InSize);
			InternalCopyEmplace(InSize, Value, Data);
			Size = InSize;
		}
	}

	FORCEINLINE void InternalConstruct(ConstIterator InBegin, ConstIterator InEnd)
	{
		const SizeType Distance = InternalDistance(InBegin, InEnd);
		if (Distance > 0)
		{
			InternalAllocData(Distance);
			InternalCopyEmplace(InBegin.Ptr, InEnd.Ptr, Data);
			Size = Distance;
		}
	}

	FORCEINLINE void InternalConstruct(T* InBegin, T* InEnd)
	{
		const SizeType Distance = InternalDistance(InBegin, InEnd);
		if (Distance > 0)
		{
			InternalAllocData(Distance);
			InternalMoveEmplace(InBegin, InEnd, Data);
			Size = Distance;
		}
	}

	FORCEINLINE void InternalMove(TArray&& Other)
	{
		Data = Other.Data;
		Size = Other.Size;
		Capacity = Other.Capacity;

		Other.Data = nullptr;
		Other.Size = 0;
		Other.Capacity = 0;
	}

	// Emplace
	FORCEINLINE void InternalCopyEmplace(const T* InBegin, const T* InEnd, T* Dest)
	{
		// This function assumes that there is no overlap
		if constexpr (std::is_trivially_copy_constructible<T>())
		{
			const SizeType Count = InternalDistance(InBegin, InEnd);
			const SizeType CpySize = Count * sizeof(T);
			memcpy(Dest, InBegin, CpySize);
		}
		else
		{
			while (InBegin != InEnd)
			{
				new(reinterpret_cast<void*>(Dest)) T(*InBegin);
				InBegin++;
				Dest++;
			}
		}
	}

	FORCEINLINE void InternalCopyEmplace(SizeType InSize, const T& Value, T* Dest)
	{
		T* ItEnd = Dest + InSize;
		while (Dest != ItEnd)
		{
			new(reinterpret_cast<void*>(Dest)) T(Value);
			Dest++;
		}
	}

	FORCEINLINE void InternalMoveEmplace(T* InBegin, T* InEnd, T* Dest)
	{
		// This function assumes that there is no overlap
		if constexpr (std::is_trivially_move_constructible<T>())
		{
			const SizeType Count = InternalDistance(InBegin, InEnd);
			const SizeType CpySize = Count * sizeof(T);
			memcpy(Dest, InBegin, CpySize);
		}
		else
		{
			while (InBegin != InEnd)
			{
				new(reinterpret_cast<void*>(Dest)) T(Move(*InBegin));
				InBegin++;
				Dest++;
			}
		}
	}

	FORCEINLINE void InternalMemmoveBackwards(T* InBegin, T* InEnd, T* Dest)
	{
		VALIDATE(InBegin <= InEnd);
		if (InBegin == InEnd)
		{
			return;
		}

		VALIDATE(InEnd <= Data + Capacity);

		// Move each object in the range to the destination
		const SizeType Count = InternalDistance(InBegin, InEnd);
		if constexpr (std::is_trivially_move_assignable<T>())
		{
			const SizeType CpySize = Count * sizeof(T);
			memmove(Dest, InBegin, CpySize); // Assumes that data can overlap
		}
		else
		{
			while (InBegin != InEnd)
			{
				if constexpr (std::is_move_assignable<T>())
				{
					(*Dest) = Move(*InBegin);
				}
				else if constexpr (std::is_copy_assignable<T>())
				{
					(*Dest) = (*InBegin);
				}

				Dest++;
				InBegin++;
			}
		}
	}

	FORCEINLINE void InternalMemmoveForward(T* InBegin, T* InEnd, T* Dest)
	{
		// Move each object in the range to the destination, starts in the "End" and moves forward
		const SizeType Count = InternalDistance(InBegin, InEnd);
		if constexpr (std::is_trivially_move_assignable<T>())
		{
			if (Count > 0)
			{
				const SizeType CpySize = Count * sizeof(T);
				const SizeType OffsetSize = (Count - 1) * sizeof(T);
				memmove(reinterpret_cast<Char*>(Dest) - OffsetSize, InBegin, CpySize);
			}
		}
		else
		{
			while (InEnd != InBegin)
			{
				InEnd--;
				if constexpr (std::is_move_assignable<T>())
				{
					(*Dest) = Move(*InEnd);
				}
				else if constexpr (std::is_copy_assignable<T>())
				{
					(*Dest) = (*InEnd);
				}
				Dest--;
			}
		}
	}

	FORCEINLINE void InternalDestruct(const T* Pos)
	{
		// Calls the destructor (If it needs to be called)
		if constexpr (std::is_trivially_destructible<T>() == false)
		{
			(*Pos).~T();
		}
	}

	FORCEINLINE void InternalDestructRange(const T* InBegin, const T* InEnd)
	{
		VALIDATE(InBegin <= InEnd);

		// Calls the destructor for every object in the range (If it needs to be called)
		if constexpr (std::is_trivially_destructible<T>() == false)
		{
			while (InBegin != InEnd)
			{
				InternalDestruct(InBegin);
				InBegin++;
			}
		}
	}

	FORCEINLINE void InternalDefaultConstructRange(T* InBegin, T* InEnd)
	{
		VALIDATE(InBegin <= InEnd);

		// Calls the default constructor for every object in the range (If it can be called)
		if constexpr (std::is_default_constructible<T>())
		{
			while (InBegin != InEnd)
			{
				new(reinterpret_cast<void*>(InBegin)) T();
				InBegin++;
			}
		}
	}

	T* Data;
	SizeType Size;
	SizeType Capacity;
};
